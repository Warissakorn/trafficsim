#include "canvas.hpp"
#include "../model/network/right_of_way.hpp"
#include <QGraphicsPathItem>
#include <QPen>

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
}
void EditorCanvas::setHighlightedConflict(std::string id) {
    if (highlightedConflict_ == id) return;
    highlightedConflict_ = std::move(id); redraw();
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
            const bool yields = (side == &area.first) == (area.priority == ConflictPriority::firstYields);
            const QColor tint = area.priority == ConflictPriority::undetermined ? QColor(234, 179, 8, 150)
                              : yields ? QColor(220, 38, 38, 150) : QColor(22, 163, 74, 150);
            QPen pen(lit ? QColor("#ffb454") : tint.darker(), lit ? 3 : 1); pen.setCosmetic(true);
            auto* item = scene_.addPath(outlinePath(outline), pen, QBrush(tint));
            item->setZValue(level * 100. + 6); item->setToolTip(QString::fromStdString(area.id));
            item->setData(0, QStringLiteral("conflict-area")); item->setData(1, QString::fromStdString(area.id));
        }
    }
    for (const auto& line : n.rightOfWay.waitingLines) {
        const int level = levelOf(n, line.point.path);
        if (!levelVisible(level)) continue;
        const auto bar = waitingLineBar(n, line.point);
        if (!bar) continue;
        QPen pen(QColor("#f59e0b"), 2, Qt::DashLine); pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
        auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
        item->setZValue(level * 100. + 7);
        item->setData(0, QStringLiteral("waiting-line")); item->setData(1, QString::fromStdString(line.id));
    }
}
}
