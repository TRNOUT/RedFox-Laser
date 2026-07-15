#include <catch2/catch_test_macros.hpp>

#include "audio/AudioAnalysis.hpp"
#include "audio/BeatDetector.hpp"

#include <cmath>
#include <vector>

using namespace redfox;

namespace {
constexpr double kTwoPi = 6.28318530717958647692;

std::vector<float> sine(double freq, double sampleRate, std::size_t n, float amp = 1.0f) {
    std::vector<float> s(n);
    for (std::size_t i = 0; i < n; ++i) {
        s[i] = amp * static_cast<float>(std::sin(kTwoPi * freq * i / sampleRate));
    }
    return s;
}
bool near(float a, float b, float eps) { return std::fabs(a - b) < eps; }
} // namespace

TEST_CASE("RMS level of a full-scale sine is ~0.707", "[audio]") {
    const auto s = sine(1000.0, 44100.0, 1024);
    const audio::AudioFeatures f = audio::analyze(s.data(), s.size(), 44100.0f);
    REQUIRE(near(f.level, 0.707f, 0.02f));
}

TEST_CASE("silence yields near-zero features", "[audio]") {
    std::vector<float> s(1024, 0.0f);
    const audio::AudioFeatures f = audio::analyze(s.data(), s.size(), 44100.0f);
    REQUIRE(near(f.level, 0.0f, 0.001f));
    REQUIRE(near(f.bass, 0.0f, 0.001f));
    REQUIRE(near(f.mid, 0.0f, 0.001f));
    REQUIRE(near(f.high, 0.0f, 0.001f));
}

TEST_CASE("a tone lands in the expected band", "[audio]") {
    const double sr = 44100.0;

    const auto bassTone = sine(120.0, sr, 1024);
    const audio::AudioFeatures fb = audio::analyze(bassTone.data(), bassTone.size(), (float)sr);
    REQUIRE(fb.bass > fb.mid);
    REQUIRE(fb.bass > fb.high);

    const auto midTone = sine(1500.0, sr, 1024);
    const audio::AudioFeatures fm = audio::analyze(midTone.data(), midTone.size(), (float)sr);
    REQUIRE(fm.mid > fm.bass);
    REQUIRE(fm.mid > fm.high);

    const auto highTone = sine(8000.0, sr, 1024);
    const audio::AudioFeatures fh = audio::analyze(highTone.data(), highTone.size(), (float)sr);
    REQUIRE(fh.high > fh.bass);
    REQUIRE(fh.high > fh.mid);
}

TEST_CASE("beat detector flags an energy spike, not a steady level", "[audio]") {
    audio::BeatDetector detector(/*window*/ 8, /*sensitivity*/ 1.5f,
                                 /*minLevel*/ 0.02f, /*refractory*/ 2);

    // Steady low energy: no beats once the average is established.
    for (int i = 0; i < 8; ++i) {
        detector.process(0.05f);
    }
    REQUIRE_FALSE(detector.process(0.05f));

    // A spike well above the recent average triggers a beat.
    REQUIRE(detector.process(0.6f));

    // The immediately following high frame is suppressed by the refractory.
    REQUIRE_FALSE(detector.process(0.6f));
}
