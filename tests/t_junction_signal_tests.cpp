#include "t_junction_support.hpp"
#include "../src/core/simulation.hpp"
#include <set>
using namespace trafficsim;
using namespace tjunction;
// M3.2.7c, on the T-junction fixture: a signal head on the minor approach composed with its
// Stop/Yield lines (A20 end to end), and a congested major road in which headway, not gap time,
// decides some blocks -- the traffic the M3.2.7b headway arm lacked (docs/evidence/m3.2.7-sweep.md).
namespace {
const std::vector<SignalPhase> kGreen{{60, SignalColor::green}}, kRed{{60, SignalColor::red}},
    kCycle{{30, SignalColor::green}, {3, SignalColor::amber}, {27, SignalColor::red}};
fixture::TJunction variant(std::vector<SignalPhase> signal, std::optional<StopMode> control = StopMode::yield,
                           DrivingSide side = DrivingSide::right) {
    fixture::TJunctionOptions o; o.minorSignal = std::move(signal); o.control = control; o.side = side;
    return fixture::tJunction(o);
}
// What the minor head saw: minor fronts passing it on red, and which minor vehicles a Stop served.
struct Watch { int passedOnRed{}; std::set<std::uint64_t> served, completedMinor; };
Watch watch(const fixture::TJunction& t, std::uint32_t seed = 42) {
    const auto s0 = compiled(t);
    const auto head = std::find_if(s0.signalHeads.begin(), s0.signalHeads.end(), [&](const auto& h) { return h.id == t.minorHead; });
    if (head == s0.signalHeads.end()) throw std::runtime_error("no minor head");
    const auto program = *std::find_if(s0.signalPrograms.begin(), s0.signalPrograms.end(), [&](const auto& p) { return p.id == head->programId; });
    const auto minor = [&](const SimState& s, const Vehicle& v) {
        const auto& id = s.scenario->routes[v.routeIndex].id;
        return id == t.crossingRoute || id == t.nearRoute;
    };
    Watch w;
    auto s = createSimulation(s0, seed);
    while (s.tick < totalTicks(*s.scenario)) {
        const auto before = s;
        s = stepSimulation(s);
        // Minor routes start on the minor Link's lane, so route distance is the head's station there.
        const bool red = signalColorAt(program, before.time) == SignalColor::red;
        for (const auto& v : before.vehicles) {
            if (!minor(before, v) || v.distance > head->position) continue;
            const auto after = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& x) { return x.id == v.id; });
            if (red && (after == s.vehicles.end() || after->distance > head->position)) ++w.passedOnRed;
        }
        for (const auto& service : s.stopService) w.served.insert(service.vehicleId);
        for (const auto& e : s.events)
            if (const auto* a = std::get_if<ArrivedEvent>(&e); a && (a->routeId == t.crossingRoute || a->routeId == t.nearRoute))
                w.completedMinor.insert(a->vehicleId);
    }
    return w;
}
const std::string kCrossing = "Minor approach → Major westbound", kNear = "Minor approach → Major eastbound",
                  kEast = "Major eastbound → Major eastbound", kWest = "Major westbound → Major westbound";
}
TEST(tjunction_signal, the_base_fixture_is_untouched_by_the_new_options) {
    // The committed file and the M3.2.7b metadata describe the default options; the new ones are off.
    const fixture::TJunctionOptions o;
    CHECK(o.minorSignal.empty() && !o.congestedMajor);
    CHECK(fixture::tJunction().document.network.signalHeads.empty());
}
TEST(tjunction_signal, a_green_head_does_not_serve_the_stop) {
    const auto t = variant(kGreen, StopMode::stop);
    const auto w = watch(t);
    CHECK(!w.completedMinor.empty()); // the forcing: minor trips passed the green head
    // Every minor vehicle that finished was served at a Stop line on the way: green is the head's
    // permission only (contract §5, A20).
    for (const auto id : w.completedMinor) CHECK(w.served.contains(id));
    const auto r = run(t), plain = run(fixture::tJunction([] { fixture::TJunctionOptions o; o.control = StopMode::stop; return o; }()));
    CHECK(!r.bothSidesInside); CHECK(r.majorClamps == 0);
    // A head that is always green changes no minor trip: its line holds nobody.
    CHECK(movement(r.report, kCrossing) == movement(plain.report, kCrossing));
    CHECK(movement(r.report, kNear) == movement(plain.report, kNear));
}
TEST(tjunction_signal, a_red_head_holds_the_whole_minor_road_and_not_the_major_one) {
    const auto t = variant(kRed);
    const auto r = run(t);
    CHECK(r.report.queues.front().maxLength > 0); // the forcing: minor demand really arrived
    CHECK(movement(r.report, kCrossing).vehicles == 0); CHECK(movement(r.report, kNear).vehicles == 0);
    CHECK(watch(t).passedOnRed == 0);
    // Nothing reaches the conflict areas from the minor road, so the major road runs as if alone.
    const auto open = run(fixture::tJunction());
    CHECK(movement(r.report, kEast).vehicles == movement(open.report, kEast).vehicles);
    CHECK(movement(r.report, kWest).vehicles == movement(open.report, kWest).vehicles);
    CHECK(*movement(r.report, kEast).meanDelay <= *movement(open.report, kEast).meanDelay + 1e-9);
}
TEST(tjunction_signal, a_fixed_cycle_composes_with_the_gap_test_on_both_driving_sides) {
    for (const auto side : {DrivingSide::right, DrivingSide::left}) {
        const auto t = variant(kCycle, StopMode::yield, side);
        const auto w = watch(t);
        CHECK(!w.completedMinor.empty()); // trips complete through the green phases
        CHECK(w.passedOnRed == 0);
        const auto r = run(t);
        CHECK(!r.bothSidesInside); CHECK(r.majorClamps == 0);
        CHECK(r.report.pending == 0 && r.report.active == 0);
        // The head only adds waiting: the minor road is slower than with the gap test alone.
        fixture::TJunctionOptions base; base.side = side;
        const auto yield = run(fixture::tJunction(base));
        CHECK(movement(r.report, kEast).vehicles == movement(yield.report, kEast).vehicles);
        CHECK(*movement(r.report, kCrossing).meanDelay > *movement(yield.report, kCrossing).meanDelay);
        CHECK(*movement(r.report, kNear).meanDelay > *movement(yield.report, kNear).meanDelay);
    }
}
TEST(tjunction_signal, a_congested_major_road_lets_headway_decide) {
    // The forcing for the M3.2.7c headway arm, established before any of its rows exist: in this
    // variant some tick has a major vehicle within `headway` of the crossing entry yet outside the
    // gap-time window -- the only ticks in which headway alone decides a block.
    fixture::TJunctionOptions o; o.congestedMajor = true; o.headway = 12;
    const auto t = fixture::tJunction(o);
    const auto s0 = compiled(t);
    const auto crossing = zoneOn(s0, t.crossingTurn);
    auto s = createSimulation(s0, 42);
    long headwayOnly = 0;
    while (s.tick < totalTicks(*s.scenario)) {
        s = stepSimulation(s);
        for (const auto& v : s.vehicles) {
            const auto at = locateVehicle(*s.scenario, v, *s.index);
            if (at.segmentId != crossing.major.segmentIds.front()) continue;
            const double d = crossing.major.entry - at.position;
            if (d >= 0 && d <= crossing.headway && (v.speed <= 0 || d / v.speed >= crossing.gapTime)) ++headwayOnly;
        }
    }
    CHECK(headwayOnly > 0);
    const auto r = run(t);
    CHECK(r.report.pending == 0 && r.report.active == 0);
    CHECK(!r.bothSidesInside); CHECK(r.majorClamps == 0);
}
#include "../tools/t_junction_sweep.hpp"
#include <fstream>
TEST(tjunction_signal, the_archived_headway_metadata_still_describes_the_congested_variant) {
    // docs/evidence/m3.2.7c-headway-metadata.json was committed before the headway arm ran.
    std::ifstream f(test::root() / "docs/evidence/m3.2.7c-headway-metadata.json");
    CHECK(f.good());
    auto archived = nlohmann::ordered_json::parse(f);
    CHECK(archived.contains("build") && archived.at("variant") == "congestedMajor"); // the forcing
    archived.erase("build");
    CHECK(archived == sweep::congestedMetadata(test::root()));
}
