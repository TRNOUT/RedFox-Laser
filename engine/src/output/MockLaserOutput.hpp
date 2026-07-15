#pragma once
#include "LaserOutput.hpp"
#include <mutex>

namespace redfox::output {

class MockLaserOutput : public LaserOutput {
public:
    void setArmed(bool armed) override {
        std::lock_guard<std::mutex> lock(mutex_);
        armed_ = armed;
        if (!armed_) {
            lastFrame_.clear();
        }
    }

    bool isArmed() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return armed_;
    }

    void sendFrame(const std::vector<OutputPoint>& points) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!armed_) {
            return;
        }
        lastFrame_ = points;
        frameCount_++;
    }

    void blank() override {
        std::lock_guard<std::mutex> lock(mutex_);
        lastFrame_.clear();
        blankCount_++;
    }

    bool isHealthy() const override {
        return true;
    }

    // Test-only inspection helpers.
    std::vector<OutputPoint> lastFrame() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastFrame_;
    }

    std::size_t frameCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return frameCount_;
    }

    std::size_t blankCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return blankCount_;
    }

private:
    mutable std::mutex mutex_;
    bool armed_ = false;
    std::vector<OutputPoint> lastFrame_;
    std::size_t frameCount_ = 0;
    std::size_t blankCount_ = 0;
};

} // namespace redfox::output
