#include "canvas.hpp"
#include <QGraphicsPathItem>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
QPainterPath path(const std::vector<Point>& points) {
    QPainterPath result;
    if (!points.empty()) {
        result.moveTo(points.front().x, points.front().y);
        for (std::size_t i=1; i<points.size(); ++i) result.lineTo(points[i].x, points[i].y);
    }
    return result;
}
}
std::optional<LaneReference> EditorCanvas::hitLanePosition(Point p, bool outgoing) const {
    auto result=nearestLane(p);if(!result)return {};
    for(const auto& l:document_->network.links)if(l.id==result->linkId) {
        const auto g=laneGeometry(l,result->laneId,document_->network.drivingSide);
        const double length=polylineLength(g),station=stationOfClosestPoint(g,p);
        const double tolerance=4/std::abs(transform().m11());
        double fraction=station/length;
        if(station<tolerance)fraction=0;
        else if(length-station<tolerance)fraction=1;
        if(fraction!=(outgoing?1.:0.))result->fraction=fraction;
    }
    return result;
}
std::optional<LaneReference> EditorCanvas::connectorEndpointTarget(Point p,bool leading) const {
    const auto* connector=selectedConnector();
    if(!connector)return {};
    auto ref=hitLanePosition(p,leading);
    if(!ref)return {};
    // The grip sits in the middle of the range, so the range is centred on the lane under the
    // cursor, not started there; the range then slides inside the link rather than overflowing.
    const int count=leading?connector->fromLaneCount:connector->toLaneCount;
    for(const auto& link:document_->network.links)if(link.id==ref->linkId) {
        const int lanes=static_cast<int>(link.lanes.size());
        const auto at=std::find_if(link.lanes.begin(),link.lanes.end(),[&](const auto& l){return l.id==ref->laneId;});
        if(at==link.lanes.end())return {};
        const int index=static_cast<int>(std::distance(link.lanes.begin(),at));
        ref->laneId=link.lanes[static_cast<std::size_t>(std::clamp(index-(count-1)/2,0,std::max(0,lanes-count)))].id;
    }
    return ref;
}
void EditorCanvas::pickConnector(Point p) {
    const auto lane=hitLanePosition(p,!connectorFrom_);
    if (!lane) return;
    if (!connectorFrom_) {
        connectorFrom_=lane;
        if (connectorSourcePicked) connectorSourcePicked(*lane);
        if (connectorDraftChanged) connectorDraftChanged();
    } else {
        const auto from=*connectorFrom_;
        cancel();
        if (createConnector) createConnector(from,*lane);
    }
    redraw();
}
void EditorCanvas::drawConnectors() {
    const double radius=4/std::abs(transform().m11());
    const auto primary=selected();
    for (const auto& c : document_->network.connectors) {
        if(!levelVisible(c.level))continue;
        const double z=c.level*100.;
        const bool chosen=isSelected(c.id);
        const QColor colour=c.id==primary?QColor("#b33f8d"):chosen?QColor("#c877b0"):QColor(QString::fromStdString(style(c.displayType).connectorColor));
        QPen pen(colour,chosen?3:2); pen.setCosmetic(true);
        auto preview=c;
        if(c.id==primary && !preview_.empty()){preview.geometry=preview_;preview.laneBlend.clear();}
        if(c.id==primary && rangeCorner_)resizeConnectorEdges(document_->network,preview,previewFromCount_,previewToCount_,rangeCorner_>4);
        // Preview the re-attachment the same way the command will perform it, so what the
        // pointer shows is what one release commits. An impossible drop shows the original.
        if(c.id==primary && endpointDraft_ && endpointDrag_) try {
            auto moved=preview;
            (*endpointDrag_?moved.from:moved.to)=*endpointDraft_;
            moved.fromLaneCount=std::max(1,std::min(moved.fromLaneCount,lanesFromReference(document_->network,moved.from)));
            moved.toLaneCount=std::max(1,std::min(moved.toLaneCount,lanesFromReference(document_->network,moved.to)));
            (void)connectorPaths(document_->network,moved);
            reanchorConnector(document_->network,moved);
            preview=std::move(moved);
        } catch(const std::exception&) { /* Keep drawing the connector that still exists. */ }
        const auto& geometry=preview.geometry;
        const auto boundaries=connectorBoundaries(document_->network,preview);
        auto surface=path(boundaries.front());
        for(auto it=boundaries.back().rbegin();it!=boundaries.back().rend();++it)surface.lineTo(it->x,it->y);
        surface.closeSubpath();scene_.addPath(surface,QPen(Qt::NoPen),QBrush(colour))->setZValue(z+4);
        for(std::size_t i=0;i<boundaries.size();++i) {
            QPen marking(QColor(QString::fromStdString(style(c.displayType).laneColor)),1,
                         (i==0 || i+1==boundaries.size())?Qt::SolidLine:Qt::DashLine);marking.setCosmetic(true);
            auto* item=scene_.addPath(path(boundaries[i]),marking);item->setZValue(z+4.5);
            item->setData(0,QStringLiteral("road-marking"));item->setData(1,QString::fromStdString(c.id));
        }
        const double length=polylineLength(geometry);
        if (length>0) {
            const auto mid=pointAlong(geometry,length/2), ahead=pointAlong(geometry,length/2+length/100);
            const double angle=std::atan2(ahead.y-mid.y,ahead.x-mid.x);
            QPolygonF arrow;
            for (double offset : {0.0,2.5,-2.5})
                arrow<<QPointF(mid.x+radius*1.5*std::cos(angle+offset),mid.y+radius*1.5*std::sin(angle+offset));
            scene_.addPolygon(arrow,QPen(Qt::NoPen),QBrush(Qt::white))->setZValue(z+5);
        }
        // Grips ride the middle of the connector's whole width, not the first lane's path.
        if (c.id==primary) {
            const auto handles=connectorCentreline(document_->network,preview);
            QPen outline(QColor("#334155"),1);outline.setCosmetic(true);
            for (std::size_t i=0; i<handles.size(); ++i) {
                const auto p=handles[i];
                if (i==0 || i+1==handles.size()) {
                    const bool held=endpointDrag_ && *endpointDrag_==(i==0);
                    auto* item=scene_.addRect(p.x-radius,p.y-radius,2*radius,2*radius,pen,
                                              QBrush(held?QColor("#ffb454"):QColor("#334155")));
                    item->setZValue(z+6);item->setData(0,QStringLiteral("connector-end"));
                    item->setData(1,QString::fromStdString(c.id));item->setData(2,i==0);
                } else {
                    const auto color=static_cast<int>(i)==vertex_?QColor("#ffb454"):QColor("#ffffff");
                    scene_.addEllipse(p.x-radius,p.y-radius,2*radius,2*radius,outline,QBrush(color))->setZValue(z+6);
                }
            }
        }
    }
    if (tool_!=Tool::connect) return;
    if(connectorFrom_) {
        const auto p=laneAttachment(document_->network,*connectorFrom_,true);
        scene_.addEllipse(p.x-radius,p.y-radius,2*radius,2*radius,QPen("#087d82"),QBrush("#ffb454"))->setZValue(200008);
    }
    if (connectorFrom_ && connectorHover_) {
        try {
            QPen pen(QColor("#b33f8d"),2,Qt::DashLine); pen.setCosmetic(true);
            scene_.addPath(path(connectorCurve(document_->network,*connectorFrom_,*connectorHover_)),pen)->setZValue(200007);
        } catch (const std::exception&) { /* Coincident endpoints have no default curve preview. */ }
    }
}
}
