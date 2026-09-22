#include "canvas.hpp"
#include <QGraphicsPathItem>
#include <QPainter>
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
    for(const auto& l:document_->network.links)if(l.id==id)
        return polygon(laneBoundaryGeometry(l,0,document_->network.drivingSide),laneBoundaryGeometry(l,l.lanes.size(),document_->network.drivingSide));
    for(const auto& c:document_->network.connectors)if(c.id==id) {
        const auto& boundaries=cachedBoundaries(c);return polygon(boundaries.front(),boundaries.back());
    }
    for(const auto& h:document_->network.signalHeads)if(h.id==id)if(const auto at=headPosition(h)) {
        const double r=4/std::abs(transform().m11());QPainterPath shape;
        shape.addEllipse(QPointF(at->first.x,at->first.y),r,r);return shape;
    }
    return {};
}
void EditorCanvas::drawCopyPreview() {
    // The same outline serves the copy and the group move: both show where the selection, and
    // everything that rides with it, is about to land.
    if(!copyDragging_ && !groupDragging_)return;
    const auto offset=copyDragging_?copyOffset_:groupOffset_;
    QPen pen(QColor("#de8618"),2,Qt::DashLine);pen.setCosmetic(true);
    const auto draw=[&](const std::string& id) {
        auto shape=objectShape(id);shape.translate(offset.x,offset.y);
        auto* item=scene_.addPath(shape,pen,QBrush(QColor(222,134,24,60)));
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
