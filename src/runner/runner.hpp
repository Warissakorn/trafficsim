#pragma once
#include "../eval/movement.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace trafficsim {
// M5.2 (D136): independent runs of one scenario over a list of seeds, and their aggregate. Pure:
// no I/O, no clock, no threads; the CLI and the editor format what this returns. Not validated.

// One seed's run, measured exactly as a single `trafficsim-cli --project` run is.
struct SeedRun {
    std::uint32_t seed{};
    MovementReport report;
    std::uint64_t generated{}; // vehicles created: completed + active + pending, checked per seed
    bool operator==(const SeedRun&) const = default;
};
// Each seed runs to the scenario's end. An empty or repeated seed list throws BATCH_SEEDS; a seed
// whose vehicles do not add up throws BATCH_ACCOUNTING.
std::vector<SeedRun> runSeeds(const Scenario&, const EvaluationSpec&, const std::vector<std::uint32_t>& seeds);

// A figure across seeds: n counts the seeds that have it (a movement with no completed trip in a
// seed has no delay there -- absent, never 0). SD uses n-1; the 95 % half-width is
// t(0.975, n-1) * SD / sqrt(n). Below n = 2 only the mean exists.
struct Estimate {
    std::size_t n{};
    std::optional<double> mean, sd, halfWidth95;
    bool operator==(const Estimate&) const = default;
};
Estimate estimate(const std::vector<double>& values);
// The two-sided 95 % Student-t quantile t(0.975, df), df >= 1.
double studentT975(std::size_t df);

struct BatchMovement { std::string name; Estimate vehicles, meanDelay, meanTravelTime; bool operator==(const BatchMovement&) const = default; };
struct BatchQueue { std::string name; Estimate meanLength, maxLength; bool operator==(const BatchQueue&) const = default; };
struct BatchReport {
    std::vector<SeedRun> runs; // sorted by seed
    std::vector<BatchMovement> movements;
    std::vector<BatchQueue> queues;
    Estimate meanDelay, completed, safetyClamps;
    bool operator==(const BatchReport&) const = default;
};
// Sorted by seed before anything is summed, so any order of the same seeds gives the same bits.
// Every run must come from the same scenario (same movement and approach names): BATCH_MISMATCH.
BatchReport aggregate(std::vector<SeedRun> runs);
}
