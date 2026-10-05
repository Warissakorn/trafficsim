#include "rear_axle_pose.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
namespace {
Point unit(Point p) {
    const double n=std::hypot(p.x,p.y);
    if(!std::isfinite(n) || n<=1e-9)throw std::invalid_argument("INVALID_VEHICLE_POSE");
    return {p.x/n,p.y/n};
}
}
RearAxlePath::RearAxlePath(const std::vector<RoutePart>& parts,const VehicleType& type,
    const std::map<std::string,std::vector<Point>>& geometry,
    const std::map<std::string,ConnectorEquation>& equations,double maxStep):axles_(vehicleAxles(type)) {
    const double lever=axles_.wheelbase+axles_.frontOverhang;
    if(!std::isfinite(maxStep) || maxStep<=0 || !std::isfinite(lever) || axles_.wheelbase<=0 ||
        !std::isfinite(axles_.rearOverhang) || axles_.frontOverhang<0 || axles_.rearOverhang<0 ||
        !std::isfinite(type.length) || type.length<=0 || std::abs(lever+axles_.rearOverhang-type.length)>1e-6*std::max(1.,type.length))
        throw std::invalid_argument("INVALID_VEHICLE_POSE");
    if(parts.empty())return;
    Point previous{};double station=0,angle=0;
    try {for(const auto& part:parts) {
        const auto g=geometry.find(part.segmentId);
        if(g==geometry.end() || g->second.size()<2 || !std::isfinite(part.length) || part.length<=0 ||
            std::abs(part.start-station)>1e-6){samples_.clear();return;}
        const auto eq=equations.find(part.segmentId);
        const auto* curve=eq==equations.end()?nullptr:&eq->second;
        const auto point=[&](double s) {return curve?equationPoint(*curve,equationParameter(*curve,s)):pointAlong(g->second,s);};
        const auto tangent=[&](double s) {return unit(curve?equationDerivative(*curve,equationParameter(*curve,s)):
            directionAlong(g->second,s,true));};
        const Point start=point(0);
        if(!std::isfinite(start.x) || !std::isfinite(start.y)){samples_.clear();return;}
        if(!samples_.empty() && std::hypot(start.x-previous.x,start.y-previous.y)>1e-5){samples_.clear();return;}
        if(samples_.empty()) {
            const auto t=tangent(0);angle=std::atan2(t.y,t.x);samples_.push_back({0,angle});
        }
        // Integrate independently inside each polyline leg, so a corner is never
        // smeared across an RK4 interval. Connector derivatives are evaluated directly.
        std::vector<double> breaks{0};double sum=0;
        if(!curve)for(std::size_t i=1;i<g->second.size();++i) {
            sum+=std::hypot(g->second[i].x-g->second[i-1].x,g->second[i].y-g->second[i-1].y);
            if(sum>breaks.back()+1e-9 && sum<part.length-1e-9)breaks.push_back(sum);
        }
        breaks.push_back(part.length);
        for(std::size_t leg=1;leg<breaks.size();++leg) {
            const double begin=breaks[leg-1],end=breaks[leg];
            const double required=std::ceil((end-begin)/std::min(maxStep,lever/20));
            // Bound derived display memory, including pathological tiny wheelbases.
            if(required>1000000-samples_.size()){samples_.clear();return;}
            const int count=static_cast<int>(required);
            const double h=(end-begin)/count;
            const auto straight=curve?Point{}:tangent((begin+end)/2);
            const auto rate=[&](double s,double theta) {
                const auto t=curve?tangent(s):straight;
                // R=F-a*u. The rear no-slip condition R' dot normal(u)=0
                // gives theta'=(F' dot normal(u))/a, with F' a unit tangent.
                return (-t.x*std::sin(theta)+t.y*std::cos(theta))/lever;
            };
            for(int i=0;i<count;++i) {
                const double s=begin+i*h;
                const double k1=rate(s,angle),k2=rate(s+h/2,angle+h*k1/2),
                    k3=rate(s+h/2,angle+h*k2/2),k4=rate(s+h,angle+h*k3);
                angle+=h*(k1+2*k2+2*k3+k4)/6;
                samples_.push_back({part.start+begin+(i+1)*h,angle});
            }
        }
        previous=point(part.length);station=part.start+part.length;
    }}catch(const std::invalid_argument&){samples_.clear();}
}
std::optional<AxleVehiclePose> RearAxlePath::pose(double distance,Point front) const {
    if(!std::isfinite(distance) || distance<0 || !std::isfinite(front.x) || !std::isfinite(front.y))
        throw std::invalid_argument("INVALID_VEHICLE_POSE");
    if(samples_.empty())return {};
    auto right=std::lower_bound(samples_.begin(),samples_.end(),distance,
        [](const Sample& s,double d){return s.station<d;});
    double angle=samples_.back().angle;
    if(right==samples_.begin())angle=right->angle;
    else if(right!=samples_.end()) {
        const auto& left=*(right-1);const double f=(distance-left.station)/(right->station-left.station);
        angle=left.angle+f*(right->angle-left.angle); // unwrapped angles, including +/-pi
    }
    const Point heading{std::cos(angle),std::sin(angle)};
    const auto behind=[&](double d){return Point{front.x-d*heading.x,front.y-d*heading.y};};
    return AxleVehiclePose{front,behind(axles_.wheelbase+axles_.frontOverhang),
        behind(axles_.frontOverhang),behind(axles_.wheelbase+axles_.frontOverhang+axles_.rearOverhang),heading};
}
}
