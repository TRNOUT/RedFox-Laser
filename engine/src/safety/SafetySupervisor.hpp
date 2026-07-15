#pragma once

#include "Clock.hpp"
#include "SafetyState.hpp"
#include "../output/LaserOutput.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace redfox::safety {

struct SafetySupervisorConfig {
    std::chrono::milliseconds uiHeartbeatTimeout{250};
    std::chrono::milliseconds frameStallTimeout{200};
};

// Owns the Armed/Blanked/EStopLatched state machine described in the design
// spec (section 5). This class never touches Win32 APIs directly -- the
// Engine's watchdog thread calls tick() periodically, and the IPC layer
// calls notifyUiHeartbeat()/requestArm()/etc. in response to messages from
// the UI process.
class SafetySupervisor {
public:
    using EventCallback = std::function<void(SafetyEventCode, SafetyState)>;

    SafetySupervisor(Clock& clock, SafetySupervisorConfig config);

    void registerOutput(std::shared_ptr<output::LaserOutput> output);
    void setEventCallback(EventCallback callback);

    // Called by the Engine's IPC layer whenever a UI heartbeat arrives.
    void notifyUiHeartbeat();

    // Called by the frame generator whenever a new frame has been produced.
    // (No real frame generator exists yet in this milestone -- exercised
    // directly from tests, wired to the real generator in a later milestone.)
    void notifyFrameProduced();

    // Operator-initiated transitions.
    bool requestArm();          // false if e-stop is latched
    void triggerEmergencyStop();
    bool clearEmergencyStop();  // false if not currently latched

    // Must be called periodically (e.g. every 100ms) from the Engine's
    // watchdog thread.
    void tick();

    SafetyState state() const;

private:
    Clock& clock_;
    SafetySupervisorConfig config_;
    mutable std::mutex mutex_;

    SafetyState state_ = SafetyState::Blanked;
    std::chrono::steady_clock::time_point lastUiHeartbeat_{};
    std::chrono::steady_clock::time_point lastFrame_{};
    bool haveUiHeartbeat_ = false;
    bool haveFrame_ = false;

    std::vector<std::shared_ptr<output::LaserOutput>> outputs_;
    EventCallback eventCallback_;
};

} // namespace redfox::safety
