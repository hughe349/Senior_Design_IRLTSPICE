#include "core/netlist.hpp"
#include "core/parse.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
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

unique_ptr<RawNetlist> SpiceParser::try_parse_raw(const string &filename, string_view in) {

    RawNetlist netlist;

    Tokenizer tokenizer(in);

    vector<Token> tokens{};
    Token token = tokenizer.next();

    while (token.kind != END_OF_FILE) {
        tokens.push_back(token);
        token = tokenizer.next();
    }

    // TODO:
    // If print array
    for (Token &tok : tokens) {
        std::cout << "Kind: " << tok.kind << ", Line: " << tok.line_number << ", Val: ["
                  << tok.underlying << "]\n";
    }

    Token *tok = &tokens[0];
    vector<Token *> this_line{};
    while (tok->kind != END_OF_FILE) {
        this_line.resize(0);

        while (tok->kind == NEWLINE) {
            tok++;
        }
        if (tok->kind == END_OF_FILE)
            break;

        if (tok->kind != NORMAL) {
            throw ParseException::create(ParseException::PARSE_ERROR,
                                         string("Unexpected token: ") + get_name(token.kind) +
                                             ", Expected: NORMAL",
                                         filename, token.line_number);
        }

        bool should_continue;
        do {
            should_continue = false;
            while (tok->kind == NORMAL) {
                this_line.push_back(tok);
                tok++;
            }
            if (tok->kind == NEWLINE && (tok + 1)->kind == CONTINUATION) {
                tok = tok + 2;
                should_continue = true;
            }
        } while (should_continue);

        cout << "Line contains: [ ";
        for (const Token *t : this_line) {
            cout << '\'' << t->underlying << "', ";
        }
        cout << " ]\n";
    }

    return nullptr;
}

unique_ptr<AssignedNetlist> SpiceParser::try_assign(unique_ptr<RawNetlist> &raw) { return nullptr; }
