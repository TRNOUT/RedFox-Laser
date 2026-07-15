#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace redfox::ipc {

enum class SafetyStateWire : std::uint32_t {
    Blanked = 0,
    Armed = 1,
    EStopLatched = 2,
};

// One point of the live preview frame (normalised -1..1 position, 0..1 colour).
struct PreviewPoint {
    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

inline constexpr std::size_t kMaxPreviewPoints = 4096;

// Laid out for a Windows file-mapping shared between the Engine and UI
// processes. All fields are atomics so cross-process reads/writes are safe
// without an explicit lock; only the Engine (TelemetryHost) constructs this
// in the mapped memory, the UI/tests (TelemetryClient) only reinterpret it.
struct EngineTelemetry {
    std::atomic<std::uint64_t> uiHeartbeatEpochMs{0};
    std::atomic<std::uint64_t> engineHeartbeatEpochMs{0};
    std::atomic<std::uint64_t> framesSent{0};    // output frames sent since start
    std::atomic<std::uint32_t> safetyState{0};   // SafetyStateWire
    std::atomic<std::uint32_t> lastEventCode{0}; // last SafetyEventCode

    // Audio-reactive features (updated by the engine's audio input).
    std::atomic<float> audioLevel{0.0f};
    std::atomic<float> audioBass{0.0f};
    std::atomic<float> audioMid{0.0f};
    std::atomic<float> audioHigh{0.0f};
    std::atomic<std::uint64_t> beatCount{0};

    // Live preview of the current output frame, published with a seqlock: the
    // sequence is odd while the engine writes and even when stable, so the UI
    // can read a torn-free snapshot without a cross-process mutex.
    std::atomic<std::uint32_t> previewSeq{0};
    std::atomic<std::uint32_t> previewCount{0};
    PreviewPoint previewPoints[kMaxPreviewPoints];
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "EngineTelemetry fields must be lock-free for cross-process shared memory use");

// Engine side: publish the current frame's points.
inline void writePreview(EngineTelemetry& t, const PreviewPoint* points, std::size_t count) {
    if (count > kMaxPreviewPoints) {
        count = kMaxPreviewPoints;
    }
    const std::uint32_t seq = t.previewSeq.load(std::memory_order_relaxed);
    t.previewSeq.store(seq + 1, std::memory_order_release); // enter write (odd)
    for (std::size_t i = 0; i < count; ++i) {
        t.previewPoints[i] = points[i];
    }
    t.previewCount.store(static_cast<std::uint32_t>(count), std::memory_order_relaxed);
    t.previewSeq.store(seq + 2, std::memory_order_release); // stable (even)
}

// UI side: copy a torn-free snapshot into `out` (capacity kMaxPreviewPoints);
// returns the point count.
inline std::size_t readPreview(const EngineTelemetry& t, PreviewPoint* out) {
    for (int attempt = 0; attempt < 8; ++attempt) {
        const std::uint32_t s1 = t.previewSeq.load(std::memory_order_acquire);
        if (s1 & 1u) {
            continue; // a write is in progress
        }
        std::uint32_t count = t.previewCount.load(std::memory_order_relaxed);
        if (count > kMaxPreviewPoints) {
            count = kMaxPreviewPoints;
        }
        for (std::uint32_t i = 0; i < count; ++i) {
            out[i] = t.previewPoints[i];
        }
        const std::uint32_t s2 = t.previewSeq.load(std::memory_order_acquire);
        if (s1 == s2) {
            return count; // consistent snapshot
        }
    }
    return 0;
}

inline constexpr wchar_t kTelemetrySharedMemoryName[] = L"Local\\RedFoxLaser_Telemetry_v4";
inline constexpr std::size_t kTelemetrySharedMemorySize = sizeof(EngineTelemetry);

} // namespace redfox::ipc
