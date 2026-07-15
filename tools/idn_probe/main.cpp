// Does a laser DAC answer to IDN (ILDA Digital Network) discovery on this
// network? Only the IDN controller manager is registered, so a positive result
// can only mean an IDN-speaking device replied -- not an EtherDream/Helios/
// LaserCube/AVB device on the same segment.
//
// SAFETY: before running this against real hardware, ensure the projector is
// aimed at a safe backstop, the key/interlock are under your control, and no one
// is in the beam path. This program only emits light if you explicitly answer
// 'y' to the prompt, and then only a dim (10%), 5-second, static test pattern.

#include "libera/System.hpp"
#include "libera/idn/IdnManager.hpp"
#include "libera/idn/IdnControllerInfo.hpp"
#include "libera/core/LaserController.hpp"
#include "libera/core/LaserPoint.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace {

libera::core::Frame makeDimTestCircle() {
    libera::core::Frame frame;
    constexpr std::size_t pointCount = 200;
    constexpr float radius = 0.5f;
    constexpr float brightness = 0.1f; // deliberately dim for a first hardware test

    frame.points.reserve(pointCount);
    const float tau = 2.0f * static_cast<float>(std::acos(-1.0));
    for (std::size_t i = 0; i < pointCount; ++i) {
        const float angle = tau * static_cast<float>(i) / static_cast<float>(pointCount);
        frame.points.push_back(libera::core::LaserPoint{
            radius * std::cos(angle),
            radius * std::sin(angle),
            brightness, brightness, brightness
        });
    }
    return frame;
}

} // namespace

int main() {
    libera::System liberaSystem;

    std::cout << "Searching for IDN controllers for up to 10 seconds...\n";

    std::vector<std::unique_ptr<libera::core::ControllerInfo>> found;
    for (int attempt = 0; attempt < 20 && found.empty(); ++attempt) {
        found = liberaSystem.discoverControllers();
        if (found.empty()) {
            std::this_thread::sleep_for(500ms);
        }
    }

    if (found.empty()) {
        std::cout << "No IDN controller responded on this network.\n";
        liberaSystem.shutdown();
        return 1;
    }

    const auto& info = *found.front();
    std::cout << "Found an IDN controller: " << info.labelValue()
              << " (type=" << info.type() << ")\n";
    if (const auto& net = info.networkInfo()) {
        std::cout << "  IP: " << net->ip << " port " << net->port << "\n";
    }

    std::cout << "\nType 'y' and press Enter to send a 5-second dim test "
                 "circle to confirm the laser reacts, or anything else to skip: ";
    std::string answer;
    std::getline(std::cin, answer);

    if (answer == "y" || answer == "Y") {
        auto controller = liberaSystem.connectController(info);
        if (!controller) {
            std::cout << "Discovered but could not connect.\n";
            liberaSystem.shutdown();
            return 1;
        }

        controller->setArmed(true);
        const auto testEnd = std::chrono::steady_clock::now() + 5s;
        while (std::chrono::steady_clock::now() < testEnd) {
            if (controller->isReadyForNewFrame()) {
                controller->sendFrame(makeDimTestCircle());
            }
            std::this_thread::sleep_for(1ms);
        }
        controller->setArmed(false);
        std::cout << "Test complete, output disarmed.\n";
    }

    liberaSystem.shutdown();
    return 0;
}
