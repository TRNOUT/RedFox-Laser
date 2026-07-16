#include <catch2/catch_test_macros.hpp>

#include "effects/MotionEffect.hpp"

#include <cmath>

using namespace redfox::effects;
using redfox::ilda::IldaFrame;
using redfox::ilda::IldaPoint;

namespace {
bool near(float a, float b) { return std::fabs(a - b) < 1e-3f; }

IldaFrame lineFrame(int count) {
    // A horizontal line of `count` lit red points from x=-1 to x=+1.
    IldaFrame f;
    for (int i = 0; i < count; ++i) {
        const float t = count > 1 ? static_cast<float>(i) / (count - 1) : 0.0f;
        f.points.push_back({-1.0f + 2.0f * t, 0.0f, 0.0f, 255, 0, 0, false});
    }
    return f;
}
} // namespace

TEST_CASE("Rotate forward turns a point counter-clockwise", "[effects]") {
    IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};

    Effect e;
    e.type = EffectType::Rotate;
    e.direction = Direction::Forward;
    e.rateHz = 0.25f; // quarter turn per second
    applyEffect(f, e, 1.0);

    REQUIRE(near(f.points[0].x, 0.0f));
    REQUIRE(near(f.points[0].y, 1.0f)); // (1,0) rotated +90deg -> (0,1)
}

TEST_CASE("Rotate reverse turns clockwise", "[effects]") {
    IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};

    Effect e;
    e.type = EffectType::Rotate;
    e.direction = Direction::Reverse;
    e.rateHz = 0.25f;
    applyEffect(f, e, 1.0);

    REQUIRE(near(f.points[0].x, 0.0f));
    REQUIRE(near(f.points[0].y, -1.0f)); // -90deg -> (0,-1)
}

TEST_CASE("Rotate ping-pong oscillates and returns to zero at the phase origin",
          "[effects]") {
    Effect e;
    e.type = EffectType::Rotate;
    e.direction = Direction::PingPong;
    e.rateHz = 1.0f;
    e.amount = 0.25f; // amplitude of 0.25 turns

    IldaFrame at0;
    at0.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};
    applyEffect(at0, e, 0.0); // sin(0)=0 -> no rotation
    REQUIRE(near(at0.points[0].x, 1.0f));
    REQUIRE(near(at0.points[0].y, 0.0f));

    IldaFrame atPeak;
    atPeak.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};
    applyEffect(atPeak, e, 0.25); // sin(pi/2)=1 -> +0.25 turns = +90deg
    REQUIRE(near(atPeak.points[0].x, 0.0f));
    REQUIRE(near(atPeak.points[0].y, 1.0f));
}

TEST_CASE("Wave offsets all points along the chosen axis", "[effects]") {
    IldaFrame fx;
    fx.points = {{0.0f, 0.0f, 0.0f, 0, 0, 0, false},
                 {0.2f, 0.0f, 0.0f, 0, 0, 0, false}};
    Effect e;
    e.type = EffectType::Wave;
    e.axis = 0; // X
    e.amount = 0.5f;
    e.rateHz = 0.25f;
    applyEffect(fx, e, 1.0); // sin(pi/2)=1 -> +0.5 on x
    REQUIRE(near(fx.points[0].x, 0.5f));
    REQUIRE(near(fx.points[1].x, 0.7f));

    IldaFrame fy;
    fy.points = {{0.0f, 0.0f, 0.0f, 0, 0, 0, false}};
    e.axis = 1; // Y
    applyEffect(fy, e, 1.0);
    REQUIRE(near(fy.points[0].y, 0.5f));
}

TEST_CASE("Runner highlights a moving window and leaves the base lit", "[effects]") {
    IldaFrame f = lineFrame(5); // pos 0, .25, .5, .75, 1 ; all red
    Effect e;
    e.type = EffectType::Runner;
    e.direction = Direction::Forward;
    e.rateHz = 0.5f;   // at t=1 -> phase 0.5 (centre point)
    e.amount = 0.2f;   // window width 0.2 -> half 0.1
    e.r = 0;
    e.g = 255;
    e.b = 0;
    applyEffect(f, e, 1.0);

    // Only the centre point (pos 0.5) falls in the window and turns green.
    REQUIRE(f.points[2].g == 255);
    REQUIRE(f.points[2].r == 0);
    // The rest keep the base red and stay lit (the line is still there).
    REQUIRE(f.points[0].r == 255);
    REQUIRE(f.points[0].g == 0);
    REQUIRE_FALSE(f.points[0].blanked);
    REQUIRE(f.points[4].r == 255);
}

TEST_CASE("Strobe blanks everything during the off phase only", "[effects]") {
    Effect e;
    e.type = EffectType::Strobe;
    e.rateHz = 1.0f;
    e.amount = 0.5f; // 50% duty

    IldaFrame on = lineFrame(3);
    applyEffect(on, e, 0.25); // phase 0.25 < 0.5 -> on
    REQUIRE_FALSE(on.points[0].blanked);

    IldaFrame off = lineFrame(3);
    applyEffect(off, e, 0.75); // phase 0.75 >= 0.5 -> off
    REQUIRE(off.points[0].blanked);
    REQUIRE(off.points[2].blanked);
}

TEST_CASE("A disabled effect does nothing", "[effects]") {
    IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};
    Effect e;
    e.type = EffectType::Rotate;
    e.direction = Direction::Forward;
    e.rateHz = 0.25f;
    e.enabled = false;
    applyEffect(f, e, 1.0);
    REQUIRE(near(f.points[0].x, 1.0f)); // unchanged
    REQUIRE(near(f.points[0].y, 0.0f));
}

TEST_CASE("applyEffects composes a stack in order", "[effects]") {
    // Rotate 90deg then wave-shift on X: the rotated point (0,1) gains +0.5 x.
    IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};
    Effect rot;
    rot.type = EffectType::Rotate;
    rot.direction = Direction::Forward;
    rot.rateHz = 0.25f;
    Effect wave;
    wave.type = EffectType::Wave;
    wave.axis = 0;
    wave.amount = 0.5f;
    wave.rateHz = 0.25f;
    applyEffects(f, {rot, wave}, 1.0);
    REQUIRE(near(f.points[0].x, 0.5f)); // 0 + 0.5
    REQUIRE(near(f.points[0].y, 1.0f));
}
