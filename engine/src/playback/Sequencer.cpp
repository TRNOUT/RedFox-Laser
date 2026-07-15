#include "playback/Sequencer.hpp"

#include <vector>

namespace redfox::engine {

namespace {
// A step scheduled exactly at a cycle boundary (e.g. t=0) must still fire once
// the cycle begins. stepsInInterval is half-open on the low end, so each cycle
// starts its "already seen" marker just below zero.
constexpr double kCycleStartEpsilon = 1e-6;
} // namespace

Sequencer::Sequencer(safety::Clock& clock) : clock_(clock) {}

void Sequencer::setTimeline(show::Timeline timeline) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeline_ = std::move(timeline);
}

void Sequencer::setTriggerCallback(std::function<void(std::size_t)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void Sequencer::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    playing_ = true;
    cycleStart_ = clock_.now();
    lastElapsed_ = -kCycleStartEpsilon;
}

void Sequencer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    playing_ = false;
}

bool Sequencer::isPlaying() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return playing_;
}

double Sequencer::elapsedSeconds() const {
    return std::chrono::duration<double>(clock_.now() - cycleStart_).count();
}

void Sequencer::tick() {
    std::vector<std::size_t> toTrigger;
    std::function<void(std::size_t)> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!playing_) {
            return;
        }
        callback = callback_;

        double now = elapsedSeconds();
        const auto collect = [&](const std::vector<std::size_t>& steps) {
            for (std::size_t stepIndex : steps) {
                toTrigger.push_back(timeline_.steps[stepIndex].cueIndex);
            }
        };

        if (timeline_.loop && timeline_.durationSeconds > 0.0) {
            const double duration = timeline_.durationSeconds;
            while (now >= duration) {
                // Finish the current cycle, then roll the reference forward one
                // full duration so the next cycle's steps fire again.
                collect(show::stepsInInterval(timeline_, lastElapsed_, duration));
                cycleStart_ += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(duration));
                now -= duration;
                lastElapsed_ = -kCycleStartEpsilon;
            }
            collect(show::stepsInInterval(timeline_, lastElapsed_, now));
            lastElapsed_ = now;
        } else {
            collect(show::stepsInInterval(timeline_, lastElapsed_, now));
            lastElapsed_ = now;
            if (show::timelineIsFinished(timeline_, now)) {
                playing_ = false;
            }
        }
    }

    // Invoke the trigger callback outside the lock (it may re-enter engine
    // objects) — same discipline as the safety supervisor's event callback.
    if (callback) {
        for (std::size_t cueIndex : toTrigger) {
            callback(cueIndex);
        }
    }
}

} // namespace redfox::engine
