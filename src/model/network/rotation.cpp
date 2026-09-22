#include "rotation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <set>
#include <stdexcept>
namespace trafficsim {
std::vector<std::string> rotationObjects(const Network& n,const std::vector<std::string>& ids) {
    const std::set<std::string> selected(ids.begin(),ids.end());
    std::set<std::string> links,paths;
    std::vector<std::string> result;
    for(const auto& l:n.links)if(selected.contains(l.id)) {
        links.insert(l.id);result.push_back(l.id);
    }
    for(const auto& c:n.connectors)
        if(selected.contains(c.id) || (links.contains(c.from.linkId) && links.contains(c.to.linkId))) {
            result.push_back(c.id);
            for(int i=0;i<std::max(c.fromLaneCount,c.toLaneCount);++i)paths.insert(connectorPathId(c,i));
        }
    for(const auto& h:n.signalHeads)
        if(h.connectorId.empty()?links.contains(h.lane.linkId):paths.contains(h.connectorId))result.push_back(h.id);
    return result;
}
std::optional<Point> rotationCentre(const Network& n,const std::vector<std::string>& ids) {
    const auto objects=rotationObjects(n,ids);
    const std::set<std::string> rotating(objects.begin(),objects.end());
    Point low{std::numeric_limits<double>::max(),std::numeric_limits<double>::max()};
    Point high{-low.x,-low.y};bool found=false;
    const auto include=[&](const std::vector<Point>& geometry) {
        for(const auto& p:geometry) {
            low.x=std::min(low.x,p.x);low.y=std::min(low.y,p.y);
            high.x=std::max(high.x,p.x);high.y=std::max(high.y,p.y);found=true;
        }
    };
    for(const auto& l:n.links)if(rotating.contains(l.id)) {
        include(laneBoundaryGeometry(l,0,n.drivingSide));
        include(laneBoundaryGeometry(l,l.lanes.size(),n.drivingSide));
    }
    for(const auto& c:n.connectors)if(rotating.contains(c.id)) {
        const auto boundaries=connectorBoundaries(n,c);
        include(boundaries.front());include(boundaries.back());
    }
    if(!found)return {};
    return Point{std::midpoint(low.x,high.x),std::midpoint(low.y,high.y)};
}
Point rotatePoint(Point p,Point pivot,double degrees) {
    if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(pivot.x) ||
       !std::isfinite(pivot.y) || !std::isfinite(degrees))throw std::invalid_argument("INVALID_GEOMETRY");
    const double angle=std::remainder(degrees,360.);
    if(angle==0)return p;
    // Exact quarter turns avoid introducing tiny trigonometric errors into straight roads.
    double sine{},cosine{};
    if(angle==90)sine=1;
    else if(angle==-90)sine=-1;
    else if(std::abs(angle)==180)cosine=-1;
    else {const double radians=angle*std::numbers::pi/180.;sine=std::sin(radians);cosine=std::cos(radians);}
    const double x=p.x-pivot.x,y=p.y-pivot.y;
    const Point result{pivot.x+x*cosine-y*sine,pivot.y+x*sine+y*cosine};
    if(!std::isfinite(result.x) || !std::isfinite(result.y))throw std::invalid_argument("INVALID_GEOMETRY");
    return result;
}
}
