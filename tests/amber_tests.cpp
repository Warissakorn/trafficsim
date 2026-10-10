#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace trafficsim;
// M4.2 (D147, docs/reference/AMBER.md): amber stop-or-go, a continuous check. Rows AM1-AM7; AM8
// is the behaviour dialog in behaviour-library-ui.
namespace {
const W74Parameters kP{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16,
    .cxMult = 0, .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
// One head 100 m along the straight road, its program starting with `first` at time 0.
Scenario signalled(std::vector<SignalPhase> phases, std::optional<double> amber) {
    auto s = test::straight();
    s.signalPrograms = {{"p", 0, std::move(phases)}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    for (auto& b : s.behaviours) b.amberDeceleration = amber;
    return s;
}
const std::vector<SignalPhase> kAmberFirst{{3, SignalColor::amber}, {60, SignalColor::red}};
struct Outcome { double furthest{}; std::size_t clamps{}; bool amberAtStart{}; };
// One placed vehicle, `ticks` steps: the furthest its front got and the clamps it caused.
Outcome drive(Scenario s, double distance, double speed, int ticks = 100, bool w74 = false) {
    auto state = test::withVehicles(std::move(s), {test::vehicle(1, distance, speed)});
    if (w74) for (auto& v : state.vehicles) v.w74Traits = w74Traits(state.seed, v.id, v.driverFactor);
    Outcome o{distance, 0, signalColorAt(state.scenario->signalPrograms[0], state.time) == SignalColor::amber};
    for (int t = 0; t < ticks && !state.vehicles.empty(); ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) if (std::holds_alternative<SafetyClampEvent>(e)) ++o.clamps;
        if (!state.vehicles.empty()) o.furthest = std::max(o.furthest, state.vehicles[0].distance);
        else o.furthest = 150; // it left the road: it crossed
    }
    return o;
}
ProjectDocument routed() {
    ProjectDocument d; const auto link = addLink(d, {{0, 0}, {300, 0}}, 1, 3.5);
    const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return d;
}
Json crossingJson() {
    std::ifstream file(test::root() / "data/scenarios/crossing.json"); Json j; file >> j; return j;
}
}
TEST(amber, am1_a_driver_who_can_stop_at_amber_stops) {
    // 10 m/s, 30 m short: 100 < 2 * 3 * 30 = 180.
    const auto o = drive(signalled(kAmberFirst, 3), 70, 10);
    CHECK(o.amberAtStart); // the forcing: the decision is taken at amber
    CHECK(o.furthest <= 100 + 1e-9); CHECK(o.clamps == 0);
}
TEST(amber, am2_a_driver_who_cannot_stop_at_amber_goes) {
    // 15 m/s, 20 m short: 225 > 2 * 3 * 20 = 120.
    const auto legacy = drive(signalled(kAmberFirst, std::nullopt), 80, 15);
    CHECK(legacy.amberAtStart); CHECK(legacy.furthest <= 100 + 1e-9); // the forcing: D36 holds it
    const auto o = drive(signalled(kAmberFirst, 3), 80, 15, 30); // within the 3 s of amber
    CHECK(o.furthest > 100); CHECK(o.clamps == 0);
}
TEST(amber, am3_equality_and_a_standing_vehicle_stop) {
    // 6 m/s, 6 m short: 36 == 2 * 3 * 6, which stops.
    CHECK(drive(signalled(kAmberFirst, 3), 94, 6).furthest <= 100 + 1e-9);
    CHECK(drive(signalled(kAmberFirst, 3), 100, 0).furthest <= 100 + 1e-9);
}
TEST(amber, am4_red_holds_every_driver_who_can_stop_at_all) {
    const std::vector<SignalPhase> red{{60, SignalColor::red}};
    // 20 m/s, 10 m short of a red line needs 20 m/s^2, past the car's 8: D36 clamps it...
    const auto legacy = drive(signalled(red, std::nullopt), 90, 20);
    CHECK(legacy.furthest <= 100 + 1e-9); CHECK(legacy.clamps > 0); // the forcing
    // ...while a driver who decides at amber cannot stop at all, so goes, as at a priority rule.
    const auto o = drive(signalled(red, 3), 90, 20, 10);
    CHECK(o.furthest > 100); CHECK(o.clamps == 0);
    // Red still holds anyone who can stop, however hard: 10 m/s, 10 m short needs 5 m/s^2 <= 8.
    CHECK(drive(signalled(red, 3), 90, 10).furthest <= 100 + 1e-9);
}
TEST(amber, am5_a_w74_driver_decides_the_same_way) {
    auto s = signalled(kAmberFirst, 3);
    for (auto& b : s.behaviours) { const auto amber = b.amberDeceleration; b = DriverBehaviour{b.id}; b.w74 = kP; b.amberDeceleration = amber; }
    CHECK(drive(s, 70, 10, 100, true).furthest <= 100 + 1e-9);
    CHECK(drive(s, 80, 15, 30, true).furthest > 100);
}
TEST(amber, am6_owned_behaviours_carry_it_at_schema_28) {
    // The catalog decides at amber with ITE's 3.0 m/s^2.
    const auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    CHECK(catalog.behaviours.size() == 1);
    CHECK(catalog.behaviours[0].amberDeceleration); test::near(*catalog.behaviours[0].amberDeceleration, 3);
    // A catalog project writes no behaviour and keeps its schema.
    auto d = routed();
    CHECK(documentJson(d)["schemaVersion"] == 17);
    // Owning the catalog owns the field, which is schema 28; without it, the earlier schema.
    putDemandCatalog(d, catalog); validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 28); CHECK(j["definition"]["behaviours"][0]["amberDeceleration"] == 3.0);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto without = catalog; without.behaviours[0].amberDeceleration.reset();
    auto plain = routed(); putDemandCatalog(plain, without);
    CHECK(documentJson(plain)["schemaVersion"] == 18);
    auto older = j; older["schemaVersion"] = 27;
    test::throws([&] { parseDocument(older); }, "amberDeceleration");
    for (const double bad : {0.0, -1.0, std::nan("")}) {
        auto copy = d; copy.definition->behaviours[0].amberDeceleration = bad;
        test::throws([&] { compileDocument(copy, test::root() / "data"); }, "amberDeceleration");
    }
}
TEST(amber, am7_an_m0_scenario_keeps_amber_as_red) {
    const auto data = test::root() / "data";
    // Both readers of the frozen TS baselines' file kind resolve the catalog without it...
    for (const auto& b : loadScenario(data / "scenarios/crossing.json", data).scenario.behaviours) CHECK(!b.amberDeceleration);
    const auto m0 = parseDocument(crossingJson());
    CHECK(m0.definition->provenance.legacyAmber);
    for (const auto& b : compileDocument(m0, data).scenario.behaviours) CHECK(!b.amberDeceleration);
    // ...while a project reading the same catalog decides at amber.
    for (const auto& b : compileDocument(routed(), data).scenario.behaviours) CHECK(b.amberDeceleration);
    // Saved, the M0 file is a project: provenance is never written, and never compared, so the
    // round trip is exact while the reopened file takes the project rule.
    const auto saved = parseDocument(documentJson(m0));
    CHECK(!saved.definition->provenance.legacyAmber); CHECK(saved == m0);
    for (const auto& b : compileDocument(saved, data).scenario.behaviours) CHECK(b.amberDeceleration);
}
TEST(amber, owned_catalogs_reopen_at_every_schema_from_21) {
    // Regression (D147): an owned catalog without a library, in a file at schema 21 or later for
    // another reason, was written without each behaviour's model and could not be reopened.
    auto without = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    without.behaviours[0].amberDeceleration.reset();
    auto d = routed(); putDemandCatalog(d, without); changeEvaluationPeriod(d, 10, std::nullopt);
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 22); CHECK(j["definition"]["behaviours"][0]["model"] == "prototype");
    CHECK(parseDocument(Json::parse(j.dump())) == d);
}
