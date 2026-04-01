#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <core/route.hpp>
#include <cstddef>

#include "core/numbers.hpp"

TEST_CASE("Numbers", "") {
    REQUIRE(val_pico_t::from_str("1234") == val_pico_t::no_scaling(1234));
    REQUIRE(val_pico_t::from_str("1234u") == 1234_u);
    REQUIRE(val_pico_t::from_str("1235p") == 1235_p);
    REQUIRE(2000_u == 2_m);
    REQUIRE(val_pico_t::no_scaling(23).v == 23 * static_cast<uint64_t>(pow(10, 12)));

    REQUIRE_THROWS(val_pico_t::from_str("1234sdjfklsjdkfl"));
    REQUIRE_THROWS(val_pico_t::from_str("1234Z"));
}
