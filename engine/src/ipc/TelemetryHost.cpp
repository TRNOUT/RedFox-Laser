#include "TelemetryHost.hpp"
#include <windows.h>
#include <new>

namespace redfox::ipc {

TelemetryHost::TelemetryHost() {
    HANDLE mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE, // backed by the system paging file
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(kTelemetrySharedMemorySize),
        kTelemetrySharedMemoryName);

    if (mapping == nullptr) {
        return;
    }

    void* view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, kTelemetrySharedMemorySize);
    if (view == nullptr) {
        CloseHandle(mapping);
        return;
    }

    mapping_ = mapping;
    // Only the host placement-constructs the atomics; clients must never
    // reconstruct, only reinterpret_cast the existing bytes.
    telemetry_ = new (view) EngineTelemetry();
}

TelemetryHost::~TelemetryHost() {
    if (telemetry_ != nullptr) {
        UnmapViewOfFile(telemetry_);
    }
    if (mapping_ != nullptr) {
        CloseHandle(reinterpret_cast<HANDLE>(mapping_));
    }
}

} // namespace redfox::ipc
