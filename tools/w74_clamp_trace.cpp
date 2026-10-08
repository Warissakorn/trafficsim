// M3.3.3a: which obstacle, regime and step produces each safety clamp of the BA27 runs
// (docs/evidence/w74-clamps.md). Not in `check`: a diagnosis, like t_junction_clamps (D105).
//
//   trafficsim-w74-clamp-trace <w74 behaviour.json> <project.traffic.json> <data dir> <out.jsonl>
//
// One line per SafetyClampEvent, prototype and w74 x dt 0.1/0.25/0.5 x seeds 42-81. Each
// obstacle the tick's phase 1 caps a move by is recomputed from the pre-step snapshot with
// core's public functions; the clamp is attributed to the one setting the smallest allowance.
// The published move must equal that allowance, else the line says phase-2 (a later cap) or
// unexplained -- never dropped. Lane changes and route decisions inside the tick are flagged.
#include "w74_fixture_document.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/run.hpp"
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template <class E> bool has(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* x = std::get_if<E>(&e); return x && x->vehicleId == id;
    });
}
const char* regime(W74Regime r) {
    switch (r) {
    case W74Regime::free: return "free";
    case W74Regime::approaching: return "approaching";
    case W74Regime::following: return "following";
    case W74Regime::emergency: return "emergency";
    }
    return "?";
}
const char* mode(FollowingMode m) {
    switch (m) {
    case FollowingMode::free: return "free";
    case FollowingMode::approaching: return "approaching";
    case FollowingMode::following: return "following";
    case FollowingMode::braking: return "braking";
    }
    return "?";
}
const char* color(SignalColor c) { return c == SignalColor::red ? "red" : c == SignalColor::amber ? "amber" : "green"; }
nlohmann::ordered_json state(const std::optional<W74State>& w) {
    if (!w) return nullptr;
    return {{"regime", regime(w->regime)}, {"sign", w->sign}};
}
// The snapshot phase 1 saw: survivors, plus this tick's insertions at rest at distance zero.
std::vector<Vehicle> snapshot(const SimState& before, const SimState& after) {
    auto vehicles = before.vehicles;
    for (const auto& e : after.events) if (const auto* d = std::get_if<DepartedEvent>(&e)) {
        const auto v = std::find_if(after.vehicles.begin(), after.vehicles.end(), [&](const auto& x) { return x.id == d->vehicleId; });
        if (v == after.vehicles.end()) continue;
        auto start = *v; start.distance = 0; start.speed = 0; start.acceleration = 0;
        start.mode = FollowingMode::free; start.w74State.reset(); start.laneChangeTrace.clear();
        vehicles.push_back(start);
    }
    std::sort(vehicles.begin(), vehicles.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    return vehicles;
}
struct Obstacle { std::string kind; double allowance{}; nlohmann::ordered_json detail; };
void trace(std::ostream& out, const std::string& model, double dt, std::uint32_t seed,
           const SimState& before, const SimState& after) {
    if (std::none_of(after.events.begin(), after.events.end(), [](const auto& e) { return std::holds_alternative<SafetyClampEvent>(e); }))
        return;
    const auto& scenario = *before.scenario; const auto& index = *before.index;
    const auto vehicles = snapshot(before, after);
    const auto refs = resolveRefs(scenario, vehicles, index);
    const auto spans = occupiedSpans(scenario, vehicles, index, refs);
    const auto buckets = bucketSpans(spans, scenario.segments.size());
    const auto courtesy = index.laneChanges ? courtesyHolds(scenario, index, vehicles, refs, spans, buckets)
                                            : std::vector<CourtesyHold>{};
    const auto zones = summarizeZones(scenario, index, vehicles, refs);
    const auto service = index.stopZones ? refreshStops(scenario, index, vehicles, refs, before.stopService, before.tick)
                                         : std::vector<StopService>{};
    for (const auto& e : after.events) if (const auto* c = std::get_if<SafetyClampEvent>(&e)) {
        const auto at = std::find_if(vehicles.begin(), vehicles.end(), [&](const auto& x) { return x.id == c->vehicleId; });
        require(at != vehicles.end(), "clamped vehicle missing from the snapshot");
        const auto v = static_cast<std::size_t>(at - vehicles.begin());
        const auto& vehicle = *at; const auto& type = scenario.vehicleTypes[refs[v].type];
        const auto& behaviour = scenario.behaviours[refs[v].behaviour];
        const auto& parts = index.parts[refs[v].route];
        const bool inserted = has<DepartedEvent>(after, vehicle.id);
        nlohmann::ordered_json j{{"model", model}, {"dt", dt}, {"seed", seed}, {"tick", before.tick},
            {"time", before.time}, {"vehicleId", vehicle.id}, {"type", scenario.vehicleTypes[refs[v].type].id},
            {"route", scenario.routes[refs[v].route].id}, {"inserted", inserted},
            {"laneChange", has<LaneChangeEvent>(after, vehicle.id)}, {"distance", vehicle.distance},
            {"speed", vehicle.speed}, {"previous", state(vehicle.w74State)}};
        // Vehicle leader, exactly as closestVehicle picks it.
        std::optional<Leader> leader; std::uint64_t leaderId = 0;
        for (const auto& part : parts) {
            if (part.start + part.length < vehicle.distance) continue;
            for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
                const auto& span = spans[buckets.items[i]];
                if (span.vehicleId == vehicle.id || part.start + span.front < vehicle.distance - 1e-9) continue;
                const double gap = part.start + span.rear - vehicle.distance;
                if (!leader || gap < leader->gap) { leader = Leader{gap, span.speed, span.acceleration}; leaderId = span.vehicleId; }
            }
        }
        const auto vehicleLeader = leader;
        std::vector<Obstacle> obstacles;
        if (leader) obstacles.push_back({"leader", std::max(0.0, leader->gap - standstillGap(behaviour)),
            {{"id", leaderId}, {"gap", leader->gap}, {"speed", leader->speed}, {"acceleration", leader->acceleration},
             {"standstillGap", standstillGap(behaviour)}}});
        auto stopLine = [&](double gap) { if (!leader || gap < leader->gap) leader = Leader{gap, 0, 0}; };
        for (const auto& routeHead : index.routeHeads[refs[v].route]) {
            const auto& head = scenario.signalHeads[routeHead.headIndex];
            const double gap = routeHead.partStart + head.position - vehicle.distance;
            const auto colour = signalColorAt(scenario.signalPrograms[index.programOfHead[routeHead.headIndex]], before.time);
            if (gap < -1e-9 || colour == SignalColor::green) continue;
            obstacles.push_back({std::string("signal-") + color(colour), std::max(0.0, gap), {{"id", head.id}, {"gap", gap}}});
            stopLine(gap);
        }
        if (index.laneChanges)
            if (const double gap = std::max(0.0, index.deadEndOfRoute[refs[v].route] - vehicle.distance); std::isfinite(gap)) {
                obstacles.push_back({"dead-end", gap, {{"gap", gap}}}); stopLine(gap);
            }
        const bool yields = !courtesy.empty() && std::isfinite(courtesy[v].gap);
        if (yields && !courtesy[v].moving)
            obstacles.push_back({"courtesy-waiting", std::max(0.0, courtesy[v].gap - standstillGap(behaviour)), {{"gap", courtesy[v].gap}}});
        // Priority rules, as the tick reads them (simulation.cpp, phase 1).
        for (const auto& routeRule : index.routeRules[refs[v].route]) {
            const auto& rule = scenario.priorityRules[routeRule.ruleIndex];
            const double gap = routeRule.partStart + rule.yieldPosition - vehicle.distance;
            const auto conflict = index.conflictSegmentOfRule[routeRule.ruleIndex];
            if (gap < -1e-9 || conflict == SIZE_MAX) continue;
            const bool goes = committed(vehicle.speed, std::max(0.0, gap), type);
            bool giveWay = false;
            for (auto i = buckets.start[conflict]; i < buckets.start[conflict + 1] && !giveWay; ++i) {
                const auto& span = spans[buckets.items[i]];
                if (span.vehicleId == vehicle.id) continue;
                const double reach = rule.conflictPosition - span.front;
                if (reach < 0) { if (span.rear <= rule.conflictPosition) giveWay = true; }
                else if (goes) continue;
                else if (reach <= rule.headway) giveWay = true;
                else if (span.speed > 0 && reach / span.speed < rule.gapTime) giveWay = true;
            }
            if (!giveWay) continue;
            obstacles.push_back({"priority-rule", std::max(0.0, gap), {{"id", rule.id}, {"gap", gap}}});
            stopLine(gap);
        }
        const StopService* stop = nullptr;
        if (const auto s = std::find_if(service.begin(), service.end(), [&](const auto& x) { return x.vehicleId == vehicle.id; });
            s != service.end()) stop = &*s;
        if (!index.routeZones[refs[v].route].empty())
            if (const auto hold = zoneHold(scenario, index, zones, vehicle, refs[v], vehicleLeader, stop, before.tick)) {
                obstacles.push_back({"zone", *hold, {{"gap", *hold}}}); stopLine(*hold);
            }
        auto following = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, leader);
        std::string kept = "leader";
        if (yields) {
            auto held = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour,
                               Leader{courtesy[v].gap, courtesy[v].speed, courtesy[v].acceleration});
            if (courtesy[v].moving)
                held.acceleration = std::max(held.acceleration, -*behaviour.maxDecelerationCooperativeBraking);
            if (held.acceleration < following.acceleration) { following = held; kept = "courtesy"; }
        }
        const double candidate = integrate(vehicle.speed, following.acceleration, dt).distance;
        const auto binding = std::min_element(obstacles.begin(), obstacles.end(),
            [](const auto& a, const auto& b) { return a.allowance < b.allowance; });
        const auto published = std::find_if(after.vehicles.begin(), after.vehicles.end(), [&](const auto& x) { return x.id == vehicle.id; });
        // An arrival leaves the network in the same tick; then only the snapshot side is known.
        const std::optional<double> moved = published == after.vehicles.end() ? std::nullopt
            : std::optional<double>(published->distance - vehicle.distance);
        std::string category = "unexplained";
        if (has<LaneChangeEvent>(after, vehicle.id)) category = "lane-change-tick";
        else if (binding != obstacles.end() && candidate > binding->allowance &&
                 moved && std::abs(*moved - binding->allowance) <= 1e-9) category = binding->kind;
        else if (moved && (binding == obstacles.end() || *moved < binding->allowance - 1e-9) && *moved < candidate - 1e-9)
            category = "phase-2";
        j["category"] = category;
        j["proposed"] = {{"acceleration", following.acceleration}, {"mode", mode(following.mode)},
            {"w74", state(following.w74)}, {"kept", kept}, {"candidateDistance", candidate}};
        if (binding != obstacles.end()) {
            j["allowance"] = binding->allowance;
            j["neededDeceleration"] = binding->allowance > 0 ? vehicle.speed * vehicle.speed / (2 * binding->allowance)
                                                             : (vehicle.speed > 0 ? std::numeric_limits<double>::infinity() : 0.0);
        }
        j["maxDeceleration"] = type.maxDeceleration;
        j["moved"] = moved ? nlohmann::json(*moved) : nlohmann::json(nullptr);
        for (const auto& o : obstacles) j["obstacles"].push_back({{"kind", o.kind}, {"allowance", o.allowance}, {"detail", o.detail}});
        out << j.dump() << '\n';
    }
}
}
int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: trafficsim-w74-clamp-trace <w74 behaviour.json> <project> <data dir> <out.jsonl>\n";
        return 2;
    }
    try {
        const std::filesystem::path data = argv[3], path = argv[4];
        if (std::filesystem::exists(path)) throw std::invalid_argument("Output already exists: " + path.string());
        const auto w74 = w74fixture::behaviour(argv[1]);
        const auto base = w74fixture::base(argv[2], data);
        std::ofstream out(path, std::ios::binary);
        for (const std::string model : {"prototype", "w74"})
            for (const double dt : {0.1, 0.25, 0.5}) {
                const auto d = w74fixture::arm(base, model == "w74" ? &w74 : nullptr, dt);
                const auto snap = compileDocument(d, data);
                for (std::uint32_t seed = 42; seed <= 81; ++seed) {
                    auto s = createSimulation(snap.scenario, seed);
                    while (s.tick < totalTicks(snap.scenario)) {
                        auto next = stepSimulation(s);
                        trace(out, model, dt, seed, s, next);
                        s = std::move(next);
                    }
                    std::cerr << model << " dt " << dt << " seed " << seed << '\n';
                }
            }
        if (!out) throw std::runtime_error("Failed writing " + path.string());
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
