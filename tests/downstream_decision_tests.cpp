#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/eval/lane_changes.hpp"
#include "../src/project/demand_paths.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <map>
using namespace trafficsim;
// D93 (M3_8_CONTRACT.md §2, "Downstream routing decisions", A40-A46): a routing decision placed
// past the entry Link draws by destination, and a lane that cannot reach one changes lanes there.
namespace {
const auto data = [] { return test::root() / "data"; };
// Entry Link A feeds a two-lane Link D lane for lane; D lane 1 leaves for L, D lane 2 for R.
// A decision on D sends 60 to L and 40 to R. With `oneLaneEntry`, A has one lane, into D lane 1.
struct Drawing { ProjectDocument d; std::string a, link, left, right; };
Drawing downstream(bool oneLaneEntry = false) {
    Drawing w; w.d.definition = AuthoringDefinition{};
    auto& d = w.d;
    w.a = addLink(d, {{0, 0}, {100, 0}}, oneLaneEntry ? 1 : 2, 3.5);
    w.link = addLink(d, {{110, 0}, {310, 0}}, 2, 3.5);
    w.left = addLink(d, {{330, 10}, {430, 10}}, 1, 3.5);
    w.right = addLink(d, {{330, -10}, {430, -10}}, 1, 3.5);
    const auto lane = [&](const std::string& l, std::size_t k) { return fixture::detail::lane(d, l, k); };
    addConnector(d, {w.a, lane(w.a, 0)}, {w.link, lane(w.link, 0)});
    if (!oneLaneEntry) addConnector(d, {w.a, lane(w.a, 1)}, {w.link, lane(w.link, 1)});
    addConnector(d, {w.link, lane(w.link, 0)}, {w.left, lane(w.left, 0)});
    addConnector(d, {w.link, lane(w.link, 1)}, {w.right, lane(w.right, 0)});
    changeRunSettings(d, 900, 0.1);
    RoutingDecision decision; decision.linkId = w.link;
    decision.routes = {{"", 60, w.left}, {"", 40, w.right}};
    putRoutingDecision(d, decision);
    putInput(d, {"in", "", "car", 600, 0, 900, {}, {}, {}, {}, w.a});
    return w;
}
bool isStub(const Scenario& s, const std::string& route) {
    return std::any_of(s.routeDeadEnds.begin(), s.routeDeadEnds.end(), [&](const auto& x) { return x.routeId == route; });
}
const Route& routeOf(const Scenario& s, const std::string& id) {
    return *std::find_if(s.routes.begin(), s.routes.end(), [&](const auto& r) { return r.id == id; });
}
// A stub ends short of its destination: its vehicles arrive on the chain it changes to.
std::string arrivesOn(const Scenario& s, std::string id) {
    for (bool stub = true; stub;) {
        stub = false;
        for (const auto& span : s.laneChanges) if (span.fromRouteId == id) { id = span.toRouteId; stub = true; break; }
    }
    return id;
}
// The compiled volume by the Link a route ends on, every stub followed to where it arrives.
std::map<std::string, double> byExit(const Drawing& w, const Scenario& s) {
    std::map<std::string, double> result;
    for (const auto& input : s.inputs) {
        const auto& route = routeOf(s, arrivesOn(s, input.routeId));
        for (const auto& link : w.d.network.links)
            for (const auto& lane : link.lanes)
                if (route.segmentIds.back().rfind(lane.id, 0) == 0) result[link.id] += input.vehiclesPerHour;
    }
    return result;
}
bool onLinkOf(const Drawing& w, const std::string& linkId, const std::string& segment) {
    for (const auto& link : w.d.network.links)
        if (link.id == linkId)
            for (const auto& lane : link.lanes) if (segment.rfind(lane.id, 0) == 0) return true;
    return false;
}
// Route distance at which a route first reaches Link D.
double reachesD(const Drawing& w, const Scenario& s, const Route& route) {
    double at = 0;
    for (const auto& id : route.segmentIds) {
        if (onLinkOf(w, w.link, id)) return at;
        at += std::find_if(s.segments.begin(), s.segments.end(), [&](const auto& x) { return x.id == id; })->length;
    }
    return at;
}
}
TEST(routeless, a_downstream_decision_holds_its_proportions_exactly) { // A40
    const auto w = downstream();
    const auto s = compileDocument(w.d, data()).scenario;
    // The forcing: some of the volume rides a stub -- the lane-fixed draw would shift it.
    CHECK(std::any_of(s.inputs.begin(), s.inputs.end(), [&](const auto& i) { return isStub(s, i.routeId) && i.vehiclesPerHour > 0; }));
    const auto exits = byExit(w, s);
    test::near(exits.at(w.left), 360, 1e-9); test::near(exits.at(w.right), 240, 1e-9);
    const auto issues = routelessIssues(w.d.network, *w.d.definition);
    CHECK(issues.blocking.empty()); CHECK(issues.advisory.empty());
}
TEST(routeless, a_downstream_stub_changes_on_the_decision_link_and_arrives_on_its_movement) { // A41, A42
    const auto w = downstream();
    const auto snapshot = compileDocument(w.d, data());
    const auto& s = snapshot.scenario;
    // A42's forcing: a stub and its target start on the two lanes of A, side by side, so without
    // the clip their spans would reach back onto A.
    CHECK(!s.laneChanges.empty());
    for (const auto& span : s.laneChanges) {
        const auto& from = routeOf(s, span.fromRouteId);
        const auto& to = routeOf(s, span.toRouteId);
        CHECK(onLinkOf(w, w.a, from.segmentIds.front()) && onLinkOf(w, w.a, to.segmentIds.front()));
        CHECK(from.segmentIds.front() != to.segmentIds.front());
        CHECK(span.fromStart >= reachesD(w, s, from) - 1e-9); // A42: no span before D
        CHECK(span.toStart >= reachesD(w, s, to) - 1e-9);
    }
    const auto spec = evaluationSpec(w.d, snapshot, data());
    MovementAccumulator movements(spec);
    LaneChangeAccumulator changes(spec);
    std::uint64_t stubArrivals = 0;
    auto state = createSimulation(s, 42);
    movements.observe(state); changes.observe(state);
    while (state.tick < totalTicks(s)) {
        state = stepSimulation(std::move(state));
        movements.observe(state); changes.observe(state);
        for (const auto& e : state.events)
            if (const auto* a = std::get_if<ArrivedEvent>(&e)) stubArrivals += isStub(s, a->routeId);
    }
    const auto report = movements.report(state);
    CHECK(stubArrivals == 0); CHECK(report.unassigned == 0); CHECK(report.safetyClamps == 0);
    std::size_t carried = 0;
    for (const auto& row : report.movements) carried += row.vehicles > 0;
    CHECK(carried == 2); // A -> L and A -> R, each under its own entry
    std::uint64_t changed = 0;
    for (const auto& row : changes.report().rows) {
        changed += row.changes;
        for (double at : row.atDistance) CHECK(at >= 100); // never on A (100 m) -- A42
    }
    CHECK(changed > 0);
}
TEST(routeless, a_lane_no_entry_path_reaches_beside_stays_lane_fixed) { // A43
    const auto w = downstream(true);
    const auto placed = placedDecisions(w.d.network, *w.d.definition);
    const auto r = routelessChains(w.d.network, w.a, placed);
    // The forcing: nothing from A arrives on D's second lane.
    const auto second = fixture::detail::lane(w.d, w.link, 1);
    for (const auto& c : r.chains)
        CHECK(std::find(c.laneChain.begin(), c.laneChain.end(), second) == c.laneChain.end());
    CHECK(std::any_of(r.advisories.begin(), r.advisories.end(), [](const auto& i) { return i.code == "ROUTING_DECISION_LANE_FIXED"; }));
    const auto s = compileDocument(w.d, data()).scenario;
    CHECK(s.laneChanges.empty() && s.routeDeadEnds.empty());
    const auto exits = byExit(w, s);
    test::near(exits.at(w.left), 600, 1e-9); CHECK(!exits.contains(w.right));
}
TEST(routeless, a_downstream_stub_replays_exactly) { // A45
    const auto w = downstream();
    const auto s = compileDocument(w.d, data()).scenario;
    auto state = createSimulation(s, 7);
    for (int i = 0; i < 1500; ++i) state = stepSimulation(std::move(state));
    auto copy = state;
    std::vector<SimEvent> a, b;
    for (int i = 0; i < 1500; ++i) {
        state = stepSimulation(std::move(state)); copy = stepSimulation(std::move(copy));
        a.insert(a.end(), state.events.begin(), state.events.end());
        b.insert(b.end(), copy.events.begin(), copy.events.end());
    }
    CHECK(!a.empty()); CHECK(a == b); CHECK(state.vehicles == copy.vehicles);
}
namespace {
// A36 on D: the first stub route's changer 80 m short of its dead end at 10 m/s, beside a 10 m/s
// stream on its target with 7.5 m gaps. Steps until it leaves the stub (or 60 s pass).
struct Approach { double slowest{1e9}; bool changed{}; int clamps{}; };
Approach approach(bool cooperative) {
    const auto w = downstream();
    auto s = compileDocument(w.d, data()).scenario;
    if (!cooperative) for (auto& b : s.behaviours) b.maxDecelerationCooperativeBraking.reset();
    const auto& span = s.laneChanges.front();
    const auto deadEnd = std::find_if(s.routeDeadEnds.begin(), s.routeDeadEnds.end(),
                                      [&](const auto& x) { return x.routeId == span.fromRouteId; })->at;
    const double at = deadEnd - 80;
    const double mapped = span.toStart + (at - span.fromStart) * (span.toEnd - span.toStart) / (span.fromEnd - span.fromStart);
    std::vector<test::Placement> placed{{1, span.fromRouteId, at, 10}};
    for (std::uint64_t k = 0; k < 12; ++k) placed.push_back({10 + k, span.toRouteId, mapped + 70 - 12 * static_cast<double>(k), 10});
    auto state = test::withVehicles(s, placed);
    Approach r;
    const auto routeId = [&](const SimState& x) {
        for (const auto& v : x.vehicles) if (v.id == 1) return x.scenario->routes[v.routeIndex].id;
        return std::string{};
    };
    for (int i = 0; i < 600 && routeId(state) == span.fromRouteId; ++i) {
        for (const auto& v : state.vehicles) if (v.id == 1) r.slowest = std::min(r.slowest, v.speed);
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) r.clamps += std::holds_alternative<SafetyClampEvent>(e);
    }
    r.changed = !routeId(state).empty() && routeId(state) != span.fromRouteId;
    return r;
}
}
TEST(routeless, cooperation_acts_on_a_downstream_stub) { // A46
    // The forcing: without cooperative braking the changer comes to a stand at its dead end ...
    const auto off = approach(false);
    CHECK(off.slowest < kWaitingSpeed);
    // ... and the waiting rule still lets it in, with no clamp (A30/A35 on D).
    CHECK(off.changed && off.clamps == 0);
    // With it, it goes in before it stops (A36 on D).
    const auto on = approach(true);
    CHECK(on.changed && on.slowest >= kWaitingSpeed && on.clamps == 0);
}
