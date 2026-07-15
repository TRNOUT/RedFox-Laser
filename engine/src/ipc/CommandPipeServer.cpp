#include "CommandPipeServer.hpp"
#include <windows.h>
#include <chrono>

namespace redfox::ipc {

CommandPipeServer::CommandPipeServer(CommandHandler handler)
    : handler_(std::move(handler)) {}

CommandPipeServer::~CommandPipeServer() {
    stop();
}

void CommandPipeServer::start() {
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&CommandPipeServer::run, this);
}

void CommandPipeServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    // The server thread may be blocked in ConnectNamedPipe. Nudge it by
    // briefly opening our own pipe. A single nudge can be lost if it fires
    // before the thread has (re)created the pipe, so retry until the thread
    // records that it has exited -- once the thread is parked in
    // ConnectNamedPipe the pipe exists and a nudge is guaranteed to connect.
    while (!threadExited_.load()) {
        HANDLE nudge = CreateFileW(kCommandPipeName, GENERIC_WRITE, 0, nullptr,
                                   OPEN_EXISTING, 0, nullptr);
        if (nudge != INVALID_HANDLE_VALUE) {
            CloseHandle(nudge);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

void CommandPipeServer::run() {
    while (running_.load()) {
        HANDLE pipe = CreateNamedPipeW(
            kCommandPipeName,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, // max instances: single UI client
            0, // out buffer size (unused, inbound only)
            static_cast<DWORD>(sizeof(CommandMessage) * 8),
            0, // default timeout
            nullptr);

        if (pipe == INVALID_HANDLE_VALUE) {
            break;
        }

        const bool connected = ConnectNamedPipe(pipe, nullptr) != 0
            || GetLastError() == ERROR_PIPE_CONNECTED;

        if (!running_.load()) {
            CloseHandle(pipe);
            break;
        }

        if (connected) {
            CommandMessage message{};
            DWORD bytesRead = 0;
            while (running_.load() &&
                   ReadFile(pipe, &message, sizeof(message), &bytesRead, nullptr) &&
                   bytesRead == sizeof(message)) {
                if (handler_) {
                    handler_(static_cast<CommandType>(message.type), message.arg);
                }
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
    threadExited_.store(true);
}

} // namespace redfox::ipc
