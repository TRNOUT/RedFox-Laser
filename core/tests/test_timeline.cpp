#include <catch2/catch_test_macros.hpp>

#include "show/Timeline.hpp"

using namespace redfox::show;

namespace {
Timeline makeTimeline(std::vector<TimelineStep> steps, double duration, bool loop) {
    Timeline tl;
    tl.steps = std::move(steps);
    tl.durationSeconds = duration;
    tl.loop = loop;
    return tl;
}
} // namespace

TEST_CASE("an empty timeline fires nothing", "[timeline]") {
    const Timeline tl = makeTimeline({}, 10.0, false);
    REQUIRE(stepsInInterval(tl, 0.0, 5.0).empty());
}

TEST_CASE("a step fires when the interval crosses its time", "[timeline]") {
    const Timeline tl = makeTimeline({{1.0, 3}}, 10.0, false);
    const auto fired = stepsInInterval(tl, 0.0, 1.0);
    REQUIRE(fired.size() == 1);
    REQUIRE(fired[0] == 0);
}

TEST_CASE("the interval is half-open so a step fires exactly once", "[timeline]") {
    const Timeline tl = makeTimeline({{1.0, 3}}, 10.0, false);
    // The step at t=1 belongs to (0,1], not to the next interval (1,2].
    REQUIRE(stepsInInterval(tl, 0.0, 1.0).size() == 1);
    REQUIRE(stepsInInterval(tl, 1.0, 2.0).empty());
}

TEST_CASE("a step exactly at the interval start does not fire", "[timeline]") {
    const Timeline tl = makeTimeline({{0.0, 0}}, 10.0, false);
    // (0,1] excludes 0 — so a step at t=0 needs the sequencer's first tick to
    // fire it via a (-eps, 0]-style first interval, not this call.
    REQUIRE(stepsInInterval(tl, 0.0, 1.0).empty());
    REQUIRE(stepsInInterval(tl, -0.001, 0.0).size() == 1);
}

TEST_CASE("multiple steps within one interval all fire, in order", "[timeline]") {
    const Timeline tl = makeTimeline({{0.5, 1}, {1.0, 2}, {1.5, 3}}, 10.0, false);
    const auto fired = stepsInInterval(tl, 0.0, 2.0);
    REQUIRE(fired.size() == 3);
    REQUIRE(fired[0] == 0);
    REQUIRE(fired[1] == 1);
    REQUIRE(fired[2] == 2);
}

TEST_CASE("steps outside the interval are excluded", "[timeline]") {
    const Timeline tl = makeTimeline({{0.5, 1}, {5.0, 2}}, 10.0, false);
    const auto fired = stepsInInterval(tl, 1.0, 4.0);
    REQUIRE(fired.empty());
}

TEST_CASE("a reversed or empty interval fires nothing", "[timeline]") {
    const Timeline tl = makeTimeline({{1.0, 1}}, 10.0, false);
    REQUIRE(stepsInInterval(tl, 2.0, 1.0).empty());
    REQUIRE(stepsInInterval(tl, 1.0, 1.0).empty());
}

TEST_CASE("timelineIsFinished reports completion for non-looping timelines", "[timeline]") {
    const Timeline once = makeTimeline({{1.0, 1}}, 5.0, false);
    REQUIRE_FALSE(timelineIsFinished(once, 4.9));
    REQUIRE(timelineIsFinished(once, 5.0));
    REQUIRE(timelineIsFinished(once, 6.0));

    const Timeline looping = makeTimeline({{1.0, 1}}, 5.0, true);
    REQUIRE_FALSE(timelineIsFinished(looping, 6.0)); // loops forever
}
