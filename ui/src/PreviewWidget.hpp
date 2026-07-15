#pragma once

#include <QWidget>

#include <cstddef>
#include <vector>

#include "ipc/SharedTelemetry.hpp"

// Read-only live view of the engine's current output frame. Renders the point
// stream in the -1..+1 scan field (lit segments in colour, dark points as
// dashed travel moves).
class PreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void setPoints(const redfox::ipc::PreviewPoint* points, std::size_t count);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPointF toScreen(float x, float y) const;

    std::vector<redfox::ipc::PreviewPoint> points_;
};
