#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <algorithm>
using namespace trafficsim;
namespace {
std::string lane(const ProjectDocument& d,const std::string& id) {
    for(const auto& l:d.network.links)if(l.id==id)return l.lanes.front().id;
    throw std::runtime_error("lane");
}
ControlPathRef ref(const Connector& c){return {"","",c.id,c.from.laneId,c.to.laneId};}
double area(const ConflictPolygons& polygons) {
    double total=0;
    for(const auto& p:polygons) {
        double sum=0;
        for(std::size_t i=0;i<p.size();++i)sum+=p[i].x*p[(i+1)%p.size()].y-p[i].y*p[(i+1)%p.size()].x;
        total+=std::abs(sum)/2;
    }
    return total;
}
ProjectDocument join() {
    ProjectDocument d;
    const auto a=addLink(d,{{-100,30},{100,30}},1,3.5),b=addLink(d,{{0,0},{200,0}},1,3.5);
    addConnector(d,{a,lane(d,a),100},{b,lane(d,b),100});
    return d;
}
}
TEST(conflict_geometry, oblique_crossing_keeps_the_actual_intersection_polygon) {
    ProjectDocument d;
    const auto a=addLink(d,{{0,0},{200,0}},1,3.5),b=addLink(d,{{0,-100},{200,100}},1,3.5);
    const ControlPathRef pa{a,lane(d,a),"","",""},pb{b,lane(d,b),"","",""};
    const auto raw=surfaceOverlaps(d.network,pa,pb);
    CHECK(raw.size()==1 && raw[0].status==SurfaceOverlap::Status::overlap);
    CHECK(!raw[0].polygons.empty());
    // Independent analytical area of two width-3.5 strips meeting at 45 degrees.
    test::near(area(raw[0].polygons),3.5*3.5/std::sqrt(0.5),1e-8);
    const ConflictSide first{pa,raw[0].first.from,raw[0].first.to,""},second{pb,raw[0].second.from,raw[0].second.to,""};
    const auto polygons=conflictAreaPolygons(d.network,ConflictKind::crossing,first,second);
    CHECK(polygons==raw[0].polygons);
    CHECK(area({conflictSideOutline(d.network,first)})>area(polygons)+1);
    const auto reverse=surfaceOverlaps(d.network,pb,pa);
    test::near(area(reverse[0].polygons),area(polygons),1e-8);
    test::near(reverse[0].first.from,raw[0].second.from);
}
TEST(conflict_geometry, a_mouth_keeps_its_geometry_and_classifies_the_two_ends) {
    const auto d=join();const auto& c=d.network.connectors.front();
    for(bool source:{true,false}) {
        const auto& attachment=source?c.from:c.to;
        const ControlPathRef road{attachment.linkId,attachment.laneId,"","",""};
        const auto raw=surfaceOverlaps(d.network,ref(c),road);
        CHECK(raw.size()==1 && raw[0].status==SurfaceOverlap::Status::overlap);
        const auto classified=classifiedOverlaps(d.network,ref(c),road);
        CHECK(classified[0].polygons==raw[0].polygons);
        CHECK(classified[0].geometryKind==(source?ConflictGeometryKind::branching:ConflictGeometryKind::merge));
        CHECK(crossingOverlaps(d.network,ref(c),road)[0].status==SurfaceOverlap::Status::none);
        const auto reverse=classifiedOverlaps(d.network,road,ref(c));
        CHECK(reverse[0].geometryKind==classified[0].geometryKind);
        test::near(area(reverse[0].polygons),area(raw[0].polygons),1e-8);
    }
    const auto automatic=automaticConflicts(d.network);
    const auto merge=std::find_if(automatic.begin(),automatic.end(),[](const auto& a){return a.kind==ConflictKind::merge;});
    CHECK(merge!=automatic.end() && !merge->polygons.empty());
    const auto mouth=classifiedOverlaps(d.network,ref(c),{c.to.linkId,c.to.laneId,"","",""});
    test::near(area(merge->polygons),area(mouth[0].polygons));
}
TEST(conflict_geometry, branching_is_selectable_data_but_cannot_author_a_priority) {
    auto d=join();const auto automatic=automaticConflicts(d.network);
    const auto branch=std::find_if(automatic.begin(),automatic.end(),[](const auto& a){return a.kind==ConflictKind::branching;});
    CHECK(branch!=automatic.end() && !branch->polygons.empty());
    const auto groups=conflictGroups(d.network,automatic);
    const auto group=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return conflictGroupContains(g,branch->key);});
    CHECK(group!=groups.end() && group->geometryKind==ConflictGeometryKind::branching);
    const auto before=d;
    test::throws([&]{authorAutomaticConflict(d,*branch,{3,7});},"EDIT_BRANCHING_PRIORITY");CHECK(d==before);
    test::throws([&]{authorConflictGroup(d,group->key,{3,7});},"EDIT_BRANCHING_PRIORITY");CHECK(d==before);
    auto disguised=*branch;disguised.kind=ConflictKind::crossing;disguised.geometryKind=ConflictGeometryKind::crossing;
    test::throws([&]{authorAutomaticConflict(d,disguised,{3,7});},"EDIT_BRANCHING_PRIORITY");CHECK(d==before);
    test::throws([&]{putConflictArea(d,{"","",ConflictKind::branching,branch->first,branch->second});},"EDIT_BRANCHING_PRIORITY");
    CHECK(d==before);
    test::throws([]{conflictKindFromName("branching");},"INVALID_ENUM");
    test::throws([]{conflictKindName(ConflictKind::branching);},"INVALID_ENUM");
    const auto loaded=parseDocument(documentJson(d));
    CHECK(automaticConflicts(loaded.network)==automatic);
    CHECK(loaded.network.rightOfWay.empty());
}
TEST(conflict_geometry, one_stream_terminal_attachments_are_continuations) {
    ProjectDocument d;
    const auto a=addLink(d,{{-100,30},{100,30}},1,3.5),b=addLink(d,{{100,0},{300,0}},1,3.5);
    addConnector(d,{a,lane(d,a)},{b,lane(d,b)});
    const auto& c=d.network.connectors.front();bool measured=false;
    for(const auto& attachment:{c.from,c.to}) {
        const auto pieces=classifiedOverlaps(d.network,ref(c),{attachment.linkId,attachment.laneId,"","",""});
        for(const auto& piece:pieces) {
            CHECK(piece.status==SurfaceOverlap::Status::none || piece.status==SurfaceOverlap::Status::overlap);
            if(piece.status!=SurfaceOverlap::Status::overlap)continue;
            measured=true;
            CHECK(piece.geometryKind==ConflictGeometryKind::continuation && !piece.polygons.empty());
        }
    }
    CHECK(measured);
    CHECK(automaticConflicts(d.network).empty());
}
TEST(conflict_geometry, separate_locations_keep_separate_polygons_after_edits_and_undo) {
    ProjectDocument d;
    const auto a=addLink(d,{{0,0},{200,0}},1,3.5),b=addLink(d,{{60,-60},{60,40},{140,40},{140,-60}},1,3.5);
    const auto automatic=automaticConflicts(d.network);
    CHECK(automatic.size()==2 && !automatic[0].polygons.empty() && !automatic[1].polygons.empty());
    CHECK(conflictGroups(d.network,automatic).size()==2);
    const auto created=authorAutomaticConflict(d,automatic[0],{3,7});
    const auto before=conflictAreaPolygons(d.network,ConflictKind::crossing,d.network.rightOfWay.conflictAreas[0].first,d.network.rightOfWay.conflictAreas[0].second);
    History h;h.reset(d);
    CHECK(h.execute("move",[&](auto& candidate){changeGeometry(candidate,b,{{70,-60},{70,40},{150,40},{150,-60}});}));
    const auto& changed=h.document().network.rightOfWay.conflictAreas.front();
    CHECK(changed.id==created);
    CHECK(conflictAreaPolygons(h.document().network,changed.kind,changed.first,changed.second)!=before);
    CHECK(resolveRightOfWay(h.document().network,runtimeSections(h.document().network),{3,7}).issues.empty());
    h.undo();CHECK(h.document()==d);
    CHECK(conflictAreaPolygons(h.document().network,ConflictKind::crossing,d.network.rightOfWay.conflictAreas[0].first,d.network.rightOfWay.conflictAreas[0].second)==before);
    CHECK(!a.empty());
}
