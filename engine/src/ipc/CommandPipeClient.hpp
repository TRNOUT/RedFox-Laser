#pragma once
#include "Commands.hpp"

namespace redfox::ipc {

class CommandPipeClient {
public:
    // Connects immediately. Use isConnected() to check success.
    CommandPipeClient();
    ~CommandPipeClient();

    CommandPipeClient(const CommandPipeClient&) = delete;
    CommandPipeClient& operator=(const CommandPipeClient&) = delete;

    bool isConnected() const;
    bool send(CommandType type, std::uint32_t arg = 0);

private:
    void* pipeHandle_ = nullptr; // HANDLE, kept as void* to keep windows.h out of this header
};

} // namespace redfox::ipc
