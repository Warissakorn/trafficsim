#include "runner.hpp"
#include "../core/simulation.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <stdexcept>

namespace trafficsim {
std::vector<SeedRun> runSeeds(const Scenario& scenario, const EvaluationSpec& spec, const std::vector<std::uint32_t>& seeds) {
    if (seeds.empty() || std::set<std::uint32_t>(seeds.begin(), seeds.end()).size() != seeds.size())
        throw std::invalid_argument("BATCH_SEEDS");
    std::vector<SeedRun> runs;
    runs.reserve(seeds.size());
    const auto ticks = totalTicks(scenario);
    for (const auto seed : seeds) {
        // The loop of a single project run: observe the created state, then every step.
        MovementAccumulator movements(spec);
        auto state = createSimulation(scenario, seed);
        movements.observe(state);
        while (state.tick < ticks) { state = stepSimulation(std::move(state)); movements.observe(state); }
        SeedRun run{seed, movements.report(state), state.nextVehicleId - 1};
        if (run.generated != run.report.completed + run.report.active + run.report.pending)
            throw std::logic_error("BATCH_ACCOUNTING");
        runs.push_back(std::move(run));
    }
    return runs;
}
double studentT975(std::size_t df) {
    if (df == 0) throw std::invalid_argument("BATCH_DEGREES_OF_FREEDOM");
    // Tabulated to nine decimals for df 1-30; mathematical constants, not tunable data.
    static constexpr std::array<double, 30> table{
        12.706204736, 4.302652730, 3.182446305, 2.776445105, 2.570581836, 2.446911851, 2.364624252,
        2.306004135, 2.262157163, 2.228138852, 2.200985160, 2.178812830, 2.160368656, 2.144786688,
        2.131449546, 2.119905299, 2.109815578, 2.100922040, 2.093024054, 2.085963447, 2.079613845,
        2.073873068, 2.068657610, 2.063898562, 2.059538553, 2.055529439, 2.051830516, 2.048407142,
        2.045229642, 2.042272456};
    if (df <= table.size()) return table[df - 1];
    // Above 30, the Cornish-Fisher expansion about the normal quantile (Abramowitz & Stegun
    // 26.7.5), within 1e-6 of the exact quantile there.
    const double z = 1.959963984540054, v = static_cast<double>(df);
    const double z3 = z * z * z, z5 = z3 * z * z, z7 = z5 * z * z, z9 = z7 * z * z;
    return z + (z3 + z) / (4 * v) + (5 * z5 + 16 * z3 + 3 * z) / (96 * v * v) +
           (3 * z7 + 19 * z5 + 17 * z3 - 15 * z) / (384 * v * v * v) +
           (79 * z9 + 776 * z7 + 1482 * z5 - 1920 * z3 - 945 * z) / (92160 * v * v * v * v);
}
Estimate estimate(const std::vector<double>& values) {
    Estimate e;
    e.n = values.size();
    if (values.empty()) return e;
    double sum = 0;
    for (const double x : values) sum += x; // in the given (seed) order
    const double mean = sum / static_cast<double>(e.n);
    e.mean = mean;
    if (e.n < 2) return e;
    double squares = 0;
    for (const double x : values) squares += (x - mean) * (x - mean);
    e.sd = std::sqrt(squares / static_cast<double>(e.n - 1));
    e.halfWidth95 = studentT975(e.n - 1) * *e.sd / std::sqrt(static_cast<double>(e.n));
    return e;
}
BatchReport aggregate(std::vector<SeedRun> runs) {
    if (runs.empty()) throw std::invalid_argument("BATCH_SEEDS");
    std::sort(runs.begin(), runs.end(), [](const auto& a, const auto& b) { return a.seed < b.seed; });
    for (std::size_t i = 1; i < runs.size(); ++i) if (runs[i].seed == runs[i - 1].seed) throw std::invalid_argument("BATCH_SEEDS");
    const auto& first = runs.front().report;
    for (const auto& run : runs) {
        const auto& r = run.report;
        bool same = r.movements.size() == first.movements.size() && r.queues.size() == first.queues.size();
        for (std::size_t i = 0; same && i < r.movements.size(); ++i) same = r.movements[i].name == first.movements[i].name;
        for (std::size_t i = 0; same && i < r.queues.size(); ++i) same = r.queues[i].name == first.queues[i].name;
        if (!same) throw std::invalid_argument("BATCH_MISMATCH");
    }
    // One figure across every run, skipping the runs that do not have it.
    const auto across = [&](const auto& value) {
        std::vector<double> values;
        for (const auto& run : runs) if (const std::optional<double> v = value(run.report)) values.push_back(*v);
        return estimate(values);
    };
    BatchReport report;
    for (std::size_t i = 0; i < first.movements.size(); ++i)
        report.movements.push_back({first.movements[i].name,
            across([i](const MovementReport& r) { return std::optional<double>(static_cast<double>(r.movements[i].vehicles)); }),
            across([i](const MovementReport& r) { return r.movements[i].meanDelay; }),
            across([i](const MovementReport& r) { return r.movements[i].meanTravelTime; })});
    for (std::size_t i = 0; i < first.queues.size(); ++i)
        report.queues.push_back({first.queues[i].name,
            across([i](const MovementReport& r) { return std::optional<double>(r.queues[i].meanLength); }),
            across([i](const MovementReport& r) { return std::optional<double>(r.queues[i].maxLength); })});
    report.meanDelay = across([](const MovementReport& r) { return r.meanDelay; });
    report.completed = across([](const MovementReport& r) { return std::optional<double>(static_cast<double>(r.completed)); });
    report.safetyClamps = across([](const MovementReport& r) { return std::optional<double>(static_cast<double>(r.safetyClamps)); });
    report.runs = std::move(runs);
    return report;
}
}
