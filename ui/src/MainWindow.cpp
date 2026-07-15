#include "MainWindow.hpp"

#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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

    layout->addWidget(connectionLabel_);
    layout->addWidget(stateLabel_);
    layout->addWidget(framesLabel_);

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
        telemetry_->telemetry().uiHeartbeatEpochMs.store(nowEpochMs());
        const std::uint32_t state = telemetry_->telemetry().safetyState.load();
        stateLabel_->setText(QString("Safety state: ") + stateName(state));
        framesLabel_->setText(
            QString("Frames sent: %1")
                .arg(static_cast<qulonglong>(telemetry_->telemetry().framesSent.load())));
    } else {
        stateLabel_->setText("Safety state: —");
        framesLabel_->setText("Frames sent: —");
    }
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
