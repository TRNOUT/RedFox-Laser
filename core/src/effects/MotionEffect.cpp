#include "effects/MotionEffect.hpp"

#include <cmath>

namespace redfox::effects {

namespace {

constexpr double kTwoPi = 6.283185307179586;

// Fractional part in [0,1).
double frac(double x) { return x - std::floor(x); }

// A 0..1 triangle that goes 0 -> 1 -> 0 over one period, for ping-pong travel.
double triangle01(double phase) {
    const double p = frac(phase);
    return p < 0.5 ? p * 2.0 : 2.0 - p * 2.0;
}

void applyRotate(ilda::IldaFrame& frame, const Effect& e, double t) {
    double angle = 0.0;
    switch (e.direction) {
        case Direction::Forward:
            angle = kTwoPi * e.rateHz * t;
            break;
        case Direction::Reverse:
            angle = -kTwoPi * e.rateHz * t;
            break;
        case Direction::PingPong:
            angle = kTwoPi * e.amount * std::sin(kTwoPi * e.rateHz * t);
            break;
    }
    const float c = static_cast<float>(std::cos(angle));
    const float s = static_cast<float>(std::sin(angle));
    for (ilda::IldaPoint& p : frame.points) {
        const float x = p.x;
        const float y = p.y;
        p.x = x * c - y * s;
        p.y = x * s + y * c;
    }
}

void applyWave(ilda::IldaFrame& frame, const Effect& e, double t) {
    float offset = e.amount * static_cast<float>(std::sin(kTwoPi * e.rateHz * t));
    if (e.direction == Direction::Reverse) {
        offset = -offset;
    }
    for (ilda::IldaPoint& p : frame.points) {
        if (e.axis == 0) {
            p.x += offset;
        } else {
            p.y += offset;
        }
    }
}

void applyRunner(ilda::IldaFrame& frame, const Effect& e, double t) {
    double phase = 0.0;
    switch (e.direction) {
        case Direction::Forward:
            phase = frac(e.rateHz * t);
            break;
        case Direction::Reverse:
            phase = frac(-e.rateHz * t);
            break;
        case Direction::PingPong:
            phase = triangle01(e.rateHz * t);
            break;
    }
    const double halfWidth = e.amount * 0.5;
    const std::size_t n = frame.points.size();
    for (std::size_t i = 0; i < n; ++i) {
        const double pos = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
        double d = std::fabs(pos - phase);
        d = std::min(d, 1.0 - d); // wrap around the ends of the shape
        if (d <= halfWidth) {
            frame.points[i].r = e.r;
            frame.points[i].g = e.g;
            frame.points[i].b = e.b;
        }
    }
}

void applyStrobe(ilda::IldaFrame& frame, const Effect& e, double t) {
    // On for the first `amount` fraction of each cycle, blanked for the rest.
    const double phase = frac(e.rateHz * t);
    if (phase >= e.amount) {
        for (ilda::IldaPoint& p : frame.points) {
            p.blanked = true;
        }
    }
}

void applyPulse(ilda::IldaFrame& frame, const Effect& e, double t) {
    // Scale factor breathes around 1.0 by +/- amount. Reverse flips the phase so
    // it starts by shrinking instead of growing.
    double phase = kTwoPi * e.rateHz * t;
    if (e.direction == Direction::Reverse) {
        phase = -phase;
    }
    const float scale = 1.0f + e.amount * static_cast<float>(std::sin(phase));
    for (ilda::IldaPoint& p : frame.points) {
        p.x *= scale;
        p.y *= scale;
    }
}

// Full-saturation, full-value HSV -> RGB. `h` is wrapped into [0,1).
void hsvToRgb(double h, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
    const double hh = frac(h) * 6.0;
    const int sector = static_cast<int>(hh) % 6;
    const double f = hh - std::floor(hh);
    double rf = 0.0, gf = 0.0, bf = 0.0;
    switch (sector) {
        case 0: rf = 1.0; gf = f; bf = 0.0; break;
        case 1: rf = 1.0 - f; gf = 1.0; bf = 0.0; break;
        case 2: rf = 0.0; gf = 1.0; bf = f; break;
        case 3: rf = 0.0; gf = 1.0 - f; bf = 1.0; break;
        case 4: rf = f; gf = 0.0; bf = 1.0; break;
        default: rf = 1.0; gf = 0.0; bf = 1.0 - f; break;
    }
    r = static_cast<std::uint8_t>(std::lround(rf * 255.0));
    g = static_cast<std::uint8_t>(std::lround(gf * 255.0));
    b = static_cast<std::uint8_t>(std::lround(bf * 255.0));
}

void applyColorCycle(ilda::IldaFrame& frame, const Effect& e, double t) {
    double timeHue = e.rateHz * t;
    if (e.direction == Direction::Reverse) {
        timeHue = -timeHue;
    }
    const std::size_t n = frame.points.size();
    for (std::size_t i = 0; i < n; ++i) {
        ilda::IldaPoint& p = frame.points[i];
        if (p.blanked) {
            continue; // keep dark points dark
        }
        const double pos = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
        hsvToRgb(timeHue + pos * e.amount, p.r, p.g, p.b);
    }
}

void applyBounce(ilda::IldaFrame& frame, const Effect& e, double t) {
    // A ball-like hop: the shape rises by amount*|sin| and drops back to the
    // floor each half-cycle, never going below the baseline. Reverse hops the
    // other way (down / left).
    float offset = e.amount * static_cast<float>(std::fabs(std::sin(kTwoPi * e.rateHz * t)));
    if (e.direction == Direction::Reverse) {
        offset = -offset;
    }
    for (ilda::IldaPoint& p : frame.points) {
        if (e.axis == 0) {
            p.x += offset;
        } else {
            p.y += offset;
        }
    }
}

void applyTrace(ilda::IldaFrame& frame, const Effect& e, double t) {
    // Reveal the shape up to a moving cursor and blank the rest, so it appears to
    // draw itself on and loop.
    double phase = 0.0;
    switch (e.direction) {
        case Direction::Forward:
            phase = frac(e.rateHz * t);
            break;
        case Direction::Reverse:
            phase = frac(-e.rateHz * t);
            break;
        case Direction::PingPong:
            phase = triangle01(e.rateHz * t);
            break;
    }
    const std::size_t n = frame.points.size();
    for (std::size_t i = 0; i < n; ++i) {
        const double pos = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
        if (pos > phase) {
            frame.points[i].blanked = true;
        }
    }
}

// Cheap deterministic hash -> [0,1). Same index+tick always gives the same
// value, so a sparkle pattern is stable within a time step and reproducible.
double hashUnit(std::uint32_t index, std::uint64_t tick) {
    std::uint32_t h = index * 2654435761u ^ static_cast<std::uint32_t>(tick) * 668265263u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<double>(h & 0xFFFFFFu) / static_cast<double>(0x1000000u);
}

void applyTwinkle(ilda::IldaFrame& frame, const Effect& e, double t) {
    // Each time step (rate cycles/second) re-rolls which points are lit; `amount`
    // is the fraction kept on. amount>=1 leaves all lit, amount<=0 blanks all.
    const auto tick = static_cast<std::uint64_t>(std::floor(e.rateHz * t));
    const std::size_t n = frame.points.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (hashUnit(static_cast<std::uint32_t>(i), tick) >= e.amount) {
            frame.points[i].blanked = true;
        }
    }
}

} // namespace

void applyEffect(ilda::IldaFrame& frame, const Effect& e, double seconds) {
    if (!e.enabled) {
        return;
    }
    switch (e.type) {
        case EffectType::Rotate:
            applyRotate(frame, e, seconds);
            break;
        case EffectType::Wave:
            applyWave(frame, e, seconds);
            break;
        case EffectType::Runner:
            applyRunner(frame, e, seconds);
            break;
        case EffectType::Strobe:
            applyStrobe(frame, e, seconds);
            break;
        case EffectType::Pulse:
            applyPulse(frame, e, seconds);
            break;
        case EffectType::ColorCycle:
            applyColorCycle(frame, e, seconds);
            break;
        case EffectType::Bounce:
            applyBounce(frame, e, seconds);
            break;
        case EffectType::Trace:
            applyTrace(frame, e, seconds);
            break;
        case EffectType::Twinkle:
            applyTwinkle(frame, e, seconds);
            break;
    }
}

void applyEffects(ilda::IldaFrame& frame, const std::vector<Effect>& effects,
                  double seconds) {
    for (const Effect& e : effects) {
        applyEffect(frame, e, seconds);
    }
}

} // namespace redfox::effects
