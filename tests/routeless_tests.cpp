#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/project/demand_paths.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <map>
using namespace trafficsim;
// M2.1.1: vehicle inputs on a Link with no route, and routing decisions placed on a Link. The
// shares are pinned by hand from the drawing; the engine sees only static routes.
namespace {
const auto data = [] { return test::root() / "data"; };
std::map<std::string, double> volumes(const Scenario& s) {
    std::map<std::string, double> v;
    for (const auto& i : s.inputs) v[i.id] += i.vehiclesPerHour;
    return v;
}
double total(const Scenario& s) { double t = 0; for (const auto& i : s.inputs) t += i.vehiclesPerHour; return t; }
// Link A (one lane) with a Connector to each of B and C: a plain diverge.
ProjectDocument diverge() {
    ProjectDocument d; d.definition = AuthoringDefinition{};
    const auto a = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto b = addLink(d, {{120, 10}, {220, 10}}, 1, 3.5);
    const auto c = addLink(d, {{120, -10}, {220, -10}}, 1, 3.5);
    const auto lane = [&](const std::string& l) { return fixture::detail::lane(d, l, 0); };
    addConnector(d, {a, lane(a)}, {b, lane(b)});
    addConnector(d, {a, lane(a)}, {c, lane(c)});
    changeRunSettings(d, 300, 0.1);
    return d;
}
// The four-leg drawing with its West approach demand made routeless.
struct West { ProjectDocument d; std::string upstream, pocket, eastExit, northExit, southExit; };
West westRouteless() {
    auto built = fixture::fourLegIntersection();
    West w{built.document, {}, {}, {}, {}, {}};
    auto& def = *w.d.definition;
    const auto& through = def.routes[0].segmentIds;
    w.upstream = through[0]; w.pocket = through[2]; w.eastExit = through.back();
    w.northExit = def.routes[1].segmentIds.back(); w.southExit = def.routes[2].segmentIds.back();
    // The three West inputs become one Link input of the same total.
    std::erase_if(def.inputs, [&](const auto& i) { return i.routeId == def.routes[0].id || i.routeId == def.routes[1].id || i.routeId == def.routes[2].id; });
    putInput(w.d, {"west", "", "car", 720, 0, 900, {}, {}, {}, {}, w.upstream});
    return w;
}
}
TEST(routeless, a_link_with_no_way_out_is_one_path) {
    ProjectDocument d; d.definition = AuthoringDefinition{};
    const auto a = addLink(d, {{0, 0}, {150, 0}}, 1, 3.5);
    changeRunSettings(d, 120, 0.1);
    putInput(d, {"in", "", "car", 600, 0, 60, {}, {}, {}, {}, a});
    const auto r = routelessChains(d.network, a, {});
    CHECK(r.chains.size() == 1); CHECK(r.chains[0].share == 1); CHECK(r.issues.empty());
    const auto s = compileDocument(d, data()).scenario;
    CHECK(s.inputs.size() == 1); CHECK(s.inputs[0].id == "in"); CHECK(s.inputs[0].routeId == "link:" + a);
    CHECK(s.inputs[0].vehiclesPerHour == 600);
    CHECK(s.inputs[0].linkId.empty()); // authoring only: the core never sees it
}
TEST(routeless, an_equal_share_at_every_branch) {
    auto d = diverge();
    const auto a = d.network.links[0].id;
    putInput(d, {"in", "", "car", 800, 0, 300, {}, {}, {}, {}, a});
    const auto r = routelessChains(d.network, a, {});
    CHECK(r.chains.size() == 2);
    for (const auto& c : r.chains) CHECK(c.share == 0.5);
    const auto s = compileDocument(d, data()).scenario;
    CHECK(volumes(s) == (std::map<std::string, double>{{"in/path-1", 400}, {"in/path-2", 400}}));
    // Every vehicle leaves by one of the two, and replay is exact.
    std::map<std::string, int> arrived;
    const auto sink = [&](const SimEvent& e) { if (const auto* a = std::get_if<ArrivedEvent>(&e)) arrived[a->routeId]++; };
    runSimulation(s, 42, sink, false);
    CHECK(arrived.size() == 2);
}
TEST(routeless, lane_shares_weight_the_links_lanes) {
    ProjectDocument d; d.definition = AuthoringDefinition{};
    const auto a = addLink(d, {{0, 0}, {150, 0}}, 2, 3.5);
    changeRunSettings(d, 120, 0.1);
    putInput(d, {"in", "", "car", 400, 0, 60, {3, 1}, {}, {}, {}, a});
    const auto s = compileDocument(d, data()).scenario;
    CHECK(volumes(s) == (std::map<std::string, double>{{"in/path-1", 300}, {"in/path-2", 100}}));
}
TEST(routeless, the_four_leg_approach_by_hand) {
    auto w = westRouteless();
    const auto r = routelessChains(w.d.network, w.upstream, {});
    CHECK(r.issues.empty());
    // Kerb lane: onto pocket lane 0, then through or left. Inner lane: taper or pocket entry,
    // then through from pocket lane 1 or right from pocket lane 2. Per starting lane:
    std::map<std::string, double> byExit;
    for (const auto& c : r.chains) {
        const auto& last = c.laneChain.back();
        for (const auto& link : w.d.network.links)
            for (const auto& lane : link.lanes) if (lane.id == last) byExit[link.id] += 0.5 * c.share;
    }
    CHECK(r.chains.size() == 4);
    CHECK(byExit == (std::map<std::string, double>{{w.eastExit, 0.5}, {w.northExit, 0.25}, {w.southExit, 0.25}}));
    // Compiled, the approach total is kept, and the movements are the authored ones.
    const auto snapshot = compileDocument(w.d, data());
    test::near(total(snapshot.scenario), 720 + 450 + 110 + 90 + 250 + 60 + 50 + 220 + 50 + 40, 1e-9);
    const auto spec = evaluationSpec(w.d, snapshot, data());
    CHECK(spec.movementNames.size() == 12);
    CHECK(spec.movementOfRoute.size() == snapshot.scenario.routes.size());
    MovementAccumulator m(spec);
    auto state = createSimulation(snapshot.scenario, 42); m.observe(state);
    while (state.tick < totalTicks(snapshot.scenario)) { state = stepSimulation(state); m.observe(state); }
    const auto report = m.report(state);
    std::uint64_t trips = 0; for (const auto& row : report.movements) trips += row.vehicles;
    CHECK(report.unassigned == 0); CHECK(trips == report.completed);
    CHECK(report.movements[0].vehicles > 0); // West -> East, now from the routeless paths
}
TEST(routeless, a_placed_decision_splits_by_destination) {
    auto w = westRouteless();
    auto& def = *w.d.definition;
    RoutingDecision decision; decision.linkId = w.pocket;
    decision.routes = {{"", 3, w.eastExit}, {"", 1, w.northExit}};
    putRoutingDecision(w.d, decision);
    const auto placed = placedDecisions(w.d.network, def);
    CHECK(placed.size() == 1); CHECK(placed[0].destinations.size() == 2);
    const auto r = routelessChains(w.d.network, w.upstream, placed);
    std::map<std::string, double> byExit;
    for (const auto& c : r.chains)
        for (const auto& link : w.d.network.links)
            for (const auto& lane : link.lanes) if (lane.id == c.laneChain.back()) byExit[link.id] += 0.5 * c.share;
    // Pocket lane 0 reaches both (3:1); lane 1 only East; lane 2 neither, so it carries on
    // routeless to the South exit and the decision says so.
    CHECK(byExit.size() == 3);
    test::near(byExit[w.eastExit], 0.5 * 0.75 + 0.25); test::near(byExit[w.northExit], 0.5 * 0.25);
    test::near(byExit[w.southExit], 0.25);
    CHECK(r.advisories.size() == 1); CHECK(r.advisories[0].code == "ROUTING_DECISION_LANE_UNSERVED");
    const auto issues = routelessIssues(w.d.network, def);
    CHECK(issues.blocking.empty()); CHECK(issues.advisory.size() == 1);
    // An input naming a placed decision is a routeless input on its Link. The pocket Link is fed
    // from upstream, and the engine starts no input part way into the network, so this uses a
    // decision on the approach itself.
    RoutingDecision entry; entry.linkId = w.upstream; entry.routes = {{"", 1, w.eastExit}, {"", 1, w.southExit}};
    const auto entryId = putRoutingDecision(w.d, entry);
    auto viaDecision = w.d; auto viaLink = w.d;
    putInput(viaDecision, {"extra", "", "car", 100, 0, 900, {}, {}, {}, entryId, ""});
    putInput(viaLink, {"extra", "", "car", 100, 0, 900, {}, {}, {}, {}, w.upstream});
    CHECK(compileDocument(viaDecision, data()).scenario.inputs == compileDocument(viaLink, data()).scenario.inputs);
}
TEST(routeless, a_decision_on_the_entry_link_holds_its_proportions_exactly) {
    // Counted turning volumes typed into a decision on the approach itself. With no lane
    // changing, which lane a vehicle is in fixes where it can turn, so the decision chooses the
    // lanes: each destination's flow goes to the lanes that reach it, and the counts hold.
    auto w = westRouteless();
    RoutingDecision counted; counted.linkId = w.upstream;
    counted.routes = {{"", 500, w.eastExit}, {"", 120, w.northExit}, {"", 100, w.southExit}};
    putRoutingDecision(w.d, counted);
    const auto r = routelessChains(w.d.network, w.upstream, placedDecisions(w.d.network, *w.d.definition));
    CHECK(r.byDestination); CHECK(r.issues.empty());
    const auto s = compileDocument(w.d, data()).scenario;
    std::map<std::string, double> byExit;
    for (const auto& input : s.inputs) {
        if (input.id.rfind("west", 0) != 0) continue;
        const auto route = std::find_if(s.routes.begin(), s.routes.end(), [&](const auto& x) { return x.id == input.routeId; });
        for (const auto& link : w.d.network.links)
            for (const auto& lane : link.lanes)
                if (route->segmentIds.back().rfind(lane.id, 0) == 0) byExit[link.id] += input.vehiclesPerHour;
    }
    test::near(byExit[w.eastExit], 500, 1e-9); test::near(byExit[w.northExit], 120, 1e-9);
    test::near(byExit[w.southExit], 100, 1e-9);
}
TEST(routeless, problems_are_named_and_block_run) {
    auto w = westRouteless();
    // A destination the decision's Link cannot reach.
    RoutingDecision back; back.linkId = w.pocket; back.routes = {{"", 1, w.upstream}};
    auto d = w.d; putRoutingDecision(d, back);
    std::vector<ValidationIssue> issues; placedDecisions(d.network, *d.definition, &issues);
    CHECK(issues.size() == 1); CHECK(issues[0].code == "ROUTING_DECISION_UNREACHABLE");
    test::throws([&] { compileDocument(d, data()); }, "ROUTING_DECISION_UNREACHABLE");
    // Two decisions on one Link, and a destination on an unplaced decision.
    auto two = w.d; RoutingDecision a; a.linkId = w.pocket; a.routes = {{"", 1, w.eastExit}};
    putRoutingDecision(two, a); a.id.clear(); putRoutingDecision(two, a);
    CHECK(routingDecisionIssues(*two.definition).at(0).code == "ROUTING_DECISION_DUPLICATE_LINK");
    RoutingDecision unplaced; unplaced.routes = {{"", 1, w.eastExit}};
    auto loose = w.d; putRoutingDecision(loose, unplaced);
    CHECK(routingDecisionIssues(*loose.definition).at(0).code == "ROUTING_DECISION_NEEDS_LINK");
    // An input on a Link that does not exist.
    auto unknown = w.d; putInput(unknown, {"ghost", "", "car", 100, 0, 900, {}, {}, {}, {}, "no-such-link"});
    const auto blocking = routelessIssues(unknown.network, *unknown.definition).blocking;
    CHECK(blocking.size() == 1); CHECK(blocking[0].code == "UNKNOWN_LINK");
    test::throws([&] { compileDocument(unknown, data()); }, "UNKNOWN_LINK");
    // A route and a Link on one input is two answers to one question.
    auto both = w.d; both.definition->inputs.push_back({"both", both.definition->routes[3].id, "car", 1, 0, 900, {}, {}, {}, {}, w.upstream});
    CHECK(routingDecisionIssues(*both.definition).at(0).code == "INPUT_TARGET_CONFLICT");
}
TEST(routeless, a_loop_is_refused) {
    ProjectDocument d; d.definition = AuthoringDefinition{};
    const auto a = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto b = addLink(d, {{100, 20}, {0, 20}}, 1, 3.5);
    const auto lane = [&](const std::string& l) { return fixture::detail::lane(d, l, 0); };
    addConnector(d, {a, lane(a)}, {b, lane(b)});
    addConnector(d, {b, lane(b)}, {a, lane(a)});
    changeRunSettings(d, 120, 0.1);
    putInput(d, {"in", "", "car", 100, 0, 60, {}, {}, {}, {}, a});
    // The forcing: the drawing really is a closed loop, A -> B -> A.
    CHECK(routeChainTo(d.network, {a}, b).size() == 2); CHECK(routeChainTo(d.network, {b}, a).size() == 2);
    const auto r = routelessChains(d.network, a, {});
    CHECK(r.chains.empty()); CHECK(r.issues.size() == 1); CHECK(r.issues[0].code == "ROUTELESS_CYCLE");
    test::throws([&] { compileDocument(d, data()); }, "ROUTELESS_CYCLE");
}
TEST(routeless, round_trip_and_cascade) {
    auto w = westRouteless();
    RoutingDecision decision; decision.linkId = w.pocket; decision.name = "West turns";
    decision.routes = {{"", 3, w.eastExit}, {"", 1, w.northExit}};
    putRoutingDecision(w.d, decision);
    const auto j = documentJson(w.d);
    CHECK(j["schemaVersion"] == 15);
    const auto back = parseDocument(j);
    CHECK(back.definition->inputs == w.d.definition->inputs);
    CHECK(back.definition->routingDecisions == w.d.definition->routingDecisions);
    CHECK(documentJson(back) == j);
    // Deleting the destination Link drops that entry; deleting the upstream Link drops its input.
    auto cut = w.d;
    deleteObjects(cut, {w.northExit});
    CHECK(cut.definition->routingDecisions.at(0).routes.size() == 1);
    deleteObjects(cut, {w.upstream});
    CHECK(std::none_of(cut.definition->inputs.begin(), cut.definition->inputs.end(), [](const auto& i) { return i.id == "west"; }));
    deleteObjects(cut, {w.pocket});
    CHECK(cut.definition->routingDecisions.empty());
}
TEST(routeless, a_connector_drawn_just_short_of_the_end_leaves_from_the_end) {
    // D44: the author clicked near the end. First assert the drawing really keeps the stations.
    const auto split = [](double station) {
        ProjectDocument d; d.definition = AuthoringDefinition{};
        const auto a = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
        const auto b = addLink(d, {{120, 10}, {220, 10}}, 1, 3.5);
        const auto c = addLink(d, {{120, -10}, {220, -10}}, 1, 3.5);
        const auto lane = [&](const std::string& l) { return fixture::detail::lane(d, l, 0); };
        addConnector(d, {a, lane(a), station}, {b, lane(b)});
        addConnector(d, {a, lane(a), station}, {c, lane(c)});
        CHECK(d.network.connectors[0].from.station.has_value());
        return routelessChains(d.network, a, {}).chains.size();
    };
    CHECK(split(100 - kRoutelessStubLength + 1) == 2); // a 3.5 m remainder is not an exit
    CHECK(split(100 - kRoutelessStubLength - 1) == 3); // a 5.5 m one is: a third leave there
}
