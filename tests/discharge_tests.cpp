#include "test.hpp"
#include "../src/eval/discharge.hpp"
#include "../src/core/routes.hpp"
#include "../src/project/evaluation.hpp"
#include <nlohmann/json.hpp>
using namespace trafficsim;
namespace {
DischargeSpec spec() { return {0,100,0,3,5,2}; }
DischargeCycle cycle() {
    return {"head","lane",0,20,.1,true,{},
        {{1,"car",3,true},{2,"bus",5.5,true},{3,"car",7.5,true},
         {4,"car",9.5,true},{5,"bus",11.5,true}}};
}
Scenario network() {
    Scenario s; s.duration=30; s.timeStep=.1;
    s.segments={{"lane",20,{}}}; s.routes={{"route",{"lane"}}};
    s.behaviours={{"driver",2,2,3,1.5,.2}};
    s.vehicleTypes={{"car",4,2,{10,10},2,3,6,"driver"}};
    s.signalPrograms={{"signal",0,{{1,SignalColor::red},{20,SignalColor::green},{9,SignalColor::red}}}};
    s.signalHeads={{"head","lane",20,"signal"}};
    return s;
}
SimState state() { return test::withVehicles(network(),{{1,"route",18,0}}); }
}
TEST(discharge, analytic_queue_discharge_and_startup) {
    const auto r=estimateDischarge(cycle(),spec());
    CHECK(r.reason.empty()); CHECK(r.samples==3);
    test::near(*r.meanHeadway,2); test::near(*r.dischargeVehiclesPerHour,1800);
    test::near(*r.startupLostTime,1.5);
}
TEST(discharge, unavailable_empty_short_partial_and_interrupted) {
    auto c=cycle(); c.crossings.clear(); CHECK(estimateDischarge(c,spec()).reason=="insufficient_crossings");
    c=cycle(); c.complete=false; CHECK(estimateDischarge(c,spec()).reason=="partial_cycle");
    c=cycle(); c.crossings[1].queuedAtGo=false;
    CHECK(estimateDischarge(c,spec()).reason=="queue_not_sustained");
    c=cycle(); c.crossings.resize(1); CHECK(estimateDischarge(c,spec()).reason=="insufficient_crossings");
}
TEST(discharge, half_open_windows_and_warmup_exclude_whole_cycles) {
    auto s=spec(); s.windowStart=1; CHECK(estimateDischarge(cycle(),s).reason=="outside_window");
    s=spec(); s.windowEnd=20; CHECK(estimateDischarge(cycle(),s).reason.empty());
    s.warmup=.1; CHECK(estimateDischarge(cycle(),s).reason=="outside_window");
    auto c=cycle(); c.crossings.back().time=c.end;
    CHECK(estimateDischarge(c,spec()).reason=="invalid_crossings");
}
TEST(discharge, invalid_specs_records_and_signed_startup) {
    auto s=spec(); s.steadyFirst=1; test::throws([&]{validateDischargeSpec(s);});
    auto c=cycle(); c.crossings[2].time=c.crossings[1].time;
    CHECK(estimateDischarge(c,spec()).reason=="nonpositive_headway");
    c=cycle(); c.crossings[2].vehicleId=1; CHECK(estimateDischarge(c,spec()).reason=="invalid_crossings");
    c=cycle(); c.crossings[0].time=1; c.crossings[1].time=2;
    c.crossings[2].time=4; c.crossings[3].time=6; c.crossings[4].time=8;
    test::near(*estimateDischarge(c,spec()).startupLostTime,-2);
}
TEST(discharge, snapshot_sink_crossing_duplicate_tick_and_partial_cycle) {
    auto a=state(); DischargeAccumulator d(spec(),{.5,1,10}); d.observe(a);
    // Signal turns Go with the queued vehicle still present, then it crosses at the sink.
    a.tick=1; a.time=.1; a.events={SignalEvent{.1,"head",SignalColor::green}}; d.observe(a);
    a.tick=2; a.time=.2; a.vehicles.clear(); a.events={ArrivedEvent{.2,1,"route",0,0,0}};
    d.observe(a); d.observe(a);
    auto rows=d.report(); CHECK(rows.size()==1); CHECK(rows[0].crossings.size()==1);
    CHECK(rows[0].crossings[0].queuedAtGo); CHECK(!rows[0].complete);
    CHECK(estimateDischarge(rows[0],spec()).reason=="partial_cycle");
}
TEST(discharge, independent_lanes_and_discontinuous_observation) {
    auto a=state(); auto s=*a.scenario;
    s.segments.push_back({"other",20,{}}); s.routes.push_back({"other-route",{"other"}});
    s.signalHeads.push_back({"other-head","other",20,"signal"});
    a=test::withVehicles(s,{{1,"route",18,0},{2,"other-route",18,0}});
    DischargeAccumulator d(spec(),{.5,1,10}); d.observe(a);
    a.tick=1; a.time=.1; a.events={SignalEvent{.1,"head",SignalColor::green},SignalEvent{.1,"other-head",SignalColor::green}};
    d.observe(a); a.tick=2; a.time=.2; a.vehicles.clear();
    a.events={ArrivedEvent{.2,1,"route",0,0,0},ArrivedEvent{.2,2,"other-route",0,0,0}};
    d.observe(a); auto rows=d.report(); CHECK(rows.size()==2);
    CHECK(rows[0].crossings.size()==1 && rows[1].crossings.size()==1);
    CHECK(rows[0].laneId!=rows[1].laneId);
    a.tick=4; test::throws([&]{d.observe(a);},"consecutive");
}
TEST(discharge, observer_preserves_copied_seeded_trajectory) {
    auto a=state(); auto b=a; DischargeAccumulator d(spec(),{.5,1,10}); d.observe(a);
    for(unsigned i=0;i<220;++i) {
        a=stepSimulation(std::move(a)); b=stepSimulation(std::move(b)); d.observe(a);
        CHECK(a.events==b.events); CHECK(a.vehicles==b.vehicles);
        CHECK(a.inputs==b.inputs); CHECK(a.randomState==b.randomState); CHECK(a.completed==b.completed);
        CHECK(a.seed==b.seed && a.tick==b.tick && a.time==b.time && a.nextVehicleId==b.nextVehicleId);
        CHECK(a.stopService==b.stopService); CHECK(a.scenario==b.scenario && a.index==b.index);
    }
    CHECK(a.completed==1); auto rows=d.report(); CHECK(rows.size()==1);
    CHECK(rows[0].complete); CHECK(rows[0].crossings.size()==1);
}

TEST(discharge, json_metadata_and_null_unavailable_estimates) {
    const auto result=dischargeJson({cycle()},spec());
    CHECK(result["steadyFirstRank"]==3); CHECK(result["classFilter"].is_string());
    CHECK(result["cycles"][0]["crossings"][1]["vehicleTypeId"]=="bus");
    test::near(result["cycles"][0]["startupLostTimeSeconds"].get<double>(),1.5);
    auto c=cycle(); c.complete=false;
    const auto row=dischargeJson({c},spec())["cycles"][0];
    CHECK(row["meanHeadwaySeconds"].is_null()); CHECK(row["unavailableReason"]=="partial_cycle");
}
TEST(discharge, remaps_and_untracked_sources_make_cycles_unavailable) {
    auto a=state(); DischargeAccumulator d(spec(),{.5,1,10}); d.observe(a);
    a.tick=1; a.time=.1; a.events={SignalEvent{.1,"head",SignalColor::green}}; d.observe(a);
    a.tick=2; a.time=.2; a.events={LaneChangeEvent{.2,1,"route","route"}}; d.observe(a);
    CHECK(d.report()[0].unavailable=="route_or_lane_change");
    DischargeAccumulator source(spec(),{.5,1,10}); a=state(); source.observe(a);
    a.tick=1; a.time=.1; a.events={SignalEvent{.1,"head",SignalColor::green}}; source.observe(a);
    a.tick=2; a.time=.2; a.events={DepartedEvent{.1,2,"route",0,10}}; source.observe(a);
    CHECK(source.report()[0].unavailable=="untracked_source_passage");
}
TEST(discharge, timestep_metadata_initial_green_and_boundary_crossing) {
    for(const auto dt:{.1,.2}) {
        auto s=network(); s.timeStep=dt;
        auto a=test::withVehicles(s,{{1,"route",18,0}});
        DischargeAccumulator d(spec(),{.5,1,10}); d.observe(a);
        a.tick=1; a.time=dt; a.events={SignalEvent{dt,"head",SignalColor::green}}; d.observe(a);
        a.tick=2; a.time=2*dt; a.vehicles.clear();
        a.events={ArrivedEvent{a.time,1,"route",0,0,0},SignalEvent{a.time,"head",SignalColor::red}};
        d.observe(a); CHECK(d.report()[0].crossings.empty());
        test::near(d.report()[0].timeStep,dt); CHECK(d.report()[0].complete);
    }
    auto a=state(); a.events={SignalEvent{0,"head",SignalColor::green}};
    DischargeAccumulator d(spec(),{.5,1,10}); d.observe(a);
    a.tick=1; a.time=.1; a.events={SignalEvent{.1,"head",SignalColor::red}}; d.observe(a);
    CHECK(estimateDischarge(d.report()[0],spec()).reason=="initial_partial_cycle");
}
