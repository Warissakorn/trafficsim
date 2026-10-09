#include "test.hpp"
#include "../src/runner/batch.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace trafficsim;
// M5.2. The aggregate cases pin the statistics by hand; the engine cases pin that a batch seed is
// exactly today's single run, and that every generated vehicle is accounted for.
namespace {
SeedRun seedWith(std::uint32_t seed, std::optional<double> delay, double queue = 0) {
    SeedRun run; run.seed = seed;
    run.report.movements = {{"a → b", delay ? 10u : 0u, delay, delay ? std::optional<double>(*delay + 5) : std::nullopt}};
    run.report.queues = {{"a", queue, 2 * queue}};
    run.report.meanDelay = delay; run.report.completed = delay ? 10 : 0;
    return run;
}
struct FourLeg { RunSnapshot snapshot; EvaluationSpec spec; };
FourLeg fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    const auto d = parseDocument(j);
    const auto data = test::root() / "data";
    auto snapshot = compileDocument(d, data);
    auto spec = evaluationSpec(d, snapshot, data);
    return {std::move(snapshot), std::move(spec)};
}
}
TEST(batch, t_quantile_table) {
    test::near(tQuantile975(1), 12.706, 1e-9);
    test::near(tQuantile975(9), 2.262, 1e-9);
    test::near(tQuantile975(30), 2.042, 1e-9);
    // Between table rows the lower degrees of freedom are used: wider, never narrower.
    test::near(tQuantile975(45), 2.021, 1e-9);
    test::near(tQuantile975(119), 2.000, 1e-9);
    test::near(tQuantile975(120), 1.980, 1e-9);
    test::near(tQuantile975(1000), 1.980, 1e-9);
    test::throws([] { tQuantile975(0); }, "degrees of freedom");
}
TEST(batch, aggregate_by_hand) {
    const auto r = aggregate({seedWith(1, 10, 3), seedWith(2, 20, 6), seedWith(3, 30, 9)});
    CHECK(r.seeds == std::vector<std::uint32_t>{1, 2, 3});
    const auto& d = r.movements.at(0).meanDelay;
    CHECK(d.n == 3); test::near(*d.mean, 20); test::near(*d.sd, 10);
    test::near(*d.halfWidth95, 4.303 * 10 / std::sqrt(3.0));
    test::near(*r.movements[0].meanTravelTime.mean, 25);
    test::near(*r.queues.at(0).meanLength.mean, 6); test::near(*r.queues[0].maxLength.mean, 12);
    test::near(*r.meanDelay.mean, 20);
    // A seed with no vehicles on the movement has no delay: it leaves that n, not the vehicle count.
    const auto gap = aggregate({seedWith(1, 10), seedWith(2, std::nullopt), seedWith(3, 30)});
    CHECK(gap.movements[0].meanDelay.n == 2); test::near(*gap.movements[0].meanDelay.mean, 20);
    CHECK(gap.movements[0].vehicles.n == 3); test::near(*gap.movements[0].vehicles.mean, 20.0 / 3);
    // One value has a mean and nothing else: no spread can be claimed from one run.
    const auto one = aggregate({seedWith(7, 12)});
    CHECK(one.movements[0].meanDelay.n == 1); test::near(*one.movements[0].meanDelay.mean, 12);
    CHECK(!one.movements[0].meanDelay.sd); CHECK(!one.movements[0].meanDelay.halfWidth95);
    // No values at all: n 0 and no mean.
    const auto none = aggregate({seedWith(1, std::nullopt), seedWith(2, std::nullopt)});
    CHECK(none.movements[0].meanDelay.n == 0); CHECK(!none.movements[0].meanDelay.mean);
    test::throws([] { aggregate({}); }, "no runs");
}
TEST(batch, aggregate_is_order_independent_and_checks_rows) {
    const auto forward = aggregate({seedWith(1, 10.1), seedWith(2, 20.7), seedWith(3, 30.3)});
    const auto permuted = aggregate({seedWith(3, 30.3), seedWith(1, 10.1), seedWith(2, 20.7)});
    CHECK(forward == permuted);
    auto other = seedWith(2, 5); other.report.movements[0].name = "c → d";
    test::throws([&] { aggregate({seedWith(1, 10), other}); }, "different movements");
    test::throws([&] { aggregate({seedWith(1, 10), seedWith(1, 20)}); }, "duplicate seed");
}
TEST(batch, overloaded_seeds_stay_in_the_mean) {
    auto a = seedWith(1, 10), b = seedWith(2, 40); b.overloaded = true;
    const auto r = aggregate({b, a});
    CHECK(r.overloadedSeeds == std::vector<std::uint32_t>{2});
    test::near(*r.movements[0].meanDelay.mean, 25);
}
TEST(batch, one_seed_equals_the_single_run) {
    const auto f = fourLeg();
    auto state = createSimulation(f.snapshot.scenario, 42);
    MovementAccumulator m(f.spec); m.observe(state);
    while (state.tick < totalTicks(*state.scenario)) { state = stepSimulation(std::move(state)); m.observe(state); }
    const auto single = m.report(state);
    const auto run = runSeed(f.snapshot.scenario, f.spec, 42);
    CHECK(run.seed == 42); CHECK(run.report == single);
    CHECK(movementCsv(run.report) == movementCsv(single));
    CHECK(!run.overloaded);
}
TEST(batch, every_generated_vehicle_is_accounted_for) {
    const auto f = fourLeg();
    const auto runs = runSeeds(f.snapshot.scenario, f.spec, {44, 42, 43});
    CHECK(runs.size() == 3); CHECK(runs[0].seed == 44); // results keep the requested order
    for (const auto& r : runs) {
        CHECK(r.generated > 0);
        CHECK(r.generated == r.report.completed + r.report.active + r.report.pending);
    }
    CHECK(!(runs[0].report == runs[1].report)); // seeds really differ
    test::throws([&] { runSeeds(f.snapshot.scenario, f.spec, {1, 1}); }, "duplicate seed");
    std::size_t calls = 0;
    const auto stopped = runSeeds(f.snapshot.scenario, f.spec, {1, 2, 3}, [&](std::size_t done) { ++calls; return done < 1; });
    CHECK(calls == 1); CHECK(stopped.empty()); // a cancelled batch claims nothing
}
TEST(batch, overload_flag_from_a_saturated_input) {
    // 3600 veh/h into a road held at red for the whole run: the source queue cannot drain.
    auto s = test::straight(); s.duration = 60;
    s.inputs = {{"input", "route", "car", 3600, 0, 60}};
    s.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 20, "p"}};
    EvaluationSpec spec; spec.movementNames = {"in → out"}; spec.movementOfRoute = {{"route", 0}};
    const auto blocked = runSeed(s, spec, 42);
    // The forcing worked first: vehicles really are waiting at the source.
    CHECK(blocked.report.pending > 0);
    CHECK(blocked.report.pending * 20 > blocked.generated);
    CHECK(blocked.overloaded);
    s.signalPrograms[0].phases[0].color = SignalColor::green;
    s.inputs[0].vehiclesPerHour = 300;
    CHECK(!runSeed(s, spec, 42).overloaded);
}
TEST(batch, parse_seed_list) {
    CHECK(parseSeedList("42-45") == std::vector<std::uint32_t>{42, 43, 44, 45});
    CHECK(parseSeedList("1,5,9") == std::vector<std::uint32_t>{1, 5, 9});
    CHECK(parseSeedList("1,3-4,10") == std::vector<std::uint32_t>{1, 3, 4, 10});
    CHECK(parseSeedList("7") == std::vector<std::uint32_t>{7});
    CHECK(parseSeedList("4294967294-4294967295").size() == 2);
    for (const auto* bad : {"", "5-3", "1,1", "1-3,2", "1,,2", "-3", "3-", "a", "4294967296", "1-1001"})
        test::throws([&] { parseSeedList(bad); }, "seed");
}
TEST(batch, outputs_carry_the_marker_and_the_flag) {
    auto b = seedWith(2, 40); b.overloaded = true; b.generated = 99;
    const std::vector<SeedRun> runs{seedWith(1, 10), b};
    const auto r = aggregate(runs);
    const auto csv = batchCsv(r, runs);
    CHECK(csv.starts_with("# TrafficSim - not yet validated. Simulated movement delay, not HCM control delay or LOS; mean of 2 runs"));
    CHECK(csv.find("\"a → b\",2,25.00,") != std::string::npos);
    CHECK(csv.find("# WARNING: overloaded seeds (pending over 5% of generated) are included in the means: 2") != std::string::npos);
    const auto j = batchJson(r, runs);
    CHECK(j["validated"] == false);
    CHECK(j["overloadedSeeds"] == Json::array({2}));
    CHECK(j["perSeed"][1]["generated"] == 99);
    CHECK(j["movements"][0]["meanDelay"]["n"] == 2);
}
