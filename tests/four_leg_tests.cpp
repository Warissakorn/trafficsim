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
    // Each staggered arrival derives one M3.1 rule: a left and a right per exit.
    CHECK(snapshot.scenario.priorityRules.size() == 8);
    std::map<std::string, int> arrived;
    std::vector<ArrivedEvent> trips;
    const auto end = runSimulation(snapshot.scenario, 42, [&](const SimEvent& e) {
        if (const auto* a = std::get_if<ArrivedEvent>(&e)) { arrived[authoredRoute(a->routeId)]++; trips.push_back(*a); }
    }, false);
    for (const auto& route : d.definition->routes) CHECK(arrived[route.id] > 0);
    CHECK(pendingCount(end) == 0); // All demand entered the network; none is queued off it.
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
    // Twelve movements, and the four through movements expand to both through lanes: 16 routes.
    // Were the pocket entry left implied, the inner upstream lane would be ambiguous and dropped.
    CHECK(snapshot.scenario.routes.size() == 16);
}
TEST(fourleg, natural_drawing_is_refused_until_conflict_areas_exist) {
    // Every turn meeting its exit at the exit's start is how an engineer draws this in Vissim.
    // The engine has no conflict areas before M3, so three movements feeding one lane start is
    // a merge nobody arbitrates. M3 flips this test; until then it records the gap honestly.
    fixture::FourLegOptions natural; natural.leftArrival = natural.rightArrival = 0;
    const auto d = fixture::fourLegIntersection(natural).document;
    // The forcing: no Connector in this drawing arrives part way along a Link.
    CHECK(std::none_of(d.network.connectors.begin(), d.network.connectors.end(),
                       [](const auto& c) { return c.to.station.has_value(); }));
    const auto issues = runDiagnostics(d, test::root() / "data");
    CHECK(std::count_if(issues.begin(), issues.end(),
                        [](const auto& i) { return i.code == "UNSUPPORTED_MERGE"; }) == 8);
    test::throws([&] { compileDocument(d, test::root() / "data"); }, "UNSUPPORTED_MERGE");
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
    CHECK(compileDocument(d, test::root() / "data").scenario.routes.size() == 15);
}
TEST(fourleg, every_safety_clamp_is_a_vehicle_caught_at_its_stop_line_by_amber) {
    // M2.0.3. The run clamps a handful of vehicles, and every one is the same thing: a vehicle
    // within a couple of metres of its stop line at speed when the head turns amber. The engine
    // treats amber as red with no stop-or-go decision (SIMULATION.md), so a vehicle that cannot
    // stop is halted at the line by the clamp. The frozen TS baselines hold the same clamps
    // (seeds 43 and 4294967295), which is why fixing it is a decision, not a patch. This test
    // pins the diagnosis, so a clamp from anything else -- a priority rule, a merge -- fails here.
    const auto snapshot = compileDocument(parseDocument(committed()), test::root() / "data");
    std::map<std::string, const SignalHead*> headOnSegment;
    for (const auto& head : snapshot.scenario.signalHeads) headOnSegment[head.segmentId] = &head;
    std::map<std::string, SignalColor> color;
    std::map<std::uint64_t, MovedEvent> last;
    int clamps = 0, explained = 0;
    runSimulation(snapshot.scenario, 42, [&](const SimEvent& e) {
        if (const auto* s = std::get_if<SignalEvent>(&e)) color[s->signalId] = s->color;
        // A clamp is emitted before the same step's moves, so `last` is the position it acted on.
        if (const auto* c = std::get_if<SafetyClampEvent>(&e)) {
            ++clamps;
            const auto& before = last[c->vehicleId];
            const auto head = headOnSegment.find(before.segmentId);
            if (head != headOnSegment.end() && color[head->second->id] == SignalColor::amber &&
                head->second->position - before.position >= 0 &&
                head->second->position - before.position < 2) ++explained;
        }
        if (const auto* m = std::get_if<MovedEvent>(&e)) last[m->vehicleId] = *m;
    }, true);
    CHECK(clamps > 0); // The forcing: there is something to explain.
    CHECK(explained == clamps);
}
