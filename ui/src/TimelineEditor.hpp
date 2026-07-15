#pragma once

#include <QMainWindow>

#include "show/Show.hpp"

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QTableWidget;
class TimelineRuler;

// Authors a show's timeline: the sequence of {time, cue} steps the engine's
// Sequencer plays back. It loads the shared show file (falling back to the
// built-in demo show so there are cues to reference), lets you edit the steps
// both graphically — dragging markers on a time ruler — and as a table, and
// saves the whole show back. A running engine is told to reload it live.
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
    void refreshRuler();

    redfox::show::Show show_;
    std::string showPath_;

    QLabel* cueLegend_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QDoubleSpinBox* durationSpin_ = nullptr;
    QCheckBox* loopCheck_ = nullptr;
    TimelineRuler* ruler_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};
