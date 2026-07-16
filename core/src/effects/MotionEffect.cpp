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
    }
}

void applyEffects(ilda::IldaFrame& frame, const std::vector<Effect>& effects,
                  double seconds) {
    for (const Effect& e : effects) {
        applyEffect(frame, e, seconds);
    }
}

} // namespace redfox::effects
