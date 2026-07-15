#pragma once

#include "ilda/IldaTypes.hpp"

namespace redfox::show {

// A 2D affine transform plus a brightness scale, applied to frame points before
// output. Rotation is in full turns (1.0 = 360 degrees). Order: rotate, scale,
// then translate; brightness multiplies the colour channels.
struct Transform {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float scale = 1.0f;
    float rotationTurns = 0.0f;
    float brightness = 1.0f;
};

ilda::IldaPoint applyTransform(const ilda::IldaPoint& point, const Transform& transform);

} // namespace redfox::show
