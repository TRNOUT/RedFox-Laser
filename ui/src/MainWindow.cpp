#include "MainWindow.hpp"

#include "EditorWindow.hpp"
#include "TimelineEditor.hpp"
#include "PreviewWidget.hpp"

#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <chrono>
#include <cstdint>

using namespace redfox::ipc;

namespace {

std::uint64_t nowEpochMs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

const char* stateName(std::uint32_t wire) {
    switch (wire) {
        case static_cast<std::uint32_t>(SafetyStateWire::Blanked): return "Blanked";
        case static_cast<std::uint32_t>(SafetyStateWire::Armed): return "Armed";
        case static_cast<std::uint32_t>(SafetyStateWire::EStopLatched): return "Emergency Stop";
        default: return "unknown";
    }
}

// Build a labelled horizontal slider and append it as a row to `form`. The
// slider is an integer 0..max; callers convert to the float range they need.
QSlider* addSlider(QGridLayout* form, int row, const QString& label,
                   QWidget* parent, int max, int initial) {
    auto* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(0, max);
    slider->setValue(initial);
    form->addWidget(new QLabel(label, parent), row, 0);
    form->addWidget(slider, row, 1);
    return slider;
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("RedFox-Laser");

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    connectionLabel_ = new QLabel("Connecting to engine...", central);
    stateLabel_ = new QLabel("Safety state: —", central);
    QFont stateFont = stateLabel_->font();
    stateFont.setPointSize(18);
    stateFont.setBold(true);
    stateLabel_->setFont(stateFont);
    framesLabel_ = new QLabel("Frames sent: 0", central);
    audioLabel_ = new QLabel("Audio: —", central);

    layout->addWidget(connectionLabel_);
    layout->addWidget(stateLabel_);
    layout->addWidget(framesLabel_);
    layout->addWidget(audioLabel_);

    preview_ = new PreviewWidget(central);
    layout->addWidget(preview_, 1);
    previewBuffer_.resize(redfox::ipc::kMaxPreviewPoints);

    auto* buttons = new QHBoxLayout();
    auto* armButton = new QPushButton("Arm", central);
    auto* eStopButton = new QPushButton("EMERGENCY STOP", central);
    auto* clearButton = new QPushButton("Clear E-Stop", central);
    eStopButton->setStyleSheet("background-color:#c0392b; color:white; font-weight:bold;");
    buttons->addWidget(armButton);
    buttons->addWidget(eStopButton);
    buttons->addWidget(clearButton);
    layout->addLayout(buttons);

    // Cue grid: each button triggers the cue at that index in the loaded show.
    layout->addWidget(new QLabel("Cues", central));
    auto* cueGrid = new QGridLayout();
    for (int i = 0; i < kCueButtonCount; ++i) {
        auto* cueButton = new QPushButton(QString("Cue %1").arg(i), central);
        connect(cueButton, &QPushButton::clicked, this, [this, i]() {
            if (commands_ && commands_->isConnected()) {
                commands_->send(redfox::ipc::CommandType::TriggerCue,
                                static_cast<std::uint32_t>(i));
            }
        });
        cueGrid->addWidget(cueButton, i / 4, i % 4);
    }
    layout->addLayout(cueGrid);

    auto* stopCueButton = new QPushButton("Stop Cue", central);
    connect(stopCueButton, &QPushButton::clicked, this, [this]() {
        if (commands_ && commands_->isConnected()) {
            commands_->send(redfox::ipc::CommandType::StopCue);
        }
    });
    layout->addWidget(stopCueButton);

    // Timeline transport: play/stop the loaded show sequence.
    auto* sequenceButtons = new QHBoxLayout();
    auto* playSequenceButton = new QPushButton("Play Sequence", central);
    connect(playSequenceButton, &QPushButton::clicked, this, [this]() {
        if (commands_ && commands_->isConnected()) {
            commands_->send(redfox::ipc::CommandType::PlaySequence);
        }
    });
    auto* stopSequenceButton = new QPushButton("Stop Sequence", central);
    connect(stopSequenceButton, &QPushButton::clicked, this, [this]() {
        if (commands_ && commands_->isConnected()) {
            commands_->send(redfox::ipc::CommandType::StopSequence);
        }
    });
    sequenceButtons->addWidget(playSequenceButton);
    sequenceButtons->addWidget(stopSequenceButton);
    layout->addLayout(sequenceButtons);

    // Live master controls: continuous values pushed to the engine over shared
    // memory each tick (see tick()). Ranges are chosen so the initial value is
    // the engine's identity default.
    layout->addWidget(new QLabel("Master", central));
    auto* controls = new QGridLayout();
    brightnessSlider_ = addSlider(controls, 0, "Brightness", central, 100, 100);
    scaleSlider_ = addSlider(controls, 1, "Scale", central, 200, 100);
    rotationSlider_ = addSlider(controls, 2, "Rotation", central, 100, 0);
    audioAmountSlider_ = addSlider(controls, 3, "Audio react", central, 100, 30);
    controls->addWidget(new QLabel("Tempo (BPM)", central), 4, 0);
    bpmSpin_ = new QSpinBox(central);
    bpmSpin_->setRange(20, 300);
    bpmSpin_->setValue(120);
    bpmSpin_->setToolTip("Master tempo — effects set to “Sync to master BPM” run at this rate.");
    controls->addWidget(bpmSpin_, 4, 1);
    layout->addLayout(controls);

    auto* editorButton = new QPushButton("Open Vector Editor", central);
    connect(editorButton, &QPushButton::clicked, this, &MainWindow::openEditor);
    layout->addWidget(editorButton);

    auto* timelineButton = new QPushButton("Open Timeline Editor", central);
    connect(timelineButton, &QPushButton::clicked, this, &MainWindow::openTimelineEditor);
    layout->addWidget(timelineButton);

    setCentralWidget(central);

    connect(armButton, &QPushButton::clicked, this, &MainWindow::onArm);
    connect(eStopButton, &QPushButton::clicked, this, &MainWindow::onEmergencyStop);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearEStop);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MainWindow::tick);
    timer_->start(50); // 50 ms: matches the UI heartbeat cadence the engine expects

    ensureConnected();
}

void MainWindow::ensureConnected() {
    if (!telemetry_ || !telemetry_->isValid()) {
        telemetry_ = std::make_unique<TelemetryClient>();
    }
    if (!commands_ || !commands_->isConnected()) {
        commands_ = std::make_unique<CommandPipeClient>();
    }
}

void MainWindow::tick() {
    ensureConnected();

    const bool connected = telemetry_ && telemetry_->isValid() &&
                           commands_ && commands_->isConnected();
    connectionLabel_->setText(connected
                                  ? "Engine: connected"
                                  : "Engine: not running (start redfox_engine.exe)");

    if (telemetry_ && telemetry_->isValid()) {
        // Keep the engine's heartbeat watchdog satisfied.
        auto& tel = telemetry_->telemetry();
        tel.uiHeartbeatEpochMs.store(nowEpochMs());

        // Push the live master controls to the engine.
        tel.ctrlMasterBrightness.store(brightnessSlider_->value() / 100.0f,
                                       std::memory_order_relaxed);
        tel.ctrlMasterScale.store(scaleSlider_->value() / 100.0f,
                                  std::memory_order_relaxed);
        tel.ctrlMasterRotationTurns.store(rotationSlider_->value() / 100.0f,
                                          std::memory_order_relaxed);
        tel.ctrlAudioAmount.store(audioAmountSlider_->value() / 100.0f,
                                  std::memory_order_relaxed);
        tel.ctrlMasterBpm.store(static_cast<float>(bpmSpin_->value()),
                                std::memory_order_relaxed);
        const std::uint32_t state = telemetry_->telemetry().safetyState.load();
        stateLabel_->setText(QString("Safety state: ") + stateName(state));
        framesLabel_->setText(
            QString("Frames sent: %1")
                .arg(static_cast<qulonglong>(telemetry_->telemetry().framesSent.load())));

        const std::size_t count =
            redfox::ipc::readPreview(telemetry_->telemetry(), previewBuffer_.data());
        preview_->setPoints(previewBuffer_.data(), count);

        const auto& t = telemetry_->telemetry();
        audioLabel_->setText(
            QString("Audio  level %1  bass %2  mid %3  high %4   beats %5")
                .arg(t.audioLevel.load(), 0, 'f', 2)
                .arg(t.audioBass.load(), 0, 'f', 2)
                .arg(t.audioMid.load(), 0, 'f', 2)
                .arg(t.audioHigh.load(), 0, 'f', 2)
                .arg(static_cast<qulonglong>(t.beatCount.load())));
    } else {
        stateLabel_->setText("Safety state: —");
        framesLabel_->setText("Frames sent: —");
        audioLabel_->setText("Audio: —");
        preview_->setPoints(nullptr, 0);
    }
}

void MainWindow::openEditor() {
    if (!editor_) {
        editor_ = new EditorWindow(this);
    }
    editor_->show();
    editor_->raise();
    editor_->activateWindow();
}

void MainWindow::openTimelineEditor() {
    if (!timelineEditor_) {
        timelineEditor_ = new TimelineEditor(this);
    }
    timelineEditor_->show();
    timelineEditor_->raise();
    timelineEditor_->activateWindow();
}

void MainWindow::onArm() {
    if (commands_ && commands_->isConnected()) {
        commands_->send(CommandType::Arm);
    }
}

void MainWindow::onEmergencyStop() {
    if (commands_ && commands_->isConnected()) {
        commands_->send(CommandType::EmergencyStop);
    }
}

void MainWindow::onClearEStop() {
    if (commands_ && commands_->isConnected()) {
        commands_->send(CommandType::ClearEmergencyStop);
    }
}
