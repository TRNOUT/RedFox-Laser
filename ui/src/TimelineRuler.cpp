#include "TimelineRuler.hpp"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kMarginX = 12;      // px kept free left and right of the ruler
constexpr int kBaselineFromBottom = 20;
constexpr int kFlagTop = 6;       // y where marker flags start
constexpr int kFlagHeight = 18;
constexpr int kHitRadiusPx = 7;   // horizontal grab distance for a marker

// A readable tick spacing: the smallest candidate that keeps ticks >= ~50 px
// apart at the current width.
double tickStepSeconds(double duration, int rulerWidthPx) {
    static constexpr double kCandidates[] = {0.1, 0.2, 0.5, 1.0, 2.0,  5.0,
                                             10.0, 15.0, 30.0, 60.0, 120.0, 300.0};
    for (double c : kCandidates) {
        if (duration <= 0.0 || rulerWidthPx * c / duration >= 50.0) {
            return c;
        }
    }
    return 600.0;
}

// One distinct colour per cue index (golden-angle hue walk).
QColor cueColor(int cueIndex) {
    return QColor::fromHsv((cueIndex * 137) % 360, 200, 230);
}

} // namespace

TimelineRuler::TimelineRuler(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(kFlagTop + kFlagHeight + kBaselineFromBottom + 28);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void TimelineRuler::setSteps(std::vector<Step> steps, double durationSeconds) {
    steps_ = std::move(steps);
    duration_ = std::max(durationSeconds, 0.001);
    if (selected_ >= static_cast<int>(steps_.size())) {
        selected_ = -1;
    }
    update();
}

void TimelineRuler::setSelected(int index) {
    selected_ = (index >= 0 && index < static_cast<int>(steps_.size())) ? index : -1;
    update();
}

int TimelineRuler::xForTime(double seconds) const {
    const int span = width() - 2 * kMarginX;
    return kMarginX + static_cast<int>(std::lround(seconds / duration_ * span));
}

double TimelineRuler::timeForX(int x) const {
    const int span = width() - 2 * kMarginX;
    if (span <= 0) {
        return 0.0;
    }
    const double t = static_cast<double>(x - kMarginX) / span * duration_;
    return std::clamp(t, 0.0, duration_);
}

int TimelineRuler::hitTest(const QPoint& pos) const {
    // Prefer the latest-added marker when several overlap (drawn on top).
    for (int i = static_cast<int>(steps_.size()) - 1; i >= 0; --i) {
        if (std::abs(pos.x() - xForTime(steps_[i].timeSeconds)) <= kHitRadiusPx) {
            return i;
        }
    }
    return -1;
}

void TimelineRuler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int baseline = height() - kBaselineFromBottom;
    const int left = kMarginX;
    const int right = width() - kMarginX;

    p.fillRect(rect(), palette().base());

    // Ruler line with tick marks and time labels.
    p.setPen(palette().color(QPalette::Mid));
    p.drawLine(left, baseline, right, baseline);
    const double tick = tickStepSeconds(duration_, right - left);
    const QFontMetrics fm = p.fontMetrics();
    for (double t = 0.0; t <= duration_ + 1e-9; t += tick) {
        const int x = xForTime(std::min(t, duration_));
        p.drawLine(x, baseline, x, baseline + 5);
        const QString label = QString::number(t, 'g', 4) + "s";
        p.drawText(x - fm.horizontalAdvance(label) / 2, baseline + 6 + fm.ascent(), label);
    }
    // End-of-sequence marker.
    p.setPen(QPen(palette().color(QPalette::Dark), 2));
    p.drawLine(right, kFlagTop, right, baseline);

    // Step markers: a stem down to the baseline and a labelled flag on top.
    for (std::size_t i = 0; i < steps_.size(); ++i) {
        const int x = xForTime(steps_[i].timeSeconds);
        const QColor color = cueColor(steps_[i].cueIndex);
        const bool isSelected = (static_cast<int>(i) == selected_);

        p.setPen(QPen(color, isSelected ? 3 : 2));
        p.drawLine(x, kFlagTop + kFlagHeight, x, baseline);

        const QString label = QString::number(steps_[i].cueIndex);
        const int flagWidth = std::max(18, fm.horizontalAdvance(label) + 8);
        QRect flag(x, kFlagTop, flagWidth, kFlagHeight);
        p.setPen(isSelected ? QPen(palette().color(QPalette::Text), 2) : QPen(color.darker(140)));
        p.setBrush(color);
        p.drawRoundedRect(flag, 3, 3);
        p.setPen(Qt::black);
        p.drawText(flag, Qt::AlignCenter, label);
    }

    if (steps_.empty()) {
        p.setPen(palette().color(QPalette::Mid));
        p.drawText(rect().adjusted(0, 0, 0, -kBaselineFromBottom), Qt::AlignCenter,
                   "Double-click to add a step");
    }
}

void TimelineRuler::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const int hit = hitTest(event->pos());
    if (hit >= 0) {
        selected_ = hit;
        dragIndex_ = hit;
        emit stepClicked(hit);
        update();
    }
}

void TimelineRuler::mouseMoveEvent(QMouseEvent* event) {
    if (dragIndex_ >= 0 && dragIndex_ < static_cast<int>(steps_.size())) {
        const double t = timeForX(event->pos().x());
        steps_[dragIndex_].timeSeconds = t; // local echo for a smooth drag
        emit stepDragged(dragIndex_, t);
        update();
    }
}

void TimelineRuler::mouseReleaseEvent(QMouseEvent*) {
    dragIndex_ = -1;
}

void TimelineRuler::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && hitTest(event->pos()) < 0) {
        emit addRequested(timeForX(event->pos().x()));
    }
}
