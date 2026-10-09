#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/commands/history.hpp"
#include "../src/project/counted_volumes.hpp"
#include "../src/project/demand_paths.hpp"
#include "../src/project/run.hpp"
#include <map>
using namespace trafficsim;
// D142 (rows VC1-VC7 of plans/DEMAND_IMPROVEMENT.md §7): an input whose volume is its entry
// decision's turning counts, so a count sheet is typed once and each movement gets what was counted.
namespace {
const auto data = [] { return test::root() / "data"; };
struct West { ProjectDocument d; std::vector<std::string> routes; std::string upstream, east, north, south; };
West west() {
    auto built = fixture::fourLegIntersection();
    West w{std::move(built.document), {built.routes[0], built.routes[1], built.routes[2]}, {}, {}, {}, {}};
    const auto& def = *w.d.definition;
    w.upstream = def.routes[0].segmentIds.front(); w.east = def.routes[0].segmentIds.back();
    w.north = def.routes[1].segmentIds.back(); w.south = def.routes[2].segmentIds.back();
    w.d.definition->inputs.clear();
    changeRunSettings(w.d, 1800, 0.1);
    return w;
}
// Two 15-minute counts: left 40 then 10, through 20 then 50 vehicles.
std::string counted(West& w) {
    RoutingDecision decision{"", "turns", {{w.routes[0], 1, {}, {40, 10}}, {w.routes[1], 1, {}, {20, 50}}}};
    decision.intervals = {{0, 900}, {900, 1800}};
    const auto id = putRoutingDecision(w.d, decision);
    VehicleInput input{"in", "", "car", 999, 0, 1800, {}, {}}; input.routingDecisionId = id; input.volumeFromCounts = true;
    input.intervals = {{0, 1800, 5}}; // typed by mistake: the counts win
    putInput(w.d, input);
    return id;
}
double vehicles(const VolumeInterval& p) { return p.vehiclesPerHour * (p.endTime - p.startTime) / 3600; }
}
TEST(demandcounts, the_volume_is_the_counts_summed_per_interval) { // VC1
    auto w = west(); counted(w);
    const auto& input = w.d.definition->inputs[0];
    CHECK(input.intervals == (std::vector<VolumeInterval>{{0, 900, 240}, {900, 1800, 240}}));
    test::near(input.vehiclesPerHour, 240, 1e-12);
    CHECK(countedVolumeIssues(*w.d.definition).empty());
}
TEST(demandcounts, each_movement_gets_exactly_what_was_counted) { // VC2, unplaced
    auto w = west(); counted(w);
    const auto split = withRoutingDecisions(*w.d.definition).inputs;
    CHECK(split.size() == 2);
    CHECK(split[0].routeId == w.routes[0]);
    test::near(vehicles(split[0].intervals[0]), 40, 1e-9); test::near(vehicles(split[0].intervals[1]), 10, 1e-9);
    test::near(vehicles(split[1].intervals[0]), 20, 1e-9); test::near(vehicles(split[1].intervals[1]), 50, 1e-9);
}
TEST(demandcounts, a_placed_entry_decision_sends_its_counts) { // VC2, placed on the entry Link
    auto w = west();
    RoutingDecision decision; decision.linkId = w.upstream;
    decision.routes = {{"", 1, w.east, {30, 6}}, {"", 1, w.north, {12, 24}}, {"", 1, w.south, {3, 0}}};
    decision.intervals = {{0, 900}, {900, 1800}};
    putRoutingDecision(w.d, decision);
    VehicleInput input{"in", "", "car", 0, 0, 1800, {}, {}, {}, {}, w.upstream}; input.volumeFromCounts = true;
    putInput(w.d, input);
    const auto& def = *w.d.definition;
    CHECK(def.inputs[0].intervals == (std::vector<VolumeInterval>{{0, 900, 180}, {900, 1800, 120}}));
    const auto first = routelessChains(w.d.network, w.upstream, placedDecisions(w.d.network, def, nullptr, 450.0));
    std::map<std::string, std::string> exitOf;
    for (std::size_t k = 0; k < first.chains.size(); ++k) {
        const auto& c = first.chains[k];
        const auto id = routelessRouteId(w.upstream, k, first.chains.size());
        if (c.stub) { const auto& family = c.families.back().name; exitOf[id] = family.substr(family.find('>') + 1); continue; }
        for (const auto& link : w.d.network.links) for (const auto& lane : link.lanes)
            if (lane.id == c.laneChain.back()) exitOf[id] = link.id;
    }
    std::map<std::pair<std::string, double>, double> byExit; // (exit, interval start) -> vehicles
    for (const auto& i : expandRouteless(w.d.network, def, withRoutingDecisions(def)).inputs)
        for (const auto& p : i.intervals) byExit[{exitOf[i.routeId], p.startTime}] += vehicles(p);
    test::near(byExit[{w.east, 0}], 30, 1e-9); test::near(byExit[{w.east, 900}], 6, 1e-9);
    test::near(byExit[{w.north, 0}], 12, 1e-9); test::near(byExit[{w.north, 900}], 24, 1e-9);
    test::near(byExit[{w.south, 0}], 3, 1e-9); test::near(byExit[{w.south, 900}], 0, 1e-9);
    CHECK(compileDocument(w.d, data()).scenario.inputs.size() > 0);
}
TEST(demandcounts, editing_the_counts_moves_the_volume_in_one_undo_step) { // VC3
    auto w = west(); const auto id = counted(w);
    History history; history.reset(w.d);
    const auto before = history.document();
    CHECK(history.execute("counts", [&](ProjectDocument& d) {
        for (auto& x : d.definition->routingDecisions) if (x.id == id) x.routes[0].intervalFlows = {100, 10};
    }));
    CHECK(history.document().definition->inputs[0].intervals[0] == (VolumeInterval{0, 900, 480}));
    history.undo();
    CHECK(history.document() == before);
    CHECK(history.document().definition->inputs[0].intervals[0] == (VolumeInterval{0, 900, 240}));
}
TEST(demandcounts, off_the_input_volume_stays_the_authority) { // VC4 (D46)
    auto w = west(); counted(w);
    w.d.definition->inputs[0].volumeFromCounts = false;
    w.d.definition->inputs[0].intervals = {{0, 1800, 600}};
    putInput(w.d, w.d.definition->inputs[0]);
    CHECK(w.d.definition->inputs[0].intervals == (std::vector<VolumeInterval>{{0, 1800, 600}}));
    CHECK(documentJson(w.d)["schemaVersion"] == 17);
}
TEST(demandcounts, schema_26_round_trips_and_older_files_refuse_the_key) { // VC5
    auto w = west(); counted(w);
    const auto j = documentJson(w.d);
    CHECK(j["schemaVersion"] == 26);
    const auto& written = j["definition"]["inputs"][0];
    CHECK(written["volumeFromCounts"] == true); CHECK(!written.contains("intervals")); // one truth in the file
    const auto back = parseDocument(j);
    CHECK(back.definition->inputs == w.d.definition->inputs);
    auto old = j; old["schemaVersion"] = 25;
    test::throws([&] { parseDocument(old); }, "EDIT_UNSUPPORTED_FIELD");
    auto newer = j; newer["schemaVersion"] = 27;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
    auto typo = j; typo["definition"]["inputs"][0]["volumeFromCounts"] = "yes";
    test::throws([&] { parseDocument(typo); }, "true or false");
}
TEST(demandcounts, inputs_without_usable_counts_are_named) { // VC6
    auto w = west();
    VehicleInput routed{"a", w.routes[0], "car", 100, 0, 1800, {}, {}}; routed.volumeFromCounts = true;
    w.d.definition->inputs = {routed};
    auto issues = countedVolumeIssues(*w.d.definition);
    CHECK(issues.size() == 1); CHECK(issues[0].code == "INPUT_COUNTS_NO_DECISION");
    RoutingDecision uncounted{"", "", {{w.routes[0], 1, {}, {}}, {w.routes[1], 1, {}, {}}}};
    const auto id = putRoutingDecision(w.d, uncounted);
    w.d.definition->inputs[0].routeId.clear(); w.d.definition->inputs[0].routingDecisionId = id;
    issues = countedVolumeIssues(*w.d.definition);
    CHECK(issues.size() == 1); CHECK(issues[0].code == "INPUT_COUNTS_EMPTY");
    test::throws([&] { validateDocument(w.d); }, "INPUT_COUNTS_EMPTY");
    RoutingDecision positioned; positioned.id = "station"; positioned.linkId = w.upstream; positioned.position = 10;
    positioned.routes = {{"", 1, w.east, {5}}}; positioned.intervals = {{0, 900}};
    w.d.definition->routingDecisions = {positioned};
    w.d.definition->inputs[0].routingDecisionId = "station";
    issues = countedVolumeIssues(*w.d.definition);
    CHECK(issues.size() == 1); CHECK(issues[0].code == "INPUT_COUNTS_POSITIONED");
}
TEST(demandcounts, type_rules_change_the_split_not_the_total) { // VC7
    auto w = west(); const auto id = counted(w);
    for (auto& x : w.d.definition->routingDecisions)
        if (x.id == id) x.typeRules = {RoutingTypeRule{"car", {1, 0}, {{1, 0}, {1, 0}}}};
    putRoutingDecision(w.d, w.d.definition->routingDecisions[0]);
    CHECK(w.d.definition->inputs[0].intervals == (std::vector<VolumeInterval>{{0, 900, 240}, {900, 1800, 240}}));
}
