#include "output/ArtNetOutput.hpp"

#include "output/ArtNetPacket.hpp"

// winsock2.h must precede windows.h; this TU pulls it in directly.
#include <winsock2.h>
#include <ws2tcpip.h>

namespace redfox::output {

struct ArtNetOutput::Impl {
    bool wsaStarted = false;
    SOCKET sock = INVALID_SOCKET;
    sockaddr_in target{};
    std::uint8_t sequence = 0;

    ~Impl() { closeSocket(); }

    void closeSocket() {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        if (wsaStarted) {
            WSACleanup();
            wsaStarted = false;
        }
    }
};

ArtNetOutput::ArtNetOutput() : impl_(std::make_unique<Impl>()) {}

ArtNetOutput::~ArtNetOutput() = default;

bool ArtNetOutput::open(const std::string& targetIp, std::uint16_t port) {
    close();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    impl_->wsaStarted = true;

    impl_->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (impl_->sock == INVALID_SOCKET) {
        impl_->closeSocket();
        return false;
    }

    impl_->target = {};
    impl_->target.sin_family = AF_INET;
    impl_->target.sin_port = htons(port);
    if (inet_pton(AF_INET, targetIp.c_str(), &impl_->target.sin_addr) != 1) {
        impl_->closeSocket();
        return false;
    }
    impl_->sequence = 0;
    return true;
}

bool ArtNetOutput::isOpen() const { return impl_->sock != INVALID_SOCKET; }

bool ArtNetOutput::sendUniverse(std::uint8_t net, std::uint8_t subUni,
                                const std::uint8_t* data, std::uint16_t length) {
    if (!isOpen()) {
        return false;
    }
    // Sequence 0 means "disabled"; keep it in 1..255 for a live stream.
    if (++impl_->sequence == 0) {
        impl_->sequence = 1;
    }
    const std::vector<std::uint8_t> packet =
        buildArtDmx(net, subUni, impl_->sequence, data, length);
    const int sent = sendto(impl_->sock, reinterpret_cast<const char*>(packet.data()),
                            static_cast<int>(packet.size()), 0,
                            reinterpret_cast<const sockaddr*>(&impl_->target),
                            sizeof(impl_->target));
    return sent == static_cast<int>(packet.size());
}

void ArtNetOutput::close() { impl_->closeSocket(); }

} // namespace redfox::output
