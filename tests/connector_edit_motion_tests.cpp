#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <algorithm>
#include <numeric>
using namespace trafficsim;
namespace {
Point midpoint(Point a,Point b) { return {std::midpoint(a.x,b.x),std::midpoint(a.y,b.y)}; }
void near(Point a,Point b) { test::near(a.x,b.x,1e-8);test::near(a.y,b.y,1e-8); }
ProjectDocument turn(DrivingSide side,int from,int to) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={{"a",{{-80,0},{0,0}},{{"a1",3.5},{"a2",3.5},{"a3",3.5}}},
                     {"b",{{30,30},{30,100}},{{"b1",3.5},{"b2",3.5},{"b3",3.5}}}};
    addConnectorRange(d,{"a","a1"},{"b","b1"},from,to);
    return d;
}
void followsRails(const Network& n) {
    const auto& c=n.connectors.front();const auto rails=connectorBoundaries(n,c);
    const auto paths=connectorPaths(n,c);
    for(std::size_t k=0;k<paths.size();++k) {
        const auto& p=paths[k];CHECK(!p.equation);
        // Independent expected path: painted rail midpoints and named lane attachments.
        std::vector<Point> expected{laneAttachment(n,p.from,true)};
        for(std::size_t j=1;j+1<c.geometry.size();++j)expected.push_back(midpoint(rails[k][j],rails[k+1][j]));
        expected.push_back(laneAttachment(n,p.to,false));
        test::near(connectorPathLength(p),polylineLength(expected),1e-8);
        double at=0;
        for(std::size_t j=1;j<expected.size();++j) {
            const double leg=std::hypot(expected[j].x-expected[j-1].x,expected[j].y-expected[j-1].y);
            near(connectorPathPoint(p,at+leg*.37),
                {expected[j-1].x+.37*(expected[j].x-expected[j-1].x),expected[j-1].y+.37*(expected[j].y-expected[j-1].y)});
            at+=leg;
        }
    }
}
}
TEST(lane_centres, interior_edit_changes_motion_with_fixed_attachments_and_round_trips) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(auto counts:{std::pair{3,3},std::pair{3,2},std::pair{2,3}}) {
        auto d=turn(side,counts.first,counts.second);const auto c=d.network.connectors.front();
        const auto before=connectorPaths(d.network,c);auto geometry=c.geometry;
        geometry[2].x+=5;geometry[2].y-=4;
        CHECK(geometry.front()==c.geometry.front() && geometry.back()==c.geometry.back());
        History history;history.reset(d);
        CHECK(history.execute("reshape",[&](auto& m){changeConnectorGeometry(m,c.id,geometry);}));
        const auto& edited=history.document();const auto& changed=edited.network.connectors.front();
        CHECK(changed.from==c.from && changed.to==c.to);
        const auto after=connectorPaths(edited.network,changed);
        for(std::size_t k=0;k<after.size();++k) {
            CHECK(std::hypot(after[k].geometry[2].x-before[k].geometry[2].x,after[k].geometry[2].y-before[k].geometry[2].y)>1);
            CHECK(std::abs(connectorPathLength(after[k])-connectorPathLength(before[k]))>1e-4);
        }
        followsRails(edited.network);
        auto definition=static_cast<ScenarioDefinition>(test::straight());definition.routes={{"r",{"a",c.id,"b"}}};
        definition.inputs={{"i","r","car",900,0,20}};definition.priorityDefaults={3,1};
        const auto scenario=compileScenario(edited.network,definition);
        for(const auto& p:after) {
            const auto s=std::find_if(scenario.segments.begin(),scenario.segments.end(),[&](const auto& s){return s.id==p.id;});
            CHECK(s!=scenario.segments.end());test::near(s->length,polylineLength(p.geometry),1e-8);
        }
        const auto reopened=parseDocument(documentJson(edited));followsRails(reopened.network);
        const auto saved=edited;history.undo();CHECK(history.document()==d);followsRails(history.document().network);
        history.redo();CHECK(history.document()==saved);followsRails(history.document().network);
    }
}
TEST(lane_centres, head_and_waiting_control_share_the_edited_lane_path) {
    auto d=turn(DrivingSide::right,3,3);auto& c=d.network.connectors.front();
    auto geometry=c.geometry;geometry[2].x+=5;geometry[2].y-=4;changeConnectorGeometry(d,c.id,geometry);
    const auto paths=connectorPaths(d.network,c);const auto table=runtimeSections(d.network);
    for(const auto& path:paths) {
        const double station=polylineLength(c.geometry)*.47;
        const ControlPoint control{{"","",c.id,path.from.laneId,path.to.laneId},station};
        const auto located=locateControlPoint(d.network,table,control);CHECK(located && located->segment==path.id);
        const double expected=matchedStation(c.geometry,path.geometry,station);
        test::near(located->position,expected,1e-9);
        test::near(connectorAuthoringStation(c,path,expected),station,1e-8);
        const auto front=pointAlong(path.geometry,expected);
        const auto head=nearestHeadSlot(d.network,front,0);CHECK(head && head->slot.connectorId==path.id);
        test::near(head->station,expected,1e-7);
        const auto bar=waitingLineBar(d.network,control);CHECK(bar);
        const auto direction=directionAlong(path.geometry,expected,true);
        for(Point p:{bar->first,bar->second})test::near((p.x-front.x)*direction.x+(p.y-front.y)*direction.y,0,1e-8);
        const auto offset=offsetControlStation(d.network,control.path,station,-1);
        test::near(controlStationDistance(d.network,control.path,offset,station),1,1e-8);
    }
}
