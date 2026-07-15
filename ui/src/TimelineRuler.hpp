#pragma once

#include <QWidget>

#include <vector>

// A graphical view of a show timeline: a horizontal time ruler with one marker
// per step, colour-coded by cue. Markers can be dragged along the ruler to
// change their trigger time, clicked to select, and double-clicking empty space
// requests a new step at that time. The widget is a pure view: it renders the
// steps it is given and reports edits via signals; the TimelineEditor owns the
// data (its step table stays the source of truth).
class TimelineRuler : public QWidget {
    Q_OBJECT

public:
    struct Step {
        double timeSeconds = 0.0;
        int cueIndex = 0;
    };

    explicit TimelineRuler(QWidget* parent = nullptr);

    void setSteps(std::vector<Step> steps, double durationSeconds);
    void setSelected(int index); // -1 = none

signals:
    void stepDragged(int index, double newTimeSeconds);
    void stepClicked(int index);
    void addRequested(double timeSeconds);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    int xForTime(double seconds) const;
    double timeForX(int x) const;
    int hitTest(const QPoint& pos) const; // marker index or -1

    std::vector<Step> steps_;
    double duration_ = 10.0;
    int selected_ = -1;
    int dragIndex_ = -1;
};
