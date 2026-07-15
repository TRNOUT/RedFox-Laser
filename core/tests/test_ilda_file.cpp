#include <catch2/catch_test_macros.hpp>

#include "ilda/IldaCodec.hpp"

#include <cmath>
#include <filesystem>
#include <string>

using namespace redfox::ilda;

TEST_CASE("a show round-trips through an .ild file on disk", "[ilda][file]") {
    IldaFrame frame;
    frame.name = "FILE";
    frame.company = "REDFOX";
    frame.points = {
        {0.3f, -0.3f, 0.0f, 12, 34, 56, false},
        {-0.3f, 0.3f, 0.0f, 0, 0, 0, true},
    };

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "redfox_ilda_roundtrip.ild";

    REQUIRE(writeIldaFile(path.string(), {frame}));

    const ParseResult parsed = readIldaFile(path.string());
    REQUIRE(parsed.ok);
    REQUIRE(parsed.frames.size() == 1);
    REQUIRE(parsed.frames[0].name == "FILE");
    REQUIRE(parsed.frames[0].points.size() == 2);
    REQUIRE(parsed.frames[0].points[0].r == 12);
    REQUIRE(parsed.frames[0].points[0].g == 34);
    REQUIRE(parsed.frames[0].points[0].b == 56);
    REQUIRE(parsed.frames[0].points[1].blanked);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("reading a missing file fails cleanly", "[ilda][file]") {
    const ParseResult parsed = readIldaFile("this_file_does_not_exist_12345.ild");
    REQUIRE_FALSE(parsed.ok);
    REQUIRE_FALSE(parsed.error.empty());
}
