#include <catch2/catch_test_macros.hpp>
#include <core/routing.hpp>
#include <cstddef>

TEST_CASE("Routing Temporary", "[core][routing]") { REQUIRE(do_routing(NULL) == -1); }
