#include "../src/editor/lane_change_pose.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace trafficsim;
namespace {
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
void near(Point a,Point b,double e=1e-7){require(std::hypot(a.x-b.x,a.y-b.y)<e,"Journey pose differs");}
Scenario scenario() {
    Scenario s;s.segments={{"a",100,{}},{"b",100,{}},{"c",100,{}}};
    s.routes={{"a",{"a"}},{"b",{"b"}},{"c",{"c"}}};
    VehicleType t;t.length=4.5;t.axles=VehicleAxles{2.7,.9,.9};s.vehicleTypes={t};return s;
}
const std::map<std::string,std::vector<Point>> geometry{{"a",{{0,0},{100,0}}},
    {"b",{{0,3.5},{100,3.5}}},{"c",{{0,7},{100,7}}}};
Vehicle changer(){Vehicle v;v.routeIndex=1;v.distance=20;v.speed=10;v.laneChangeTrace={{0,1,20,20,10}};return v;}
AxleVehiclePose at(const LaneChangePath& path,Vehicle v,double distance) {
    v.distance=distance;const auto p=path.pose(v);require(p.has_value(),"Valid journey has no pose");return *p;
}
// Independent RK4 on the smooth analytic front guide, not the sampled polyline.
double reference(double travel,double lever) {
    double theta=0;const int count=static_cast<int>(std::ceil(travel/.001));
    if(!count)return 0;
    const double h=travel/count;
    const auto rate=[&](double x,double t) {
        const double f=std::clamp(x/30.,0.,1.);
        const double dy=3.5*30*f*f*(1-f)*(1-f)/30;
        return (-std::sin(t)+dy*std::cos(t))/lever;
    };
    for(int i=0;i<count;++i) {
        const double x=i*h,k1=rate(x,theta),k2=rate(x+h/2,theta+h*k1/2),
            k3=rate(x+h/2,theta+h*k2/2),k4=rate(x+h,theta+h*k3);
        theta+=h*(k1+2*k2+2*k3+k4)/6;
    }
    return theta;
}
void analyticAndSlip() {
    auto s=scenario();auto heavy=s.vehicleTypes.front();heavy.length=12;heavy.axles=VehicleAxles{6,2,4};
    for(const auto& type:{s.vehicleTypes.front(),heavy}) {
        s.vehicleTypes.front()=type;auto v=changer();const double lever=vehicleAxles(type).wheelbase+vehicleAxles(type).frontOverhang;
        LaneChangePath path(s,v,geometry,{}),fine(s,v,geometry,{},.01);
        near(at(path,v,20).front,{20,0});near(at(path,v,20).heading,{1,0});
        for(double x:{1.,8.,15.,25.,30.,35.,60.}) {
            const auto p=at(path,v,20+x);const double f=std::min(x/30.,1.);
            near(p.front,{20+x,3.5*f*f*f*(10+f*(-15+6*f))});
            require(std::abs(std::atan2(p.heading.y,p.heading.x)-reference(x,lever))<2e-5,"Heading differs from analytic no-slip guide");
            near(p.heading,at(fine,v,20+x).heading,2e-5);
            require(std::abs(std::hypot(p.frontAxle.x-p.rearAxle.x,p.frontAxle.y-p.rearAxle.y)-vehicleAxles(type).wheelbase)<1e-9,"Wheelbase changed");
            require(std::abs(std::hypot(p.front.x-p.rearBumper.x,p.front.y-p.rearBumper.y)-type.length)<1e-9,"Body length changed");
        }
        const double d=33.037,h=.001;const auto p=at(path,v,d),a=at(path,v,d-h),b=at(path,v,d+h);
        const Point velocity{(b.rearAxle.x-a.rearAxle.x)/(2*h),(b.rearAxle.y-a.rearAxle.y)/(2*h)};
        require(std::abs(-velocity.x*p.heading.y+velocity.y*p.heading.x)<.001,"Rear slips sideways on lane change");
        require(at(path,v,50).heading.y>.01,"Setup: heading must still settle after the front reaches its lane");
        near(at(path,v,50-1e-6).heading,at(path,v,50+1e-6).heading,1e-6);
        v.speed=0;near(at(path,v,d).rearAxle,p.rearAxle,1e-12); // stopped, speed cannot rotate/slide body
        at(path,v,90);at(path,v,20);near(at(path,v,d).heading,p.heading,1e-12);
    }
}
void curvedJoin() {
    auto s=scenario();auto v=changer();auto curved=geometry;
    curved["b"]={{0,3.5},{40,3.5},{40,63.5}};
    LaneChangePath path(s,v,curved,{}),fine(s,v,curved,{},.01);
    for(double d:{39.9,40.,40.1,50.,60.,90.})
        near(at(path,v,d).heading,at(fine,v,d).heading,1e-4);
    near(at(path,v,40-1e-6).front,at(path,v,40+1e-6).front,3e-6);
    near(at(path,v,40-1e-6).heading,at(path,v,40+1e-6).heading,1e-6);
    near(at(path,v,60).front,{40,23.5});
}
void sourceHeadingAndConnectorPaint() {
    auto s=scenario();auto v=changer();auto bent=geometry;
    bent["a"]={{0,0},{10,0},{10,90}};bent["b"]={{3.5,0},{13.5,0},{13.5,90}};
    RearAxlePath source({{"a",0,0,100}},s.vehicleTypes.front(),bent,{});
    const auto prior=source.pose(20,{10,10});require(prior.has_value(),"Setup: source pose missing");
    require(prior->heading.x>.01,"Setup: initial rear heading must differ from guide tangent");
    LaneChangePath journey(s,v,bent,{});near(at(journey,v,20).heading,prior->heading,1e-12);
    const auto eq=makeConnectorEquation({Point{0,3.5},{30,3.5},{60,20},{60,50}});
    s.segments[1].length=eq.arcStations.back();auto straight=geometry;
    straight["b"]={{0,3.5},{60,50}};auto edited=straight;
    edited["b"]={{0,3.5},{-50,70},{60,50}};
    const std::map<std::string,ConnectorEquation> equations{{"b",eq}};
    LaneChangePath direct(s,v,straight,equations),painted(s,v,edited,equations);
    for(double d:{20.,35.123,50.,60.}) {
        near(at(direct,v,d).front,at(painted,v,d).front,1e-12);
        near(at(direct,v,d).heading,at(painted,v,d).heading,1e-12);
    }
}
void overlappingAndMapping() {
    auto s=scenario();auto v=changer();LaneChangePath first(s,v,geometry,{});
    const auto prior=at(first,v,28);
    // Different target route stations: records capture the actual engine remap.
    // Move c's guide upstream by 23 m so its station 5 aligns with source station 28.
    auto mapped=geometry;mapped["c"]={{23,7},{123,7}};
    v.laneChangeTrace.push_back({1,2,28,5,10});v.routeIndex=2;v.distance=5;
    LaneChangePath second(s,v,mapped,{});
    require(prior.front.y>0 && prior.front.y<3.5,"Setup: second change must overlap the first");
    near(at(second,v,5).front,prior.front);near(at(second,v,5).heading,prior.heading,2e-5);
    near(at(second,v,5+1e-6).front,prior.front,2e-6);
    near(at(second,v,5+1e-6).heading,prior.heading,2e-5);
    near(at(second,v,45).front,{68,7});
    require(!first.matches(v),"New change retained old cache key");
    v.laneChangeTrace[1].fromRoute=0;LaneChangePath bad(s,v,mapped,{});
    require(!bad.pose(v),"Inconsistent trace produced a pose");
    v=changer();auto missing=geometry;missing.erase("a");LaneChangePath absent(s,v,missing,{});
    require(!absent.pose(v),"Missing source produced invented guidance");
    LaneChangePath bounded(s,v,geometry,{},1e-300);require(!bounded.pose(v),"Tiny step escaped memory bound");
    auto changed=v;changed.typeIndex=1;require(!first.matches(changed),"Type change retained old cache key");
}
}
int main(){try{analyticAndSlip();curvedJoin();sourceHeadingAndConnectorPaint();overlappingAndMapping();std::cout<<"Lane-change guide tests passed\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
