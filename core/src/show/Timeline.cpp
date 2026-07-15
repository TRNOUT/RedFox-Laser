#include "show/Timeline.hpp"

namespace redfox::show {

std::vector<std::size_t> stepsInInterval(const Timeline& timeline, double fromSeconds,
                                         double toSeconds) {
    std::vector<std::size_t> fired;
    if (!(fromSeconds < toSeconds)) {
        return fired; // reversed or empty interval
    }
    for (std::size_t i = 0; i < timeline.steps.size(); ++i) {
        const double t = timeline.steps[i].timeSeconds;
        if (t > fromSeconds && t <= toSeconds) {
            fired.push_back(i);
        }
    }
    return fired;
}

bool timelineIsFinished(const Timeline& timeline, double elapsedSeconds) {
    if (timeline.loop) {
        return false;
    }
    return elapsedSeconds >= timeline.durationSeconds;
}

} // namespace redfox::show
