#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/model/network/conflict_surface.hpp"
#include <algorithm>
using namespace trafficsim;
namespace {
const PriorityDefaults defaults{3,7};
std::string lane(const ProjectDocument& d,const std::string& id) {for(const auto& l:d.network.links)if(l.id==id)return l.lanes.front().id;throw std::runtime_error("lane");}
ControlPathRef connectorRef(const Connector& c){return {"","",c.id,c.from.laneId,c.to.laneId};}
int pairCount(const Network& n,const std::string& a,const std::string& b) {
    int count=0;for(const auto& c:automaticConflicts(n)) {
        const auto first=conflictOwner(c.first.path),second=conflictOwner(c.second.path);
        count+=(first==a && second==b)||(first==b && second==a);
    }return count;
}
ProjectDocument curved(int points) {
    ProjectDocument d;d.network.drivingSide=DrivingSide::right;
    const auto a=addLink(d,{{-100,0},{0,0}},1,3.5),b=addLink(d,{{100,100},{100,200}},1,3.5);
    addLink(d,{{40,-5},{40,30}},1,1);
    addConnector(d,{a,lane(d,a)},{b,lane(d,b)});
    auto& c=d.network.connectors.front();c.geometry=connectorCurve(d.network,c.from,c.to,points);return d;
}
std::pair<double,double> runtimeExtent(const ProjectDocument& d) {
    const auto& c=d.network.connectors.front();const auto cp=connectorPaths(d.network,c).front();
    const auto& l=d.network.links.back();const ControlPathRef road{l.id,l.lanes.front().id,"","",""};
    const auto overlaps=surfaceOverlaps(d.network,connectorRef(c),road);
    CHECK(overlaps.size()==1 && overlaps.front().status==SurfaceOverlap::Status::overlap);
    return {connectorRuntimeStation(c,cp,overlaps[0].first.from),connectorRuntimeStation(c,cp,overlaps[0].first.to)};
}
}
TEST(conflict_geometry, runtime_overlap_is_invariant_under_point_count_and_interior_drag) {
    const auto d=curved(0);const auto expected=runtimeExtent(d);
    CHECK(pairCount(d.network,d.network.connectors.front().id,d.network.links.back().id)==1);
    for(int count:{3,40}) {
        auto other=curved(count);auto extent=runtimeExtent(other);
        test::near(extent.first,expected.first,1e-8);test::near(extent.second,expected.second,1e-8);
        other.network.connectors.front().geometry[1].y+=50;
        extent=runtimeExtent(other);test::near(extent.first,expected.first,1e-8);test::near(extent.second,expected.second,1e-8);
        CHECK(pairCount(other.network,other.network.connectors.front().id,other.network.links.back().id)==1);
    }
}
TEST(conflict_geometry, a_wide_connector_is_not_dropped_by_a_four_metre_prefilter) {
    ProjectDocument d;const auto a=addLink(d,{{-20,0},{0,0}},1,20),b=addLink(d,{{100,0},{120,0}},1,20);
    const auto other=addLink(d,{{20,8},{80,8}},1,1);
    const auto c=addConnector(d,{a,lane(d,a)},{b,lane(d,b)});
    const auto overlap=surfaceOverlaps(d.network,connectorRef(d.network.connectors.front()),{other,lane(d,other),"","",""});
    CHECK(overlap.front().status==SurfaceOverlap::Status::overlap);
    CHECK(validateNetwork(d.network).empty() && connectorRuntimeIssues(d.network).empty());
    CHECK(pairCount(d.network,c,other)==1);
}
TEST(conflict_geometry, sharing_a_lane_at_distinct_stations_does_not_hide_a_crossing) {
    ProjectDocument d;d.network.drivingSide=DrivingSide::right;
    // Large scale keeps both curved offsets regular, including the second path's tight turn.
    const auto source=addLink(d,{{0,0},{2000,0}},1,3.5),a=addLink(d,{{1400,1000},{1000,1400}},1,3.5),b=addLink(d,{{600,1000},{1000,1400}},1,3.5);
    const auto ca=addConnector(d,{source,lane(d,source),600},{a,lane(d,a)}),cb=addConnector(d,{source,lane(d,source),1400},{b,lane(d,b)});
    CHECK(validateNetwork(d.network).empty() && connectorRuntimeIssues(d.network).empty());
    CHECK(surfaceOverlaps(d.network,connectorRef(d.network.connectors[0]),connectorRef(d.network.connectors[1])).front().status==SurfaceOverlap::Status::overlap);
    CHECK(pairCount(d.network,ca,cb)==1);
    auto reverse=d;
    for(auto& l:reverse.network.links)std::reverse(l.geometry.begin(),l.geometry.end());
    for(auto& c:reverse.network.connectors) {
        std::swap(c.from,c.to);if(c.to.station)c.to.station=2000-*c.to.station;
        c.geometry=connectorCurve(reverse.network,c.from,c.to,20);
    }
    CHECK(validateNetwork(reverse.network).empty() && connectorRuntimeIssues(reverse.network).empty());
    CHECK(pairCount(reverse.network,ca,cb)==1);
}
TEST(conflict_geometry, nine_pairs_are_one_group_with_atomic_author_edit_delete_and_roundtrip) {
    ProjectDocument d;addLink(d,{{0,0},{200,0}},3,3.5);addLink(d,{{100,-60},{100,60}},3,3.5);
    const auto automatic=automaticConflicts(d.network);CHECK(automatic.size()==9);
    const auto groups=conflictGroups(d.network,automatic);CHECK(groups.size()==1 && groups[0].automaticKeys.size()==9);
    History h;h.reset(d);std::string id;
    CHECK(h.execute("author",[&](auto& d){id=authorConflictGroup(d,groups[0].key,defaults);}));
    CHECK(h.document().network.rightOfWay.conflictAreas.size()==9);
    CHECK(h.document().network.rightOfWay.waitingLines.size()==6);
    CHECK(automaticConflicts(h.document().network).empty());
    CHECK(resolveRightOfWay(h.document().network,runtimeSections(h.document().network),defaults).zones.size()==9);
    h.undo();CHECK(h.document()==d);h.redo();
    CHECK(h.execute("edit",[&](auto& d){setConflictGroupControl(d,id,"junction",ConflictPriority::firstYields,4,10);setConflictGroupStopControl(d,id,StopMode::yield);}));
    for(const auto& a:h.document().network.rightOfWay.conflictAreas)CHECK(a.name=="junction");
    for(const auto& r:h.document().network.rightOfWay.priorityRules)CHECK(r.gapTime==4 && r.headway==10);
    CHECK(h.document().network.rightOfWay.stopControls.size()==3);
    const auto saved=h.document();const auto loaded=parseDocument(documentJson(saved));
    CHECK(loaded.network==saved.network);CHECK(conflictGroups(loaded.network,{}).size()==1);
    CHECK(h.execute("delete",[&](auto& d){removeConflictGroup(d,id);}));
    CHECK(h.document().network.rightOfWay.empty());h.undo();CHECK(h.document()==saved);
}
TEST(conflict_geometry, two_separate_places_of_the_same_pair_remain_two_groups) {
    ProjectDocument d;addLink(d,{{0,0},{200,0}},3,3.5);
    addLink(d,{{60,-60},{60,40},{140,40},{140,-60}},3,3.5);
    const auto automatic=automaticConflicts(d.network);CHECK(automatic.size()==18);
    const auto groups=conflictGroups(d.network,automatic);CHECK(groups.size()==2);
    for(const auto& g:groups)CHECK(g.automaticKeys.size()==9);
    authorConflictGroup(d,groups[0].key,defaults);
    CHECK(d.network.rightOfWay.conflictAreas.size()==9 && automaticConflicts(d.network).size()==9);
}
TEST(conflict_geometry, group_priority_is_by_owner_even_when_pair_sides_are_reversed) {
    ProjectDocument d;const auto a=addLink(d,{{0,0},{200,0}},3,3.5),b=addLink(d,{{100,-60},{100,60}},3,3.5);
    addCrossingAreas(d,a,b,a,defaults);CHECK(d.network.rightOfWay.conflictAreas.size()==9);
    auto& reversed=d.network.rightOfWay.conflictAreas.back();std::swap(reversed.first,reversed.second);reversed.priority=ConflictPriority::secondYields;
    const auto id=d.network.rightOfWay.conflictAreas.front().id;
    setConflictGroupControl(d,id,"",ConflictPriority::firstYields,3,7);
    for(const auto& area:d.network.rightOfWay.conflictAreas)CHECK(conflictOwner(area.priority==ConflictPriority::firstYields?area.first.path:area.second.path)==a);
    cycleConflictGroupPriority(d,id,defaults);
    for(const auto& area:d.network.rightOfWay.conflictAreas)CHECK(conflictOwner(area.priority==ConflictPriority::firstYields?area.first.path:area.second.path)==b);
}
TEST(conflict_geometry, distinct_single_lane_links_keep_nine_independent_groups) {
    ProjectDocument d;
    for(int i=0;i<3;++i)addLink(d,{{0,i*3.5},{200,i*3.5}},1,3.5);
    for(int i=0;i<3;++i)addLink(d,{{100+i*3.5,-60},{100+i*3.5,60}},1,3.5);
    const auto automatic=automaticConflicts(d.network);
    CHECK(automatic.size()==9 && conflictGroups(d.network,automatic).size()==9);
}
TEST(conflict_geometry, three_lane_connectors_group_with_links_and_other_connectors) {
    for(bool otherConnector:{false,true}) {
        ProjectDocument d;
        const auto in=addLink(d,{{-200,0},{-100,0}},3,3.5),out=addLink(d,{{100,0},{200,0}},3,3.5);
        const auto a=addConnectorRange(d,{in,lane(d,in)},{out,lane(d,out)},3,3);
        std::string b;
        if(otherConnector) {
            const auto low=addLink(d,{{0,-200},{0,-100}},3,3.5),high=addLink(d,{{0,100},{0,200}},3,3.5);
            b=addConnectorRange(d,{low,lane(d,low)},{high,lane(d,high)},3,3);
        }else b=addLink(d,{{0,-100},{0,100}},3,3.5);
        CHECK(pairCount(d.network,a,b)==9);
        const auto groups=conflictGroups(d.network,automaticConflicts(d.network));
        const auto g=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return (g.firstOwner==a && g.secondOwner==b)||(g.firstOwner==b && g.secondOwner==a);});
        CHECK(g!=groups.end() && g->automaticKeys.size()==9);
    }
}
TEST(conflict_geometry, editing_drawing_rebases_authored_stations_without_moving_runtime_extents) {
    auto d=curved(3);const auto c=d.network.connectors.front();const auto road=d.network.links.back().id;
    addCrossingAreas(d,c.id,road,c.id,defaults);CHECK(d.network.rightOfWay.conflictAreas.size()==1);
    History h;h.reset(d);const auto expected=runtimeExtent(d);
    CHECK(h.execute("paint",[&](auto& d){d.network.connectors.front().geometry[1].y+=50;}));
    const auto actual=runtimeExtent(h.document());test::near(actual.first,expected.first,1e-8);test::near(actual.second,expected.second,1e-8);
    const auto r=resolveRightOfWay(h.document().network,runtimeSections(h.document().network),defaults);
    CHECK(r.issues.empty() && r.zones.size()==1);
    const auto& z=r.zones.front();test::near(z.minor.entry,expected.first,1e-8);test::near(z.minor.exit,expected.second,1e-8);
    const auto before=resolveRightOfWay(d.network,runtimeSections(d.network),defaults);
    test::near(z.waitPosition,before.zones.front().waitPosition,1e-8);
    CHECK(!conflictSideOutline(h.document().network,h.document().network.rightOfWay.conflictAreas.front().first).empty());
    h.undo();CHECK(h.document()==d);
}
