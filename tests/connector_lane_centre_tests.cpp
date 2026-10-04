#include "test.hpp"
#include "../src/model/network/connector_surface.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/core/routes.hpp"
#include "../src/model/network/right_of_way.hpp"
#include "../src/model/network/rotation.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
using namespace trafficsim;
namespace {
ProjectDocument turn(DrivingSide side,int from,int to,bool authored=false) {
    ProjectDocument d;d.network.id="lane-centres";d.network.drivingSide=side;
    d.network.links={
        {"a",{{0,0},{40,0}},{{"a1",3.5},{"a2",3.5},{"a3",3.5}}},
        {"b",{{65,25},{65,65}},{{"b1",3.5},{"b2",3.5},{"b3",3.5}}}};
    addConnectorRange(d,{"a","a1"},{"b","b1"},from,to);
    auto& c=d.network.connectors.front();c.geometry=connectorCurve(d.network,c.from,c.to,19);
    if(authored)c.laneWidths=std::vector<double>(std::max(from,to),5);
    return d;
}
Point middle(Point a,Point b){return {std::midpoint(a.x,b.x),std::midpoint(a.y,b.y)};}
void near(Point a,Point b){test::near(a.x,b.x,1e-9);test::near(a.y,b.y,1e-9);}
void centred(const Network& n) {
    const auto& c=n.connectors.front();const auto paths=connectorPaths(n,c);
    const auto rails=connectorBoundaries(n,c);
    CHECK(paths.size()==static_cast<std::size_t>(std::max(c.fromLaneCount,c.toLaneCount)));
    CHECK(rails.size()==paths.size()+1);
    const auto scenario=buildScenario(n,ScenarioDefinition{});
    for(std::size_t k=0;k<paths.size();++k) {
        const auto& p=paths[k];CHECK(p.geometry.size()==c.geometry.size());
        CHECK(p.id==connectorPathId(c,static_cast<int>(k)));
        near(p.geometry.front(),laneAttachment(n,p.from,true));
        near(p.geometry.back(),laneAttachment(n,p.to,false));
        for(std::size_t j=1;j+1<p.geometry.size();++j)
            near(p.geometry[j],middle(rails[k][j],rails[k+1][j]));
        // Length and location use this very path, including the terminal transitions.
        const auto segment=std::find_if(scenario.segments.begin(),scenario.segments.end(),
            [&](const auto& s){return s.id==p.id;});CHECK(segment!=scenario.segments.end());
        test::near(segment->length,polylineLength(p.geometry),1e-12);
        Scenario located;located.segments={*segment};located.routes={{"r",{p.id}}};
        Vehicle vehicle;vehicle.routeIndex=0;vehicle.distance=segment->length*.37;
        const auto at=locateVehicle(located,vehicle);CHECK(at.segmentId==p.id);
        near(pointAlong(p.geometry,at.position),pointAlong(p.geometry,vehicle.distance));
    }
}
}
TEST(lane_centres, curved_multilane_runtime_follows_the_painted_dividers) {
    auto d=turn(DrivingSide::right,3,3);
    const auto& c=d.network.connectors.front();const auto rails=connectorBoundaries(d.network,c);
    // A real 90-degree turn with a measured first-lane discrepancy, not a straight fixture.
    double discrepancy=0;
    for(std::size_t j=1;j+1<c.geometry.size();++j) {
        const auto p=middle(rails[0][j],rails[1][j]);
        discrepancy=std::max(discrepancy,std::hypot(p.x-c.geometry[j].x,p.y-c.geometry[j].y));
    }
    CHECK(discrepancy>1);centred(d.network);
}
TEST(lane_centres, widths_handedness_and_both_taper_sides_keep_lane_pairing) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(int from=1;from<=3;++from)
    for(int to=1;to<=3;++to)for(bool authored:{false,true}) {
        auto d=turn(side,from,to,authored);centred(d.network);
        if(std::abs(from-to)==1) {
            d.network.connectors.front().laneChangeSide=side==DrivingSide::left?LaneSide::right:LaneSide::left;
            centred(d.network);
        }
    }
}
TEST(lane_centres, file_round_trip_and_undo_preserve_authored_geometry_and_derived_paths) {
    auto d=turn(DrivingSide::left,3,2);const auto stored=d.network.connectors.front().geometry;
    const auto original=connectorPaths(d.network,d.network.connectors.front());
    auto reopened=parseDocument(Json::parse(documentJson(d).dump()));
    CHECK(reopened.network.connectors.front().geometry==stored);centred(reopened.network);
    const auto paths=connectorPaths(reopened.network,reopened.network.connectors.front());
    for(std::size_t k=0;k<paths.size();++k)CHECK(paths[k].geometry==original[k].geometry);
    History history;history.reset(d);
    history.execute("width",[](auto& m){m.network.connectors.front().laneWidths={4,5,6};});
    centred(history.document().network);history.undo();CHECK(history.document()==d);
    history.redo();centred(history.document().network);
}

TEST(lane_centres, rotations_body_attachments_and_frozen_weights_use_the_same_surface) {
    for(auto side:{DrivingSide::left,DrivingSide::right})for(double angle:{0.,37.,173.}) {
        auto d=turn(side,3,3);auto& c=d.network.connectors.front();
        d.network.links[0].geometry={{0,0},{25,0},{50,10}};
        d.network.links[1].geometry={{65,25},{65,50},{80,75}};
        c.from.station=15;c.to.station=12;
        c.geometry=connectorCurve(d.network,c.from,c.to,19);
        c.laneBlend=connectorBlendWeights(c);
        for(auto& l:d.network.links)for(auto& p:l.geometry)p=rotatePoint(p,{7,-3},angle);
        for(auto& p:c.geometry)p=rotatePoint(p,{7,-3},angle);
        centred(d.network);const auto table=runtimeSections(d.network);CHECK(table.unsectionable.empty());
        for(const auto& path:table.paths) {
            const ControlPoint point{{"","",c.id,path.from.laneId,path.to.laneId},polylineLength(c.geometry)*.41};
            const auto at=locateControlPoint(d.network,table,point);CHECK(at);CHECK(at->segment==path.id);
            test::near(at->position,matchedStation(c.geometry,path.geometry,point.station),1e-12);
            const auto target=std::find_if(table.sections.begin(),table.sections.end(),
                [&](const auto& section){return section.id==table.pathNext[&path-table.paths.data()];});
            CHECK(target!=table.sections.end());near(path.geometry.back(),target->geometry.front());
        }
    }
}
TEST(lane_centres, straight_single_lane_keeps_exact_geometry_and_singular_drafts_remain_editable) {
    auto d=turn(DrivingSide::right,1,1);auto& c=d.network.connectors.front();
    CHECK(connectorPaths(d.network,c).front().geometry==c.geometry);
    d.network.links[1].geometry={{60,0},{100,0}};c.geometry=connectorCurve(d.network,c.from,c.to,0);
    CHECK(connectorPaths(d.network,c).front().geometry==c.geometry);
    c.laneWidths={5};const auto surface=connectorSurface(d.network,c);
    CHECK(!surface.source);CHECK(!surface.target);centred(d.network);
    c.geometry.clear();CHECK(connectorPaths(d.network,c).front().geometry.empty());
    CHECK(!validateNetwork(d.network).empty());
}

TEST(lane_centres, waiting_bar_intersects_rails_at_the_runtime_vehicle_station) {
    auto d=turn(DrivingSide::right,3,3);const auto& c=d.network.connectors.front();
    const auto table=runtimeSections(d.network);const auto& path=table.paths.front();
    const ControlPoint point{{"","",c.id,path.from.laneId,path.to.laneId},polylineLength(c.geometry)*.5};
    const auto at=locateControlPoint(d.network,table,point);CHECK(at);
    const auto origin=pointAlong(path.geometry,at->position);
    const auto authored=pointAlong(c.geometry,point.station);
    CHECK(std::hypot(origin.x-authored.x,origin.y-authored.y)>.8);
    const auto bar=waitingLineBar(d.network,point);CHECK(bar);
    const auto direction=directionAlong(path.geometry,at->position,false);
    for(const auto p:{bar->first,bar->second})
        test::near((p.x-origin.x)*direction.x+(p.y-origin.y)*direction.y,0,1e-9);
    const Point across{bar->second.x-bar->first.x,bar->second.y-bar->first.y};
    const double span=across.x*across.x+across.y*across.y;CHECK(span>0);
    const double fraction=((origin.x-bar->first.x)*across.x+(origin.y-bar->first.y)*across.y)/span;
    CHECK(fraction>0);CHECK(fraction<1);
}

TEST(lane_centres, forty_seeds_run_curves_and_both_count_changes_with_replay_and_accounting) {
    unsigned onInterior=0;
    for(auto side:{DrivingSide::left,DrivingSide::right})
    for(const auto counts:{std::pair{3,3},std::pair{3,2},std::pair{2,3}}) {
        auto d=turn(side,counts.first,counts.second);const auto& c=d.network.connectors.front();
        auto definition=static_cast<ScenarioDefinition>(test::straight());definition.duration=30;
        definition.priorityDefaults={3,1};definition.routes={{"r",{"a",c.id,"b"}}};
        definition.inputs={{"i","r","car",1800,0,20}};
        const auto scenario=compileScenario(d.network,definition);const auto rails=connectorBoundaries(d.network,c);
        const auto paths=connectorPaths(d.network,c);
        for(std::uint32_t seed=42;seed<=81;++seed) {
            auto state=createSimulation(scenario,seed),replay=state;
            for(auto tick=0;tick<totalTicks(scenario);++tick) {
                state=stepSimulation(std::move(state));replay=stepSimulation(std::move(replay));
                CHECK(state.vehicles==replay.vehicles);CHECK(state.events==replay.events);
                CHECK(state.inputs==replay.inputs);CHECK(state.stopService==replay.stopService);
                CHECK(state.completed==replay.completed);CHECK(state.randomState==replay.randomState);
                CHECK(state.nextVehicleId-1==state.completed+state.vehicles.size()+pendingCount(state));
                auto spans=occupiedSpans(*state.scenario,state.vehicles,*state.index);
                std::sort(spans.begin(),spans.end(),[](const auto& a,const auto& b){
                    return a.segmentIndex!=b.segmentIndex?a.segmentIndex<b.segmentIndex:a.rear<b.rear;});
                for(std::size_t j=1;j<spans.size();++j)
                    if(spans[j-1].segmentIndex==spans[j].segmentIndex && spans[j-1].vehicleId!=spans[j].vehicleId)
                        CHECK(spans[j-1].front<=spans[j].rear+1e-7);
                for(const auto& vehicle:state.vehicles) {
                    const auto at=locateVehicle(*state.scenario,vehicle,*state.index);
                    for(std::size_t k=0;k<paths.size();++k)if(paths[k].id==at.segmentId) {
                        const auto& g=paths[k].geometry;
                        const double first=std::hypot(g[1].x-g.front().x,g[1].y-g.front().y);
                        const double last=polylineLength(g)-std::hypot(g.back().x-g[g.size()-2].x,g.back().y-g[g.size()-2].y);
                        if(at.position>first && at.position<last) {
                            ++onInterior;
                            near(pointAlong(g,at.position),middle(
                                pointAlong(rails[k],matchedStation(g,rails[k],at.position)),
                                pointAlong(rails[k+1],matchedStation(g,rails[k+1],at.position))));
                        }
                    }
                }
            }
        }
    }
    CHECK(onInterior>1000); // Forces actual vehicle travel through curved lane interiors.
}
