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
std::optional<LaneReference> EditorCanvas::hitLaneEnd(Point p, bool outgoing) const {
    std::optional<LaneReference> result;
    double best=12/std::abs(transform().m11());
    if (document_) for (const auto& link : document_->network.links) for (const auto& lane : link.lanes) {
        const auto geometry=laneGeometry(link,lane.id,document_->network.drivingSide);
        const auto end=outgoing?geometry.back():geometry.front();
        const double distance=std::hypot(end.x-p.x,end.y-p.y);
        if (distance<best) { best=distance; result=LaneReference{link.id,lane.id}; }
    }
    return result;
}
void EditorCanvas::pickConnector(Point p) {
    const auto lane=hitLaneEnd(p,!connectorFrom_);
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
        const bool chosen=isSelected(c.id);
        const auto& geometry=c.id==primary&&!preview_.empty()?preview_:c.geometry;
        const QColor colour=c.id==primary?QColor("#b33f8d"):chosen?QColor("#c877b0"):QColor("#8b4cac");
        QPen pen(colour,chosen?3:2); pen.setCosmetic(true);
        scene_.addPath(path(geometry),pen)->setZValue(4);
        const double length=polylineLength(geometry);
        if (length>0) {
            const auto mid=pointAlong(geometry,length/2), ahead=pointAlong(geometry,length/2+length/100);
            const double angle=std::atan2(ahead.y-mid.y,ahead.x-mid.x);
            QPolygonF arrow;
            for (double offset : {0.0,2.5,-2.5})
                arrow<<QPointF(mid.x+radius*1.5*std::cos(angle+offset),mid.y+radius*1.5*std::sin(angle+offset));
            scene_.addPolygon(arrow,QPen(Qt::NoPen),QBrush(pen.color()))->setZValue(5);
        }
        if (c.id==primary) for (std::size_t i=0; i<geometry.size(); ++i) {
            const auto p=geometry[i];
            if (i==0 || i+1==geometry.size()) {
                scene_.addRect(p.x-radius,p.y-radius,2*radius,2*radius,pen,QBrush("#334155"))->setZValue(6);
            } else {
                const auto color=static_cast<int>(i)==vertex_?QColor("#ffb454"):QColor("#ffffff");
                scene_.addEllipse(p.x-radius,p.y-radius,2*radius,2*radius,pen,QBrush(color))->setZValue(6);
            }
        }
    }
    if (tool_!=Tool::connect) return;
    for (const auto& link : document_->network.links) for (const auto& lane : link.lanes) {
        const auto geometry=laneGeometry(link,lane.id,document_->network.drivingSide);
        const auto p=connectorFrom_?geometry.front():geometry.back();
        const QColor color=connectorFrom_?QColor("#087d82"):QColor("#b9660b");
        QPen pen(color,2); pen.setCosmetic(true);
        scene_.addEllipse(p.x-radius,p.y-radius,2*radius,2*radius,pen,QBrush(Qt::white))->setZValue(7);
        if (connectorFrom_ && connectorFrom_->laneId==lane.id) {
            const auto from=geometry.back();
            scene_.addEllipse(from.x-radius,from.y-radius,2*radius,2*radius,pen,QBrush("#ffb454"))->setZValue(8);
        }
    }
    if (connectorFrom_ && connectorHover_) {
        try {
            QPen pen(QColor("#b33f8d"),2,Qt::DashLine); pen.setCosmetic(true);
            scene_.addPath(path(connectorCurve(document_->network,*connectorFrom_,*connectorHover_)),pen)->setZValue(7);
        } catch (const std::exception&) { /* Coincident endpoints have no default curve preview. */ }
    }
}
}
