#pragma once
#include "SharedTelemetry.hpp"

namespace redfox::ipc {

// Engine-side owner of the telemetry shared memory. Creates the mapping and
// placement-constructs EngineTelemetry into it.
class TelemetryHost {
public:
    TelemetryHost();
    ~TelemetryHost();

    TelemetryHost(const TelemetryHost&) = delete;
    TelemetryHost& operator=(const TelemetryHost&) = delete;

    bool isValid() const { return telemetry_ != nullptr; }
    EngineTelemetry& telemetry() { return *telemetry_; }

private:
    void* mapping_ = nullptr; // HANDLE, kept as void* to keep windows.h out of this header
    EngineTelemetry* telemetry_ = nullptr;
};

} // namespace redfox::ipc
