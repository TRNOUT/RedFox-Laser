#include "MainWindow.hpp"

#include <QFont>
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

    layout->addWidget(connectionLabel_);
    layout->addWidget(stateLabel_);

    auto* buttons = new QHBoxLayout();
    auto* armButton = new QPushButton("Arm", central);
    auto* eStopButton = new QPushButton("EMERGENCY STOP", central);
    auto* clearButton = new QPushButton("Clear E-Stop", central);
    eStopButton->setStyleSheet("background-color:#c0392b; color:white; font-weight:bold;");
    buttons->addWidget(armButton);
    buttons->addWidget(eStopButton);
    buttons->addWidget(clearButton);
    layout->addLayout(buttons);

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
    } else {
        stateLabel_->setText("Safety state: —");
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
