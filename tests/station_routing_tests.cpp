#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/project/run.hpp"
#include "../src/core/routing.hpp"
using namespace trafficsim;
namespace {
Scenario routingFork() {
    auto s=test::straight();
    s.segments={{"road",100,{"left","right"}},{"left",100,{}},{"right",100,{}}};
    s.routes={{"a",{"road","left"}},{"b",{"road","right"}}};s.inputs.clear();
    s.routeDecisions={{"decision","a",60,{{0,1,0}},{{"a",1,{0}},{"b",1,{1}}}},
                      {"decision","b",60,{{0,1,0}},{{"a",1,{0}},{"b",1,{1}}}}};return s;
}
ProjectDocument drawing(int lanes=1) {
    ProjectDocument d;d.definition=AuthoringDefinition{};
    const auto a=addLink(d,{{0,0},{100,0}},lanes,3.5);
    const auto b=addLink(d,{{120,0},{220,0}},lanes,3.5);
    const auto c=addLink(d,{{120,50},{220,50}},lanes,3.5);
    const auto x=addConnector(d,{a,d.network.links[0].lanes[0].id},{b,d.network.links[1].lanes[0].id});
    const auto y=addConnector(d,{a,d.network.links[0].lanes[0].id},{c,d.network.links[2].lanes[0].id});
    const auto r1=putRoute(d,{{},{a,x,b}}),r2=putRoute(d,{{},{a,y,c}});
    RoutingDecision decision;decision.id="decision";decision.linkId=a;decision.position=40;
    decision.routes={{r1,3},{r2,1}};putRoutingDecision(d,decision);
    putInput(d,{"in","","car",800,0,60,{},{},{},{},a});return d;
}
}
TEST(stationrouting, recognizes_once_at_the_line_not_at_source) {
    const auto before=test::withVehicles(routingFork(),{{1,"a",59.8,15}});
    CHECK(before.vehicles[0].passedDecisions.empty());
    const auto upstream=test::withVehicles(routingFork(),{{1,"a",10,15}});
    const auto unaware=stepSimulation(upstream);
    CHECK(unaware.vehicles[0].passedDecisions.empty() && unaware.randomState==upstream.randomState);
    CHECK(unaware.vehicles[0].routeIndex==upstream.vehicles[0].routeIndex);
    const auto after=stepSimulation(before);
    CHECK(before.vehicles[0].distance==59.8 && before.vehicles[0].passedDecisions.empty());
    CHECK(after.vehicles.size()==1);test::near(after.vehicles[0].distance,60);
    CHECK(after.scenario->routes[after.vehicles[0].routeIndex].id=="b");
    CHECK(after.vehicles[0].speed>0); // recognition is not a stop line
    CHECK(after.vehicles[0].passedDecisions==std::vector<std::string>{"decision"});
    CHECK(after.randomState!=before.randomState);
    CHECK(stepSimulation(before).events==after.events);
    const auto again=stepSimulation(after);CHECK(again.randomState==after.randomState);
    CHECK(again.vehicles[0].distance>60);
    CHECK(std::none_of(again.events.begin(),again.events.end(),[](const auto& e){return std::holds_alternative<RoutingEvent>(e);}));
    for(const auto& e:after.events)if(std::holds_alternative<RoutingEvent>(e))CHECK(eventJson(e)["kind"]=="routing");
    CHECK(checkpointJson(after)["vehicles"][0]["passedDecisions"][0]=="decision");
}
TEST(stationrouting, uses_passage_time_and_half_open_intervals) {
    auto s=routingFork();
    auto state=test::withVehicles(s,{{1,"a",59.8,15}});state.tick=9;state.time=.9;
    // At exactly 1 s the interval is over; default weights are deliberately all-left.
    for(auto& d:s.routeDecisions){d.choices[0].weight=1;d.choices[1].weight=0;}
    state=test::withVehicles(s,{{1,"a",59.8,15}});state.tick=9;state.time=.9;
    state.vehicles[0].scheduledTime=0;
    auto after=stepSimulation(state);CHECK(after.scenario->routes[after.vehicles[0].routeIndex].id=="a");
    CHECK(after.vehicles[0].passedDecisions.size()==1);
    for(auto& gate:s.routeDecisions)for(auto& choice:gate.choices)choice.intervalWeights={0};
    auto fallback=stepSimulation(test::withVehicles(s,{{1,"a",59.8,15}}));
    CHECK(fallback.scenario->routes[fallback.vehicles[0].routeIndex].id=="a");
}
TEST(stationrouting, zero_position_and_downstream_arrival) {
    auto s=routingFork();for(auto& d:s.routeDecisions)d.at=0;
    const auto after=stepSimulation(test::withVehicles(s,{{1,"a",0,0}}));
    CHECK(after.vehicles[0].passedDecisions.size()==1);CHECK(after.vehicles[0].distance>0);
    const auto past=stepSimulation(test::withVehicles(routingFork(),{{1,"a",70,15}}));
    CHECK(past.vehicles[0].passedDecisions.empty());
}
TEST(stationrouting, consecutive_lines_keep_independent_once_state) {
    auto s=routingFork();auto first=s.routeDecisions;
    for(auto& d:first){d.id="earlier";d.at=20;}
    s.routeDecisions.insert(s.routeDecisions.end(),first.begin(),first.end());
    auto state=test::withVehicles(s,{{1,"a",19.8,15}});
    state=stepSimulation(state);CHECK(state.vehicles[0].passedDecisions==std::vector<std::string>{"earlier"});
    for(int i=0;i<50 && state.vehicles[0].passedDecisions.size()<2;++i)state=stepSimulation(state);
    CHECK(state.vehicles[0].passedDecisions==std::vector<std::string>({"earlier","decision"}));
}
TEST(stationrouting, rejects_teleporting_prefix_and_bad_weights) {
    auto s=routingFork();s.routeDecisions[0].at=110;
    CHECK(!routingIssues(s).empty());
    test::throws([&]{createSimulation(s,42);},"ROUTING_PREFIX_MISMATCH");
    s=routingFork();s.routeDecisions[0].choices[0].weight=-1;CHECK(!routingIssues(s).empty());
    s=routingFork();s.conflictZones={{"zone",{{"right"},0,10},{{"road"},50,70},45,3,3}};
    const auto issues=routingIssues(s);
    CHECK(std::any_of(issues.begin(),issues.end(),[](const auto& i){return i.code=="ROUTING_POSITION_IN_CONFLICT";}));
}
TEST(stationrouting, schema_roundtrip_and_legacy_opt_in) {
    auto d=drawing();validateDocument(d);
    const auto json=documentJson(d);CHECK(json["schemaVersion"]==20);
    const auto loaded=parseDocument(json);CHECK(loaded.definition->routingDecisions==d.definition->routingDecisions);
    auto old=json;old["schemaVersion"]=19;test::throws([&]{parseDocument(old);},"UNSUPPORTED_FIELD");
    d.definition->routingDecisions[0].position.reset();CHECK(documentJson(d)["schemaVersion"]==17);
    const auto s=compileDocument(d,test::root()/"data").scenario;CHECK(s.routeDecisions.empty());
}
TEST(stationrouting, compile_volume_lanes_type_rules_and_intervals) {
    auto d=drawing();auto& decision=d.definition->routingDecisions[0];
    decision.intervals={{0,10},{20,30}};decision.routes[0].intervalFlows={0,3};decision.routes[1].intervalFlows={4,0};
    decision.typeRules={{"car",{0,1},{{1,0},{0,1}}}};
    const auto s=compileDocument(d,test::root()/"data").scenario;
    CHECK(s.routeDecisions.size()==2);double volume=0;for(const auto& input:s.inputs)volume+=input.vehiclesPerHour;
    test::near(volume,800);CHECK(s.inputs.size()==2); // passage intervals never split the source stream
    for(const auto& gate:s.routeDecisions) {
        test::near(gate.at,40);CHECK(gate.choices.size()==2);
        test::near(gate.choices[0].weight,0);test::near(gate.choices[1].weight,1);
        CHECK(gate.choices[0].intervalWeights==std::vector<double>({1,0}));
    }
    CHECK(validateScenario(s).empty());
}
TEST(stationrouting, lane_awareness_starts_at_station_and_keeps_volume) {
    auto d=drawing(2);d.definition->inputs[0].laneShares={3,1};
    const auto s=compileDocument(d,test::root()/"data").scenario;CHECK(!s.laneChanges.empty());
    double total=0;for(const auto& input:s.inputs)total+=input.vehiclesPerHour;test::near(total,800);
    bool recognizedSpan=false;
    for(const auto& span:s.laneChanges)if(std::any_of(s.routeDecisions.begin(),s.routeDecisions.end(),[&](const auto& gate){return gate.fromRouteId==span.fromRouteId;})) {
        CHECK(span.fromStart>=40-1e-7 && span.toStart>=40-1e-7);recognizedSpan=true;
    }
    CHECK(recognizedSpan); // fixed authored Route inputs retain their legacy awareness
    for(const auto& gate:s.routeDecisions)CHECK(gate.at==40);
    auto moved=d;moved.definition->routingDecisions[0].position=100;
    test::throws([&]{validateDocument(moved);},"INVALID_POSITION");
}
TEST(stationrouting, different_traces_to_one_destination_keep_their_weights) {
    ProjectDocument d;d.definition=AuthoringDefinition{};
    const auto a=addLink(d,{{0,0},{100,0}},1,3.5);
    const auto b=addLink(d,{{130,0},{230,0}},1,3.5);
    const auto c=addLink(d,{{130,50},{230,50}},1,3.5);
    const auto end=addLink(d,{{270,0},{370,0}},1,3.5);
    const auto lane=[&](int k){return LaneReference{d.network.links[k].id,d.network.links[k].lanes[0].id};};
    const auto ab=addConnector(d,lane(0),lane(1)),ac=addConnector(d,lane(0),lane(2));
    const auto be=addConnector(d,lane(1),lane(3)),ce=addConnector(d,lane(2),lane(3));
    const auto r1=putRoute(d,{{},{a,ab,b,be,end}}),r2=putRoute(d,{{},{a,ac,c,ce,end}});
    RoutingDecision decision;decision.id="point";decision.linkId=a;decision.position=40;decision.routes={{r1,3},{r2,1}};
    putRoutingDecision(d,decision);putInput(d,{"in","","car",600,0,60,{},{},{},{},a});
    const auto s=compileDocument(d,test::root()/"data").scenario;
    CHECK(s.routeDecisions.size()==2);
    for(const auto& gate:s.routeDecisions) {
        CHECK(gate.choices.size()==2);CHECK(gate.choices[0].routeId!=gate.choices[1].routeId);
        test::near(gate.choices[0].weight,3);test::near(gate.choices[1].weight,1);
    }
}
