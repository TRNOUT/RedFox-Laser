#include <catch2/catch_test_macros.hpp>
#include "../src/ipc/TelemetryHost.hpp"
#include "../src/ipc/TelemetryClient.hpp"

using namespace redfox::ipc;

TEST_CASE("TelemetryClient reads what TelemetryHost writes, and vice versa", "[ipc]") {
    TelemetryHost host;
    REQUIRE(host.isValid());

    TelemetryClient client;
    REQUIRE(client.isValid());

    host.telemetry().engineHeartbeatEpochMs.store(123456789ULL);
    host.telemetry().safetyState.store(static_cast<std::uint32_t>(SafetyStateWire::Armed));

    REQUIRE(client.telemetry().engineHeartbeatEpochMs.load() == 123456789ULL);
    REQUIRE(client.telemetry().safetyState.load() ==
            static_cast<std::uint32_t>(SafetyStateWire::Armed));

    client.telemetry().uiHeartbeatEpochMs.store(987654321ULL);
    REQUIRE(host.telemetry().uiHeartbeatEpochMs.load() == 987654321ULL);
}
