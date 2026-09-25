#include "canvas.hpp"
#include "../model/network/right_of_way.hpp"
#include <QGraphicsLineItem>
#include <QMouseEvent>
#include <QPen>
#include <algorithm>

// M3.2.6c: the Queue counter tool and the counters' marks. A counter line is drawn exactly where
// evaluation measures it -- a head's stop line, a waiting line's bar, or an explicit point's bar,
// each from the helper that also draws or resolves that object -- so what is shown is what counts.
namespace trafficsim {
namespace {
int pathLevel(const Network& n, const ControlPathRef& ref) {
    for (const auto& l : n.links) if (l.id == ref.linkId) return l.level;
    for (const auto& c : n.connectors) if (c.id == ref.connectorId) return c.level;
    return 0;
}
const char* kindOf(const MeasurementLine& l, const Network& n) {
    if (l.point) return "point";
    const bool head = std::any_of(n.signalHeads.begin(), n.signalHeads.end(), [&](const auto& h) { return h.id == l.referenceId; });
    return head ? "head" : "line";
}
}
std::optional<MeasurementLine> EditorCanvas::counterLineAt(Point p) const {
    if (!document_) return std::nullopt;
    const auto& n = document_->network;
    // A stop line first, then a waiting line: both are thin targets drawn on a lane, and naming
    // them keeps the counter's line on the object when that object is later moved.
    for (const auto& h : n.signalHeads) {
        const auto geometry = headGeometry(h);
        if (!geometry || !levelVisible(geometry->level)) continue;
        if (headShape(h).contains(QPointF(p.x, p.y))) return MeasurementLine{h.id, std::nullopt};
    }
    if (const auto line = waitingLineAt(p); !line.empty()) return MeasurementLine{line, std::nullopt};
    // Anywhere else on a Link lane is an explicit point. A Connector path is not offered: an
    // approach's queue is counted on the Link that carries it.
    const auto placed = headAt(p);
    if (!placed || !placed->slot.connectorId.empty()) return std::nullopt;
    const auto point = laneControlPoint(n, placed->slot.lane, placed->station);
    if (!point) return std::nullopt;
    return MeasurementLine{"", point};
}
bool EditorCanvas::counterPress(QMouseEvent* e) {
    if (tool_ != Tool::counter || e->button() != Qt::LeftButton || !document_) return false;
    const auto p = world(e->pos(), false);
    lastPick_ = p;
    auto line = counterLineAt(p);
    if (!line) { reject(); return true; }
    // A head or waiting line measured twice is still one line.
    if (!line->point && std::any_of(counterDraft_.begin(), counterDraft_.end(), [&](const auto& l) { return l.referenceId == line->referenceId; }))
        return true;
    counterDraft_.push_back(std::move(*line));
    redraw();
    return true;
}
void EditorCanvas::commitCounterDraft() {
    if (counterDraft_.empty()) return;
    auto lines = std::move(counterDraft_); counterDraft_.clear();
    redraw();
    if (counterDraftCommitted) counterDraftCommitted(std::move(lines));
}
void EditorCanvas::drawCounters() {
    if (!document_) return;
    const auto& n = document_->network;
    const auto mark = [&](const MeasurementLine& l, const std::string& counter, bool draft) {
        std::optional<std::pair<Point, Point>> bar;
        int level = 0;
        if (l.point) { bar = waitingLineBar(n, *l.point); level = pathLevel(n, l.point->path); }
        else {
            for (const auto& h : n.signalHeads) if (h.id == l.referenceId)
                if (const auto g = headGeometry(h)) { bar = headBar(h); level = g->level; }
            for (const auto& w : n.rightOfWay.waitingLines) if (w.id == l.referenceId) {
                bar = waitingLineBar(n, w.point); level = pathLevel(n, w.point.path);
            }
        }
        if (!bar || !levelVisible(level)) return; // a line that no longer resolves is not drawn
        // Violet and dotted, above stop lines and waiting lines, so a counter on a head still shows.
        QPen pen(QColor("#7c3aed"), draft ? 4 : 2, draft ? Qt::DashLine : Qt::DotLine);
        pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
        auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
        item->setZValue(level * 100. + 11.5);
        item->setData(0, QStringLiteral("queue-counter")); item->setData(1, QString::fromStdString(counter));
        item->setData(2, QString::fromLatin1(kindOf(l, n)));
    };
    for (const auto& c : n.queueCounters) for (const auto& l : c.lines) mark(l, c.id, false);
    if (tool_ == Tool::counter) for (const auto& l : counterDraft_) mark(l, {}, true);
}
}
