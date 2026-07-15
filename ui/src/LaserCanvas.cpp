#include "LaserCanvas.hpp"

#include <QMouseEvent>
#include <QPainter>

LaserCanvas::LaserCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(360, 360);
    setAutoFillBackground(false);
}

void LaserCanvas::setFrame(const redfox::ilda::IldaFrame& frame) {
    frame_ = frame;
    emit pointCountChanged(static_cast<int>(frame_.points.size()));
    update();
}

void LaserCanvas::clearFrame() {
    frame_.points.clear();
    emit pointCountChanged(0);
    update();
}

void LaserCanvas::undoPoint() {
    if (!frame_.points.empty()) {
        frame_.points.pop_back();
        emit pointCountChanged(static_cast<int>(frame_.points.size()));
        update();
    }
}

QPointF LaserCanvas::toScreen(float x, float y) const {
    const double w = width();
    const double h = height();
    // -1..+1 -> pixels, flipping y so positive is up.
    return QPointF((x * 0.5 + 0.5) * w, (0.5 - y * 0.5) * h);
}

void LaserCanvas::mousePressEvent(QMouseEvent* event) {
    const double w = width();
    const double h = height();
    if (w <= 0 || h <= 0) {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        const float x = static_cast<float>(event->position().x() / w * 2.0 - 1.0);
        const float y = static_cast<float>(1.0 - event->position().y() / h * 2.0);
        redfox::ilda::IldaPoint p;
        p.x = x;
        p.y = y;
        p.blanked = nextBlanked_;
        if (!nextBlanked_) {
            p.r = static_cast<std::uint8_t>(color_.red());
            p.g = static_cast<std::uint8_t>(color_.green());
            p.b = static_cast<std::uint8_t>(color_.blue());
        }
        frame_.points.push_back(p);
        emit pointCountChanged(static_cast<int>(frame_.points.size()));
        update();
    } else if (event->button() == Qt::RightButton) {
        undoPoint();
    }
}

void LaserCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Scan-field background and border.
    painter.fillRect(rect(), QColor(18, 18, 22));
    painter.setPen(QColor(60, 60, 70));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    // Centre cross-hair.
    painter.drawLine(toScreen(-1, 0), toScreen(1, 0));
    painter.drawLine(toScreen(0, -1), toScreen(0, 1));

    const auto& pts = frame_.points;
    for (std::size_t i = 1; i < pts.size(); ++i) {
        const QPointF a = toScreen(pts[i - 1].x, pts[i - 1].y);
        const QPointF b = toScreen(pts[i].x, pts[i].y);
        if (pts[i].blanked) {
            QPen pen(QColor(90, 90, 90));
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
        } else {
            painter.setPen(QPen(QColor(pts[i].r, pts[i].g, pts[i].b), 2.0));
        }
        painter.drawLine(a, b);
    }

    // Point markers.
    for (const auto& p : pts) {
        const QPointF s = toScreen(p.x, p.y);
        painter.setPen(Qt::NoPen);
        painter.setBrush(p.blanked ? QColor(120, 120, 120)
                                   : QColor(p.r, p.g, p.b));
        painter.drawEllipse(s, 3.0, 3.0);
    }
}
