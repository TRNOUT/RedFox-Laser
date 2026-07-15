#include "SafetySupervisor.hpp"

namespace redfox::safety {

SafetySupervisor::SafetySupervisor(Clock& clock, SafetySupervisorConfig config)
    : clock_(clock), config_(config) {}

void SafetySupervisor::registerOutput(std::shared_ptr<output::LaserOutput> output) {
    std::lock_guard<std::mutex> lock(mutex_);
    outputs_.push_back(std::move(output));
}

void SafetySupervisor::setEventCallback(EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    eventCallback_ = std::move(callback);
}

void SafetySupervisor::notifyUiHeartbeat() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastUiHeartbeat_ = clock_.now();
    haveUiHeartbeat_ = true;
}

void SafetySupervisor::notifyFrameProduced() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastFrame_ = clock_.now();
    haveFrame_ = true;
}

bool SafetySupervisor::requestArm() {
    // Callbacks are invoked outside the lock so a callback that calls back
    // into this SafetySupervisor (e.g. state()) cannot deadlock.
    EventCallback callbackCopy;
    SafetyState newState;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == SafetyState::EStopLatched) {
            return false;
        }
        state_ = SafetyState::Armed;
        const auto now = clock_.now();
        lastUiHeartbeat_ = now;
        haveUiHeartbeat_ = true;
        // Deliberately do NOT arm the frame-stall watchdog here. It must stay
        // dormant until a real frame is actually produced
        // (notifyFrameProduced()); otherwise arming while no frame generator
        // is running would trip an immediate false frame-stall. The
        // frame-stall watchdog activates only once frames start flowing.
        for (auto& out : outputs_) {
            out->setArmed(true);
        }
        newState = state_;
        callbackCopy = eventCallback_;
    }
    if (callbackCopy) {
        callbackCopy(SafetyEventCode::ManualArm, newState);
    }
    return true;
}

void SafetySupervisor::triggerEmergencyStop() {
    EventCallback callbackCopy;
    SafetyState newState;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = SafetyState::EStopLatched;
        for (auto& out : outputs_) {
            out->setArmed(false);
            out->blank();
        }
        newState = state_;
        callbackCopy = eventCallback_;
    }
    if (callbackCopy) {
        callbackCopy(SafetyEventCode::EmergencyStop, newState);
    }
}

bool SafetySupervisor::clearEmergencyStop() {
    EventCallback callbackCopy;
    SafetyState newState;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != SafetyState::EStopLatched) {
            return false;
        }
        state_ = SafetyState::Blanked;
        newState = state_;
        callbackCopy = eventCallback_;
    }
    if (callbackCopy) {
        callbackCopy(SafetyEventCode::ManualReArm, newState);
    }
    return true;
}

void SafetySupervisor::tick() {
    EventCallback callbackCopy;
    SafetyState newState;
    SafetyEventCode reason = SafetyEventCode::None;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != SafetyState::Armed) {
            return;
        }

        const auto now = clock_.now();

        if (haveUiHeartbeat_ && (now - lastUiHeartbeat_) > config_.uiHeartbeatTimeout) {
            reason = SafetyEventCode::UiHeartbeatLost;
        } else if (haveFrame_ && (now - lastFrame_) > config_.frameStallTimeout) {
            reason = SafetyEventCode::FrameStall;
        }

        if (reason == SafetyEventCode::None) {
            return;
        }

        state_ = SafetyState::Blanked;
        for (auto& out : outputs_) {
            out->setArmed(false);
            out->blank();
        }
        newState = state_;
        callbackCopy = eventCallback_;
    }
    if (callbackCopy) {
        callbackCopy(reason, newState);
    }
}

SafetyState SafetySupervisor::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

} // namespace redfox::safety
