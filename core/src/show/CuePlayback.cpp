#include "show/CuePlayback.hpp"

#include <cmath>

namespace redfox::show {

std::size_t cueFrameIndexAt(const Cue& cue, double elapsedSeconds) {
    const std::size_t frameCount = cue.frames.size();
    if (frameCount <= 1) {
        return 0;
    }
    if (elapsedSeconds <= 0.0 || cue.framesPerSecond <= 0.0f) {
        return 0;
    }

    const double rawIndex =
        std::floor(elapsedSeconds * static_cast<double>(cue.framesPerSecond));
    if (rawIndex < 0.0) {
        return 0;
    }

    const std::size_t index = static_cast<std::size_t>(rawIndex);
    if (cue.loop) {
        return index % frameCount;
    }
    return index < frameCount ? index : frameCount - 1;
}

} // namespace redfox::show
