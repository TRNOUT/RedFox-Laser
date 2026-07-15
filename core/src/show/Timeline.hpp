#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace redfox::show {

// One scheduled event on a timeline: trigger cue `cueIndex` at `timeSeconds`
// measured from the sequence start.
struct TimelineStep {
    double timeSeconds = 0.0;
    std::size_t cueIndex = 0;
};

// A show sequence: an ordered list of cue triggers over time. `steps` is
// assumed sorted ascending by `timeSeconds`. The sequence spans
// `durationSeconds`; when `loop` is set it restarts from the top after that.
struct Timeline {
    std::string name;
    std::vector<TimelineStep> steps;
    double durationSeconds = 0.0;
    bool loop = false;
};

// Indices of the steps whose trigger time lies in the half-open interval
// (fromSeconds, toSeconds]. The interval is half-open so that as a sequencer
// advances time across successive calls each step fires exactly once. Returns
// them in timeline order. A reversed or empty interval yields nothing.
std::vector<std::size_t> stepsInInterval(const Timeline& timeline, double fromSeconds,
                                         double toSeconds);

// Whether a non-looping timeline has run past its end at `elapsedSeconds`.
// Looping timelines are never finished.
bool timelineIsFinished(const Timeline& timeline, double elapsedSeconds);

} // namespace redfox::show
