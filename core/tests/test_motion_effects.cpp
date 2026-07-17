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

TEST_CASE("Pulse breathes the shape's scale in and out", "[effects]") {
    IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};
    Effect e;
    e.type = EffectType::Pulse;
    e.rateHz = 0.25f;
    e.amount = 0.5f; // +/-50% scale
    applyEffect(f, e, 1.0); // sin(pi/2)=1 -> scale 1.5
    REQUIRE(near(f.points[0].x, 1.5f));
    REQUIRE(near(f.points[0].y, 0.0f));

    IldaFrame at0;
    at0.points = {{1.0f, 0.0f, 0.0f, 255, 255, 255, false}};
    applyEffect(at0, e, 0.0); // sin(0)=0 -> unchanged
    REQUIRE(near(at0.points[0].x, 1.0f));
}

TEST_CASE("ColorCycle sweeps a lit point's hue over time", "[effects]") {
    IldaFrame f;
    f.points = {{0.0f, 0.0f, 0.0f, 255, 0, 0, false}}; // starts red
    Effect e;
    e.type = EffectType::ColorCycle;
    e.rateHz = 1.0f;
    e.amount = 0.0f; // no spatial spread: whole shape shares one hue

    applyEffect(f, e, 1.0 / 3.0); // hue 1/3 -> green
    REQUIRE(f.points[0].r == 0);
    REQUIRE(f.points[0].g == 255);
    REQUIRE(f.points[0].b == 0);

    IldaFrame b;
    b.points = {{0.0f, 0.0f, 0.0f, 255, 0, 0, false}};
    applyEffect(b, e, 2.0 / 3.0); // hue 2/3 -> blue
    REQUIRE(b.points[0].r == 0);
    REQUIRE(b.points[0].g == 0);
    REQUIRE(b.points[0].b == 255);
}

TEST_CASE("ColorCycle leaves blanked points dark", "[effects]") {
    IldaFrame f;
    f.points = {{0.0f, 0.0f, 0.0f, 0, 0, 0, true}}; // blanked
    Effect e;
    e.type = EffectType::ColorCycle;
    e.rateHz = 1.0f;
    applyEffect(f, e, 1.0 / 3.0);
    REQUIRE(f.points[0].blanked);
    REQUIRE(f.points[0].r == 0);
    REQUIRE(f.points[0].g == 0);
    REQUIRE(f.points[0].b == 0);
}

TEST_CASE("Bounce hops the whole shape along an axis without going negative",
          "[effects]") {
    IldaFrame f;
    f.points = {{0.0f, 0.0f, 0.0f, 255, 255, 255, false},
                {0.3f, 0.0f, 0.0f, 255, 255, 255, false}};
    Effect e;
    e.type = EffectType::Bounce;
    e.axis = 1; // Y
    e.rateHz = 0.25f;
    e.amount = 0.4f;
    applyEffect(f, e, 1.0); // |sin(pi/2)| = 1 -> +0.4 on y
    REQUIRE(near(f.points[0].y, 0.4f));
    REQUIRE(near(f.points[1].y, 0.4f));

    IldaFrame half;
    half.points = {{0.0f, 0.0f, 0.0f, 255, 255, 255, false}};
    applyEffect(half, e, 2.0); // sin(pi) = 0 -> back on the floor
    REQUIRE(near(half.points[0].y, 0.0f));
}

TEST_CASE("Trace progressively reveals the shape and blanks the rest", "[effects]") {
    IldaFrame f = lineFrame(5); // pos 0, .25, .5, .75, 1 ; all lit
    Effect e;
    e.type = EffectType::Trace;
    e.direction = Direction::Forward;
    e.rateHz = 0.5f; // at t=1 -> phase 0.5

    applyEffect(f, e, 1.0);
    REQUIRE_FALSE(f.points[0].blanked); // pos 0.0 <= 0.5 -> drawn
    REQUIRE_FALSE(f.points[2].blanked); // pos 0.5 <= 0.5 -> drawn
    REQUIRE(f.points[3].blanked);       // pos 0.75 > 0.5 -> not yet
    REQUIRE(f.points[4].blanked);       // pos 1.0 > 0.5 -> not yet
}

TEST_CASE("Twinkle keeps all points at amount=1 and blanks all at amount=0",
          "[effects]") {
    Effect e;
    e.type = EffectType::Twinkle;
    e.rateHz = 4.0f;

    IldaFrame all = lineFrame(10);
    e.amount = 1.0f;
    applyEffect(all, e, 0.3);
    for (const auto& p : all.points) {
        REQUIRE_FALSE(p.blanked);
    }

    IldaFrame none = lineFrame(10);
    e.amount = 0.0f;
    applyEffect(none, e, 0.3);
    for (const auto& p : none.points) {
        REQUIRE(p.blanked);
    }
}

TEST_CASE("Twinkle is deterministic at a given time", "[effects]") {
    Effect e;
    e.type = EffectType::Twinkle;
    e.rateHz = 4.0f;
    e.amount = 0.5f;

    IldaFrame a = lineFrame(20);
    IldaFrame b = lineFrame(20);
    applyEffect(a, e, 0.7);
    applyEffect(b, e, 0.7);
    for (std::size_t i = 0; i < a.points.size(); ++i) {
        REQUIRE(a.points[i].blanked == b.points[i].blanked);
    }
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
