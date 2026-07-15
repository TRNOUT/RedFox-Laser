#include <catch2/catch_test_macros.hpp>
#include "../src/output/MockLaserOutput.hpp"

using namespace redfox::output;

TEST_CASE("MockLaserOutput starts disarmed and ignores frames", "[output]") {
    MockLaserOutput output;
    REQUIRE_FALSE(output.isArmed());

    output.sendFrame({OutputPoint{0.1f, 0.2f, 1.0f, 0.0f, 0.0f}});
    REQUIRE(output.frameCount() == 0);
    REQUIRE(output.lastFrame().empty());
}

TEST_CASE("MockLaserOutput records frames once armed", "[output]") {
    MockLaserOutput output;
    output.setArmed(true);
    REQUIRE(output.isArmed());

    std::vector<OutputPoint> points{
        OutputPoint{0.1f, 0.2f, 1.0f, 0.0f, 0.0f},
        OutputPoint{-0.1f, -0.2f, 0.0f, 1.0f, 0.0f},
    };
    output.sendFrame(points);

    REQUIRE(output.frameCount() == 1);
    REQUIRE(output.lastFrame().size() == 2);
}

TEST_CASE("MockLaserOutput.blank clears the last frame and counts blanks", "[output]") {
    MockLaserOutput output;
    output.setArmed(true);
    output.sendFrame({OutputPoint{0.0f, 0.0f, 1.0f, 1.0f, 1.0f}});
    REQUIRE_FALSE(output.lastFrame().empty());

    output.blank();
    REQUIRE(output.lastFrame().empty());
    REQUIRE(output.blankCount() == 1);
}
