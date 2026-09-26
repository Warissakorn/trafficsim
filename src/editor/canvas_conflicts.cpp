#include "canvas.hpp"
#include "../model/network/right_of_way.hpp"
#include <QApplication>
#include <QGraphicsPathItem>
#include <QMouseEvent>
#include <QPen>
#include <algorithm>
#include <cmath>

// M3.2.4: authored conflict areas and waiting lines on the canvas, drawn from the same lane
// strips the resolver measures coverage with (conflictSideOutline), so what is shown is what runs.
namespace trafficsim {
namespace {
QPainterPath outlinePath(const std::vector<Point>& points) {
    QPainterPath shape(QPointF(points.front().x, points.front().y));
    for (std::size_t i = 1; i < points.size(); ++i) shape.lineTo(points[i].x, points[i].y);
    shape.closeSubpath();
    return shape;
}
int levelOf(const Network& n, const ControlPathRef& ref) {
    for (const auto& l : n.links) if (l.id == ref.linkId) return l.level;
    for (const auto& c : n.connectors) if (c.id == ref.connectorId) return c.level;
    return 0;
}
double segmentDistance(Point p, Point a, Point b) {
    const double dx = b.x - a.x, dy = b.y - a.y, n = dx * dx + dy * dy;
    const double t = n <= 0 ? 0 : std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / n, 0.0, 1.0);
    return std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy);
}
}
void EditorCanvas::setHighlightedConflict(std::string id) {
    if (highlightedConflict_ == id) return;
    highlightedConflict_ = std::move(id); redraw();
}
std::vector<std::string> EditorCanvas::conflictsAt(Point p) const {
    std::vector<std::string> found;
    if (!document_) return found;
    const auto& n = document_->network;
    for (const auto& area : n.rightOfWay.conflictAreas)
        for (const auto* side : {&area.first, &area.second}) {
            if (!levelVisible(levelOf(n, side->path))) continue;
            const auto outline = conflictSideOutline(n, *side);
            if (outline.size() >= 3 && outlinePath(outline).contains(QPointF(p.x, p.y))) { found.push_back(area.id); break; }
        }
    std::sort(found.begin(), found.end());
    return found;
}
std::string EditorCanvas::waitingLineAt(Point p) const {
    if (!document_) return {};
    const auto& n = document_->network;
    // Four pixels either side at any zoom, as a stop line is hit (headShape).
    double best = 4 / std::abs(transform().m11());
    std::string found;
    for (const auto& line : n.rightOfWay.waitingLines) {
        if (!levelVisible(levelOf(n, line.point.path))) continue;
        const auto bar = waitingLineBar(n, line.point);
        if (!bar) continue;
        const double d = segmentDistance(p, bar->first, bar->second);
        if (d <= best) { best = d; found = line.id; }
    }
    return found;
}
bool EditorCanvas::conflictPress(QMouseEvent* e) {
    if (tool_ != Tool::conflict || e->button() != Qt::LeftButton || !document_) return false;
    const auto p = world(e->pos(), false);
    lastPick_ = p;
    // A line wins over an area: it is the smaller target, and it always stands just outside one.
    if (const auto id = waitingLineAt(p); !id.empty()) {
        for (const auto& line : document_->network.rightOfWay.waitingLines) if (line.id == id) {
            auto polyline = controlPathPolyline(document_->network, line.point.path);
            if (polyline.size() < 2) break;
            lineDrag_ = LineDrag{id, std::move(polyline), line.point.station, line.point.station, false};
            dragPress_ = e->pos();
            return true;
        }
    }
    const auto areas = conflictsAt(p);
    if (areas.empty()) { reject(); return true; }
    if (std::find(areas.begin(), areas.end(), highlightedConflict_) != areas.end()) {
        if (conflictCycled) conflictCycled(highlightedConflict_);
    } else if (conflictPicked) conflictPicked(areas.front());
    return true;
}
void EditorCanvas::updateLineDrag(QPoint position) {
    if (!lineDrag_) return;
    if ((position - dragPress_).manhattanLength() < QApplication::startDragDistance() && !lineDrag_->moved) return;
    lineDrag_->moved = true;
    // Along its own path only, as a stop line drags: another lane is another line.
    const double length = polylineLength(lineDrag_->polyline);
    lineDrag_->station = std::clamp(stationOfClosestPoint(lineDrag_->polyline, world(position, false)), 0.0, length);
    redraw();
}
void EditorCanvas::finishLineDrag(QPoint position) {
    if (!lineDrag_) return;
    updateLineDrag(position); // the release is authoritative: the last move may be coalesced
    const auto drag = *lineDrag_; lineDrag_.reset();
    redraw();
    if (drag.moved && std::abs(drag.station - drag.original) > 1e-9 && waitingLineMoved) waitingLineMoved(drag.id, drag.station);
}
void EditorCanvas::drawConflicts() {
    if (!document_) return;
    const auto& n = document_->network;
    for (const auto& area : n.rightOfWay.conflictAreas) {
        const bool lit = area.id == highlightedConflict_;
        for (const auto* side : {&area.first, &area.second}) {
            const int level = levelOf(n, side->path);
            if (!levelVisible(level)) continue;
            const auto outline = conflictSideOutline(n, *side);
            if (outline.size() < 3) continue;
            const bool undetermined = area.priority == ConflictPriority::undetermined;
            const bool yields = (side == &area.first) == (area.priority == ConflictPriority::firstYields);
            const QColor tint = undetermined ? QColor(234, 179, 8, 150) : yields ? QColor(220, 38, 38, 150) : QColor(22, 163, 74, 150);
            // The two sides of a crossing cover the same square. The one that gives way (the second
            // while undetermined) is hatched and drawn above, so the other still shows through.
            const bool hatched = undetermined ? side == &area.second : yields;
            QPen pen(lit ? QColor("#ffb454") : hatched ? tint.darker() : tint.darker(130), lit ? 3 : hatched ? 2 : 1);
            pen.setCosmetic(true);
            QColor solid = tint; if (hatched) solid.setAlpha(230);
            auto* item = scene_.addPath(outlinePath(outline), pen, QBrush(solid, hatched ? Qt::BDiagPattern : Qt::SolidPattern));
            item->setZValue(level * 100. + (hatched ? 6.5 : 6)); item->setToolTip(QString::fromStdString(area.id));
            item->setData(0, QStringLiteral("conflict-area")); item->setData(1, QString::fromStdString(area.id));
        }
    }
    for (const auto& line : n.rightOfWay.waitingLines) {
        const int level = levelOf(n, line.point.path);
        if (!levelVisible(level)) continue;
        auto point = line.point;
        const bool dragged = lineDrag_ && lineDrag_->id == line.id && lineDrag_->moved;
        if (dragged) point.station = lineDrag_->station;
        const auto bar = waitingLineBar(n, point);
        if (!bar) continue;
        // M3.2.5b: the line's control shows in the bar -- dashed for none, solid amber for Yield,
        // solid red and heavier for Stop. No text: the canvas has no locale.
        const auto control = std::find_if(n.rightOfWay.stopControls.begin(), n.rightOfWay.stopControls.end(),
                                          [&](const auto& c) { return c.waitingLineId == line.id; });
        const bool controlled = control != n.rightOfWay.stopControls.end();
        const bool stop = controlled && control->mode == StopMode::stop;
        QPen pen(dragged ? QColor("#ffb454") : stop ? QColor("#dc2626") : QColor("#f59e0b"), dragged || stop ? 3 : 2,
                 controlled ? Qt::SolidLine : Qt::DashLine);
        pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
        auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
        item->setZValue(level * 100. + 7);
        item->setData(0, QStringLiteral("waiting-line")); item->setData(1, QString::fromStdString(line.id));
        item->setData(2, QString::fromLatin1(!controlled ? "" : stop ? "stop" : "yield"));
    }
}
}
