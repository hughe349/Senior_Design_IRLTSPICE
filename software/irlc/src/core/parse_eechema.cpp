#include "core/netlist.hpp"
#include "core/parse.hpp"

INetlistParser *EeschemaParser::make_if_likely_compatible(const std::string &filename,
                                                          std::istream &in) {
    if (filename.ends_with(".xml")) {
        return new EeschemaParser();
    } else {
        return NULL;
    }
};

RawNetlist *EeschemaParser::parse_raw(std::istream &in) { return NULL; }

AssignedNetlist *EeschemaParser::parse_assigned(std::istream &in) { return NULL; }
