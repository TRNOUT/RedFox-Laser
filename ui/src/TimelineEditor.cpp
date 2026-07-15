#include "TimelineEditor.hpp"

#include "TimelineRuler.hpp"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
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
#include <QVBoxLayout>
#include <QWidget>

#include "ilda/IldaCodec.hpp"
#include "ipc/CommandPipeClient.hpp"
#include "ipc/Commands.hpp"
#include "show/DemoContent.hpp"
#include "show/ShowEditing.hpp"
#include "show/ShowFile.hpp"

#include <algorithm>
#include <filesystem>

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
    auto* importButton = new QPushButton("Import .ild as Cue", cueGroup);
    auto* renameButton = new QPushButton("Rename", cueGroup);
    auto* deleteButton = new QPushButton("Delete", cueGroup);
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

    connect(importButton, &QPushButton::clicked, this, &TimelineEditor::importCue);
    connect(renameButton, &QPushButton::clicked, this, &TimelineEditor::renameCue);
    connect(deleteButton, &QPushButton::clicked, this, &TimelineEditor::deleteCue);
    connect(cueList_, &QListWidget::currentRowChanged, this,
            &TimelineEditor::onCueSelectionChanged);
    connect(cueFpsSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applyCueSettings(); });
    connect(cueLoopCheck_, &QCheckBox::toggled, this, [this](bool) { applyCueSettings(); });

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

    reload();
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

    redfox::show::Cue cue;
    cue.name = std::filesystem::path(path.toStdString()).stem().string();
    if (cue.name.empty()) {
        cue.name = "Imported";
    }
    cue.frames = parsed.frames;

    commitTableToShow(); // preserve current step edits before rebuilding
    show_.cues.push_back(std::move(cue));
    loadFromShow(show_);
    cueList_->setCurrentRow(static_cast<int>(show_.cues.size()) - 1);
    statusLabel_->setText(QString("Imported cue %1 (%2 frames).")
                              .arg(QString::fromStdString(show_.cues.back().name))
                              .arg(parsed.frames.size()));
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
