#pragma once
#include "Commands.hpp"
#include <atomic>
#include <functional>
#include <thread>

namespace redfox::ipc {

// Engine-side named-pipe server. Runs a dedicated blocking thread rather
// than overlapped I/O: command traffic is low-frequency (arm/e-stop/etc.,
// not per-frame data), so a simple blocking loop avoids overlapped I/O's
// bug surface for negligible cost. One client (the UI) at a time.
class CommandPipeServer {
public:
    using CommandHandler = std::function<void(CommandType, std::uint32_t)>;

    explicit CommandPipeServer(CommandHandler handler);
    ~CommandPipeServer();

    CommandPipeServer(const CommandPipeServer&) = delete;
    CommandPipeServer& operator=(const CommandPipeServer&) = delete;

    void start();
    void stop();

private:
    void run();

    CommandHandler handler_;
    std::atomic<bool> running_{false};
    std::atomic<bool> threadExited_{false};
    std::thread thread_;
};

} // namespace redfox::ipc
