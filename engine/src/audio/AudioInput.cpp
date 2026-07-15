#include "audio/AudioInput.hpp"

#include "audio/BeatDetector.hpp"

#include <mutex>
#include <vector>

#include "miniaudio.h" // implementation lives in miniaudio_impl.cpp

namespace redfox::engine {

namespace {
constexpr std::size_t kBlockSize = 1024; // analysis block (power of two)
constexpr std::uint32_t kSampleRate = 44100;
} // namespace

struct AudioInput::Impl {
    ma_device device{};
    bool started = false;

    std::mutex mutex;
    AudioInput::FeaturesCallback callback;
    audio::BeatDetector beat;
    std::vector<float> accum; // accumulates mono samples until a full block

    static void deviceCallback(ma_device* device, void*, const void* input,
                               ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(device->pUserData);
        if (impl != nullptr && input != nullptr) {
            impl->onSamples(static_cast<const float*>(input), frameCount);
        }
    }

    void onSamples(const float* input, std::size_t frames) {
        AudioInput::FeaturesCallback cb;
        {
            std::lock_guard<std::mutex> lock(mutex);
            cb = callback;
        }
        for (std::size_t i = 0; i < frames; ++i) {
            accum.push_back(input[i]);
            if (accum.size() >= kBlockSize) {
                const audio::AudioFeatures features =
                    audio::analyze(accum.data(), accum.size(),
                                   static_cast<float>(kSampleRate));
                bool isBeat = false;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    isBeat = beat.process(features.bass);
                }
                if (cb) {
                    cb(features, isBeat);
                }
                accum.clear();
            }
        }
    }
};

AudioInput::AudioInput() : impl_(std::make_unique<Impl>()) {}

AudioInput::~AudioInput() { stop(); }

void AudioInput::setFeaturesCallback(FeaturesCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->callback = std::move(callback);
}

bool AudioInput::start(bool loopback) {
    if (impl_->started) {
        return true;
    }
    ma_device_config config = ma_device_config_init(
        loopback ? ma_device_type_loopback : ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1; // mono (miniaudio down-mixes as needed)
    config.sampleRate = kSampleRate;
    config.dataCallback = &Impl::deviceCallback;
    config.pUserData = impl_.get();

    if (ma_device_init(nullptr, &config, &impl_->device) != MA_SUCCESS) {
        return false;
    }
    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        ma_device_uninit(&impl_->device);
        return false;
    }
    impl_->started = true;
    return true;
}

void AudioInput::stop() {
    if (impl_->started) {
        ma_device_uninit(&impl_->device);
        impl_->started = false;
    }
}

bool AudioInput::isRunning() const { return impl_->started; }

} // namespace redfox::engine
