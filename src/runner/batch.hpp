#pragma once
#include "../eval/movement.hpp"
#include <functional>

namespace trafficsim {
// Multi-seed batches (M5.2). Independent seeds of one compiled scenario, each observed exactly as a
// single run, then aggregated in seed order so the result never depends on the order requested or
// on which thread ran what. NOT HCM control delay, NOT LOS, not validated (rule 4).
struct SeedRun {
    std::uint32_t seed{};
    MovementReport report;
    std::uint64_t generated{}; // vehicles the inputs created: completed + active + pending
    // More than maxPendingShare of generated still waiting at a source at the end: demand the
    // network did not take. Flagged, and kept in every mean (owner's choice, D131).
    bool overloaded{};
    bool operator==(const SeedRun&) const = default;
};
// One seed: createSimulation, observe, then step and observe until totalTicks -- the CLI's loop.
SeedRun runSeed(const Scenario&, const EvaluationSpec&, std::uint32_t seed, double maxPendingShare = 0.05);
// Seeds in the order given. `progress` is called after each seed with the number done; returning
// false cancels, and a cancelled batch returns nothing rather than a partial result. Duplicate
// seeds throw: a repeated seed is the same run counted twice.
std::vector<SeedRun> runSeeds(const Scenario&, const EvaluationSpec&, const std::vector<std::uint32_t>& seeds,
                              const std::function<bool(std::size_t done)>& progress = {});

// One quantity across seeds: n values, their mean, sample SD (n - 1) and the half-width of the
// two-sided 95 % confidence interval of the mean, t(0.975, n - 1) * sd / sqrt(n). A seed without a
// value (no vehicle on the movement) is not in n. sd and halfWidth95 need n >= 2.
struct Estimate {
    std::size_t n{};
    std::optional<double> mean, sd, halfWidth95;
    bool operator==(const Estimate&) const = default;
};
struct BatchMovementRow {
    std::string name; Estimate vehicles, meanDelay, meanTravelTime, unfinished;
    bool operator==(const BatchMovementRow&) const = default;
};
struct BatchQueueRow {
    std::string name; Estimate meanLength, maxLength;
    bool operator==(const BatchQueueRow&) const = default;
};
struct BatchReport {
    std::vector<std::uint32_t> seeds, overloadedSeeds; // ascending
    std::vector<BatchMovementRow> movements;
    std::vector<BatchQueueRow> queues;
    std::vector<BatchMovementRow> sections; // M5.4: a section row has a movement row's four fields
    Estimate meanDelay, completed, pending, safetyClamps;
    bool operator==(const BatchReport&) const = default;
};
// Sorts by seed first, so any permutation of the same runs gives the same report, bit for bit.
// Throws on no runs, a duplicate seed, or runs whose movement, approach or section rows differ.
BatchReport aggregate(std::vector<SeedRun> runs);
// Two-sided 95 % Student t quantile. Tabulated for df 1-30, 40, 60 and 120; between rows the
// lower df is used, which widens the interval rather than narrowing it.
double tQuantile975(std::size_t degreesOfFreedom);
// M5.3: movements whose mean unfinished trips exceed `share` of mean (completed + unfinished) --
// completed-trip delay reads low there, because the stuck vehicles never arrive.
std::vector<std::string> movementsWithUnfinished(const BatchReport&, double share = 0.05);
}
