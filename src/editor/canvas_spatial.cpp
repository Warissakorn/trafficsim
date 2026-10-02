#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsPathItem>
#include <QPainter>
#include <QPainterPathStroker>
#include <cmath>
namespace trafficsim {
std::optional<std::pair<Point,int>> EditorCanvas::headPosition(const NetworkSignalHead& head) const {
    if(head.connectorId.empty()) {
        for(const auto& l:document_->network.links)if(l.id==head.lane.linkId)
            return std::pair{pointAlong(laneGeometry(l,head.lane.laneId,document_->network.drivingSide),head.position),l.level};
    } else for(const auto& c:document_->network.connectors)for(const auto& path:cachedPaths(c))
        if(path.id==head.connectorId)return std::pair{pointAlong(path.geometry,head.position),c.level};
    return {};
}
QPainterPath EditorCanvas::objectShape(const std::string& id) const {
    const auto polygon=[](const std::vector<Point>& a,const std::vector<Point>& b) {
        QPainterPath shape;shape.moveTo(a.front().x,a.front().y);
        for(const auto& p:a)shape.lineTo(p.x,p.y);
        for(auto it=b.rbegin();it!=b.rend();++it)shape.lineTo(it->x,it->y);
        shape.closeSubpath();return shape;
    };
    if(!document_)return {};
    // Wireframe: what is drawn is what is hit, band-selected and framed -- the centre line.
    if(wireframe_) {
        for(const auto& l:document_->network.links)if(l.id==id)
            return centreStroke(linkCentreline(l,document_->network.drivingSide),1);
        for(const auto& c:document_->network.connectors)if(c.id==id) {
            try { return centreStroke(connectorCentreline(document_->network,c),1); }
            catch(const std::exception&) { return {}; }
        }
    }
    for(const auto& l:document_->network.links)if(l.id==id)
        return polygon(laneBoundaryGeometry(l,0,document_->network.drivingSide),laneBoundaryGeometry(l,l.lanes.size(),document_->network.drivingSide));
    for(const auto& c:document_->network.connectors)if(c.id==id) {
        const auto& drawing=cachedSurface(c);const auto& ring=drawing.outline;
        QPainterPath shape;
        if(ring.empty()) {
            // A singular mouth has no fabricated square cap, but its open rails remain
            // selectable through hitObjects' existing stroke tolerance.
            for(const auto& marking:drawing.markings) {
                if(marking.geometry.empty())continue;
                shape.moveTo(marking.geometry.front().x,marking.geometry.front().y);
                for(std::size_t i=1;i<marking.geometry.size();++i)shape.lineTo(marking.geometry[i].x,marking.geometry[i].y);
            }
            // QPainterPath::contains implicitly closes an open subpath. Return only a
            // thin stroked region, otherwise selection could hit an invisible polygon.
            QPainterPathStroker stroke;stroke.setWidth(1/std::abs(transform().m11()));
            return stroke.createStroke(shape);
        }
        shape.moveTo(ring.front().x,ring.front().y);
        for(std::size_t i=1;i<ring.size();++i)shape.lineTo(ring[i].x,ring[i].y);
        shape.closeSubpath();shape.setFillRule(Qt::WindingFill);return shape;
    }
    for(const auto& h:document_->network.signalHeads)if(h.id==id)return headShape(h);
    return {};
}
void EditorCanvas::drawCopyPreview() {
    // The same outline serves the copy and the group move: both show where the selection, and
    // everything that rides with it, is about to land.
    if(!copyDragging_ && !groupDragging_)return;
    const auto offset=copyDragging_?copyOffset_:groupOffset_;
    QPen pen(canvasStyle::active(),2,Qt::DashLine);pen.setCosmetic(true);
    QColor wash=canvasStyle::active();wash.setAlpha(40);
    const auto draw=[&](const std::string& id) {
        auto shape=objectShape(id);shape.translate(offset.x,offset.y);
        auto* item=scene_.addPath(shape,pen,QBrush(wash));
        item->setZValue(200008);item->setData(0,QStringLiteral("copy-preview"));
    };
    for(const auto& id:selection_)draw(id);
    for(const auto& c:document_->network.connectors)
        if(!isSelected(c.id) && isSelected(c.from.linkId) && isSelected(c.to.linkId))draw(c.id);
    for(const auto& h:document_->network.signalHeads) {
        bool copied=isSelected(h.lane.linkId);
        for(const auto& c:document_->network.connectors)for(const auto& path:cachedPaths(c))
            if(path.id==h.connectorId)copied=isSelected(c.id) || (isSelected(c.from.linkId) && isSelected(c.to.linkId));
        if(copied && !isSelected(h.id))draw(h.id);
    }
}
}
