#include "canvas.hpp"
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
QPainterPath polylinePath(const std::vector<Point>& points) {
    QPainterPath path;
    if(points.empty())return path;
    path.moveTo(points.front().x,points.front().y);
    for(std::size_t i=1;i<points.size();++i)path.lineTo(points[i].x,points[i].y);
    return path;
}
double distanceTo(const std::vector<Point>& geometry,Point p) {
    if(geometry.size()<2)return 1e300;
    const auto at=pointAlong(geometry,stationOfClosestPoint(geometry,p));
    return std::hypot(at.x-p.x,at.y-p.y);
}
}
// What a click means to the route and input tools: the lane or Connector path under the
// pointer, by distance to the thing a vehicle would actually drive along. hitObjects answers
// with Link and Connector ids, and a route names neither.
std::string EditorCanvas::segmentAt(Point p) const {
    if(!document_)return {};
    const double tolerance=12/std::abs(transform().m11());
    std::string found;double best=tolerance;
    for(const auto& link:document_->network.links) {
        if(!levelVisible(link.level))continue;
        for(const auto& lane:link.lanes) {
            const double distance=distanceTo(laneGeometry(link,lane.id,document_->network.drivingSide),p);
            // Half the lane counts as "on it": an author clicks the road, not its middle.
            const double reach=std::max(tolerance,lane.width/2);
            if(distance<=reach && distance<best){best=distance;found=lane.id;}
        }
    }
    for(const auto& connector:document_->network.connectors) {
        if(!levelVisible(connector.level))continue;
        std::vector<ConnectorPath> paths;
        try {paths=connectorPaths(document_->network,connector);} catch(const std::exception&){continue;}
        for(const auto& path:paths) {
            const double distance=distanceTo(path.geometry,p);
            if(distance<=tolerance && distance<best){best=distance;found=path.id;}
        }
    }
    return found;
}
std::vector<std::string> EditorCanvas::routeDraftWith(const std::string& target) const {
    if(!document_ || target.empty())return {};
    if(routeDraft_.empty())return {target};
    return routeChainTo(document_->network,routeDraft_,target);
}
void EditorCanvas::startRouteDraft(const std::string& segmentId) {
    if(!document_ || segmentId.empty())return;
    if(routeContinuations(document_->network,{}).empty())return;
    routeDraft_={segmentId};hoverSegment_.clear();animate();redraw();
}
std::pair<std::string,std::string> EditorCanvas::demandObjectAt(QPoint viewportPosition) const {
    for(const auto* item:items(viewportPosition)) {
        const auto kind=item->data(0).toString();
        if(kind=="input-marker")return {"input",item->data(1).toString().toStdString()};
        if(kind=="route-overlay")return {"route",item->data(1).toString().toStdString()};
    }
    return {};
}
bool EditorCanvas::demandPress(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    // Left-click, or Vissim's Ctrl+right-click on the link the decision sits on.
    const bool ctrlRight=e->button()==Qt::RightButton && (e->modifiers()&Qt::ControlModifier);
    if(e->button()!=Qt::LeftButton && !ctrlRight)return false;
    const auto p=world(e->pos(),false);
    const auto target=segmentAt(p);
    if(tool_==Tool::input) {
        // A vehicle input is placed on a lane, the way Vissim places one on a Link.
        if(const auto lane=laneOf(target)) {if(inputPlaced)inputPlaced(*lane);}
        else reject();
        return true;
    }
    const auto chain=routeDraftWith(target);
    if(chain.empty()) {reject();return true;}
    // One click can append a whole chain: that is what makes "click the destination" work.
    routeDraft_.insert(routeDraft_.end(),chain.begin(),chain.end());
    animate();redraw();
    return true;
}
std::optional<LaneReference> EditorCanvas::laneOf(const std::string& segmentId) const {
    if(!document_ || segmentId.empty())return {};
    for(const auto& link:document_->network.links)for(const auto& lane:link.lanes)
        if(lane.id==segmentId)return LaneReference{link.id,lane.id};
    return {};
}
bool EditorCanvas::demandHover(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    const auto p=world(e->pos(),false);
    const auto hovered=segmentAt(p);
    const bool reachable=tool_==Tool::input?laneOf(hovered).has_value():!routeDraftWith(hovered).empty();
    if(hovered!=hoverSegment_ || reachable!=hoverReachable_ || !routeDraft_.empty()) {
        hoverSegment_=hovered;hoverReachable_=reachable;hoverPoint_=p;redraw();
    }
    return true;
}
void EditorCanvas::commitRouteDraft() {
    if(routeDraft_.empty())return;
    const auto segments=routeDraft_;
    pulseGeometry_=document_?routeGeometry(document_->network,segments):std::vector<Point>{};
    routeDraft_.clear();hoverSegment_.clear();
    commitPulse_=8;animate();
    if(routeDraftCommitted)routeDraftCommitted(segments);
    redraw();
}
void EditorCanvas::dropLastRouteSegment() {
    if(routeDraft_.empty())return;
    routeDraft_.pop_back();redraw();
}
void EditorCanvas::clearRouteDraft() {
    if(routeDraft_.empty() && hoverSegment_.empty())return;
    routeDraft_.clear();hoverSegment_.clear();
}
void EditorCanvas::reject() {
    rejectPulse_=6;animate();
    if(creationRejected)creationRejected();
    redraw();
}
void EditorCanvas::setHighlightedRoute(std::string id) {
    if(highlightedRoute_==id)return;
    highlightedRoute_=std::move(id);animate();redraw();
}
// One timer for the whole canvas, and only while there is something moving to draw: an editor
// sitting still must not repaint itself forever. Nothing here reaches the document or the
// engine -- the phase is paint state, so no measured number can depend on it.
void EditorCanvas::animate() {
    if(!animation_) {
        animation_=new QTimer(this);animation_->setInterval(50);
        connect(animation_,&QTimer::timeout,this,[this]{
            ++animationPhase_;
            if(commitPulse_>0)--commitPulse_;
            if(rejectPulse_>0)--rejectPulse_;
            if(!animating())animation_->stop();
            redraw();
        });
    }
    if(animating() && !animation_->isActive())animation_->start();
}
bool EditorCanvas::animating() const {
    return !routeDraft_.empty() || !highlightedRoute_.empty() || commitPulse_>0 || rejectPulse_>0;
}
void EditorCanvas::setAnimationPhase(int phase) {animationPhase_=phase;redraw();}
namespace {
// A dash pattern that crawls along the route, so a drawn route reads as a direction of travel.
QPen marchingPen(QColor colour,int phase,double width) {
    QPen pen(colour,width);pen.setCosmetic(true);pen.setDashPattern({6,4});
    pen.setDashOffset(-static_cast<double>(phase%10));
    return pen;
}
}
void EditorCanvas::drawDemandOverlay() {
    if(!document_)return;
    const double scale=std::abs(transform().m11());
    // Hover halo: nearestLane used to pick in silence, so the author learned where a click
    // landed only after the dialog opened.
    if((tool_==Tool::route || tool_==Tool::input) && !hoverSegment_.empty()) {
        const auto geometry=routeGeometry(document_->network,{hoverSegment_});
        if(geometry.size()>1) {
            const QColor colour=hoverReachable_?QColor(22,123,152,90):QColor(239,68,68,90);
            QPen halo(colour,14);halo.setCosmetic(true);halo.setCapStyle(Qt::RoundCap);
            auto* item=scene_.addPath(polylinePath(geometry),halo);
            item->setZValue(200003);item->setData(0,QStringLiteral("demand-hover"));
            item->setData(1,QString::fromStdString(hoverSegment_));
            item->setData(2,hoverReachable_);
        }
    }
    // Every vehicle input draws where its traffic enters, with the volume beside it.
    if(document_->definition)for(const auto& input:document_->definition->inputs) {
        std::vector<std::string> segments;
        for(const auto& route:document_->definition->routes)if(route.id==input.routeId)segments=route.segmentIds;
        if(segments.empty())continue;
        const auto geometry=routeGeometry(document_->network,{segments.front()});
        if(geometry.size()<2)continue;
        const bool lit=input.routeId==highlightedRoute_;
        const double pulse=lit?1+0.15*std::sin(animationPhase_*0.3):1;
        const double r=(7/scale)*pulse;
        const auto at=pointAlong(geometry,0),ahead=pointAlong(geometry,std::min(0.5,polylineLength(geometry)));
        const double angle=std::atan2(ahead.y-at.y,ahead.x-at.x);
        QPolygonF chevron;
        for(double offset:{0.0,2.4,-2.4})chevron<<QPointF(at.x+r*std::cos(angle+offset),at.y+r*std::sin(angle+offset));
        QPen pen(QColor("#0f766e"),1);pen.setCosmetic(true);
        auto* marker=scene_.addPolygon(chevron,pen,QBrush(lit?QColor("#14b8a6"):QColor("#99f6e4")));
        marker->setZValue(200005);marker->setData(0,QStringLiteral("input-marker"));
        marker->setData(1,QString::fromStdString(input.id));
        auto* label=scene_.addSimpleText(QString::number(input.vehiclesPerHour,'f',0));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setBrush(QColor("#0f766e"));label->setPos(at.x+r,at.y+r);label->setZValue(200006);
        label->setData(0,QStringLiteral("input-volume"));
    }
    // A committed route draws only while it is selected, so the canvas does not silt up.
    if(document_->definition && !highlightedRoute_.empty())
        for(const auto& route:document_->definition->routes)if(route.id==highlightedRoute_) {
            const auto geometry=routeGeometry(document_->network,route.segmentIds);
            if(geometry.size()<2)continue;
            auto* item=scene_.addPath(polylinePath(geometry),marchingPen(QColor("#7c3aed"),animationPhase_,3));
            item->setZValue(200004);item->setData(0,QStringLiteral("route-overlay"));
            item->setData(1,QString::fromStdString(route.id));
            drawRouteArrows(geometry,QColor("#7c3aed"));
        }
    // A committed route flashes once, so the author sees which drawing became the new row.
    if(commitPulse_>0 && pulseGeometry_.size()>1) {
        QPen pen(QColor(124,58,237,static_cast<int>(20*commitPulse_)),10);
        pen.setCosmetic(true);pen.setCapStyle(Qt::RoundCap);
        auto* item=scene_.addPath(polylinePath(pulseGeometry_),pen);
        item->setZValue(200002);item->setData(0,QStringLiteral("route-committed-pulse"));
    }
    // A refused click flashes where it was refused, rather than only writing to the error line.
    if(rejectPulse_>0) {
        const double r=(10+2.0*(6-rejectPulse_))/scale;
        QPen pen(QColor(239,68,68,static_cast<int>(30*rejectPulse_)),2);pen.setCosmetic(true);
        auto* item=scene_.addEllipse(hoverPoint_.x-r,hoverPoint_.y-r,2*r,2*r,pen);
        item->setZValue(200009);item->setData(0,QStringLiteral("demand-reject-pulse"));
    }
    if(routeDraft_.empty())return;
    const auto drawn=routeGeometry(document_->network,routeDraft_);
    if(drawn.size()>1) {
        auto* item=scene_.addPath(polylinePath(drawn),marchingPen(QColor("#de8618"),animationPhase_,3));
        item->setZValue(200007);item->setData(0,QStringLiteral("route-draft"));
        drawRouteArrows(drawn,QColor("#de8618"));
    }
    // The rubber band says, before the click, whether the click will be taken.
    if(!drawn.empty()) {
        const QColor colour=hoverSegment_.empty()||hoverReachable_?QColor("#de8618"):QColor("#ef4444");
        QPen pen(colour,1,Qt::DashLine);pen.setCosmetic(true);
        auto* band=scene_.addLine(drawn.back().x,drawn.back().y,hoverPoint_.x,hoverPoint_.y,pen);
        band->setZValue(200007);band->setData(0,QStringLiteral("route-band"));
        band->setData(2,hoverSegment_.empty()||hoverReachable_);
    }
}
void EditorCanvas::drawRouteArrows(const std::vector<Point>& geometry,QColor colour) {
    const double length=polylineLength(geometry);
    if(!(length>0))return;
    const double r=6/std::abs(transform().m11());
    // Spaced along the route rather than one per segment: a long lane needs more than one
    // arrow to read as a direction, and a short connector needs none of its own.
    for(double station=length/8;station<length;station+=length/4) {
        const auto at=pointAlong(geometry,station);
        const auto ahead=pointAlong(geometry,std::min(length,station+0.05));
        const double angle=std::atan2(ahead.y-at.y,ahead.x-at.x);
        QPolygonF arrow;
        for(double offset:{0.0,2.5,-2.5})arrow<<QPointF(at.x+r*std::cos(angle+offset),at.y+r*std::sin(angle+offset));
        auto* item=scene_.addPolygon(arrow,QPen(Qt::NoPen),QBrush(colour));
        item->setZValue(200008);item->setData(0,QStringLiteral("route-arrow"));
    }
}
}
