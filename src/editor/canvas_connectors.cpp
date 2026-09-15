#include "canvas.hpp"
#include <QGraphicsPathItem>
#include <QPainter>
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
        const auto& geometry=c.id==primary&&!preview_.empty()?preview_:c.geometry;
        const QColor colour=c.id==primary?QColor("#b33f8d"):chosen?QColor("#c877b0"):QColor(QString::fromStdString(style(c.displayType).connectorColor));
        QPen pen(colour,chosen?3:2); pen.setCosmetic(true);
        auto preview=c;preview.geometry=geometry;
        if(c.id==primary && rangeCorner_){preview.fromLaneCount=previewFromCount_;preview.toLaneCount=previewToCount_;}
        for(const auto& lanePath:connectorPaths(document_->network,preview))scene_.addPath(path(lanePath.geometry),pen)->setZValue(z+4);
        const double length=polylineLength(geometry);
        if (length>0) {
            const auto mid=pointAlong(geometry,length/2), ahead=pointAlong(geometry,length/2+length/100);
            const double angle=std::atan2(ahead.y-mid.y,ahead.x-mid.x);
            QPolygonF arrow;
            for (double offset : {0.0,2.5,-2.5})
                arrow<<QPointF(mid.x+radius*1.5*std::cos(angle+offset),mid.y+radius*1.5*std::sin(angle+offset));
            scene_.addPolygon(arrow,QPen(Qt::NoPen),QBrush(pen.color()))->setZValue(z+5);
        }
        if (c.id==primary) for (std::size_t i=0; i<geometry.size(); ++i) {
            const auto p=geometry[i];
            if (i==0 || i+1==geometry.size()) {
                scene_.addRect(p.x-radius,p.y-radius,2*radius,2*radius,pen,QBrush("#334155"))->setZValue(z+6);
            } else {
                const auto color=static_cast<int>(i)==vertex_?QColor("#ffb454"):QColor("#ffffff");
                scene_.addEllipse(p.x-radius,p.y-radius,2*radius,2*radius,pen,QBrush(color))->setZValue(z+6);
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
