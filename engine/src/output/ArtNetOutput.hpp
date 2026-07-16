#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace redfox::output {

// Streams DMX universes to an Art-Net node (e.g. the ShowNET box, or any
// Art-Net-capable lighting interface) over UDP. This is the open, licence-free
// path for the DMX side of a show: it carries 512-channel universes, not laser
// point streams. The sequence counter auto-increments so a receiver can detect
// dropped/out-of-order datagrams.
class ArtNetOutput {
public:
    ArtNetOutput();
    ~ArtNetOutput();
    ArtNetOutput(const ArtNetOutput&) = delete;
    ArtNetOutput& operator=(const ArtNetOutput&) = delete;

    // Point the sender at a target IP (and optionally a non-standard port).
    // Returns false if the socket or address could not be set up.
    bool open(const std::string& targetIp, std::uint16_t port = 6454);
    bool isOpen() const;

    // Send one universe as an ArtDMX datagram. Returns false on a socket error.
    bool sendUniverse(std::uint8_t net, std::uint8_t subUni, const std::uint8_t* data,
                      std::uint16_t length);

    void close();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace redfox::output
