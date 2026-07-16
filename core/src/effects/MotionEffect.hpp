#pragma once

#include "ilda/IldaTypes.hpp"

#include <cstdint>
#include <vector>

namespace redfox::effects {

// The kind of automatic motion an effect applies to a cue's frame.
enum class EffectType : std::uint8_t {
    Rotate, // spin the whole shape around the origin
    Wave,   // oscillate every point along one axis (shake)
    Runner, // a bright window that travels along the point list (chase)
    Strobe, // blank everything on an on/off duty cycle
};

// Direction of travel for the time-driven parameter. For Rotate: Forward =
// counter-clockwise, Reverse = clockwise, PingPong = oscillate ±amount turns.
// For Wave/Runner it flips the direction; PingPong makes it bounce.
enum class Direction : std::uint8_t {
    Forward,
    Reverse,
    PingPong,
};

// One procedural motion effect. Effects are evaluated live per frame from the
// playback time, so a single stored frame animates without baking. `amount` is
// interpreted per type: Rotate PingPong = turn amplitude; Wave = offset
// amplitude; Runner = window width (0..1 of the shape); Strobe = duty (0..1).
struct Effect {
    EffectType type = EffectType::Rotate;
    Direction direction = Direction::Forward;
    float rateHz = 1.0f; // cycles per second (the UI can derive this from BPM)
    float amount = 0.5f; // type-specific magnitude
    std::uint8_t axis = 0; // Wave: 0 = X, 1 = Y
    std::uint8_t r = 255;  // Runner highlight colour
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    bool enabled = true;
};

// Apply one effect to `frame` in place at time `seconds`. A disabled effect is
// a no-op.
void applyEffect(ilda::IldaFrame& frame, const Effect& effect, double seconds);

// Apply a stack of effects in order (each sees the previous one's result).
void applyEffects(ilda::IldaFrame& frame, const std::vector<Effect>& effects,
                  double seconds);

} // namespace redfox::effects
