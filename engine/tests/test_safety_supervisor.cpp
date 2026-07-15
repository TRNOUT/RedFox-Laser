#include <catch2/catch_test_macros.hpp>

#include "../src/safety/Clock.hpp"
#include "../src/safety/SafetySupervisor.hpp"
#include "../src/output/MockLaserOutput.hpp"

using namespace redfox::safety;
using namespace redfox::output;

TEST_CASE("SafetySupervisor starts blanked", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    SafetySupervisor supervisor(clock, config);

    auto output = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(output);

    REQUIRE(supervisor.state() == SafetyState::Blanked);
    REQUIRE_FALSE(output->isArmed());
}

TEST_CASE("requestArm arms all registered outputs", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    SafetySupervisor supervisor(clock, config);

    auto output = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(output);

    REQUIRE(supervisor.requestArm());
    REQUIRE(supervisor.state() == SafetyState::Armed);
    REQUIRE(output->isArmed());
}

TEST_CASE("tick blanks outputs after the UI heartbeat times out", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    config.uiHeartbeatTimeout = std::chrono::milliseconds(250);
    config.frameStallTimeout = std::chrono::milliseconds(10000); // isolate heartbeat behaviour
    SafetySupervisor supervisor(clock, config);

    auto output = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(output);

    supervisor.requestArm();
    supervisor.notifyUiHeartbeat();

    clock.advance(std::chrono::milliseconds(100));
    supervisor.tick();
    REQUIRE(supervisor.state() == SafetyState::Armed);

    clock.advance(std::chrono::milliseconds(200)); // total 300ms since last heartbeat
    supervisor.tick();
    REQUIRE(supervisor.state() == SafetyState::Blanked);
    REQUIRE_FALSE(output->isArmed());
    REQUIRE(output->blankCount() >= 1);
}

TEST_CASE("tick blanks outputs after a frame stall", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    config.uiHeartbeatTimeout = std::chrono::milliseconds(10000); // isolate frame-stall behaviour
    config.frameStallTimeout = std::chrono::milliseconds(200);
    SafetySupervisor supervisor(clock, config);

    auto output = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(output);

    supervisor.requestArm();
    supervisor.notifyUiHeartbeat();
    supervisor.notifyFrameProduced();

    clock.advance(std::chrono::milliseconds(300));
    supervisor.tick();

    REQUIRE(supervisor.state() == SafetyState::Blanked);
}

TEST_CASE("emergency stop latches until explicitly cleared", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    SafetySupervisor supervisor(clock, config);

    auto output = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(output);

    supervisor.requestArm();
    supervisor.triggerEmergencyStop();

    REQUIRE_FALSE(output->isArmed());
    REQUIRE(output->blankCount() >= 1);
    REQUIRE(supervisor.state() == SafetyState::EStopLatched);
    REQUIRE_FALSE(supervisor.requestArm()); // arming must be refused while latched
    REQUIRE(supervisor.state() == SafetyState::EStopLatched);

    REQUIRE(supervisor.clearEmergencyStop());
    REQUIRE(supervisor.state() == SafetyState::Blanked);

    REQUIRE(supervisor.requestArm());
    REQUIRE(supervisor.state() == SafetyState::Armed);
}

TEST_CASE("event callback fires with the tripping reason", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    config.uiHeartbeatTimeout = std::chrono::milliseconds(250);
    config.frameStallTimeout = std::chrono::milliseconds(10000);
    SafetySupervisor supervisor(clock, config);

    auto output = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(output);

    supervisor.requestArm();

    // Register the callback AFTER arming: requestArm() itself emits a
    // ManualArm event, so registering earlier would capture that too.
    // This test's scope is the trip reason.
    std::vector<SafetyEventCode> events;
    supervisor.setEventCallback([&events](SafetyEventCode code, SafetyState) {
        events.push_back(code);
    });

    supervisor.notifyUiHeartbeat();
    clock.advance(std::chrono::milliseconds(300));
    supervisor.tick();

    REQUIRE(events.size() == 1);
    REQUIRE(events.front() == SafetyEventCode::UiHeartbeatLost);
}

TEST_CASE("a trip disarms and blanks every registered output", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    config.uiHeartbeatTimeout = std::chrono::milliseconds(250);
    config.frameStallTimeout = std::chrono::milliseconds(10000);
    SafetySupervisor supervisor(clock, config);

    auto outputA = std::make_shared<MockLaserOutput>();
    auto outputB = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(outputA);
    supervisor.registerOutput(outputB);

    supervisor.requestArm();
    supervisor.notifyUiHeartbeat();
    REQUIRE(outputA->isArmed());
    REQUIRE(outputB->isArmed());

    clock.advance(std::chrono::milliseconds(300));
    supervisor.tick();

    REQUIRE(supervisor.state() == SafetyState::Blanked);
    REQUIRE_FALSE(outputA->isArmed());
    REQUIRE_FALSE(outputB->isArmed());
    REQUIRE(outputA->blankCount() >= 1);
    REQUIRE(outputB->blankCount() >= 1);
}

TEST_CASE("an armed engine with heartbeats but no frames does not trip on frame-stall", "[safety]") {
    FakeClock clock;
    SafetySupervisorConfig config;
    config.uiHeartbeatTimeout = std::chrono::milliseconds(250);
    config.frameStallTimeout = std::chrono::milliseconds(200);
    SafetySupervisor supervisor(clock, config);

    auto output = std::make_shared<MockLaserOutput>();
    supervisor.registerOutput(output);

    supervisor.requestArm();

    // No frames are ever produced (no frame generator in this milestone).
    // Keep the UI heartbeat fresh and advance well past frameStallTimeout;
    // the frame-stall watchdog must stay dormant until the first real frame,
    // so the supervisor must remain Armed.
    for (int i = 0; i < 10; ++i) {
        clock.advance(std::chrono::milliseconds(100));
        supervisor.notifyUiHeartbeat();
        supervisor.tick();
    }

    REQUIRE(supervisor.state() == SafetyState::Armed);
    REQUIRE(output->isArmed());

    // Once frames DO start and then stop, the frame-stall watchdog trips.
    supervisor.notifyFrameProduced();
    clock.advance(std::chrono::milliseconds(300));
    supervisor.tick();
    REQUIRE(supervisor.state() == SafetyState::Blanked);
}
