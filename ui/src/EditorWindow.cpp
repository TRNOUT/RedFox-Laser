#include "EditorWindow.hpp"

#include "LaserCanvas.hpp"

#include <QColorDialog>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "ilda/IldaCodec.hpp"

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("RedFox-Laser - Vector Editor");

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    canvas_ = new LaserCanvas(central);
    layout->addWidget(canvas_, 1);

    auto* tools = new QHBoxLayout();
    auto* colorButton = new QPushButton("Colour...", central);
    auto* blankButton = new QPushButton("Blank point", central);
    blankButton->setCheckable(true);
    auto* undoButton = new QPushButton("Undo", central);
    auto* clearButton = new QPushButton("Clear", central);
    auto* loadButton = new QPushButton("Load .ild", central);
    auto* saveButton = new QPushButton("Save .ild", central);
    addAsCueButton_ = new QPushButton("Add as Cue", central);
    addAsCueButton_->setVisible(false); // shown only when hosted by the timeline
    pointCountLabel_ = new QLabel("0 points", central);

    tools->addWidget(colorButton);
    tools->addWidget(blankButton);
    tools->addWidget(undoButton);
    tools->addWidget(clearButton);
    tools->addWidget(loadButton);
    tools->addWidget(saveButton);
    tools->addWidget(addAsCueButton_);
    tools->addStretch(1);
    tools->addWidget(pointCountLabel_);
    layout->addLayout(tools);

    layout->insertWidget(0, new QLabel(
        "Left-click to add a point, right-click to remove the last one.", central));

    setCentralWidget(central);

    connect(colorButton, &QPushButton::clicked, this, &EditorWindow::pickColor);
    connect(blankButton, &QPushButton::toggled, this, &EditorWindow::toggleBlank);
    connect(undoButton, &QPushButton::clicked, canvas_, &LaserCanvas::undoPoint);
    connect(clearButton, &QPushButton::clicked, canvas_, &LaserCanvas::clearFrame);
    connect(loadButton, &QPushButton::clicked, this, &EditorWindow::loadIlda);
    connect(saveButton, &QPushButton::clicked, this, &EditorWindow::saveIlda);
    connect(canvas_, &LaserCanvas::pointCountChanged, this, [this](int count) {
        pointCountLabel_->setText(QString("%1 points").arg(count));
    });
    connect(addAsCueButton_, &QPushButton::clicked, this,
            [this]() { emit frameReady(canvas_->frame()); });
}

void EditorWindow::enableAddAsCue() {
    addAsCueButton_->setVisible(true);
}

void EditorWindow::pickColor() {
    const QColor chosen = QColorDialog::getColor(canvas_->drawColor(), this, "Draw colour");
    if (chosen.isValid()) {
        canvas_->setDrawColor(chosen);
    }
}

void EditorWindow::toggleBlank(bool blanked) { canvas_->setNextBlanked(blanked); }

void EditorWindow::saveIlda() {
    const QString path =
        QFileDialog::getSaveFileName(this, "Save ILDA frame", {}, "ILDA files (*.ild)");
    if (path.isEmpty()) {
        return;
    }
    redfox::ilda::IldaFrame frame = canvas_->frame();
    if (frame.name.empty()) {
        frame.name = "EDIT";
    }
    if (!redfox::ilda::writeIldaFile(path.toStdString(), {frame})) {
        QMessageBox::warning(this, "Save failed", "Could not write " + path);
    }
}

void EditorWindow::loadIlda() {
    const QString path =
        QFileDialog::getOpenFileName(this, "Load ILDA frame", {}, "ILDA files (*.ild)");
    if (path.isEmpty()) {
        return;
    }
    const redfox::ilda::ParseResult parsed =
        redfox::ilda::readIldaFile(path.toStdString());
    if (!parsed.ok || parsed.frames.empty()) {
        QMessageBox::warning(this, "Load failed",
                             QString::fromStdString(parsed.ok ? "File has no frames."
                                                              : parsed.error));
        return;
    }
    canvas_->setFrame(parsed.frames.front());
}
