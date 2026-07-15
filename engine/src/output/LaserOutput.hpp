#pragma once
#include <vector>

namespace redfox::output {

struct OutputPoint {
    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

// Our own seam between the Engine's safety/scheduling logic and whatever
// actually sends light to a laser. MockLaserOutput implements this for tests;
// LiberaLaserOutput implements it for real hardware.
class LaserOutput {
public:
    virtual ~LaserOutput() = default;
    virtual void setArmed(bool armed) = 0;
    virtual bool isArmed() const = 0;
    virtual void sendFrame(const std::vector<OutputPoint>& points) = 0;
    virtual void blank() = 0;
    virtual bool isHealthy() const = 0;
};

} // namespace redfox::output
