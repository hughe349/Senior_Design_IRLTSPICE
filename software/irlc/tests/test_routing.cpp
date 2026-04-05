#include <catch2/catch_test_macros.hpp>
#include <core/board_info.hpp>
#include <core/route.hpp>
#include <cstddef>

TEST_CASE("Routing Temporary", "[core][routing]") { REQUIRE(true); }

TEST_CASE("Valid Board Info", "[core][routing]") { REQUIRE(validate_simple_tspice(MAIN_BOARD)); }
