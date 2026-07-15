#include "effects/Oscillator.hpp"

#include <cmath>

namespace redfox::effects {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
double frac(double v) { return v - std::floor(v); }
} // namespace

float waveform(Waveform shape, double phase) {
    phase = frac(phase);
    switch (shape) {
        case Waveform::Sine:
            return static_cast<float>(std::sin(phase * kTwoPi));
        case Waveform::Triangle:
            // -1 at 0, +1 at 0.5, back to -1 at 1.
            return phase < 0.5 ? static_cast<float>(4.0 * phase - 1.0)
                               : static_cast<float>(3.0 - 4.0 * phase);
        case Waveform::Square:
            return phase < 0.5 ? 1.0f : -1.0f;
        case Waveform::Sawtooth:
            return static_cast<float>(2.0 * phase - 1.0);
    }
    return 0.0f;
}

float Oscillator::valueAt(double seconds) const {
    const double phaseValue = seconds * static_cast<double>(frequencyHz) +
                              static_cast<double>(phase);
    return offset + amplitude * waveform(shape, phaseValue);
}

} // namespace redfox::effects
