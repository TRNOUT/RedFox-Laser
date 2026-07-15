#pragma once

#include "safety/Clock.hpp"
#include "show/Timeline.hpp"

#include <cstddef>
#include <functional>
#include <mutex>

namespace redfox::engine {

// Drives a show timeline: as the injected clock advances, it fires each
// scheduled step's cue trigger exactly once via a callback, handling looping
// and end-of-sequence. Time comes from a Clock so it is fully testable with a
// FakeClock, mirroring FrameGenerator. Thread-safe: start/stop/setTimeline may
// be called from a control thread while the engine loop calls tick().
class Sequencer {
public:
    explicit Sequencer(safety::Clock& clock);

    void setTimeline(show::Timeline timeline);
    // Called with a cue index when a step comes due. Typically forwards to
    // FrameGenerator::triggerCue.
    void setTriggerCallback(std::function<void(std::size_t)> callback);

    void start(); // (re)start playback from the beginning of the timeline
    void stop();
    bool isPlaying() const;

    // Advance to the current clock time and fire any steps now due. Call this
    // periodically from the engine loop.
    void tick();

private:
    // Elapsed seconds since the current cycle's start reference.
    double elapsedSeconds() const;

    safety::Clock& clock_;
    mutable std::mutex mutex_;
    show::Timeline timeline_;
    std::function<void(std::size_t)> callback_;
    bool playing_ = false;
    std::chrono::steady_clock::time_point cycleStart_{};
    double lastElapsed_ = 0.0;
};

} // namespace redfox::engine
