#pragma once

#include <cstddef>

namespace redfox::audio {

// Frequency-band features extracted from a block of mono audio samples.
struct AudioFeatures {
    float level = 0.0f; // overall RMS level (a full-scale sine is ~0.707)
    float bass = 0.0f;  // ~20-250 Hz energy
    float mid = 0.0f;   // ~250-4000 Hz energy
    float high = 0.0f;  // ~4000-20000 Hz energy
};

// Analyse `n` mono samples (n should be a power of two for the band analysis;
// otherwise only `level` is computed). `sampleRate` is in Hz.
AudioFeatures analyze(const float* samples, std::size_t n, float sampleRate);

} // namespace redfox::audio
