#include "t_junction_support.hpp"
#include <cmath>
using namespace trafficsim;
using namespace tjunction;
// M3.2.7a, A26 of docs/M3_ACCEPTANCE.md §2 "Controlled cases before stochastic sweeps": on the
// T-junction fixture's own compiled scenario, with test-owned initial states and no demand. Every
// expectation below is derived from the setup before the run, never read back from it.
namespace {
struct Setup { fixture::TJunction t; Scenario s; ConflictZone crossing; std::string majorRoute, minorRoute; };
Setup setup(double gapTime, double headway, DrivingSide side = DrivingSide::right) {
    fixture::TJunctionOptions o; o.side = side; o.gapTime = gapTime; o.headway = headway; o.control = std::nullopt;
    Setup u{fixture::tJunction(o), {}, {}, {}, {}};
    u.s = compiled(u.t); u.s.inputs.clear();
    u.crossing = zoneOn(u.s, u.t.crossingTurn);
    // A single-lane authored route keeps its id as the runtime route (routeLaneChains).
    u.majorRoute = u.t.eastRoute; u.minorRoute = u.t.crossingRoute;
    return u;
}
// Route distances: the major side's section starts the eastbound route; the minor route runs the
// minor Link's lane, then the Connector.
double minorOffset(const Setup& u) {
    const auto& route = *std::find_if(u.s.routes.begin(), u.s.routes.end(), [&](const auto& r) { return r.id == u.minorRoute; });
    double offset = 0;
    for (const auto& id : route.segmentIds) {
        if (id == u.crossing.minor.segmentIds.front()) return offset;
        offset += std::find_if(u.s.segments.begin(), u.s.segments.end(), [&](const auto& x) { return x.id == id; })->length;
    }
    throw std::runtime_error("minor route misses the crossing");
}
// A major vehicle whose front is `ahead` metres before the crossing entry, and a minor vehicle
// standing just short of its waiting line.
SimState state(const Setup& u, double ahead, double speed) {
    const double wait = minorOffset(u) + u.crossing.waitPosition;
    return test::withVehicles(u.s, {{1, u.majorRoute, u.crossing.major.entry - ahead, speed},
                                    {2, u.minorRoute, wait - 0.5, 0}});
}
}
TEST(tjunction_controlled, the_gap_time_threshold_admits_at_and_below_the_time_to_entry) {
    // §2's example: front 50 m before entry at 10 m/s, headway 7 m -- 5 s to entry.
    for (const auto side : {DrivingSide::right, DrivingSide::left})
        for (const auto [gap, blocks] : {std::pair{4.0, false}, {5.0, false}, {6.0, true}}) {
            const auto u = setup(gap, 7, side);
            const auto s = state(u, 50, 10);
            const auto at = locateVehicle(*s.scenario, s.vehicles.front(), *s.index);
            CHECK(at.segmentId == u.crossing.major.segmentIds.front()); // the forcing: 50 m short, 5 s away
            test::near(u.crossing.major.entry - at.position, 50, 1e-9);
            CHECK(majorBlocks(s, u.crossing.id) == blocks);
        }
}
TEST(tjunction_controlled, the_headway_threshold_blocks_up_to_and_including_its_distance) {
    // Time blocking inactive: the major stands still, so its time to entry is unbounded.
    const double eps = 0.01;
    for (const auto [ahead, blocks] : {std::pair{7 - eps, true}, {7.0, true}, {7 + eps, false}}) {
        const auto u = setup(0.1, 7);
        CHECK(majorBlocks(state(u, ahead, 0), u.crossing.id) == blocks);
    }
    // Equality blocks, as in A09 (conflict_zone.a09): at exactly the headway the major is "within" it.
}
TEST(tjunction_controlled, a_denied_gap_postpones_admission_until_the_major_clears) {
    // The major at a steady 15 m/s -- a placed vehicle's desired speed, so it neither brakes nor
    // accelerates -- 75 m short of entry: 5 s away. gapTime 4 admits; 6 denies.
    constexpr double speed = 15, ahead = 75;
    const auto base = setup(4, 7);
    const auto& type = *std::find_if(base.s.vehicleTypes.begin(), base.s.vehicleTypes.end(), [](const auto& x) { return x.id == "car"; });
    // Clearance margin first: from standstill at full acceleration, the minor clears the crossing
    // (its rear past the minor exit) before the major can reach the major entry.
    const double clear = base.crossing.minor.exit - base.crossing.waitPosition + 0.5 + type.length;
    const double clearTime = std::sqrt(2 * clear / type.maxAcceleration);
    CHECK(clearTime + 0.5 < ahead / speed);
    // Stated before running: permitted, the minor front crosses its line within the time it takes
    // to cover the 0.5 m from standstill, plus a second for car-following to start moving it.
    const double permittedBound = std::sqrt(2 * 0.5 / type.maxAcceleration) + 1;
    // Denied, the gap test fails until the major's rear has left the area exit; at 15 m/s that is
    // (ahead + major area length + car length) / speed, and admission may not come before it.
    const double majorClears = (ahead + base.crossing.major.exit - base.crossing.major.entry + type.length) / speed;
    struct Outcome { double admitted = -1, majorRearOut = -1, arrived = -1; };
    const auto go = [&](double gap) {
        const auto u = setup(gap, 7);
        auto s = state(u, ahead, speed);
        s.vehicles.front().desiredSpeed = speed;
        const double line = minorOffset(u) + u.crossing.waitPosition;
        Outcome o;
        while (s.tick < totalTicks(*s.scenario) && o.arrived < 0) {
            s = stepSimulation(s);
            for (const auto& e : s.events)
                if (const auto* a = std::get_if<ArrivedEvent>(&e); a && a->vehicleId == 2) o.arrived = a->time;
            for (const auto& v : s.vehicles) {
                if (v.id == 2 && o.admitted < 0 && v.distance > line) o.admitted = s.time;
                if (v.id == 1 && o.majorRearOut < 0) {
                    const auto at = locateVehicle(*s.scenario, v, *s.index);
                    if (at.segmentId != u.crossing.major.segmentIds.front() || at.position - type.length > u.crossing.major.exit)
                        o.majorRearOut = s.time;
                }
            }
        }
        return o;
    };
    const auto permitted = go(4), denied = go(6);
    CHECK(permitted.admitted > 0 && permitted.admitted <= permittedBound);
    CHECK(denied.admitted > 0 && denied.majorRearOut > 0);
    CHECK(denied.admitted >= denied.majorRearOut);                           // never before the major has gone
    CHECK(denied.admitted >= majorClears - 0.2 && denied.admitted <= majorClears + 10);
    CHECK(denied.admitted > permitted.admitted);
    // Both finish; the same route from the same start, so the later arrival is the larger delay.
    CHECK(permitted.arrived > 0 && denied.arrived > permitted.arrived);
}
