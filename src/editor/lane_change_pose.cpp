#include "lane_change_pose.hpp"
#include "../core/routes.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
namespace {
using Geometry=std::map<std::string,std::vector<Point>>;
using Equations=std::map<std::string,ConnectorEquation>;
Point pointAt(const std::vector<RoutePart>& parts,double distance,const Geometry& geometry,const Equations& equations) {
    const auto* part=&parts.back();
    for(const auto& p:parts)if(distance<p.start+p.length){part=&p;break;}
    const auto& g=geometry.at(part->segmentId);
    const auto eq=equations.find(part->segmentId);
    const double local=distance-part->start,s=std::clamp(local,0.,part->length);
    Point p=eq==equations.end()?pointAlong(g,s):equationPoint(eq->second,equationParameter(eq->second,s));
    // A source lane can end before its changer finishes; extend its end tangent.
    // This extrapolation is display guidance, never an engine route extension.
    if(local!=s) {
        const auto t=eq==equations.end()?directionAlong(g,s,true):equationDerivative(eq->second,equationParameter(eq->second,s));
        const double n=std::hypot(t.x,t.y);
        if(n<=1e-9)throw std::invalid_argument("INVALID_VEHICLE_POSE");
        p.x+=(local-s)*t.x/n;p.y+=(local-s)*t.y/n;
    }
    return p;
}
}
LaneChangePath::LaneChangePath(const Scenario& scenario,const Vehicle& vehicle,
    const Geometry& geometry,const Equations& equations,double maxStep):trace_(vehicle.laneChangeTrace),type_(vehicle.typeIndex) {
    if(!std::isfinite(maxStep) || maxStep<=0)throw std::invalid_argument("INVALID_VEHICLE_POSE");
    if(trace_.empty() || type_>=scenario.vehicleTypes.size())return;
    const auto& type=scenario.vehicleTypes[type_];
    std::map<std::uint32_t,std::vector<RoutePart>> routes;
    const auto addRoute=[&](std::uint32_t id) {
        if(id>=scenario.routes.size())return false;
        if(routes.contains(id))return true;
        auto parts=routeParts(scenario,scenario.routes[id]);
        RearAxlePath check(parts,type,geometry,equations);
        if(!check.pose(0,{}))return false;
        routes.emplace(id,std::move(parts));return true;
    };
    std::vector<double> starts;double start=0;
    for(std::size_t i=0;i<trace_.size();++i) {
        const auto& c=trace_[i];
        if(!std::isfinite(c.fromDistance) || !std::isfinite(c.toDistance) || !std::isfinite(c.speed) ||
            c.fromDistance<0 || c.toDistance<0 || c.speed<0 || !addRoute(c.fromRoute) || !addRoute(c.toRoute))return;
        if(i) {
            const auto& previous=trace_[i-1];
            if(c.fromRoute!=previous.toRoute || c.fromDistance<previous.toDistance)return;
            start+=c.fromDistance-previous.toDistance;
        }
        const auto& from=routes.at(c.fromRoute);const auto& to=routes.at(c.toRoute);
        if(c.fromDistance>from.back().start+from.back().length || c.toDistance>to.back().start+to.back().length)return;
        starts.push_back(start);
    }
    const auto& last=trace_.back();if(vehicle.routeIndex!=last.toRoute)return;
    offset_=last.toDistance-starts.back();
    const auto& destination=routes.at(last.toRoute);
    const double end=destination.back().start+destination.back().length-offset_;
    if(end<=0 || std::ceil(end/maxStep)>500000)return; // bound derived memory/work
    const auto& source=routes.at(trace_.front().fromRoute);
    RearAxlePath sourcePath(source,type,geometry,equations);
    const auto initial=sourcePath.pose(trace_.front().fromDistance,
        pointAt(source,trace_.front().fromDistance,geometry,equations));
    if(!initial)return;
    const auto guide=[&](double travel) {
        Point p=pointAt(source,trace_.front().fromDistance+travel,geometry,equations);
        // Compose blends: a second change starts at the existing guide, including
        // its derivative, rather than resetting to the centre of the engine lane.
        for(std::size_t i=0;i<trace_.size() && starts[i]<=travel;++i) {
            const auto& c=trace_[i];
            const double f=std::clamp((travel-starts[i])/(3*std::max(c.speed,5.)),0.,1.);
            const double w=f*f*f*(10+f*(-15+6*f)); // quintic, zero first/second endpoint derivatives
            const auto q=pointAt(routes.at(c.toRoute),c.toDistance+travel-starts[i],geometry,equations);
            p={p.x+w*(q.x-p.x),p.y+w*(q.y-p.y)};
        }
        return p;
    };
    std::vector<double> breaks{0,end};
    const auto splitRoute=[&](std::uint32_t route,double offset) {
        for(const auto& part:routes.at(route)) {
            breaks.push_back(part.start-offset);breaks.push_back(part.start+part.length-offset);
            if(!equations.contains(part.segmentId)) {
                double station=part.start;
                const auto& g=geometry.at(part.segmentId);
                for(std::size_t j=1;j<g.size();++j) {
                    station+=std::hypot(g[j].x-g[j-1].x,g[j].y-g[j-1].y);
                    breaks.push_back(station-offset);
                }
            }
        }
    };
    splitRoute(trace_.front().fromRoute,trace_.front().fromDistance);
    for(std::size_t i=0;i<trace_.size();++i) {
        splitRoute(trace_[i].toRoute,trace_[i].toDistance-starts[i]);
        breaks.push_back(starts[i]);breaks.push_back(starts[i]+3*std::max(trace_[i].speed,5.));
    }
    std::erase_if(breaks,[&](double x){return x<0 || x>end;});
    std::sort(breaks.begin(),breaks.end());
    breaks.erase(std::unique(breaks.begin(),breaks.end()),breaks.end());
    std::vector<Point> polyline{guide(0)};samples_.push_back({0,0,polyline.front()});
    double arc=0;
    for(std::size_t leg=1;leg<breaks.size();++leg) {
        const double begin=breaks[leg-1],length=breaks[leg]-begin;
        const auto count=static_cast<std::size_t>(std::ceil(length/maxStep));
        if(count>500000-samples_.size()){samples_.clear();return;}
        for(std::size_t j=1;j<=count;++j) {
            const double x=begin+length*static_cast<double>(j)/static_cast<double>(count);
            const auto p=guide(x);arc+=std::hypot(p.x-polyline.back().x,p.y-polyline.back().y);
            polyline.push_back(p);samples_.push_back({x,arc,p});
        }
    }
    if(arc<=0){samples_.clear();return;}
    rolling_.emplace(std::vector<RoutePart>{{"guide",0,0,arc}},type,
        Geometry{{"guide",std::move(polyline)}},Equations{},maxStep,initial->heading);
}
bool LaneChangePath::matches(const Vehicle& vehicle) const {
    return type_==vehicle.typeIndex && trace_==vehicle.laneChangeTrace;
}
std::optional<AxleVehiclePose> LaneChangePath::pose(const Vehicle& vehicle) const {
    if(!rolling_ || !matches(vehicle) || vehicle.routeIndex!=trace_.back().toRoute || !std::isfinite(vehicle.distance))return {};
    const double x=vehicle.distance-offset_;
    if(x<0 || x>samples_.back().travel+1e-7)return {};
    auto right=std::lower_bound(samples_.begin(),samples_.end(),x,[](const Sample& s,double d){return s.travel<d;});
    if(right==samples_.begin())return rolling_->pose(right->arc,right->front);
    if(right==samples_.end())right=samples_.end()-1;
    const auto& left=*(right-1);const double f=(x-left.travel)/(right->travel-left.travel);
    return rolling_->pose(left.arc+f*(right->arc-left.arc),
        {left.front.x+f*(right->front.x-left.front.x),left.front.y+f*(right->front.y-left.front.y)});
}
}
