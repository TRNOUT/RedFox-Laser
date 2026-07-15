#include "TimelineEditor.hpp"

#include "TimelineRuler.hpp"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "ipc/CommandPipeClient.hpp"
#include "ipc/Commands.hpp"
#include "show/DemoContent.hpp"
#include "show/ShowFile.hpp"

#include <algorithm>

TimelineEditor::TimelineEditor(QWidget* parent)
    : QMainWindow(parent), showPath_(redfox::show::defaultShowFilePath()) {
    setWindowTitle("RedFox-Laser - Timeline");
    resize(720, 520);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    cueLegend_ = new QLabel(central);
    cueLegend_->setWordWrap(true);
    layout->addWidget(cueLegend_);

    // Sequence-level settings.
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

    // The graphical timeline: drag a marker to move its step in time,
    // double-click empty space to add one. Mirrors the table below.
    ruler_ = new TimelineRuler(central);
    layout->addWidget(ruler_);

    // The step table: one row per {time, cue} trigger.
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
    rebuildCueLegend();
    nameEdit_->setText(QString::fromStdString(show.timeline.name));
    durationSpin_->setValue(show.timeline.durationSeconds);
    loopCheck_->setChecked(show.timeline.loop);

    table_->setRowCount(0);
    for (const redfox::show::TimelineStep& step : show.timeline.steps) {
        addRow(step.timeSeconds, static_cast<int>(step.cueIndex));
    }
    refreshRuler();
}

void TimelineEditor::rebuildCueLegend() {
    QString text = QString("Cues in \"%1\":  ").arg(QString::fromStdString(show_.name));
    for (std::size_t i = 0; i < show_.cues.size(); ++i) {
        text += QString("[%1] %2   ")
                    .arg(i)
                    .arg(QString::fromStdString(show_.cues[i].name));
    }
    cueLegend_->setText(text);
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

void TimelineEditor::save() {
    // Gather the table back into the show's timeline, keeping it sorted by time
    // so the sequencer (which assumes ascending order) plays it correctly.
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
                                  .arg(timeline.steps.size()));
    } else {
        statusLabel_->setText(
            QString("Saved %1 steps to %2. Engine not running — it will load "
                    "the show at next start.")
                .arg(timeline.steps.size())
                .arg(QString::fromStdString(showPath_)));
    }
}
