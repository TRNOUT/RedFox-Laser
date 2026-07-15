#pragma once

#include <cstddef>
#include <deque>

namespace redfox::audio {

// Simple energy-based beat detector: flags a beat when the current energy
// spikes above a moving average of recent energy, with a refractory period so a
// single transient isn't reported twice.
class BeatDetector {
public:
    explicit BeatDetector(std::size_t window = 43, float sensitivity = 1.5f,
                          float minLevel = 0.02f, int refractoryFrames = 2);

    // Feed the current instantaneous energy (e.g. AudioFeatures::level or bass).
    // Returns true on the frame a beat is detected.
    bool process(float energy);

    void reset();

private:
    std::deque<float> history_;
    double sum_ = 0.0;
    std::size_t window_;
    float sensitivity_;
    float minLevel_;
    int refractoryFrames_;
    int cooldown_ = 0;
};

} // namespace redfox::audio
