#pragma once

namespace redfox::safety {

enum class SafetyState {
    Blanked,      // startup default, or after any trip
    Armed,        // actively allowed to emit
    EStopLatched  // emergency stop; requires an explicit clear
};

enum class SafetyEventCode {
    None,
    UiHeartbeatLost,
    FrameStall,
    EmergencyStop,
    ManualArm,
    ManualReArm,
};

} // namespace redfox::safety
