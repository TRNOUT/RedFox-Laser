#include <catch2/catch_test_macros.hpp>

#include "ilda/IldaCodec.hpp"

#include <cmath>

using namespace redfox::ilda;

namespace {
bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }
} // namespace

TEST_CASE("a 2D true-colour frame round-trips through ILDA", "[ilda]") {
    IldaFrame frame;
    frame.name = "TEST";
    frame.company = "REDFOX";
    frame.points = {
        {0.5f, 0.5f, 0.0f, 255, 0, 0, false},
        {-0.5f, 0.5f, 0.0f, 0, 255, 0, false},
        {-0.5f, -0.5f, 0.0f, 0, 0, 255, true}, // blanked
        {0.5f, -0.5f, 0.0f, 255, 255, 255, false},
    };

    const std::vector<std::uint8_t> bytes = writeIlda({frame});
    const ParseResult parsed = readIlda(bytes);

    REQUIRE(parsed.ok);
    REQUIRE(parsed.frames.size() == 1);

    const IldaFrame& out = parsed.frames[0];
    REQUIRE(out.name == "TEST");
    REQUIRE(out.company == "REDFOX");
    REQUIRE(out.points.size() == 4);

    REQUIRE(near(out.points[0].x, 0.5f));
    REQUIRE(near(out.points[0].y, 0.5f));
    REQUIRE(out.points[0].r == 255);
    REQUIRE(out.points[0].g == 0);
    REQUIRE(out.points[0].b == 0);

    REQUIRE(out.points[1].g == 255);
    REQUIRE(out.points[2].blanked);
    REQUIRE(out.points[3].r == 255);
    REQUIRE(out.points[3].g == 255);
    REQUIRE(out.points[3].b == 255);
}

TEST_CASE("multiple frames round-trip and preserve order", "[ilda]") {
    IldaFrame a;
    a.name = "A";
    a.points = {{0.0f, 0.0f, 0.0f, 10, 20, 30, false}};
    IldaFrame b;
    b.name = "B";
    b.points = {
        {0.1f, 0.2f, 0.0f, 40, 50, 60, false},
        {-0.1f, -0.2f, 0.0f, 0, 0, 0, true},
    };

    const ParseResult parsed = readIlda(writeIlda({a, b}));

    REQUIRE(parsed.ok);
    REQUIRE(parsed.frames.size() == 2);
    REQUIRE(parsed.frames[0].name == "A");
    REQUIRE(parsed.frames[0].points.size() == 1);
    REQUIRE(parsed.frames[1].name == "B");
    REQUIRE(parsed.frames[1].points.size() == 2);
    REQUIRE(parsed.frames[1].points[1].blanked);
}

TEST_CASE("a 3D frame round-trips with z preserved", "[ilda]") {
    IldaFrame frame;
    frame.name = "3D";
    frame.is3D = true;
    frame.points = {
        {0.25f, -0.25f, 0.5f, 200, 100, 50, false},
        {-0.25f, 0.25f, -0.5f, 1, 2, 3, false},
    };

    const ParseResult parsed = readIlda(writeIlda({frame}));

    REQUIRE(parsed.ok);
    REQUIRE(parsed.frames.size() == 1);
    REQUIRE(parsed.frames[0].is3D);
    REQUIRE(near(parsed.frames[0].points[0].z, 0.5f));
    REQUIRE(near(parsed.frames[0].points[1].z, -0.5f));
}

TEST_CASE("garbage input is rejected cleanly", "[ilda]") {
    const std::vector<std::uint8_t> junk = {'n', 'o', 't', 'i', 'l', 'd', 'a'};
    const ParseResult parsed = readIlda(junk);
    REQUIRE_FALSE(parsed.ok);
    REQUIRE_FALSE(parsed.error.empty());
}
