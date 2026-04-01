#include "boost/graph/properties.hpp"
#include <algorithm> // for std::for_each
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream> // for std::cout
#include <utility>  // for std::pair

using namespace boost;
typedef adjacency_list<vecS, vecS, bidirectionalS> Graph;

TEST_CASE("Bost Graph Included", "[boost]") {
    // Make convenient labels for the vertices
    enum { A, B, C, D, E, N };
    const int num_vertices = N;
    const char *name = "ABCDE";

    // writing out the edges in the graph
    typedef std::pair<int, int> Edge;
    Edge edge_array[] = {Edge(A, B), Edge(A, D), Edge(C, A), Edge(D, C),
                         Edge(C, E), Edge(B, D), Edge(D, E)};
    const int num_edges = sizeof(edge_array) / sizeof(edge_array[0]);

    // declare a graph object
    Graph g(num_vertices);

    // add the edges to the graph object
    for (int i = 0; i < num_edges; ++i)
        add_edge(edge_array[i].first, edge_array[i].second, g);

    typedef graph_traits<Graph>::vertex_descriptor Vertex;

    // Parker note:
    //    Line one is just a typedef.
    //    We are defining a property map which can map a vertex/edge to a given
    //    property, in this case vertex_index. We then are getting that map for
    //    this specific graph. This get function is completely separate from the
    //    get function in a normal property map, it's just hella confusing.
    //    adjacency_list is inherits ProperyGraph, which defines this get
    //    function:
    //    https://www.boost.org/doc/libs/latest/libs/graph/doc/PropertyGraph.html
    //    The first value is only really used for its type, so vertex_index is the
    //    one and only enum member of the enum vertex_index_t. it is really only
    //    being used here so the generic can recognize we want data related to
    //    vertex_index_t. Note that this is just a tag that can be mapped to the
    //    actual vertex index type, not the actual index type, which is nuts. So
    //    now we have a property_map index which can map from vertex_descriptors
    //    to vertex_index Holy shit boost is hard. No chatgpt btw.
    //
    // get the property map for vertex indices
    typedef property_map<Graph, vertex_index_t>::type IndexMap;
    IndexMap index = get(vertex_index, g);

    std::cout << "vertices(g) = ";
    typedef graph_traits<Graph>::vertex_iterator vertex_iter;
    std::pair<vertex_iter, vertex_iter> vp;
    for (vp = vertices(g); vp.first != vp.second; ++vp.first) {
        Vertex v = *vp.first;
        IndexMap::value_type ind = index[v];
        std::cout << ind << " ";
    }
    std::cout << std::endl;
}
