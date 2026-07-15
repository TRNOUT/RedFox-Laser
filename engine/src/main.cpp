#include "safety/Clock.hpp"
#include "safety/SafetySupervisor.hpp"
#include "safety/SafetyEventLog.hpp"
#include "output/LaserOutput.hpp"
#include "output/MockLaserOutput.hpp"
#include "playback/FrameGenerator.hpp"
#include "midi/MidiInput.hpp"
#include "midi/MidiMap.hpp"
#include "midi/MidiTypes.hpp"
#include "show/Show.hpp"
#include "ilda/IldaTypes.hpp"
#include "ipc/TelemetryHost.hpp"
#include "ipc/CommandPipeServer.hpp"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

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

// A tiny built-in show so the playback path can be exercised end-to-end against
// the mock output before real content loading exists.
std::shared_ptr<const redfox::show::Show> makeDemoShow() {
    using namespace redfox;
    auto show = std::make_shared<show::Show>();
    show->name = "Demo";

    show::Cue square;
    square.name = "Square";
    ilda::IldaFrame squareFrame;
    constexpr std::uint8_t kWhite = 255;
    squareFrame.points = {
        {-0.5f, -0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {0.5f, -0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {0.5f, 0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {-0.5f, 0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {-0.5f, -0.5f, 0.0f, kWhite, kWhite, kWhite, false},
    };
    square.frames = {squareFrame};
    show->cues.push_back(square);

    show::Cue blink;
    blink.name = "Blink";
    blink.framesPerSecond = 4.0f;
    ilda::IldaFrame dotOn;
    dotOn.points = {{0.0f, 0.0f, 0.0f, 255, 0, 0, false}};
    ilda::IldaFrame dotOff;
    dotOff.points = {{0.0f, 0.0f, 0.0f, 0, 0, 0, true}};
    blink.frames = {dotOn, dotOff};
    show->cues.push_back(blink);

    return show;
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

    redfox::ipc::TelemetryHost telemetryHost;
    if (!telemetryHost.isValid()) {
        std::cerr << "Failed to create telemetry shared memory.\n";
        return 1;
    }

    redfox::safety::SafetyEventLog eventLog(".\\logs");

    supervisor.setEventCallback([&eventLog, &telemetryHost](
                                    redfox::safety::SafetyEventCode code,
                                    redfox::safety::SafetyState state) {
        std::cout << "[safety] event=" << static_cast<int>(code)
                  << " state=" << static_cast<int>(state) << std::endl;
        eventLog.record(code, state);
        telemetryHost.telemetry().lastEventCode.store(static_cast<std::uint32_t>(code));
    });

    redfox::engine::FrameGenerator frameGenerator(clock);
    frameGenerator.setShow(makeDemoShow());

    // MIDI input: pads (notes 36..43) trigger cues 0..7; note 44 arms, note 45
    // is a panic/blackout. Runs on libremidi's thread; the targets it calls
    // (supervisor, frame generator) are thread-safe.
    redfox::engine::MidiInput midiInput;
    {
        redfox::midi::MidiMap midiMap;
        for (int i = 0; i < 8; ++i) {
            midiMap.bind({redfox::midi::MidiType::NoteOn, 0,
                          static_cast<std::uint8_t>(36 + i),
                          redfox::midi::ActionType::TriggerCue, i});
        }
        midiMap.bind({redfox::midi::MidiType::NoteOn, 0, 44, redfox::midi::ActionType::Arm, 0});
        midiMap.bind({redfox::midi::MidiType::NoteOn, 0, 45, redfox::midi::ActionType::Blackout, 0});
        midiInput.setMap(midiMap);
    }
    midiInput.setActionCallback(
        [&supervisor, &frameGenerator](const redfox::midi::Action& action) {
            using redfox::midi::ActionType;
            switch (action.type) {
                case ActionType::TriggerCue:
                    frameGenerator.triggerCue(static_cast<std::size_t>(action.cueIndex));
                    break;
                case ActionType::Arm:
                    supervisor.requestArm();
                    break;
                case ActionType::Blackout:
                    supervisor.triggerEmergencyStop();
                    break;
                case ActionType::SetMasterBrightness:
                case ActionType::None:
                    break;
            }
        });
    const std::string midiPort = midiInput.openFirstPort();
    if (!midiPort.empty()) {
        std::cout << "MIDI: listening on \"" << midiPort << "\"" << std::endl;
    } else {
        std::cout << "MIDI: no input device found (controls still work over IPC)."
                  << std::endl;
    }

    redfox::ipc::CommandPipeServer commandServer(
        [&supervisor, &frameGenerator](redfox::ipc::CommandType type, std::uint32_t arg) {
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
                case CommandType::TriggerCue:
                    frameGenerator.triggerCue(arg);
                    break;
                case CommandType::StopCue:
                    frameGenerator.stop();
                    break;
            }
        });
    commandServer.start();

    std::cout << "RedFox-Laser Engine running (mock output). Ctrl+C to stop." << std::endl;

    std::vector<redfox::output::OutputPoint> outputPoints;
    std::uint64_t lastSeenUiHeartbeat = 0;
    while (g_shouldRun.load()) {
        auto& telemetry = telemetryHost.telemetry();

        const std::uint64_t uiHeartbeat = telemetry.uiHeartbeatEpochMs.load();
        if (uiHeartbeat != lastSeenUiHeartbeat) {
            lastSeenUiHeartbeat = uiHeartbeat;
            supervisor.notifyUiHeartbeat();
        }

        supervisor.tick();

        // While armed with an active cue, render its current frame to the
        // output and keep the frame-stall watchdog satisfied.
        if (supervisor.state() == redfox::safety::SafetyState::Armed &&
            frameGenerator.hasActiveCue()) {
            if (frameGenerator.currentFrame(outputPoints)) {
                mockOutput->sendFrame(outputPoints);
                supervisor.notifyFrameProduced();
                telemetry.framesSent.fetch_add(1, std::memory_order_relaxed);
            }
        }

        telemetry.engineHeartbeatEpochMs.store(nowEpochMs());
        telemetry.safetyState.store(static_cast<std::uint32_t>(supervisor.state()));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    commandServer.stop();
    std::cout << "Engine shut down." << std::endl;
    return 0;
}
