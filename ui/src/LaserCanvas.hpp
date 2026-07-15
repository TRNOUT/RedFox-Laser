#pragma once

#include <QColor>
#include <QWidget>

#include "ilda/IldaTypes.hpp"

// A drawing surface for one ILDA frame. The square represents the -1..+1 scan
// field. Left-click adds a point at the cursor (in the current colour, or
// blanked); right-click removes the last point. Blanked points render as faint
// dashed "travel" moves, lit points/segments in their colour.
class LaserCanvas : public QWidget {
    Q_OBJECT

public:
    explicit LaserCanvas(QWidget* parent = nullptr);

    const redfox::ilda::IldaFrame& frame() const { return frame_; }
    void setFrame(const redfox::ilda::IldaFrame& frame);
    void clearFrame();
    void undoPoint();

    void setDrawColor(const QColor& color) { color_ = color; }
    QColor drawColor() const { return color_; }
    void setNextBlanked(bool blanked) { nextBlanked_ = blanked; }

signals:
    void pointCountChanged(int count);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QPointF toScreen(float x, float y) const;

    redfox::ilda::IldaFrame frame_;
    QColor color_{255, 255, 255};
    bool nextBlanked_ = false;
};
