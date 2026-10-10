#pragma once
#include "batch.hpp"

namespace trafficsim {
// Scenario comparison (M5.8a, D148, BATCH §7): two batches over one seed list, alternative minus
// base, row by row. Equal seeds are not common random numbers -- each run's stream diverges once
// the projects differ -- so every interval is Welch's for independent samples. NOT HCM control
// delay, NOT LOS, not validated (rule 4).
struct Difference {
    std::size_t nBase{}, nAlternative{};
    std::optional<double> base, alternative;              // the two means
    std::optional<double> difference;                     // alternative - base
    std::optional<double> halfWidth95, degreesOfFreedom;  // empty with n < 2 on a side
    bool operator==(const Difference&) const = default;
};
// d = mean_a - mean_b, SE = sqrt(s_b^2/n_b + s_a^2/n_a), Welch-Satterthwaite nu, half-width
// tQuantile975(floor(nu)) * SE. Both SDs zero: half-width 0 and no nu.
Difference welch(const Estimate& base, const Estimate& alternative);

struct ComparisonRow {
    std::string name; Difference value;
    bool operator==(const ComparisonRow&) const = default;
};
// The names a block did not compare: in one batch only, or more than once in either.
struct Unmatched {
    std::vector<std::string> baseOnly, alternativeOnly, ambiguous;
    bool empty() const { return baseOnly.empty() && alternativeOnly.empty() && ambiguous.empty(); }
    bool operator==(const Unmatched&) const = default;
};
struct Comparison {
    std::vector<std::uint32_t> seeds;
    std::vector<ComparisonRow> movements, sections; // mean delay, s
    std::vector<ComparisonRow> queues;              // mean queue length, m
    Difference network;                             // network mean delay, s
    Unmatched unmatchedMovements, unmatchedSections, unmatchedQueues;
    bool operator==(const Comparison&) const = default;
};
// Rows matched by name in the base's order. Throws when the batches ran different seed lists.
Comparison compareBatches(const BatchReport& base, const BatchReport& alternative);
}
