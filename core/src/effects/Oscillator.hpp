#pragma once

namespace redfox::effects {

enum class Waveform {
    Sine,
    Triangle,
    Square,
    Sawtooth,
};

// Evaluate a waveform at phase in [0,1) -> value in [-1,1].
float waveform(Waveform shape, double phase);

// A low-frequency oscillator used to modulate parameters over time.
// value = offset + amplitude * waveform(shape, frac(seconds * frequencyHz + phase)).
struct Oscillator {
    Waveform shape = Waveform::Sine;
    float frequencyHz = 1.0f;
    float amplitude = 1.0f;
    float offset = 0.0f;
    float phase = 0.0f; // starting phase, 0..1

    float valueAt(double seconds) const;
};

} // namespace redfox::effects
