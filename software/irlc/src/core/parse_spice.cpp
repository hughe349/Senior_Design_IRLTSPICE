#include "core/netlist.hpp"
#include "core/parse.hpp"

INetlistParser *SpiceParser::make_if_likely_compatible(const std::string &filename,
                                                       std::istream &in) {
    if (filename.ends_with(".spice")) {
        return new SpiceParser();
    } else {
        return NULL;
    }
};

RawNetlist *SpiceParser::parse_raw(std::istream &in) { return NULL; }
