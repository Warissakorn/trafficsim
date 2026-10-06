#include "road_crossbar.hpp"
#include "ui_design_tokens.hpp"
#include <QPainterPathStroker>
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
// At a mouth the normal can meet one Connector rail and one attached Link rail.
// These are actual road edges, never extrapolated imaginary rail segments.
std::pair<std::vector<Point>,std::vector<Point>> attachmentRails(const Network& n,
    const LaneReference& first,const LaneReference& last) {
    for(const auto& l:n.links)if(l.id==first.linkId && l.id==last.linkId) {
        std::size_t a=l.lanes.size(),b=l.lanes.size();
        for(std::size_t k=0;k<l.lanes.size();++k) {
            if(l.lanes[k].id==first.laneId)a=k;
            if(l.lanes[k].id==last.laneId)b=k;
        }
        if(a<l.lanes.size() && b<l.lanes.size())return {
            laneBoundaryGeometry(l,std::min(a,b),n.drivingSide),laneBoundaryGeometry(l,std::max(a,b)+1,n.drivingSide)};
    }
    return {};
}
}
std::optional<RoadCrossbar> roadCrossbar(const std::vector<Point>& centre,
    const std::vector<Point>& firstRail, const std::vector<Point>& secondRail,
    double station, int level,const std::vector<Point>& firstContinuation,const std::vector<Point>& secondContinuation) {
    if(centre.size()<2 || firstRail.size()<2 || secondRail.size()<2 || !std::isfinite(station))return {};
    const double length=polylineLength(centre);
    if(!(length>0) || !std::isfinite(length))return {};
    station=std::clamp(station,0.,length);
    const auto origin=pointAlong(centre,station),u=directionAlong(centre,station,station==length);
    const auto meet=[&](const std::vector<Point>& rail,const std::vector<Point>& continuation)->std::optional<Point> {
        const auto guess=pointAlong(rail,matchedStation(centre,rail,station));
        std::optional<Point> result;double nearest=INFINITY;
        const auto scan=[&](const std::vector<Point>& edge) {
        for(std::size_t i=1;i<edge.size();++i) {
            const Point d{edge[i].x-edge[i-1].x,edge[i].y-edge[i-1].y};
            const double denominator=d.x*u.x+d.y*u.y;
            if(std::abs(denominator)<1e-12)continue;
            const double t=((origin.x-edge[i-1].x)*u.x+(origin.y-edge[i-1].y)*u.y)/denominator;
            if(t < -1e-8 || t > 1+1e-8)continue;
            const Point hit{edge[i-1].x+std::clamp(t,0.,1.)*d.x,edge[i-1].y+std::clamp(t,0.,1.)*d.y};
            const double distance=std::hypot(hit.x-guess.x,hit.y-guess.y);
            if(distance<nearest){nearest=distance;result=hit;}
        }
        };
        scan(rail);scan(continuation);
        return result;
    };
    const auto a=meet(firstRail,firstContinuation),b=meet(secondRail,secondContinuation);
    if(!a || !b || std::hypot(a->x-b->x,a->y-b->y)<1e-9)return {};
    return RoadCrossbar{*a,*b,level};
}
std::optional<RoadCrossbar> objectCrossbar(const Network& n,const std::string& id,bool end) {
    try {
        for(const auto& l:n.links)if(l.id==id) {
            const auto axis=linkCentreline(l,n.drivingSide);
            return roadCrossbar(axis,laneBoundaryGeometry(l,0,n.drivingSide),
                laneBoundaryGeometry(l,l.lanes.size(),n.drivingSide),end?polylineLength(axis):0,l.level);
        }
        for(const auto& c:n.connectors)if(c.id==id) {
            const auto rails=connectorBoundaries(n,c);
            if(rails.size()<2)return {};
            const auto paths=connectorPaths(n,c);
            if(paths.empty() || rails.front().size()!=rails.back().size())return {};
            // Rail midpoints agree with edited Connector paths, including its mouths.
            std::vector<Point> axis;
            for(std::size_t i=0;i<rails.front().size();++i)
                axis.push_back({(rails.front()[i].x+rails.back()[i].x)/2,(rails.front()[i].y+rails.back()[i].y)/2});
            const auto extra=attachmentRails(n,end?paths.front().to:paths.front().from,end?paths.back().to:paths.back().from);
            return roadCrossbar(axis,rails.front(),rails.back(),end?polylineLength(axis):0,c.level,extra.first,extra.second);
        }
    } catch(const std::exception&) {}
    return {};
}
std::optional<RoadCrossbar> signalCrossbar(const Network& n,const NetworkSignalHead& h) {
    try {
        if(h.connectorId.empty())for(const auto& l:n.links)if(l.id==h.lane.linkId)
            for(std::size_t k=0;k<l.lanes.size();++k)if(l.lanes[k].id==h.lane.laneId)
                return roadCrossbar(laneGeometry(l,h.lane.laneId,n.drivingSide),
                    laneBoundaryGeometry(l,k,n.drivingSide),laneBoundaryGeometry(l,k+1,n.drivingSide),h.position,l.level);
        for(const auto& c:n.connectors) {
            const auto paths=connectorPaths(n,c);
            for(std::size_t k=0;k<paths.size();++k)if(paths[k].id==h.connectorId) {
                const auto rails=connectorBoundaries(n,c);
                if(k+1>=rails.size())return {};
                const double length=connectorPathLength(paths[k]);
                const auto origin=connectorPathPoint(paths[k],h.position);
                const bool source=laneContains(n,paths[k].from,origin);
                const bool target=laneContains(n,paths[k].to,origin);
                const auto ref=source && (!target || h.position<length/2)?paths[k].from:paths[k].to;
                const auto extra=source || target?attachmentRails(n,ref,ref):std::pair<std::vector<Point>,std::vector<Point>>{};
                return roadCrossbar(paths[k].geometry,rails[k],rails[k+1],h.position,c.level,extra.first,extra.second);
            }
        }
    } catch(const std::exception&) {}
    return {};
}
RoadCrossbarItem::RoadCrossbarItem(const RoadCrossbar& bar,QColor colour,double scale,bool dashed)
    : QGraphicsLineItem(bar.first.x,bar.first.y,bar.second.x,bar.second.y),
      hitWidth_(editorDesign::crossbarHitPixels/std::max(1e-9,std::abs(scale))) {
    QPen pen(colour,editorDesign::crossbarPixels,dashed?Qt::DashLine:Qt::SolidLine);
    pen.setCosmetic(true);pen.setCapStyle(Qt::FlatCap);setPen(pen);
}
QPainterPath RoadCrossbarItem::shape() const {
    QPainterPath linePath;linePath.moveTo(line().p1());linePath.lineTo(line().p2());
    // Widen along the road, never past the lane edges into a neighbouring head.
    QPainterPathStroker stroke;stroke.setWidth(hitWidth_);stroke.setCapStyle(Qt::FlatCap);
    return stroke.createStroke(linePath);
}
QRectF RoadCrossbarItem::boundingRect() const { return shape().boundingRect(); }
}
