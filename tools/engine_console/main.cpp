// Manual test tool: stands in for the future UI process. Sends a heartbeat
// every 50ms (matching design spec section 5) and lets you send commands by
// typing them, so you can exercise the Engine's IPC and safety behaviour
// without building the real UI yet. Also doubles as the watchdog-kill-drill
// tool: run this, type "arm", then kill this process and watch the Engine's
// own console output for the heartbeat-loss blanking event.

#include "ipc/TelemetryClient.hpp"
#include "ipc/CommandPipeClient.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

std::atomic<bool> g_running{true};

std::uint64_t nowEpochMs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

void heartbeatLoop(redfox::ipc::TelemetryClient& telemetry) {
    while (g_running.load()) {
        if (telemetry.isValid()) {
            telemetry.telemetry().uiHeartbeatEpochMs.store(nowEpochMs());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace

int main() {
    redfox::ipc::TelemetryClient telemetry;
    if (!telemetry.isValid()) {
        std::cerr << "Could not attach to Engine telemetry. Is redfox_engine.exe running?\n";
        return 1;
    }

    redfox::ipc::CommandPipeClient commands;
    if (!commands.isConnected()) {
        std::cerr << "Could not connect to Engine command pipe. Is redfox_engine.exe running?\n";
        return 1;
    }

    std::thread heartbeatThread(heartbeatLoop, std::ref(telemetry));

    std::cout << "Connected. Commands: arm, estop, clear, ping, cue <n>, "
                 "stopcue, status, quit" << std::endl;

    std::string line;
    while (g_running.load() && std::getline(std::cin, line)) {
        if (line == "arm") {
            commands.send(redfox::ipc::CommandType::Arm);
        } else if (line == "estop") {
            commands.send(redfox::ipc::CommandType::EmergencyStop);
        } else if (line == "clear") {
            commands.send(redfox::ipc::CommandType::ClearEmergencyStop);
        } else if (line == "ping") {
            commands.send(redfox::ipc::CommandType::Ping);
        } else if (line.rfind("cue ", 0) == 0) {
            try {
                const std::uint32_t index =
                    static_cast<std::uint32_t>(std::stoul(line.substr(4)));
                commands.send(redfox::ipc::CommandType::TriggerCue, index);
            } catch (const std::exception&) {
                std::cout << "usage: cue <index>" << std::endl;
            }
        } else if (line == "stopcue") {
            commands.send(redfox::ipc::CommandType::StopCue);
        } else if (line == "status") {
            std::cout << "safetyState=" << telemetry.telemetry().safetyState.load()
                      << " framesSent=" << telemetry.telemetry().framesSent.load()
                      << " engineHeartbeatEpochMs=" << telemetry.telemetry().engineHeartbeatEpochMs.load()
                      << std::endl;
        } else if (line == "quit") {
            g_running = false;
        } else {
            std::cout << "Unknown command: " << line << std::endl;
        }
    }

    g_running = false;
    heartbeatThread.join();
    return 0;
}
