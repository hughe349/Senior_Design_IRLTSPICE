#include "core/parse.hpp"
#include <variant>

using namespace std;

bool EeschemaParser::matches_filename(const string &filename) {
    return filename.ends_with(".xml");
};

ParseResult EeschemaParser::try_parse(string_view in) { return monostate(); }
