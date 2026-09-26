#pragma once
// Shared by the T-junction test files (M3.2.7): the compiled fixture, its zones, and one run
// through the evaluation the CLI and editor use. One copy (hard rule 3).
#include "test.hpp"
#include "../tools/t_junction_network.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include "../src/model/network/right_of_way.hpp"

namespace tjunction {
using namespace trafficsim;
inline Scenario compiled(const fixture::TJunction& t) {
    return compileDocument(t.document, test::root() / "data").scenario;
}
// The zone whose minor side starts on `segment`: a single-lane Connector's segment is its id.
inline const ConflictZone& zoneOn(const Scenario& s, const std::string& segment) {
    for (const auto& z : s.conflictZones) if (z.minor.segmentIds.front() == segment) return z;
    throw std::runtime_error("no zone on " + segment);
}
inline std::size_t zoneIndex(const Scenario& canonical, const std::string& id) {
    for (std::size_t k = 0; k < canonical.conflictZones.size(); ++k) if (canonical.conflictZones[k].id == id) return k;
    throw std::runtime_error("no zone " + id);
}
inline bool majorBlocks(const SimState& s, const std::string& zoneId) {
    return summarizeZones(*s.scenario, *s.index, s.vehicles, resolveRefs(*s.scenario, s.vehicles, *s.index))
        .at(zoneIndex(*s.scenario, zoneId)).majorBlocks;
}
// Is this vehicle's body, front back to rear, over [entry, exit] of `side` right now? For a side on
// one segment; the T-junction's crossing lies within a single section on each road.
inline bool over(const SimState& s, const Vehicle& v, const ZoneSide& side) {
    if (side.segmentIds.size() != 1) throw std::runtime_error("side spans a section cut");
    const auto at = locateVehicle(*s.scenario, v, *s.index);
    if (at.segmentId != side.segmentIds.front()) return false;
    const double length = s.scenario->vehicleTypes[v.typeIndex].length;
    return at.position > side.entry && at.position - length < side.exit;
}
struct Run { MovementReport report; bool bothSidesInside{}; int majorClamps{}, minorClamps{}; };
// One run of the whole document; also watches the crossing for both sides inside at once, and
// counts safety clamps by whether the clamped vehicle came from the minor approach or the major road.
inline Run run(const ProjectDocument& d, const fixture::TJunction& t, std::uint32_t seed = 42) {
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    MovementAccumulator m(evaluationSpec(d, snapshot, data));
    auto s = createSimulation(snapshot.scenario, seed);
    const auto zone = zoneOn(*s.scenario, t.crossingTurn);
    const auto fromMinor = [&](const Vehicle& v) {
        const auto& first = s.scenario->routes[v.routeIndex].segmentIds.front();
        return std::any_of(t.document.network.links.begin(), t.document.network.links.end(), [&](const auto& l) {
            return l.id == t.minor && l.lanes.front().id == first; });
    };
    Run r;
    m.observe(s);
    while (s.tick < totalTicks(*s.scenario)) {
        const auto before = s;
        s = stepSimulation(s);
        m.observe(s);
        bool minor = false, major = false;
        for (const auto& v : s.vehicles) { minor = minor || over(s, v, zone.minor); major = major || over(s, v, zone.major); }
        r.bothSidesInside = r.bothSidesInside || (minor && major);
        for (const auto& e : s.events)
            if (const auto* c = std::get_if<SafetyClampEvent>(&e))
                for (const auto& v : before.vehicles) if (v.id == c->vehicleId) (fromMinor(v) ? r.minorClamps : r.majorClamps)++;
    }
    r.report = m.report(s);
    return r;
}
inline Run run(const fixture::TJunction& t, std::uint32_t seed = 42) { return run(t.document, t, seed); }
inline const MovementRow& movement(const MovementReport& r, const std::string& name) {
    for (const auto& m : r.movements) if (m.name == name) return m;
    throw std::runtime_error("no movement " + name);
}
}
