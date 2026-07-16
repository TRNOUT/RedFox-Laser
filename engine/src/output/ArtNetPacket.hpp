#pragma once

#include <cstdint>
#include <vector>

namespace redfox::output {

// Build an Art-Net ArtDMX (OpOutput) packet carrying one DMX universe.
//
// `net` (top 7 bits) and `subUni` (low byte) together form the 15-bit Art-Net
// port address; `sequence` lets a receiver detect out-of-order UDP (0 disables
// it). `data`/`length` are the DMX channel values (0..512). Art-Net requires an
// even slot count of at least 2, so an odd length is padded up by one zero and
// the whole thing is clamped to a 512-channel universe. The returned bytes are
// ready to send as a single UDP datagram to port 6454.
std::vector<std::uint8_t> buildArtDmx(std::uint8_t net, std::uint8_t subUni,
                                      std::uint8_t sequence, const std::uint8_t* data,
                                      std::uint16_t length);

} // namespace redfox::output
