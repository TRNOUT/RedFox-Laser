#include "PreviewWidget.hpp"

#include <QPainter>

#include <algorithm>
#include <cmath>

namespace {
bool isDark(const redfox::ipc::PreviewPoint& p) {
    return p.r < 0.02f && p.g < 0.02f && p.b < 0.02f;
}
int to255(float c) {
    return std::clamp(static_cast<int>(std::lround(c * 255.0f)), 0, 255);
}
} // namespace

PreviewWidget::PreviewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(240, 240);
}

void PreviewWidget::setPoints(const redfox::ipc::PreviewPoint* points, std::size_t count) {
    points_.assign(points, points + count);
    update();
}

QPointF PreviewWidget::toScreen(float x, float y) const {
    const double w = width();
    const double h = height();
    return QPointF((x * 0.5 + 0.5) * w, (0.5 - y * 0.5) * h);
}

void PreviewWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), QColor(10, 10, 14));
    painter.setPen(QColor(45, 45, 55));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    for (std::size_t i = 1; i < points_.size(); ++i) {
        const redfox::ipc::PreviewPoint& p = points_[i];
        const QPointF a = toScreen(points_[i - 1].x, points_[i - 1].y);
        const QPointF b = toScreen(p.x, p.y);
        if (isDark(p)) {
            QPen pen(QColor(70, 70, 70));
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
        } else {
            painter.setPen(QPen(QColor(to255(p.r), to255(p.g), to255(p.b)), 2.0));
        }
        painter.drawLine(a, b);
    }
}
