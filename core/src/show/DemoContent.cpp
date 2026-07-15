#include "show/DemoContent.hpp"

namespace redfox::show {

Show makeDemoShow() {
    Show show;
    show.name = "Demo";

    Cue square;
    square.name = "Square";
    ilda::IldaFrame squareFrame;
    constexpr std::uint8_t kWhite = 255;
    squareFrame.points = {
        {-0.5f, -0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {0.5f, -0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {0.5f, 0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {-0.5f, 0.5f, 0.0f, kWhite, kWhite, kWhite, false},
        {-0.5f, -0.5f, 0.0f, kWhite, kWhite, kWhite, false},
    };
    square.frames = {squareFrame};
    square.spinTurnsPerSec = 0.2f; // slowly rotating, to show effects in the preview
    show.cues.push_back(square);

    Cue blink;
    blink.name = "Blink";
    blink.framesPerSecond = 4.0f;
    ilda::IldaFrame dotOn;
    dotOn.points = {{0.0f, 0.0f, 0.0f, 255, 0, 0, false}};
    ilda::IldaFrame dotOff;
    dotOff.points = {{0.0f, 0.0f, 0.0f, 0, 0, 0, true}};
    blink.frames = {dotOn, dotOff};
    show.cues.push_back(blink);

    show.timeline = makeDemoTimeline();
    return show;
}

Timeline makeDemoTimeline() {
    Timeline timeline;
    timeline.name = "Demo Sequence";
    timeline.steps = {
        {0.0, 0}, // Square
        {3.0, 1}, // Blink
        {6.0, 0}, // Square again
    };
    timeline.durationSeconds = 9.0;
    timeline.loop = true;
    return timeline;
}

} // namespace redfox::show
