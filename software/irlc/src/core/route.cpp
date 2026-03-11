#include "boost/assert/source_location.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/compressed_sparse_row_graph.hpp"
#include "boost/graph/filtered_graph.hpp"
#include "boost/graph/properties.hpp"
#include "boost/unordered/unordered_map_fwd.hpp"
#include "core/netlist.hpp"
#include "core/verify.hpp"
#include "util/boost_util.hpp"
#include "util/macros.hpp"
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

void prune_unconnected_nets(RawNetlist &netlist) {
    pair verts = boost::vertices(netlist);
    auto current = verts.first;
    while (current != verts.second) {
        RawVert prev = *current;
        current++;

        if (netlist[prev].kind == NET &&
            netlist[prev].value.net_value == RawNetlistVertexInfo::WIRE &&
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
    for (auto &cell : this->cells) {
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

    CellAssigningVisitor(StdCell &cell, auto &colormap, auto &buffer_outputs, auto &previous_map)
        : cell(cell), vertex_coloring(colormap), buffer_outputs(buffer_outputs),
          previous_map(previous_map) {}

    // This gets called right before the target's color is checked, so here
    // we can make all the other buffers black, and if we are going into a buffer's input we can
    // emit an error.
    void examine_edge(RawEdge e, const RawNetlist &g) {

        auto edge = g[e];
        auto u = boost::source(e, g);
        auto v = boost::target(e, g);
        std::cout << "Edge from [" << g[u].name << "] to [" << g[v].name << "]\n";

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
            case RawNetlistVertexInfo::V_HIGH:
            case RawNetlistVertexInfo::V_NEG: {
                cout << "  V\n";
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
            case RawNetlistVertexInfo::V_GND:
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
            case RawNetlistVertexInfo::INPUT:
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
            case RawNetlistVertexInfo::OUTPUT:
                // Since the output is driven by a cell, we treat it as just another cell output to
                // be routed back to us through the main switch. Thus we can fall through and re-use
                // the wire logic.
                if (!buffer_outputs.contains(v)) {
                    rule_failed<"Circuit output must be driven by only a buffer">();
                }
            case RawNetlistVertexInfo::WIRE:
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
            assert(g[u].value.net_value == RawNetlistVertexInfo::WIRE);
            switch (g[v].kind) {
            case R:
            case C:
            case DIODE:
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

unique_ptr<AssignedNetlist> try_assign(unique_ptr<RawNetlist> &raw) {

    unique_ptr<AssignedNetlist> assigned(new AssignedNetlist);
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

        cout << "Assigning cell: " << cell.id << ", which starts at: " << (*raw)[cell.buffer].name
             << "\n";

        cell.nets = vector<RawVert>{};
        cell.components = vector<RawVert>{};

        pmap.clear();
        cmap.clear();
        CellAssigningVisitor vis = CellAssigningVisitor(cell, cmap, buffer_outputs, pmap);

        boost::breadth_first_search(
            *raw, cell.buffer, boost::visitor(vis).color_map(boost::make_assoc_property_map(cmap)));

        cell.to_str(cout, *raw);
    }

    // Only after success can we take ownership of raw
    assigned->raw_list = std::move(raw);
    raw = nullptr;

    return assigned;
}
