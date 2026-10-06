#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/model/network/conflict_display.hpp"
#include "../src/model/network/connector_surface.hpp"
#include <algorithm>
#include <cmath>
using namespace trafficsim;
namespace {
bool inside(const std::vector<Point>& p,Point q) {
    bool result=false;
    for(std::size_t i=0,j=p.size()-1;i<p.size();j=i++)
        if((p[i].y>q.y)!=(p[j].y>q.y) && q.x<(p[j].x-p[i].x)*(q.y-p[i].y)/(p[j].y-p[i].y)+p[i].x)result=!result;
    return result;
}
ControlPathRef ref(const Connector& c){return {"","",c.id,c.from.laneId,c.to.laneId};}
std::string yielding(const ConflictArea& a) {
    return conflictOwner(a.priority==ConflictPriority::firstYields?a.first.path:a.second.path);
}
}
TEST(conflict_geometry, rail_offsets_are_half_a_metre_normal_to_an_oblique_lane) {
    for(const auto handed:{DrivingSide::left,DrivingSide::right})for(const double width:{3.5,0.5}) {
        ProjectDocument d;d.network.drivingSide=handed;
        const auto id=addLink(d,{{0,0},{100,100}},1,width);const auto& l=d.network.links.front();
        const ConflictSide s{{id,l.lanes.front().id,"","",""},0,polylineLength(l.geometry),""};
        const auto original=conflictSideOutline(d.network,s),band=conflictBandOutline(d.network,s);
        CHECK(original.size()==4 && band.size()==4);
        const auto distance=[](Point p,Point a,Point b) {return std::abs((b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x))/std::hypot(b.x-a.x,b.y-a.y);};
        const double expected=std::min(0.5,width*0.2);
        test::near(distance(band[0],original[0],original[1]),expected,1e-8);
        test::near(distance(band[3],original[3],original[2]),expected,1e-8);
        test::near(std::hypot(band[0].x-band[3].x,band[0].y-band[3].y),width-2*expected,1e-8);
    }
}
TEST(conflict_geometry, mouth_bands_follow_the_attached_link_lane_through_p3_p4) {
    for(const auto handed:{DrivingSide::left,DrivingSide::right}) {
        ProjectDocument d;d.network.drivingSide=handed;
        const auto a=addLink(d,{{0,30},{200,30}},1,3.5),b=addLink(d,{{0,0},{200,0}},1,3.5);
        const auto la=d.network.links[0].lanes.front().id,lb=d.network.links[1].lanes.front().id;
        const auto id=addConnector(d,{a,la,100},{b,lb,100});
        changeConnectorGeometry(d,id,{{100,30},{100,0}});
        const auto& c=d.network.connectors.front();const auto surface=connectorSurface(d.network,c);
        CHECK(surface.source && surface.target && !surface.selfIntersecting);
        for(const bool source:{true,false}) {
            const auto parts=conflictMouthBands(d.network,ref(c),source,!source);
            CHECK(!parts.empty());const auto& link=d.network.links[source?0:1];
            const auto& mouth=*(source?surface.source:surface.target);
            const double farStation=stationOfClosestPoint(link.geometry,mouth.points[3]);
            const double nearStation=stationOfClosestPoint(link.geometry,mouth.points[2]);
            CHECK(std::abs(farStation-nearStation)>0.1);
            bool covers=false;
            for(const auto& part:parts) {
                CHECK(part.lane.path.linkId==link.id && !part.clips.empty());
                CHECK(part.lane.entryStation>=0 && part.lane.exitStation<=polylineLength(link.geometry));
                const double at=(nearStation+farStation)/2;
                if(part.lane.entryStation<=at && at<=part.lane.exitStation) {
                    const auto lane=laneGeometry(link,part.lane.path.laneId,handed);
                    const auto car=pointAlong(lane,matchedStation(link.geometry,lane,at));
                    CHECK(inside(conflictBandOutline(d.network,part.lane),car));covers=true;
                }
            }
            CHECK(covers);
        }
        const auto runtime=connectorPaths(d.network,c).front();
        CHECK(runtime.geometry.front()==laneAttachment(d.network,c.from,true));
        CHECK(runtime.geometry.back()==laneAttachment(d.network,c.to,false));
        const auto before=conflictMouthBands(d.network,ref(c),false,true);
        History h;h.reset(d);
        CHECK(h.execute("mouth",[&](auto& candidate){changeConnectorGeometry(candidate,id,{{100,30},{102,15},{100,0}});}));
        const auto after=conflictMouthBands(h.document().network,ref(h.document().network.connectors.front()),false,true);
        CHECK(!after.empty());CHECK(after.front().lane.entryStation!=before.front().lane.entryStation || after.front().lane.exitStation!=before.front().lane.exitStation);
        h.undo();CHECK(h.document()==d);
        test::near(conflictMouthBands(h.document().network,ref(c),false,true).front().lane.exitStation,before.front().lane.exitStation);
    }
}
TEST(conflict_geometry, one_connected_owner_pair_can_group_all_three_kinds_without_authoring_branching) {
    ProjectDocument d;
    const auto road=addLink(d,{{0,0},{200,0}},3,3.5);
    const auto from=d.network.links.front().lanes.front().id,to=d.network.links.front().lanes.back().id;
    const auto connector=addConnector(d,{road,from,100},{road,to,110});
    const auto& c=d.network.connectors.front();
    changeConnectorGeometry(d,connector,{laneAttachment(d.network,c.from,true),laneAttachment(d.network,c.to,false)});
    const auto automatic=automaticConflicts(d.network);
    const auto groups=conflictGroups(d.network,automatic);
    CHECK(groups.size()==1);const auto group=groups.front();
    CHECK(group.kinds.size()==3 && group.kind!=ConflictKind::branching);
    const auto branches=std::count_if(automatic.begin(),automatic.end(),[](const auto& a){return a.kind==ConflictKind::branching;});
    CHECK(branches>0);
    const auto id=authorConflictGroup(d,group.key,{3,7});
    CHECK(d.network.rightOfWay.conflictAreas.size()==automatic.size()-static_cast<std::size_t>(branches));
    for(const auto& a:d.network.rightOfWay.conflictAreas)CHECK(a.kind!=ConflictKind::branching);
    const auto representative=d.network.rightOfWay.conflictAreas.front();
    const auto linkYields=conflictOwner(representative.first.path)==road?ConflictPriority::firstYields:ConflictPriority::secondYields;
    setConflictGroupControl(d,id,"site",linkYields,4,8);
    for(const auto& a:d.network.rightOfWay.conflictAreas)CHECK(yielding(a)==road);
    const auto remaining=automaticConflicts(d.network);
    CHECK(std::count_if(remaining.begin(),remaining.end(),[](const auto& a){return a.kind==ConflictKind::branching;})==branches);
    CHECK(resolveRightOfWay(d.network,runtimeSections(d.network),{3,7}).issues.empty());
    const auto before=d;History h;h.reset(d);
    CHECK(h.execute("cycle",[&](auto& candidate){cycleConflictGroupPriority(candidate,id,{3,7});}));
    h.undo();CHECK(h.document()==before);h.redo();
    for(const auto& a:h.document().network.rightOfWay.conflictAreas)CHECK(a.priority==ConflictPriority::undetermined);
}
TEST(conflict_geometry, a_group_priority_cycle_refuses_without_partial_changes) {
    auto t=rowfixture::threeWayMerge();takeOverMerge(t.d,rowfixture::mergeAt(t.d,3),{3,7});
    CHECK(t.d.network.rightOfWay.conflictAreas.size()==3);
    const auto a=t.d.network.rightOfWay.conflictAreas[1];const auto before=t.d;
    test::throws([&]{setConflictGroupControl(t.d,a.id,"cycle",ConflictPriority::secondYields,3,7);},"CONFLICT_PRIORITY_CYCLE");
    CHECK(t.d==before);
}
