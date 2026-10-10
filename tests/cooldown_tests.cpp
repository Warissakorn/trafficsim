#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/eval/movement.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/evaluation_period.hpp"
#include "../src/project/run.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>
using namespace trafficsim;
// M5.9 (D146, docs/reference/BATCH.md §5): the evaluation cool-down. The run continues past
// `duration` with no new demand; the window then selects trips by when their demand was released,
// and only window trips the cool-down did not clear are unfinished. Rows CD1-CD8.
namespace {
ProjectDocument routed() {
    ProjectDocument d; const auto link = addLink(d, {{0, 0}, {300, 0}}, 1, 3.5);
    const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 600, .1);
    putInput(d, {"in", route, "car", 600, 0, 600});
    validateDocument(d);
    return d;
}
Json fourLegJson() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return j;
}
EvaluationSpec oneMovement(double warmup, std::optional<double> end, std::optional<double> cooldown) {
    EvaluationSpec spec; spec.movementNames = {"in → out"}; spec.movementOfRoute = {{"route", 0}};
    spec.warmup = warmup; spec.end = end; spec.cooldown = cooldown; return spec;
}
SimState stateAt(const Scenario& s, double time, std::vector<SimEvent> events) {
    auto state = createSimulation(s, 1); state.time = time; state.events = std::move(events); return state;
}
// The smallest of 120, 300 and 600 s that leaves no window trip of four-leg unfinished for seeds
// 42-51; 120 s already clears the 5 % warning (docs/evidence/m5.9-cooldown.md).
constexpr double kFourLegCooldown = 300;
// A trip of 10 s with no source wait: released at time - 10.
ArrivedEvent arrival(double time) { return {time, 1, "route", 10, 0, 5}; }
}
TEST(cooldown, cd1_schema27_round_trips_and_files_without_it_keep_their_bytes) {
    auto d = routed();
    const auto plain = documentJson(d);
    changeEvaluationPeriod(d, 0, std::nullopt, 300); validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 27);
    CHECK(j["definition"]["evaluation"] == Json({{"warmup", 0.0}, {"cooldown", 300.0}}));
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d); CHECK(documentJson(reopened) == j);
    // A warm-up alone stays schema 22, as before M5.9.
    changeEvaluationPeriod(d, 60, std::nullopt, 0);
    CHECK(documentJson(d)["schemaVersion"] == 22);
    CHECK(!documentJson(d)["definition"]["evaluation"].contains("cooldown"));
    // Clearing every part restores the earlier schema and bytes exactly.
    changeEvaluationPeriod(d, 0, std::nullopt, 0);
    CHECK(!d.definition->evaluation); CHECK(documentJson(d) == plain);
    // The committed example carries no cool-down and keeps schema 17.
    CHECK(documentJson(parseDocument(fourLegJson()))["schemaVersion"] == 17);
}
TEST(cooldown, cd1_older_schemas_and_bad_cool_downs_are_refused) {
    auto d = routed(); changeEvaluationPeriod(d, 0, std::nullopt, 300);
    const auto j = documentJson(d);
    auto older = j; older["schemaVersion"] = 26;
    test::throws([&] { parseDocument(older); }, "definition.evaluation.cooldown");
    auto newer = j; newer["schemaVersion"] = 29;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
    // Written only above 0: a stored 0 is not this writer's file.
    auto zero = j; zero["definition"]["evaluation"]["cooldown"] = 0;
    test::throws([&] { parseDocument(zero); }, "EVALUATION_PERIOD_INVALID");
    auto text = j; text["definition"]["evaluation"]["cooldown"] = "300";
    test::throws([&] { parseDocument(text); }, "EVALUATION_PERIOD_INVALID");
    // Negative, non-finite and off the 0.1 s grid.
    for (const double bad : {-10.0, std::nan(""), std::numeric_limits<double>::infinity(), 300.05}) {
        auto copy = routed(); changeEvaluationPeriod(copy, 0, std::nullopt, bad);
        test::throws([&] { validateDocument(copy); }, "EVALUATION_PERIOD_INVALID");
    }
}
TEST(cooldown, a_new_project_has_a_cool_down_and_history_undoes_it) {
    const auto fresh = newProjectDocument();
    CHECK(fresh.definition->evaluation); CHECK(fresh.definition->evaluation->cooldown == 900);
    CHECK(documentJson(fresh)["schemaVersion"] == 27);
    CHECK(parseDocument(documentJson(fresh)) == fresh);
    History h; h.reset(routed());
    CHECK(h.execute("cool-down", [](auto& doc) { changeEvaluationPeriod(doc, 0, std::nullopt, 120); }));
    CHECK(h.document().definition->evaluation->cooldown == 120);
    h.undo();
    CHECK(!h.document().definition->evaluation);
}
TEST(cooldown, the_run_lasts_duration_plus_cool_down_and_the_window_ends_at_duration) {
    auto d = routed(); changeEvaluationPeriod(d, 0, std::nullopt, 300);
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    test::near(snapshot.scenario.duration, 900);
    for (const auto& input : snapshot.scenario.inputs) CHECK(input.endTime <= 600);
    const auto spec = evaluationSpec(d, snapshot, data);
    CHECK(spec.end); test::near(*spec.end, 600); CHECK(spec.cooldown); test::near(*spec.cooldown, 300);
    const auto r = runSeed(snapshot.scenario, spec, 42).report;
    test::near(r.time, 900); test::near(r.evaluationEnd, 600);
    CHECK(movementJson(r)["evaluationPeriod"] == Json({{"warmup", 0.0}, {"end", 600.0}, {"cooldown", 300.0}}));
    CHECK(movementCsv(r).find("\nevaluationPeriod_s,0.00,600.00\ncooldown_s,300.00\n") != std::string::npos);
}
TEST(cooldown, cd2_a_trip_released_at_the_window_end_counts_when_it_finishes) {
    auto s = test::straight(); s.inputs.clear();
    const auto run = [&](std::optional<double> cooldown) {
        MovementAccumulator m(oneMovement(100, 200, cooldown));
        const auto end = stateAt(s, 210, {arrival(210)}); // released at exactly 200
        CHECK(end.time > 200); // the forcing: it finishes after the window
        m.observe(end);
        return m.report(end);
    };
    CHECK(run(std::nullopt).movements[0].vehicles == 0); // D132: it ends outside the period
    const auto cooled = run(60);
    CHECK(cooled.movements[0].vehicles == 1); test::near(*cooled.movements[0].meanDelay, 5);
    CHECK(cooled.cooldown); test::near(*cooled.cooldown, 60);
}
TEST(cooldown, cd3_a_trip_released_after_the_window_counts_nowhere) {
    auto s = test::straight(); s.inputs.clear();
    MovementAccumulator m(oneMovement(0, 100, 60));
    m.observe(stateAt(s, 105, {arrival(105)}));  // released at 95: counts
    m.observe(stateAt(s, 115, {arrival(115)}));  // released at 105: does not
    // Still in the network at the end: one released inside the window, one after it.
    auto end = test::withVehicles(s, {test::vehicle(7, 10), test::vehicle(8, 20)});
    end.vehicles[0].scheduledTime = 99; end.vehicles[1].scheduledTime = 101; end.time = 160;
    m.observe(end);
    const auto r = m.report(end);
    CHECK(r.movements[0].vehicles == 1);
    CHECK(r.movements[0].unfinished == 1);
    CHECK(r.completed == 2); // run totals stay whole-run
}
TEST(cooldown, cd4_gridlock_stays_unfinished_and_warned) {
    auto s = test::straight(); s.duration = 60 + 120;
    s.inputs = {{"input", "route", "car", 1800, 0, 60}};
    s.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    std::vector<SeedRun> runs;
    for (const std::uint32_t seed : {42u, 43u}) {
        runs.push_back(runSeed(s, oneMovement(0, 60, 120), seed));
        const auto& r = runs.back().report;
        CHECK(r.completed == 0); // the forcing: nothing passes the red
        CHECK(r.movements[0].unfinished == runs.back().generated);
        CHECK(r.movements[0].unfinished > 0);
    }
    const auto warned = movementsWithUnfinished(aggregate(runs));
    CHECK(warned.size() == 1); CHECK(warned[0] == "in → out");
}
TEST(cooldown, cd5_no_demand_is_released_during_the_cool_down) {
    auto plain = test::straight(); plain.duration = 60;
    auto cooled = plain; cooled.duration = 60 + 60;
    const auto spec = oneMovement(0, 60, 60);
    // Same arrivals either way: the cool-down adds ticks, not demand.
    CHECK(runSeed(plain, oneMovement(0, std::nullopt, std::nullopt), 42).generated == runSeed(cooled, spec, 42).generated);
    auto state = createSimulation(cooled, 42);
    double latest = 0;
    while (state.tick < totalTicks(cooled)) {
        state = stepSimulation(std::move(state));
        for (const auto& v : state.vehicles) latest = std::max(latest, v.scheduledTime);
        for (const auto& input : state.inputs) for (const auto& v : input.queue) latest = std::max(latest, v.scheduledTime);
    }
    CHECK(latest > 0); CHECK(latest < 60);
    CHECK(state.vehicles.empty()); // a free road clears in the cool-down
}
TEST(cooldown, cd6_no_cool_down_keeps_the_d132_numbers) {
    const auto d = parseDocument(fourLegJson()); const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    test::near(snapshot.scenario.duration, 900);
    const auto spec = evaluationSpec(d, snapshot, data);
    CHECK(!spec.cooldown); CHECK(!spec.end);
    const auto runs = runSeeds(snapshot.scenario, spec, {42, 43});
    const auto csv = batchCsv(aggregate(runs), runs);
    CHECK(csv.find("Cool-down") == std::string::npos);
    CHECK(!batchJson(aggregate(runs), runs)["evaluationPeriod"].contains("cooldown"));
    CHECK(movementCsv(runs[0].report).find("cooldown_s") == std::string::npos);
    // Re-saved through the M5.9 codec, the same file gives the same bytes and the same batch.
    const auto resaved = parseDocument(documentJson(d));
    CHECK(documentJson(resaved) == documentJson(d));
    const auto again = runSeeds(compileDocument(resaved, data).scenario, evaluationSpec(resaved, compileDocument(resaved, data), data), {42, 43});
    CHECK(batchCsv(aggregate(again), again) == csv);
}
TEST(cooldown, cd7_four_leg_in_flight_traffic_is_no_longer_unfinished) {
    auto d = parseDocument(fourLegJson()); const auto data = test::root() / "data";
    const std::vector<std::uint32_t> seeds{42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
    {   // The forcing: without a cool-down in-flight traffic is warned (three seeds suffice here).
        const auto snapshot = compileDocument(d, data);
        const auto runs = runSeeds(snapshot.scenario, evaluationSpec(d, snapshot, data), {42, 43, 44});
        CHECK(movementsWithUnfinished(aggregate(runs)).size() > 1);
    }
    changeEvaluationPeriod(d, 0, std::nullopt, kFourLegCooldown);
    const auto snapshot = compileDocument(d, data);
    const auto runs = runSeeds(snapshot.scenario, evaluationSpec(d, snapshot, data), seeds);
    const auto report = aggregate(runs);
    CHECK(movementsWithUnfinished(report).empty());
    for (const auto& m : report.movements) CHECK(m.unfinished.mean.value_or(0) == 0);
}
TEST(cooldown, cd8_section_trips_follow_the_window_by_release) {
    auto s = test::straight(); s.duration = 60;
    s.inputs = {{"input", "route", "car", 1800, 0, 60}};
    auto spec = oneMovement(0, std::nullopt, std::nullopt);
    spec.sections = {{"section", {{"road", 100}}, {{"road", 140}}, {}, ""}};
    const auto plain = runSeed(s, spec, 42).report.sections[0];
    CHECK(plain.unfinished > 0); // the forcing: trips are open in the section when the run ends
    s.duration = 60 + 60; spec.end = 60; spec.cooldown = 60;
    const auto cooled = runSeed(s, spec, 42).report.sections[0];
    CHECK(cooled.unfinished == 0);
    CHECK(cooled.vehicles >= plain.vehicles + plain.unfinished); CHECK(cooled.vehicles > plain.vehicles);
}
