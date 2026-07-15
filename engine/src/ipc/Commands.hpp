#pragma once
#include <cstdint>

namespace redfox::ipc {

enum class CommandType : std::uint32_t {
    Ping = 0,
    Arm = 1,
    EmergencyStop = 2,
    ClearEmergencyStop = 3,
};

struct CommandMessage {
    std::uint32_t type = 0; // CommandType
};

inline constexpr wchar_t kCommandPipeName[] = L"\\\\.\\pipe\\RedFoxLaser_Commands_v1";

} // namespace redfox::ipc
