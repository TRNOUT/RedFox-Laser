#include "TimelineEditor.hpp"

#include "EditorWindow.hpp"
#include "PreviewWidget.hpp"
#include "TimelineRuler.hpp"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "effects/MotionEffect.hpp"
#include "ilda/IldaCodec.hpp"
#include "ipc/CommandPipeClient.hpp"
#include "ipc/Commands.hpp"
#include "ipc/SharedTelemetry.hpp"
#include "show/CuePlayback.hpp"
#include "show/DemoContent.hpp"
#include "show/ShowEditing.hpp"
#include "show/ShowFile.hpp"
#include "show/Transform.hpp"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <iterator>
#include <vector>

TimelineEditor::TimelineEditor(QWidget* parent)
    : QMainWindow(parent), showPath_(redfox::show::defaultShowFilePath()) {
    setWindowTitle("RedFox-Laser - Timeline");
    resize(760, 620);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    // --- Cues: the pool of triggerable clips the timeline references. ---
    auto* cueGroup = new QGroupBox("Cues", central);
    auto* cueLayout = new QHBoxLayout(cueGroup);
    cueList_ = new QListWidget(cueGroup);
    cueLayout->addWidget(cueList_, 1);

    auto* cueSide = new QVBoxLayout();
    auto* drawButton = new QPushButton("Draw New Cue…", cueGroup);
    auto* importButton = new QPushButton("Import .ild as Cue", cueGroup);
    auto* renameButton = new QPushButton("Rename", cueGroup);
    auto* deleteButton = new QPushButton("Delete", cueGroup);
    cueSide->addWidget(drawButton);
    cueSide->addWidget(importButton);
    cueSide->addWidget(renameButton);
    cueSide->addWidget(deleteButton);
    auto* cueSettings = new QHBoxLayout();
    cueSettings->addWidget(new QLabel("FPS", cueGroup));
    cueFpsSpin_ = new QDoubleSpinBox(cueGroup);
    cueFpsSpin_->setRange(0.1, 240.0);
    cueFpsSpin_->setDecimals(1);
    cueSettings->addWidget(cueFpsSpin_);
    cueLoopCheck_ = new QCheckBox("Loop", cueGroup);
    cueSettings->addWidget(cueLoopCheck_);
    cueSide->addLayout(cueSettings);
    cueSide->addStretch(1);
    cueLayout->addLayout(cueSide);
    layout->addWidget(cueGroup);

    // --- Motion effects for the selected cue. ---
    auto* fxGroup = new QGroupBox("Motion Effects (selected cue)", central);
    auto* fxLayout = new QHBoxLayout(fxGroup);
    effectList_ = new QListWidget(fxGroup);
    fxLayout->addWidget(effectList_, 1);

    auto* fxForm = new QFormLayout();
    effectType_ = new QComboBox(fxGroup);
    effectType_->addItems({"Rotate", "Wave", "Runner", "Strobe", "Pulse (Zoom)",
                           "Color Cycle", "Bounce", "Trace", "Twinkle"});
    fxForm->addRow("Type", effectType_);
    effectDir_ = new QComboBox(fxGroup);
    effectDir_->addItems({"Forward / CW", "Reverse / CCW", "Ping-Pong"});
    fxForm->addRow("Direction", effectDir_);
    effectBpm_ = new QDoubleSpinBox(fxGroup);
    effectBpm_->setRange(0.0, 6000.0);
    effectBpm_->setDecimals(1);
    effectBpm_->setSuffix(" BPM");
    fxForm->addRow("Speed", effectBpm_);
    effectAmount_ = new QDoubleSpinBox(fxGroup);
    effectAmount_->setRange(0.0, 1.0);
    effectAmount_->setSingleStep(0.05);
    effectAmount_->setDecimals(2);
    fxForm->addRow("Amount", effectAmount_);
    effectAxis_ = new QComboBox(fxGroup);
    effectAxis_->addItems({"X", "Y"});
    fxForm->addRow("Wave axis", effectAxis_);
    effectColour_ = new QPushButton("Colour…", fxGroup);
    fxForm->addRow("Runner colour", effectColour_);
    effectEnabled_ = new QCheckBox("Enabled", fxGroup);
    fxForm->addRow("", effectEnabled_);
    effectSync_ = new QCheckBox("Sync to master BPM", fxGroup);
    fxForm->addRow("", effectSync_);
    auto* fxButtons = new QHBoxLayout();
    auto* addEffectButton = new QPushButton("Add", fxGroup);
    auto* removeEffectButton = new QPushButton("Remove", fxGroup);
    fxButtons->addWidget(addEffectButton);
    fxButtons->addWidget(removeEffectButton);
    fxForm->addRow(fxButtons);
    fxLayout->addLayout(fxForm, 1);

    // Live, engine-free preview of the selected cue with its effects animating.
    auto* previewColumn = new QVBoxLayout();
    previewColumn->addWidget(new QLabel("Live preview", fxGroup));
    effectPreview_ = new PreviewWidget(fxGroup);
    effectPreview_->setMinimumSize(180, 180);
    previewColumn->addWidget(effectPreview_, 1);
    fxLayout->addLayout(previewColumn, 1);
    layout->addWidget(fxGroup);

    // --- Sequence-level settings. ---
    auto* settings = new QHBoxLayout();
    settings->addWidget(new QLabel("Name", central));
    nameEdit_ = new QLineEdit(central);
    settings->addWidget(nameEdit_, 1);
    settings->addWidget(new QLabel("Duration (s)", central));
    durationSpin_ = new QDoubleSpinBox(central);
    durationSpin_->setRange(0.0, 3600.0);
    durationSpin_->setDecimals(2);
    settings->addWidget(durationSpin_);
    loopCheck_ = new QCheckBox("Loop", central);
    settings->addWidget(loopCheck_);
    layout->addLayout(settings);

    // --- The graphical timeline: drag markers, double-click to add. ---
    ruler_ = new TimelineRuler(central);
    layout->addWidget(ruler_);

    // --- The step table: one row per {time, cue} trigger. ---
    table_ = new QTableWidget(0, 2, central);
    table_->setHorizontalHeaderLabels({"Time (s)", "Cue"});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(table_, 1);

    auto* buttons = new QHBoxLayout();
    auto* addButton = new QPushButton("Add Step", central);
    auto* removeButton = new QPushButton("Remove Step", central);
    auto* reloadButton = new QPushButton("Reload", central);
    auto* saveButton = new QPushButton("Save Show", central);
    buttons->addWidget(addButton);
    buttons->addWidget(removeButton);
    buttons->addStretch(1);
    buttons->addWidget(reloadButton);
    buttons->addWidget(saveButton);
    layout->addLayout(buttons);

    statusLabel_ = new QLabel(central);
    layout->addWidget(statusLabel_);

    setCentralWidget(central);

    connect(addButton, &QPushButton::clicked, this, &TimelineEditor::addStep);
    connect(removeButton, &QPushButton::clicked, this, &TimelineEditor::removeSelectedStep);
    connect(reloadButton, &QPushButton::clicked, this, &TimelineEditor::reload);
    connect(saveButton, &QPushButton::clicked, this, &TimelineEditor::save);

    connect(drawButton, &QPushButton::clicked, this, &TimelineEditor::drawNewCue);
    connect(importButton, &QPushButton::clicked, this, &TimelineEditor::importCue);
    connect(renameButton, &QPushButton::clicked, this, &TimelineEditor::renameCue);
    connect(deleteButton, &QPushButton::clicked, this, &TimelineEditor::deleteCue);
    connect(cueList_, &QListWidget::currentRowChanged, this,
            &TimelineEditor::onCueSelectionChanged);
    connect(cueFpsSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applyCueSettings(); });
    connect(cueLoopCheck_, &QCheckBox::toggled, this, [this](bool) { applyCueSettings(); });

    connect(addEffectButton, &QPushButton::clicked, this, &TimelineEditor::addEffect);
    connect(removeEffectButton, &QPushButton::clicked, this, &TimelineEditor::removeEffect);
    connect(effectList_, &QListWidget::currentRowChanged, this,
            &TimelineEditor::onEffectSelectionChanged);
    connect(effectColour_, &QPushButton::clicked, this, &TimelineEditor::pickEffectColour);
    connect(effectType_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { applyEffectSettings(); });
    connect(effectDir_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { applyEffectSettings(); });
    connect(effectAxis_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { applyEffectSettings(); });
    connect(effectBpm_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applyEffectSettings(); });
    connect(effectAmount_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applyEffectSettings(); });
    connect(effectEnabled_, &QCheckBox::toggled, this,
            [this](bool) { applyEffectSettings(); });
    connect(effectSync_, &QCheckBox::toggled, this,
            [this](bool) { applyEffectSettings(); });

    // Ruler edits flow into the table (the source of truth), whose change
    // signals then refresh the ruler — one loop, no divergence.
    connect(ruler_, &TimelineRuler::stepDragged, this, [this](int row, double t) {
        auto* timeSpin = qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 0));
        if (timeSpin) {
            timeSpin->setValue(t);
        }
    });
    connect(ruler_, &TimelineRuler::stepClicked, this,
            [this](int row) { table_->selectRow(row); });
    connect(ruler_, &TimelineRuler::addRequested, this, [this](double t) {
        addRow(t, 0);
        refreshRuler();
    });
    connect(table_, &QTableWidget::itemSelectionChanged, this,
            [this]() { ruler_->setSelected(table_->currentRow()); });
    connect(durationSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { refreshRuler(); });

    // Drive the live preview at ~30 fps from a free-running clock.
    previewClock_.start();
    previewTimer_ = new QTimer(this);
    connect(previewTimer_, &QTimer::timeout, this, &TimelineEditor::updatePreview);
    previewTimer_->start(33);

    reload();
}

void TimelineEditor::updatePreview() {
    const int cue = selectedCue();
    if (cue < 0 || show_.cues[cue].frames.empty()) {
        effectPreview_->setPoints(nullptr, 0);
        return;
    }
    const redfox::show::Cue& c = show_.cues[cue];
    const double elapsed = previewClock_.elapsed() / 1000.0;

    // Mirror the engine's FrameGenerator: pick the animation frame, apply the
    // motion effects to a copy (tempo-synced ones at a nominal 120 BPM here,
    // since the editor has no live tempo), then the cue's transform + spin.
    const std::size_t frameIndex = redfox::show::cueFrameIndexAt(c, elapsed);
    redfox::ilda::IldaFrame frame = c.frames[frameIndex];
    if (!c.effects.empty()) {
        std::vector<redfox::effects::Effect> live = c.effects;
        for (auto& e : live) {
            if (e.syncToTempo) {
                e.rateHz = 120.0f / 60.0f;
            }
        }
        redfox::effects::applyEffects(frame, live, elapsed);
    }

    redfox::show::Transform transform = c.transform;
    transform.rotationTurns += c.spinTurnsPerSec * static_cast<float>(elapsed);

    std::vector<redfox::ipc::PreviewPoint> pts;
    pts.reserve(frame.points.size());
    for (const redfox::ilda::IldaPoint& raw : frame.points) {
        const redfox::ilda::IldaPoint p = redfox::show::applyTransform(raw, transform);
        redfox::ipc::PreviewPoint pp;
        pp.x = p.x;
        pp.y = p.y;
        if (!p.blanked) {
            pp.r = static_cast<float>(p.r) / 255.0f;
            pp.g = static_cast<float>(p.g) / 255.0f;
            pp.b = static_cast<float>(p.b) / 255.0f;
        }
        pts.push_back(pp);
    }
    effectPreview_->setPoints(pts.data(), pts.size());
}

void TimelineEditor::reload() {
    const redfox::show::ShowParseResult parsed = redfox::show::readShowFile(showPath_);
    if (parsed.ok && !parsed.show.cues.empty()) {
        show_ = parsed.show;
        statusLabel_->setText(QString("Loaded %1.").arg(QString::fromStdString(showPath_)));
    } else {
        show_ = redfox::show::makeDemoShow();
        statusLabel_->setText("No saved show yet — starting from the built-in demo.");
    }
    loadFromShow(show_);
}

void TimelineEditor::loadFromShow(const redfox::show::Show& show) {
    rebuildCueList();
    nameEdit_->setText(QString::fromStdString(show.timeline.name));
    durationSpin_->setValue(show.timeline.durationSeconds);
    loopCheck_->setChecked(show.timeline.loop);

    table_->setRowCount(0);
    for (const redfox::show::TimelineStep& step : show.timeline.steps) {
        addRow(step.timeSeconds, static_cast<int>(step.cueIndex));
    }
    refreshRuler();
}

void TimelineEditor::rebuildCueList() {
    const int previous = cueList_->currentRow();
    cueList_->clear();
    for (std::size_t i = 0; i < show_.cues.size(); ++i) {
        cueList_->addItem(QString("[%1] %2")
                              .arg(i)
                              .arg(QString::fromStdString(show_.cues[i].name)));
    }
    if (!show_.cues.empty()) {
        const int row = std::clamp(previous, 0, static_cast<int>(show_.cues.size()) - 1);
        cueList_->setCurrentRow(row);
    }
    onCueSelectionChanged();
}

int TimelineEditor::selectedCue() const {
    const int row = cueList_->currentRow();
    return (row >= 0 && row < static_cast<int>(show_.cues.size())) ? row : -1;
}

void TimelineEditor::onCueSelectionChanged() {
    const int cue = selectedCue();
    updatingCueSettings_ = true;
    if (cue >= 0) {
        cueFpsSpin_->setValue(show_.cues[cue].framesPerSecond);
        cueLoopCheck_->setChecked(show_.cues[cue].loop);
        cueFpsSpin_->setEnabled(true);
        cueLoopCheck_->setEnabled(true);
    } else {
        cueFpsSpin_->setEnabled(false);
        cueLoopCheck_->setEnabled(false);
    }
    updatingCueSettings_ = false;
    rebuildEffectList();
}

void TimelineEditor::applyCueSettings() {
    if (updatingCueSettings_) {
        return;
    }
    const int cue = selectedCue();
    if (cue >= 0) {
        show_.cues[cue].framesPerSecond = static_cast<float>(cueFpsSpin_->value());
        show_.cues[cue].loop = cueLoopCheck_->isChecked();
    }
}

int TimelineEditor::selectedEffect() const {
    const int cue = selectedCue();
    if (cue < 0) {
        return -1;
    }
    const int row = effectList_->currentRow();
    return (row >= 0 && row < static_cast<int>(show_.cues[cue].effects.size())) ? row : -1;
}

void TimelineEditor::rebuildEffectList() {
    const int previous = effectList_->currentRow();
    updatingEffectSettings_ = true;
    effectList_->clear();

    const int cue = selectedCue();
    if (cue >= 0) {
        static const char* kTypeNames[] = {"Rotate",      "Wave",   "Runner",
                                            "Strobe",      "Pulse",  "Color Cycle",
                                            "Bounce",      "Trace",  "Twinkle"};
        constexpr std::size_t kTypeCount = std::size(kTypeNames);
        const auto& fx = show_.cues[cue].effects;
        for (std::size_t i = 0; i < fx.size(); ++i) {
            const auto type = static_cast<std::size_t>(fx[i].type);
            const double bpm = fx[i].rateHz * 60.0;
            QString label = QString("%1  %2 BPM")
                                .arg(kTypeNames[type < kTypeCount ? type : 0])
                                .arg(bpm, 0, 'f', 0);
            if (!fx[i].enabled) {
                label += "  (off)";
            }
            effectList_->addItem(label);
        }
        if (!fx.empty()) {
            const int row = std::clamp(previous, 0, static_cast<int>(fx.size()) - 1);
            effectList_->setCurrentRow(row);
        }
    }
    updatingEffectSettings_ = false;
    onEffectSelectionChanged();
}

void TimelineEditor::onEffectSelectionChanged() {
    const int cue = selectedCue();
    const int fx = selectedEffect();
    const bool have = (fx >= 0);

    updatingEffectSettings_ = true;
    const std::initializer_list<QWidget*> fxWidgets = {
        effectType_,  effectDir_,     effectBpm_,   effectAmount_,   effectAxis_,
        effectColour_, effectEnabled_, effectSync_};
    for (QWidget* w : fxWidgets) {
        w->setEnabled(have);
    }
    if (have) {
        const redfox::effects::Effect& e = show_.cues[cue].effects[fx];
        effectType_->setCurrentIndex(static_cast<int>(e.type));
        effectDir_->setCurrentIndex(static_cast<int>(e.direction));
        effectBpm_->setValue(e.rateHz * 60.0);
        effectAmount_->setValue(e.amount);
        effectAxis_->setCurrentIndex(e.axis == 0 ? 0 : 1);
        effectEnabled_->setChecked(e.enabled);
        effectSync_->setChecked(e.syncToTempo);
        // While synced, the effect follows the master tempo, so its own BPM is
        // irrelevant — grey it out to make that clear.
        effectBpm_->setEnabled(!e.syncToTempo);
        effectR_ = e.r;
        effectG_ = e.g;
        effectB_ = e.b;
    }
    updateColourSwatch();
    updatingEffectSettings_ = false;
}

void TimelineEditor::applyEffectSettings() {
    if (updatingEffectSettings_) {
        return;
    }
    const int cue = selectedCue();
    const int fx = selectedEffect();
    if (cue < 0 || fx < 0) {
        return;
    }
    redfox::effects::Effect& e = show_.cues[cue].effects[fx];
    e.type = static_cast<redfox::effects::EffectType>(effectType_->currentIndex());
    e.direction = static_cast<redfox::effects::Direction>(effectDir_->currentIndex());
    e.rateHz = static_cast<float>(effectBpm_->value() / 60.0);
    e.amount = static_cast<float>(effectAmount_->value());
    e.axis = static_cast<std::uint8_t>(effectAxis_->currentIndex());
    e.r = effectR_;
    e.g = effectG_;
    e.b = effectB_;
    e.enabled = effectEnabled_->isChecked();
    e.syncToTempo = effectSync_->isChecked();
    effectBpm_->setEnabled(!e.syncToTempo);

    // Refresh only the list label without disturbing the current selection.
    const int row = effectList_->currentRow();
    if (row >= 0 && row < effectList_->count()) {
        static const char* kTypeNames[] = {"Rotate",      "Wave",   "Runner",
                                            "Strobe",      "Pulse",  "Color Cycle",
                                            "Bounce",      "Trace",  "Twinkle"};
        QString label = QString("%1  %2 BPM")
                            .arg(kTypeNames[static_cast<int>(e.type)])
                            .arg(effectBpm_->value(), 0, 'f', 0);
        if (!e.enabled) {
            label += "  (off)";
        }
        effectList_->item(row)->setText(label);
    }
}

void TimelineEditor::addEffect() {
    const int cue = selectedCue();
    if (cue < 0) {
        QMessageBox::information(this, "No cue", "Select a cue first.");
        return;
    }
    redfox::effects::Effect e; // sensible defaults: 60 BPM rotate
    e.rateHz = 1.0f;
    show_.cues[cue].effects.push_back(e);
    rebuildEffectList();
    effectList_->setCurrentRow(static_cast<int>(show_.cues[cue].effects.size()) - 1);
}

void TimelineEditor::removeEffect() {
    const int cue = selectedCue();
    const int fx = selectedEffect();
    if (cue < 0 || fx < 0) {
        return;
    }
    auto& effects = show_.cues[cue].effects;
    effects.erase(effects.begin() + fx);
    rebuildEffectList();
}

void TimelineEditor::pickEffectColour() {
    const QColor initial(effectR_, effectG_, effectB_);
    const QColor chosen =
        QColorDialog::getColor(initial, this, "Runner highlight colour");
    if (!chosen.isValid()) {
        return;
    }
    effectR_ = static_cast<std::uint8_t>(chosen.red());
    effectG_ = static_cast<std::uint8_t>(chosen.green());
    effectB_ = static_cast<std::uint8_t>(chosen.blue());
    updateColourSwatch();
    applyEffectSettings();
}

void TimelineEditor::updateColourSwatch() {
    const QColor c(effectR_, effectG_, effectB_);
    // Pick readable text against the swatch background.
    const int luma = (c.red() * 299 + c.green() * 587 + c.blue() * 114) / 1000;
    const char* fg = luma > 128 ? "#000000" : "#ffffff";
    effectColour_->setStyleSheet(
        QString("background-color:%1;color:%2;").arg(c.name()).arg(fg));
    effectColour_->setText(c.name().toUpper());
}

void TimelineEditor::importCue() {
    const QString path =
        QFileDialog::getOpenFileName(this, "Import ILDA as cue", {}, "ILDA files (*.ild)");
    if (path.isEmpty()) {
        return;
    }
    const redfox::ilda::ParseResult parsed =
        redfox::ilda::readIldaFile(path.toStdString());
    if (!parsed.ok || parsed.frames.empty()) {
        QMessageBox::warning(this, "Import failed",
                             QString::fromStdString(parsed.ok ? "File has no frames."
                                                              : parsed.error));
        return;
    }

    std::string name = std::filesystem::path(path.toStdString()).stem().string();
    if (name.empty()) {
        name = "Imported";
    }
    // An imported .ild may hold an animation; use all its frames.
    redfox::show::Cue cue;
    cue.name = name;
    cue.frames = parsed.frames;

    commitTableToShow(); // preserve current step edits before rebuilding
    show_.cues.push_back(std::move(cue));
    loadFromShow(show_);
    cueList_->setCurrentRow(static_cast<int>(show_.cues.size()) - 1);
    statusLabel_->setText(QString("Imported cue %1 (%2 frames).")
                              .arg(QString::fromStdString(name))
                              .arg(parsed.frames.size()));
}

void TimelineEditor::drawNewCue() {
    if (!drawEditor_) {
        drawEditor_ = new EditorWindow(this);
        drawEditor_->enableAddAsCue();
        // Each "Add as Cue" click turns the current drawing into a show cue.
        connect(drawEditor_, &EditorWindow::frameReady, this,
                [this](const redfox::ilda::IldaFrame& frame) {
                    if (frame.points.empty()) {
                        QMessageBox::information(this, "Empty frame",
                                                 "Draw at least one point first.");
                        return;
                    }
                    bool ok = false;
                    const QString name = QInputDialog::getText(
                        this, "New cue", "Cue name:", QLineEdit::Normal,
                        QString("Cue %1").arg(show_.cues.size()), &ok);
                    if (ok) {
                        addCueFromFrame(frame, name.isEmpty() ? "Cue" : name.toStdString());
                    }
                });
    }
    drawEditor_->show();
    drawEditor_->raise();
    drawEditor_->activateWindow();
}

void TimelineEditor::addCueFromFrame(const redfox::ilda::IldaFrame& frame,
                                     const std::string& name) {
    redfox::show::Cue cue;
    cue.name = name;
    cue.frames = {frame};

    commitTableToShow(); // preserve current step edits before rebuilding
    show_.cues.push_back(std::move(cue));
    loadFromShow(show_);
    cueList_->setCurrentRow(static_cast<int>(show_.cues.size()) - 1);
    statusLabel_->setText(QString("Added drawn cue %1.")
                              .arg(QString::fromStdString(name)));
}

void TimelineEditor::renameCue() {
    const int cue = selectedCue();
    if (cue < 0) {
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, "Rename cue", "Name:", QLineEdit::Normal,
        QString::fromStdString(show_.cues[cue].name), &ok);
    if (ok && !name.isEmpty()) {
        show_.cues[cue].name = name.toStdString();
        rebuildCueList();
    }
}

void TimelineEditor::deleteCue() {
    const int cue = selectedCue();
    if (cue < 0) {
        return;
    }
    commitTableToShow(); // keep the current step edits, then fix up indices
    redfox::show::removeCue(show_, static_cast<std::size_t>(cue));
    loadFromShow(show_);
    statusLabel_->setText("Cue deleted; timeline steps updated.");
}

void TimelineEditor::refreshRuler() {
    std::vector<TimelineRuler::Step> steps;
    steps.reserve(static_cast<std::size_t>(table_->rowCount()));
    for (int row = 0; row < table_->rowCount(); ++row) {
        auto* timeSpin = qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 0));
        auto* cueSpin = qobject_cast<QSpinBox*>(table_->cellWidget(row, 1));
        if (timeSpin && cueSpin) {
            steps.push_back({timeSpin->value(), cueSpin->value()});
        }
    }
    ruler_->setSteps(std::move(steps), durationSpin_->value());
}

void TimelineEditor::addRow(double timeSeconds, int cueIndex) {
    const int row = table_->rowCount();
    table_->insertRow(row);

    auto* timeSpin = new QDoubleSpinBox(table_);
    timeSpin->setRange(0.0, 3600.0);
    timeSpin->setDecimals(2);
    timeSpin->setValue(timeSeconds);
    table_->setCellWidget(row, 0, timeSpin);

    auto* cueSpin = new QSpinBox(table_);
    const int maxCue = show_.cues.empty() ? 0 : static_cast<int>(show_.cues.size()) - 1;
    cueSpin->setRange(0, maxCue);
    cueSpin->setValue(std::min(std::max(cueIndex, 0), maxCue));
    table_->setCellWidget(row, 1, cueSpin);

    // Any table edit repaints the graphical timeline.
    connect(timeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { refreshRuler(); });
    connect(cueSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int) { refreshRuler(); });
}

void TimelineEditor::addStep() {
    // New steps land at the end of the sequence by default.
    addRow(durationSpin_->value(), 0);
    refreshRuler();
}

void TimelineEditor::removeSelectedStep() {
    const int row = table_->currentRow();
    if (row >= 0) {
        table_->removeRow(row);
        refreshRuler();
    }
}

void TimelineEditor::commitTableToShow() {
    redfox::show::Timeline timeline;
    timeline.name = nameEdit_->text().toStdString();
    timeline.durationSeconds = durationSpin_->value();
    timeline.loop = loopCheck_->isChecked();
    for (int row = 0; row < table_->rowCount(); ++row) {
        auto* timeSpin = qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 0));
        auto* cueSpin = qobject_cast<QSpinBox*>(table_->cellWidget(row, 1));
        if (timeSpin && cueSpin) {
            timeline.steps.push_back(
                {timeSpin->value(), static_cast<std::size_t>(cueSpin->value())});
        }
    }
    std::sort(timeline.steps.begin(), timeline.steps.end(),
              [](const redfox::show::TimelineStep& a, const redfox::show::TimelineStep& b) {
                  return a.timeSeconds < b.timeSeconds;
              });
    show_.timeline = timeline;
}

void TimelineEditor::save() {
    commitTableToShow();

    if (!redfox::show::writeShowFile(showPath_, show_)) {
        QMessageBox::warning(this, "Save failed",
                             QString("Could not write %1.")
                                 .arg(QString::fromStdString(showPath_)));
        return;
    }
    loadFromShow(show_); // reflect the sorted order back into table and ruler

    // Tell a running engine to pick the saved show up immediately.
    redfox::ipc::CommandPipeClient commands;
    if (commands.isConnected() &&
        commands.send(redfox::ipc::CommandType::ReloadShow)) {
        statusLabel_->setText(QString("Saved %1 steps — engine reloaded live.")
                                  .arg(show_.timeline.steps.size()));
    } else {
        statusLabel_->setText(
            QString("Saved to %1. Engine not running — it will load the show at "
                    "next start.")
                .arg(QString::fromStdString(showPath_)));
    }
}
