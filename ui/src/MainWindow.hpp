#pragma once

#include <QMainWindow>

#include <memory>
#include <vector>

#include "ipc/CommandPipeClient.hpp"
#include "ipc/SharedTelemetry.hpp"
#include "ipc/TelemetryClient.hpp"

class QLabel;
class QSlider;
class QSpinBox;
class QTimer;
class EditorWindow;
class TimelineEditor;
class PreviewWidget;

// The main application window. It stands in front of the headless engine,
// connecting to it over the existing shared-memory + named-pipe IPC: it shows
// the live safety state, sends the UI heartbeat that keeps the engine's
// watchdog satisfied, and offers Arm / Emergency-Stop / Clear controls.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onArm();
    void onEmergencyStop();
    void onClearEStop();
    void openEditor();
    void openTimelineEditor();
    void tick();

private:
    void ensureConnected();

    std::unique_ptr<redfox::ipc::TelemetryClient> telemetry_;
    std::unique_ptr<redfox::ipc::CommandPipeClient> commands_;

    QLabel* connectionLabel_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QLabel* framesLabel_ = nullptr;
    QLabel* audioLabel_ = nullptr;
    QSlider* brightnessSlider_ = nullptr;
    QSlider* scaleSlider_ = nullptr;
    QSlider* rotationSlider_ = nullptr;
    QSlider* audioAmountSlider_ = nullptr;
    QSpinBox* bpmSpin_ = nullptr; // master tempo for tempo-synced effects
    QTimer* timer_ = nullptr;
    EditorWindow* editor_ = nullptr;
    TimelineEditor* timelineEditor_ = nullptr;
    PreviewWidget* preview_ = nullptr;
    std::vector<redfox::ipc::PreviewPoint> previewBuffer_;

    static constexpr int kCueButtonCount = 8;
};
