#include "core/netlist.hpp"
#include "core/parse.hpp"
#include <memory>
#include <stdexcept>
#include <variant>

using namespace std;

bool EeschemaParser::matches_filename(const string &filename) {
    return filename.ends_with(".xml");
};

unique_ptr<RawNetlist> EeschemaParser::try_parse(const std::string &filename, string_view in) {
    throw runtime_error("Eeschema parsing not yet implemented.");
}
