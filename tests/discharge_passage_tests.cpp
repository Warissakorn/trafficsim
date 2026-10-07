#include "test.hpp"
#include "../src/eval/discharge.hpp"
using namespace trafficsim;
namespace {
DischargeSpec passageSpec() {return {0,100,0,3,5,2};}
Scenario passageNetwork(double length=20,double head=19,double dt=.1) {
    Scenario s;s.duration=30;s.timeStep=dt;
    s.segments={{"lane",length,{}}};s.routes={{"route",{"lane"}}};
    s.behaviours={{"driver",2,2,3,1.5,.2}};
    s.vehicleTypes={{"car",4,2,{10,10},2,3,6,"driver"}};
    s.signalPrograms={{"signal",0,{{dt,SignalColor::red},{20,SignalColor::green},{10-dt,SignalColor::red}}}};
    s.signalHeads={{"head","lane",head,"signal"}};return s;
}
void go(SimState& a,DischargeAccumulator& d) {
    a.tick=1;a.time=a.scenario->timeStep;a.events.clear();
    for(const auto& h:a.scenario->signalHeads)a.events.emplace_back(SignalEvent{a.time,h.id,SignalColor::green});
    d.observe(a);
}
std::uint32_t routeSlot(const SimState& a,const std::string& id) {
    for(std::uint32_t i=0;i<a.scenario->routes.size();++i)if(a.scenario->routes[i].id==id)return i;
    throw std::runtime_error("Missing fixture route");
}
Scenario lateralNetwork(double dt=.1) {
    auto s=passageNetwork(20,19,dt);s.segments.push_back({"old",20,{}});
    s.routes.push_back({"stub",{"old"}});s.signalHeads.push_back({"old-head","old",19,"signal"});
    s.laneChanges={{"stub","route",0,20,0,20}};s.routeDeadEnds={{"stub",20}};return s;
}
void equalDynamic(const SimState& a,const SimState& b) {
    CHECK(a.vehicles==b.vehicles && a.events==b.events && a.inputs==b.inputs);
    CHECK(a.randomState==b.randomState && a.completed==b.completed && a.nextVehicleId==b.nextVehicleId);
    CHECK(a.tick==b.tick && a.time==b.time && a.seed==b.seed && a.stopService==b.stopService);
    CHECK(a.scenario==b.scenario && a.index==b.index);
}
}
TEST(discharge, insertion_tick_survivor_and_sink_use_pending_type_without_mutation) {
    for(double dt:{.1,.2})for(bool sink:{false,true}) {
        auto s=passageNetwork(sink?.005:20,.004,dt);s.inputs={{"input","route","car",0,0,30}};
        auto a=createSimulation(s,42);PendingVehicle pending;pending.id=1;pending.inputIndex=0;
        pending.routeIndex=routeSlot(a,"route");pending.typeIndex=0;pending.desiredSpeed=10;pending.driverFactor=.5;
        a.inputs[0].queue.push_back(pending);a.nextVehicleId=2;
        DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);auto b=a;
        a=stepSimulation(std::move(a));b=stepSimulation(std::move(b));
        CHECK(std::any_of(a.events.begin(),a.events.end(),[](const auto& e){return std::holds_alternative<DepartedEvent>(e);}));
        CHECK(sink?a.completed==1:a.vehicles.size()==1);equalDynamic(a,b);d.observe(a);d.observe(a);equalDynamic(a,b);
        const auto row=d.report()[0];CHECK(row.unavailable.empty());CHECK(row.crossings.size()==1);
        CHECK(row.crossings[0].vehicleId==1 && row.crossings[0].vehicleTypeId=="car");
        CHECK(!row.crossings[0].queuedAtGo);test::near(row.crossings[0].time,2*dt);
    }
}
TEST(discharge, generated_insertion_survivor_uses_current_type) {
    auto s=passageNetwork(20,.004);auto a=createSimulation(s,42);
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    Vehicle v;v.id=7;v.routeIndex=routeSlot(a,"route");v.typeIndex=0;v.distance=.01;
    a.tick=2;a.time=.2;a.vehicles={v};a.events={DepartedEvent{.1,7,"route",.1,10}};d.observe(a);
    CHECK(d.report()[0].unavailable.empty());CHECK(d.report()[0].crossings.size()==1);
    CHECK(!d.report()[0].crossings[0].queuedAtGo);
}
TEST(discharge, unknown_insertion_sink_type_stays_unavailable) {
    auto a=createSimulation(passageNetwork(.005,.004),42);
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    CHECK(a.inputs.empty()&&a.vehicles.empty());
    a.tick=2;a.time=.2;a.events={DepartedEvent{.1,7,"route",.1,10},ArrivedEvent{.2,7,"route",.1,0,0}};d.observe(a);
    CHECK(d.report()[0].crossings.empty());CHECK(d.report()[0].unavailable=="untracked_source_passage");
}
TEST(discharge, insertion_at_head_is_not_longitudinal_passage) {
    auto a=createSimulation(passageNetwork(20,0),42);
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    Vehicle v;v.id=7;v.routeIndex=routeSlot(a,"route");v.typeIndex=0;v.distance=.01;
    a.tick=2;a.time=.2;a.vehicles={v};a.events={DepartedEvent{.1,7,"route",.1,10}};d.observe(a);
    CHECK(d.report()[0].crossings.empty());CHECK(d.report()[0].unavailable.empty());
}
TEST(discharge, actual_lateral_motion_counts_target_crossing_not_departed_lane) {
    for(double dt:{.1,.2}) {
        auto a=test::withVehicles(lateralNetwork(dt),{{1,"stub",18.9,10}});
        DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);auto b=a;
        a=stepSimulation(std::move(a));b=stepSimulation(std::move(b));
        CHECK(std::any_of(a.events.begin(),a.events.end(),[](const auto& e){return std::holds_alternative<LaneChangeEvent>(e);}));
        CHECK(a.completed==1||(!a.vehicles.empty()&&a.vehicles[0].distance>19));
        d.observe(a);d.observe(a);equalDynamic(a,b);
        auto rows=d.report();CHECK(rows.size()==2);CHECK(rows[0].headId=="head");
        CHECK(rows[0].crossings.size()==1 && !rows[0].crossings[0].queuedAtGo);
        // D125: a moving, never-queued changer cannot shift queued ranks; the arrival is kept as
        // an unqueued crossing (which estimateDischarge rejects inside the ranks) instead.
        CHECK(rows[1].crossings.empty());CHECK(rows[0].unavailable.empty());CHECK(rows[1].unavailable.empty());
        CHECK(estimateDischarge(rows[0],passageSpec()).reason=="partial_cycle");
        for(int i=0;i<15;++i) {a=stepSimulation(std::move(a));b=stepSimulation(std::move(b));d.observe(a);equalDynamic(a,b);}
        CHECK(a.completed==1);CHECK(d.report()[0].crossings.size()==1);
    }
}
TEST(discharge, lateral_station_jump_does_not_invent_crossing) {
    auto s=lateralNetwork();s.laneChanges[0].toStart=5;s.laneChanges[0].toEnd=20;
    auto a=test::withVehicles(s,{{1,"stub",18.9,0}});
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    a.tick=2;a.time=.2;a.vehicles[0].routeIndex=routeSlot(a,"route");a.vehicles[0].distance=19.3;
    CHECK(5+18.9*15/20>19);a.events={LaneChangeEvent{.1,1,"stub","route"}};d.observe(a);
    for(const auto& row:d.report())CHECK(row.crossings.empty());
}
TEST(discharge, ambiguous_lateral_maps_suppress_target_inference) {
    auto s=lateralNetwork();s.laneChanges.push_back({"stub","route",0,20,0,19});
    CHECK(18.9!=18.9*19/20);
    auto a=test::withVehicles(s,{{1,"stub",18.9,0}});
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    a.tick=2;a.time=.2;a.vehicles[0].routeIndex=routeSlot(a,"route");a.vehicles[0].distance=19.1;
    a.events={LaneChangeEvent{.1,1,"stub","route"}};d.observe(a);
    for(const auto& row:d.report()) {CHECK(row.crossings.empty());CHECK(row.unavailable=="route_or_lane_change");}
}
TEST(discharge, downstream_lateral_change_leaves_prior_head_cycle_available) {
    auto s=lateralNetwork();s.signalHeads.erase(s.signalHeads.begin()+1);s.signalHeads[0].position=10;
    auto a=test::withVehicles(s,{{1,"stub",18.9,0}});
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    a.tick=2;a.time=.2;a.vehicles[0].routeIndex=routeSlot(a,"route");a.vehicles[0].distance=19.1;
    a.events={LaneChangeEvent{.1,1,"stub","route"}};d.observe(a);
    CHECK(d.report()[0].crossings.empty());CHECK(d.report()[0].unavailable.empty());
}
TEST(discharge, ambiguous_maps_straddling_head_invalidate_target) {
    auto s=lateralNetwork();s.laneChanges[0].toStart=5;s.laneChanges.push_back({"stub","route",0,20,0,20});
    auto a=test::withVehicles(s,{{1,"stub",18.9,0}});
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    a.tick=2;a.time=.2;a.vehicles[0].routeIndex=routeSlot(a,"route");a.vehicles[0].distance=19.3;
    CHECK(5+18.9*15/20>19 && 18.9<19);
    a.events={LaneChangeEvent{.1,1,"stub","route"}};d.observe(a);
    for(const auto& row:d.report()) {CHECK(row.crossings.empty());CHECK(row.unavailable=="route_or_lane_change");}
}
TEST(discharge, insertion_tick_at_green_end_is_excluded) {
    auto a=createSimulation(passageNetwork(20,.004),42);
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    Vehicle v;v.id=7;v.routeIndex=routeSlot(a,"route");v.typeIndex=0;v.distance=.01;
    a.tick=2;a.time=.2;a.vehicles={v};
    a.events={DepartedEvent{.1,7,"route",.1,10},SignalEvent{.2,"head",SignalColor::red}};d.observe(a);
    const auto row=d.report()[0];CHECK(row.complete);CHECK(row.crossings.empty());CHECK(row.unavailable.empty());
}
TEST(discharge, missing_remap_event_invalidates_both_possible_head_paths) {
    auto a=test::withVehicles(lateralNetwork(),{{1,"stub",18.9,0}});
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    a.tick=2;a.time=.2;a.vehicles[0].routeIndex=routeSlot(a,"route");a.vehicles[0].distance=19.1;a.events.clear();d.observe(a);
    for(const auto& row:d.report()) {CHECK(row.crossings.empty());CHECK(row.unavailable=="route_or_lane_change");}
}
TEST(discharge, repeated_head_passage_never_duplicates_raw_id) {
    auto s=passageNetwork();s.routes.push_back({"other",{"lane"}});
    s.laneChanges={{"route","other",0,20,0,10}};
    auto a=test::withVehicles(s,{{1,"route",18.9,0}});
    DischargeAccumulator d(passageSpec(),{.5,1,10});d.observe(a);go(a,d);
    a.tick=2;a.time=.2;a.vehicles[0].distance=19.1;a.events.clear();d.observe(a);
    CHECK(d.report()[0].crossings.size()==1);CHECK(19.1*10/20<19);
    a.tick=3;a.time=.3;a.vehicles[0].routeIndex=routeSlot(a,"other");a.vehicles[0].distance=19.3;
    a.events={LaneChangeEvent{.2,1,"route","other"}};d.observe(a);
    CHECK(d.report()[0].crossings.size()==1);CHECK(d.report()[0].unavailable=="repeated_head_passage");
}
