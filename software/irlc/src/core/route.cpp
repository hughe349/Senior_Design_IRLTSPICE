#include "boost/graph/breadth_first_search.hpp"
#include "core/netlist.hpp"
#include <core/route.hpp>
#include <iostream>
#include <ranges>
#include <utility>

using namespace std;
using namespace boost;

void prune_unconnected_nets(RawNetlist &netlist) {
    pair verts = boost::vertices(netlist);
    auto current = verts.first;
    while (current != verts.second) {
        RawNetlist::vertex_descriptor prev = *current;
        current++;

        if (netlist[prev].kind == NET &&
            netlist[prev].value.net_value == RawNetlistVertexInfo::WIRE &&
            in_degree(prev, netlist) < 2) {
            clear_vertex(prev, netlist);
            remove_vertex(prev, netlist);
        }
    }
}

// Cells start white, get marked gray when in queue, then balck once finished
//
// The concept/intercae this needs to conform to
// https://www.boost.org/doc/libs/latest/libs/graph/doc/BFSVisitor.html
// Initialize Vertex 	vis.initialize_vertex(s, g) 	void 	This is invoked on every vertex of
// the graph before the start of the graph search. Discover Vertex 	vis.discover_vertex(u, g)
// void 	This is invoked when a vertex is encountered for the first time. Examine Vertex
// vis.examine_vertex(u, g) 	void 	This is invoked on a vertex as it is popped from the queue.
// This happens immediately before examine_edge() is invoked on each of the out-edges of vertex u.
// Examine Edge 	vis.examine_edge(e, g) 	void 	This is invoked on every out-edge of each
// vertex after it is discovered. Tree Edge 	vis.tree_edge(e, g) 	void 	This is invoked on
// each edge as it becomes a member of the edges that form the search tree. Non-Tree Edge
// vis.non_tree_edge(e, g) 	void 	This is invoked on back or cross edges for directed graphs
// and cross edges for undirected graphs. Gray Target 	vis.gray_target(e, g) 	void 	This is
// invoked on the subset of non-tree edges whose target vertex is colored gray at the time of
// examination. The color gray indicates that the vertex is currently in the queue. Black Target
// vis.black_target(e, g) 	void 	This is invoked on the subset of non-tree edges whose target
// vertex is colored black at the time of examination. The color black indicates that the vertex has
// been removed from the queue. Finish Vertex 	vis.finish_vertex(u, g) 	void 	This invoked
// on a vertex after all of its out edges have been added to the search tree and all of the adjacent
// vertices have been discovered (but before the out-edges of the adjacent vertices have been
// examined).
class CellAssigningVisitor : public boost::bfs_visitor<> {
  public:
    RawNetlist::vertex_descriptor out_buff;
    vector<RawNetlist::vertex_descriptor> &members;
    using colormap = std::map<RawNetlist::vertex_descriptor, boost::default_color_type>;
    colormap &vertex_coloring;

    CellAssigningVisitor(auto out_buff, auto &members, auto &colormap)
        : out_buff(out_buff), members(members), vertex_coloring(colormap) {}

    typedef boost::graph_traits<RawNetlist>::vertex_descriptor Vertex;
    typedef boost::graph_traits<RawNetlist>::edge_descriptor Edge;

    // This gets called right before the target's color is checked, so here
    // we can make all the other buffers black, and if we are going into a buffer's input we can
    // emit an error.
    void examine_edge(Edge e, const RawNetlist &g) {

        auto edge = g[e];
        auto u = boost::source(e, g);
        auto v = boost::target(e, g);
        std::cout << "Edge from [" << g[u].name << "] to [" << g[v].name << "]\n";

        // If we're going to gnd/5V don't go there
        if (g[v].kind == NET && g[v].value.net_value != RawNetlistVertexInfo::NetValue::WIRE) {
            vertex_coloring[v] = boost::default_color_type::black_color;
            return;
        }

        if (edge.kind == PIN_CELL_BUFFER_IN) {
            if (u == out_buff && g[v].kind == NET) {
                // cout << "Found a input to buffer: " << g[v].name;
                // // We don't want to go out the tip of our starter guy
                // vertex_coloring[v] = boost::default_color_type::black_color;
            } else if (g[u].kind == NET && v == out_buff) {

            } else {
                throw runtime_error("AAAHHH");
            }
        } else if (edge.kind == PIN_CELL_BUFFER_OUT) {
            // Either
            //  A) u==out_buff
            //  or
            //  B) v==another cells buffer
            // Either way we don't want to travers this
            vertex_coloring[v] = boost::default_color_type::black_color;
            // if (u == out_buff) {
            //     // We don't want to go out the tip of our starter guy
            //     vertex_coloring[v] = boost::default_color_type::black_color;
            // }
        } else {
            // Do nothing, just add it
        }

        // // boost::property_map<boost::vertex_color_t, RawNetlist>::const_type map =
        // boost::get(boost::vertex_color, g); if (g[u].kind == CELL_BUFFER && u!=out_buff) {
        //     // boost::set_property(g, boost::vertex_color,
        //     boost::color_traits<boost::default_color_type>::black());
        // }
        // std::cout << "Discovered node: " << g[u].name << ", w/ color: " << vertex_coloring[u] <<
        // "\n";
    }

    // This gets called on each white node only once (right before it becomes gray), so we can
    // hijack this to add the node to our cell members
    void discover_vertex(Vertex u, const RawNetlist &g) {
        this->members.push_back(u);
        std::cout << "Added node: " << g[u].name << " to the cell.\n";
        std::cout << "Size: " << this->members.size() << "\n";
    }
};

void try_assign(unique_ptr<RawNetlist> &raw) {

    // Iterater over vertex_descriptors
    auto verts = boost::vertices(*raw);

    //                              .first = begin, .second = end
    for (auto buff : ranges::subrange(verts.first, verts.second) |
                         views::filter([&](auto x) { return (*raw)[x].kind == CELL_BUFFER; })) {

        auto members = vector<RawNetlist::vertex_descriptor>{};

        CellAssigningVisitor::colormap cmap{};

        CellAssigningVisitor vis = CellAssigningVisitor(buff, members, cmap);

        auto map_p = boost::make_assoc_property_map(cmap);

        // auto c = get(map_p, buff);
        // put(map_p, buff, c);

        boost::breadth_first_search(*raw, buff, boost::visitor(vis).color_map(map_p));

        cout << "\nMembers: " << members.size() << "\nTHE CELL: ";
        for (auto &v : members | views::filter([&](auto &v) { return (*raw)[v].kind != NET; })) {
            cout << (*raw)[v].name << ", ";
        }
        cout << "\n\n";
    }
}
