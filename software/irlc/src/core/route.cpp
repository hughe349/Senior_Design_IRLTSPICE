#include "boost/assert/source_location.hpp"
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/compressed_sparse_row_graph.hpp"
#include "boost/graph/filtered_graph.hpp"
#include "boost/graph/properties.hpp"
#include "boost/unordered/unordered_map_fwd.hpp"
#include "core/board_info.hpp"
#include "core/compiler.hpp"
#include "core/netlist.hpp"
#include "core/verify.hpp"
#include "util/boost_util.hpp"
#include "util/macros.hpp"
#include <algorithm>
#include <cassert>
#include <core/route.hpp>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

using namespace std;
using namespace boost;

void SimpleTspiceRouter::prune_unconnected_nets(RawNetlist &netlist) {
    pair verts = boost::vertices(netlist);
    auto current = verts.first;
    while (current != verts.second) {
        RawVert prev = *current;
        current++;

        if (netlist[prev].kind == NET && netlist[prev].value.net_value == WIRE &&
            in_degree(prev, netlist) < 2) {
            clear_vertex(prev, netlist);
            remove_vertex(prev, netlist);
        }
    }
}

void StdCell::to_str(std::ostream &ostream, RawNetlist const raw) {
    ostream << "Cell (" << id << "):";
    ostream << "\n  Output buffer: " << raw[buffer].name;
    ostream << "\n  Nets (" << nets.size() << "): ";
    for (auto &v : nets) {
        ostream << raw[v].name << ", ";
    }
    ostream << "\n  Components (" << components.size() << "): ";
    for (auto &v : components) {
        ostream << raw[v].name << ", ";
    }
    ostream << "\n  Output net: " << raw[pre_buffer_output_net].name;
    ostream << "\n  Input nets (" << input_nets.size() << "): ";
    for (auto &v : input_nets) {
        ostream << "(" << raw[v.first].name << " = ";
        visit(overloads{
                  [&](uint32_t cid) { ostream << "Cell " << cid; },
                  [&](circuit_input_t _) { ostream << "Circuit Input"; },
                  [&](ground_input_t _) { ostream << "Ground"; },
              },
              v.second);
        ostream << "), ";
    }
    ostream << "\n";
}

StdCell const &AssignedNetlist::get_cell(RawVert v) {
    // NOTE:
    // Could just make this an unordered_map but I like saving memory :)
    for (auto &cell : cells) {
        if (cell.buffer == v) {
            return cell;
        }
    }
    throw runtime_error("AssignedNetlist::get_cell called with vert that is not a cell's buffer.");
}

/* clang-format off */
// Cells start white, get marked gray when in queue, then balck once finished
//
// See boost::breadth_first_visit in for implementation
//
// The concept/intercae this needs to conform to
// https://www.boost.org/doc/libs/latest/libs/graph/doc/BFSVisitor.html
// Described here
// ==============
// Initialize Vertex 	vis.initialize_vertex(s, g) 	void 	This is invoked on every vertex of the graph before the start of the graph search.
// Discover Vertex 	vis.discover_vertex(u, g) 	void 	This is invoked when a vertex is encountered for the first time.
// Examine Vertex 	vis.examine_vertex(u, g) 	void 	This is invoked on a vertex as it is popped from the queue. This happens immediately before examine_edge() is invoked on each of the out-edges of vertex u.
// Examine Edge 	vis.examine_edge(e, g)  	void 	This is invoked on every out-edge of each vertex after it is discovered.
// Tree Edge    	vis.tree_edge(e, g)     	void 	This is invoked on each edge as it becomes a member of the edges that form the search tree.
// Non-Tree Edge 	vis.non_tree_edge(e, g) 	void 	This is invoked on back or cross edges for directed graphs and cross edges for undirected graphs.
// Gray Target  	vis.gray_target(e, g)   	void 	This is invoked on the subset of non-tree edges whose target vertex is colored gray at the time of examination. The color gray indicates that the vertex is currently in the queue. 
// Black Target 	vis.black_target(e, g)  	void 	This is invoked on the subset of non-tree edges whose target vertex is colored black at the time of examination. The color black indicates that the vertex has been removed from the queue.
// Finish Vertex 	vis.finish_vertex(u, g) 	void 	This invoked on a vertex after all of its out edges have been added to the search tree and all of the adjacent vertices have been discovered (but before the out-edges of the adjacent vertices have been examined).
/* clang-format on */
class CellAssigningVisitor : public boost::bfs_visitor<> {
  public:
    using colormap = std::unordered_map<RawVert, boost::default_color_type>;
    using idmap = std::unordered_map<RawVert, uint32_t>;
    using prevmap = std::unordered_map<RawVert, RawVert>;

    StdCell &cell;
    colormap &vertex_coloring;
    prevmap &previous_map;
    idmap const &buffer_outputs;
    IrlCompiler const &compiler;

    CellAssigningVisitor(StdCell &cell, auto &colormap, auto &buffer_outputs, auto &previous_map,
                         IrlCompiler const &compiler)
        : cell(cell), vertex_coloring(colormap), buffer_outputs(buffer_outputs),
          previous_map(previous_map), compiler(compiler) {}

    // This gets called right before the target's color is checked, so here
    // we can make all the other buffers black, and if we are going into a buffer's input we can
    // emit an error.
    void examine_edge(RawEdge e, const RawNetlist &g) {

        auto edge = g[e];
        auto u = boost::source(e, g);
        auto v = boost::target(e, g);
        if (compiler.opts.should_verbose_cell_assign())
            compiler.log_fd << "Edge from [" << g[u].name << "] to [" << g[v].name << "]\n";

        // If we are starting...
        //   if we're leaving out the input of the buffer we want to do normal stuff.
        //   But, we want to prevent leaving out the output.
        if (u == cell.buffer && g[e].kind == PIN_CELL_BUFFER_OUT) {
            assert(g[v].kind == NET);
            // We mark this as black, but if we have a circuit that loops back it should still get
            // visited later
            vertex_coloring[v] = boost::black_color;
            // At that point it will be recognized as a buffer output and added to the cell's inputs
            assert(buffer_outputs.contains(v));
        } else if (g[v].kind == NET) {
            // Here we are going to a net. Lots of special cases here.
            switch (g[v].value.net_value) {
            case V_HIGH:
            case V_NEG: {
                if (compiler.opts.should_verbose_cell_assign())
                    compiler.log_fd << "  V\n";
                if (buffer_outputs.contains(v)) {
                    rule_failed<"Buffer must not drive voltage">();
                }
                // Power is illegal in the user's circuit
                // No matter what, we don't want to visit it as that could breach to other cells
                vertex_coloring[v] = boost::black_color;
                // Edges allowed to be connected to power
                constexpr NetlistEdgeKind power_edges[] = {PIN_OPAMP_SUPPLY_PLUS,
                                                           PIN_OPAMP_SUPPLY_MINUS};
                if (ranges::find(power_edges, g[e].kind) != ranges::end(power_edges)) {
                    // A power edge. We chill.
                    return;
                } else {
                    // Not a power edge. Very illegal
                    throw runtime_error(std::format("cell {} has component: \"{}\" connected to "
                                                    "{}. Illegal connection to power",
                                                    cell.id, g[u].name,
                                                    net_name(g[v].value.net_value)));
                }
            } break;
            case V_GND:
                if (compiler.opts.should_verbose_cell_assign())
                    compiler.log_fd << "  GND\n";
                if (buffer_outputs.contains(v)) {
                    rule_failed<"Buffer must not drive voltage">();
                }
                // We do want ground as a net, because it is a valid row in the crossbars
                if (vertex_coloring[v] == boost::white_color) {
                    cell.nets.push_back(v);
                    cell.input_nets.push_back(pair{v, GROUND_INPUT});
                }
                // But we don't want to visit it, because it leads to other cells
                vertex_coloring[v] = boost::black_color;
                break;
            case INPUT:
                if (compiler.opts.should_verbose_cell_assign())
                    compiler.log_fd << "  INPUT\n";
                if (buffer_outputs.contains(v)) {
                    rule_failed<"Buffer must not drive voltage">();
                }
                // But we do want input as a net, because it is a valid row in the crossbars
                // And we need it to be in input_nets, so we can route it in the top level switch
                if (vertex_coloring[v] == boost::white_color) {
                    cell.nets.push_back(v);
                    cell.input_nets.push_back(pair{v, CIRCUIT_INPUT});
                }
                // We don't want to visit it, though, because it leads to other cells
                vertex_coloring[v] = boost::black_color;
                break;
            case OUTPUT:
                // Since the output is driven by a cell, we treat it as just another cell output to
                // be routed back to us through the main switch. Thus we can fall through and re-use
                // the wire logic.
                if (!buffer_outputs.contains(v)) {
                    rule_failed<"Circuit output must be driven by only a buffer">();
                }
            case WIRE:
                if (vertex_coloring[v] == boost::white_color) {
                    // Add this guy to our nets!
                    cell.nets.push_back(v);
                    // If it's the output of another cell we don't want to visit it cuz it could
                    // feed into other cells We also need to mark it as an input to this cell.
                    auto other_out = buffer_outputs.find(v);
                    if (other_out != buffer_outputs.end()) {
                        vertex_coloring[v] = boost::black_color;
                        uint32_t other_id = (*other_out).second;
                        cell.input_nets.push_back(pair{v, other_id});
                    }
                }
                break;
            }
            return;
        } else {
            // We are coming from a net to a component
            // We should only bfs into wires. If this failed then the net branch in this big if else
            // statement is messed up and failed to mark it as black.
            assert(g[u].value.net_value == WIRE);
            switch (g[v].kind) {
            case R:
            case C:
            case DIODE:
            case CUSTOM:
                // Simple passive. We just add and go.
                if (vertex_coloring[v] == boost::white_color)
                    cell.components.push_back(v);
                break;
            case OPAMP:
                // If we are coming in through v+ or v- we fucked up.
                if (g[e].kind == PIN_OPAMP_SUPPLY_PLUS) {
                    rule_failed<"Opamp supply plus must be V_HIGH or unconnectd">();
                }
                if (g[e].kind == PIN_OPAMP_SUPPLY_MINUS) {
                    rule_failed<"Opamp supply minus must be V_NEG or unconnectd">();
                }
                // Otherwise just add and go!
                if (vertex_coloring[v] == boost::white_color)
                    cell.components.push_back(v);
                break;
            case CELL_BUFFER:
                if (vertex_coloring[v] == boost::white_color) {
                    // This is terrible. We should not get here.
                    // Always a terrible terrible bad day.
                    if (g[e].kind == PIN_CELL_BUFFER_OUT) {
                        if (!buffer_outputs.contains(u)) {
                            throw runtime_error(string("Somehow, ") + g[u].name +
                                                " is not in the buffer outputs set, but it targets "
                                                "a cell buffer with an out edge.");
                        } else {
                            throw runtime_error(string("Even though") + g[u].name +
                                                " is a cell output it still got visited.");
                        }
                    } else {
                        assert(g[e].kind == PIN_CELL_BUFFER_IN);
                        // This just means they messed up their circuit.
                        // Emit a normal error
                        ostringstream bad_chain(
                            "Illegal connection between the inputs of two cell output buffers: ");
                        bad_chain << g[v].name;
                        auto current = u;
                        while (previous_map.contains(u)) {
                            bad_chain << ", " << g[u].name;
                            u = previous_map[u];
                        }
                        bad_chain << ", " << g[cell.buffer].name;
                        throw runtime_error(bad_chain.str());
                    }
                }
                break;
            case Q_PNP:
            case Q_NPN:
                throw runtime_error("TODO");
                break;
            case NET:
                __builtin_unreachable();
                break;
            }
        }
    }

    // Here to record the chain of how we got to this node so we can print a nice message :)
    void tree_edge(RawEdge e, const RawNetlist &g) { previous_map[target(e, g)] = source(e, g); }
};

unique_ptr<AssignedNetlist> SimpleTspiceRouter::try_assign(unique_ptr<RawNetlist> &raw) {

    unique_ptr<AssignedNetlist> assigned(new AssignedNetlist{});
    assigned->cells = vector<StdCell>{};

    auto cell_buffs_iter = pair_to_iter(boost::vertices(*raw)) |
                           views::filter([&](auto x) { return (*raw)[x].kind == CELL_BUFFER; });

    // Collect so we can iter twice.
    //  Once to pre-populate cell->buffer fields, once to do rest of fields.
    //  1st needed cuz giving input_nets a cell_id needs to know which buffer is which cell
    vector<RawVert> cell_buffs(cell_buffs_iter.begin(), cell_buffs_iter.end());
    std::unordered_map<RawVert, uint32_t> buffer_outputs{};

    // Enumerate does not exist in C++20 i'm gonna kill myself this language is actually such a
    // shitshow. In 20 they had the beautiful rust iterators to base this off of and they couldn't
    // even implement collect, enumerate, or zip. So so dogshit. I should not have to write a for
    // loop that's incrementing like this in 2026. It's not even rust it's just any language with a
    // half-decent iterator system. Lowkey should use Boost but the documentation is so rough in BGL
    // so I'm too wary.
    uint32_t i = 0;
    for (RawVert buff : cell_buffs) {
        assigned->cells.push_back(StdCell{
            .id = i,
            .buffer = buff,
        });
        for (RawEdge e : pair_to_iter(out_edges(buff, *raw))) {
            if ((*raw)[e].kind == PIN_CELL_BUFFER_OUT) {
                buffer_outputs[target(e, *raw)] = i;
                if ((*raw)[target(e, *raw)].value.net_value == OUTPUT) {
                    if (assigned->output_cell != UINT32_MAX) {
                        rule_failed<"Circuit output must be driven by only a buffer">();
                    }
                    assigned->output_cell = i;
                    if (compiler.opts.should_verbose_cell_assign()) {
                        compiler.log_fd << "Cell id: " << i << ", is the output cell\n";
                    }
                }
            } else {
                assert((*raw)[e].kind == PIN_CELL_BUFFER_IN);
                assigned->cells[i].pre_buffer_output_net = target(e, *raw);
            }
        }
        i++;
    }

    CellAssigningVisitor::prevmap pmap{};
    CellAssigningVisitor::colormap cmap{};

    for (StdCell &cell : assigned->cells) {

        if (compiler.opts.should_verbose_cell_assign())
            compiler.log_fd << "Assigning cell: " << cell.id
                            << ", which starts at: " << (*raw)[cell.buffer].name << "\n";

        cell.nets = vector<RawVert>{};
        cell.components = vector<RawVert>{};

        pmap.clear();
        cmap.clear();
        CellAssigningVisitor vis = CellAssigningVisitor(cell, cmap, buffer_outputs, pmap, compiler);

        boost::breadth_first_search(
            *raw, cell.buffer, boost::visitor(vis).color_map(boost::make_assoc_property_map(cmap)));

        if (compiler.opts.should_verbose_cell_assign())
            cell.to_str(compiler.log_fd, *raw);
    }

    // Only after success can we take ownership of raw
    assigned->raw_list = std::move(raw);
    raw = nullptr;

    return assigned;
}

class RoutingError : public runtime_error {
  public:
    RoutingError(int32_t cell_id, const std::string &what)
        : runtime_error(
              (std::ostringstream("Error routing cell: ") << cell_id << ", message: " << what)
                  .str()) {}
};

vector<CrossbarCon> SimpleTspiceRouter::make_connections(unique_ptr<AssignedNetlist> &assigned) {
    vector<CrossbarCon> connections{};
    // Step 1: Net allocation. which rows are used for which nets
    // We first make a map of which root rows are free
    vector<bool> free_root = vector(board.root.rows().size(), true);
    // And one for which std cell rows are free
    vector<vector<bool>> free_cells;
    free_cells.resize(board.cells.size());
    for (auto const &cell : board.cells) {
        free_cells[cell.id] = vector(cell.crossbars.rows().size(), true);
    }

    // First, a non-optional connection is the output cell's output to the circuit output.
    {
        // Find the output column
        ColConIter output_col =
            std::find_if(board.root.cols_begin(), board.root.cols_end(), [](const ColCon &con) {
                if (SpecialNetCon const *spnet = std::get_if<SpecialNetCon>(&con)) {
                    if (spnet->kind == OUTPUT) {
                        return true;
                    }
                }
                return false;
            });
        if (output_col == ColConIter()) {
            throw RoutingError(-1, "Top level crossbar has no circuit output column");
        }
        // Find the top-level row that corresponds to the output cell's output
        assert(assigned->output_cell != UINT32_MAX);
        auto input_row = std::find_if(
            board.root.rows().begin(), board.root.rows().end(), [&assigned](RowCon const &con) {
                return std::visit(overloads{[&assigned](BufferOutputRowCon b) {
                                                return b.id == assigned->output_cell;
                                            },
                                            [](auto a) { return false; }},
                                  con);
            });

        connections.push_back(CrossbarCon{
            .crossbar_id = output_col.get_bar_phys_id(),
            .row = static_cast<uint8_t>(input_row - board.root.rows().begin()),
            .col = output_col.get_bar_col(),
        });
        if (compiler.opts.should_verbose_connections()) {
            // Dumbass language needs a cast or gets formatted as ascii
            compiler.log_fd << "Output connection is Bar: " << (size_t)output_col.get_bar_phys_id()
                            << ", Row: " << input_row - board.root.rows().begin()
                            << ", Col: " << (size_t)output_col.get_bar_col() << "\n";
        }
    }

    // Now we need to make the map of where nets are actually physically located
    // This map is a temporary for each cell.
    // We populate it with nets, then go thru the components and connect them to their nets
    std::unordered_map<RawVert, size_t> netmap{};
    size_t i = -1;
    for (auto const &cell : assigned->cells) {
        i++;
        auto const &phy_cell = board.cells[i];
        netmap.clear();
        // Priority 1 is the output. Locate it on the standard cell
        for (RowCon const &row : phy_cell.crossbars.rows()) {
            int row_i = &row - &phy_cell.crossbars.rows()[0];
            if (std::holds_alternative<BufferInputRowCon>(row)) {
                // No for-else in c++ hence goto. Also no enumerate hence the pointer arithmetic.
                // dogshit language.
                assert(free_cells[i][row_i]);
                netmap[cell.pre_buffer_output_net] = row_i;
                free_cells[i][row_i] = false;
                goto found_output;
            }
        }
        throw RoutingError(i, "No output row on crossbar");
    found_output:
        // Priority 2 is cell inputs.
        for (pair<RawVert, CellInputId> const &input : cell.input_nets) {
            bool we_good = std::visit(overloads{
                                          [&](ground_input_t _) {
                                              // If this is a ground input, just find the special
                                              // ground in the crossbars
                                              for (RowCon const &row : phy_cell.crossbars.rows()) {
                                                  int row_i = &row - &phy_cell.crossbars.rows()[0];
                                                  if (SpecialNetCon const *con =
                                                          std::get_if<SpecialNetCon>(&row);
                                                      con && con->kind == V_GND) {
                                                      assert(free_cells[i][row_i]);
                                                      netmap[cell.pre_buffer_output_net] = row_i;
                                                      free_cells[i][row_i] = false;
                                                      return true;
                                                  }
                                              }
                                              return false;
                                          },

                                          // If it ain't a ground input we're in big trouble. We
                                          // gotta find where it enters the top level crossbar and
                                          // then route that to this place by pushing a connection

                                          [&](circuit_input_t _) { return false; },
                                          [&](uint32_t input_cell) { return false; },
                                      },
                                      input.second);

            if (!we_good)
                throw RoutingError(i, "No input row compatible with input: " +
                                          (*assigned->raw_list)[input.first].name);
        }
    }

    return connections;
}
