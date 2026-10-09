#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/project/run.hpp"
#include "../src/eval/summary.hpp"
#include <algorithm>
#include <fstream>
#include <map>
using namespace trafficsim;
// The drawing M1's done-condition asks for and M2's done-condition runs (M2_PLAN.md M2.0). Until
// this file, no test anywhere built a four-leg intersection, so nobody knew whether one would run.
namespace {
const auto kFile = "data/projects/four-leg-signalised.traffic.json";
Json committed() {
    std::ifstream file(test::root() / kFile); Json j; file >> j; return j;
}
// A runtime route is the authored id when it expands to one lane, `id/lane-N` otherwise.
std::string authoredRoute(const std::string& runtime) { return runtime.substr(0, runtime.find('/')); }
// Structure and strings exactly, numbers to 1e-9 m: Connector curves are computed geometry, and
// another compiler (MSVC in native.yml) may round a last bit differently. That is not drift.
bool same(const Json& a, const Json& b) {
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it)
            if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) {
        for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false;
        return true;
    }
    return a == b;
}
}
TEST(fourleg, committed_file_is_the_builder_output) {
    // The builder is the drawing's one source. A hand edit to the file, or a builder change
    // without regenerating it (trafficsim-four-leg-fixture), fails here rather than drifting.
    const auto built = documentJson(fixture::fourLegIntersection().document);
    CHECK(same(committed(), built));
    // The comparison can fail: a moved point is caught, not absorbed by the tolerance.
    auto moved = built; moved["network"]["links"][0]["geometry"][0]["x"] = built["network"]["links"][0]["geometry"][0]["x"].get<double>() + 0.01;
    CHECK(!same(committed(), moved));
}
TEST(fourleg, reopens_exactly) {
    const auto j = committed();
    CHECK(documentJson(parseDocument(j)) == j);
}
TEST(fourleg, runs_and_every_movement_delivers) {
    const auto d = parseDocument(committed());
    CHECK(d.network.links.size() == 12); CHECK(d.definition->routes.size() == 12);
    // Nothing blocks Run and nothing is even advised: a drawing an engineer would hand in.
    CHECK(runDiagnostics(d, test::root() / "data").empty());
    const auto snapshot = compileDocument(d, test::root() / "data");
    // The natural drawing: every turn meets its exit at the exit's start (M2.0.1, D35).
    CHECK(std::none_of(d.network.connectors.begin(), d.network.connectors.end(),
                       [](const auto& c) { return c.to.station.has_value(); }));
    // Each exit lane is fed by two movements -- through and a turn -- so one drawn-order rule
    // each: two lanes per exit, four exits.
    CHECK(snapshot.scenario.priorityRules.size() == 8);
    CHECK(std::all_of(snapshot.scenario.priorityRules.begin(), snapshot.scenario.priorityRules.end(),
                      [](const auto& r) { return r.id.find("/to/") != std::string::npos; }));
    std::map<std::string, int> arrived;
    std::vector<ArrivedEvent> trips;
    const auto end = runSimulation(snapshot.scenario, 42, [&](const SimEvent& e) {
        if (const auto* a = std::get_if<ArrivedEvent>(&e)) { arrived[authoredRoute(a->routeId)]++; trips.push_back(*a); }
    }, false);
    for (const auto& route : d.definition->routes) CHECK(arrived[route.id] > 0);
    // All demand entered the network; none is queued off it. The only exception is an arrival
    // due in the run's last tick, which the engine retains but cannot insert (M3.2.8b made one
    // appear here: the trajectories moved).
    for (std::size_t i = 0; i < end.inputs.size(); ++i)
        for (const auto& pending : end.inputs[i].queue)
            CHECK(pending.scheduledTime >= end.scenario->duration - end.scenario->timeStep);
    // Same scenario, same seed, same build: the same trips (hard rule 2).
    std::vector<ArrivedEvent> again;
    runSimulation(snapshot.scenario, 42, [&](const SimEvent& e) {
        if (const auto* a = std::get_if<ArrivedEvent>(&e)) again.push_back(*a);
    }, false);
    CHECK(trips == again);
}
TEST(fourleg, right_turn_reaches_the_pocket_only_through_its_named_entry) {
    const auto d = parseDocument(committed());
    const auto snapshot = compileDocument(d, test::root() / "data");
    // Twelve movements, one chain per upstream lane each (M3.2.8b): 24 routes. The turns reach
    // their Connector from one lane only, so each has one stub -- 8 -- and the through movements
    // reach from both. Were the pocket entry left implied, the inner upstream lane would be
    // ambiguous and dropped.
    CHECK(snapshot.scenario.routes.size() == 24);
    CHECK(snapshot.scenario.routeDeadEnds.size() == 8);
}
TEST(fourleg, the_natural_drawing_needed_no_more_than_start_of_lane_ordering) {
    // Before M2.0.1 the natural drawing was refused with 8 x UNSUPPORTED_MERGE: three movements
    // feeding one lane start, nobody arbitrating. The rule it needed is M3.1's, applied where it
    // was skipped. Without those rules the merge guard still fires -- it was narrowed by
    // construction, never removed.
    const auto d = parseDocument(committed());
    auto scenario = compileDocument(d, test::root() / "data").scenario;
    CHECK(validateScenario(scenario).empty());
    scenario.priorityRules.clear();
    const auto issues = validateScenario(scenario);
    CHECK(std::count_if(issues.begin(), issues.end(),
                        [](const auto& i) { return i.code == "UNSUPPORTED_MERGE"; }) == 8);
}
TEST(fourleg, the_staggered_drawing_still_runs) {
    // Turns joining part way along each exit: the lane has priority over the turn there.
    fixture::FourLegOptions staggered; staggered.leftArrival = 10; staggered.rightArrival = 20;
    const auto d = fixture::fourLegIntersection(staggered).document;
    CHECK(runDiagnostics(d, test::root() / "data").empty());
    const auto snapshot = compileDocument(d, test::root() / "data");
    CHECK(snapshot.scenario.priorityRules.size() == 8);
    std::map<std::string, int> arrived;
    runSimulation(snapshot.scenario, 42, [&](const SimEvent& e) {
        if (const auto* a = std::get_if<ArrivedEvent>(&e)) arrived[authoredRoute(a->routeId)]++;
    }, false);
    for (const auto& route : d.definition->routes) CHECK(arrived[route.id] > 0);
}
TEST(fourleg, a_lane_dropped_at_an_ambiguous_implied_step_is_reported) {
    // M2.0.2. The inner upstream lane reaches the pocket Link twice -- on through, and into the
    // pocket -- so a route naming only the two Links cannot say which, and drops that lane.
    auto d = fixture::fourLegIntersection().document;
    auto& route = d.definition->routes.front();
    const auto named = route.segmentIds;
    CHECK(named.size() == 5); // upstream, taper, pocket, through, exit
    auto implied = named; implied.erase(implied.begin() + 1);
    // The forcing: naming the taper carries both lanes; leaving it implied really does lose one.
    std::vector<std::string> dropped;
    CHECK(routeLaneChains(d.network, named, &dropped).size() == 2); CHECK(dropped.empty());
    CHECK(routeLaneChains(d.network, implied, &dropped).size() == 1);
    CHECK(dropped == std::vector<std::string>{fixture::detail::lane(d, named.front(), 1)});
    // The consequence: one advisory naming the route, and Run is not blocked by it.
    CHECK(runDiagnostics(d, test::root() / "data").empty());
    route.segmentIds = implied;
    const auto rows = runDiagnostics(d, test::root() / "data");
    CHECK(rows.size() == 1);
    CHECK(rows.front().code == "AMBIGUOUS_ROUTE_STEP"); CHECK(rows.front().path == "routes[0]");
    CHECK(rows.front().severity == DiagnosticSeverity::advisory);
    CHECK(compileDocument(d, test::root() / "data").scenario.routes.size() == 23);
}
namespace {
// Clamps of one four-leg run, and how many are a vehicle held at its stop line when the head is
// `held` (or the vehicle just behind one, halted with it). A clamp is emitted before the same
// step's moves, so `last` is the position it acted on.
struct ClampCount { int clamps{}, explained{}; };
ClampCount clampsAtTheLine(const Scenario& scenario, SignalColor held) {
    std::map<std::string, const SignalHead*> headOnSegment;
    for (const auto& head : scenario.signalHeads) headOnSegment[head.segmentId] = &head;
    std::map<std::string, SignalColor> color;
    std::map<std::uint64_t, MovedEvent> last;
    ClampCount count;
    std::vector<MovedEvent> atLine; // where, and when, each explained clamp held its vehicle
    runSimulation(scenario, 42, [&](const SimEvent& e) {
        if (const auto* s = std::get_if<SignalEvent>(&e)) color[s->signalId] = s->color;
        if (const auto* c = std::get_if<SafetyClampEvent>(&e)) {
            ++count.clamps;
            const auto& before = last[c->vehicleId];
            const auto head = headOnSegment.find(before.segmentId);
            if (head != headOnSegment.end() && color[head->second->id] == held &&
                head->second->position - before.position >= 0 &&
                head->second->position - before.position < 2) { ++count.explained; atLine.push_back(before); return; }
            // The same thing one vehicle back: its leader was just halted at the line, with no
            // warning, so it is halted behind it (seen since M3.2.8b moved trajectories).
            if (std::any_of(atLine.begin(), atLine.end(), [&](const auto& a) {
                    return a.segmentId == before.segmentId && a.position > before.position &&
                           a.position - before.position < 10 && before.time - a.time < 5; })) ++count.explained;
        }
        if (const auto* m = std::get_if<MovedEvent>(&e)) last[m->vehicleId] = *m;
    }, true);
    return count;
}
}
TEST(fourleg, every_safety_clamp_is_a_vehicle_caught_at_its_stop_line_by_amber) {
    // M2.0.3 / D36. With amber as red the run clamps a handful of vehicles, and every one is the
    // same thing: a vehicle within a couple of metres of its stop line at speed when the head
    // turns amber. The frozen TS baselines hold the same clamps (seeds 43 and 4294967295), which
    // is why M0 scenarios keep that rule. This pins the diagnosis on the project with the catalog
    // behaviour's amber decision switched off, as D36 ran it.
    auto scenario = compileDocument(parseDocument(committed()), test::root() / "data").scenario;
    for (auto& b : scenario.behaviours) b.amberDeceleration.reset();
    const auto d36 = clampsAtTheLine(scenario, SignalColor::amber);
    CHECK(d36.clamps > 0); // The forcing: there is something to explain.
    CHECK(d36.explained == d36.clamps);
}
TEST(fourleg, amber_stop_or_go_removes_the_amber_clamps) {
    // M4.2 (D147, AMBER.md): a driver who cannot stop at 3 m/s^2 goes on amber, and one who cannot
    // stop at all goes on red as at a priority rule, so the clamps D36 explained go. Any clamp left
    // must still be a vehicle at a line its head holds, never a priority rule or a merge.
    auto scenario = compileDocument(parseDocument(committed()), test::root() / "data").scenario;
    CHECK(scenario.behaviours[0].amberDeceleration); // the catalog decides at amber
    const auto now = clampsAtTheLine(scenario, SignalColor::red);
    for (auto& b : scenario.behaviours) b.amberDeceleration.reset();
    const auto d36 = clampsAtTheLine(scenario, SignalColor::amber);
    CHECK(d36.clamps > 0); // the forcing: seed 42 has 7 under D36
    CHECK(now.clamps == 0); CHECK(now.explained == now.clamps);
}
