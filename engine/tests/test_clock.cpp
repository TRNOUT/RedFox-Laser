#include <catch2/catch_test_macros.hpp>
#include "../src/safety/Clock.hpp"

using namespace redfox::safety;

TEST_CASE("FakeClock only advances when told to", "[clock]") {
    FakeClock clock;
    const auto t0 = clock.now();

    clock.advance(std::chrono::milliseconds(100));
    const auto t1 = clock.now();

    REQUIRE(t1 - t0 == std::chrono::milliseconds(100));

    const auto t2 = clock.now();
    REQUIRE(t2 == t1); // no implicit advance just from reading now()
}
