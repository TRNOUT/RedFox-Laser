#pragma once

#include <QMainWindow>

#include "ilda/IldaTypes.hpp"

class LaserCanvas;
class QLabel;
class QPushButton;

// A minimal vector editor: draw a frame on the canvas and save/load it as an
// ILDA (.ild) file. Standalone by default; when hosted by the Timeline Editor,
// enableAddAsCue() reveals an "Add as Cue" button that emits the drawn frame so
// it can be turned into a show cue without a round-trip through disk.
class EditorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);

    // Reveal the "Add as Cue" action (hidden by default).
    void enableAddAsCue();

signals:
    // Emitted when the user clicks "Add as Cue": the current canvas frame.
    void frameReady(const redfox::ilda::IldaFrame& frame);

private slots:
    void pickColor();
    void toggleBlank(bool blanked);
    void saveIlda();
    void loadIlda();

private:
    LaserCanvas* canvas_ = nullptr;
    QLabel* pointCountLabel_ = nullptr;
    QPushButton* addAsCueButton_ = nullptr;
};
