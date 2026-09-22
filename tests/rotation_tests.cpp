#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/model/network/rotation.hpp"
#include <limits>
using namespace trafficsim;
namespace {
ProjectDocument junction(DrivingSide side) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={{"a",{{0,0},{30,0},{80,10}},{{"a1",3},{"a2",4},{"a3",3.5}}},
                     {"b",{{100,20},{140,20},{160,60}},{{"b1",4},{"b2",3},{"b3",3.5}}},
                     {"other",{{200,150},{300,150}},{{"other1",3.5}}}};
    d.network.links[0].laneOffset=2;d.network.links[1].laneOffset=-3;
    d.network.links[0].name="Approach";d.network.links[0].level=2;
    d.network.links[0].boundaryMarkings={MarkingType::solid,MarkingType::dashed,MarkingType::none,MarkingType::doubleLine};
    const auto id=addConnectorRange(d,{"a","a1",40},{"b","b1",25},3,2);
    auto& c=d.network.connectors.front();c.name="Turn";c.level=2;
    c.geometry[2].y+=4;c.laneBlend=connectorBlendWeights(c);
    changeConnectorLanes(d,id,{3.1,3.6,4.1},{MarkingType::dashed,MarkingType::doubleLine});
    const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
    putSignalHead(d,{"head-a",{"a","a1"},10,program,{}});
    putSignalHead(d,{"head-c",{},5,program,connectorPathId(c,0)});
    putSignalHead(d,{"head-other",{"other","other1"},20,program,{}});
    const auto route=putRoute(d,{"",{"a1",connectorPathId(c,0),"b1"}});
    putInput(d,{"",route,"car",600,0,60});
    validateDocument(d);return d;
}
// Independent oracle: the rigid rotation must preserve every derived lane, boundary and head.
Point expected(Point p,Point pivot,double degrees) {
    const double radians=degrees*std::acos(-1.)/180.;
    return {pivot.x+(p.x-pivot.x)*std::cos(radians)-(p.y-pivot.y)*std::sin(radians),
            pivot.y+(p.x-pivot.x)*std::sin(radians)+(p.y-pivot.y)*std::cos(radians)};
}
void near(Point a,Point b) {test::near(a.x,b.x,2e-7);test::near(a.y,b.y,2e-7);}
void rigid(const std::vector<Point>& before,const std::vector<Point>& after,Point pivot,double degrees) {
    CHECK(before.size()==after.size());
    for(std::size_t i=0;i<before.size();++i)near(after[i],expected(before[i],pivot,degrees));
    test::near(polylineLength(before),polylineLength(after),2e-7);
}
}
TEST(rotation, a_group_keeps_its_geometry_metadata_references_and_heads) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(double degrees:{90.,-90.,180.,37.5,-171.25}) {
        const auto before=junction(side);History h;h.reset(before);
        const auto& old=before.network;const auto id=old.connectors.front().id;
        CHECK(old.connectors[0].geometry.size()>2 && !old.connectors[0].laneBlend.empty());
        CHECK(before.definition->routes.size()==1 && before.definition->inputs.size()==1);
        const auto objects=rotationObjects(old,{"b",id,"a","a","head-other"});
        CHECK(objects==std::vector<std::string>({"a","b",id,"head-a","head-c"}));
        // Explicitly selecting the automatic Connector must not rotate it twice.
        const Point pivot{1000000.25,-900000.5};
        CHECK(h.execute("rotate",[&](auto& d){rotateObjects(d,{"a","b",id,"head-other"},pivot,degrees);}));
        const auto after=h.document();const auto& n=after.network;
        CHECK(n.connectors.size()==1 && n.links[2]==old.links[2]);
        for(std::size_t i=0;i<2;++i) {
            rigid(old.links[i].geometry,n.links[i].geometry,pivot,degrees);
            auto metadata=n.links[i];metadata.geometry=old.links[i].geometry;CHECK(metadata==old.links[i]);
            for(const auto& lane:old.links[i].lanes)
                rigid(laneGeometry(old.links[i],lane.id,side),laneGeometry(n.links[i],lane.id,side),pivot,degrees);
        }
        rigid(old.connectors[0].geometry,n.connectors[0].geometry,pivot,degrees);
        auto metadata=n.connectors[0];metadata.geometry=old.connectors[0].geometry;CHECK(metadata==old.connectors[0]);
        const auto oldPaths=connectorPaths(old,old.connectors[0]),paths=connectorPaths(n,n.connectors[0]);
        for(std::size_t i=0;i<paths.size();++i)rigid(oldPaths[i].geometry,paths[i].geometry,pivot,degrees);
        const auto oldBounds=connectorBoundaries(old,old.connectors[0]),bounds=connectorBoundaries(n,n.connectors[0]);
        for(std::size_t i=0;i<bounds.size();++i)rigid(oldBounds[i],bounds[i],pivot,degrees);
        CHECK(n.signalHeads==old.signalHeads && after.definition==before.definition);
        near(pointAlong(laneGeometry(n.links[0],"a1",side),10),
             expected(pointAlong(laneGeometry(old.links[0],"a1",side),10),pivot,degrees));
        near(pointAlong(paths[0].geometry,5),expected(pointAlong(oldPaths[0].geometry,5),pivot,degrees));
        CHECK(parseDocument(Json::parse(documentJson(after).dump()))==after);
        CHECK(h.states().size()==2);h.undo();CHECK(h.document()==before);h.redo();CHECK(h.document()==after);
    }
}
TEST(rotation, partial_rotation_cleans_detached_dependants_and_undo_restores_them) {
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        const auto before=junction(side);const auto id=before.network.connectors[0].id;
        for(const auto& selection:std::vector<std::vector<std::string>>{{"a"},{id}}) {
            History h;h.reset(before);
            // The chosen end leaves both the old lane and the whole carriageway.
            auto forced=before.network;const Point pivot{-100,-100};
            if(selection[0]=="a")for(auto& p:forced.links[0].geometry)p=expected(p,pivot,90);
            else for(auto& p:forced.connectors[0].geometry)p=expected(p,pivot,90);
            CHECK(!reanchorConnector(forced,forced.connectors[0]));
            h.execute("rotate",[&](auto& d){rotateObjects(d,selection,pivot,90);});
            CHECK(h.document().network.connectors.empty());
            CHECK(h.document().network.signalHeads.size()==2);
            CHECK(h.document().definition->routes.empty() && h.document().definition->inputs.empty());
            CHECK(h.document().network.links[1]==before.network.links[1]);
            const auto after=h.document();h.undo();CHECK(h.document()==before);h.redo();CHECK(h.document()==after);
        }
    }
}
TEST(rotation, a_stationary_connector_survives_when_the_rotated_lane_still_supports_it) {
    ProjectDocument d;d.network.links={{"a",{{0,0},{100,0}},{{"a1",4}}},
                                     {"b",{{120,0},{220,0}},{{"b1",4}}}};
    addConnector(d,{"a","a1",50},{"b","b1",50});History h;h.reset(d);
    const auto before=h.document();const auto pivot=before.network.connectors[0].geometry.front();
    h.execute("rotate",[&](auto& m){rotateObjects(m,{"a"},pivot,90);});
    CHECK(h.document().network.connectors==before.network.connectors);
    CHECK(h.document().network.links[0]!=before.network.links[0]);
    h.undo();CHECK(h.document()==before);
}
TEST(rotation, invalid_inputs_and_full_turns_leave_history_and_saved_state_untouched) {
    History h;h.reset(junction(DrivingSide::left));
    h.execute("rotate",[](auto& d){rotateObjects(d,{"a","b"},{},15);});h.undo();
    const auto before=h.document();CHECK(h.canRedo() && !h.dirty());
    for(double degrees:{0.,360.,-360.,720.})
        CHECK(!h.execute("no-op",[&](auto& d){rotateObjects(d,{"a","b"},{},degrees);}));
    const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
    for(double bad:{nan,inf,-inf}) {
        test::throws([&]{h.execute("bad angle",[&](auto& d){rotateObjects(d,{"a"},{},bad);});},"INVALID_GEOMETRY");
        test::throws([&]{h.execute("bad pivot",[&](auto& d){rotateObjects(d,{"a"},{bad,0},30);});},"INVALID_GEOMETRY");
    }
    test::throws([&]{h.execute("overflow",[](auto& d){rotateObjects(d,{"a"},{1e308,1e308},180);});},"INVALID_GEOMETRY");
    for(const auto& ids:std::vector<std::vector<std::string>>{{},{"head-a"}})
        test::throws([&]{h.execute("empty",[&](auto& d){rotateObjects(d,ids,{},30);});},"EDIT_ROTATE_TARGET");
    test::throws([&]{h.execute("stale",[](auto& d){rotateObjects(d,{"a","missing"},{},30);});},"EDIT_UNKNOWN_OBJECT");
    CHECK(h.document()==before && h.canRedo() && !h.dirty());CHECK(h.states().size()==2);
    CHECK(!rotationCentre(before.network,{"head-a"}));
    Network plain;plain.links={{"a",{{0,0},{100,0}},{{"a1",4}}}};
    plain.links[0].laneOffset=3;
    CHECK(rotationCentre(plain,{"a"})==Point{50,3});
    plain.drivingSide=DrivingSide::right;CHECK(rotationCentre(plain,{"a"})==Point{50,-3});
    CHECK(rotatePoint({3,4},{1,2},90)==Point{-1,4});
    CHECK(rotatePoint({3,4},{1,2},-90)==Point{3,0});
    CHECK(rotatePoint({3,4},{1,2},180)==Point{-1,0});
}
