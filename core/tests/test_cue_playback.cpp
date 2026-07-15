#include <catch2/catch_test_macros.hpp>

#include "show/CuePlayback.hpp"

using namespace redfox::show;

namespace {
Cue makeCue(std::size_t frameCount, float fps, bool loop) {
    Cue cue;
    cue.framesPerSecond = fps;
    cue.loop = loop;
    cue.frames.resize(frameCount); // content doesn't matter for index selection
    return cue;
}
} // namespace

TEST_CASE("an empty cue always resolves to frame 0", "[show]") {
    Cue cue;
    REQUIRE(cueFrameIndexAt(cue, 0.0) == 0);
    REQUIRE(cueFrameIndexAt(cue, 5.0) == 0);
}

TEST_CASE("a single-frame cue always shows frame 0", "[show]") {
    const Cue cue = makeCue(1, 30.0f, true);
    REQUIRE(cueFrameIndexAt(cue, 0.0) == 0);
    REQUIRE(cueFrameIndexAt(cue, 100.0) == 0);
}

TEST_CASE("a looping animation wraps modulo the frame count", "[show]") {
    const Cue cue = makeCue(3, 10.0f, true); // 3 frames at 10 fps -> 0.1s each
    REQUIRE(cueFrameIndexAt(cue, 0.00) == 0);
    REQUIRE(cueFrameIndexAt(cue, 0.05) == 0);
    REQUIRE(cueFrameIndexAt(cue, 0.10) == 1);
    REQUIRE(cueFrameIndexAt(cue, 0.25) == 2);
    REQUIRE(cueFrameIndexAt(cue, 0.30) == 0); // wrapped
    REQUIRE(cueFrameIndexAt(cue, 0.40) == 1);
}

TEST_CASE("a non-looping animation holds the last frame", "[show]") {
    const Cue cue = makeCue(3, 10.0f, false);
    REQUIRE(cueFrameIndexAt(cue, 0.00) == 0);
    REQUIRE(cueFrameIndexAt(cue, 0.10) == 1);
    REQUIRE(cueFrameIndexAt(cue, 0.20) == 2);
    REQUIRE(cueFrameIndexAt(cue, 0.30) == 2); // held
    REQUIRE(cueFrameIndexAt(cue, 99.0) == 2);
}

TEST_CASE("negative or zero elapsed time resolves to frame 0", "[show]") {
    const Cue cue = makeCue(4, 25.0f, true);
    REQUIRE(cueFrameIndexAt(cue, 0.0) == 0);
    REQUIRE(cueFrameIndexAt(cue, -1.0) == 0);
}
