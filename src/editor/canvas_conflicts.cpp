#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../model/network/right_of_way.hpp"
#include "../model/network/conflict_display.hpp"
#include "../model/network/conflict_surface.hpp"
#include <QApplication>
#include <QGraphicsPathItem>
#include <QMouseEvent>
#include <QPen>
#include <algorithm>
#include <cmath>

// M3.2.4: authored conflict areas and waiting lines on the canvas, drawn from the same lane
// painted strips. Runtime coverage uses those same rails; grouping shares controls, not outlines.
namespace trafficsim {
namespace {
QPainterPath outlinePath(const std::vector<Point>& points) {
    QPainterPath shape(QPointF(points.front().x, points.front().y));
    for (std::size_t i = 1; i < points.size(); ++i) shape.lineTo(points[i].x, points[i].y);
    shape.closeSubpath();
    return shape;
}
// Offset laterally inside the painted rails, retaining the measured entry/exit cuts.
// Cap the inset at 20% of local width so narrow/tapered lanes and short mouths remain visible.
QPainterPath bandPath(const Network& n,const std::vector<ConflictSide>& spans) {
    QPainterPath shape;
    for(const auto& side:spans) {
        const auto surface=conflictSurface(n,side.path);if(!surface)continue;
        auto left=surface->left,right=surface->right;
        if(left.size()!=right.size() || left.size()!=surface->base.size())continue;
        for(std::size_t i=0;i<left.size();++i) {
            const auto a=left[i],b=right[i];const double width=std::hypot(b.x-a.x,b.y-a.y);
            if(width<=1e-12)continue;
            const double fraction=std::min(0.3/width,0.2);
            left[i]={a.x+fraction*(b.x-a.x),a.y+fraction*(b.y-a.y)};
            right[i]={b.x+fraction*(a.x-b.x),b.y+fraction*(a.y-b.y)};
        }
        auto outline=polylineSpan(left,matchedStation(surface->base,left,side.entryStation),matchedStation(surface->base,left,side.exitStation));
        const auto other=polylineSpan(right,matchedStation(surface->base,right,side.entryStation),matchedStation(surface->base,right,side.exitStation));
        outline.insert(outline.end(),other.rbegin(),other.rend());
        if(outline.size()>=3)shape=shape.united(outlinePath(outline));
    }
    return shape;
}
QPainterPath displayPath(const Network& n,const ConflictAreaGeometry& geometry) {
    return bandPath(n,geometry.first).united(bandPath(n,geometry.second));
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
    for (const auto& area : n.rightOfWay.conflictAreas) {
        if(!levelVisible(levelOf(n,area.first.path)) || !levelVisible(levelOf(n,area.second.path)))continue;
        const auto shape=displayPath(n,conflictAreaGeometry(n,area.kind,area.first,area.second));
        if(shape.contains(QPointF(p.x,p.y))) {
            std::string key=area.id;
            for(const auto& g:conflictGroups_)if(conflictGroupContains(g,area.id)){key=g.key;break;}
            found.push_back(key);
        }
    }
    std::sort(found.begin(), found.end());
    found.erase(std::unique(found.begin(),found.end()),found.end());
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
    if (tool_ != Tool::conflict || !document_) return false;
    const bool change = e->button() == Qt::RightButton && (e->modifiers() & Qt::ControlModifier);
    if (!change && e->button() != Qt::LeftButton) return false;
    const auto p = world(e->pos(), false);
    lastPick_ = p;
    // D84: a left click selects an area and drags a waiting line; Ctrl+right-click authors a
    // passive area or sets the next priority.
    // A line wins over an area: it is the smaller target, and it always stands just outside one.
    if (const auto id = waitingLineAt(p); !change && !id.empty()) {
        for (const auto& line : document_->network.rightOfWay.waitingLines) if (line.id == id) {
            auto polyline = controlPathPolyline(document_->network, line.point.path);
            if (polyline.size() < 2) break;
            lineDrag_ = LineDrag{id, std::move(polyline), line.point.station, line.point.station, false};
            dragPress_ = e->pos();
            return true;
        }
    }
    const auto areas = conflictsAt(p);
    if (areas.empty()) {
        // No authored area here: an automatic one is authored by the click (D68), as Vissim's
        // first click sets a passive area's priority.
        if(const auto key=automaticAt(p);!key.empty()) {
            if(conflictPicked)conflictPicked(key);
            const auto group=std::find_if(conflictGroups_.begin(),conflictGroups_.end(),[&](const auto& g){return g.key==key;});
            const bool branching=group!=conflictGroups_.end() && group->geometryKind==ConflictGeometryKind::branching;
            if(change && !branching && conflictAuthored)conflictAuthored(key);
            return true;
        }
        clearSelection(false); return true;
    }
    const bool current = std::find(areas.begin(), areas.end(), highlightedConflict_) != areas.end();
    const auto area = current ? highlightedConflict_ : areas.front();
    if (!current && conflictPicked) conflictPicked(area);
    if (change && conflictCycled) conflictCycled(area);
    return true;
}
void EditorCanvas::updateLineDrag(QPoint position) {
    if (!lineDrag_) return;
    if ((position - dragPress_).manhattanLength() < QApplication::startDragDistance() && !lineDrag_->moved) return;
    lineDrag_->moved = true; showMoveCursor();
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
void EditorCanvas::setAutomaticConflicts(std::vector<AutomaticConflict> automatic) {
    if (automatic_ == automatic) return;
    automatic_ = std::move(automatic);
    if (tool_ == Tool::conflict) redraw();
}
std::string EditorCanvas::automaticAt(Point p) const {
    if (!document_ || tool_ != Tool::conflict) return {};
    const auto& n = document_->network;
    for(const auto& a:automatic_) {
        if(!levelVisible(levelOf(n,a.first.path)) || !levelVisible(levelOf(n,a.second.path)))continue;
        if(displayPath(n,conflictAreaGeometry(n,a.kind,a.first,a.second)).contains(QPointF(p.x,p.y))) {
            for(const auto& g:conflictGroups_)if(conflictGroupContains(g,a.key))return g.key;
            return a.key;
        }
    }
    return {};
}
void EditorCanvas::drawConflicts() {
    if (!document_) return;
    const auto& n = document_->network;
    conflictGroups_=conflictGroups(n,automatic_);
    // Separate bands follow each participant's driving direction; grouping uses measured polygons.
    const auto draw=[&](const std::string& id,ConflictKind kind,ConflictPriority priority,const ConflictSide& first,const ConflictSide& second,bool automatic,ConflictGeometryKind geometryKind) {
        std::string key=id;
        for(const auto& g:conflictGroups_)if(conflictGroupContains(g,id)){key=g.key;break;}
        if(!levelVisible(levelOf(n,first.path)) || !levelVisible(levelOf(n,second.path)))return;
        const auto geometry=conflictAreaGeometry(n,kind,first,second);
        const bool branching=geometryKind==ConflictGeometryKind::branching;
        for(const auto* side:{&first,&second}) {
            const auto shape=bandPath(n,side==&first?geometry.first:geometry.second);if(shape.isEmpty())continue;
            const int level=levelOf(n,side->path);
            const bool undecided=priority==ConflictPriority::undetermined;
            const bool yields=(side==&first)==(priority==ConflictPriority::firstYields);
            const bool lit=key==highlightedConflict_,hover=key==hoverConflict_ || key==hoverAutomatic_;
            QColor tint=branching?canvasStyle::error():undecided?(automatic?canvasStyle::ink():canvasStyle::warning()):yields?canvasStyle::error():canvasStyle::ok();
            tint.setAlpha(automatic?(undecided?90:80):150);
            const bool hatched=!branching && !undecided && yields;
            QPen pen(lit?canvasStyle::selection():hover?canvasStyle::hover():tint.darker(150),lit?2.5:hover?2:1,
                     automatic?Qt::DashLine:Qt::SolidLine);pen.setCosmetic(true);
            auto* item=scene_.addPath(shape,pen,QBrush(tint,hatched?Qt::BDiagPattern:Qt::SolidPattern));
            item->setZValue(level*100.+(automatic?(hatched?5.6:5.5):hatched?6.5:6));item->setToolTip(QString::fromStdString(key));
            item->setData(0,QStringLiteral("conflict-area"));
            if(automatic && key.rfind("auto/",0)==0)item->setData(0,QStringLiteral("auto-conflict"));
            item->setData(1,QString::fromStdString(key));
            if(automatic)item->setData(2,QString::fromLatin1(branching?"branching":undecided?"passive":"merge"));
            item->setData(4,QString::fromLatin1(branching?"branching":geometryKind==ConflictGeometryKind::merge?"merge":"crossing"));
            item->setData(3,QString::fromStdString(id)); // lane-pair identity, independent of group selection
        }
    };
    for(const auto& a:n.rightOfWay.conflictAreas)draw(a.id,a.kind,a.priority,a.first,a.second,false,
        a.kind==ConflictKind::crossing?ConflictGeometryKind::crossing:ConflictGeometryKind::merge);
    if(tool_==Tool::conflict)for(const auto& a:automatic_)draw(a.key,a.kind,a.priority,a.first,a.second,true,a.geometryKind);
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
        QPen pen(dragged ? canvasStyle::active() : line.id==hoverWaitingLine_ ? canvasStyle::hover() : stop ? canvasStyle::error() : canvasStyle::warning(), dragged || stop ? 3 : 2,
                 controlled ? Qt::SolidLine : Qt::DashLine);
        pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
        auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
        item->setZValue(level * 100. + 7);
        item->setData(0, QStringLiteral("waiting-line")); item->setData(1, QString::fromStdString(line.id));
        item->setData(2, QString::fromLatin1(!controlled ? "" : stop ? "stop" : "yield"));
    }
}
}
