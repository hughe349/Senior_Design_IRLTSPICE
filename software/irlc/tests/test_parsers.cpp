#include "core/parse.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

TEST_CASE("Factory Identifies Parsers", "[parse]") {

    std::string filename1 = "fakefile.cir";
    std::string filename2 = "fakefile.xml";

    std::stringstream filecontents;
    filecontents.str("Blah Blah Blah");

    auto spiceparsers = AllParsersFactory::make_parsers_prioritized(filename1);

    REQUIRE(spiceparsers.size() == 2);
    REQUIRE(dynamic_cast<SpiceParser *>(spiceparsers[0]) != nullptr);
    REQUIRE(dynamic_cast<EeschemaParser *>(spiceparsers[1]) != nullptr);

    auto eeschemaparsers = AllParsersFactory::make_parsers_prioritized(filename2);

    REQUIRE(eeschemaparsers.size() == 2);
    REQUIRE(dynamic_cast<EeschemaParser *>(eeschemaparsers[0]) != nullptr);
    REQUIRE(dynamic_cast<SpiceParser *>(eeschemaparsers[1]) != nullptr);
}

TEST_CASE("Parsing Tokenizes", "[parse]") {

    std::string filename1 = "../../tests/data/User.spice";

    std::ifstream file(filename1);

    std::filesystem::path currentPath = std::filesystem::current_path();

    std::cout << currentPath << "\n";

    REQUIRE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

    auto spiceparsers = AllParsersFactory::make_parsers_prioritized(filename1);

    spiceparsers[0]->try_parse(filename1, content);

    REQUIRE(false);
}
