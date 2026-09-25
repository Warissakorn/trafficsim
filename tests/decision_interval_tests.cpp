#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/project/demand_paths.hpp"
#include "../src/project/run.hpp"
#include <map>
using namespace trafficsim;
// M2.1.2: turning proportions per counted interval on a routing decision (D45). Expanded at
// compile time like every other demand concept, so the core sees ordinary inputs.
namespace {
const auto data = [] { return test::root() / "data"; };
// The four-leg West approach: three routes leaving one Link, and its Links for a placed decision.
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
}
TEST(demand, an_unplaced_decision_splits_each_interval_by_its_own_counts) {
    auto w = west();
    RoutingDecision decision{"", "", {{w.routes[0], 4, {}, {300, 100}}, {w.routes[1], 4, {}, {100, 300}}}};
    decision.intervals = {{0, 900}, {900, 1800}};
    const auto id = putRoutingDecision(w.d, decision);
    VehicleInput input{"in", "", "car", 800, 0, 1800, {}, {}}; input.routingDecisionId = id;
    putInput(w.d, input);
    const auto split = withRoutingDecisions(*w.d.definition).inputs;
    CHECK(split.size() == 2);
    CHECK(split[0].routeId == w.routes[0]); CHECK(split[1].routeId == w.routes[1]);
    CHECK(split[0].intervals == (std::vector<VolumeInterval>{{0, 900, 600}, {900, 1800, 200}}));
    CHECK(split[1].intervals == (std::vector<VolumeInterval>{{0, 900, 200}, {900, 1800, 600}}));
    // Outside the counted intervals the whole-period flow holds: 4:4.
    auto later = *w.d.definition;
    later.inputs[0].startTime = 1800; later.inputs[0].endTime = 2700; later.duration = 2700;
    const auto after = withRoutingDecisions(later).inputs;
    CHECK(after.size() == 2);
    CHECK(after[0].intervals == (std::vector<VolumeInterval>{{1800, 2700, 400}}));
    // And it compiles and runs.
    CHECK(compileDocument(w.d, data()).scenario.inputs.size() > 0);
}
TEST(routeless, a_placed_decision_on_the_entry_follows_each_intervals_counts) {
    auto w = west();
    RoutingDecision decision; decision.linkId = w.upstream;
    decision.routes = {{"", 400, w.east, {300, 100}}, {"", 400, w.north, {100, 300}}, {"", 200, w.south, {100, 100}}};
    decision.intervals = {{0, 900}, {900, 1800}};
    putRoutingDecision(w.d, decision);
    putInput(w.d, {"in", "", "car", 1000, 0, 1800, {}, {}, {}, {}, w.upstream});
    auto& def = *w.d.definition;
    CHECK(routelessIssues(w.d.network, def).blocking.empty());
    // Which exit each runtime route ends on, from the walk in the first interval.
    const auto first = routelessChains(w.d.network, w.upstream, placedDecisions(w.d.network, def, nullptr, 450.0));
    CHECK(first.byDestination);
    std::map<std::string, std::string> exitOf;
    for (std::size_t k = 0; k < first.chains.size(); ++k)
        for (const auto& link : w.d.network.links) for (const auto& lane : link.lanes)
            if (lane.id == first.chains[k].laneChain.back()) exitOf[routelessRouteId(w.upstream, k, first.chains.size())] = link.id;
    const auto expanded = expandRouteless(w.d.network, def, withRoutingDecisions(def));
    std::map<std::pair<std::string, double>, double> byExit; // (exit, interval start) -> veh/h
    for (const auto& i : expanded.inputs) {
        CHECK(exitOf.count(i.routeId) == 1); // one runtime route per path, shared by both intervals
        for (const auto& p : i.intervals) byExit[{exitOf[i.routeId], p.startTime}] += p.vehiclesPerHour;
    }
    test::near(byExit[{w.east, 0}], 600, 1e-9); test::near(byExit[{w.east, 900}], 200, 1e-9);
    test::near(byExit[{w.north, 0}], 200, 1e-9); test::near(byExit[{w.north, 900}], 600, 1e-9);
    test::near(byExit[{w.south, 0}], 200, 1e-9); test::near(byExit[{w.south, 900}], 200, 1e-9);
    // Runs, and replays exactly.
    const auto s = compileDocument(w.d, data()).scenario;
    std::vector<std::string> a, b;
    runSimulation(s, 7, [&](const SimEvent& e) { if (const auto* x = std::get_if<ArrivedEvent>(&e)) a.push_back(x->routeId); }, false);
    runSimulation(s, 7, [&](const SimEvent& e) { if (const auto* x = std::get_if<ArrivedEvent>(&e)) b.push_back(x->routeId); }, false);
    CHECK(!a.empty()); CHECK(a == b);
}
TEST(demand, decision_intervals_are_validated) {
    auto w = west();
    const auto refused = [&](RoutingDecision x, const std::string& code) {
        auto def = *w.d.definition; x.id = "x"; def.routingDecisions.push_back(x);
        const auto issues = routingDecisionIssues(def);
        CHECK(std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code; }));
    };
    RoutingDecision ok{"", "", {{w.routes[0], 1, {}, {1, 2}}}}; ok.intervals = {{0, 900}, {900, 1800}};
    { auto def = *w.d.definition; ok.id = "x"; def.routingDecisions.push_back(ok); CHECK(routingDecisionIssues(def).empty()); }
    auto mismatch = ok; mismatch.routes[0].intervalFlows = {1}; refused(mismatch, "ROUTING_DECISION_INTERVALS");
    auto overlap = ok; overlap.intervals[1].startTime = 800; refused(overlap, "INVALID_INTERVAL");
    auto negative = ok; negative.routes[0].intervalFlows = {1, -1}; refused(negative, "INVALID_SHARE");
    // An interval with nothing counted is not refused (D46): it falls back, tested below.
    { auto def = *w.d.definition; auto empty = ok; empty.id = "x"; empty.routes[0].intervalFlows = {1, 0};
      def.routingDecisions.push_back(empty); CHECK(routingDecisionIssues(def).empty()); }
}
TEST(demand, decision_intervals_round_trip) {
    auto w = west();
    RoutingDecision decision{"", "turns", {{w.routes[0], 4, {}, {3, 1}}, {w.routes[1], 4, {}, {1, 3}}}};
    decision.intervals = {{0, 900}, {900, 1800}};
    putRoutingDecision(w.d, decision);
    const auto j = documentJson(w.d);
    CHECK(j["schemaVersion"] == 15);
    const auto back = parseDocument(j);
    CHECK(back.definition->routingDecisions == w.d.definition->routingDecisions);
}
TEST(demand, the_inputs_volume_is_split_by_counts_that_do_not_match_it) {
    // D46: the decision's counts are proportions of the INPUT's volume. Here they disagree with
    // it in total (input 1200 then 600 veh/h, counts 40+20 and 10+50 vehicles), in interval length
    // (inputs by 15 min, turns by 10 min from 300 s), in coverage (no turns counted before 300 s
    // or after 1500 s), and one interval has no turn counted at all.
    auto w = west();
    RoutingDecision decision{"", "", {{w.routes[0], 3, {}, {40, 10, 0}}, {w.routes[1], 1, {}, {20, 50, 0}}}};
    decision.intervals = {{300, 900}, {900, 1500}, {1500, 1800}};
    const auto id = putRoutingDecision(w.d, decision);
    VehicleInput input{"in", "", "car", 0, 0, 0, {}, {}}; input.routingDecisionId = id;
    input.intervals = {{0, 900, 1200}, {900, 1800, 600}};
    putInput(w.d, input);
    CHECK(routingDecisionIssues(*w.d.definition).empty());
    const auto split = withRoutingDecisions(*w.d.definition).inputs;
    CHECK(split.size() == 2);
    // Before 300 s and in the uncounted last interval: whole-period 3:1. Otherwise the counts.
    CHECK(split[0].intervals == (std::vector<VolumeInterval>{{0, 300, 900}, {300, 900, 800}, {900, 1500, 100}, {1500, 1800, 450}}));
    CHECK(split[1].intervals == (std::vector<VolumeInterval>{{0, 300, 300}, {300, 900, 400}, {900, 1500, 500}, {1500, 1800, 150}}));
    // Every vehicle of the input is still sent somewhere: the input's own total holds.
    double vehicles = 0;
    for (const auto& part : split) for (const auto& p : part.intervals) vehicles += p.vehiclesPerHour * (p.endTime - p.startTime) / 3600;
    test::near(vehicles, 1200 * 0.25 + 600 * 0.25, 1e-9);
    CHECK(compileDocument(w.d, data()).scenario.inputs.size() > 0);
}
