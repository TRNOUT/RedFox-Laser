#include "audio/AudioAnalysis.hpp"

#include <cmath>
#include <complex>
#include <vector>

namespace redfox::audio {
namespace {

constexpr double kTwoPi = 6.28318530717958647692;

bool isPowerOfTwo(std::size_t n) { return n != 0 && (n & (n - 1)) == 0; }

// In-place iterative radix-2 Cooley-Tukey FFT.
void fft(std::vector<std::complex<float>>& a) {
    const std::size_t n = a.size();
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }
    for (std::size_t len = 2; len <= n; len <<= 1) {
        const double ang = -kTwoPi / static_cast<double>(len);
        const std::complex<float> wlen(static_cast<float>(std::cos(ang)),
                                       static_cast<float>(std::sin(ang)));
        for (std::size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (std::size_t k = 0; k < len / 2; ++k) {
                const std::complex<float> u = a[i + k];
                const std::complex<float> v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

} // namespace

AudioFeatures analyze(const float* samples, std::size_t n, float sampleRate) {
    AudioFeatures f;
    if (samples == nullptr || n == 0) {
        return f;
    }

    double sumSq = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        sumSq += static_cast<double>(samples[i]) * samples[i];
    }
    f.level = static_cast<float>(std::sqrt(sumSq / static_cast<double>(n)));

    if (!isPowerOfTwo(n) || sampleRate <= 0.0f) {
        return f; // level only
    }

    // Hann-windowed copy into the complex spectrum buffer.
    std::vector<std::complex<float>> spec(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double window =
            0.5 * (1.0 - std::cos(kTwoPi * static_cast<double>(i) / (n - 1)));
        spec[i] = std::complex<float>(static_cast<float>(samples[i] * window), 0.0f);
    }
    fft(spec);

    double bass = 0.0, mid = 0.0, high = 0.0;
    for (std::size_t k = 1; k < n / 2; ++k) {
        const double freq = static_cast<double>(k) * sampleRate / static_cast<double>(n);
        const double mag = std::abs(spec[k]);
        if (freq >= 20.0 && freq < 250.0) {
            bass += mag;
        } else if (freq >= 250.0 && freq < 4000.0) {
            mid += mag;
        } else if (freq >= 4000.0 && freq < 20000.0) {
            high += mag;
        }
    }

    const double scale = 2.0 / static_cast<double>(n);
    f.bass = static_cast<float>(bass * scale);
    f.mid = static_cast<float>(mid * scale);
    f.high = static_cast<float>(high * scale);
    return f;
}

} // namespace redfox::audio
