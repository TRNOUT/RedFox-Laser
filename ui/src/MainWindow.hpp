#pragma once

#include <QMainWindow>

#include <memory>
#include <vector>

#include "ipc/CommandPipeClient.hpp"
#include "ipc/SharedTelemetry.hpp"
#include "ipc/TelemetryClient.hpp"

class QLabel;
class QTimer;
class EditorWindow;
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
    void tick();

private:
    void ensureConnected();

    std::unique_ptr<redfox::ipc::TelemetryClient> telemetry_;
    std::unique_ptr<redfox::ipc::CommandPipeClient> commands_;

    QLabel* connectionLabel_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QLabel* framesLabel_ = nullptr;
    QLabel* audioLabel_ = nullptr;
    QTimer* timer_ = nullptr;
    EditorWindow* editor_ = nullptr;
    PreviewWidget* preview_ = nullptr;
    std::vector<redfox::ipc::PreviewPoint> previewBuffer_;

    static constexpr int kCueButtonCount = 8;
};
