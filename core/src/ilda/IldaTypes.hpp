#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace redfox::ilda {

// A single laser sample in a device-independent form. Coordinates are
// normalised to -1..+1 (the full ILDA int16 range maps onto this); colour is
// resolved 8-bit RGB regardless of whether the source used a palette.
struct IldaPoint {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    bool blanked = false; // true = laser off (the scanner still moves here)
};

// One frame (one drawing). ILDA limits name/company to 8 ASCII chars each.
struct IldaFrame {
    std::string name;
    std::string company;
    bool is3D = false; // whether z is meaningful (written as a 3D ILDA format)
    std::vector<IldaPoint> points;
};

using IldaShow = std::vector<IldaFrame>;

} // namespace redfox::ilda
