#include "output/ArtNetPacket.hpp"

#include <algorithm>
#include <cstring>

namespace redfox::output {

namespace {
constexpr std::uint16_t kMaxSlots = 512;
constexpr std::uint16_t kMinSlots = 2;
constexpr std::size_t kHeaderSize = 18;
} // namespace

std::vector<std::uint8_t> buildArtDmx(std::uint8_t net, std::uint8_t subUni,
                                      std::uint8_t sequence, const std::uint8_t* data,
                                      std::uint16_t length) {
    // Art-Net wants an even slot count, at least 2 and at most a full universe.
    std::uint16_t slots = std::min(length, kMaxSlots);
    if (slots < kMinSlots) {
        slots = kMinSlots;
    }
    if (slots % 2 != 0) {
        ++slots;
    }

    std::vector<std::uint8_t> pkt(kHeaderSize + slots, 0);
    std::memcpy(pkt.data(), "Art-Net\0", 8);
    pkt[8] = 0x00;  // OpCode 0x5000 (OpOutput), little-endian
    pkt[9] = 0x50;
    pkt[10] = 0x00; // protocol version 14, big-endian
    pkt[11] = 0x0e;
    pkt[12] = sequence;
    pkt[13] = 0;      // physical (informational only)
    pkt[14] = subUni; // low byte of the 15-bit port address
    pkt[15] = net;    // high 7 bits
    pkt[16] = static_cast<std::uint8_t>((slots >> 8) & 0xFF); // length, big-endian
    pkt[17] = static_cast<std::uint8_t>(slots & 0xFF);

    // Copy the caller's channels (never more than they supplied); any pad slots
    // stay zero.
    const std::uint16_t toCopy = std::min(length, slots);
    if (data != nullptr && toCopy > 0) {
        std::memcpy(pkt.data() + kHeaderSize, data, toCopy);
    }
    return pkt;
}

} // namespace redfox::output
