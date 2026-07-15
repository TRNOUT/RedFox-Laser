#include "show/Transform.hpp"

#include <algorithm>
#include <cmath>

namespace redfox::show {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;

std::uint8_t scaleColour(std::uint8_t c, float brightness) {
    const int v = static_cast<int>(std::lround(static_cast<float>(c) * brightness));
    return static_cast<std::uint8_t>(std::clamp(v, 0, 255));
}
} // namespace

ilda::IldaPoint applyTransform(const ilda::IldaPoint& point, const Transform& transform) {
    ilda::IldaPoint out = point;

    const double angle = static_cast<double>(transform.rotationTurns) * kTwoPi;
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);

    const double rx = point.x * cosA - point.y * sinA;
    const double ry = point.x * sinA + point.y * cosA;

    out.x = static_cast<float>(rx * transform.scale) + transform.offsetX;
    out.y = static_cast<float>(ry * transform.scale) + transform.offsetY;
    out.z = point.z; // 2D transform leaves z untouched

    if (!point.blanked) {
        out.r = scaleColour(point.r, transform.brightness);
        out.g = scaleColour(point.g, transform.brightness);
        out.b = scaleColour(point.b, transform.brightness);
    }
    return out;
}

} // namespace redfox::show
