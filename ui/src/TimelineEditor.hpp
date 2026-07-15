#pragma once

#include <QMainWindow>

#include "show/Show.hpp"

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QTableWidget;

// Authors a show's timeline: the sequence of {time, cue} steps the engine's
// Sequencer plays back. It loads show.rfsh (falling back to the built-in demo
// show so there are cues to reference), lets you edit the step list, duration,
// loop and name, and saves the whole show back to show.rfsh — which the engine
// picks up at startup.
class TimelineEditor : public QMainWindow {
    Q_OBJECT

public:
    explicit TimelineEditor(QWidget* parent = nullptr);

private slots:
    void addStep();
    void removeSelectedStep();
    void save();
    void reload();

private:
    void loadFromShow(const redfox::show::Show& show);
    void addRow(double timeSeconds, int cueIndex);
    void rebuildCueLegend();

    redfox::show::Show show_;

    QLabel* cueLegend_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QDoubleSpinBox* durationSpin_ = nullptr;
    QCheckBox* loopCheck_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};
