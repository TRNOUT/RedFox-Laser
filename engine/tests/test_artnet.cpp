#include <catch2/catch_test_macros.hpp>

#include "output/ArtNetPacket.hpp"

#include <cstring>

using namespace redfox::output;

TEST_CASE("an ArtDMX packet has the correct Art-Net header", "[artnet]") {
    const std::uint8_t data[4] = {10, 20, 30, 40};
    const std::vector<std::uint8_t> pkt = buildArtDmx(/*net=*/2, /*subUni=*/5,
                                                      /*sequence=*/7, data, 4);

    // "Art-Net\0" identifier.
    REQUIRE(std::memcmp(pkt.data(), "Art-Net\0", 8) == 0);
    // OpCode 0x5000 (OpOutput/ArtDMX), transmitted little-endian.
    REQUIRE(pkt[8] == 0x00);
    REQUIRE(pkt[9] == 0x50);
    // Protocol version 14, big-endian.
    REQUIRE(pkt[10] == 0x00);
    REQUIRE(pkt[11] == 0x0e);
    // Sequence, physical.
    REQUIRE(pkt[12] == 7);
    REQUIRE(pkt[13] == 0);
    // SubUni (low) then Net (high).
    REQUIRE(pkt[14] == 5);
    REQUIRE(pkt[15] == 2);
    // Length, big-endian.
    REQUIRE(pkt[16] == 0x00);
    REQUIRE(pkt[17] == 4);
    // Payload.
    REQUIRE(pkt[18] == 10);
    REQUIRE(pkt[19] == 20);
    REQUIRE(pkt[20] == 30);
    REQUIRE(pkt[21] == 40);
    REQUIRE(pkt.size() == 18 + 4);
}

TEST_CASE("an odd data length is padded up to the next even number", "[artnet]") {
    const std::uint8_t data[3] = {1, 2, 3};
    const std::vector<std::uint8_t> pkt = buildArtDmx(0, 0, 0, data, 3);

    // Art-Net requires an even slot count; length becomes 4 and the pad is 0.
    REQUIRE(pkt[16] == 0x00);
    REQUIRE(pkt[17] == 4);
    REQUIRE(pkt[18] == 1);
    REQUIRE(pkt[19] == 2);
    REQUIRE(pkt[20] == 3);
    REQUIRE(pkt[21] == 0);
    REQUIRE(pkt.size() == 18 + 4);
}

TEST_CASE("length is clamped to a full universe and a minimum of two", "[artnet]") {
    std::vector<std::uint8_t> big(1000, 0xAB);
    const std::vector<std::uint8_t> pkt = buildArtDmx(0, 0, 0, big.data(), 1000);
    REQUIRE(pkt[16] == 0x02); // 512 = 0x0200
    REQUIRE(pkt[17] == 0x00);
    REQUIRE(pkt.size() == 18 + 512);

    const std::uint8_t one = 9;
    const std::vector<std::uint8_t> tiny = buildArtDmx(0, 0, 0, &one, 1);
    REQUIRE(pkt.size() >= 18 + 2);
    REQUIRE(tiny[17] == 2);   // minimum length is 2
    REQUIRE(tiny[18] == 9);
    REQUIRE(tiny[19] == 0);
}

TEST_CASE("a high length byte encodes big-endian correctly", "[artnet]") {
    std::vector<std::uint8_t> data(300, 1);
    const std::vector<std::uint8_t> pkt = buildArtDmx(0, 0, 0, data.data(), 300);
    // 300 = 0x012C
    REQUIRE(pkt[16] == 0x01);
    REQUIRE(pkt[17] == 0x2C);
    REQUIRE(pkt.size() == 18 + 300);
}
