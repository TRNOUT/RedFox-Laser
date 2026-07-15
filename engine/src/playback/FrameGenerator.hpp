#pragma once

#include "output/LaserOutput.hpp"
#include "safety/Clock.hpp"
#include "show/Show.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace redfox::engine {

// Turns the active cue of a loaded show into output points for the current
// instant. It is deliberately pure with respect to hardware: it only computes
// "what should be showing now" from the show, the active cue, and the clock;
// the caller sends the points to a LaserOutput and notifies the safety
// supervisor. Time is taken from an injected Clock so playback is testable.
class FrameGenerator {
public:
    explicit FrameGenerator(safety::Clock& clock);

    void setShow(std::shared_ptr<const show::Show> show);
    void triggerCue(std::size_t cueIndex); // start playing a cue from now
    void stop();                           // clear the active cue

    bool hasActiveCue() const { return active_; }

    // Fill `out` with the active cue's current frame (ILDA points converted to
    // normalised output points; blanked points emit no colour). Returns false
    // when there is no active cue or the frame is empty.
    bool currentFrame(std::vector<output::OutputPoint>& out) const;

private:
    safety::Clock& clock_;
    std::shared_ptr<const show::Show> show_;
    bool active_ = false;
    std::size_t cueIndex_ = 0;
    std::chrono::steady_clock::time_point startTime_{};
};

} // namespace redfox::engine
