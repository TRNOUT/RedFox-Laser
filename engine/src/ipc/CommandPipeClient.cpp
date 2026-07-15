#include "CommandPipeClient.hpp"
#include <windows.h>

namespace redfox::ipc {

CommandPipeClient::CommandPipeClient() {
    HANDLE h = CreateFileW(kCommandPipeName, GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        pipeHandle_ = h;
    }
}

CommandPipeClient::~CommandPipeClient() {
    if (pipeHandle_ != nullptr) {
        CloseHandle(reinterpret_cast<HANDLE>(pipeHandle_));
    }
}

bool CommandPipeClient::isConnected() const {
    return pipeHandle_ != nullptr;
}

bool CommandPipeClient::send(CommandType type) {
    if (pipeHandle_ == nullptr) {
        return false;
    }
    CommandMessage message{static_cast<std::uint32_t>(type)};
    DWORD bytesWritten = 0;
    const BOOL ok = WriteFile(reinterpret_cast<HANDLE>(pipeHandle_), &message,
                              sizeof(message), &bytesWritten, nullptr);
    return ok && bytesWritten == sizeof(message);
}

} // namespace redfox::ipc
