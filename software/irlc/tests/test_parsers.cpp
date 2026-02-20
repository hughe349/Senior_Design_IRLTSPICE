#include "core/parse.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>

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

TEST_CASE("String view hashing", "[misc]") {
    typedef std::unordered_map<std::string_view, int> NetNameMap;

    std::string str("abc 123 abc");

    auto sv1 = std::string_view(str.data(), 3);
    auto sv2 = std::string_view(str.data() + 4, str.data() + 7);
    auto sv3 = std::string_view(str.data() + 8, str.data() + 11);

    auto hash = std::hash<std::string_view>{};
    REQUIRE(hash(sv1) != hash(sv2));
    REQUIRE(hash(sv1) == hash(sv3));

    NetNameMap map{};

    map[sv1] = 1;
    map[sv2] = 2;
    map[sv3] = 3;

    REQUIRE(map[sv1] == 3);
}
