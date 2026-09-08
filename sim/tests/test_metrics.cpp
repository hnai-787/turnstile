#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "schedsim/metrics.hpp"

using namespace schedsim;

TEST_CASE("Jain index is 1.0 for equal values", "[metrics]") {
    REQUIRE(jainIndex({10.0, 10.0, 10.0, 10.0}) == Catch::Approx(1.0));
}

TEST_CASE("Jain index is 1/n for one task monopolizing service", "[metrics]") {
    // classic Jain worst case: all service to one of n tasks
    double result = jainIndex({100.0, 0.0, 0.0, 0.0});
    REQUIRE(result == Catch::Approx(0.25));
}

TEST_CASE("Jain index handles the empty/all-zero edge cases without dividing by zero", "[metrics]") {
    REQUIRE(jainIndex({}) == Catch::Approx(1.0));
    REQUIRE(jainIndex({0.0, 0.0}) == Catch::Approx(1.0));
}

TEST_CASE("Jain index for two unequal but nonzero values is between 1/n and 1", "[metrics]") {
    double result = jainIndex({30.0, 10.0});
    REQUIRE(result < 1.0);
    REQUIRE(result > 0.5);
}
