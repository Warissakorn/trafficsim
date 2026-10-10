#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/eval/movement.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/evaluation_period.hpp"
#include "../src/project/diagnostics.hpp"
#include "../src/project/run.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>
using namespace trafficsim;
// M5.3 (D132): the evaluation period (warm-up, end) and unfinished trips. With no period every
// number is the whole-run figure it was before; with one, movement rows count trips that END in
// the period and queues average its ticks, while run totals stay whole-run.
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
ArrivedEvent arrival(double time) { return {time, 1, "route", 20, 0, 15}; } // delay 5 s each
SimState stateAt(const Scenario& s, double time, std::vector<SimEvent> events) {
    auto state = createSimulation(s, 1); state.time = time; state.events = std::move(events); return state;
}
EvaluationSpec oneMovement(double warmup, std::optional<double> end) {
    EvaluationSpec spec; spec.movementNames = {"in → out"}; spec.movementOfRoute = {{"route", 0}};
    spec.warmup = warmup; spec.end = end; return spec;
}
}
TEST(evaluationperiod, schema22_round_trips_and_files_without_it_keep_their_bytes) {
    auto d = routed();
    const auto plain = documentJson(d);
    CHECK(plain["schemaVersion"] == 17); CHECK(!plain["definition"].contains("evaluation"));
    changeEvaluationPeriod(d, 120, 540); validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 22);
    CHECK(j["definition"]["evaluation"] == Json({{"warmup", 120.0}, {"end", 540.0}}));
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d); CHECK(documentJson(reopened) == j);
    // Without an end the key is left out, not written as null.
    changeEvaluationPeriod(d, 60, std::nullopt);
    CHECK(documentJson(d)["definition"]["evaluation"] == Json({{"warmup", 60.0}}));
    // Clearing restores the earlier schema and bytes exactly.
    changeEvaluationPeriod(d, 0, std::nullopt);
    CHECK(!d.definition->evaluation); CHECK(documentJson(d) == plain);
    // The committed examples carry no period and read as warm-up 0.
    const auto fourLeg = parseDocument(fourLegJson());
    CHECK(!fourLeg.definition->evaluation); CHECK(documentJson(fourLeg)["schemaVersion"] == 17);
}
TEST(evaluationperiod, older_schemas_and_bad_periods_are_refused) {
    auto d = routed(); changeEvaluationPeriod(d, 120, std::nullopt);
    auto j = documentJson(d);
    auto older = j; older["schemaVersion"] = 21;
    test::throws([&] { parseDocument(older); }, "definition.evaluation");
    auto extra = j; extra["definition"]["evaluation"]["start"] = 3;
    test::throws([&] { parseDocument(extra); }, "EVALUATION_PERIOD_INVALID");
    auto text = j; text["definition"]["evaluation"]["warmup"] = "900";
    test::throws([&] { parseDocument(text); }, "EVALUATION_PERIOD_INVALID");
    auto newer = j; newer["schemaVersion"] = 29;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
    // duration 600: each of these leaves no period inside the run.
    const std::vector<std::pair<double, std::optional<double>>> bad{
        {-1, std::nullopt}, {std::nan(""), std::nullopt}, {600, std::nullopt}, {700, std::nullopt},
        {100, 100}, {100, 50}, {100, 601}, {0, std::numeric_limits<double>::infinity()}};
    for (const auto& [warmup, end] : bad) {
        auto copy = routed(); changeEvaluationPeriod(copy, warmup, end);
        test::throws([&] { validateDocument(copy); }, "EVALUATION_PERIOD_INVALID");
    }
    // The edges that leave a period are accepted: end equal to duration, a tiny window.
    for (const auto& [warmup, end] : std::vector<std::pair<double, std::optional<double>>>{{599, std::nullopt}, {0, 600}, {100, 100.1}}) {
        auto copy = routed(); changeEvaluationPeriod(copy, warmup, end); validateDocument(copy);
    }
    // Shortening the run below the warm-up is refused atomically through History.
    History h; h.reset(d);
    test::throws([&] { h.execute("settings", [](auto& doc) { changeEvaluationPeriod(doc, 600, std::nullopt); }); }, "EVALUATION_PERIOD_INVALID");
    CHECK(h.document() == d);
}
TEST(evaluationperiod, a_new_project_has_a_warm_up_and_an_hour_after_it) {
    const auto d = newProjectDocument();
    CHECK(d.definition); CHECK(d.definition->duration == 4500); CHECK(d.definition->timeStep == 0.1);
    CHECK(d.definition->evaluation); CHECK(d.definition->evaluation->warmup == 900); CHECK(!d.definition->evaluation->end);
    validateDocument(d);
    CHECK(parseDocument(documentJson(d)) == d);
    // Until a route or timing is authored it is still a drawing: Problems lists its topology and
    // asks for demand, rather than running demand checks on nothing.
    CHECK(onlyRunSettings(*d.definition));
    auto drawn = d; addLink(drawn, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto rows = runDiagnostics(drawn, test::root() / "data");
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "EDIT_NO_INPUTS"; }));
    putRoute(drawn, {"route", {drawn.network.links[0].id}});
    CHECK(!onlyRunSettings(*drawn.definition));
}
TEST(evaluationperiod, history_applies_and_undoes_the_period) {
    History h; h.reset(routed());
    CHECK(h.execute("period", [](auto& d) { changeRunSettings(d, 900, .1); changeEvaluationPeriod(d, 300, std::nullopt); }));
    CHECK(h.document().definition->duration == 900); CHECK(h.document().definition->evaluation->warmup == 300);
    h.undo();
    CHECK(h.document().definition->duration == 600); CHECK(!h.document().definition->evaluation);
}
TEST(evaluationperiod, arrivals_count_only_when_they_end_inside_the_period) {
    auto s = test::straight(); s.inputs.clear();
    MovementAccumulator m(oneMovement(100, 200));
    // Each equality side: 99.9 out, 100 in, 200 in, 200.1 out.
    m.observe(stateAt(s, 99.9, {arrival(99.9)}));
    m.observe(stateAt(s, 100, {arrival(100)}));
    m.observe(stateAt(s, 200, {arrival(200)}));
    const auto end = stateAt(s, 200.1, {arrival(200.1)});
    m.observe(end);
    const auto r = m.report(end);
    CHECK(r.movements[0].vehicles == 2); test::near(*r.movements[0].meanDelay, 5);
    // Run totals stay whole-run, so the vehicle accounting still closes.
    CHECK(r.completed == 4);
    CHECK(r.warmup == 100); CHECK(r.evaluationEnd == 200);
}
TEST(evaluationperiod, queues_average_only_the_period) {
    // A red for the first 60 s builds a queue that green then clears; with the warm-up covering
    // the red and its discharge, the period sees no queue at all. Input runs 0-60 s.
    auto s = test::straight(); s.duration = 120;
    s.signalPrograms = {{"p", 0, {{60, SignalColor::red}, {600, SignalColor::green}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    auto spec = oneMovement(0, std::nullopt); spec.counters = {{"approach", {{"road", 100}}}};
    spec.queue = {5 / 3.6, 10 / 3.6, 20};
    const auto whole = runSeed(s, spec, 42).report;
    CHECK(whole.queues[0].maxLength > 0); // the forcing worked: there is a queue in the red
    spec.warmup = 110; // input stopped at 60 s; the queue has long cleared
    const auto later = runSeed(s, spec, 42).report;
    CHECK(later.queues[0].maxLength == 0); CHECK(later.queues[0].meanLength == 0);
}
TEST(evaluationperiod, unfinished_trips_are_counted_per_movement) {
    auto s = test::straight(); s.duration = 60;
    s.inputs = {{"input", "route", "car", 3600, 0, 60}};
    s.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 20, "p"}};
    const auto run = runSeed(s, oneMovement(0, std::nullopt), 42);
    CHECK(run.report.active + run.report.pending > 0);
    CHECK(run.report.movements[0].unfinished == run.report.active + run.report.pending);
    CHECK(run.report.movements[0].vehicles == 0);
}
TEST(evaluationperiod, warm_up_zero_reproduces_the_whole_run_numbers) {
    const auto d = parseDocument(fourLegJson()); const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    auto spec = evaluationSpec(d, snapshot, data);
    CHECK(spec.warmup == 0); CHECK(!spec.end); // the whole run, with no bound to round against
    const auto r = runSeed(snapshot.scenario, spec, 42).report;
    std::uint64_t sum = 0; for (const auto& m : r.movements) sum += m.vehicles;
    CHECK(sum == r.completed); // every trip counted, as before M5.3
    // A warm-up removes early trips from the rows but not from the run totals.
    spec.warmup = 300;
    const auto w = runSeed(snapshot.scenario, spec, 42).report;
    std::uint64_t later = 0; for (const auto& m : w.movements) later += m.vehicles;
    CHECK(later < sum); CHECK(w.completed == r.completed);
    CHECK(movementJson(w)["evaluationPeriod"] == Json({{"warmup", 300.0}, {"end", 900.0}}));
    CHECK(movementCsv(w).find("\nevaluationPeriod_s,300.00,900.00\n") != std::string::npos);
}
TEST(evaluationperiod, batches_carry_unfinished_and_warn) {
    SeedRun a, b; a.seed = 1; b.seed = 2;
    a.report.movements = {{"m", 90, 10.0, 20.0, 10}}; b.report.movements = {{"m", 100, 12.0, 22.0, 0}};
    const auto r = aggregate({a, b});
    test::near(*r.movements[0].unfinished.mean, 5);
    // 5 of 100 mean is exactly 5 %: no warning; over it, a warning naming the movement.
    CHECK(movementsWithUnfinished(r).empty());
    a.report.movements[0].unfinished = 12;
    const auto warned = movementsWithUnfinished(aggregate({a, b}));
    CHECK(warned.size() == 1); CHECK(warned[0] == "m");
}
