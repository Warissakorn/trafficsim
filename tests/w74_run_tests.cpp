#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/w74.hpp"
#include <algorithm>
#include <map>
using namespace trafficsim;
// M3.3.3a (D133): W74 composed into the tick -- rows BA23, BA24, BA25 and BA28 of
// docs/plans/DRIVING_BEHAVIOUR.md against docs/reference/W74.md §6-§7, on hand-built scenarios.
namespace {
// No trait enters the thresholds (every *Mult that a hashed trait scales is 0), so hand values
// hold for any vehicle id: at v = 4, BX = 5, ABX = 7, SDX = 12; bNull = 0.25.
const W74Parameters kP{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16,
    .cxMult = 0, .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
DriverBehaviour w74(const std::string& id) { DriverBehaviour b{id}; b.w74 = kP; return b; }
DriverBehaviour prototype(const std::string& id) { return {id, 2, 2, 3, 1.5, .2}; }
// One route over the given 50 m segments; cars (4 m, max 3 / comfortable 2 / max 6 m/s²) use `w74`,
// trucks the prototype, "carProto" the prototype, "truckW74" w74 -- equal lengths, so only the model differs.
Scenario road(std::vector<std::string> segments, double length = 50) {
    Scenario s; s.duration = 600; s.timeStep = 0.1;
    for (std::size_t i = 0; i < segments.size(); ++i)
        s.segments.push_back({segments[i], length, i + 1 < segments.size() ? std::vector<std::string>{segments[i + 1]} : std::vector<std::string>{}});
    s.routes = {{"route", segments}};
    s.behaviours = {w74("w74"), w74("w74b"), prototype("proto")};
    s.vehicleTypes = {{"car", 4, 2, {15, 15}, 3, 2, 6, "w74"}, {"truck", 4, 2, {15, 15}, 3, 2, 6, "proto"},
                      {"carProto", 4, 2, {15, 15}, 3, 2, 6, "proto"}, {"truckW74", 4, 2, {15, 15}, 3, 2, 6, "w74"}};
    return s;
}
struct Placed { std::uint64_t id; double distance, speed, acceleration{}; std::string type{"car"};
                std::optional<W74State> state{}; };
// Hand-placed vehicles with the traits generation would have given them (D132).
SimState placed(const Scenario& s, const std::vector<Placed>& list) {
    std::vector<test::Placement> placements;
    for (const auto& p : list) placements.push_back({p.id, "route", p.distance, p.speed, p.type});
    auto state = test::withVehicles(s, placements);
    for (auto& v : state.vehicles)
        for (const auto& p : list) if (p.id == v.id) {
            v.acceleration = p.acceleration; v.w74State = p.state;
            v.w74Traits = w74Traits(state.seed, v.id, v.driverFactor);
        }
    return state;
}
const Vehicle& find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return v;
    throw std::runtime_error("vehicle left");
}
bool clamped(const SimState& s) {
    return std::any_of(s.events.begin(), s.events.end(), [](const auto& e) { return std::holds_alternative<SafetyClampEvent>(e); });
}
}

TEST(w74run, the_leaders_acceleration_reaches_the_follower_and_static_obstacles_give_zero) { // BA28
    const auto s = road({"a", "b", "c", "d", "e", "f"});
    // Follower front at 96, leader rear at 106: g = 10 < SDX, dv = 2 > CLDV = 1 -> approaching:
    // 0.5 * 4 / (7 - 10) + 0.5 * aL.
    for (const double aL : {0.0, 1.0, -1.5}) {
        const auto next = stepSimulation(placed(s, {{1, 96, 4}, {2, 110, 2, aL}}));
        const auto& f = find(next, 1);
        CHECK(f.mode == FollowingMode::approaching);
        test::near(f.acceleration, -2.0 / 3 + 0.5 * aL, 1e-12);
        CHECK(f.w74State && f.w74State->regime == W74Regime::approaching && f.w74State->sign == 0);
    }
    // A red head 11.5 m ahead is a static obstacle: 0.5 * 16 / (7 - 11.5), with no aL term.
    auto red = s; red.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    red.signalHeads = {{"h", "c", 7.5, "p"}};
    const auto& f = find(stepSimulation(placed(red, {{1, 96, 4}})), 1);
    test::near(f.acceleration, -16.0 / 9, 1e-12);
}

TEST(w74run, model_switches_follow_the_front_segment) { // BA24
    // a: proto, b: w74, c: w74b (another w74 behaviour), d: proto. A leader 10 m ahead at the same
    // speed: following, where entry gives s = +1 (dv = 0) and a kept state its own sign.
    auto s = road({"a", "b", "c", "d", "e", "f"});
    s.vehicleTypes[0].behaviourId = "proto";
    s.segmentBehaviours = {{"b", "car", "w74"}, {"c", "car", "w74b"}};
    const auto step = [&](double front, std::optional<W74State> before) {
        return find(stepSimulation(placed(s, {{1, front, 4, 0, "car", before}, {2, front + 14, 4, 0, "truck"}})), 1);
    };
    const W74State down{W74Regime::following, -1};
    // Same-model boundary b|c (front exactly at the join is on c): the state is kept.
    auto v = step(100, down);
    CHECK(v.w74State == down); test::near(v.acceleration, -0.25, 1e-12);
    // Entry (no state) at a|b and at b|c: initialised from the snapshot, dv = 0 -> +1.
    CHECK(step(50, std::nullopt).w74State == W74State{W74Regime::following, 1});
    CHECK(step(100, std::nullopt).w74State == W74State{W74Regime::following, 1});
    // Exit at c|d onto the prototype, and anywhere on a: cleared.
    CHECK(!step(150, down).w74State);
    CHECK(!step(49.99, down).w74State);
    // Re-entry after a prototype tick is an entry again: the cleared state re-initialises.
    CHECK(step(50, std::nullopt).acceleration == 0.25);
}

TEST(w74run, the_leaders_model_never_changes_the_followers_result) { // BA25
    const auto s = road({"a", "b", "c", "d"});
    for (const char* follower : {"car", "carProto"}) {
        // A standing leader and a moving one, each of either model.
        for (const double speed : {0.0, 3.0}) {
            const auto withProto = find(stepSimulation(placed(s, {{1, 96, 4, 0, follower}, {2, 110, speed, 0, "truck"}})), 1);
            const auto withW74 = find(stepSimulation(placed(s, {{1, 96, 4, 0, follower}, {2, 110, speed, 0, "truckW74"}})), 1);
            CHECK(withProto.acceleration == withW74.acceleration); CHECK(withProto.w74State == withW74.w74State);
            CHECK(withProto.distance == withW74.distance);
        }
    }
}

TEST(w74run, the_hard_cap_source_rule_and_d105_hold_for_w74) { // BA28
    // A red head 1 m ahead of a car at 15 m/s: maximum deceleration still covers 1.47 m, so the
    // hard cap stops it at the head and counts the clamp.
    auto red = road({"a", "b", "c", "d"});
    red.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    red.signalHeads = {{"h", "c", 1, "p"}};
    auto state = stepSimulation(placed(red, {{1, 100, 15}}));
    CHECK(clamped(state)); CHECK(find(state, 1).distance <= 101 + 1e-9); CHECK(find(state, 1).speed == 0);
    // D105: at rest within ax of a leader pulling away, no positive acceleration.
    for (const double gap : {2.0, 1.0}) {
        const auto& v = find(stepSimulation(placed(red, {{1, 96 - gap, 0}, {2, 100, 5, 2, "truck"}})), 1);
        CHECK(v.acceleration <= 0); CHECK(v.speed == 0);
    }
    // Source (D108 with ax): a w74 input does not depart while the entry vehicle is within ax.
    auto source = road({"a", "b"}); source.duration = 1;
    source.inputs = {{"in", "route", "car", 3600, 0, 1}};
    auto s = createSimulation(source, 42);
    Vehicle blocker; blocker.id = 99; blocker.typeIndex = 1; blocker.distance = 5.5; blocker.desiredSpeed = 0;
    blocker.w74Traits = w74Traits(42, 99, 0.5);
    s.vehicles.push_back(blocker); s.nextVehicleId = 100;
    for (int i = 0; i < 10; ++i) {
        s = stepSimulation(s);
        CHECK(std::none_of(s.events.begin(), s.events.end(), [](const auto& e) { return std::holds_alternative<DepartedEvent>(e); }));
    }
}

TEST(w74run, a_w74_vehicle_serves_a_stop_line) { // BA28, standstill s = +1
    const auto source = test::demo().scenario;
    Scenario s; s.duration = 300; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = {w74("w74")};
    for (auto& t : s.vehicleTypes) t.behaviourId = "w74";
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, ZoneControl::stop}};
    const auto serve = [&](double distance, double speed, std::optional<W74State> before) {
        auto state = test::withVehicles(s, {{1, "minorRoute", distance, speed}});
        for (auto& v : state.vehicles) { v.w74Traits = w74Traits(state.seed, v.id, v.driverFactor); v.w74State = before; }
        int atLine = 0; bool crossed = false;
        for (int t = 0; t < 3000 && !crossed; ++t) {
            const auto& v = state.vehicles.front();
            if (v.distance > 90 + 1e-9) crossed = true;
            else if (v.speed == 0 && 90 - v.distance <= stopLineReach(s.behaviours.front(), v.driverFactor)) ++atLine;
            state = stepSimulation(state);
        }
        return std::pair{crossed, atLine};
    };
    // Arriving at speed.
    const auto [arrived, arrivedAtLine] = serve(40, 10, std::nullopt);
    CHECK(arrived); CHECK(arrivedAtLine >= 2);
    // W74.md §7's case: at rest 3.3 m short of the line -- inside the following band at v = 0
    // (ABX = 2.79, SDX = 3.58) but beyond stopLineReach (2.89) -- after braking (s would be -1).
    // Only the standstill override lets it creep up, be served and go.
    const W74State braking{W74Regime::approaching, 0};
    CHECK(90 - 86.7 > stopLineReach(s.behaviours.front(), 0.5));
    const auto [crept, creptAtLine] = serve(86.7, 0, braking);
    CHECK(crept); CHECK(creptAtLine >= 2);
}

TEST(w74run, a_signalised_queue_discharges_and_an_open_platoon_never_clamps) { // BA23
    const W74Parameters& p = kP;
    const auto check = [&](const Scenario& scenario, bool expectNoClamp) {
        auto state = createSimulation(scenario, 42);
        std::size_t following = 0, standingInside = 0;
        while (state.tick < totalTicks(*state.scenario)) {
            auto order = state.vehicles;
            std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.distance > b.distance; });
            std::map<std::uint64_t, double> gapOf; // gap to the vehicle ahead, one route of one segment
            for (std::size_t i = 1; i < order.size(); ++i) gapOf[order[i].id] = order[i - 1].distance - 4 - order[i].distance;
            const auto before = state;
            state = stepSimulation(std::move(state));
            if (expectNoClamp) CHECK(!clamped(state));
            for (const auto& v : state.vehicles) {
                const auto was = std::find_if(before.vehicles.begin(), before.vehicles.end(), [&](const auto& x) { return x.id == v.id; });
                if (was == before.vehicles.end() || !gapOf.contains(v.id)) continue;
                // Following accelerates by exactly +-bNull, unless the type's bounds or a cap cut it.
                if (v.mode == FollowingMode::following && v.acceleration > -was->speed / scenario.timeStep + 1e-9) {
                    CHECK(std::abs(std::abs(v.acceleration) - p.bNullAdd) < 1e-12 || v.acceleration == 0); ++following;
                }
                // At rest within ABX(0.1) of the vehicle ahead: it does not start (D105, emergency dv² term).
                const double abx = p.ax + (p.bxAdd + p.bxMult * was->driverFactor) * std::sqrt(0.1);
                if (was->speed == 0 && gapOf[v.id] <= abx) { CHECK(v.speed == 0); ++standingInside; }
            }
        }
        return std::pair{following, standingInside};
    };
    // 60 s red, then green, on one 400 m segment: a queue forms and discharges.
    auto signalised = road({"main"}, 400);
    signalised.inputs = {{"in", "route", "car", 900, 0, 300}};
    signalised.signalPrograms = {{"p", 0, {{60, SignalColor::red}, {540, SignalColor::green}}}};
    signalised.signalHeads = {{"h", "main", 300, "p"}};
    const auto [queueFollowing, standing] = check(signalised, false);
    CHECK(standing > 0);
    // An uncongested platoon for 600 s on a 2 km road: never a safety clamp.
    auto open = road({"main"}, 2000);
    open.inputs = {{"in", "route", "car", 600, 0, 600}};
    const auto [platoonFollowing, unused] = check(open, true);
    CHECK(platoonFollowing + queueFollowing > 0);
}

TEST(w74run, a_w74_run_replays_exactly) { // BA26/BA28: state and traits copied with SimState
    auto s = road({"main"}, 400);
    s.duration = 120; s.inputs = {{"in", "route", "car", 900, 0, 120}};
    s.signalPrograms = {{"p", 0, {{30, SignalColor::red}, {90, SignalColor::green}}}};
    s.signalHeads = {{"h", "main", 300, "p"}};
    auto a = createSimulation(s, 42);
    for (int i = 0; i < 400; ++i) a = stepSimulation(a);
    auto b = a;
    bool sawState = false;
    while (a.tick < totalTicks(*a.scenario)) {
        a = stepSimulation(a); b = stepSimulation(b);
        CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events);
        for (const auto& v : a.vehicles) sawState = sawState || v.w74State.has_value();
    }
    CHECK(sawState);
    CHECK(test::finish(createSimulation(s, 42)).vehicles == test::finish(createSimulation(s, 42)).vehicles);
}

TEST(w74run, a_courtesy_hold_that_wins_stores_its_own_state) { // BA28, W74.md §7 second obstacle
    // A35's road: a changer waiting at its dead end beside a 10 m/s stream, every behaviour w74.
    auto road = test::lanes();
    for (auto& b : road.behaviours) b = w74(b.id);
    std::vector<test::Placement> list{test::on(1, "stubA", 198, 0)};
    for (std::uint64_t k = 0; k < 15; ++k) list.push_back(test::on(10 + k, "full", 190 - 12 * static_cast<double>(k), 10));
    auto s = test::withVehicles(road, list);
    for (auto& v : s.vehicles) v.w74Traits = w74Traits(s.seed, v.id, v.driverFactor);
    std::size_t won = 0;
    for (int t = 0; t < 600; ++t) {
        const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
        const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
        const auto holds = courtesyHolds(*s.scenario, *s.index, s.vehicles, refs, spans, bucketSpans(spans, s.scenario->segments.size()));
        std::map<std::uint64_t, FollowingResult> held;
        for (std::size_t v = 0; v < holds.size(); ++v)
            if (std::isfinite(holds[v].gap)) {
                const auto& x = s.vehicles[v];
                held[x.id] = follow(x.speed, x, x.w74State, s.scenario->vehicleTypes[refs[v].type],
                                    s.scenario->behaviours[refs[v].behaviour],
                                    Leader{holds[v].gap, holds[v].speed, holds[v].acceleration});
            }
        s = stepSimulation(s);
        for (const auto& v : s.vehicles) {
            const auto h = held.find(v.id);
            const bool clampedHere = std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
                const auto* c = std::get_if<SafetyClampEvent>(&e); return c && c->vehicleId == v.id; });
            if (h == held.end() || clampedHere || v.acceleration != h->second.acceleration) continue;
            CHECK(v.w74State == h->second.w74); ++won;
        }
    }
    CHECK(won > 0); // the forcing: the courtesy obstacle really was the kept result
}
