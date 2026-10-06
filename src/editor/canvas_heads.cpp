#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QApplication>
#include <QGraphicsLineItem>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

namespace trafficsim {
// A Signal head IS its stop line: the runtime holds a vehicle at the head's station (the
// clamp in core/simulation.cpp), so the canvas draws the line there, across the lane it holds,
// and a click puts it exactly where the pointer is -- on a Link lane or a Connector path.
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
        return HeadGeometry{path.geometry, width, c.level,path.equation};
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
    if (e->button() != Qt::RightButton || !(e->modifiers() & Qt::ControlModifier)) return false; // D84
    const auto placed = headAt(world(e->pos(), false));
    hoverHead_.reset();
    if (!placed) { clearSelection(false); return true; }
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
        headDrag_ = HeadDrag{id, geometry->points, h.position, h.position, false,geometry->equation};
        dragPress_ = press;
        return true;
    }
    return false;
}
void EditorCanvas::updateHeadDrag(QPoint position) {
    if (!headDrag_) return;
    if ((position - dragPress_).manhattanLength() < QApplication::startDragDistance() && !headDrag_->moved) return;
    headDrag_->moved = true; showMoveCursor();
    // Along its own lane only: moving a stop line to another lane is a different object, and
    // the dialog is where that is chosen.
    const auto& curve=headDrag_->equation;
    const double length=curve?curve->arcStations.back():polylineLength(headDrag_->geometry);
    const double station=curve?equationClosestStation(*curve,world(position,false)):stationOfClosestPoint(headDrag_->geometry,world(position,false));
    headDrag_->station=std::clamp(station,0.,length);
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
    if(!document_ || runFrame_.scenario)return;
    const double scale=std::abs(transform().m11());
    const auto draw=[&](const NetworkSignalHead& head,QColor colour,bool dashed) {
        const auto bar=signalCrossbar(document_->network,head);
        if(!bar || !levelVisible(bar->level))return;
        if(!dashed && isSelected(head.id)) {
            QPen halo(canvasStyle::selection(),editorDesign::crossbarPixels+4);halo.setCosmetic(true);halo.setCapStyle(Qt::FlatCap);
            scene_.addLine(bar->first.x,bar->first.y,bar->second.x,bar->second.y,halo)->setZValue(bar->level*100.+10);
        } else if(!dashed) {
            QPen outline(editorDesign::role(QPalette::Text),editorDesign::crossbarPixels+2);
            outline.setCosmetic(true);outline.setCapStyle(Qt::FlatCap);
            scene_.addLine(bar->first.x,bar->first.y,bar->second.x,bar->second.y,outline)->setZValue(bar->level*100.+10);
        }
        auto* line=new RoadCrossbarItem(*bar,colour,scale,dashed);scene_.addItem(line);
        line->setZValue(bar->level*100.+10.5);line->setData(0,QStringLiteral("stop-line"));
        line->setData(1,QString::fromStdString(head.id));
    };
    for(auto head:document_->network.signalHeads) {
        if(headDrag_ && headDrag_->id==head.id && headDrag_->moved)head.position=headDrag_->station;
        draw(head,head.id==hoverObject_?canvasStyle::hover():editorDesign::role(QPalette::Base),false);
    }
    if(hoverHead_ && tool_==Tool::head) {
        NetworkSignalHead preview;preview.lane=hoverHead_->slot.lane;
        preview.connectorId=hoverHead_->slot.connectorId;preview.position=hoverHead_->station;
        draw(preview,canvasStyle::hover(),true);
    }
}
std::optional<std::pair<Point,Point>> EditorCanvas::headBar(const NetworkSignalHead& head) const {
    if(!document_)return {};
    const auto bar=signalCrossbar(document_->network,head);
    if(!bar)return {};
    return std::pair{bar->first,bar->second};
}
QPainterPath EditorCanvas::headShape(const NetworkSignalHead& head) const {
    if(!document_)return {};
    const auto bar=signalCrossbar(document_->network,head);
    if(!bar)return {};
    return RoadCrossbarItem(*bar,canvasStyle::active(),std::abs(transform().m11())).shape();
}
}
