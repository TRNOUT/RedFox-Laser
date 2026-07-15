#include <catch2/catch_test_macros.hpp>

#include "show/ShowFile.hpp"

#include <cmath>
#include <filesystem>

using namespace redfox;

namespace {
bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }

show::Show makeSampleShow() {
    show::Show s;
    s.name = "My Show";

    show::Cue square;
    square.name = "Square";
    square.framesPerSecond = 24.0f;
    square.loop = false;
    square.transform.offsetX = 0.1f;
    square.transform.scale = 0.5f;
    square.spinTurnsPerSec = 2.0f;
    ilda::IldaFrame f;
    f.points = {
        {0.5f, 0.5f, 0.0f, 255, 0, 0, false},
        {-0.5f, -0.5f, 0.0f, 0, 0, 0, true},
    };
    square.frames = {f};
    s.cues.push_back(square);

    show::Cue blink;
    blink.name = "Blink";
    blink.framesPerSecond = 8.0f;
    blink.loop = true;
    ilda::IldaFrame a;
    a.points = {{0.0f, 0.0f, 0.0f, 10, 20, 30, false}};
    ilda::IldaFrame b;
    b.points = {{0.1f, -0.1f, 0.0f, 40, 50, 60, false}};
    blink.frames = {a, b};
    s.cues.push_back(blink);

    s.timeline.name = "Main Sequence";
    s.timeline.steps = {{0.0, 0}, {2.5, 1}, {5.0, 0}};
    s.timeline.durationSeconds = 8.0;
    s.timeline.loop = true;

    return s;
}
} // namespace

TEST_CASE("a show round-trips through the binary format", "[show][file]") {
    const show::Show original = makeSampleShow();
    const show::ShowParseResult parsed = show::readShow(show::writeShow(original));

    REQUIRE(parsed.ok);
    REQUIRE(parsed.show.name == "My Show");
    REQUIRE(parsed.show.cues.size() == 2);

    const show::Cue& c0 = parsed.show.cues[0];
    REQUIRE(c0.name == "Square");
    REQUIRE(near(c0.framesPerSecond, 24.0f));
    REQUIRE_FALSE(c0.loop);
    REQUIRE(near(c0.transform.offsetX, 0.1f));
    REQUIRE(near(c0.transform.scale, 0.5f));
    REQUIRE(near(c0.spinTurnsPerSec, 2.0f));
    REQUIRE(c0.frames.size() == 1);
    REQUIRE(c0.frames[0].points.size() == 2);
    REQUIRE(c0.frames[0].points[0].r == 255);
    REQUIRE(c0.frames[0].points[1].blanked);

    const show::Cue& c1 = parsed.show.cues[1];
    REQUIRE(c1.name == "Blink");
    REQUIRE(near(c1.framesPerSecond, 8.0f));
    REQUIRE(c1.loop);
    REQUIRE(c1.frames.size() == 2);
    REQUIRE(c1.frames[1].points[0].g == 50);

    const show::Timeline& tl = parsed.show.timeline;
    REQUIRE(tl.name == "Main Sequence");
    REQUIRE(tl.loop);
    REQUIRE(near(static_cast<float>(tl.durationSeconds), 8.0f));
    REQUIRE(tl.steps.size() == 3);
    REQUIRE(near(static_cast<float>(tl.steps[1].timeSeconds), 2.5f));
    REQUIRE(tl.steps[1].cueIndex == 1);
    REQUIRE(tl.steps[2].cueIndex == 0);
}

TEST_CASE("a show round-trips through a file", "[show][file]") {
    const show::Show original = makeSampleShow();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "redfox_show_roundtrip.rfsh";

    REQUIRE(show::writeShowFile(path.string(), original));
    const show::ShowParseResult parsed = show::readShowFile(path.string());

    REQUIRE(parsed.ok);
    REQUIRE(parsed.show.cues.size() == 2);
    REQUIRE(parsed.show.cues[1].name == "Blink");

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("the default show path is usable and ends in show.rfsh", "[show][file]") {
    const std::string path = show::defaultShowFilePath();
    REQUIRE_FALSE(path.empty());
    REQUIRE(path.size() >= 9);
    REQUIRE(path.substr(path.size() - 9) == "show.rfsh");
    // Its directory must exist (created on demand) so a save can succeed.
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    REQUIRE((parent.empty() || std::filesystem::exists(parent)));
}

TEST_CASE("garbage is rejected and a missing file fails cleanly", "[show][file]") {
    const std::vector<std::uint8_t> junk = {'x', 'y', 'z'};
    REQUIRE_FALSE(show::readShow(junk).ok);

    const show::ShowParseResult missing = show::readShowFile("no_such_show_98765.rfsh");
    REQUIRE_FALSE(missing.ok);
    REQUIRE_FALSE(missing.error.empty());
}
