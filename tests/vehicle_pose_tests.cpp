#include "../src/editor/vehicle_pose.hpp"
#include "../src/core/routes.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace trafficsim;
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void near(Point a,Point b,double tolerance=1e-7) {
    require(std::isfinite(a.x) && std::isfinite(a.y) &&
        std::hypot(a.x-b.x,a.y-b.y)<=tolerance,"Position differs from the independent route sample");
}
double angle(Point p){return std::atan2(p.y,p.x)*180/std::acos(-1.);}
void sameHeading(Point a,Point b,double tolerance=1e-7) {
    require(std::abs(std::remainder(angle(a)-angle(b),360.))<=tolerance,"Heading differs");
}
struct Fixture {
    std::vector<RoutePart> parts;
    std::map<std::string,std::vector<Point>> geometry;
    std::map<std::string,ConnectorEquation> equations;
    double curveLength{};
    Fixture() {
        Network n;n.links={{"a",{{-80,0},{0,0}},{{"a1",3.5}}},
                           {"b",{{30,30},{30,100}},{{"b1",3.5}}}};
        const auto curve=connectorEquation(n,{"a","a1"},{"b","b1"});
        curveLength=curve.arcStations.back();
        parts={{"a1",0,0,80},{"curve",1,80,curveLength},{"b1",2,80+curveLength,70}};
        geometry={{"a1",{{-80,0},{0,0}}},{"curve",{{0,0},{30,30}}},{"b1",{{30,30},{30,100}}}};
        equations={{"curve",curve}};
    }
    // Independent piecewise coordinates, not the production route lookup.
    Point point(double d) const {
        if(d<=80)return {d-80,0};
        if(d<=80+curveLength)return equationPoint(equations.at("curve"),equationParameter(equations.at("curve"),d-80));
        return {30,30+std::min(70.,d-80-curveLength)};
    }
    VehiclePose pose(double d,double length) const {
        const auto result=vehiclePose(parts,d,length,geometry,equations);
        require(result.has_value(),"Valid route has no pose");return *result;
    }
};
void joinsAndLengthThreshold() {
    Fixture f;
    for(double length:{4.5,12.})for(double boundary:{80.,80.+length,80.+f.curveLength,80.+f.curveLength+length}) {
        for(double offset:{-1e-6,0.,1e-6}) {
            const double d=boundary+offset;const auto pose=f.pose(d,length);
            const auto front=f.point(d),rear=f.point(d-length);
            near(pose.front,front);sameHeading(pose.heading,{front.x-rear.x,front.y-rear.y});
        }
        sameHeading(f.pose(boundary-1e-6,length).heading,f.pose(boundary+1e-6,length).heading,1e-4);
    }
    // Ensure this fixture exposes the OLD tangent fallback at a real segment exit.
    for(double length:{4.5,12.}) {
        const auto pose=f.pose(80+f.curveLength,length);
        require(std::abs(angle(pose.heading)-90)>4,"Setup: old tangent fallback would not fail");
    }
}
void routeEntryAndShortSegments() {
    Fixture f;
    for(double length:{4.5,12.})for(double d:{0.,1e-6,length-1e-6,length,length+1e-6}) {
        const auto pose=f.pose(d,length);near(pose.front,f.point(d));sameHeading(pose.heading,{1,0});
    }
    // A 12 m vehicle spans the short Connector and an additional Link section.
    const auto curve=makeConnectorEquation({Point{0,0},{1,0},{2,1},{2,2}});
    const double c=curve.arcStations.back();require(c<4.5,"Setup: Connector is not short");
    std::vector<RoutePart> parts{{"a",0,0,20},{"curve",1,20,c},{"b1",2,20+c,2},{"b2",3,22+c,20}};
    std::map<std::string,std::vector<Point>> geometry{{"a",{{-20,0},{0,0}}},{"curve",{{0,0},{2,2}}},
        {"b1",{{2,2},{2,4}}},{"b2",{{2,4},{2,24}}}};
    const auto pose=vehiclePose(parts,23+c,12,geometry,{{"curve",curve}});
    require(pose.has_value(),"Short-segment route has no pose");near(pose->front,{2,5});
    sameHeading(pose->heading,{2-(11+c-20),5});
}
void sectionsAndRepeatedIds() {
    const std::vector<Point> bent{{0,0},{10,0},{10,10},{20,10}};
    const std::map<std::string,std::vector<Point>> whole{{"lane",bent}};
    const std::vector<RoutePart> uncut{{"lane",0,0,30}};
    const std::vector<RoutePart> cut{{"s1",0,0,7},{"s2",1,7,8},{"s3",2,15,15}};
    const std::map<std::string,std::vector<Point>> pieces{{"s1",polylineSpan(bent,0,7)},
        {"s2",polylineSpan(bent,7,15)},{"s3",polylineSpan(bent,15,30)}};
    for(double length:{4.5,12.})for(double d:{0.,7.,9.,10.,15.,18.,20.,25.,30.}) {
        auto a=vehiclePose(uncut,d,length,whole,{}),b=vehiclePose(cut,d,length,pieces,{});
        require(a && b,"Sectioned route has no pose");near(a->front,b->front);sameHeading(a->heading,b->heading);
    }
    // A route returns to the same segment; resolve the occurrence by route distance.
    const std::map<std::string,std::vector<Point>> loop{{"a",{{0,0},{10,0}}},{"back",{{10,0},{0,0}}}};
    const std::vector<RoutePart> repeats{{"a",0,0,10},{"back",1,10,10},{"a",0,20,10}};
    auto p=vehiclePose(repeats,21,4.5,loop,{});require(p.has_value(),"Repeated segment has no pose");
    near(p->front,{1,0});sameHeading(p->heading,{-1,0});
}
void endpointAndDegenerateCases() {
    Fixture f;const double end=150+f.curveLength;
    near(f.pose(end,4.5).front,{30,100});sameHeading(f.pose(end,4.5).heading,{0,1});
    auto missing=f.geometry;missing.erase("a1");
    require(!vehiclePose(f.parts,81,12,missing,f.equations),"Missing rear geometry must not draw a false pose");
    require(!vehiclePose({},0,4.5,f.geometry,f.equations),"Empty route must not be dereferenced");
    const std::vector<RoutePart> loop{{"loop",0,0,40}};
    const std::map<std::string,std::vector<Point>> g{{"loop",{{0,0},{10,0},{10,10},{0,10},{0,0}}}};
    auto p=vehiclePose(loop,40,40,g,{});require(p.has_value(),"Coincident chord endpoints need a tangent fallback");
    sameHeading(p->heading,{0,-1});
    for(double length:{0.,-1.,static_cast<double>(INFINITY)}) {
        bool threw=false;try{vehiclePose(f.parts,80,length,f.geometry,f.equations);}catch(const std::invalid_argument&){threw=true;}
        require(threw,"Invalid length was accepted");
    }
    for(double distance:{-1.,static_cast<double>(NAN),static_cast<double>(INFINITY)}) {
        bool threw=false;try{vehiclePose(f.parts,distance,4.5,f.geometry,f.equations);}catch(const std::invalid_argument&){threw=true;}
        require(threw,"Invalid distance was accepted");
    }
}
void laneCentresAndInternalAttachments() {
    Network n;n.links={{"a",{{-80,0},{0,0}},{{"a1",3.5},{"a2",3.5},{"a3",3.5}}},
                       {"b",{{30,30},{30,100}},{{"b1",3.5},{"b2",3.5},{"b3",3.5}}}};
    for(auto side:{DrivingSide::left,DrivingSide::right})for(int lane=1;lane<=3;++lane) {
        n.drivingSide=side;
        LaneReference from{"a","a"+std::to_string(lane),40},to{"b","b"+std::to_string(lane),20};
        const auto curve=connectorEquation(n,from,to);const double c=curve.arcStations.back();
        const auto a=polylineSpan(laneGeometry(n.links[0],from.laneId,side),0,40);
        const auto b=polylineSpan(laneGeometry(n.links[1],to.laneId,side),20,70);
        const std::vector<RoutePart> parts{{"source",0,0,40},{"curve",1,40,c},{"target",2,40+c,50}};
        const std::map<std::string,std::vector<Point>> geometry{{"source",a},{"curve",{a.back(),b.front()}},{"target",b}};
        const auto point=[&](double d) {
            if(d<=40)return pointAlong(a,d);
            if(d<=40+c)return equationPoint(curve,equationParameter(curve,d-40));
            return pointAlong(b,d-40-c);
        };
        for(double length:{4.5,12.})for(double d:{40.,41.,40.+length,40.+c,41.+c}) {
            const auto pose=vehiclePose(parts,d,length,geometry,{{"curve",curve}});
            require(pose.has_value(),"Lane-centre route has no pose");const auto front=point(d),rear=point(d-length);
            near(pose->front,front);sameHeading(pose->heading,{front.x-rear.x,front.y-rear.y});
        }
    }
}
}
int main() {
    try {joinsAndLengthThreshold();routeEntryAndShortSegments();sectionsAndRepeatedIds();endpointAndDegenerateCases();laneCentresAndInternalAttachments();
        std::cout<<"Vehicle pose tests passed: joins, route entry, short sections, repeated IDs and fallbacks\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
