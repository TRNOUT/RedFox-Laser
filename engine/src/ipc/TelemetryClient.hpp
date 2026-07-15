#pragma once
#include "SharedTelemetry.hpp"

namespace redfox::ipc {

// UI/test-side opener of the telemetry shared memory created by TelemetryHost.
class TelemetryClient {
public:
    TelemetryClient();
    ~TelemetryClient();

    TelemetryClient(const TelemetryClient&) = delete;
    TelemetryClient& operator=(const TelemetryClient&) = delete;

    bool isValid() const { return telemetry_ != nullptr; }
    EngineTelemetry& telemetry() { return *telemetry_; }

private:
    void* mapping_ = nullptr;
    EngineTelemetry* telemetry_ = nullptr;
};

} // namespace redfox::ipc
