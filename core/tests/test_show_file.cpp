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

TEST_CASE("garbage is rejected and a missing file fails cleanly", "[show][file]") {
    const std::vector<std::uint8_t> junk = {'x', 'y', 'z'};
    REQUIRE_FALSE(show::readShow(junk).ok);

    const show::ShowParseResult missing = show::readShowFile("no_such_show_98765.rfsh");
    REQUIRE_FALSE(missing.ok);
    REQUIRE_FALSE(missing.error.empty());
}
