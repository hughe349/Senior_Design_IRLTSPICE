#pragma once

#include "core/netlist.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>

// An interface for a parser that can take an input circuit into a raw netlist
class INetlistParser {
  protected:
    INetlistParser() {}

  public:
    virtual std::unique_ptr<RawNetlist> try_parse(const std::string &filename,
                                                  std::string_view in) = 0;
};

class ParseException : public std::runtime_error {
  private:
    ParseException(std::string what) : std::runtime_error(what) {}

  public:
    typedef enum parseExceptionKind { LEX_ERROR, PARSE_ERROR } ParseExceptionKind;

    static inline ParseException create(ParseExceptionKind kind, std::string description,
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
    };
};

// Simple spice netlist format parser
class SpiceParser : public INetlistParser {
  public:
    SpiceParser() {}

    static bool matches_filename(const std::string &filename);

    virtual std::unique_ptr<RawNetlist> try_parse(const std::string &filename,
                                                  std::string_view in) override;
};

// KiCad eeschema xml parser
class EeschemaParser : public INetlistParser {
  public:
    EeschemaParser() {}

    static bool matches_filename(const std::string &filename);

    virtual std::unique_ptr<RawNetlist> try_parse(const std::string &filename,
                                                  std::string_view in) override;
};

// Factory for making parsers
template <typename... Parsers> class ParserFactory {
  public:
    // Gets the parsers in a prioritized list based on the terrible filename heuristic
    // Parser 0 is best, parser 1 second best, etc
    static std::array<INetlistParser *, sizeof...(Parsers)>
    make_parsers_prioritized(const std::string &filename) {
        std::array<INetlistParser *, sizeof...(Parsers)> all{};

        INetlistParser **good_ind = &all[0];
        INetlistParser **bad_ind = &all[all.size() - 1];

        (
            [&]() {
                bool parser_good = Parsers::matches_filename(filename);
                if (parser_good) {
                    std::cout << "good\n";
                    *good_ind = new Parsers();
                    good_ind++;
                } else {
                    std::cout << "bad\n";
                    *bad_ind = new Parsers();
                    bad_ind--;
                }
            }(),
            ...);

        return all;
    }
};

typedef ParserFactory<EeschemaParser, SpiceParser> AllParsersFactory;
