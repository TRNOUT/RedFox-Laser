#pragma once

#include "show/Show.hpp"

#include <cstddef>

namespace redfox::show {

// Which frame of a cue is showing after `elapsedSeconds` of playback?
// - empty cue -> 0
// - single frame -> always 0
// - looping animation -> wraps modulo the frame count
// - non-looping animation -> advances then holds the last frame
std::size_t cueFrameIndexAt(const Cue& cue, double elapsedSeconds);

} // namespace redfox::show
