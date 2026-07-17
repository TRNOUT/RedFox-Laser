#pragma once

#include "ilda/IldaTypes.hpp"

#include <cstdint>
#include <vector>

namespace redfox::effects {

// The kind of automatic motion an effect applies to a cue's frame.
enum class EffectType : std::uint8_t {
    Rotate,     // spin the whole shape around the origin
    Wave,       // oscillate every point along one axis (shake)
    Runner,     // a bright window that travels along the point list (chase)
    Strobe,     // blank everything on an on/off duty cycle
    Pulse,      // breathe the shape's scale in and out (zoom)
    ColorCycle, // sweep a rainbow hue over time (and along the shape)
    Bounce,     // shove the whole shape along an axis with a bouncing |sin| hop
    Trace,      // progressively reveal the shape, as if drawing it on and looping
    Twinkle,    // sparkle: pseudo-randomly blank a fraction of the points
    // NOTE: only append new types here — the value is persisted as a raw byte,
    // so reordering would silently change effects in existing show files.
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
// amplitude; Runner = window width (0..1 of the shape); Strobe = duty (0..1);
// Pulse = scale depth (0..1 of shrink/grow); ColorCycle = rainbow spread along
// the shape (0 = whole shape one colour, 1 = a full rainbow across it); Bounce =
// hop height; Trace = unused (the whole shape draws on each cycle); Twinkle =
// fraction of points kept lit (1 = all on, 0 = all dark).
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
    // When set, the effect ignores its own rateHz and runs at the show's master
    // tempo (one full cycle per beat), so effects lock to the music together.
    bool syncToTempo = false;
};

// Apply one effect to `frame` in place at time `seconds`. A disabled effect is
// a no-op.
void applyEffect(ilda::IldaFrame& frame, const Effect& effect, double seconds);

// Apply a stack of effects in order (each sees the previous one's result).
void applyEffects(ilda::IldaFrame& frame, const std::vector<Effect>& effects,
                  double seconds);

} // namespace redfox::effects
