#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/demand_preview.hpp"
#include "../src/project/demand_paths.hpp"
#include "../tools/four_leg_network.hpp"
#include <map>
#include <limits>
#include <fstream>
using namespace trafficsim;
namespace {
ProjectDocument timedFile() {
    ProjectDocument d;const auto link=addLink(d,{{0,0},{400,0}},2,3.5);putRoute(d,{"route",{link}});changeRunSettings(d,120,.1);
    auto catalog=resolveDemandCatalog(AuthoringDefinition{},test::root()/"data");
    for(auto& b:catalog.behaviours)b.amberDeceleration.reset(); // M4.2 (D147): an owned amberDeceleration is schema 28 (amber tests); this fixture exercises the schemas before it.
    catalog.compositions={{"mix",{{"car",3},{"heavy-vehicle",1}}}};putDemandCatalog(d,catalog);
    VehicleInput input{"in","route","",600,0,120};input.compositionId="mix";putInput(d,input);return d;
}
Json shares(double car,double heavy) {Json a=Json::array();if(car>0)a.push_back({{"vehicleTypeId","car"},{"share",car}});if(heavy>0)a.push_back({{"vehicleTypeId","heavy-vehicle"},{"share",heavy}});return a;}
Json periods() {return Json::array({{{"startTime",20},{"endTime",40},{"types",shares(0,1)}},{{"startTime",60},{"endTime",100},{"types",shares(1,0)}}});}
double rate(const Scenario& s,double t,const std::string& type="") {double r=0;for(const auto& i:s.inputs)if(t>=i.startTime&&t<i.endTime&&(type.empty()||i.vehicleTypeId==type))r+=i.vehiclesPerHour;return r;}
}
TEST(demand, timed_compositions_round_trip_and_conserve_every_breakpoint) {
    auto j=documentJson(timedFile());j["definition"]["compositions"][0]["intervals"]=periods();
    const auto d=parseDocument(j);CHECK(documentJson(d)["definition"]["compositions"][0]["intervals"]==periods());
    const auto s=compileDocument(d,test::root()/"missing").scenario;
    for(double t:{0.,19.,20.,39.,40.,59.,60.,99.,100.,119.})test::near(rate(s,t),600);
    test::near(rate(s,20,"heavy-vehicle"),600);test::near(rate(s,40,"car"),450);test::near(rate(s,60,"car"),600);
    test::near(rate(s,120),0);const auto preview=previewDemand(d,test::root()/"missing");double total=0;for(const auto& row:preview.rows)total+=row.expectedVehicles;test::near(total,20);
    auto large=d;large.definition->compositions[0].intervals[0].types[0].share=1e308;
    CHECK(std::isfinite(large.definition->compositions[0].intervals[0].types[0].share));
    test::near(rate(compileDocument(large,test::root()/"missing").scenario,20,"heavy-vehicle"),600);
}
TEST(demand, timed_compositions_reject_invalid_unused_periods_atomically) {
    auto j=documentJson(timedFile());j["definition"]["inputs"]=Json::array();
    j["definition"]["compositions"][0]["intervals"]=periods();auto& ps=j["definition"]["compositions"][0]["intervals"];
    ps[1]["startTime"]=30;CHECK(ps[1]["startTime"]<ps[0]["endTime"]);test::throws([&]{parseDocument(j);},"INVALID_INTERVAL");
    ps[1]["startTime"]=60;ps[0]["types"]=Json::array();CHECK(ps[0]["types"].empty());test::throws([&]{parseDocument(j);},"INVALID_SHARE");
}
TEST(demand, type_routing_is_applied_after_composition_split) {
    auto d=timedFile();d.definition->routes.push_back({"second",d.definition->routes[0].segmentIds});
    auto j=documentJson(d);j["definition"]["routingDecisions"]=Json::array({{{"id","choose"},{"routes",Json::array({{{"routeId","route"},{"relativeFlow",1}},{{"routeId","second"},{"relativeFlow",1}}})},
        {"typeRules",Json::array({{{"vehicleTypeId","car"},{"relativeFlows",{1,0}},{"intervalFlows",Json::array({Json::array(),Json::array()})}},{{"vehicleTypeId","heavy-vehicle"},{"relativeFlows",{0,1}},{"intervalFlows",Json::array({Json::array(),Json::array()})}}})}}});
    j["definition"]["inputs"][0]["routeId"]="";j["definition"]["inputs"][0]["routingDecisionId"]="choose";
    auto loaded=parseDocument(j);CHECK(documentJson(loaded)["definition"]["routingDecisions"][0]["typeRules"]==j["definition"]["routingDecisions"][0]["typeRules"]);
    const auto s=compileDocument(loaded,test::root()/"missing").scenario;CHECK(!s.inputs.empty());
    for(const auto& i:s.inputs)CHECK(i.vehicleTypeId=="car"?i.routeId.starts_with("route/"):i.routeId.starts_with("second/"));
    test::near(rate(s,30),600);
}
TEST(demand, invalid_type_rule_vectors_are_not_silently_ignored) {
    auto d=timedFile();auto j=documentJson(d);
    j["definition"]["routingDecisions"]=Json::array({{{"id","choose"},{"routes",Json::array({{{"routeId","route"},{"relativeFlow",1}}})},
        {"typeRules",Json::array({{{"vehicleTypeId","car"},{"relativeFlows",{0}},{"intervalFlows",Json::array({Json::array()})}}})}}});
    CHECK(j["definition"]["routingDecisions"][0]["typeRules"][0]["relativeFlows"][0]==0);
    test::throws([&]{parseDocument(j);},"INVALID_SHARE");
}
TEST(demand, type_flows_have_half_open_periods_and_type_specific_zero_fallback) {
    auto d=timedFile();auto& def=*d.definition;
    def.compositions[0].intervals={{20,40,{{"heavy-vehicle",1}}},{60,100,{{"car",1}}}};
    def.inputs[0].intervals={{0,50,600},{70,120,1200}};deriveInputTotals(def.inputs[0]);
    def.routes.push_back({"second",def.routes[0].segmentIds});
    RoutingDecision decision{"choose","",{{"route",1,"",{1,0}},{"second",1,"",{0,1}}}};
    decision.intervals={{10,30},{80,90}};
    decision.typeRules={{"car",{0,1},{{2,0},{0,0}}}};def.routingDecisions={decision};
    def.inputs[0].routeId.clear();def.inputs[0].routingDecisionId=decision.id;
    const auto s=compileDocument(d,test::root()/"missing").scenario;
    for(double t:{0.,10.,19.,20.,29.,30.,39.,40.,49.})test::near(rate(s,t),600);
    for(double t:{70.,79.,80.,89.,90.,99.,100.,119.})test::near(rate(s,t),1200);
    for(double t:{50.,60.,69.,120.})test::near(rate(s,t),0);
    const auto routed=[&](double t,const std::string& type,const std::string& route){double r=0;for(const auto& i:s.inputs)if(t>=i.startTime&&t<i.endTime&&i.vehicleTypeId==type&&i.routeId.starts_with(route+"/"))r+=i.vehiclesPerHour;return r;};
    test::near(routed(10,"car","route"),450);test::near(routed(30,"car","second"),0);
    test::near(routed(40,"car","second"),450);test::near(routed(80,"car","second"),1200);
    // Missing heavy override uses defaults; car's all-zero interval uses its own whole-period 0:1.
    test::near(routed(20,"heavy-vehicle","route"),600);
    test::near(previewDemand(d,test::root()/"missing").expectedVehicles,25);
}
TEST(demand, placed_type_rules_keep_route_families_separate_and_replay) {
    auto built=fixture::fourLegIntersection();auto d=std::move(built.document);auto& def=*d.definition;
    const auto origin=def.routes[0].segmentIds.front();changeRunSettings(d,120,.1);
    auto catalog=resolveDemandCatalog(def,test::root()/"data");catalog.compositions={{"mix",{{"car",1},{"heavy-vehicle",1}}}};putDemandCatalog(d,catalog);
    RoutingDecision decision{"choose","",{{built.routes[0],1},{built.routes[1],1}},origin};
    decision.typeRules={{"car",{1,0},{{},{}}},{"heavy-vehicle",{0,1},{{},{}}}};def.routingDecisions={decision};
    def.inputs.clear();VehicleInput input{"in","","",600,0,120};input.linkId=origin;input.compositionId="mix";putInput(d,input);
    const auto s=compileDocument(d,test::root()/"data").scenario;test::near(rate(s,20),600);
    std::map<std::string,std::string> owner;
    for(const auto& i:s.inputs){CHECK(i.routeId.find("/type-")!=std::string::npos);auto [it,fresh]=owner.emplace(i.routeId,i.vehicleTypeId);CHECK(fresh||it->second==i.vehicleTypeId);}
    CHECK(!s.laneChanges.empty());
    std::size_t typedChanges=0;
    for(const auto& span:s.laneChanges) {
        const auto from=span.fromRouteId.find("/type-"),to=span.toRouteId.find("/type-");
        if(from==std::string::npos && to==std::string::npos)continue; // Unused authored-route families also compile.
        CHECK(from!=std::string::npos && to!=std::string::npos);CHECK(span.fromRouteId.substr(from)==span.toRouteId.substr(to));++typedChanges;
    }
    CHECK(typedChanges>0);
    for(const auto& i:s.inputs) {
        auto routeId=i.routeId;
        for(std::size_t guard=0;guard<s.routes.size();++guard) {
            const bool stub=std::any_of(s.routeDeadEnds.begin(),s.routeDeadEnds.end(),[&](const auto& end){return end.routeId==routeId;});
            if(!stub)break;
            const auto span=std::find_if(s.laneChanges.begin(),s.laneChanges.end(),[&](const auto& change){return change.fromRouteId==routeId;});
            CHECK(span!=s.laneChanges.end());routeId=span->toRouteId;
        }
        const auto route=std::find_if(s.routes.begin(),s.routes.end(),[&](const auto& r){return r.id==routeId;});CHECK(route!=s.routes.end());
        const auto authored=std::find_if(def.routes.begin(),def.routes.end(),[&](const auto& r){return r.id==built.routes[i.vehicleTypeId=="car"?0:1];});CHECK(authored!=def.routes.end());
        const auto link=std::find_if(d.network.links.begin(),d.network.links.end(),[&](const auto& l){return l.id==authored->segmentIds.back();});CHECK(link!=d.network.links.end());
        CHECK(std::any_of(link->lanes.begin(),link->lanes.end(),[&](const auto& lane){return route->segmentIds.back().starts_with(lane.id);}));
    }
    std::vector<std::string> a,b;runSimulation(s,42,[&](const SimEvent& e){a.push_back(eventJson(e).dump());});
    runSimulation(s,42,[&](const SimEvent& e){b.push_back(eventJson(e).dump());});CHECK(!a.empty());CHECK(a==b);
}
TEST(demand, unused_type_rules_and_period_members_reject_unknown_duplicate_and_infinite_values) {
    auto d=timedFile();d.definition->inputs.clear();RoutingDecision x{"choose","",{{"route",1}}};
    x.typeRules={{"missing",{1},{{}}}};d.definition->routingDecisions={x};CHECK(!d.definition->externalVehicleTypes);
    test::throws([&]{validateDocument(d);},"UNKNOWN_VEHICLE_TYPE");
    d.definition->routingDecisions[0].typeRules={{"car",{1},{{}}},{"car",{1},{{}}}};
    CHECK(d.definition->routingDecisions[0].typeRules.size()==2);test::throws([&]{validateDocument(d);},"DUPLICATE_ID");
    d.definition->routingDecisions[0].typeRules={{"car",{1},{{1}}}};
    CHECK(d.definition->routingDecisions[0].intervals.empty());test::throws([&]{validateDocument(d);},"ROUTING_DECISION_INTERVALS");
    d.definition->routingDecisions.clear();auto& c=d.definition->compositions[0];c.intervals={{10,20,{{"car",1},{"car",1}}}};
    test::throws([&]{validateDocument(d);},"DUPLICATE_ID");c.intervals[0].types={{"missing",1}};
    test::throws([&]{validateDocument(d);},"UNKNOWN_VEHICLE_TYPE");c.intervals[0].types={{"car",std::numeric_limits<double>::infinity()}};
    CHECK(!std::isfinite(c.intervals[0].types[0].share));test::throws([&]{validateDocument(d);},"INVALID_SHARE");
}
TEST(demand, timed_rule_history_rollback_and_legacy_save_versions) {
    auto d=timedFile();CHECK(documentJson(d)["schemaVersion"]==18);History h;h.reset(d);const auto original=documentJson(h.document());
    h.execute("timed",[](auto& candidate){candidate.definition->compositions[0].intervals={{10,30,{{"car",1}}}};});
    CHECK(documentJson(h.document())["schemaVersion"]==19);const auto changed=documentJson(h.document());const auto revision=h.document().revision;
    test::throws([&]{h.execute("broken",[](auto& candidate){candidate.definition->compositions[0].intervals.push_back({20,40,{{"car",1}}});});},"INVALID_INTERVAL");
    CHECK(h.document().revision==revision && documentJson(h.document())==changed);h.undo();CHECK(documentJson(h.document())==original);h.redo();CHECK(documentJson(h.document())==changed);
    CHECK(documentJson(parseDocument(changed))==changed);
}
TEST(demand, downstream_decisions_use_each_types_destinations) {
    ProjectDocument d;const auto a=addLink(d,{{0,0},{100,0}},1,3.5);
    const auto middle=addLink(d,{{110,0},{210,0}},1,3.5);
    const auto left=addLink(d,{{230,10},{330,10}},1,3.5),right=addLink(d,{{230,-10},{330,-10}},1,3.5);
    const auto lane=[&](const auto& id){return fixture::detail::lane(d,id,0);};
    addConnector(d,{a,lane(a)},{middle,lane(middle)});addConnector(d,{middle,lane(middle)},{left,lane(left)});addConnector(d,{middle,lane(middle)},{right,lane(right)});
    changeRunSettings(d,120,.1);auto catalog=resolveDemandCatalog(AuthoringDefinition{},test::root()/"data");
    catalog.compositions={{"mix",{{"car",1},{"heavy-vehicle",1}}}};putDemandCatalog(d,catalog);
    RoutingDecision x{"choose","",{{"",1,left,{1}},{"",1,right,{1}}},middle};x.intervals={{10,20}};
    x.typeRules={{"car",{1,0},{{0},{1}}},{"heavy-vehicle",{0,1},{{1},{0}}}};putRoutingDecision(d,x);
    VehicleInput input{"in","","",600,0,120};input.linkId=a;input.compositionId="mix";putInput(d,input);
    const auto s=compileDocument(d,test::root()/"missing").scenario;
    const auto exitRate=[&](double t,const std::string& type,const std::string& exit){double result=0;for(const auto& i:s.inputs)if(i.vehicleTypeId==type&&t>=i.startTime&&t<i.endTime){
        const auto r=std::find_if(s.routes.begin(),s.routes.end(),[&](const auto& route){return route.id==i.routeId;});CHECK(r!=s.routes.end());if(r->segmentIds.back()==lane(exit))result+=i.vehiclesPerHour;
    }return result;};
    test::near(exitRate(0,"car",left),300);test::near(exitRate(10,"car",right),300);
    test::near(exitRate(10,"heavy-vehicle",left),300);test::near(exitRate(20,"heavy-vehicle",right),300);
    for(double t:{0.,9.,10.,19.,20.,119.})test::near(rate(s,t),600);
}
TEST(demand, queued_arrivals_keep_the_scheduled_time_type_route_and_replay) {
    auto d=timedFile();auto& def=*d.definition;def.routes.push_back({"second",def.routes[0].segmentIds});
    RoutingDecision x{"choose","",{{"route",1,"",{1}},{"second",1,"",{1}}}};x.intervals={{0,20}};
    x.typeRules={{"car",{0,1},{{1},{0}}}};def.routingDecisions={x};def.inputs.clear();
    VehicleInput input{"in","","car",60000,0,40};input.routingDecisionId=x.id;putInput(d,input);def.duration=40;
    const auto s=compileDocument(d,test::root()/"missing").scenario;auto state=createSimulation(s,42);
    while(state.time<19.9)state=stepSimulation(std::move(state));
    std::size_t before=0;for(std::size_t k=0;k<state.inputs.size();++k)if(state.scenario->inputs[k].endTime==20)before+=state.inputs[k].queue.size();CHECK(before>0);
    while(state.time<25)state=stepSimulation(std::move(state));
    std::size_t retained=0;for(std::size_t k=0;k<state.inputs.size();++k)if(state.scenario->inputs[k].endTime==20)for(const auto& pending:state.inputs[k].queue) {
        CHECK(pending.scheduledTime<20);CHECK(state.scenario->routes[pending.routeIndex].id.starts_with("route/"));++retained;
    }CHECK(retained>0);
    std::vector<std::string> a,b;runSimulation(s,42,[&](const SimEvent& e){a.push_back(eventJson(e).dump());});runSimulation(s,42,[&](const SimEvent& e){b.push_back(eventJson(e).dump());});CHECK(!a.empty());CHECK(a==b);
}
TEST(demand, removing_a_route_remaps_complete_type_matrices_and_rejects_a_lost_only_destination) {
    auto d=timedFile();auto& def=*d.definition;def.inputs.clear();def.routes.push_back({"second",def.routes[0].segmentIds});
    RoutingDecision x{"choose","",{{"route",1},{"second",1}}};x.typeRules={{"car",{1,1},{{},{}}}};putRoutingDecision(d,x);
    History h;h.reset(d);h.execute("delete",[](auto& candidate){deleteRoute(candidate,"second");});
    CHECK(h.document().definition->routingDecisions[0].typeRules[0].relativeFlows==std::vector<double>{1});
    h.undo();h.execute("only",[](auto& candidate){candidate.definition->routingDecisions[0].typeRules[0].relativeFlows={0,1};});const auto before=documentJson(h.document());
    test::throws([&]{h.execute("delete",[](auto& candidate){deleteRoute(candidate,"second");});},"INVALID_SHARE");CHECK(documentJson(h.document())==before);
}
TEST(demand, type_only_routing_does_not_read_unused_external_composition_files) {
    auto d=timedFile();auto& def=*d.definition;def.externalCompositions=true;def.compositions.clear();
    def.inputs[0].compositionId.clear();def.inputs[0].vehicleTypeId="car";
    RoutingDecision x{"choose","",{{"route",1}}};x.typeRules={{"car",{1},{{}}}};def.routingDecisions={x};
    def.inputs[0].routeId.clear();def.inputs[0].routingDecisionId="choose";
    const auto dir=std::filesystem::temp_directory_path()/"trafficsim-unused-compositions-slice6";
    std::filesystem::remove_all(dir);std::filesystem::create_directories(dir/"compositions");
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}} cleanup{dir};
    std::ofstream broken(dir/"compositions"/"unused.json");broken<<"invalid json";broken.close();
    CHECK(std::filesystem::exists(dir/"compositions"/"unused.json"));test::throws([&]{loadCompositions(dir);},"EDIT_CATALOG_READ");
    const auto s=compileDocument(d,dir).scenario;test::near(rate(s,30),600);
}
