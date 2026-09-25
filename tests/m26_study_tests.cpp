#include "test.hpp"
#include "../tools/m26_study_network.hpp"
#include "../src/project/run.hpp"
#include <fstream>
#include <map>
using namespace trafficsim;
// The M2.6 study template (tools/m26_study_network.hpp): pockets, the Thai left turn at all times,
// the owner's timing plan and interval-counted demand. Placeholder volumes; not the gate study.
namespace {
const auto kFile = "data/projects/m2.6-study-template.traffic.json";
Json committed() {
    std::ifstream file(test::root() / kFile); Json j; file >> j; return j;
}
bool same(const Json& a, const Json& b) { // numbers to 1e-9, as fourleg: MSVC may round a last bit
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
struct Outcome { std::size_t pending{}; std::map<std::string, int> arrived; };
Outcome run(const Scenario& scenario) {
    Outcome out;
    const auto end = runSimulation(scenario, 42, [&](const SimEvent& e) {
        if (const auto* a = std::get_if<ArrivedEvent>(&e)) out.arrived[a->routeId]++;
    }, false);
    out.pending = pendingCount(end);
    return out;
}
}
TEST(m26study, committed_file_is_the_builder_output_and_reopens_exactly) {
    CHECK(same(committed(), documentJson(fixture::m26StudyTemplate())));
    CHECK(documentJson(parseDocument(committed())) == committed());
}
TEST(m26study, the_left_turn_leaves_before_the_stop_line) {
    const auto d = parseDocument(committed());
    int bypassing = 0;
    for (const auto& c : d.network.connectors) {
        if (c.name.find(" left") == std::string::npos) continue;
        for (const auto& head : d.network.signalHeads)
            if (head.lane.laneId == c.from.laneId && c.from.station && *c.from.station < head.position)
                ++bypassing;
    }
    CHECK(bypassing == 4);
}
TEST(m26study, runs_an_hour_with_every_movement_delivering) {
    const auto d = parseDocument(committed());
    CHECK(d.definition->duration == 3600);
    CHECK(runDiagnostics(d, test::root() / "data").empty());
    const auto out = run(compileDocument(d, test::root() / "data").scenario);
    CHECK(out.pending == 0);
    std::map<std::string, int> movements; // entry Link -> distinct runtime paths that delivered
    for (const auto& [route, count] : out.arrived) if (count > 0) movements[route]++;
    CHECK(movements.size() >= 12);
}
TEST(m26study, a_minor_vehicle_held_at_the_join_itself_deadlocks_the_merge) {
    // Why a derived rule's stop line sits short of the join (sections.cpp, kYieldClearance). The
    // forcing: put every derived stop line back ON the join, and check it moved.
    auto scenario = compileDocument(parseDocument(committed()), test::root() / "data").scenario;
    CHECK(!scenario.priorityRules.empty());
    int moved = 0;
    for (auto& rule : scenario.priorityRules)
        for (const auto& segment : scenario.segments)
            if (segment.id == rule.yieldSegmentId && rule.yieldPosition < segment.length) {
                rule.yieldPosition = segment.length; ++moved;
            }
    CHECK(moved == static_cast<int>(scenario.priorityRules.size()));
    // The consequence: a waiting left turn's front is on the shared lane, the through vehicle
    // behind it stops inside the headway, and demand backs up off the network.
    CHECK(run(scenario).pending > 0);
}
