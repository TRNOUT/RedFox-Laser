#pragma once

#include <QMainWindow>

class LaserCanvas;
class QLabel;

// A minimal vector editor: draw a frame on the canvas and save/load it as an
// ILDA (.ild) file. Standalone — it does not need the engine.
class EditorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);

private slots:
    void pickColor();
    void toggleBlank(bool blanked);
    void saveIlda();
    void loadIlda();

private:
    LaserCanvas* canvas_ = nullptr;
    QLabel* pointCountLabel_ = nullptr;
};
