#include "batch.hpp"
#include "../core/simulation.hpp"
#include "../eval/summary.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
namespace {
void rejectDuplicates(std::vector<std::uint32_t> seeds) {
    std::sort(seeds.begin(), seeds.end());
    if (std::adjacent_find(seeds.begin(), seeds.end()) != seeds.end()) throw std::invalid_argument("duplicate seed in batch");
}
Estimate estimate(const std::vector<std::optional<double>>& values) {
    Estimate e;
    double sum = 0;
    for (const auto& v : values) if (v) { ++e.n; sum += *v; }
    if (e.n == 0) return e;
    const double n = static_cast<double>(e.n);
    e.mean = sum / n;
    if (e.n < 2) return e;
    double squares = 0;
    for (const auto& v : values) if (v) squares += (*v - *e.mean) * (*v - *e.mean);
    e.sd = std::sqrt(squares / (n - 1));
    e.halfWidth95 = tQuantile975(e.n - 1) * *e.sd / std::sqrt(n);
    return e;
}
template<class F> Estimate over(const std::vector<SeedRun>& runs, F value) {
    std::vector<std::optional<double>> values;
    values.reserve(runs.size());
    for (const auto& run : runs) values.push_back(value(run));
    return estimate(values);
}
}
double tQuantile975(std::size_t df) {
    static constexpr std::array<double, 30> table{
        12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
        2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
        2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042};
    if (df == 0) throw std::invalid_argument("t quantile needs degrees of freedom >= 1");
    if (df <= table.size()) return table[df - 1];
    if (df < 40) return table.back();
    if (df < 60) return 2.021;
    if (df < 120) return 2.000;
    return 1.980;
}
std::vector<std::string> movementsWithUnfinished(const BatchReport& r, double share) {
    std::vector<std::string> names;
    for (const auto& m : r.movements) {
        const double unfinished = m.unfinished.mean.value_or(0), total = m.vehicles.mean.value_or(0) + unfinished;
        if (unfinished > share * total) names.push_back(m.name);
    }
    return names;
}
SeedRun runSeed(const Scenario& scenario, const EvaluationSpec& spec, std::uint32_t seed, double maxPendingShare) {
    MovementAccumulator movements(spec);
    auto state = createSimulation(scenario, seed);
    movements.observe(state);
    const auto ticks = totalTicks(scenario);
    while (state.tick < ticks) { state = stepSimulation(std::move(state)); movements.observe(state); }
    SeedRun run;
    run.seed = seed;
    run.report = movements.report(state);
    run.generated = state.nextVehicleId - 1;
    run.overloaded = static_cast<double>(run.report.pending) > maxPendingShare * static_cast<double>(run.generated);
    return run;
}
std::vector<SeedRun> runSeeds(const Scenario& scenario, const EvaluationSpec& spec, const std::vector<std::uint32_t>& seeds,
                              const std::function<bool(std::size_t)>& progress) {
    rejectDuplicates(seeds);
    std::vector<SeedRun> runs;
    runs.reserve(seeds.size());
    for (const auto seed : seeds) {
        runs.push_back(runSeed(scenario, spec, seed));
        if (progress && !progress(runs.size())) return {};
    }
    return runs;
}
BatchReport aggregate(std::vector<SeedRun> runs) {
    if (runs.empty()) throw std::invalid_argument("aggregate: no runs");
    std::sort(runs.begin(), runs.end(), [](const SeedRun& a, const SeedRun& b) { return a.seed < b.seed; });
    BatchReport r;
    for (const auto& run : runs) {
        if (!r.seeds.empty() && r.seeds.back() == run.seed) throw std::invalid_argument("aggregate: duplicate seed");
        r.seeds.push_back(run.seed);
        if (run.overloaded) r.overloadedSeeds.push_back(run.seed);
        const auto& first = runs.front().report;
        bool same = run.report.movements.size() == first.movements.size() && run.report.queues.size() == first.queues.size();
        for (std::size_t i = 0; same && i < first.movements.size(); ++i) same = run.report.movements[i].name == first.movements[i].name;
        for (std::size_t i = 0; same && i < first.queues.size(); ++i) same = run.report.queues[i].name == first.queues[i].name;
        same = same && run.report.sections.size() == first.sections.size();
        for (std::size_t i = 0; same && i < first.sections.size(); ++i) same = run.report.sections[i].name == first.sections[i].name;
        if (!same) throw std::invalid_argument("aggregate: runs have different movements or approaches");
    }
    const auto& first = runs.front().report;
    for (std::size_t i = 0; i < first.movements.size(); ++i) {
        const auto row = [i](const SeedRun& run) -> const MovementRow& { return run.report.movements[i]; };
        r.movements.push_back({first.movements[i].name,
            over(runs, [&](const SeedRun& s) { return std::optional<double>(static_cast<double>(row(s).vehicles)); }),
            over(runs, [&](const SeedRun& s) { return row(s).meanDelay; }),
            over(runs, [&](const SeedRun& s) { return row(s).meanTravelTime; }),
            over(runs, [&](const SeedRun& s) { return std::optional<double>(static_cast<double>(row(s).unfinished)); })});
    }
    for (std::size_t i = 0; i < first.queues.size(); ++i)
        r.queues.push_back({first.queues[i].name,
            over(runs, [i](const SeedRun& s) { return std::optional<double>(s.report.queues[i].meanLength); }),
            over(runs, [i](const SeedRun& s) { return std::optional<double>(s.report.queues[i].maxLength); })});
    for (std::size_t i = 0; i < first.sections.size(); ++i) {
        const auto row = [i](const SeedRun& run) -> const SectionRow& { return run.report.sections[i]; };
        r.sections.push_back({first.sections[i].name,
            over(runs, [&](const SeedRun& s) { return std::optional<double>(static_cast<double>(row(s).vehicles)); }),
            over(runs, [&](const SeedRun& s) { return row(s).meanDelay; }),
            over(runs, [&](const SeedRun& s) { return row(s).meanTravelTime; }),
            over(runs, [&](const SeedRun& s) { return std::optional<double>(static_cast<double>(row(s).unfinished)); })});
    }
    r.meanDelay = over(runs, [](const SeedRun& s) { return s.report.meanDelay; });
    r.completed = over(runs, [](const SeedRun& s) { return std::optional<double>(static_cast<double>(s.report.completed)); });
    r.pending = over(runs, [](const SeedRun& s) { return std::optional<double>(static_cast<double>(s.report.pending)); });
    r.safetyClamps = over(runs, [](const SeedRun& s) { return std::optional<double>(static_cast<double>(s.report.safetyClamps)); });
    return r;
}
}
