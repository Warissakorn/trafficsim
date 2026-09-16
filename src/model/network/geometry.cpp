#include "network.hpp"
#include <limits>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
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
std::vector<Point> offsetGeometry(const std::vector<Point>& geometry,double offset) {
    std::vector<Point> points;
    for (std::size_t i = 0; i < geometry.size(); ++i) {
        const auto& p = geometry[i];
        const auto& previous = geometry[i == 0 ? 0 : i - 1];
        const auto& next = geometry[std::min(geometry.size() - 1, i + 1)];
        double dx = next.x - previous.x, dy = next.y - previous.y;
        if (dx == 0 && dy == 0) { dx = next.x - p.x; dy = next.y - p.y; }
        const double norm = std::hypot(dx, dy);
        points.push_back(norm == 0 ? p : Point{p.x - dy / norm * offset, p.y + dx / norm * offset});
    }
    return points;
}
}
