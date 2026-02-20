#include "core/netlist.hpp"
#include "core/parse.hpp"
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
#include <variant>

using namespace std;

bool SpiceParser::matches_filename(const string &filename) {
    return filename.ends_with(".spice") || filename.ends_with(".sp") || filename.ends_with(".cir");
};

ParseResult SpiceParser::try_parse(const std::string &filename, string_view in) {
    unique_ptr<RawNetlist> raw = this->try_parse_raw(filename, in);
    if (raw == nullptr) {
        return monostate();
    }

    unique_ptr<AssignedNetlist> assigned = this->try_assign(raw);
    if (assigned == nullptr) {
        return raw;
    } else {
        return assigned;
    }
}

ParseException ParseException::create(ParseExceptionKind kind, std::string description,
                                      std::string_view filename, uint32_t line) {
    std::ostringstream what;
    if (kind == LEX_ERROR) {
        what << "Lexing Error";
    } else if (kind == PARSE_ERROR) {
        what << "Parsing Error";
    }
    what << " at [" << filename << ":" << line << "]: " << description;
    std::string what_str = what.str();
    return ParseException(what_str);
}

// =======
// Helpers
// =======

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

// ================
// Parse Algorithms
// ================
template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

// WARN:
// If you move this out of this file it can't be string views they don't own.
typedef unordered_map<string_view, RawNetlist::vertex_descriptor> NetNameMap;

static inline void assert_net_count(int num, string_view reference, const string &filename,
                                    const vector<Token *> &line) {
    if (line.size() != num + 2) {
        throw ParseException::create(ParseException::PARSE_ERROR,
                                     string("Component: ") + string(reference) +
                                         ", has bad number of nets. Expected: " + to_string(num) +
                                         ", got: " + to_string(line.size() - 2),
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
            .kind = RawNetlistVertexInfo::NET,
            .name = string(net_name),
            .value = {.net_value = IRL_NET_IS_V_GND(net_name)
                                       ? RawNetlistVertexInfo::RawNetlistVertexValue::V_GND
                                   : IRL_NET_IS_V_HIGH(net_name)
                                       ? RawNetlistVertexInfo::RawNetlistVertexValue::V_HIGH
                                       : RawNetlistVertexInfo::RawNetlistVertexValue::WIRE}},
        netlist);

    netnames[net_name] = net;
    cout << "Added string_view\n";
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

    // NOTE:
    // If slow, we could do a regex/FA on a concatenated reference + value to find proper
    // initializer
    // This is a perfomative comment to show I thought of a cool idea to do this but was too lazy

    if (reference_kind.length() == 1) {
        switch (toupper(reference_kind[0])) {
        case 'R': {
            assert_net_count(2, reference, filename, line);
            float ohms = get_component_value(reference, filename, line);
            auto r_vert = boost::add_vertex(
                RawNetlistVertexInfo{
                    .kind = RawNetlistVertexInfo::R,
                    .name = string(reference),
                    .value = {.r_value = ohms},
                },
                netlist);

            auto pad_1 = get_or_add_net(netlist, netnames, line[1]->underlying);
            auto pad_2 = get_or_add_net(netlist, netnames, line[2]->underlying);

            boost::add_edge(r_vert, pad_1, RawNetlistEdgeInfo{.kind = RawNetlistEdgeInfo::R},
                            netlist);

            boost::add_edge(r_vert, pad_2, RawNetlistEdgeInfo{.kind = RawNetlistEdgeInfo::R},
                            netlist);
        } break;
        case 'C': {
            assert_net_count(2, reference, filename, line);
            float farads = get_component_value(reference, filename, line);
            auto r_vert = boost::add_vertex(
                RawNetlistVertexInfo{
                    .kind = RawNetlistVertexInfo::C,
                    .name = string(reference),
                    .value = {.c_value = farads},
                },
                netlist);

            auto pad_1 = get_or_add_net(netlist, netnames, line[1]->underlying);
            auto pad_2 = get_or_add_net(netlist, netnames, line[2]->underlying);

            boost::add_edge(r_vert, pad_1, RawNetlistEdgeInfo{.kind = RawNetlistEdgeInfo::C},
                            netlist);

            boost::add_edge(r_vert, pad_2, RawNetlistEdgeInfo{.kind = RawNetlistEdgeInfo::C},
                            netlist);
        } break;
        case 'U':
            break;
        default:
            throw ParseException::create(ParseException::PARSE_ERROR,
                                         "Unknown reference designator kind/letter used: " +
                                             string(reference),
                                         filename, line[0]->line_number);
            break;
        }
    }
}

unique_ptr<RawNetlist> SpiceParser::try_parse_raw(const string &filename, string_view in) {

    RawNetlist netlist;

    Tokenizer tokenizer(in);

    vector<Token> tokens{};
    for (Token token = tokenizer.next(); token.kind != END_OF_FILE; token = tokenizer.next()) {
        tokens.push_back(token);
    }

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
            add_element(netlist, netnames, line, filename);
        }
    }

    cout << "COMPONENT OVERVIEW:\n";
    for (const auto &vertex : netlist.m_vertices) {
        if (vertex.m_property.kind != RawNetlistVertexInfo::NET) {
            cout << vertex.m_property.name << ", connected to: ";
            for (const auto &edge : vertex.m_out_edges) {
                cout << netlist[edge.get_target()].name << ", ";
            }
            cout << "\n";
        }
    }
    cout << "NET OVERVIEW:\n";
    for (const auto &vertex : netlist.m_vertices) {
        if (vertex.m_property.kind == RawNetlistVertexInfo::NET) {
            if (vertex.m_out_edges.size() < 2) {
                cout << "Net: " << vertex.m_property.name << ", is unconnected";
            } else {
                cout << "Net: " << vertex.m_property.name << ", bridges: ";
                for (const auto &edge : vertex.m_out_edges) {
                    cout << netlist[edge.get_target()].name << ", ";
                }
            }
            cout << "\n";
        }
    }

    return nullptr;
}

unique_ptr<AssignedNetlist> SpiceParser::try_assign(unique_ptr<RawNetlist> &raw) { return nullptr; }
