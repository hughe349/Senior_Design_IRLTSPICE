#include "boost/graph/detail/adjacency_list.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "core/netlist.hpp"
#include <core/route.hpp>
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
