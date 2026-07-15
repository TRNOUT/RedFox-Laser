#pragma once
#include <atomic>
#include <cstdint>

namespace redfox::ipc {

enum class SafetyStateWire : std::uint32_t {
    Blanked = 0,
    Armed = 1,
    EStopLatched = 2,
};

// Laid out for a Windows file-mapping shared between the Engine and UI
// processes. All fields are atomics so cross-process reads/writes are safe
// without an explicit lock; only the Engine (TelemetryHost) constructs this
// in the mapped memory, the UI/tests (TelemetryClient) only reinterpret it.
struct EngineTelemetry {
    std::atomic<std::uint64_t> uiHeartbeatEpochMs{0};
    std::atomic<std::uint64_t> engineHeartbeatEpochMs{0};
    std::atomic<std::uint32_t> safetyState{0};   // SafetyStateWire
    std::atomic<std::uint32_t> lastEventCode{0};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "EngineTelemetry fields must be lock-free for cross-process shared memory use");

inline constexpr wchar_t kTelemetrySharedMemoryName[] = L"Local\\RedFoxLaser_Telemetry_v1";
inline constexpr std::size_t kTelemetrySharedMemorySize = sizeof(EngineTelemetry);

} // namespace redfox::ipc
