#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/eval/movement.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include "../src/runner/runner.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
using namespace trafficsim;
// M5.4 (D139): the author's evaluation period. A trip counts by its arrival time in
// (warmup, end]; queue samples by their state's time; the hysteresis runs on every state; no
// declared period leaves every figure and file as before.
namespace {
constexpr double kmh = 1 / 3.6;
EvaluationSpec oneLane(std::optional<MeasurementWindow> window) {
    EvaluationSpec spec;
    spec.movementNames = {"in → out"};
    spec.movementOfRoute = {{"route", 0}};
    spec.counters = {{"approach", {{"road", 100}}}};
    spec.queue = {5 * kmh, 10 * kmh, 20};
    spec.window = window;
    return spec;
}
Scenario road() {
    auto s = test::straight(); s.duration = 1000;
    s.signalPrograms = {{"p", 0, {{600, SignalColor::red}, {600, SignalColor::green}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    return s;
}
// A state at `time` with one vehicle 1 m before the line at `speed`, carrying `events`.
SimState at(double time, double speed, std::vector<SimEvent> events = {}) {
    auto state = test::withVehicles(road(), {test::vehicle(1, 99, speed)});
    state.time = time; state.events = std::move(events);
    return state;
}
Json fourLegJson() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return j;
}
}
TEST(evalperiod, trips_count_by_arrival_time_inside_the_window) {
    // Arrivals at 50, 150 and 950 s. By arrival only the 150 s trip is in (100, 900]; by departure
    // (arrival - travel time: 70, 870) the other one would be, so the two definitions differ here.
    const ArrivedEvent early{50, 7, "route", 40, 0, 30}, middle{150, 8, "route", 80, 5, 30}, late{950, 9, "route", 80, 0, 30};
    MovementAccumulator m(oneLane(MeasurementWindow{100, 900}));
    m.observe(at(50, 20, {early})); m.observe(at(150, 20, {middle})); m.observe(at(950, 20, {late}));
    const auto r = m.report(at(950, 20));
    CHECK(r.movements[0].vehicles == 1);
    test::near(*r.movements[0].meanDelay, tripDelay(middle));
    test::near(*r.meanDelay, tripDelay(middle));
    // Accounting still adds up: the run completed three, one measured and two outside the window.
    CHECK(r.completed == 3 && r.outsideWindow == 2 && r.unassigned == 0);
    CHECK(r.window == MeasurementWindow{100, 900});
    // The edges: a trip arriving exactly at the warm-up is outside, exactly at the end inside.
    MovementAccumulator edges(oneLane(MeasurementWindow{100, 900}));
    edges.observe(at(100, 20, {ArrivedEvent{100, 1, "route", 10, 0, 10}}));
    edges.observe(at(900, 20, {ArrivedEvent{900, 2, "route", 10, 0, 10}}));
    const auto e = edges.report(at(900, 20));
    CHECK(e.movements[0].vehicles == 1 && e.outsideWindow == 1);
}
TEST(evalperiod, queue_samples_are_the_windows_and_hysteresis_runs_throughout) {
    // Stopped at 50 s (outside), 8 km/h at 150 s: still queued only because it joined at 50 s
    // (it never fell below the 5 km/h join speed inside the window). Moving at 950 s (outside).
    MovementAccumulator m(oneLane(MeasurementWindow{100, 900}));
    m.observe(at(50, 0)); m.observe(at(150, 8 * kmh)); m.observe(at(950, 20));
    const auto r = m.report(at(950, 20));
    CHECK(r.queues[0].maxLength == 5.5);
    test::near(r.queues[0].meanLength, 5.5); // one sample, the 150 s one
    // Without a window all three states are sampled, as before M5.4.
    MovementAccumulator whole(oneLane(std::nullopt));
    whole.observe(at(50, 0)); whole.observe(at(150, 8 * kmh)); whole.observe(at(950, 20));
    const auto w = whole.report(at(950, 20));
    test::near(w.queues[0].meanLength, 2 * 5.5 / 3);
    CHECK(!w.window && w.outsideWindow == 0);
    CHECK(movementCsv(w).find("measured") == std::string::npos && !movementJson(w).contains("window"));
}
TEST(evalperiod, the_codec_writes_schema_23_only_with_a_period_and_refuses_bad_ones) {
    auto document = parseDocument(fourLegJson());
    CHECK(documentJson(document).at("schemaVersion").get<int>() < 23);
    CHECK(!documentJson(document).at("definition").contains("evaluationPeriod"));
    document.definition->evaluationPeriod = EvaluationPeriod{300, 900};
    validateDocument(document);
    const auto saved = documentJson(document);
    CHECK(saved.at("schemaVersion") == 23);
    CHECK(saved.at("definition").at("evaluationPeriod") == Json({{"warmup", 300.0}, {"end", 900.0}}));
    CHECK(parseDocument(saved) == document);
    const auto refused = [&](EvaluationPeriod p, const char* code) {
        auto d = document; d.definition->evaluationPeriod = p;
        test::throws([&] { validateDocument(d); }, code);
    };
    refused({-0.1, 900}, "EVAL_PERIOD_RANGE");
    refused({900, 900}, "EVAL_PERIOD_RANGE");
    refused({300, document.definition->duration + 0.1}, "EVAL_PERIOD_RANGE");
    refused({300.05, 900}, "EVAL_PERIOD_GRID");
    // Both values or the file is refused: none is filled in by code.
    auto missing = saved; missing["definition"]["evaluationPeriod"].erase("end");
    test::throws([&] { parseDocument(missing); }, "EVAL_PERIOD_INVALID");
    auto older = saved; older["schemaVersion"] = 22;
    test::throws([&] { parseDocument(older); }, "UNSUPPORTED_FIELD: definition.evaluationPeriod");
}
TEST(evalperiod, history_sets_and_undoes_it_and_a_shorter_run_is_refused) {
    History history; history.reset(parseDocument(fourLegJson()));
    CHECK(history.execute("period", [](auto& d) { changeEvaluationPeriod(d, EvaluationPeriod{300, 900}); }));
    CHECK(history.document().definition->evaluationPeriod == EvaluationPeriod{300, 900});
    const auto before = history.document();
    // A duration that would cut the window is refused, never clamped.
    test::throws([&] { history.execute("settings", [](auto& d) { changeRunSettings(d, 600, 0.1); }); }, "EVAL_PERIOD_RANGE");
    CHECK(history.document() == before);
    history.undo();
    CHECK(!history.document().definition->evaluationPeriod);
}
TEST(evalperiod, a_four_leg_batch_measures_the_window_and_every_seed_adds_up) {
    auto document = parseDocument(fourLegJson());
    document.definition->evaluationPeriod = EvaluationPeriod{300, 900};
    const auto snapshot = compileDocument(document, test::root() / "data");
    const auto spec = evaluationSpec(document, snapshot, test::root() / "data");
    CHECK(spec.window == MeasurementWindow{300, 900});
    const auto runs = runSeeds(snapshot.scenario, spec, {42, 43});
    for (const auto& run : runs) {
        std::uint64_t measured = run.report.unassigned;
        for (const auto& m : run.report.movements) measured += m.vehicles;
        CHECK(measured + run.report.outsideWindow == run.report.completed);
        CHECK(run.report.outsideWindow > 0 && measured > 0);
    }
    const auto batch = aggregate(runs);
    CHECK(batchCsv(batch).find("measured from 300 s (after the warm-up) to 900 s") != std::string::npos);
    CHECK(batchJson(batch).at("window") == Json({{"warmup", 300.0}, {"end", 900.0}}));
    // Runs measured over different windows are never averaged together.
    auto other = runs; other[1].report.window = MeasurementWindow{0, 900};
    test::throws([&] { aggregate(other); }, "BATCH_MISMATCH");
}
