#include "core/debug.hpp"

void debug_print_netlist(std::ostream &out, RawNetlist const &netlist) {
    out << "COMPONENT OVERVIEW:\n";
    std::pair verts = vertices(netlist);
    for (auto v = verts.first; v != verts.second; v++) {
        auto vertex = (netlist)[*v];
        if (vertex.kind != NET) {
            out << vertex.name << ", connected to: ";
            std::pair edges = out_edges(*v, netlist);
            for (auto e = edges.first; e != edges.second; e++) {
                out << (netlist)[target(*e, netlist)].name << ", ";
            }
            out << "\n";
        }
    }
    out << std::endl;
    out << "NET OVERVIEW:\n";
    for (auto v = verts.first; v != verts.second; v++) {
        auto vertex = (netlist)[*v];
        std::pair edges = out_edges(*v, netlist);
        if (vertex.kind == NET) {
            if (edges.first == edges.second || edges.first + 1 == edges.second) {
                out << "Net: " << vertex.name << ", is unconnected";
            } else {
                out << "Net: " << vertex.name << ", bridges: ";
                for (auto e = edges.first; e != edges.second; e++) {
                    out << (netlist)[target(*e, netlist)].name << ", ";
                }
            }
            out << "\n";
        }
    }
}
