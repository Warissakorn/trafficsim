// D105: reproducible diagnosis and 40-seed verification, not calibration or an M5 batch API.
// --trace emits the clamp's pre-step state; --sweep verifies replay, accounting and geometry.
#include "t_junction_sweep.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include <bit>
#include <iostream>
#include <set>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
const Vehicle* find(const SimState& s, std::uint64_t id) {
    const auto v = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& x) { return x.id == id; });
    return v == s.vehicles.end() ? nullptr : &*v;
}
// A vehicle inserted this tick has no pre-step public snapshot. Insertion starts it at rest
// at route distance zero. Reconstruct that origin only after confirming its departure event.
Vehicle clampStart(const SimState& before, const SimState& after, std::uint64_t id) {
    if (const auto* v = find(before, id)) return *v;
    const auto* v = find(after, id);
    require(v && v->speed == 0, "clamped insertion missing or still moving");
    require(std::any_of(after.events.begin(), after.events.end(), [&](const auto& e) {
        const auto* d = std::get_if<DepartedEvent>(&e); return d && d->vehicleId == id;
    }), "clamp without snapshot or insertion event");
    auto start = *v; start.distance = 0; start.speed = 0; start.acceleration = 0;
    start.mode = FollowingMode::free; return start;
}
struct Nearest { std::optional<Leader> leader; std::uint64_t id{}; };
Nearest nearest(const SimState& s, const Vehicle& v) {
    Nearest result;
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index);
    for (const auto& p : s.index->parts[v.routeIndex]) {
        if (p.start + p.length < v.distance) continue;
        for (const auto& span : spans) {
            if (span.segmentIndex != p.segmentIndex || span.vehicleId == v.id ||
                p.start + span.front < v.distance - 1e-9) continue;
            const double gap = p.start + span.rear - v.distance;
            if (!result.leader || gap < result.leader->gap) {
                result.leader = Leader{gap, span.speed}; result.id = span.vehicleId;
            }
        }
    }
    return result;
}
// Same-build trajectory digest, numerical zero normalized so -0 and +0 compare as C++ does.
// Replay separately compares full events and every mutable snapshot field exactly.
struct Digest {
    std::uint64_t value = 1469598103934665603ull;
    void number(std::uint64_t x) { for (int i = 0; i < 8; ++i) { value ^= (x >> (i * 8)) & 255; value *= 1099511628211ull; } }
    void real(double x) { number(std::bit_cast<std::uint64_t>(x == 0 ? 0.0 : x)); }
    void state(const SimState& s) {
        number(s.tick); real(s.time); number(s.randomState); number(s.nextVehicleId); number(s.completed);
        number(s.vehicles.size());
        for (const auto& v : s.vehicles) {
            number(v.id); number(v.routeIndex); number(v.typeIndex); number(v.inputIndex);
            real(v.scheduledTime); real(v.desiredSpeed); real(v.driverFactor);
            real(v.distance); real(v.speed); real(v.acceleration); number(static_cast<unsigned>(v.mode));
            number(v.lastLaneChange.has_value());
            if (v.lastLaneChange) { number(v.lastLaneChange->tick); number(v.lastLaneChange->fromRoute); }
        }
        for (const auto& i : s.inputs) {
            number(i.nextArrival.has_value()); if (i.nextArrival) real(*i.nextArrival);
            number(i.queue.size());
            for (const auto& v : i.queue) {
                number(v.id); number(v.routeIndex); number(v.typeIndex); number(v.inputIndex);
                real(v.scheduledTime); real(v.desiredSpeed); real(v.driverFactor);
            }
        }
        for (const auto& stop : s.stopService) { number(stop.vehicleId); real(stop.line); number(stop.since); }
    }
};
bool same(const SimState& a, const SimState& b) {
    return a.tick == b.tick && a.time == b.time && a.seed == b.seed && a.randomState == b.randomState &&
        a.nextVehicleId == b.nextVehicleId && a.completed == b.completed && a.vehicles == b.vehicles &&
        a.inputs == b.inputs && a.stopService == b.stopService && a.events == b.events;
}
void accounting(const SimState& s) {
    require(s.nextVehicleId - 1 == s.completed + s.vehicles.size() + pendingCount(s), "vehicle accounting");
}
void bodies(const SimState& s) {
    auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index);
    std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) {
        return a.segmentIndex != b.segmentIndex ? a.segmentIndex < b.segmentIndex : a.rear < b.rear;
    });
    for (std::size_t i = 1; i < spans.size(); ++i)
        if (spans[i - 1].segmentIndex == spans[i].segmentIndex && spans[i - 1].vehicleId != spans[i].vehicleId)
            require(spans[i - 1].front <= spans[i].rear + 1e-7, "vehicle bodies overlap on a segment");
}
void sweptCrossing(const SimState& before, const SimState& after, const std::string& zoneId) {
    bool major = false, minor = false;
    for (const auto& v : before.vehicles) {
        const auto* next = find(after, v.id);
        double to = next ? next->distance : before.index->parts[v.routeIndex].back().start +
                                              before.index->parts[v.routeIndex].back().length;
        const double length = before.scenario->vehicleTypes[v.typeIndex].length;
        for (const auto& rz : before.index->routeZones[v.routeIndex])
            if (before.scenario->conflictZones[rz.zoneIndex].id == zoneId &&
                to > rz.entryAt && v.distance - length < rz.exitAt)
                (rz.role == ZoneRole::major ? major : minor) = true;
    }
    require(!(major && minor), "swept crossing overlap");
}
void trace(std::ostream& out, const SimState& before, const SimState& after,
           std::uint32_t seed, double headway, const std::string& minorFirst) {
    for (const auto& e : after.events) if (const auto* c = std::get_if<SafetyClampEvent>(&e)) {
        const auto start = clampStart(before, after, c->vehicleId); const auto* v = &start;
        const auto& route = before.scenario->routes[v->routeIndex];
        const auto ref = resolveRefs(*before.scenario, std::vector<Vehicle>{*v}, *before.index).front();
        const auto& type = before.scenario->vehicleTypes[ref.type];
        const auto& behaviour = before.scenario->behaviours[ref.behaviour];
        nlohmann::ordered_json j{{"seed", seed}, {"headway", headway}, {"tick", before.tick},
            {"time", before.time}, {"vehicleId", v->id}, {"routeId", route.id},
            {"minor", route.segmentIds.front() == minorFirst}, {"insertedThisTick", find(before, v->id) == nullptr}, {"distance", v->distance}, {"speed", v->speed}};
        const auto n = nearest(before, *v);
        if (n.leader) {
            const auto f = followingAcceleration(v->speed, v->desiredSpeed, v->driverFactor, type, behaviour, n.leader);
            j["leader"] = {{"id", n.id}, {"gap", n.leader->gap}, {"speed", n.leader->speed},
                {"standstillDistance", behaviour.standstillDistance},
                {"allowedDistance", std::max(0.0, n.leader->gap - behaviour.standstillDistance)},
                {"followingAcceleration", f.acceleration}, {"candidateDistance", integrate(v->speed, f.acceleration, before.scenario->timeStep).distance}};
        }
        for (const auto& rz : before.index->routeZones[v->routeIndex])
            j["zones"].push_back({{"id", before.scenario->conflictZones[rz.zoneIndex].id},
                {"minor", rz.role == ZoneRole::minor}, {"lineGap", rz.waitAt - v->distance},
                {"entryGap", rz.entryAt - v->distance}});
        for (const auto& h : before.index->routeHeads[v->routeIndex])
            j["heads"].push_back({{"id", before.scenario->signalHeads[h.headIndex].id},
                {"gap", h.partStart + before.scenario->signalHeads[h.headIndex].position - v->distance}});
        out << j.dump() << '\n';
    }
}
void run(std::ostream& out, const std::filesystem::path& root, std::uint32_t seed,
         double headway, bool traced) {
    fixture::TJunctionOptions o; o.congestedMajor = true; o.headway = headway;
    const auto t = fixture::tJunction(o);
    const auto snap = compileDocument(t.document, root / "data");
    auto s = createSimulation(snap.scenario, seed), replay = s;
    MovementAccumulator m(evaluationSpec(t.document, snap, root / "data")); m.observe(s);
    Digest digest; digest.state(s);
    std::uint64_t minorClamps = 0, movingMinorClamps = 0, stoppedMinorClamps = 0;
    const auto minorFirst = std::find_if(t.document.network.links.begin(), t.document.network.links.end(),
        [&](const auto& l) { return l.id == t.minor; })->lanes.front().id;
    while (s.tick < totalTicks(*s.scenario)) {
        auto next = stepSimulation(s); replay = stepSimulation(std::move(replay));
        require(same(next, replay), "same-build replay differs"); accounting(next); bodies(next);
        sweptCrossing(s, next, "right-of-way/" + t.crossingArea);
        std::set<std::uint64_t> clamped;
        for (const auto& e : next.events) if (const auto* c = std::get_if<SafetyClampEvent>(&e)) {
            clamped.insert(c->vehicleId); const auto start = clampStart(s, next, c->vehicleId); const auto* v = &start;
            if (s.scenario->routes[v->routeIndex].segmentIds.front() == minorFirst) {
                ++minorClamps; (v->speed == 0 ? stoppedMinorClamps : movingMinorClamps)++;
            }
        }
        for (const auto& v : next.vehicles) if (!clamped.contains(v.id))
            require(v.acceleration >= -next.scenario->vehicleTypes[v.typeIndex].maxDeceleration - 1e-9,
                    "unreported excessive braking");
        if (traced) trace(out, s, next, seed, headway, minorFirst);
        m.observe(next); digest.state(next); s = std::move(next);
    }
    const auto r = m.report(s);
    if (!traced) {
        nlohmann::ordered_json j{{"seed", seed}, {"gapTime", 5}, {"headway", headway},
            {"generated", s.nextVehicleId - 1}, {"completed", r.completed}, {"active", r.active},
            {"pending", r.pending}, {"safetyClamps", r.safetyClamps}, {"minorClamps", minorClamps},
            {"movingMinorClamps", movingMinorClamps}, {"stoppedMinorClamps", stoppedMinorClamps},
            {"trajectoryDigest", digest.value}, {"checks", "replay/accounting/bodies/swept-crossing/braking"}};
        for (const auto& row : r.movements) j["movements"].push_back({{"name", row.name}, {"vehicles", row.vehicles}, {"meanDelay", row.meanDelay ? nlohmann::json(*row.meanDelay) : nlohmann::json(nullptr)}});
        for (const auto& row : r.queues) j["queues"].push_back({{"name", row.name}, {"mean", row.meanLength}, {"max", row.maxLength}});
        out << j.dump() << '\n';
    }
}
}
int main(int argc, char** argv) {
    try {
        const std::string mode = argc == 4 ? argv[1] : "";
        if (mode != "--trace" && mode != "--trace-all" && mode != "--sweep") {
            std::cerr << "usage: trafficsim-t-junction-clamps --trace|--trace-all|--sweep <out.jsonl> <repo root>\n"; return 2;
        }
        std::ofstream out(argv[2], std::ios::binary); require(bool(out), "cannot open output");
        for (const double h : {3.0, 7.0, 12.0})
            if (mode == "--trace") for (const auto seed : {42u, 43u}) run(out, argv[3], seed, h, true);
            else for (std::uint32_t seed = 42; seed <= 81; ++seed) run(out, argv[3], seed, h, mode == "--trace-all");
        require(bool(out), "cannot write output"); return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
