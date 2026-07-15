#include <catch2/catch_test_macros.hpp>

#include "ilda/IldaCodec.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace redfox::ilda;

namespace {

void putU16BE(std::vector<std::uint8_t>& b, std::uint16_t v) {
    b.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void putI16BE(std::vector<std::uint8_t>& b, std::int16_t v) {
    putU16BE(b, static_cast<std::uint16_t>(v));
}

void putName(std::vector<std::uint8_t>& b, const std::string& s) {
    for (int i = 0; i < 8; ++i) {
        b.push_back(i < static_cast<int>(s.size()) ? static_cast<std::uint8_t>(s[i]) : 0);
    }
}

void appendHeader(std::vector<std::uint8_t>& b, std::uint8_t format,
                  const std::string& name, std::uint16_t records) {
    b.push_back('I');
    b.push_back('L');
    b.push_back('D');
    b.push_back('A');
    b.push_back(0);
    b.push_back(0);
    b.push_back(0);
    b.push_back(format);
    putName(b, name);
    putName(b, std::string());
    putU16BE(b, records);
    putU16BE(b, 0);
    putU16BE(b, 1);
    b.push_back(0);
    b.push_back(0);
}

void appendEof(std::vector<std::uint8_t>& b) { appendHeader(b, 1, std::string(), 0); }

} // namespace

TEST_CASE("a 2D indexed frame resolves colours from the default palette", "[ilda]") {
    std::vector<std::uint8_t> b;
    appendHeader(b, 1, "IDX", 2); // format 1 (2D indexed), 2 points
    // point 0: default-palette index 2 = red (255,0,0)
    putI16BE(b, 0);
    putI16BE(b, 0);
    b.push_back(0x00); // status
    b.push_back(2);    // colour index
    // point 1: index 4 = green (0,255,0), last point + blanked
    putI16BE(b, 100);
    putI16BE(b, -100);
    b.push_back(0xC0); // last-point (0x80) | blanking (0x40)
    b.push_back(4);
    appendEof(b);

    const ParseResult r = readIlda(b);
    REQUIRE(r.ok);
    REQUIRE(r.frames.size() == 1);
    REQUIRE(r.frames[0].points.size() == 2);

    REQUIRE(r.frames[0].points[0].r == 255);
    REQUIRE(r.frames[0].points[0].g == 0);
    REQUIRE(r.frames[0].points[0].b == 0);

    REQUIRE(r.frames[0].points[1].r == 0);
    REQUIRE(r.frames[0].points[1].g == 255);
    REQUIRE(r.frames[0].points[1].b == 0);
    REQUIRE(r.frames[0].points[1].blanked);
}

TEST_CASE("a format-2 palette section overrides the default palette", "[ilda]") {
    std::vector<std::uint8_t> b;
    appendHeader(b, 2, "PAL", 2); // palette with 2 entries
    b.push_back(10);
    b.push_back(20);
    b.push_back(30); // index 0
    b.push_back(40);
    b.push_back(50);
    b.push_back(60); // index 1
    appendHeader(b, 1, "IDX", 2); // 2D indexed frame using that palette
    putI16BE(b, 0);
    putI16BE(b, 0);
    b.push_back(0x00);
    b.push_back(0); // index 0 -> (10,20,30)
    putI16BE(b, 0);
    putI16BE(b, 0);
    b.push_back(0x80);
    b.push_back(1); // index 1 -> (40,50,60), last
    appendEof(b);

    const ParseResult r = readIlda(b);
    REQUIRE(r.ok);
    REQUIRE(r.frames.size() == 1);
    REQUIRE(r.frames[0].points[0].r == 10);
    REQUIRE(r.frames[0].points[0].g == 20);
    REQUIRE(r.frames[0].points[0].b == 30);
    REQUIRE(r.frames[0].points[1].r == 40);
    REQUIRE(r.frames[0].points[1].g == 50);
    REQUIRE(r.frames[0].points[1].b == 60);
}
