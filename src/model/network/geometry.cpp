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
Point directionAlong(const std::vector<Point>& points,double station,bool arriving) {
    if(!std::isfinite(station))throw std::invalid_argument("INVALID_GEOMETRY");
    double remaining=std::max(0.,station);std::optional<Point> last;
    for(std::size_t i=1;i<points.size();++i) {
        const double dx=points[i].x-points[i-1].x,dy=points[i].y-points[i-1].y,length=std::hypot(dx,dy);
        if(!std::isfinite(length))throw std::invalid_argument("INVALID_GEOMETRY");
        if(length<=0)continue;
        last=Point{dx/length,dy/length};
        if(remaining<length || (arriving && remaining<=length))return *last;
        remaining-=length;
    }
    if(last)return *last;
    throw std::invalid_argument("INVALID_GEOMETRY");
}
std::vector<Point> trimSelfIntersections(const std::vector<Point>& points) {
    // An offset of a bend tighter than the offset loops back on itself. The swept area is still
    // road, so the fill is right either way, but the line drawn round it must not double back:
    // cut every loop out and join the two segments at the point where they cross.
    std::vector<Point> result;
    for(std::size_t i=0;i+1<points.size();++i) {
        result.push_back(points[i]);
        const auto a=points[i],b=points[i+1];
        const double rx=b.x-a.x,ry=b.y-a.y;
        for(std::size_t j=points.size()-1;j>i+2;--j) {
            const auto c=points[j-1],d=points[j];
            const double sx=d.x-c.x,sy=d.y-c.y,denominator=rx*sy-ry*sx;
            if(std::abs(denominator)<1e-12)continue;
            const double t=((c.x-a.x)*sy-(c.y-a.y)*sx)/denominator;
            const double u=((c.x-a.x)*ry-(c.y-a.y)*rx)/denominator;
            if(t<0 || t>1 || u<0 || u>1)continue;
            result.push_back({a.x+t*rx,a.y+t*ry});
            i=j-1; // Resume from the far side of the loop, which the crossing point replaces.
            break;
        }
    }
    if(!points.empty())result.push_back(points.back());
    return result;
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
    if(!link.boundaryMarkings.empty()) {
        if(link.boundaryMarkings.size()!=link.lanes.size()+1)throw std::invalid_argument("EDIT_LANES");
        std::vector<MarkingType> markings(lanes.size()+1,MarkingType::dashed);
        markings.front()=markings.back()=MarkingType::solid;
        const auto shift=leading?static_cast<std::ptrdiff_t>(lanes.size())-
            static_cast<std::ptrdiff_t>(link.lanes.size()):0;
        for(std::size_t i=0;i<link.boundaryMarkings.size();++i) {
            const auto next=static_cast<std::ptrdiff_t>(i)+shift;
            if(next>=0 && next<static_cast<std::ptrdiff_t>(markings.size()))
                markings[static_cast<std::size_t>(next)]=link.boundaryMarkings[i];
        }
        link.boundaryMarkings=std::move(markings);
    }
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
std::vector<Point> polylineSpan(const std::vector<Point>& points,double from,double to) {
    const double total=polylineLength(points);
    if(points.size()<2 || !std::isfinite(from) || !std::isfinite(to) || from<0 || to>total || to<=from)
        throw std::invalid_argument("INVALID_GEOMETRY");
    // The whole polyline is returned as itself, not rebuilt from two pointAlong calls, so a lane
    // with nothing attached to its body is bit-for-bit the lane it was before sectioning existed.
    if(from==0 && to==total)return points;
    std::vector<Point> result{pointAlong(points,from)};
    double station=0;
    for(std::size_t i=1;i+1<points.size();++i) {
        station+=std::hypot(points[i].x-points[i-1].x,points[i].y-points[i-1].y);
        // Strict comparisons on both sides: a cut landing exactly on a vertex is already carried
        // by the pointAlong ends, and admitting it here would repeat the point.
        if(station>from && station<to)result.push_back(points[i]);
    }
    result.push_back(pointAlong(points,to));
    return result;
}
std::vector<Point> offsetGeometry(const std::vector<Point>& geometry,double offset) {
    return offsetGeometry(geometry,std::vector<double>(geometry.size(),offset));
}
std::vector<Point> offsetGeometry(const std::vector<Point>& geometry,const std::vector<double>& offsets) {
    if(offsets.size()!=geometry.size())throw std::invalid_argument("INVALID_GEOMETRY");
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
        points.push_back({p.x+miter.x*offsets[i],p.y+miter.y*offsets[i]});
    }
    return points;
}
}
