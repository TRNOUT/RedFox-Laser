#pragma once

#include <QElapsedTimer>
#include <QMainWindow>

#include "show/Show.hpp"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTimer;
class EditorWindow;
class PreviewWidget;
class TimelineRuler;

namespace redfox::ilda { struct IldaFrame; }

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
    void importCue();
    void drawNewCue();
    void renameCue();
    void deleteCue();
    void onCueSelectionChanged();
    void applyCueSettings();
    void addEffect();
    void removeEffect();
    void onEffectSelectionChanged();
    void applyEffectSettings();
    void pickEffectColour();

private:
    void loadFromShow(const redfox::show::Show& show);
    void addRow(double timeSeconds, int cueIndex);
    void rebuildCueList();
    void rebuildEffectList();
    void refreshRuler();
    void commitTableToShow();  // fold the step table into show_.timeline (sorted)
    void addCueFromFrame(const redfox::ilda::IldaFrame& frame, const std::string& name);
    int selectedCue() const;   // index into show_.cues, or -1
    int selectedEffect() const; // index into the selected cue's effects, or -1
    void updateColourSwatch();
    void updatePreview();      // animate the selected cue + its effects live

    redfox::show::Show show_;
    std::string showPath_;

    QListWidget* cueList_ = nullptr;
    QDoubleSpinBox* cueFpsSpin_ = nullptr;
    QCheckBox* cueLoopCheck_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QDoubleSpinBox* durationSpin_ = nullptr;
    QCheckBox* loopCheck_ = nullptr;
    TimelineRuler* ruler_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    EditorWindow* drawEditor_ = nullptr; // vector editor for drawing new cues
    bool updatingCueSettings_ = false; // guard against feedback while populating

    // Per-cue motion-effects editor.
    QListWidget* effectList_ = nullptr;
    QComboBox* effectType_ = nullptr;
    QComboBox* effectDir_ = nullptr;
    QDoubleSpinBox* effectBpm_ = nullptr;    // stored as rateHz = BPM / 60
    QDoubleSpinBox* effectAmount_ = nullptr; // type-specific magnitude, 0..1
    QComboBox* effectAxis_ = nullptr;        // Wave axis: X / Y
    QPushButton* effectColour_ = nullptr;    // Runner highlight colour
    QCheckBox* effectEnabled_ = nullptr;
    QCheckBox* effectSync_ = nullptr;        // run at the master BPM instead of rateHz
    std::uint8_t effectR_ = 255, effectG_ = 255, effectB_ = 255;
    bool updatingEffectSettings_ = false;

    // Live, engine-free preview of the selected cue with its effects applied.
    PreviewWidget* effectPreview_ = nullptr;
    QTimer* previewTimer_ = nullptr;
    QElapsedTimer previewClock_;
};
