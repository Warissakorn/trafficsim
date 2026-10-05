#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/model/network/conflict_surface.hpp"
#include <algorithm>
#include <cmath>
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
TEST(conflict_geometry, runtime_overlap_moves_with_the_edited_painted_lane) {
    auto d=curved(3);const auto before=runtimeExtent(d);
    const auto c=d.network.connectors.front();auto geometry=c.geometry;geometry[1].y+=5;
    changeConnectorGeometry(d,c.id,geometry);
    const auto after=runtimeExtent(d);
    CHECK(std::abs(after.first-before.first)>1);
    const auto surface=conflictSurface(d.network,connectorRef(d.network.connectors.front()));CHECK(surface);
    const auto rails=connectorBoundaries(d.network,d.network.connectors.front());
    CHECK(surface->left==rails[0] && surface->right==rails[1]);
    const auto path=connectorPaths(d.network,d.network.connectors.front()).front();
    const auto road=d.network.links.back();
    const auto entry=connectorPathPoint(path,after.first),exit=connectorPathPoint(path,after.second);
    // The conflict interval must physically contain the crossing road's centre.
    CHECK(std::min(entry.x,exit.x)<=road.geometry.front().x);
    CHECK(std::max(entry.x,exit.x)>=road.geometry.front().x);
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
TEST(conflict_geometry, a_separate_interior_crossing_of_the_attached_lane_is_retained) {
    ProjectDocument d;
    const auto source=addLink(d,{{-100,0},{0,0}},1,3.5),target=addLink(d,{{100,100},{100,200}},1,3.5);
    const auto id=addConnector(d,{source,lane(d,source)},{target,lane(d,target)});
    changeConnectorGeometry(d,id,{{0,0},{20,20},{-50,20},{-50,-20},{50,-20},{100,100}});
    const auto& c=d.network.connectors.front();const ControlPathRef road{source,lane(d,source),"","",""};
    const auto raw=surfaceOverlaps(d.network,connectorRef(c),road);
    CHECK(raw.size()==2 && raw[0].status==SurfaceOverlap::Status::overlap);
    const auto crossings=crossingOverlaps(d.network,connectorRef(c),road);
    CHECK(crossings.size()==1 && crossings[0].status==SurfaceOverlap::Status::overlap);
    CHECK(crossings[0].first.from>20); // interior piece survives the terminal-mouth exclusion
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
TEST(conflict_geometry, editing_lane_rebases_authored_controls_to_the_new_runtime_extents) {
    auto d=curved(3);const auto c=d.network.connectors.front();const auto road=d.network.links.back().id;
    addCrossingAreas(d,c.id,road,c.id,defaults);CHECK(d.network.rightOfWay.conflictAreas.size()==1);
    History h;h.reset(d);const auto expected=runtimeExtent(d);
    CHECK(h.execute("paint",[&](auto& d){auto g=d.network.connectors.front().geometry;g[1].y+=5;changeConnectorGeometry(d,c.id,g);}));
    const auto actual=runtimeExtent(h.document());CHECK(std::abs(actual.first-expected.first)>1);
    const auto r=resolveRightOfWay(h.document().network,runtimeSections(h.document().network),defaults);
    CHECK(r.issues.empty() && r.zones.size()==1);
    const auto& z=r.zones.front();test::near(z.minor.entry,actual.first,1e-8);test::near(z.minor.exit,actual.second,1e-8);
    const auto before=resolveRightOfWay(d.network,runtimeSections(d.network),defaults);
    CHECK(std::abs(z.waitPosition-before.zones.front().waitPosition)>1);
    test::near(z.minor.entry-z.waitPosition,1,1e-8);
    CHECK(!conflictSideOutline(h.document().network,h.document().network.rightOfWay.conflictAreas.front().first).empty());
    h.undo();CHECK(h.document()==d);
}
TEST(conflict_geometry, add_crossing_does_not_duplicate_an_attached_link_mouth) {
    ProjectDocument d;
    const auto source=addLink(d,{{-100,30},{100,30}},1,3.5),target=addLink(d,{{0,0},{200,0}},1,3.5);
    const auto id=addConnector(d,{source,lane(d,source),100},{target,lane(d,target),100});
    const auto ref=connectorRef(d.network.connectors.front());
    CHECK(validateNetwork(d.network).empty() && connectorRuntimeIssues(d.network).empty());
    for(const auto& link:{source,target}) {
        const ControlPathRef road{link,lane(d,link),"","",""};
        CHECK(surfaceOverlaps(d.network,ref,road).front().status==SurfaceOverlap::Status::overlap);
        CHECK(crossingOverlaps(d.network,ref,road).front().status==SurfaceOverlap::Status::none);
        CHECK(crossingOverlaps(d.network,road,ref).front().status==SurfaceOverlap::Status::none);
        CHECK(pairCount(d.network,id,link)==(link==target?1:0)); // actual target merge, no crossing
        const auto before=d;
        test::throws([&]{addCrossingAreas(d,id,link,id,defaults);},"EDIT_NO_CROSSING");
        CHECK(d==before);
    }
    // A retained old crossing at that mouth is rejected by Run, rather than taking over a merge.
    const ControlPathRef road{target,lane(d,target),"","",""};
    const auto o=surfaceOverlaps(d.network,ref,road).front();
    const auto firstLine=putWaitingLine(d,{"","",{ref,offsetControlStation(d.network,ref,o.first.from,-1)}});
    const auto secondLine=putWaitingLine(d,{"","",{road,offsetControlStation(d.network,road,o.second.from,-1)}});
    const auto area=putConflictArea(d,{"","",ConflictKind::crossing,{ref,o.first.from,o.first.to,firstLine},
        {road,o.second.from,o.second.to,secondLine},ConflictPriority::firstYields});
    putPriorityRule(d,{"","",area,3,7});
    const auto r=resolveRightOfWay(d.network,runtimeSections(d.network),defaults);
    CHECK(std::any_of(r.issues.begin(),r.issues.end(),[](const auto& i){return i.code=="CONFLICT_NO_OVERLAP";}));
}
TEST(conflict_geometry, manual_crossing_keeps_the_neighbour_lane_swept_before_a_join) {
    ProjectDocument d;
    const auto main=addLink(d,{{0,0},{200,0}},2,3.5),minor=addLink(d,{{100,-60},{100,-15}},1,3.5);
    const auto landing=d.network.links.front().lanes.back().id;
    const auto c=addConnector(d,{minor,lane(d,minor)},{main,landing,130});
    const auto created=addCrossingAreas(d,c,main,c,defaults);
    CHECK(!created.empty());
    for(const auto& a:d.network.rightOfWay.conflictAreas) {
        const auto& side=a.first.path.linkId==main?a.first:a.second;
        CHECK(side.path.linkId==main && side.path.laneId!=landing);
    }
}
TEST(conflict_geometry, mapped_lane_mouths_are_excluded_at_link_ends_and_internal_stations) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(int count:{1,2,3})
        for(double station:{0.,100.,200.})for(Point u:{Point{0,-1},Point{1,0},Point{0,1}}) {
            ProjectDocument d;d.network.drivingSide=side;
            const Point end{station-50*u.x,-50*u.y};
            const auto source=addLink(d,{{end.x-100*u.x,end.y-100*u.y},end},count,3.5);
            const auto target=addLink(d,{{0,0},{200,0}},count,3.5);
            addConnectorRange(d,{source,lane(d,source)},{target,lane(d,target),station},count,count);
            const auto& c=d.network.connectors.front();
            CHECK(validateNetwork(d.network).empty());
            const auto issues=connectorRuntimeIssues(d.network);
            // A target at its very end has no receiving section: still a valid geometry draft.
            if(station==200)CHECK(std::any_of(issues.begin(),issues.end(),[](const auto& i){return i.code=="UNSUPPORTED_CONNECTOR_POSITION";}));
            else CHECK(issues.empty());
            for(const auto& path:connectorPaths(d.network,c)) {
                const ControlPathRef ref{"","",c.id,path.from.laneId,path.to.laneId};
                const ControlPathRef joined{target,path.to.laneId,"","",""};
                const auto measured=surfaceOverlaps(d.network,ref,joined);
                CHECK(measured.front().status==SurfaceOverlap::Status::none || measured.front().status==SurfaceOverlap::Status::overlap);
                const auto crossing=crossingOverlaps(d.network,ref,joined);
                if(crossing.front().status!=SurfaceOverlap::Status::none)
                    throw std::runtime_error("mouth: count="+std::to_string(count)+" direction="+std::to_string(u.x)+","+std::to_string(u.y)+
                        " station="+std::to_string(station)+" lane="+path.to.laneId+
                        " connector interval="+std::to_string(crossing.front().first.from)+","+std::to_string(crossing.front().first.to)+
                        " link interval="+std::to_string(crossing.front().second.from)+","+std::to_string(crossing.front().second.to));
            }
        }
}
