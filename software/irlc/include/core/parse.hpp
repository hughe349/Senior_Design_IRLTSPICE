#pragma once

#include "core/netlist.hpp"
#include <istream>
#include <vector>

// An interface for a parser that can take an input circuit into a raw netlist
class INetlistParser {
  protected:
    INetlistParser() {}

  public:
    static INetlistParser *make_if_likely_compatible(const std::string &filename,
                                                     std::istream &in) {
        return NULL;
    };

    virtual bool can_parse_raw() { return false; };
    virtual RawNetlist *parse_raw(std::istream &in) { return NULL; };

    virtual bool can_parse_assigned() { return false; };
    virtual AssignedNetlist *parse_assigned(std::istream &in) { return NULL; };
};

// Simple spice netlist format parser
class SpiceParser : INetlistParser {
  public:
    static INetlistParser *make_if_likely_compatible(const std::string &filename, std::istream &in);

    virtual bool can_parse_raw() override { return true; };
    virtual RawNetlist *parse_raw(std::istream &in) override;
};

// KiCad eeschema xml parser
class EeschemaParser : INetlistParser {
  public:
    static INetlistParser *make_if_likely_compatible(const std::string &filename, std::istream &in);

    virtual bool can_parse_raw() override { return true; };
    virtual RawNetlist *parse_raw(std::istream &in) override;

    virtual bool can_parse_assigned() override { return true; };
    virtual AssignedNetlist *parse_assigned(std::istream &in) override;
};

// Factory for making parsers
template <typename... Parsers> class ParserFactory {
  public:
    static std::vector<INetlistParser *> get_compatible_parsers(const std::string &filename,
                                                                std::istream &filein) {
        std::vector<INetlistParser *> all = std::vector<INetlistParser *>();

        (all.push_back(Parsers::make_if_likely_compatible(filename, filein)), ...);
        std::vector<INetlistParser *> out;

        std::copy_if(all.begin(), all.end(), std::back_inserter(out),
                     [](INetlistParser *parser) { return parser != NULL; });

        return out;
    }
};

typedef ParserFactory<SpiceParser, EeschemaParser> AllParsersFactory;
