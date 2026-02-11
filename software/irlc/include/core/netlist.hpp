#pragma once

// Master docs:
// https://www.boost.org/doc/libs/latest/libs/graph/doc/table_of_contents.html

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <sys/types.h>
#include <utility>
#include <vector>

// Netlists are represented as a bipartite graph, where the left side
//   consists of vertecies representing nets and the right side consists
//   of vertecies representing components.
//   Each edge has a type corresponding to which pin of the component
//     it is.
//   Each vertex contains metadata about the component/net, i.e.
//     if it is GND/5V or an OpAmp or what resistance it should be

struct RawNetlistVertexInfo {
    enum RawNetlistVertexKind {
        NET,
        R,
        C,
        OPAMP,
        DIODE,
        Q_PNP,
        Q_NPN,
    } kind;
    union RawNetlistVertexValue {
        uint r_value;
        uint c_value;
        enum RawNetlistNetValue {
            WIRE,
            V_LOW,
            V_HIGH,
        } net_value;
    } value;
};

struct RawNetlistEdgeInfo {
    enum RawNetlistEdgeKind {
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
};

// See:
// https://www.boost.org/doc/libs/latest/libs/graph/doc/using_adjacency_list.html
// for tradeoffs. By using vecS for all we get performance, but technically we
// could insert parallel edges if we aren't careful
//
// A RawNetlist contains info about all of the components, with no notion of
// standard cells.
// BGL Properties:
//      Vertex - RawNetlistVertexInfo
//      Edge - RawNetlistEdgeInfo
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, RawNetlistVertexInfo,
                              RawNetlistEdgeInfo>
    RawNetlist;

// An AssignedNetList has assigned each component (not each net) to std cells
// The raw_list should not be modified, as this struct may hold references to it.
typedef struct _AssignedNetList {
    const RawNetlist raw_list;
    uint std_cell_count;
    // A list of pairs mapping (std_cell_id, component_vertex)
    std::vector<std::pair<uint, RawNetlist::vertex_descriptor>> std_cell_assignments;
} AssignedNetlist;
