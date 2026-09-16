#include "network.hpp"
#include <limits>
#include <optional>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
namespace {
// Longest miter, as a multiple of the offset, before a hairpin is cut back.
constexpr double kMiterLimit=4;
}
std::string signalSegment(const NetworkSignalHead& head) {
    return head.connectorId.empty()?head.lane.laneId:head.connectorId;
}
double stationOfClosestPoint(const std::vector<Point>& geometry, Point p) {
    double best=std::numeric_limits<double>::infinity(), station=0, result=0;
    for(std::size_t i=1;i<geometry.size();++i) {
        const auto a=geometry[i-1],b=geometry[i];const double dx=b.x-a.x,dy=b.y-a.y,len=std::hypot(dx,dy);
        if(len<=0)continue;
        const double t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/(len*len),0.,1.);
        const double distance=std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);
        if(distance<best){best=distance;result=station+t*len;}station+=len;
    }
    return result;
}
double polylineLength(const std::vector<Point>& points) {
    double length = 0;
    for (std::size_t i = 1; i < points.size(); ++i)
        length += std::hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
    return length;
}
Point pointAlong(const std::vector<Point>& points, double distance) {
    const double total = polylineLength(points);
    if (points.size() < 2 || !std::isfinite(distance) || !std::isfinite(total) || total <= 0 ||
        std::any_of(points.begin(), points.end(), [](const auto& p) { return !std::isfinite(p.x) || !std::isfinite(p.y); }))
        throw std::invalid_argument("INVALID_GEOMETRY");
    double remaining = std::max(0.0, distance);
    for (std::size_t i = 1; i < points.size(); ++i) {
        const auto& a = points[i - 1]; const auto& b = points[i];
        const double length = std::hypot(b.x - a.x, b.y - a.y);
        if (length == 0) continue;
        if (remaining <= length) {
            const double ratio = remaining / length;
            return {a.x + ratio * (b.x - a.x), a.y + ratio * (b.y - a.y)};
        }
        remaining -= length;
    }
    return points.back();
}
std::vector<Point> laneGeometry(const Link& link, const std::string& laneId, DrivingSide side) {
    if (side != DrivingSide::left && side != DrivingSide::right) throw std::invalid_argument("INVALID_DRIVING_SIDE");
    const auto lane = std::find_if(link.lanes.begin(), link.lanes.end(), [&](const auto& l) { return l.id == laneId; });
    if (lane == link.lanes.end()) throw std::invalid_argument("UNKNOWN_LANE");
    double totalWidth = 0, before = 0;
    for (auto it = link.lanes.begin(); it != link.lanes.end(); ++it) {
        totalWidth += it->width;
        if (it < lane) before += it->width;
    }
    const double offset = (link.laneOffset + totalWidth / 2 - before - lane->width / 2) * (side == DrivingSide::left ? 1 : -1);
    return offsetGeometry(link.geometry,offset);
}
std::vector<Point> laneBoundaryGeometry(const Link& link,std::size_t boundary,DrivingSide side) {
    if(boundary>link.lanes.size())throw std::invalid_argument("EDIT_LANES");
    double total=0,before=0;
    for(std::size_t i=0;i<link.lanes.size();++i){total+=link.lanes[i].width;if(i<boundary)before+=link.lanes[i].width;}
    return offsetGeometry(link.geometry,(link.laneOffset+total/2-before)*(side==DrivingSide::left?1.:-1.));
}
void replaceLaneBundle(Link& link,std::vector<Lane> lanes,bool leading) {
    double oldWidth=0,newWidth=0;
    for(const auto& l:link.lanes)oldWidth+=l.width;
    for(const auto& l:lanes)newWidth+=l.width;
    link.laneOffset+=(newWidth-oldWidth)*(leading?.5:-.5);
    link.lanes=std::move(lanes);
}
std::vector<Point> linkCentreline(const Link& link,DrivingSide side) {
    if(side!=DrivingSide::left && side!=DrivingSide::right)throw std::invalid_argument("INVALID_DRIVING_SIDE");
    return offsetGeometry(link.geometry,link.laneOffset*(side==DrivingSide::left?1.:-1.));
}
double matchedStation(const std::vector<Point>& from,const std::vector<Point>& to,double station) {
    if(from.size()!=to.size() || from.size()<2)throw std::invalid_argument("INVALID_GEOMETRY");
    if(!std::isfinite(station))throw std::invalid_argument("INVALID_GEOMETRY");
    double remaining=std::max(0.,station),matched=0;
    for(std::size_t i=1;i<from.size();++i) {
        const double length=std::hypot(from[i].x-from[i-1].x,from[i].y-from[i-1].y);
        const double step=std::hypot(to[i].x-to[i-1].x,to[i].y-to[i-1].y);
        if(length>0 && remaining<=length)return matched+step*remaining/length;
        if(length>0)remaining-=length;
        matched+=step;
    }
    return matched; // Past the end of `from`, which clamps to the end of `to`.
}
std::vector<Point> offsetGeometry(const std::vector<Point>& geometry,double offset) {
    // A corner needs a miter, not a plain normal. Offsetting a bend vertex by `offset` along
    // the average normal leaves it offset*cos(theta/2) from the original line, so both lane
    // edges pull in and the carriageway visibly pinches at every bend: 18% at 63 degrees,
    // 30% at a right angle. The miter vector (n1+n2)/(1+d1.d2) has length 1/cos(theta/2),
    // which is exactly the distance from the corner to the intersection of the two offset
    // legs. A straight polyline reduces to the old single normal, bit for bit.
    std::vector<Point> points;
    const auto unit=[](Point from,Point to)->std::optional<Point> {
        const double dx=to.x-from.x,dy=to.y-from.y,norm=std::hypot(dx,dy);
        if(norm<=0)return {};
        return Point{dx/norm,dy/norm};
    };
    for (std::size_t i = 0; i < geometry.size(); ++i) {
        const auto& p = geometry[i];
        // At an end there is only one segment, and its own normal is already exact.
        auto incoming = i==0 ? std::optional<Point>{} : unit(geometry[i-1],p);
        auto outgoing = i+1==geometry.size() ? std::optional<Point>{} : unit(p,geometry[i+1]);
        if(!incoming)incoming=outgoing;
        if(!outgoing)outgoing=incoming;
        if(!incoming){points.push_back(p);continue;} // Repeated points keep their position.
        const Point n1{-incoming->y,incoming->x},n2{-outgoing->y,outgoing->x};
        const double denominator=1+incoming->x*outgoing->x+incoming->y*outgoing->y;
        Point miter{n1.x+n2.x,n1.y+n2.y};
        // A turn sharper than about 151 degrees would spike towards infinity. Clamp it to the
        // same limit a renderer would, so a hairpin stays drawable and stays deterministic.
        if(denominator>2/(kMiterLimit*kMiterLimit)) { miter.x/=denominator;miter.y/=denominator; }
        else {
            const double norm=std::hypot(miter.x,miter.y);
            if(norm>0){miter.x=miter.x/norm*kMiterLimit;miter.y=miter.y/norm*kMiterLimit;}
            else miter=n1; // An exact reversal has no bisector; use the incoming normal.
        }
        points.push_back({p.x+miter.x*offset,p.y+miter.y*offset});
    }
    return points;
}
}
