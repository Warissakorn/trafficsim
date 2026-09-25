#include "canvas.hpp"
#include <QApplication>
#include <QGraphicsLineItem>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

namespace trafficsim {
// A Signal head IS its stop line: the runtime holds a vehicle at the head's station (the
// clamp in core/simulation.cpp), so the canvas draws the line there, across the lane it holds,
// and a click puts it exactly where the pointer is -- on a Link lane or a Connector path.
namespace {
std::pair<Point,Point> barEnds(const std::vector<Point>& g, double station, double width) {
    const double length = polylineLength(g);
    const double s = std::clamp(station, 0.0, length);
    const auto a = pointAlong(g, std::max(0.0, s - .05)), b = pointAlong(g, std::min(length, s + .05));
    const auto at = pointAlong(g, s);
    double dx = b.x - a.x, dy = b.y - a.y; const double n = std::hypot(dx, dy);
    if (n <= 0) { dx = 1; dy = 0; } else { dx /= n; dy /= n; }
    const double h = width / 2;
    return {{at.x - dy * h, at.y + dx * h}, {at.x + dy * h, at.y - dx * h}};
}
}
std::optional<EditorCanvas::HeadGeometry> EditorCanvas::headGeometry(const NetworkSignalHead& head) const {
    if (!document_) return {};
    const auto& n = document_->network;
    if (head.connectorId.empty()) {
        for (const auto& l : n.links) if (l.id == head.lane.linkId)
            for (const auto& lane : l.lanes) if (lane.id == head.lane.laneId)
                return HeadGeometry{laneGeometry(l, lane.id, n.drivingSide), lane.width, l.level};
        return {};
    }
    for (const auto& c : n.connectors) for (const auto& path : cachedPaths(c)) if (path.id == head.connectorId) {
        double width = 3.5;
        for (const auto& l : n.links) for (const auto& lane : l.lanes) if (lane.id == path.from.laneId) width = lane.width;
        return HeadGeometry{path.geometry, width, c.level};
    }
    return {};
}
std::optional<HeadPlacement> EditorCanvas::headAt(Point p) const {
    if (!document_) return {};
    // The level is the one of whatever is on top under the pointer, so a head never lands on a
    // bridge's hidden underside -- the same rule every other pick on this canvas follows.
    for (const auto& hit : hitObjects(p)) {
        const auto level = objectLevel(hit.first);
        if (!level || !levelVisible(*level)) continue;
        if (auto placed = nearestHeadSlot(document_->network, p, *level)) return placed;
    }
    return {};
}
bool EditorCanvas::headPress(QMouseEvent* e) {
    if (tool_ != Tool::head) return false;
    const bool ctrlRight = e->button() == Qt::RightButton && (e->modifiers() & Qt::ControlModifier);
    if (e->button() != Qt::LeftButton && !ctrlRight) return false;
    const auto placed = headAt(world(e->pos(), false));
    hoverHead_.reset();
    if (!placed) { reject(); return true; }
    redraw();
    if (headPlaced) headPlaced(*placed);
    return true;
}
bool EditorCanvas::headHover(QMouseEvent* e) {
    if (tool_ != Tool::head) return false;
    auto placed = headAt(world(e->pos(), false));
    const bool same = placed.has_value() == hoverHead_.has_value() &&
        (!placed || (placed->slot.lane == hoverHead_->slot.lane && placed->slot.connectorId == hoverHead_->slot.connectorId &&
                     std::abs(placed->station - hoverHead_->station) < .05));
    if (same) return true;
    hoverHead_ = std::move(placed); redraw();
    return true;
}
bool EditorCanvas::startHeadDrag(const std::string& id, QPoint press) {
    if (!document_ || tool_ != Tool::select) return false;
    for (const auto& h : document_->network.signalHeads) if (h.id == id) {
        const auto geometry = headGeometry(h);
        if (!geometry || geometry->points.size() < 2) return false;
        headDrag_ = HeadDrag{id, geometry->points, h.position, h.position, false};
        dragPress_ = press;
        return true;
    }
    return false;
}
void EditorCanvas::updateHeadDrag(QPoint position) {
    if (!headDrag_) return;
    if ((position - dragPress_).manhattanLength() < QApplication::startDragDistance() && !headDrag_->moved) return;
    headDrag_->moved = true;
    // Along its own lane only: moving a stop line to another lane is a different object, and
    // the dialog is where that is chosen.
    const double length = polylineLength(headDrag_->geometry);
    headDrag_->station = std::clamp(stationOfClosestPoint(headDrag_->geometry, world(position, false)), 0.0, length);
    redraw();
}
void EditorCanvas::finishHeadDrag(QPoint position) {
    if (!headDrag_) return;
    updateHeadDrag(position); // the release is authoritative: the last move may be coalesced
    const auto drag = *headDrag_; headDrag_.reset();
    redraw();
    if (drag.moved && std::abs(drag.station - drag.original) > 1e-9 && headMoved) headMoved(drag.id, drag.station);
}
void EditorCanvas::drawHeads() {
    if (!document_) return;
    const auto bar = [&](const std::vector<Point>& g, double station, double width, int level, QColor fill, bool dashed) {
        const auto [a, b] = barEnds(g, station, width);
        QPen under(QColor("#1f2937"), 5); under.setCosmetic(true); under.setCapStyle(Qt::FlatCap);
        QPen over(fill, 3, dashed ? Qt::DashLine : Qt::SolidLine); over.setCosmetic(true); over.setCapStyle(Qt::FlatCap);
        if (!dashed) scene_.addLine(a.x, a.y, b.x, b.y, under)->setZValue(level * 100. + 10);
        auto* line = scene_.addLine(a.x, a.y, b.x, b.y, over); line->setZValue(level * 100. + 10.5);
        line->setData(0, QStringLiteral("stop-line"));
    };
    for (const auto& head : document_->network.signalHeads) {
        const auto geometry = headGeometry(head);
        if (!geometry || geometry->points.size() < 2 || !levelVisible(geometry->level)) continue;
        const bool dragged = headDrag_ && headDrag_->id == head.id && headDrag_->moved;
        const double station = dragged ? headDrag_->station : head.position;
        bar(geometry->points, station, geometry->width, geometry->level,
            isSelected(head.id) ? QColor("#ffb454") : QColor(Qt::white), false);
        const auto p = pointAlong(geometry->points, station); const double r = 3 / std::abs(transform().m11());
        scene_.addEllipse(p.x - r, p.y - r, 2 * r, 2 * r, QPen(Qt::darkGray), QBrush(QColor("#dc2626")))
            ->setZValue(geometry->level * 100. + 11);
    }
    if (hoverHead_ && tool_ == Tool::head)
        bar(hoverHead_->slot.geometry, hoverHead_->station, hoverHead_->slot.width, hoverHead_->slot.level, QColor("#167b98"), true);
}
std::optional<std::pair<Point,Point>> EditorCanvas::headBar(const NetworkSignalHead& head) const {
    const auto geometry = headGeometry(head);
    if (!geometry || geometry->points.size() < 2) return std::nullopt;
    return barEnds(geometry->points, head.position, geometry->width);
}
QPainterPath EditorCanvas::headShape(const NetworkSignalHead& head) const {
    QPainterPath shape;
    const auto bar = headBar(head);
    if (!bar) return shape;
    const auto [a, b] = *bar;
    // Wide enough to hit at any zoom: four pixels either side of the line.
    const double pad = 4 / std::abs(transform().m11());
    const double dx = b.x - a.x, dy = b.y - a.y, n = std::max(1e-9, std::hypot(dx, dy));
    const double ox = -dy / n * pad, oy = dx / n * pad;
    shape.moveTo(a.x + ox, a.y + oy); shape.lineTo(b.x + ox, b.y + oy);
    shape.lineTo(b.x - ox, b.y - oy); shape.lineTo(a.x - ox, a.y - oy); shape.closeSubpath();
    return shape;
}
}
