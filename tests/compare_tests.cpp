#include "test.hpp"
#include "../src/runner/compare.hpp"
#include "../src/project/comparison_output.hpp"
#include "../src/project/csv_format.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <cmath>
#include <fstream>
using namespace trafficsim;
// M5.8a (D148, BATCH §7): scenario comparison. CMP1-CMP4 pin the statistic and the matching by
// hand; CMP5-CMP7 run four-leg-signalised.
namespace {
Estimate estimate(std::size_t n, std::optional<double> mean, std::optional<double> sd = std::nullopt) {
    return {n, mean, sd, std::nullopt};
}
BatchMovementRow movement(const std::string& name, double delay) {
    return {name, {}, estimate(3, delay, 1.0), {}, {}};
}
BatchReport report(std::vector<BatchMovementRow> movements) {
    BatchReport r; r.seeds = {42, 43, 44}; r.movements = std::move(movements);
    r.meanDelay = estimate(3, 10.0, 2.0);
    return r;
}
ProjectDocument fourLegDocument() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    return parseDocument(j);
}
ComparedBatch batchOf(const ProjectDocument& d, const std::string& name, const std::vector<std::uint32_t>& seeds) {
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    ComparedBatch b{name, {}, runSeeds(snapshot.scenario, evaluationSpec(d, snapshot, data), seeds)};
    b.report = aggregate(b.runs);
    return b;
}
// Four-leg over seeds 42-43 and an identical copy, run once for CMP5-CMP7 (a Debug four-leg seed
// is the cost of this group).
const std::vector<std::uint32_t> kSeeds{42, 43};
const ComparedBatch& fourLeg() { static const auto b = batchOf(fourLegDocument(), "four-leg.traffic.json", kSeeds); return b; }
const ComparedBatch& fourLegCopy() { static const auto b = batchOf(fourLegDocument(), "copy.traffic.json", kSeeds); return b; }
}
TEST(compare, cmp1_welch_by_hand) {
    // Base 1, 2, 3, 4 (mean 2.5, variance 5/3); alternative 2, 4, 6, 8, 10 (mean 6, variance 10).
    const auto d = welch(estimate(4, 2.5, std::sqrt(5.0 / 3)), estimate(5, 6.0, std::sqrt(10.0)));
    CHECK(d.nBase == 4); CHECK(d.nAlternative == 5);
    test::near(*d.difference, 3.5);
    // SE^2 = 5/12 + 2 = 29/12; nu = (29/12)^2 / ((5/12)^2 / 3 + 2^2 / 4) = 5.52079...
    const double variance = 29.0 / 12, nu = variance * variance / ((5.0 / 12) * (5.0 / 12) / 3 + 1);
    test::near(*d.degreesOfFreedom, nu); test::near(nu, 5.520788, 1e-6);
    CHECK(*d.degreesOfFreedom >= 3 && *d.degreesOfFreedom <= 7); // min(n) - 1 .. n_b + n_a - 2
    // floor(nu) = 5: t = 2.571, the lower whole df.
    test::near(*d.halfWidth95, 2.571 * std::sqrt(variance));
}
TEST(compare, cmp2_swapping_negates_the_difference_only) {
    const auto base = estimate(4, 2.5, std::sqrt(5.0 / 3)), alternative = estimate(5, 6.0, std::sqrt(10.0));
    const auto forward = welch(base, alternative), back = welch(alternative, base);
    CHECK(*back.difference == -*forward.difference);
    CHECK(*back.halfWidth95 == *forward.halfWidth95); CHECK(*back.degreesOfFreedom == *forward.degreesOfFreedom);
    const auto a = report({movement("a", 10), movement("b", 20)}), b = report({movement("a", 13), movement("b", 18)});
    const auto ab = compareBatches(a, b), ba = compareBatches(b, a);
    for (std::size_t i = 0; i < ab.movements.size(); ++i) {
        CHECK(*ba.movements[i].value.difference == -*ab.movements[i].value.difference);
        CHECK(ba.movements[i].value.halfWidth95 == ab.movements[i].value.halfWidth95);
    }
}
TEST(compare, cmp3_rows_match_by_name_and_none_is_dropped) {
    const auto base = report({movement("a", 1), movement("dup", 2), movement("b", 3), movement("dup", 4), movement("x", 5)});
    const auto alternative = report({movement("b", 6), movement("y", 7), movement("a", 8), movement("dup", 9),
                                     movement("z", 10), movement("z", 11)});
    const auto c = compareBatches(base, alternative);
    CHECK(c.movements.size() == 2); // the base's order
    CHECK(c.movements[0].name == "a"); test::near(*c.movements[0].value.difference, 7);
    CHECK(c.movements[1].name == "b"); test::near(*c.movements[1].value.difference, 3);
    CHECK(c.unmatchedMovements.baseOnly == std::vector<std::string>{"x"});
    CHECK(c.unmatchedMovements.alternativeOnly == std::vector<std::string>{"y"});
    CHECK((c.unmatchedMovements.ambiguous == std::vector<std::string>{"dup", "z"}));
    // ...and the output says so on its own line, and in JSON.
    const ComparedBatch b{"base.traffic.json", base, {}}, a{"alt.traffic.json", alternative, {}};
    const auto csv = comparisonCsv(c, b, a);
    CHECK(csv.find("# Unmatched, not compared: movement base only \"x\"; movement alternative only \"y\"; "
                   "movement ambiguous \"dup\" \"z\"\n") != std::string::npos);
    CHECK(comparisonJson(c, b, a)["unmatched"]["movements"]["ambiguous"].size() == 2);
}
TEST(compare, cmp4_refusals_and_edge_cases) {
    auto other = report({movement("a", 1)}); other.seeds = {42, 43, 45};
    test::throws([&] { compareBatches(report({movement("a", 1)}), other); }, "different seed lists");
    EvaluationSpec base, alternative;
    requireSameEvaluationPeriod(base, 900, alternative, 900);
    alternative.end = 900; // the same window, once written out
    requireSameEvaluationPeriod(base, 900, alternative, 900);
    test::throws([&] { requireSameEvaluationPeriod(base, 600, alternative, 900); }, "different evaluation periods");
    alternative.warmup = 300;
    test::throws([&] { requireSameEvaluationPeriod(base, 900, alternative, 900); }, "different evaluation periods");
    alternative.warmup = 0; alternative.cooldown = 300;
    test::throws([&] { requireSameEvaluationPeriod(base, 900, alternative, 900); }, "different evaluation periods");
    // One seed on a side: the difference, no interval.
    const auto single = welch(estimate(1, 5.0), estimate(3, 7.0, 1.0));
    test::near(*single.difference, 2); CHECK(!single.halfWidth95); CHECK(!single.degreesOfFreedom);
    // Both SDs zero: a zero half-width and no degrees of freedom.
    const auto flat = welch(estimate(3, 2.0, 0.0), estimate(3, 4.0, 0.0));
    test::near(*flat.difference, 2); CHECK(*flat.halfWidth95 == 0); CHECK(!flat.degreesOfFreedom);
    // No vehicle on a side: no difference at all.
    const auto empty = welch(estimate(0, std::nullopt), estimate(3, 4.0, 1.0));
    CHECK(!empty.difference); CHECK(!empty.halfWidth95);
}
TEST(compare, cmp5_a_project_against_itself_differs_by_exactly_zero) {
    const auto& base = fourLeg();
    const auto c = compareBatches(base.report, fourLegCopy().report);
    CHECK(c.movements.size() == base.report.movements.size()); CHECK(c.unmatchedMovements.empty());
    for (const auto& row : c.movements) {
        CHECK(*row.value.difference == 0);
        CHECK(*row.value.halfWidth95 > 0); // two independent batches, not a paired zero
    }
    for (const auto& row : c.queues) CHECK(*row.value.difference == 0);
    CHECK(*c.network.difference == 0); CHECK(*c.network.halfWidth95 > 0);
}
TEST(compare, cmp6_each_difference_is_the_two_batches_means_apart) {
    auto busier = fourLegDocument();
    busier.definition->inputs[0].vehiclesPerHour *= 1.5;
    const auto& base = fourLeg();
    const auto alternative = batchOf(busier, "busier.traffic.json", kSeeds);
    const auto c = compareBatches(base.report, alternative.report);
    CHECK(c.movements.size() == base.report.movements.size());
    bool changed = false;
    for (std::size_t i = 0; i < c.movements.size(); ++i) {
        const auto& b = base.report.movements[i].meanDelay; const auto& a = alternative.report.movements[i].meanDelay;
        CHECK(c.movements[i].name == base.report.movements[i].name);
        CHECK(*c.movements[i].value.difference == *a.mean - *b.mean); // bit for bit
        changed = changed || *c.movements[i].value.difference != 0;
    }
    CHECK(changed); // the forcing: the alternative really differs
    CHECK(*c.network.difference == *alternative.report.meanDelay.mean - *base.report.meanDelay.mean);
}
TEST(compare, cmp7_output_marker_first_and_reproducible) {
    const auto& base = fourLeg(); const auto& alternative = fourLegCopy();
    const auto c = compareBatches(base.report, alternative.report);
    const auto csv = comparisonCsv(c, base, alternative);
    CHECK(csv.rfind("# TrafficSim - not yet validated. Difference of means, alternative minus base", 0) == 0);
    CHECK(csv.find("# Base: \"four-leg.traffic.json\"; alternative: \"copy.traffic.json\"\n") != std::string::npos);
    CHECK(csv.find("movement,base_n,base_meanDelay_s,alternative_n,alternative_meanDelay_s,difference_s,ci95_s,df\n") != std::string::npos);
    CHECK(csv.find("approach,base_n,base_meanQueue_m,") != std::string::npos);
    CHECK(csv.find("\"network\",2,") != std::string::npos);
    CHECK(csv.find("section,") == std::string::npos); // four-leg has no section
    const auto j = comparisonJson(c, base, alternative);
    CHECK(j["validated"] == false); CHECK(!j.contains("sections"));
    CHECK(j["movements"][0]["difference"] == 0.0);
    // Same inputs, same bytes.
    CHECK(comparisonCsv(compareBatches(base.report, alternative.report), base, alternative) == csv);
    CHECK(csvToTsv(csv).rfind("# TrafficSim - not yet validated.", 0) == 0); // M5.8b's Copy path
}
