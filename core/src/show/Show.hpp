#pragma once

#include "ilda/IldaTypes.hpp"

#include <string>
#include <vector>

namespace redfox::show {

// A triggerable unit: one still frame or a multi-frame animation, plus how it
// should play back. Frames use the device-independent ILDA frame model.
struct Cue {
    std::string name;
    ilda::IldaShow frames;          // 1 = still, >1 = animation
    float framesPerSecond = 30.0f;  // animation playback rate
    bool loop = true;               // loop vs. hold the last frame
};

// A show is a collection of cues. Grid/timeline placement is layered on top of
// this by the UI; the data model itself is just the ordered cue list.
struct Show {
    std::string name;
    std::vector<Cue> cues;
};

} // namespace redfox::show
