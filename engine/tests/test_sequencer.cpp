#include <catch2/catch_test_macros.hpp>

#include "playback/Sequencer.hpp"
#include "safety/Clock.hpp"

#include <chrono>
#include <vector>

using namespace redfox::engine;
using namespace std::chrono;

namespace {
redfox::show::Timeline makeTimeline(std::vector<redfox::show::TimelineStep> steps,
                                    double duration, bool loop) {
    redfox::show::Timeline tl;
    tl.steps = std::move(steps);
    tl.durationSeconds = duration;
    tl.loop = loop;
    return tl;
}
} // namespace

TEST_CASE("a stopped sequencer never fires", "[sequencer]") {
    redfox::safety::FakeClock clock;
    Sequencer seq(clock);
    std::vector<std::size_t> fired;
    seq.setTriggerCallback([&](std::size_t i) { fired.push_back(i); });
    seq.setTimeline(makeTimeline({{0.0, 5}}, 10.0, false));

    clock.advance(seconds(1));
    seq.tick();
    REQUIRE(fired.empty());
    REQUIRE_FALSE(seq.isPlaying());
}

TEST_CASE("a step at t=0 fires on the first tick after start", "[sequencer]") {
    redfox::safety::FakeClock clock;
    Sequencer seq(clock);
    std::vector<std::size_t> fired;
    seq.setTriggerCallback([&](std::size_t i) { fired.push_back(i); });
    seq.setTimeline(makeTimeline({{0.0, 7}}, 10.0, false));

    seq.start();
    seq.tick(); // no time has passed, but the t=0 step is due immediately
    REQUIRE(fired == std::vector<std::size_t>{7});
}

TEST_CASE("steps fire once, in order, as time advances", "[sequencer]") {
    redfox::safety::FakeClock clock;
    Sequencer seq(clock);
    std::vector<std::size_t> fired;
    seq.setTriggerCallback([&](std::size_t i) { fired.push_back(i); });
    seq.setTimeline(makeTimeline({{1.0, 1}, {2.0, 2}, {3.0, 3}}, 10.0, false));

    seq.start();
    seq.tick(); // t=0: nothing due yet
    REQUIRE(fired.empty());

    clock.advance(milliseconds(1500)); // t=1.5
    seq.tick();
    REQUIRE(fired == std::vector<std::size_t>{1});

    clock.advance(milliseconds(1500)); // t=3.0
    seq.tick();
    REQUIRE(fired == std::vector<std::size_t>{1, 2, 3});

    clock.advance(milliseconds(500)); // t=3.5, nothing new
    seq.tick();
    REQUIRE(fired == std::vector<std::size_t>{1, 2, 3});
}

TEST_CASE("a non-looping sequence stops once past its duration", "[sequencer]") {
    redfox::safety::FakeClock clock;
    Sequencer seq(clock);
    std::vector<std::size_t> fired;
    seq.setTriggerCallback([&](std::size_t i) { fired.push_back(i); });
    seq.setTimeline(makeTimeline({{1.0, 1}}, 2.0, false));

    seq.start();
    clock.advance(milliseconds(2500)); // t=2.5 > duration
    seq.tick();
    REQUIRE(fired == std::vector<std::size_t>{1}); // the step still fired
    REQUIRE_FALSE(seq.isPlaying());                // and playback ended
}

TEST_CASE("a looping sequence re-fires its steps each cycle", "[sequencer]") {
    redfox::safety::FakeClock clock;
    Sequencer seq(clock);
    std::vector<std::size_t> fired;
    seq.setTriggerCallback([&](std::size_t i) { fired.push_back(i); });
    seq.setTimeline(makeTimeline({{0.5, 9}}, 2.0, true));

    seq.start();
    clock.advance(milliseconds(1000)); // t=1.0, first cycle
    seq.tick();
    REQUIRE(fired == std::vector<std::size_t>{9});

    clock.advance(milliseconds(2000)); // t=3.0 -> wrapped past duration 2.0
    seq.tick();
    REQUIRE(fired == std::vector<std::size_t>{9, 9}); // fired again next cycle
    REQUIRE(seq.isPlaying());                          // loops keep playing
}

TEST_CASE("stop() halts firing", "[sequencer]") {
    redfox::safety::FakeClock clock;
    Sequencer seq(clock);
    std::vector<std::size_t> fired;
    seq.setTriggerCallback([&](std::size_t i) { fired.push_back(i); });
    seq.setTimeline(makeTimeline({{1.0, 1}}, 10.0, false));

    seq.start();
    seq.stop();
    clock.advance(seconds(2));
    seq.tick();
    REQUIRE(fired.empty());
}
