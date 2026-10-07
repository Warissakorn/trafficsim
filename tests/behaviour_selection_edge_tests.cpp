#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/routing.hpp"
#include <algorithm>
#include <map>
#include <set>
using namespace trafficsim;
// M3.3.2b (D127) edge rows: BA14 (positioned routing, D119) and BA17 (Stop service, M3.2.5)
// under assigned behaviours. Sets differ in standstill only, so a lone vehicle's free motion
// (which reads followingTime) is the same under every set.
namespace {
DriverBehaviour behaviour(const std::string& id, double standstill) { return {id, standstill, 2, 3, 1.5, .2}; }
std::size_t slot(const std::vector<DriverBehaviour>& items, const std::string& id) {
    for (std::size_t i = 0; i < items.size(); ++i) if (items[i].id == id) return i;
    throw std::runtime_error("Missing fixture behaviour");
}
template<class E> bool has(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* x = std::get_if<E>(&e); return x && x->vehicleId == id;
    });
}
// BA14: road (100) -> left | right; lines at 0 (source), 20 and 60 on both routes.
Scenario fork(bool assigned, std::vector<double> lines = {0, 20, 60}) {
    Scenario s; s.duration = 120; s.timeStep = .1;
    s.segments = {{"road", 100, {"left", "right"}}, {"left", 100, {}}, {"right", 100, {}}};
    s.routes = {{"a", {"road", "left"}}, {"b", {"road", "right"}}};
    s.behaviours = {behaviour("legacy", 2), behaviour("roadSet", 3), behaviour("leftSet", 1), behaviour("rightSet", 6)};
    s.vehicleTypes = {{"car", 4, 2, {8, 14}, 2, 3, 6, "legacy"}};
    s.inputs = {{"in", "a", "car", 1500, 0, 60}};
    for (std::size_t k = 0; k < lines.size(); ++k)
        for (const auto* from : {"a", "b"})
            s.routeDecisions.push_back({"line" + std::to_string(k), from, lines[k], {}, {{"a", 1, {}}, {"b", 1, {}}}});
    if (assigned) s.segmentBehaviours = {{"road", "car", "roadSet"}, {"left", "car", "leftSet"}, {"right", "car", "rightSet"}};
    return s;
}
// The set the front's segment on the vehicle's CURRENT route names, computed without the index.
std::string expectedSet(const SimState& s, const Vehicle& v) {
    if (v.distance < 100) return "roadSet";
    return s.scenario->routes[v.routeIndex].segmentIds[1] == "left" ? "leftSet" : "rightSet";
}
// BA17: minor northA (85.6) -> northB; the area is 98..102 on the minor route, the Stop line at
// 90 (4 on northB). northA uses `roomy` (standstill 4), northB `tight` (standstill 1).
constexpr double kJoin = 85.6, kLine = 90;
Scenario crossing(bool assigned) {
    Scenario s; s.duration = 300; s.timeStep = .1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"northA", kJoin, {"northB"}}, {"northB", 200, {}}};
    s.routes = {{"major", {"eastUp", "east"}}, {"minor", {"northA", "northB"}}};
    s.behaviours = {behaviour("legacy", 2), behaviour("roomy", 4), behaviour("tight", 1)};
    s.vehicleTypes = {{"car", 4, 2, {15, 15}, 2, 3, 6, "legacy"}};
    s.conflictZones = {{"zone", {{"east"}, 48, 52}, {{"northB"}, 98 - kJoin, 102 - kJoin}, kLine - kJoin, 3, 10, ZoneControl::stop}};
    if (assigned) s.segmentBehaviours = {{"northA", "car", "roomy"}, {"northB", "car", "tight"}};
    return s;
}
const Vehicle* find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return &v;
    return nullptr;
}
std::string setOf(const SimState& s, std::uint64_t id) {
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    for (std::size_t v = 0; v < s.vehicles.size(); ++v)
        if (s.vehicles[v].id == id) return s.scenario->behaviours[refs[v].behaviour].id;
    throw std::runtime_error("Missing fixture vehicle");
}
// Placed vehicles get a desired speed; an obstacle must stay where it is put.
SimState standing(SimState s, std::uint64_t id) {
    for (auto& v : s.vehicles) if (v.id == id) v.desiredSpeed = 0;
    return s;
}
const StopService* serviceOf(const SimState& s, std::uint64_t id) {
    for (const auto& x : s.stopService) if (x.vehicleId == id) return &x;
    return nullptr;
}
}
TEST(behaviourselection, ba14_selection_follows_the_recognized_route_at_every_line) {
    auto state = createSimulation(fork(true), 42);
    std::map<std::uint64_t, std::string> origin; // the route each vehicle departed on
    std::set<std::pair<std::string, std::string>> switched, seenOnSuffix;
    std::set<std::uint64_t> rerouted;
    for (int t = 0; t < 900; ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* d = std::get_if<DepartedEvent>(&e)) origin[d->vehicleId] = d->routeId;
            if (const auto* r = std::get_if<RoutingEvent>(&e); r && r->fromRouteId != r->toRouteId) rerouted.insert(r->vehicleId);
        }
        const auto refs = resolveRefs(*state.scenario, state.vehicles, *state.index);
        for (std::size_t v = 0; v < state.vehicles.size(); ++v) {
            const auto& vehicle = state.vehicles[v];
            // Source-zero: recognized before the first snapshot that sees the vehicle.
            CHECK(std::find(vehicle.passedDecisions.begin(), vehicle.passedDecisions.end(), "line0") != vehicle.passedDecisions.end());
            CHECK(state.scenario->behaviours[refs[v].behaviour].id == expectedSet(state, vehicle));
            if (vehicle.distance >= 100) {
                // No suffix is ever entered before every line on the prefix has been recognized.
                CHECK(vehicle.passedDecisions.size() == 3);
                const auto now = state.scenario->routes[vehicle.routeIndex].id;
                seenOnSuffix.insert({origin.at(vehicle.id), now});
                if (rerouted.contains(vehicle.id)) switched.insert({origin.at(vehicle.id), now});
            }
        }
    }
    // The forcing: vehicles departing on `a` reach both suffixes after a recognized change.
    CHECK(seenOnSuffix.contains({"a", "a"}) && seenOnSuffix.contains({"a", "b"}));
    CHECK(switched.contains({"a", "b"}));
}
TEST(behaviourselection, ba14_a_line_just_short_of_the_join_caps_then_selects_the_chosen_suffix) {
    for (const bool chooseB : {false, true}) {
        auto s = fork(true, {99.9});
        for (auto& d : s.routeDecisions) { d.choices[0].weight = chooseB ? 0 : 1; d.choices[1].weight = chooseB ? 1 : 0; }
        auto state = test::withVehicles(s, {{1, "a", 99, 15}});
        // The forcing: unrecognized, one free tick would carry the front past the join.
        CHECK(state.vehicles[0].distance + 15 * s.timeStep > 100);
        state = stepSimulation(std::move(state));
        test::near(state.vehicles[0].distance, 99.9);
        CHECK(state.vehicles[0].passedDecisions == std::vector<std::string>{"line0"});
        CHECK(state.scenario->routes[state.vehicles[0].routeIndex].id == (chooseB ? "b" : "a"));
        CHECK(setOf(state, 1) == "roadSet");
        state = stepSimulation(std::move(state));
        CHECK(state.vehicles[0].distance > 100);
        CHECK(setOf(state, 1) == (chooseB ? "rightSet" : "leftSet"));
    }
}
TEST(behaviourselection, ba14_routing_draws_are_unchanged_by_assignment) {
    // Every segment assigned the type's own set: the whole run is the unassigned one.
    auto same = fork(false);
    for (const auto* id : {"road", "left", "right"}) same.segmentBehaviours.push_back({id, "car", "legacy"});
    auto plain = createSimulation(fork(false), 42), assigned = createSimulation(same, 42);
    std::size_t routings = 0;
    for (int t = 0; t < 900; ++t) {
        plain = stepSimulation(std::move(plain)); assigned = stepSimulation(std::move(assigned));
        CHECK(plain.vehicles == assigned.vehicles); CHECK(plain.events == assigned.events);
        CHECK(plain.randomState == assigned.randomState && plain.completed == assigned.completed);
        routings += std::count_if(plain.events.begin(), plain.events.end(), [](const auto& e) { return std::holds_alternative<RoutingEvent>(e); });
    }
    CHECK(routings > 0); // the forcing: lines were crossed
    // Distinct sets for lone vehicles: the same draws, at the same ticks, with the same results.
    const std::vector<test::Placement> lone = {{1, "a", 0, 10}, {2, "a", 50, 12}};
    auto bare = test::withVehicles(fork(false), lone), sets = test::withVehicles(fork(true), lone);
    for (int t = 0; t < 300; ++t) {
        bare = stepSimulation(std::move(bare)); sets = stepSimulation(std::move(sets));
        CHECK(bare.randomState == sets.randomState);
        std::vector<RoutingEvent> x, y;
        for (const auto& e : bare.events) if (const auto* r = std::get_if<RoutingEvent>(&e)) x.push_back(*r);
        for (const auto& e : sets.events) if (const auto* r = std::get_if<RoutingEvent>(&e)) y.push_back(*r);
        CHECK(x == y);
    }
}
TEST(behaviourselection, ba17_a_served_stop_survives_a_profile_change_while_held) {
    // A major vehicle standing inside the area denies the crossing throughout.
    auto state = standing(test::withVehicles(crossing(true), {{1, "minor", 85.2, 0}, {2, "major", 100, 0}}), 2);
    CHECK(setOf(state, 1) == "roomy");
    CHECK(kLine - 85.2 <= stopLineReach(state.scenario->behaviours[slot(state.scenario->behaviours, "roomy")], .5));
    state = stepSimulation(std::move(state));
    const auto* first = serviceOf(state, 1);
    CHECK(first); const auto since = first->since; // the forcing: served under roomy
    bool changed = false;
    for (int t = 0; t < 400; ++t) {
        state = stepSimulation(std::move(state));
        const auto* service = serviceOf(state, 1);
        CHECK(service && service->since == since && service->line == kLine); // no reset at the same line
        CHECK(find(state, 1)->distance <= kLine + 1e-9);                       // still denied
        CHECK(!has<SafetyClampEvent>(state, 1));
        changed = changed || setOf(state, 1) == "tight";
    }
    // The forcing: the front crept across the join under service; tight's reach would not have
    // started a service from where the vehicle stood when it was served.
    CHECK(changed && find(state, 1)->distance > kJoin);
    CHECK(kLine - 85.2 > stopLineReach(state.scenario->behaviours[slot(state.scenario->behaviours, "tight")], .5));
    // The area clears: it goes from the same service without standing again.
    std::erase_if(state.vehicles, [](const auto& v) { return v.id == 2; });
    bool crossed = false;
    for (int t = 0; t < 100 && !crossed; ++t) {
        const double before = find(state, 1)->distance;
        state = stepSimulation(std::move(state));
        crossed = find(state, 1) == nullptr || find(state, 1)->distance > kLine;
        if (!crossed) { CHECK(serviceOf(state, 1)->since == since); CHECK(find(state, 1)->distance >= before); }
        CHECK(!has<SafetyClampEvent>(state, 1));
    }
    CHECK(crossed);
}
TEST(behaviourselection, ba17_the_receiving_queue_composes_with_the_current_profile) {
    // Served under `tight` at the line, area clear; a standing leader past the exit (102).
    // Required room past the exit: length 4 + the current set's standstill (tight 1, roomy 4).
    const auto admitted = [](double room) {
        auto state = standing(test::withVehicles(crossing(true), {{1, "minor", 88.5, 0}, {3, "minor", 102 + room + 4, 0}}), 3);
        CHECK(setOf(state, 1) == "tight");
        for (int t = 0; t < 3; ++t) state = stepSimulation(std::move(state));
        CHECK(serviceOf(state, 1)); // the forcing: it has served the line
        for (int t = 0; t < 100; ++t) {
            state = stepSimulation(std::move(state));
            if (find(state, 1)->distance > kLine) return true;
        }
        CHECK(serviceOf(state, 1)); // held by the queue, the service still stands
        return false;
    };
    CHECK(!admitted(4 + 1 - .5)); // short of tight's own requirement: held
    CHECK(admitted(4 + 1.5));     // enough for tight, not for legacy (2) or roomy: reads the current set
}
TEST(behaviourselection, ba17_a_genuine_clamp_is_still_reported_under_assignment) {
    for (const bool assigned : {false, true}) {
        // Too fast to stop at the line while a major vehicle stands inside the area.
        auto state = standing(test::withVehicles(crossing(assigned), {{1, "minor", 89.5, 15}, {2, "major", 100, 0}}), 2);
        const auto zones = summarizeZones(*state.scenario, *state.index, state.vehicles,
                                          resolveRefs(*state.scenario, state.vehicles, *state.index));
        CHECK(zones[0].majorInside); // the forcing
        state = stepSimulation(std::move(state));
        CHECK(has<SafetyClampEvent>(state, 1));
        CHECK(find(state, 1)->distance <= kLine + 1e-9);
    }
}
