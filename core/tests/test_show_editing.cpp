#include <catch2/catch_test_macros.hpp>

#include "show/ShowEditing.hpp"

using namespace redfox::show;

namespace {
Show makeShow(std::size_t cueCount) {
    Show s;
    for (std::size_t i = 0; i < cueCount; ++i) {
        Cue c;
        c.name = "Cue" + std::to_string(i);
        s.cues.push_back(c);
    }
    return s;
}
} // namespace

TEST_CASE("removing a cue drops timeline steps that reference it", "[show][editing]") {
    Show s = makeShow(3);
    s.timeline.steps = {{0.0, 0}, {1.0, 1}, {2.0, 2}};

    removeCue(s, 1); // remove the middle cue

    REQUIRE(s.cues.size() == 2);
    REQUIRE(s.cues[0].name == "Cue0");
    REQUIRE(s.cues[1].name == "Cue2");

    // The step for the removed cue is gone; the step for the higher cue index
    // is shifted down by one so it still points at the same cue.
    REQUIRE(s.timeline.steps.size() == 2);
    REQUIRE(s.timeline.steps[0].cueIndex == 0);
    REQUIRE(s.timeline.steps[1].cueIndex == 1); // was cue 2, now at index 1
}

TEST_CASE("removing a cue keeps steps below it untouched", "[show][editing]") {
    Show s = makeShow(4);
    s.timeline.steps = {{0.0, 0}, {1.0, 0}, {2.0, 3}};

    removeCue(s, 3); // remove the last cue

    REQUIRE(s.cues.size() == 3);
    REQUIRE(s.timeline.steps.size() == 2); // the two cue-0 steps remain
    REQUIRE(s.timeline.steps[0].cueIndex == 0);
    REQUIRE(s.timeline.steps[1].cueIndex == 0);
}

TEST_CASE("removing an out-of-range cue is a no-op", "[show][editing]") {
    Show s = makeShow(2);
    s.timeline.steps = {{0.0, 1}};

    removeCue(s, 5);

    REQUIRE(s.cues.size() == 2);
    REQUIRE(s.timeline.steps.size() == 1);
    REQUIRE(s.timeline.steps[0].cueIndex == 1);
}

TEST_CASE("removing the only referenced cue empties matching steps", "[show][editing]") {
    Show s = makeShow(1);
    s.timeline.steps = {{0.0, 0}, {1.0, 0}};

    removeCue(s, 0);

    REQUIRE(s.cues.empty());
    REQUIRE(s.timeline.steps.empty());
}
