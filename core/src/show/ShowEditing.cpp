#include "show/ShowEditing.hpp"

namespace redfox::show {

void removeCue(Show& show, std::size_t cueIndex) {
    if (cueIndex >= show.cues.size()) {
        return;
    }
    show.cues.erase(show.cues.begin() + static_cast<std::ptrdiff_t>(cueIndex));

    std::vector<TimelineStep> kept;
    kept.reserve(show.timeline.steps.size());
    for (const TimelineStep& step : show.timeline.steps) {
        if (step.cueIndex == cueIndex) {
            continue; // the cue this step triggered is gone
        }
        TimelineStep fixed = step;
        if (fixed.cueIndex > cueIndex) {
            --fixed.cueIndex; // everything above the hole shifts down one
        }
        kept.push_back(fixed);
    }
    show.timeline.steps = std::move(kept);
}

} // namespace redfox::show
