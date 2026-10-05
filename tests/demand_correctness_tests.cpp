#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/run.hpp"
#include "../src/project/demand_preview.hpp"
#include "../src/project/demand_paths.hpp"
#include "../tools/four_leg_network.hpp"
#include <limits>
using namespace trafficsim;
namespace {
ProjectDocument road() {
    ProjectDocument d;
    const auto link=addLink(d,{{0,0},{400,0}},2,3.5);
    const auto route=putRoute(d,{"route",{link}});
    putInput(d,{"input",route,"car",1000,0,120});
    return d;
}
}
TEST(demand, zero_lane_weight_keeps_that_lane_empty_and_preserves_volume) {
    auto d=road();d.definition->inputs.front().laneShares={1,0};
    CHECK(d.definition->inputs.front().laneShares.back()==0);
    const auto s=compileDocument(d,test::root()/"data").scenario;
    CHECK(s.inputs.size()==2);
    test::near(s.inputs[0].vehiclesPerHour,1000);test::near(s.inputs[1].vehiclesPerHour,0);
    CHECK(laneSplit({0,1},2,{1,0}).weighted);
}
TEST(demand, invalid_lane_weights_are_refused_without_changing_history) {
    for(const auto& weights:std::vector<std::vector<double>>{{0,0},{-1,1},{std::numeric_limits<double>::infinity(),1}}) {
        History h;h.reset(road());const auto before=documentJson(h.document());
        CHECK(weights[0]<=0 || !std::isfinite(weights[0]));
        test::throws([&]{h.execute("weights",[&](auto& d){d.definition->inputs.front().laneShares=weights;});},"INVALID_SHARE");
        CHECK(documentJson(h.document())==before);
    }
}
TEST(demand, stale_lane_weights_keep_the_default_and_invalid_values_never_fall_back) {
    const auto stale=laneSplit({0,1},2,{1,2,3});CHECK(!stale.weighted);
    test::near(stale.fraction[0],.5);test::near(stale.fraction[1],.5);
    test::throws([]{laneSplit({0,1},2,{-1,2,3});},"INVALID_SHARE");
}

TEST(demand, preview_integrates_compiled_intervals_with_gaps_and_run_horizon) {
    auto d=road();d.definition->duration=120;
    d.definition->inputs.front().intervals={{0,60,900},{90,120,1200}};
    deriveInputTotals(d.definition->inputs.front());
    const auto before=documentJson(d);const auto preview=previewDemand(d,test::root()/"data");
    CHECK(preview.revision==d.revision);CHECK(preview.rows.size()==4);
    test::near(preview.expectedVehicles,25);
    double total=0;
    for(const auto& row:preview.rows) {
        CHECK(row.entryLinkId==d.network.links.front().id);CHECK(!row.entryLaneId.empty());
        CHECK(row.endTime<=120);total+=row.expectedVehicles;
    }
    test::near(total,25);CHECK(documentJson(d)==before);
}
TEST(demand, stale_weights_are_visible_in_preview_and_do_not_change_total) {
    auto d=road();d.definition->inputs.front().laneShares={1,2,3};
    const auto preview=previewDemand(d,test::root()/"data");
    CHECK(preview.advisories.size()==1);CHECK(preview.advisories.front().code=="DEMAND_LANE_SHARES_STALE");
    CHECK(preview.advisories.front().path=="inputs[0].laneShares");
    test::near(preview.expectedVehicles,1000*120.0/3600);
}
TEST(demand, preview_refuses_invalid_inputs_instead_of_showing_partial_demand) {
    auto d=road();d.definition->inputs.front().vehicleTypeId="missing";
    test::throws([&]{previewDemand(d,test::root()/"data");},"UNKNOWN_VEHICLE_TYPE");
}

TEST(demand, preview_expands_composition_and_zero_lane_without_losing_total) {
    auto d=road();auto& input=d.definition->inputs.front();input.vehicleTypeId.clear();
    input.compositionId="urban-mixed";input.laneShares={1,0};
    const auto preview=previewDemand(d,test::root()/"data");
    double car=0,heavy=0;
    for(const auto& row:preview.rows) {
        if(row.vehicleTypeId=="car")car+=row.expectedVehicles;
        if(row.vehicleTypeId=="heavy-vehicle")heavy+=row.expectedVehicles;
    }
    test::near(car,1000*120.0/3600*.95);test::near(heavy,1000*120.0/3600*.05);
    test::near(preview.expectedVehicles,car+heavy);
}

TEST(demand, placed_entry_decision_reports_ignored_lane_weights_and_conserves_total) {
    auto built=fixture::fourLegIntersection();auto d=built.document;
    const auto routes=d.definition->routes;d.definition->inputs.clear();
    const auto link=routes.front().segmentIds.front();
    RoutingDecision decision{"entry","",{{routes[0].id,3},{routes[1].id,1}}};decision.linkId=link;
    putRoutingDecision(d,decision);
    VehicleInput input{"in","","car",800,0,900};input.linkId=link;input.laneShares={1,0};putInput(d,input);
    CHECK(!inputLanePolicy(d.network,*d.definition,input).acceptsShares);
    const auto preview=previewDemand(d,test::root()/"data");
    CHECK(preview.advisories.size()==1);CHECK(preview.advisories.front().code=="DEMAND_LANE_SHARES_IGNORED");
    test::near(preview.expectedVehicles,200);
    auto invalid=input;invalid.laneShares={0,0};
    test::throws([&]{putInput(d,invalid);validateDocument(d);},"INVALID_SHARE");
}
