#include "safety/Clock.hpp"
#include "safety/SafetySupervisor.hpp"
#include "safety/SafetyEventLog.hpp"
#include "output/MockLaserOutput.hpp"
#include "ipc/TelemetryHost.hpp"
#include "ipc/CommandPipeServer.hpp"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

// The Engine reports its SafetyState to the UI over shared memory as a
// SafetyStateWire value via a plain static_cast (see the main loop below).
// That cast is only correct while the two enums share the same numeric
// values, so guard the coupling here: reordering or inserting a SafetyState
// value without matching SafetyStateWire would otherwise silently misreport
// the safety state across the IPC boundary.
static_assert(static_cast<int>(redfox::safety::SafetyState::Blanked) ==
              static_cast<int>(redfox::ipc::SafetyStateWire::Blanked),
              "SafetyState/SafetyStateWire Blanked mismatch");
static_assert(static_cast<int>(redfox::safety::SafetyState::Armed) ==
              static_cast<int>(redfox::ipc::SafetyStateWire::Armed),
              "SafetyState/SafetyStateWire Armed mismatch");
static_assert(static_cast<int>(redfox::safety::SafetyState::EStopLatched) ==
              static_cast<int>(redfox::ipc::SafetyStateWire::EStopLatched),
              "SafetyState/SafetyStateWire EStopLatched mismatch");

namespace {

std::atomic<bool> g_shouldRun{true};

void onSignal(int) {
    g_shouldRun = false;
}

std::uint64_t nowEpochMs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

} // namespace

int main() {
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    // Design spec section 3: the Engine runs at high priority so its
    // watchdog/output timing isn't starved by other processes.
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    redfox::safety::SystemClock clock;
    redfox::safety::SafetySupervisorConfig config;
    redfox::safety::SafetySupervisor supervisor(clock, config);

    auto mockOutput = std::make_shared<redfox::output::MockLaserOutput>();
    supervisor.registerOutput(mockOutput);

    redfox::safety::SafetyEventLog eventLog(".\\logs");

    supervisor.setEventCallback([&eventLog](redfox::safety::SafetyEventCode code,
                                             redfox::safety::SafetyState state) {
        std::cout << "[safety] event=" << static_cast<int>(code)
                  << " state=" << static_cast<int>(state) << std::endl;
        eventLog.record(code, state);
    });

    redfox::ipc::TelemetryHost telemetryHost;
    if (!telemetryHost.isValid()) {
        std::cerr << "Failed to create telemetry shared memory.\n";
        return 1;
    }

    redfox::ipc::CommandPipeServer commandServer(
        [&supervisor](redfox::ipc::CommandType type) {
            using redfox::ipc::CommandType;
            switch (type) {
                case CommandType::Ping:
                    break;
                case CommandType::Arm:
                    supervisor.requestArm();
                    break;
                case CommandType::EmergencyStop:
                    supervisor.triggerEmergencyStop();
                    break;
                case CommandType::ClearEmergencyStop:
                    supervisor.clearEmergencyStop();
                    break;
            }
        });
    commandServer.start();

    std::cout << "RedFox-Laser Engine running (mock output). Ctrl+C to stop." << std::endl;

    std::uint64_t lastSeenUiHeartbeat = 0;
    while (g_shouldRun.load()) {
        auto& telemetry = telemetryHost.telemetry();

        const std::uint64_t uiHeartbeat = telemetry.uiHeartbeatEpochMs.load();
        if (uiHeartbeat != lastSeenUiHeartbeat) {
            lastSeenUiHeartbeat = uiHeartbeat;
            supervisor.notifyUiHeartbeat();
        }

        supervisor.tick();

        telemetry.engineHeartbeatEpochMs.store(nowEpochMs());
        telemetry.safetyState.store(static_cast<std::uint32_t>(supervisor.state()));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    commandServer.stop();
    std::cout << "Engine shut down." << std::endl;
    return 0;
}
