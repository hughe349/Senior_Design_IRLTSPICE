#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/graph_traits.hpp"
#include "boost/graph/named_function_params.hpp"
#include "boost/graph/properties.hpp"
#include "core/netlist.hpp"
#include "core/parse.hpp"
#include "util/macros.hpp"
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

bool SpiceParser::matches_filename(const string &filename) {
    return filename.ends_with(".spice") || filename.ends_with(".sp") || filename.ends_with(".cir");
};

// =================================================================================================
// Tokenization
// =================================================================================================

typedef enum tokenKind { END_OF_FILE, NORMAL, NEWLINE, CONTINUATION } TokenKind;

#define IF_NAME(var, name)                                                                         \
    if (var == name)                                                                               \
        return #name;

const char *get_name(TokenKind kind) {
    IF_NAME(kind, END_OF_FILE);
    IF_NAME(kind, NORMAL);
    IF_NAME(kind, NEWLINE);
    IF_NAME(kind, CONTINUATION);
    return "REALLY_REALLY_BAD";
}

// typedef variant<string_view, SpecialToken> Token;

typedef struct token {
    TokenKind kind;
    string_view underlying;
    uint32_t line_number;
} Token;

// Filters out comments and whitespace, leaving just newlines
struct Tokenizer {
    string_view::const_iterator current_begin;
    string_view::const_iterator current_end;
    string_view::const_iterator base_end;

    static inline bool is_newline(const string_view::value_type &val) {
        return val == '\n' || val == '\r';
    }

    static inline bool is_whitespace(const string_view::value_type &val) {
        return val == ' ' || val == '\t';
    }

    bool next_is_first_of_line;
    bool is_continuation;
    uint32_t line_number = 1;

    Tokenizer(string_view in) {
        current_begin = in.begin();
        base_end = in.end();

        // bool next_continuation_allowed = false;
        // bool next_line_commment_allowed = true;
        next_is_first_of_line = false;
        is_continuation = false;

        this->skip_to_newline();
    }

    // Next deref will give us a newline
    void skip_to_newline() {
        while (!is_newline(*current_begin) && current_begin != base_end) {
            current_begin++;
        }
        // Skip \r\n or just \n or whatever
        optionally_eat('\r');
        assert(*current_begin == '\n');

        current_end = current_begin + 1;
        next_is_first_of_line = true;
        // next_line_commment_allowed = true;
    }

    void optionally_eat(const string_view::value_type &c) {
        if (*current_begin == c) {
            current_begin++;
        }
    }

#define ADVANCE_OR_RET()                                                                           \
    do {                                                                                           \
        current_begin++;                                                                           \
        if (current_begin == base_end) {                                                           \
            return Token{END_OF_FILE};                                                             \
        };                                                                                         \
    } while (0);

    Token next() {
        current_begin = current_end;

        if (current_begin == base_end) {
            return Token{END_OF_FILE, string_view{current_begin, current_end}, line_number};
        }

        if (next_is_first_of_line) {
            line_number += 1;
            while (*current_begin == '*') {
                skip_to_newline();
                ADVANCE_OR_RET();
                line_number += 1;
            }
            if (*current_begin == '+') {
                current_end = current_begin + 1;
                next_is_first_of_line = false;
                return Token{CONTINUATION, string_view{current_begin, current_end}, line_number};
            }
        }
        next_is_first_of_line = false;

        while (is_whitespace(*current_begin)) {
            ADVANCE_OR_RET();
        }

        if (is_newline(*current_begin)) {
            // Just normal end of line
            skip_to_newline();
            return Token{NEWLINE, string_view{current_begin, current_end}, line_number};
        } else if (*current_begin == '$') {
            // Line comment
            skip_to_newline();
            return Token{NEWLINE, string_view{current_begin, current_end}, line_number};
        } else if (*current_begin == ';' && *(current_begin + 1) == ' ') {
            // Line comment
            skip_to_newline();
            return Token{NEWLINE, string_view{current_begin, current_end}, line_number};
        } else if (*current_begin == '/' && *(current_begin + 1) == '/') {
            // Line comment
            skip_to_newline();
            return Token{NEWLINE, string_view{current_begin, current_end}, line_number};
        } else {

            current_end = current_begin;
            // A NORMAL CHARACTER!!!
            while (!is_whitespace(*current_end) && !is_newline(*current_end) &&
                   current_end != base_end) {
                current_end++;
            }
        }

        return Token{NORMAL, string_view{current_begin, current_end}, line_number};
    }
};

// =================================================================================================
// Parse Algorithms
// =================================================================================================

// WARN:
// If you move this out of this file it can't be string views they don't own.
typedef unordered_map<string_view, RawNetlist::vertex_descriptor> NetNameMap;

static inline void assert_net_count(int num, string_view reference, const string &filename,
                                    const vector<Token *> &line) {
    if (line.size() != num + 2) {
        throw ParseException::create(ParseException::PARSE_ERROR,
                                     string("Component: ") + string(reference) +
                                         ", connected to bad number of nets. Expected: " +
                                         to_string(num) + ", got: " + to_string(line.size() - 2),
                                     filename, line[0]->line_number);
    }
}

static inline float get_component_value(string_view reference, const string &filename,
                                        const vector<Token *> &line) {
    Token *val_token = line[line.size() - 1];
    const char *end = val_token->underlying.end();
    float value = std::strtof(val_token->underlying.begin(), const_cast<char **>(&end));
    if (end != val_token->underlying.end()) {
        throw ParseException::create(
            ParseException::PARSE_ERROR,
            string("Component: ") + string(reference) +
                " , has non-numeric value: " + string(val_token->underlying),
            filename, val_token->line_number);
    }
    return value;
}

static inline RawNetlist::vertex_descriptor
get_or_add_net(RawNetlist &netlist, NetNameMap &netnames, string_view net_name) {

    if (netnames.contains(net_name)) {
        return netnames[net_name];
    }

    auto net = boost::add_vertex(
        RawNetlistVertexInfo{
            .kind = NET, .name = string(net_name), .value = {.net_value = get_net_kind(net_name)}},
        netlist);

    netnames[net_name] = net;
    // cout << "Added string_view\n";
    return net;
}

void add_element(RawNetlist &netlist, NetNameMap &netnames, const vector<Token *> &line,
                 const string &filename) {
    if (line.size() < 2) {
        throw ParseException::create(
            ParseException::PARSE_ERROR,
            "Malformed component line. Must have at least a reference designator and a value",
            filename, line[0]->line_number);
    }

    string_view reference = line[0]->underlying;
    auto first_num = &reference[reference.find_first_of("0123456789")];

    string_view reference_kind = string_view{reference.begin(), first_num};
    int reference_num;
    // Parse reference_num
    auto result = std::from_chars(first_num, reference.end(), reference_num);
    if (reference_kind.length() == 0 || result.ec != std::errc{} || result.ptr != reference.end() ||
        first_num == reference.end()) {
        throw ParseException::create(ParseException::PARSE_ERROR,
                                     "Malformed reference designator: " + string(reference) +
                                         ". Expected to be of form TYPE1234",
                                     filename, line[0]->line_number);
    }

    string_view value = line[line.size() - 1]->underlying;

    // NOTE:
    // If slow, we could do a regex/FA on a concatenated reference + value to find proper
    // initializer
    // This is a perfomative comment to show I thought of a cool idea to do this but was too lazy

    bool found_match = false;
    bool any_ref_found = false;

    for (const auto &recognized : RECOGNIZED_COMPONENTS) {
        bool matches_ref = false;
        for (const auto &ref : recognized.spice_prefixes) {
            if (reference_kind == ref) {
                matches_ref = true;
                any_ref_found = true;
                break;
            }
        }
        if (!matches_ref) {
            continue; // Go to next component
        }
        // Default to match if spice_values has no entries
        bool matches_val = recognized.spice_values.size() == 0;
        // this loop only gets run if spice_values has entries
        for (const auto &val : recognized.spice_values) {
            if (val == value) {
                matches_val = true;
                continue;
            }
        }
        if (!matches_val) {
            continue;
        }

        found_match = true;

        assert_net_count(recognized.pins.size(), reference, filename, line);
        RawNetlist::vertex_descriptor vert;
        if (recognized.numeric_value) {
            float val = get_component_value(reference, filename, line);
            vert = boost::add_vertex(RawNetlistVertexInfo{.kind = recognized.component_kind,
                                                          .name = string(reference),
                                                          .value =
                                                              {
                                                                  .numeric_value = val,
                                                              }},
                                     netlist);
        } else {
            vert = boost::add_vertex(RawNetlistVertexInfo{.kind = recognized.component_kind,
                                                          .name = string(reference),
                                                          .value = {.no_val = {}}},
                                     netlist);
        }

        for (int i = 0; i < recognized.pins.size(); i++) {
            auto pad = get_or_add_net(netlist, netnames, line[1 + i]->underlying);
            boost::add_edge(vert, pad, RawNetlistEdgeInfo{.kind = recognized.pins[i]}, netlist);
        }

        // We found a match, break out of the loop
        break;
    }

    if (!found_match) {
        if (!any_ref_found)
            throw ParseException::create(ParseException::PARSE_ERROR,
                                         "Unknown reference designator kind/letter used: " +
                                             string(reference),
                                         filename, line[0]->line_number);
        else
            throw ParseException::create(
                ParseException::PARSE_ERROR,
                string("Unknown component value : ") + string(value) +
                    ", used for reference designator: " + string(reference),
                filename, line[0]->line_number);
    }
}

unique_ptr<RawNetlist> SpiceParser::try_parse(const string &filename, string_view in) const {

    unique_ptr<RawNetlist> netlist(new RawNetlist);

    Tokenizer tokenizer(in);

    vector<Token> tokens{};
    Token token;
    do {
        token = tokenizer.next();
        tokens.push_back(token);
    } while (token.kind != END_OF_FILE);

    // TODO:
    // If print array
    for (Token &tok : tokens) {
        std::cout << "Kind: " << tok.kind << ", Line: " << tok.line_number << ", Val: ["
                  << tok.underlying << "]\n";
    }

    NetNameMap netnames{};

    Token *tok = &tokens[0];
    vector<Token *> line{};
    while (tok->kind != END_OF_FILE) {
        line.resize(0);

        while (tok->kind == NEWLINE) {
            tok++;
        }
        if (tok->kind == END_OF_FILE)
            break;

        if (tok->kind != NORMAL) {
            throw ParseException::create(ParseException::PARSE_ERROR,
                                         string("Unexpected token: ") + get_name(tok->kind) +
                                             ", Expected: NORMAL",
                                         filename, tok->line_number);
        }

        bool should_continue;
        do {
            should_continue = false;
            while (tok->kind == NORMAL) {
                line.push_back(tok++);
            }
            if (tok->kind == NEWLINE && (tok + 1)->kind == CONTINUATION) {
                tok = tok + 2;
                should_continue = true;
            }
        } while (should_continue);

        cout << "Line contains: [ ";
        for (const Token *t : line) {
            cout << '\'' << t->underlying << "', ";
        }
        cout << " ]\n";

        if (line[0]->underlying[0] == '.') {
            cout << "Line is a dot cmd. Skipping.\n";
        } else {
            add_element(*netlist, netnames, line, filename);
            cout << " Added!\n";
        }
    }

    cout << "COMPONENT OVERVIEW:\n";
    for (const auto &vertex : netlist->m_vertices) {
        if (vertex.m_property.kind != NET) {
            cout << vertex.m_property.name << ", connected to: ";
            for (const auto &edge : vertex.m_out_edges) {
                cout << (*netlist)[edge.get_target()].name << ", ";
            }
            cout << "\n";
        }
    }
    cout << std::endl;
    cout << "NET OVERVIEW:\n";
    for (const auto &vertex : netlist->m_vertices) {
        if (vertex.m_property.kind == NET) {
            if (vertex.m_out_edges.size() < 2) {
                cout << "Net: " << vertex.m_property.name << ", is unconnected";
            } else {
                cout << "Net: " << vertex.m_property.name << ", bridges: ";
                for (const auto &edge : vertex.m_out_edges) {
                    cout << (*netlist)[edge.get_target()].name << ", ";
                }
            }
            cout << "\n";
        }
    }

    return netlist;
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

// unique_ptr<AssignedNetlist> SpiceParser::try_assign(unique_ptr<RawNetlist> &raw) {
//
//     // Iterater over vertex_descriptors
//     auto verts = boost::vertices(*raw);
//
//     //                              .first = begin, .second = end
//     for (auto buff : ranges::subrange(verts.first, verts.second)
//                    | views::filter([&](auto x) { return (*raw)[x].kind == CELL_BUFFER;})){
//
//         auto members = vector<RawNetlist::vertex_descriptor>{};
//
//         CellAssigningVisitor::colormap cmap{};
//
//         CellAssigningVisitor vis = CellAssigningVisitor(buff, members, cmap);
//
//         auto map_p = boost::make_assoc_property_map(cmap);
//
//         // auto c = get(map_p, buff);
//         // put(map_p, buff, c);
//
//         boost::breadth_first_search(*raw, buff,
//                                     boost::visitor(vis).
//                                     color_map(map_p));
//
//         cout << "\nMembers: " << members.size() << "\nTHE CELL: ";
//         for (auto &v : members
//                      | views::filter([&](auto &v) {return (*raw)[v].kind != NET;})) {
//             cout << (*raw)[v].name << ", ";
//         }
//         cout << "\n\n";
//     }
//
//
//     return nullptr;
// }
