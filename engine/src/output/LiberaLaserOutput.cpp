#include "LiberaLaserOutput.hpp"

namespace redfox::output {

LiberaLaserOutput::LiberaLaserOutput(std::shared_ptr<libera::core::LaserController> controller)
    : controller_(std::move(controller)) {}

void LiberaLaserOutput::setArmed(bool armed) {
    if (controller_) {
        controller_->setArmed(armed);
    }
}

bool LiberaLaserOutput::isArmed() const {
    return controller_ && controller_->isArmed();
}

void LiberaLaserOutput::sendFrame(const std::vector<OutputPoint>& points) {
    if (!controller_ || !controller_->isArmed()) {
        return;
    }
    if (!controller_->isReadyForNewFrame()) {
        return;
    }

    libera::core::Frame frame;
    frame.points.reserve(points.size());
    for (const auto& p : points) {
        frame.points.push_back(libera::core::LaserPoint{p.x, p.y, p.r, p.g, p.b});
    }
    controller_->sendFrame(std::move(frame));
}

void LiberaLaserOutput::blank() {
    if (controller_) {
        controller_->setArmed(false);
    }
}

bool LiberaLaserOutput::isHealthy() const {
    return controller_ && controller_->getStatus() == libera::core::ControllerStatus::Good;
}

} // namespace redfox::output
