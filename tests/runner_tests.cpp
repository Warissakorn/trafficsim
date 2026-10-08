#include "test.hpp"
#include "../src/runner/runner.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
using namespace trafficsim;
// M5.2 (D136): the batch runner. The analytic cases pin the statistics by hand; the four-leg
// cases pin that a batch measures each seed exactly as a single project run does.
namespace {
// A run with one movement "A" (delay a) and one "B" (delay b, absent when the seed had no trip).
SeedRun run(std::uint32_t seed, double a, std::optional<double> b, double queue = 0) {
    MovementReport r;
    r.movements = {{"A", 10, a, a + 30}, {"B", b ? 4u : 0u, b, b ? std::optional(*b + 20) : std::nullopt}};
    r.queues = {{"North", queue, 2 * queue}};
    r.completed = b ? 14 : 10; r.meanDelay = a;
    return {seed, r, r.completed};
}
struct FourLeg { ProjectDocument document; RunSnapshot snapshot; EvaluationSpec spec; };
FourLeg fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    FourLeg f{parseDocument(j), {}, {}};
    f.snapshot = compileDocument(f.document, test::root() / "data");
    f.spec = evaluationSpec(f.document, f.snapshot, test::root() / "data");
    return f;
}
}
TEST(runner, statistics_by_hand) {
    // A: 10, 12, 14 -> mean 12, SD 2, half-width t(0.975, 2) * 2 / sqrt(3).
    // B is absent in seed 2: n = 2 over 5 and 7 -> mean 6, SD sqrt(2), half-width t(0.975, 1).
    const auto r = aggregate({run(3, 14, 7, 3), run(1, 10, 5, 1), run(2, 12, std::nullopt, 2)});
    CHECK(r.runs[0].seed == 1 && r.runs[1].seed == 2 && r.runs[2].seed == 3); // seed order
    const auto& a = r.movements[0].meanDelay;
    CHECK(a.n == 3); test::near(*a.mean, 12); test::near(*a.sd, 2); test::near(*a.halfWidth95, 4.302652730 * 2 / std::sqrt(3.0));
    const auto& b = r.movements[1].meanDelay;
    CHECK(b.n == 2); test::near(*b.mean, 6); test::near(*b.sd, std::sqrt(2.0)); test::near(*b.halfWidth95, 12.706204736);
    CHECK(r.movements[1].meanTravelTime.n == 2); test::near(*r.movements[1].meanTravelTime.mean, 26);
    test::near(*r.queues[0].meanLength.mean, 2); test::near(*r.queues[0].maxLength.mean, 4);
    // One value: a mean and nothing to spread.
    const auto one = estimate({7.5});
    CHECK(one.n == 1 && *one.mean == 7.5 && !one.sd && !one.halfWidth95);
    CHECK(estimate({}).n == 0 && !estimate({}).mean);
    // n = 10: t(0.975, 9).
    std::vector<double> ten; for (int i = 1; i <= 10; ++i) ten.push_back(i);
    const auto e = estimate(ten);
    test::near(*e.mean, 5.5); test::near(*e.sd, std::sqrt(55.0 / 6)); test::near(*e.halfWidth95, 2.262157163 * std::sqrt(55.0 / 6) / std::sqrt(10.0));
}
TEST(runner, student_t_table_and_expansion) {
    test::near(studentT975(1), 12.706204736, 1e-9); test::near(studentT975(30), 2.042272456, 1e-9);
    // Past the table, against tabulated quantiles.
    for (const auto& [df, t] : std::vector<std::pair<std::size_t, double>>{
             {31, 2.039513446}, {40, 2.021075390}, {60, 2.000297822}, {120, 1.979930405}, {1000, 1.962339055}})
        test::near(studentT975(df), t, 1e-6);
    test::throws([] { studentT975(0); }, "BATCH_DEGREES_OF_FREEDOM");
}
TEST(runner, seed_order_does_not_change_the_result) {
    const std::vector<SeedRun> forward{run(1, 10, 5), run(2, 12, std::nullopt), run(3, 14, 7)};
    auto backward = forward; std::reverse(backward.begin(), backward.end());
    CHECK(aggregate(forward) == aggregate(backward));
    CHECK(batchJson(aggregate(forward)).dump() == batchJson(aggregate(backward)).dump());
    CHECK(batchCsv(aggregate(forward)) == batchCsv(aggregate(backward)));
}
TEST(runner, refuses_bad_batches) {
    test::throws([] { aggregate({}); }, "BATCH_SEEDS");
    test::throws([] { aggregate({run(1, 10, 5), run(1, 11, 6)}); }, "BATCH_SEEDS");
    auto other = run(2, 12, 6); other.report.movements[1].name = "C";
    test::throws([&] { aggregate({run(1, 10, 5), other}); }, "BATCH_MISMATCH");
    const auto f = fourLeg();
    test::throws([&] { runSeeds(f.snapshot.scenario, f.spec, {}); }, "BATCH_SEEDS");
    test::throws([&] { runSeeds(f.snapshot.scenario, f.spec, {42, 42}); }, "BATCH_SEEDS");
}
TEST(runner, one_seed_batch_is_the_single_run) {
    const auto f = fourLeg();
    MovementAccumulator single(f.spec);
    auto state = createSimulation(f.snapshot.scenario, 42); single.observe(state);
    while (state.tick < totalTicks(f.snapshot.scenario)) { state = stepSimulation(std::move(state)); single.observe(state); }
    const auto runs = runSeeds(f.snapshot.scenario, f.spec, {42});
    CHECK(runs.size() == 1 && runs[0].report == single.report(state));
    // A one-seed batch's means are the run's own figures.
    const auto r = aggregate(runs);
    CHECK(*r.movements[0].meanDelay.mean == *runs[0].report.movements[0].meanDelay && !r.movements[0].meanDelay.sd);
}
TEST(runner, every_seed_accounts_for_its_vehicles) {
    // runSeeds checks generated = completed + active + pending itself; this is the forcing.
    const auto f = fourLeg();
    const auto runs = runSeeds(f.snapshot.scenario, f.spec, {42, 43, 44, 45, 46});
    for (const auto& r : runs) CHECK(r.generated == r.report.completed + r.report.active + r.report.pending && r.generated > 0);
    const auto report = aggregate(runs);
    CHECK(report.movements.size() == 12 && report.movements[0].meanDelay.n == 5 && report.movements[0].meanDelay.halfWidth95);
    const auto csv = batchCsv(report);
    CHECK(csv.starts_with("# TrafficSim - not yet validated. Simulated movement delay, not HCM control delay or LOS; 5 seeds"));
    CHECK(csv.find("\nseed,generated,completed,active,pending") != std::string::npos);
    const auto j = batchJson(report);
    CHECK(j["validated"] == false && j["seeds"].size() == 5 && j["runs"].size() == 5);
}
