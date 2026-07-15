#include "audio/BeatDetector.hpp"

namespace redfox::audio {

BeatDetector::BeatDetector(std::size_t window, float sensitivity, float minLevel,
                           int refractoryFrames)
    : window_(window == 0 ? 1 : window),
      sensitivity_(sensitivity),
      minLevel_(minLevel),
      refractoryFrames_(refractoryFrames) {}

bool BeatDetector::process(float energy) {
    const double average =
        history_.empty() ? 0.0 : sum_ / static_cast<double>(history_.size());

    bool beat = false;
    if (cooldown_ == 0 && !history_.empty() && energy > minLevel_ &&
        static_cast<double>(energy) > average * static_cast<double>(sensitivity_)) {
        beat = true;
        cooldown_ = refractoryFrames_;
    } else if (cooldown_ > 0) {
        --cooldown_;
    }

    history_.push_back(energy);
    sum_ += energy;
    if (history_.size() > window_) {
        sum_ -= history_.front();
        history_.pop_front();
    }
    return beat;
}

void BeatDetector::reset() {
    history_.clear();
    sum_ = 0.0;
    cooldown_ = 0;
}

} // namespace redfox::audio
