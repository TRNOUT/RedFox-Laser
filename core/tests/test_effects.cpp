#include <catch2/catch_test_macros.hpp>

#include "effects/Oscillator.hpp"
#include "show/Transform.hpp"

#include <cmath>

using namespace redfox;

namespace {
bool near(float a, float b, float eps = 0.001f) { return std::fabs(a - b) < eps; }
} // namespace

TEST_CASE("waveforms hit their characteristic values", "[effects]") {
    // Sine
    REQUIRE(near(effects::waveform(effects::Waveform::Sine, 0.0), 0.0f));
    REQUIRE(near(effects::waveform(effects::Waveform::Sine, 0.25), 1.0f));
    REQUIRE(near(effects::waveform(effects::Waveform::Sine, 0.75), -1.0f));

    // Sawtooth ramps -1 -> 1
    REQUIRE(near(effects::waveform(effects::Waveform::Sawtooth, 0.0), -1.0f));
    REQUIRE(near(effects::waveform(effects::Waveform::Sawtooth, 0.5), 0.0f));

    // Square
    REQUIRE(near(effects::waveform(effects::Waveform::Square, 0.25), 1.0f));
    REQUIRE(near(effects::waveform(effects::Waveform::Square, 0.75), -1.0f));

    // Triangle peaks at 0.5
    REQUIRE(near(effects::waveform(effects::Waveform::Triangle, 0.0), -1.0f));
    REQUIRE(near(effects::waveform(effects::Waveform::Triangle, 0.5), 1.0f));
}

TEST_CASE("an oscillator maps time to a modulated value", "[effects]") {
    effects::Oscillator osc;
    osc.shape = effects::Waveform::Sine;
    osc.frequencyHz = 1.0f;
    osc.amplitude = 0.5f;
    osc.offset = 0.5f;

    REQUIRE(near(osc.valueAt(0.0), 0.5f));   // offset + 0.5*sin(0)
    REQUIRE(near(osc.valueAt(0.25), 1.0f));  // offset + 0.5*sin(90deg)
    REQUIRE(near(osc.valueAt(0.75), 0.0f));  // offset + 0.5*(-1)
}

TEST_CASE("a translate transform moves points", "[effects]") {
    show::Transform t;
    t.offsetX = 0.2f;
    t.offsetY = -0.1f;

    const ilda::IldaPoint out =
        show::applyTransform({0.3f, 0.4f, 0.0f, 255, 128, 0, false}, t);
    REQUIRE(near(out.x, 0.5f));
    REQUIRE(near(out.y, 0.3f));
    REQUIRE(out.r == 255);
}

TEST_CASE("a scale transform scales about the origin", "[effects]") {
    show::Transform t;
    t.scale = 0.5f;
    const ilda::IldaPoint out =
        show::applyTransform({0.4f, -0.6f, 0.0f, 10, 20, 30, false}, t);
    REQUIRE(near(out.x, 0.2f));
    REQUIRE(near(out.y, -0.3f));
}

TEST_CASE("a quarter-turn rotation rotates points", "[effects]") {
    show::Transform t;
    t.rotationTurns = 0.25f; // 90 degrees CCW: (1,0) -> (0,1)
    const ilda::IldaPoint out =
        show::applyTransform({1.0f, 0.0f, 0.0f, 0, 0, 0, false}, t);
    REQUIRE(near(out.x, 0.0f));
    REQUIRE(near(out.y, 1.0f));
}

TEST_CASE("brightness scales colour and clamps, blanking is preserved", "[effects]") {
    show::Transform t;
    t.brightness = 0.5f;
    const ilda::IldaPoint lit =
        show::applyTransform({0.0f, 0.0f, 0.0f, 200, 100, 40, false}, t);
    REQUIRE(lit.r == 100);
    REQUIRE(lit.g == 50);
    REQUIRE(lit.b == 20);

    const ilda::IldaPoint blanked =
        show::applyTransform({0.0f, 0.0f, 0.0f, 255, 255, 255, true}, t);
    REQUIRE(blanked.blanked);
}
