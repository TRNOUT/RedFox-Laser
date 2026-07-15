#pragma once
#include <chrono>

namespace redfox::safety {

class Clock {
public:
    virtual ~Clock() = default;
    virtual std::chrono::steady_clock::time_point now() const = 0;
};

class SystemClock : public Clock {
public:
    std::chrono::steady_clock::time_point now() const override {
        return std::chrono::steady_clock::now();
    }
};

// Test double: time only moves when advance() is called explicitly.
class FakeClock : public Clock {
public:
    std::chrono::steady_clock::time_point now() const override {
        return current_;
    }

    void advance(std::chrono::milliseconds delta) {
        current_ += delta;
    }

private:
    std::chrono::steady_clock::time_point current_{};
};

} // namespace redfox::safety
