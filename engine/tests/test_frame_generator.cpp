#include <catch2/catch_test_macros.hpp>

#include "playback/FrameGenerator.hpp"
#include "safety/Clock.hpp"

#include <chrono>
#include <cmath>
#include <memory>

using namespace redfox;
using namespace std::chrono_literals;

namespace {

bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

std::shared_ptr<show::Show> makeShow() { return std::make_shared<show::Show>(); }

ilda::IldaFrame frameWithColour(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    ilda::IldaFrame f;
    f.points.push_back(ilda::IldaPoint{0.0f, 0.0f, 0.0f, r, g, b, false});
    return f;
}

} // namespace

TEST_CASE("no active cue produces no frame", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    REQUIRE_FALSE(gen.hasActiveCue());
    std::vector<output::OutputPoint> pts;
    REQUIRE_FALSE(gen.currentFrame(pts));
}

TEST_CASE("triggering a still cue outputs its converted points", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    ilda::IldaFrame f;
    f.points = {
        {0.5f, -0.5f, 0.0f, 255, 0, 0, false},
        {0.0f, 0.0f, 0.0f, 200, 200, 200, true}, // blanked
    };
    cue.frames = {f};
    show->cues.push_back(cue);

    gen.setShow(show);
    gen.triggerCue(0);
    REQUIRE(gen.hasActiveCue());

    std::vector<output::OutputPoint> pts;
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(pts.size() == 2);

    REQUIRE(near(pts[0].x, 0.5f));
    REQUIRE(near(pts[0].y, -0.5f));
    REQUIRE(near(pts[0].r, 1.0f));
    REQUIRE(near(pts[0].g, 0.0f));

    // A blanked point emits no colour regardless of its stored RGB.
    REQUIRE(near(pts[1].r, 0.0f));
    REQUIRE(near(pts[1].g, 0.0f));
    REQUIRE(near(pts[1].b, 0.0f));
}

TEST_CASE("a looping animation advances frames over time", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    cue.framesPerSecond = 10.0f; // 0.1s per frame
    cue.loop = true;
    cue.frames = {frameWithColour(255, 0, 0), frameWithColour(0, 255, 0)};
    show->cues.push_back(cue);

    gen.setShow(show);
    gen.triggerCue(0);

    std::vector<output::OutputPoint> pts;
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].r, 1.0f)); // frame 0 (red) at t=0
    REQUIRE(near(pts[0].g, 0.0f));

    clock.advance(100ms);
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].r, 0.0f)); // frame 1 (green) at t=0.1s
    REQUIRE(near(pts[0].g, 1.0f));

    clock.advance(100ms);
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].r, 1.0f)); // wrapped back to frame 0
}

TEST_CASE("stop() clears the active cue", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    cue.frames = {frameWithColour(10, 20, 30)};
    show->cues.push_back(cue);
    gen.setShow(show);
    gen.triggerCue(0);
    REQUIRE(gen.hasActiveCue());

    gen.stop();
    REQUIRE_FALSE(gen.hasActiveCue());
    std::vector<output::OutputPoint> pts;
    REQUIRE_FALSE(gen.currentFrame(pts));
}

TEST_CASE("a cue's base transform is applied to output points", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    cue.transform.offsetX = 0.2f;
    cue.transform.offsetY = -0.1f;
    ilda::IldaFrame f;
    f.points = {{0.3f, 0.4f, 0.0f, 255, 0, 0, false}};
    cue.frames = {f};
    show->cues.push_back(cue);

    gen.setShow(show);
    gen.triggerCue(0);

    std::vector<output::OutputPoint> pts;
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].x, 0.5f));
    REQUIRE(near(pts[0].y, 0.3f));
}

TEST_CASE("cue spin rotates the frame over time", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    cue.spinTurnsPerSec = 0.25f; // quarter turn per second
    ilda::IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 10, 10, 10, false}};
    cue.frames = {f};
    show->cues.push_back(cue);

    gen.setShow(show);
    gen.triggerCue(0);

    std::vector<output::OutputPoint> pts;
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].x, 1.0f)); // t=0: no rotation

    clock.advance(1000ms); // +0.25 turn: (1,0) -> (0,1)
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].x, 0.0f));
    REQUIRE(near(pts[0].y, 1.0f));
}

TEST_CASE("a cue's motion effect animates the frame over time", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    // A point at (1,0); a rotate effect turns it a quarter turn per second.
    ilda::IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 10, 10, 10, false}};
    cue.frames = {f};
    effects::Effect rot;
    rot.type = effects::EffectType::Rotate;
    rot.direction = effects::Direction::Forward;
    rot.rateHz = 0.25f;
    cue.effects = {rot};
    show->cues.push_back(cue);

    gen.setShow(show);
    gen.triggerCue(0);

    std::vector<output::OutputPoint> pts;
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].x, 1.0f)); // t=0: no rotation yet

    clock.advance(1000ms); // +0.25 turn: (1,0) -> (0,1)
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].x, 0.0f));
    REQUIRE(near(pts[0].y, 1.0f));
}

TEST_CASE("a tempo-synced effect follows the master BPM", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    ilda::IldaFrame f;
    f.points = {{1.0f, 0.0f, 0.0f, 10, 10, 10, false}};
    cue.frames = {f};
    effects::Effect rot;
    rot.type = effects::EffectType::Rotate;
    rot.direction = effects::Direction::Forward;
    rot.rateHz = 99.0f;        // ignored while synced
    rot.syncToTempo = true;
    cue.effects = {rot};
    show->cues.push_back(cue);

    gen.setShow(show);
    gen.setMasterBpm(15.0f); // 15 BPM = 0.25 Hz -> quarter turn per second
    gen.triggerCue(0);

    std::vector<output::OutputPoint> pts;
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].x, 1.0f)); // t=0

    clock.advance(1000ms); // one second at 0.25 Hz -> (1,0) -> (0,1)
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].x, 0.0f));
    REQUIRE(near(pts[0].y, 1.0f));
}

TEST_CASE("a strobe effect can blank the whole frame", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);

    auto show = makeShow();
    show::Cue cue;
    cue.frames = {frameWithColour(255, 255, 255)};
    effects::Effect strobe;
    strobe.type = effects::EffectType::Strobe;
    strobe.rateHz = 1.0f;
    strobe.amount = 0.5f; // on for first half of each 1s cycle
    cue.effects = {strobe};
    show->cues.push_back(cue);

    gen.setShow(show);
    gen.triggerCue(0);

    std::vector<output::OutputPoint> pts;
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].r, 1.0f)); // t=0 phase 0 -> on

    clock.advance(750ms); // phase 0.75 >= 0.5 -> blanked
    REQUIRE(gen.currentFrame(pts));
    REQUIRE(near(pts[0].r, 0.0f));
    REQUIRE(near(pts[0].g, 0.0f));
    REQUIRE(near(pts[0].b, 0.0f));
}

TEST_CASE("triggering an out-of-range cue index is ignored", "[playback]") {
    safety::FakeClock clock;
    engine::FrameGenerator gen(clock);
    gen.setShow(makeShow()); // empty show
    gen.triggerCue(3);
    REQUIRE_FALSE(gen.hasActiveCue());
}
