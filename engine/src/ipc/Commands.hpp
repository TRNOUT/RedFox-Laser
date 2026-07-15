#pragma once
#include <cstdint>

namespace redfox::ipc {

enum class CommandType : std::uint32_t {
    Ping = 0,
    Arm = 1,
    EmergencyStop = 2,
    ClearEmergencyStop = 3,
    TriggerCue = 4, // arg = cue index
    StopCue = 5,
    PlaySequence = 6, // start the loaded timeline from the beginning
    StopSequence = 7, // stop timeline playback
    ReloadShow = 8,   // re-read the show file from disk (after an editor save)
};

struct CommandMessage {
    std::uint32_t type = 0; // CommandType
    std::uint32_t arg = 0;  // command-specific (e.g. cue index for TriggerCue)
};

inline constexpr wchar_t kCommandPipeName[] = L"\\\\.\\pipe\\RedFoxLaser_Commands_v2";

} // namespace redfox::ipc
