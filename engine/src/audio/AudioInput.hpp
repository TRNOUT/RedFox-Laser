#pragma once

#include "audio/AudioAnalysis.hpp"

#include <functional>
#include <memory>

namespace redfox::engine {

// Device layer for audio capture (via miniaudio). Captures mono audio from the
// system loopback (what's playing) or a microphone, analyses it in fixed blocks,
// and delivers band features + a beat flag through a callback. miniaudio is
// hidden behind a pimpl. The callback runs on the audio thread — keep it quick.
class AudioInput {
public:
    using FeaturesCallback = std::function<void(const audio::AudioFeatures&, bool beat)>;

    AudioInput();
    ~AudioInput();

    AudioInput(const AudioInput&) = delete;
    AudioInput& operator=(const AudioInput&) = delete;

    void setFeaturesCallback(FeaturesCallback callback);

    // Start capturing. loopback=true captures system output; false captures the
    // default microphone/line-in. Returns false if the device couldn't start.
    bool start(bool loopback);
    void stop();
    bool isRunning() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace redfox::engine
