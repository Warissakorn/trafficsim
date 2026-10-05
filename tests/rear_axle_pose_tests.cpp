#include "../src/editor/rear_axle_pose.hpp"
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace trafficsim;
namespace {
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
double angle(Point p){return std::atan2(p.y,p.x);}
void near(double a,double b,double e=1e-7){require(std::abs(a-b)<e,"Rear axle value differs from analytic expectation");}
void near(Point a,Point b,double e=1e-7){near(std::hypot(a.x-b.x,a.y-b.y),0,e);}
VehicleType car(){VehicleType t;t.length=4.5;t.axles=VehicleAxles{2.7,.9,.9};return t;}
AxleVehiclePose at(const RearAxlePath& path,double s,Point front) {
    const auto p=path.pose(s,front);require(p.has_value(),"Valid rear-axle path has no pose");return *p;
}
void straightAndAnalyticTurn() {
    const std::vector<Point> guide{{0,0},{20,0},{20,30}};
    const std::vector<RoutePart> parts{{"road",0,0,50}};
    RearAxlePath path(parts,car(),{{"road",guide}},{});
    const auto start=at(path,0,{0,0});near(start.rearAxle,{-3.6,0});near(start.frontAxle,{-.9,0});near(start.rearBumper,{-4.5,0});
    near(at(path,10,{10,0}).heading,{1,0});
    for(double s:{20.,20.1,21.,25.,30.,40.,50.}) {
        const auto p=at(path,s,pointAlong(guide,s));
        const double expected=std::numbers::pi/2-2*std::atan(std::exp(-(s-20)/3.6));
        near(angle(p.heading),expected,2e-7);
        near(std::hypot(p.frontAxle.x-p.rearAxle.x,p.frontAxle.y-p.rearAxle.y),2.7);
        near(std::hypot(p.front.x-p.rearBumper.x,p.front.y-p.rearBumper.y),4.5);
    }
    // Forcing: the old arc-distance chord is observably different, not equivalent.
    const auto oldRear=pointAlong(guide,23-4.5);const Point oldChord{20-oldRear.x,3-oldRear.y};
    require(std::abs(angle(oldChord)-angle(at(path,23,{20,3}).heading))>.1,"Setup: chord and no-slip model not separated");
    const double s=25.05,h=.001;const auto p=at(path,s,pointAlong(guide,s));
    const auto before=at(path,s-h,pointAlong(guide,s-h)),after=at(path,s+h,pointAlong(guide,s+h));
    const Point velocity{(after.rearAxle.x-before.rearAxle.x)/(2*h),(after.rearAxle.y-before.rearAxle.y)/(2*h)};
    near(-velocity.x*p.heading.y+velocity.y*p.heading.x,0,2e-4); // rear lateral slip
    const auto repeat=at(path,s,pointAlong(guide,s));at(path,49,{20,29});at(path,0,{0,0});
    near(at(path,s,pointAlong(guide,s)).rearAxle,repeat.rearAxle,1e-12); // seeking/paint order
}
void circleOfftracking() {
    constexpr double radius=20;std::vector<Point> circle;
    for(int i=0;i<=1440;++i){const double a=i*2*std::numbers::pi/720;circle.push_back({radius*std::cos(a),radius*std::sin(a)});}
    const double length=polylineLength(circle),s=length-.2;
    const std::vector<RoutePart> parts{{"circle",0,0,length}};
    auto heavy=car();heavy.length=12;heavy.axles=VehicleAxles{6,2,4};
    double carRadius=0;
    for(const auto& type:{car(),heavy}) {
        RearAxlePath path(parts,type,{{"circle",circle}},{});
        const auto p=at(path,s,pointAlong(circle,s));const auto a=vehicleAxles(type);
        const double lever=a.wheelbase+a.frontOverhang,r=std::hypot(p.rearAxle.x,p.rearAxle.y);
        near(r,std::sqrt(radius*radius-lever*lever),.003);
        require(r<radius-.2,"Rear axle did not cut inside the circular guide");
        if(type.length==4.5)carRadius=r;else require(r<carRadius-1,"Long vehicle did not offtrack more than car");
    }
}
void connectorsSectionsAndConvergence() {
    Network n;n.links={{"a",{{-80,0},{0,0}},{{"a1",3.5}}},{"b",{{30,30},{30,100}},{{"b1",3.5}}}};
    const auto eq=connectorEquation(n,{"a","a1"},{"b","b1"});const double c=eq.arcStations.back();
    const std::vector<RoutePart> parts{{"a",0,0,80},{"curve",1,80,c},{"b",2,80+c,70}};
    const std::map<std::string,std::vector<Point>> geometry{{"a",{{-80,0},{0,0}}},{"curve",{{0,0},{30,30}}},{"b",{{30,30},{30,100}}}};
    const std::map<std::string,ConnectorEquation> equations{{"curve",eq}};
    RearAxlePath path(parts,car(),geometry,equations),fine(parts,car(),geometry,equations,.01);
    const auto front=[&](double d){return vehiclePose(parts,d,4.5,geometry,equations)->front;};
    for(double s:{80.,81.,90.123,80.+c,81.+c,90.+c}) {
        const auto p=at(path,s,front(s));near(p.front,front(s));
        near(angle(p.heading),angle(at(fine,s,front(s)).heading),3e-5);
    }
    for(double s:{80.,80.+c})near(at(path,s-1e-6,front(s-1e-6)).heading,at(path,s+1e-6,front(s+1e-6)).heading,1e-6);
    auto split=parts;split.erase(split.begin());split.insert(split.begin(),{{"a0",0,0,7.03},{"a1",1,7.03,72.97}});
    auto pieces=geometry;pieces["a0"]={{-80,0},{-72.97,0}};pieces["a1"]={{-72.97,0},{0,0}};
    RearAxlePath sectioned(split,car(),pieces,equations);
    near(at(sectioned,95.123,front(95.123)).heading,at(path,95.123,front(95.123)).heading,3e-5);
    // Missing and disconnected route inputs must not produce invented tracks.
    auto missing=geometry;missing.erase("a");RearAxlePath absent(parts,car(),missing,equations);
    require(!absent.pose(81,front(81)),"Missing upstream geometry produced a pose");
    auto gap=geometry;gap["b"]={{31,30},{31,100}};RearAxlePath disconnected(parts,car(),gap,equations);
    require(!disconnected.pose(81,front(81)),"Disconnected route produced a pose");
    RearAxlePath directionless({{"zero",0,0,10}},car(),{{"zero",{{0,0},{0,0}}}},{});
    require(!directionless.pose(1,{0,0}),"Directionless geometry produced a pose");
    auto invalid=car();invalid.axles->wheelbase=0;bool threw=false;
    try{RearAxlePath bad(parts,invalid,geometry,equations);}catch(const std::invalid_argument&){threw=true;}
    require(threw,"Zero wheelbase was accepted");
    RearAxlePath bounded(parts,car(),geometry,equations,1e-300);
    require(!bounded.pose(81,front(81)),"Pathological spatial step escaped memory bound");
}
void shortConnector() {
    const auto eq=makeConnectorEquation({Point{0,0},{1,0},{2,1},{2,2}});const double c=eq.arcStations.back();
    require(c<4.5,"Setup: Connector must be shorter than both bodies");
    const std::vector<RoutePart> parts{{"a",0,0,20},{"curve",1,20,c},{"b",2,20+c,20}};
    const std::map<std::string,std::vector<Point>> geometry{{"a",{{-20,0},{0,0}}},{"curve",{{0,0},{2,2}}},{"b",{{2,2},{2,22}}}};
    const std::map<std::string,ConnectorEquation> equations{{"curve",eq}};
    auto heavy=car();heavy.length=12;heavy.axles=VehicleAxles{6,2,4};
    for(const auto& type:{car(),heavy}) {
        RearAxlePath path(parts,type,geometry,equations);
        const auto point=[&](double s){return vehiclePose(parts,s,type.length,geometry,equations)->front;};
        for(double s:{20.,20.+c})near(at(path,s-1e-6,point(s-1e-6)).heading,at(path,s+1e-6,point(s+1e-6)).heading,1e-6);
        const auto p=at(path,21+c,point(21+c));
        require(p.rearAxle.x<1,"Long body rear axle must still be upstream of target centreline");
    }
}
}
int main(){try{straightAndAnalyticTurn();circleOfftracking();connectorsSectionsAndConvergence();shortConnector();
    std::cout<<"Rear axle tests passed: analytic turn, no-slip, offtracking, joins and convergence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
