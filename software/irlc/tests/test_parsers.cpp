#include "core/parse.hpp"
#include <catch2/catch_test_macros.hpp>
#include <sstream>

TEST_CASE("Factory Identifies Parsers", "[parse]") {

    std::string filename1 = "fakefile.spice";
    std::string filename2 = "fakefile.xml";

    std::stringstream filecontents;
    filecontents.str("Blah Blah Blah");

    auto spiceparsers = AllParsersFactory::get_compatible_parsers(filename1, filecontents);

    REQUIRE(spiceparsers.size() == 1);
    REQUIRE(spiceparsers[0]->can_parse_raw() == true);
    REQUIRE(spiceparsers[0]->can_parse_assigned() == false);

    auto eeschemaparsers = AllParsersFactory::get_compatible_parsers(filename2, filecontents);

    REQUIRE(eeschemaparsers.size() == 1);
    REQUIRE(eeschemaparsers[0]->can_parse_raw() == true);
    REQUIRE(eeschemaparsers[0]->can_parse_assigned() == true);
}
