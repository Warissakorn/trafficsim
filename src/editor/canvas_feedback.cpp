#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsPathItem>
#include <QEvent>
#include <cmath>

namespace trafficsim {
void EditorCanvas::clearSelectionState() {
    selection_.clear(); vertex_ = -1;
    clearHighlights();
}
void EditorCanvas::clearHighlights() {
    highlightedRoute_.clear(); highlightedConflict_.clear();
    if (selectionCleared) selectionCleared();
}
void EditorCanvas::clearSelection(bool cancelGesture) {
    if (cancelGesture) resetGesture();
    clearSelectionState();
    notifySelection();
}
void EditorCanvas::clearHover() {
    hoverObject_.clear(); hoverConflict_.clear(); hoverAutomatic_.clear(); hoverWaitingLine_.clear();
    hoverVertex_ = -1; hoverLaneKind_ = 0; hoverLaneLocation_ = -1;
    hoverSegment_.clear(); hoverHead_.reset(); connectorHover_.reset();
    setCursor(tool_ == Tool::select ? Qt::ArrowCursor : Qt::CrossCursor);
}
void EditorCanvas::updateHover(QPoint position) {
    std::string object, conflict, automatic, waiting;
    int vertex = -1, laneKind = 0, laneLocation = -1;
    auto cursor = tool_ == Tool::select ? Qt::ArrowCursor : Qt::CrossCursor;
    const auto p = world(position, false);
    if (tool_ == Tool::select) {
        if (const auto h = laneHandleAt(position)) {
            laneKind = h->kind; laneLocation = h->location;
            const double x = std::abs(h->direction.x), y = std::abs(h->direction.y);
            cursor = x > 2 * y ? Qt::SizeHorCursor : y > 2 * x ? Qt::SizeVerCursor :
                     h->direction.x * h->direction.y > 0 ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
        } else if ((vertex = vertexAt(position)) >= 0) cursor = Qt::SizeAllCursor;
        else { object = hit(p).first; if (!object.empty()) cursor = Qt::OpenHandCursor; }
    } else if (tool_ == Tool::split || tool_ == Tool::connect) {
        object = hit(p, false).first;
    } else if (tool_ == Tool::conflict) {
        waiting = waitingLineAt(p);
        if (waiting.empty()) {
            const auto areas = conflictsAt(p);
            if (!areas.empty()) conflict = areas.front();
            else automatic = automaticAt(p);
        }
        if (!waiting.empty()) cursor = Qt::SizeAllCursor;
        else if (!conflict.empty() || !automatic.empty()) cursor = Qt::PointingHandCursor;
    } else if (tool_ == Tool::counter && counterLineAt(p)) object = hit(p).first;
    const bool changed = object != hoverObject_ || conflict != hoverConflict_ || automatic != hoverAutomatic_ ||
        waiting != hoverWaitingLine_ || vertex != hoverVertex_ || laneKind != hoverLaneKind_ || laneLocation != hoverLaneLocation_;
    hoverObject_ = std::move(object); hoverConflict_ = std::move(conflict);
    hoverAutomatic_ = std::move(automatic); hoverWaitingLine_ = std::move(waiting);
    hoverVertex_ = vertex; hoverLaneKind_ = laneKind; hoverLaneLocation_ = laneLocation;
    setCursor(cursor);
    if (changed) redraw();
}
void EditorCanvas::leaveEvent(QEvent* event) {
    clearHover(); redraw();
    QGraphicsView::leaveEvent(event);
}
void EditorCanvas::drawObjectFeedback(const std::string& id, const QPainterPath& shape, double z) {
    const bool chosen = isSelected(id), hovered = id == hoverObject_;
    if ((!chosen && !hovered) || shape.isEmpty()) return;
    const bool primary = id == selected();
    const QColor colour = chosen ? canvasStyle::selection : canvasStyle::hover;
    QPen pen(colour, chosen && primary ? 2 : 1.5); pen.setCosmetic(true); pen.setJoinStyle(Qt::RoundJoin);
    QColor fill = colour; fill.setAlpha(chosen ? 32 : 16);
    auto* item = scene_.addPath(shape, pen, QBrush(fill)); item->setZValue(z);
    item->setData(0, QStringLiteral("object-feedback")); item->setData(1, QString::fromStdString(id));
    item->setData(2, chosen ? QStringLiteral("selected") : QStringLiteral("hover"));
}
void EditorCanvas::drawGeometryHandles(const std::string& id, const std::vector<Point>& points, bool connector) {
    if (tool_ != Tool::select || selection_.size() != 1 || id != selected() || copyDragging_ || rotationPivot_) return;
    const double r = 3.5 / std::abs(transform().m11());
    for (std::size_t i = 0; i < points.size(); ++i) {
        const bool end = connector && (i == 0 || i + 1 == points.size());
        const bool held = end ? endpointDrag_ && *endpointDrag_ == (i == 0) : dragging_ && vertex_ == static_cast<int>(i);
        const bool hovered = hoverVertex_ == static_cast<int>(i);
        const QColor colour = held ? canvasStyle::active : canvasStyle::selection;
        QPen pen(colour, hovered || held ? 2 : 1.5); pen.setCosmetic(true);
        auto* item = scene_.addRect(points[i].x - r, points[i].y - r, 2*r, 2*r, pen,
                                    QBrush(held || hovered ? colour : QColor(Qt::white)));
        item->setZValue(200012);
        item->setData(0, end ? QStringLiteral("connector-end") : QStringLiteral("geometry-point"));
        item->setData(1, QString::fromStdString(id));
        item->setData(2, end ? QVariant(i == 0) : QVariant(static_cast<int>(i)));
    }
}
}
