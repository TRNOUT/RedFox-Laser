#include <catch2/catch_test_macros.hpp>
#include "../src/ipc/CommandPipeServer.hpp"
#include "../src/ipc/CommandPipeClient.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace redfox::ipc;

namespace {
bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return predicate();
}
} // namespace

TEST_CASE("CommandPipeClient can send commands the server receives, in order", "[ipc]") {
    std::mutex receivedMutex;
    std::vector<CommandType> received;

    CommandPipeServer server([&](CommandType type, std::uint32_t) {
        std::lock_guard<std::mutex> lock(receivedMutex);
        received.push_back(type);
    });
    server.start();

    // Retry the client connect until the server thread is listening, rather
    // than relying on a fixed sleep (which can flake on a loaded machine).
    std::unique_ptr<CommandPipeClient> client;
    const auto connectDeadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    while (std::chrono::steady_clock::now() < connectDeadline) {
        client = std::make_unique<CommandPipeClient>();
        if (client->isConnected()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    REQUIRE(client);
    REQUIRE(client->isConnected());

    REQUIRE(client->send(CommandType::Arm));
    REQUIRE(client->send(CommandType::EmergencyStop));

    const bool gotBoth = waitUntil(
        [&] {
            std::lock_guard<std::mutex> lock(receivedMutex);
            return received.size() == 2;
        },
        std::chrono::milliseconds(1000));

    REQUIRE(gotBoth);

    std::lock_guard<std::mutex> lock(receivedMutex);
    REQUIRE(received[0] == CommandType::Arm);
    REQUIRE(received[1] == CommandType::EmergencyStop);
}
