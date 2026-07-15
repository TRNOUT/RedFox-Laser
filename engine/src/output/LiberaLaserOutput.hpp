#pragma once

#include "LaserOutput.hpp"
#include "libera/core/LaserController.hpp"

#include <memory>

namespace redfox::output {

// Wraps an already-connected libera-laser controller (obtained via
// libera::System::connectController(), see tools/idn_probe/main.cpp for a
// working discovery+connect example) behind our own LaserOutput interface.
//
// No automated test coverage: libera::core::LaserController can only be
// obtained through a real hardware discovery/connect step. Verify manually
// against real hardware with the usual laser-safety precautions.
class LiberaLaserOutput : public LaserOutput {
public:
    explicit LiberaLaserOutput(std::shared_ptr<libera::core::LaserController> controller);

    void setArmed(bool armed) override;
    bool isArmed() const override;
    void sendFrame(const std::vector<OutputPoint>& points) override;
    void blank() override;
    bool isHealthy() const override;

private:
    std::shared_ptr<libera::core::LaserController> controller_;
};

} // namespace redfox::output
