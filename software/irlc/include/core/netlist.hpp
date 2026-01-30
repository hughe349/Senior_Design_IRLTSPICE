#pragma once

// Master docs:
// https://www.boost.org/doc/libs/latest/libs/graph/doc/table_of_contents.html

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_selectors.hpp>

struct RawNetlistVertexInfo {
    enum RawNetlistVertexKind {
        WIRE,
        INTERNAL_OPAMP,
        INTERNAL_DIODE,
    } kind;
};

struct RawNetlistEdgeInfo {
    enum RawNetlistVertexKind {
        R,
        C,
        OPAMP_PLUS,
        OPAMP_MINUS,
        OPAMP_OUT,
        Q_PNP_BASE,
        Q_PNP_EMITTER,
        Q_PNP_COLLECTOR,
        Q_NPN_BASE,
        Q_NPN_EMITTER,
        Q_NPN_COLLECTOR,
        D_K,
        D_A,
    } kind;
    union {
        int r_value;
        int c_value;
    } value;
};

// See:
// https://www.boost.org/doc/libs/latest/libs/graph/doc/using_adjacency_list.html
// for tradeoffs. By using vecS for all we get performance, but technically we
// could insert parallel edges if we aren't careful
//
// A RawNetlist contains info about all of the components, with no notion of
// standard cells. Connected wires are represented as vertecies, and components
// are edges. For asymmetric components or components with multiple pins, each
// pin is represented as an edge,
//     and a new "internal wiring" node is created.
// BGL Properties:
//      Vertex - RawNetlistVertexInfo
//      Edge - RawNetlistEdgeInfo
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, RawNetlistVertexInfo,
                              RawNetlistEdgeInfo>
    RawNetlist;
