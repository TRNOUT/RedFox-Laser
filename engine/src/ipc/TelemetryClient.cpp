#include "TelemetryClient.hpp"
#include <windows.h>

namespace redfox::ipc {

TelemetryClient::TelemetryClient() {
    HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kTelemetrySharedMemoryName);
    if (mapping == nullptr) {
        return;
    }

    void* view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, kTelemetrySharedMemorySize);
    if (view == nullptr) {
        CloseHandle(mapping);
        return;
    }

    mapping_ = mapping;
    // The host already placement-constructed the atomics; just reinterpret.
    telemetry_ = reinterpret_cast<EngineTelemetry*>(view);
}

TelemetryClient::~TelemetryClient() {
    if (telemetry_ != nullptr) {
        UnmapViewOfFile(telemetry_);
    }
    if (mapping_ != nullptr) {
        CloseHandle(reinterpret_cast<HANDLE>(mapping_));
    }
}

} // namespace redfox::ipc
