#include "core/netlist.hpp"
#include "core/parse.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <variant>

using namespace std;

bool SpiceParser::matches_filename(const string &filename) {
    return filename.ends_with(".spice") || filename.ends_with(".sp") || filename.ends_with(".cir");
};

ParseResult SpiceParser::try_parse(string_view in) {
    unique_ptr<RawNetlist> raw = this->try_parse_raw(in);
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

// =======
// Helpers
// =======

typedef enum specialToken { NEWLINE, CONTINUATION } SpecialToken;

typedef variant<string_view, SpecialToken> Token;

// Filters out comments and whitespace, leaving just newlines
struct TokenIter {
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

    TokenIter(string_view in) {
        current_begin = in.begin();
        base_end = in.end();

        // bool next_continuation_allowed = false;
        // bool next_line_commment_allowed = true;
        next_is_first_of_line = false;
        is_continuation = false;

        this->skip_to_newline();
        ++*this;
    }

    // Next deref will give us a newline
    void skip_to_newline() {
        while (!is_newline(*current_begin)) {
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

    TokenIter &operator++() {
        current_begin = current_end;

        if (current_begin == base_end) {
            return *this;
        }

        if (next_is_first_of_line) {
            while (*current_begin == '*') {
                skip_to_newline();
                current_begin++;
            }
            if (*current_begin == '+') {
                is_continuation = true;
                current_end = current_begin + 1;
                return *this;
            }
        }
        next_is_first_of_line = false;
        is_continuation = false;

        while (is_whitespace(*current_begin)) {
            current_begin++;
        }

        if (is_newline(*current_begin)) {
            // Just normal end of line
            skip_to_newline();
        } else if (*current_begin == '$') {
            // Line comment
            skip_to_newline();
        } else if (*current_begin == ';' && *(current_begin + 1) == ' ') {
            // Line comment
            skip_to_newline();
        } else if (*current_begin == '/' && *(current_begin + 1) == '/') {
            // Line comment
            skip_to_newline();
        } else {

            current_end = current_begin;
            // A NORMAL CHARACTER!!!
            while (!is_whitespace(*current_end) && !is_newline(*current_end)) {
                current_end++;
            }
        }

        return *this;
    }

    Token operator*() const {
        if (*current_begin == '\n') {
            return NEWLINE;
        } else if (is_continuation) {
            return CONTINUATION;
        } else {
            return string_view(current_begin, current_end);
        }
    }

    bool is_done() { return current_begin == base_end; }
};

// ================
// Parse Algorithms
// ================
template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

unique_ptr<RawNetlist> SpiceParser::try_parse_raw(string_view in) {

    for (TokenIter tokens = TokenIter(in); !tokens.is_done(); ++tokens) {
        Token token = *tokens;
        const auto visitor = overloads{
            [](auto s) { std::cout << "NORMAL TOKEN: " << s << "\n"; },
            [](SpecialToken t) { std::cout << "SPECIAL TOKEN: " << t << "\n"; },
        };
        std::visit(visitor, token);
    }

    return nullptr;
}

unique_ptr<AssignedNetlist> SpiceParser::try_assign(unique_ptr<RawNetlist> &raw) { return nullptr; }
