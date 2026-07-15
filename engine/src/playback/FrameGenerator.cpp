#include "playback/FrameGenerator.hpp"

#include "show/CuePlayback.hpp"

#include <chrono>

namespace redfox::engine {

FrameGenerator::FrameGenerator(safety::Clock& clock) : clock_(clock) {}

void FrameGenerator::setShow(std::shared_ptr<const show::Show> show) {
    show_ = std::move(show);
    active_ = false;
}

void FrameGenerator::triggerCue(std::size_t cueIndex) {
    if (!show_ || cueIndex >= show_->cues.size()) {
        return; // ignore out-of-range / no show
    }
    cueIndex_ = cueIndex;
    startTime_ = clock_.now();
    active_ = true;
}

void FrameGenerator::stop() { active_ = false; }

bool FrameGenerator::currentFrame(std::vector<output::OutputPoint>& out) const {
    out.clear();
    if (!active_ || !show_ || cueIndex_ >= show_->cues.size()) {
        return false;
    }

    const show::Cue& cue = show_->cues[cueIndex_];
    if (cue.frames.empty()) {
        return false;
    }

    const double elapsedSeconds =
        std::chrono::duration<double>(clock_.now() - startTime_).count();
    const std::size_t frameIndex = show::cueFrameIndexAt(cue, elapsedSeconds);
    const ilda::IldaFrame& frame = cue.frames[frameIndex];
    if (frame.points.empty()) {
        return false;
    }

    out.reserve(frame.points.size());
    for (const ilda::IldaPoint& p : frame.points) {
        output::OutputPoint op;
        op.x = p.x;
        op.y = p.y;
        if (!p.blanked) {
            op.r = static_cast<float>(p.r) / 255.0f;
            op.g = static_cast<float>(p.g) / 255.0f;
            op.b = static_cast<float>(p.b) / 255.0f;
        }
        out.push_back(op);
    }
    return true;
}

} // namespace redfox::engine
