#include "vehicle_pose.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
namespace {
struct RouteSample {Point point;const std::vector<Point>* geometry;const ConnectorEquation* equation;double station;};
std::optional<RouteSample> sample(const std::vector<RoutePart>& parts,double distance,
    const std::map<std::string,std::vector<Point>>& geometry,
    const std::map<std::string,ConnectorEquation>& equations) {
    if(parts.empty())return {};
    const RoutePart* part=&parts.back();
    for(const auto& candidate:parts)if(distance<candidate.start+candidate.length){part=&candidate;break;}
    const auto g=geometry.find(part->segmentId);if(g==geometry.end())return {};
    const auto c=equations.find(part->segmentId);
    const auto* equation=c==equations.end()?nullptr:&c->second;
    const double station=std::clamp(distance-part->start,0.,part->length);
    const auto point=equation?equationPoint(*equation,equationParameter(*equation,station)):pointAlong(g->second,station);
    return RouteSample{point,&g->second,equation,station};
}
Point direction(const RouteSample& s) {
    return s.equation?equationDerivative(*s.equation,equationParameter(*s.equation,s.station)):
        directionAlong(*s.geometry,s.station,true);
}
}
std::optional<VehiclePose> vehiclePose(const std::vector<RoutePart>& parts,double distance,double length,
    const std::map<std::string,std::vector<Point>>& geometry,
    const std::map<std::string,ConnectorEquation>& equations) {
    if(!std::isfinite(distance) || distance<0 || !std::isfinite(length) || length<=0)
        throw std::invalid_argument("INVALID_VEHICLE_POSE");
    const auto front=sample(parts,distance,geometry,equations),rear=sample(parts,distance-length,geometry,equations);
    if(!front || !rear)return {};
    Point behind=rear->point;
    if(distance-length<parts.front().start) {
        const auto tangent=direction(*rear);const double norm=std::hypot(tangent.x,tangent.y);
        if(norm<=1e-9)return {};
        const double extension=distance-length-parts.front().start;
        behind={behind.x+extension*tangent.x/norm,behind.y+extension*tangent.y/norm};
    }
    Point heading{front->point.x-behind.x,front->point.y-behind.y};
    // A route can loop back onto its rear sample. Keep a finite direction at that
    // exceptional pose; ordinary segment transitions always use the same chord.
    if(std::hypot(heading.x,heading.y)<=1e-6)heading=direction(*front);
    if(!std::isfinite(heading.x) || !std::isfinite(heading.y) || std::hypot(heading.x,heading.y)<=1e-9)return {};
    return VehiclePose{front->point,heading};
}
}
