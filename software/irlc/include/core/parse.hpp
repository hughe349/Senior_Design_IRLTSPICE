#pragma once

#include "core/netlist.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <variant>

typedef std::variant<std::monostate, std::unique_ptr<RawNetlist>, std::unique_ptr<AssignedNetlist>>
    ParseResult;

// An interface for a parser that can take an input circuit into a raw netlist
class INetlistParser {
  protected:
    INetlistParser() {}

  public:
    virtual ParseResult try_parse(const std::string &filename, std::string_view in) {
        return std::monostate();
    };
};

class ParseException : public std::runtime_error {
  private:
    ParseException(std::string what) : std::runtime_error(what) {}

  public:
    typedef enum parseExceptionKind { LEX_ERROR, PARSE_ERROR } ParseExceptionKind;

    static ParseException create(ParseExceptionKind kind, std::string description,
                                 std::string_view filename, uint32_t line);
};

// Simple spice netlist format parser
class SpiceParser : public INetlistParser {
    std::unique_ptr<RawNetlist> try_parse_raw(const std::string &filename, std::string_view in);
    // Either returns nullptr (and raw is still pointing to something)
    // Or returns a valid ptr (and raw has been std::move-ed into the AssignedNetlist)
    std::unique_ptr<AssignedNetlist> try_assign(std::unique_ptr<RawNetlist> &raw);

  public:
    SpiceParser() {}

    static bool matches_filename(const std::string &filename);

    virtual ParseResult try_parse(const std::string &filename, std::string_view in) override;
};

// KiCad eeschema xml parser
class EeschemaParser : public INetlistParser {
  public:
    EeschemaParser() {}

    static bool matches_filename(const std::string &filename);

    virtual ParseResult try_parse(const std::string &filename, std::string_view in) override;
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

ParseResult best_effort_parse(const std::string &file_name, std::string_view file_contents);
