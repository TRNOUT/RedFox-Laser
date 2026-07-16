#include <catch2/catch_test_macros.hpp>

#include "output/ArtNetOutput.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstring>
#include <vector>

using namespace redfox::output;

TEST_CASE("ArtNetOutput sends a real ArtDMX datagram over UDP", "[artnet][socket]") {
    WSADATA wsa;
    REQUIRE(WSAStartup(MAKEWORD(2, 2), &wsa) == 0);

    // A loopback receiver on an ephemeral port.
    SOCKET rx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    REQUIRE(rx != INVALID_SOCKET);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    addr.sin_port = 0; // let the OS choose
    REQUIRE(bind(rx, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);

    int addrLen = sizeof(addr);
    REQUIRE(getsockname(rx, reinterpret_cast<sockaddr*>(&addr), &addrLen) == 0);
    const std::uint16_t port = ntohs(addr.sin_port);

    // Don't let a lost datagram hang the test.
    DWORD timeoutMs = 1000;
    setsockopt(rx, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs),
               sizeof(timeoutMs));

    ArtNetOutput out;
    REQUIRE(out.open("127.0.0.1", port));
    REQUIRE(out.isOpen());

    const std::uint8_t channels[3] = {255, 128, 64};
    REQUIRE(out.sendUniverse(/*net=*/1, /*subUni=*/2, channels, 3));

    std::vector<char> buf(600, 0);
    const int received = recv(rx, buf.data(), static_cast<int>(buf.size()), 0);
    REQUIRE(received > 0);
    REQUIRE(std::memcmp(buf.data(), "Art-Net\0", 8) == 0);
    REQUIRE(static_cast<std::uint8_t>(buf[15]) == 1);  // net
    REQUIRE(static_cast<std::uint8_t>(buf[14]) == 2);  // subUni
    REQUIRE(static_cast<std::uint8_t>(buf[17]) == 4);  // length padded 3 -> 4
    REQUIRE(static_cast<std::uint8_t>(buf[18]) == 255); // channels
    REQUIRE(static_cast<std::uint8_t>(buf[19]) == 128);
    REQUIRE(static_cast<std::uint8_t>(buf[20]) == 64);

    out.close();
    REQUIRE_FALSE(out.isOpen());
    closesocket(rx);
    WSACleanup();
}

TEST_CASE("opening an invalid address fails cleanly", "[artnet][socket]") {
    ArtNetOutput out;
    REQUIRE_FALSE(out.open("not-an-ip"));
    REQUIRE_FALSE(out.isOpen());
    REQUIRE_FALSE(out.sendUniverse(0, 0, nullptr, 0));
}
