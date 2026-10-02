#include "test.hpp"
#include "../tools/lane_change_network.hpp"
#include "../tools/lane_change_sweep.hpp"
#include <fstream>
#include <map>
using namespace trafficsim;
// D98: the lane-change lab (tools/lane_change_network.hpp) -- four scenes, each one lane-change
// situation. These pin what each scene compiles to, so a change to the lane-change compile shows
// up here as a named scene, and that the lab exercises D95 at all.
namespace {
const auto kFile = "data/projects/lane-change-lab.traffic.json";
Json committed() { std::ifstream f(test::root() / kFile); Json j; f >> j; return j; }
// Structure and strings exactly, numbers to 1e-9 m: Connector curves are computed geometry (as
// `fourleg` compares its file).
bool same(const Json& a, const Json& b) {
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it) if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) { for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false; return true; }
    return a == b;
}
// The compiled lab, with each runtime route's scene (its first Link), last Link and kind.
struct Compiled {
    fixture::LaneChangeLab lab;
    Scenario s;
    std::map<std::string, std::string> first, last;
    std::map<std::string, bool> stub;
    // Spans from a route of `scene`: from a stub (mandatory) or between full routes (discretionary).
    std::vector<LaneChangeSpan> spans(const std::string& scene, bool discretionary) const {
        std::vector<LaneChangeSpan> out;
        for (const auto& span : s.laneChanges)
            if (first.at(span.fromRouteId) == scene && stub.at(span.fromRouteId) != discretionary) out.push_back(span);
        return out;
    }
    std::size_t routes(const std::string& scene, bool isStub) const {
        std::size_t n = 0;
        for (const auto& [id, link] : first) if (link == scene && stub.at(id) == isStub) ++n;
        return n;
    }
};
Compiled compiled() {
    Compiled c{fixture::laneChangeLab(), {}, {}, {}, {}};
    c.s = compileDocument(c.lab.document, test::root() / "data").scenario;
    const auto table = runtimeSections(c.lab.document.network);
    const auto linkOf = [&](const std::string& section) {
        for (const auto& x : table.sections) if (x.id == section) return x.linkId;
        return std::string{};
    };
    for (const auto& r : c.s.routes) {
        c.first[r.id] = linkOf(r.segmentIds.front()); c.last[r.id] = linkOf(r.segmentIds.back()); c.stub[r.id] = false;
    }
    for (const auto& d : c.s.routeDeadEnds) c.stub[d.routeId] = true;
    return c;
}
const LaneChangeRow& row(const LaneChangeReport& r, const std::string& name) {
    for (const auto& x : r.rows) if (x.name == name) return x;
    throw std::invalid_argument("NO_ROW " + name);
}
}
TEST(lanelab, committed_file_is_the_builder_output) {
    const auto built = documentJson(fixture::laneChangeLab().document);
    CHECK(same(committed(), built));
    auto moved = built; moved["network"]["links"][0]["geometry"][0]["x"] = built["network"]["links"][0]["geometry"][0]["x"].get<double>() + 0.01;
    CHECK(!same(committed(), moved)); // the comparison can fail
    CHECK(documentJson(parseDocument(committed())) == committed()); // and the file reopens exactly
}
TEST(lanelab, every_scene_compiles_to_its_lane_change_situation) {
    const auto c = compiled();
    CHECK(validateScenario(c.s).empty());
    CHECK(c.s.conflictZones.empty()); // the scenes are lane changing alone: nothing crosses or merges
    // Overtaking and Three lanes: every lane a full route, discretionary spans between neighbours
    // both ways, and no stub.
    CHECK(c.routes(c.lab.overtaking, false) == 2 && c.routes(c.lab.overtaking, true) == 0);
    CHECK(c.spans(c.lab.overtaking, true).size() == 2 && c.spans(c.lab.overtaking, false).empty());
    CHECK(c.routes(c.lab.threeLanes, false) == 3 && c.routes(c.lab.threeLanes, true) == 0);
    CHECK(c.spans(c.lab.threeLanes, true).size() == 4 && c.spans(c.lab.threeLanes, false).empty());
    // Lane drop: the upstream second lane is a stub that must change, and with one full route there
    // is no discretionary span anywhere -- nothing reaches the downstream second lane (the D96
    // limitation D95 does not close). A fix shows up here as an intended change.
    CHECK(c.routes(c.lab.dropUpstream, true) == 1 && c.routes(c.lab.dropUpstream, false) == 1);
    CHECK(!c.spans(c.lab.dropUpstream, false).empty());
    CHECK(c.spans(c.lab.dropUpstream, true).empty());
    // Diverge: stubs change onto their exit's lanes; by choice only between the two full routes to
    // the 2-lane exit, never across exits.
    CHECK(!c.spans(c.lab.diverge, false).empty());
    const auto chosen = c.spans(c.lab.diverge, true);
    CHECK(!chosen.empty());
    for (const auto& span : chosen) CHECK(c.last.at(span.fromRouteId) == c.lab.exitB && c.last.at(span.toRouteId) == c.lab.exitB);
}
TEST(lanelab, the_catalog_makes_no_discretionary_change_and_a_threshold_does) {
    const auto data = test::root() / "data";
    const auto lab = fixture::laneChangeLab();
    auto snapshot = compileDocument(lab.document, data);
    // 300 s of the 900 s run keeps the test quick; inputs may not outlast the run.
    snapshot.scenario.duration = 300;
    for (auto& input : snapshot.scenario.inputs) input.endTime = std::min(input.endTime, 300.0);
    // The forcing: the catalog behaviour carries neither D95 field.
    for (const auto& b : snapshot.scenario.behaviours) CHECK(!b.discretionaryLaneChangeThreshold && !b.acceptedDecelerationTrailingVehicle);
    const auto absent = sweep::runLaneChanges(lab.document, snapshot, data, 42, std::nullopt);
    CHECK(absent.movements.completed > 0);
    for (const auto& r : absent.changes.rows) CHECK(r.discretionaryChanges == 0); // A51 on the lab
    const auto set = sweep::runLaneChanges(lab.document, snapshot, data, 42, 0.5);
    CHECK(row(set.changes, "Overtaking → Overtaking").discretionaryChanges > 0);
    CHECK(row(set.changes, "Three lanes → Three lanes").discretionaryChanges > 0);
    CHECK(row(set.changes, "Lane drop upstream → Lane drop downstream").discretionaryChanges == 0);
}
