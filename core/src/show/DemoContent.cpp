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

    // A constant red line with a bright green point running back and forth along
    // it — the "runner" motion effect, evaluated live (no baked frames).
    Cue runnerLine;
    runnerLine.name = "Runner Line";
    ilda::IldaFrame line;
    constexpr int kLinePoints = 40;
    for (int i = 0; i < kLinePoints; ++i) {
        const float t = static_cast<float>(i) / (kLinePoints - 1);
        line.points.push_back({-0.8f + 1.6f * t, 0.0f, 0.0f, 255, 0, 0, false});
    }
    runnerLine.frames = {line};
    {
        effects::Effect runner;
        runner.type = effects::EffectType::Runner;
        runner.direction = effects::Direction::PingPong; // hourglass-style bounce
        runner.rateHz = 0.5f; // one sweep every two seconds
        runner.amount = 0.12f; // width of the running highlight
        runner.r = 0;
        runner.g = 255;
        runner.b = 0;
        runnerLine.effects = {runner};
    }
    show.cues.push_back(runnerLine);

    // A triangle that spins around its own centre via the rotate effect, with the
    // speed set as if driven from BPM (120 BPM = 2 beats/s -> 0.5 turns/s here).
    Cue triangle;
    triangle.name = "Spinning Triangle";
    ilda::IldaFrame tri;
    tri.points = {
        {0.0f, 0.6f, 0.0f, 0, 128, 255, false},
        {0.52f, -0.3f, 0.0f, 0, 128, 255, false},
        {-0.52f, -0.3f, 0.0f, 0, 128, 255, false},
        {0.0f, 0.6f, 0.0f, 0, 128, 255, false},
    };
    triangle.frames = {tri};
    {
        effects::Effect rot;
        rot.type = effects::EffectType::Rotate;
        rot.direction = effects::Direction::Forward; // counter-clockwise
        rot.rateHz = 0.5f;
        // Stacked on top: a rainbow that cycles as it spins (effects compose).
        effects::Effect rainbow;
        rainbow.type = effects::EffectType::ColorCycle;
        rainbow.rateHz = 0.2f;
        rainbow.amount = 1.0f; // a full rainbow spread across the triangle
        triangle.effects = {rot, rainbow};
    }
    show.cues.push_back(triangle);

    show.timeline = makeDemoTimeline();
    return show;
}

Timeline makeDemoTimeline() {
    Timeline timeline;
    timeline.name = "Demo Sequence";
    timeline.steps = {
        {0.0, 0},  // Square
        {3.0, 1},  // Blink
        {6.0, 2},  // Runner Line
        {12.0, 3}, // Spinning Triangle
        {18.0, 0}, // Square again
    };
    timeline.durationSeconds = 24.0;
    timeline.loop = true;
    return timeline;
}

} // namespace redfox::show
