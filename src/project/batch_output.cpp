#include "batch_output.hpp"
#include "csv_format.hpp"
#include "los_output.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace trafficsim {
namespace {
std::string measure(std::size_t runs) {
    return "Simulated movement delay, not HCM control delay or LOS; mean of " + std::to_string(runs) +
           " runs, 95% CI of the mean (Student t)";
}
Json value(const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); }
// M5.5: letters on the mean over seeds, weighted by the mean vehicles.
std::vector<LosInput> losInputs(const BatchReport& r) {
    std::vector<LosInput> inputs;
    for (const auto& s : r.sections) inputs.push_back({s.approach, s.controlType, s.vehicles.mean.value_or(0), s.meanDelay.mean});
    return inputs;
}
Json estimateJson(const Estimate& e) {
    return {{"n", e.n}, {"mean", value(e.mean)}, {"sd", value(e.sd)}, {"halfWidth95", value(e.halfWidth95)}};
}
std::vector<SeedRun> bySeed(std::vector<SeedRun> runs) {
    std::sort(runs.begin(), runs.end(), [](const SeedRun& a, const SeedRun& b) { return a.seed < b.seed; });
    return runs;
}
std::string seedList(const std::vector<std::uint32_t>& seeds) {
    std::string out;
    for (const auto s : seeds) out += (out.empty() ? "" : " ") + std::to_string(s);
    return out;
}
}
Json batchJson(const BatchReport& r, const std::vector<SeedRun>& runs) {
    Json j;
    j["validated"] = false;
    j["measure"] = measure(r.seeds.size());
    j["seeds"] = r.seeds;
    j["overloadedSeeds"] = r.overloadedSeeds;
    j["movements"] = Json::array();
    for (const auto& m : r.movements)
        j["movements"].push_back({{"movement", m.name}, {"vehicles", estimateJson(m.vehicles)},
                                  {"meanDelay", estimateJson(m.meanDelay)}, {"meanTravelTime", estimateJson(m.meanTravelTime)},
                                  {"unfinished", estimateJson(m.unfinished)}});
    j["queues"] = Json::array();
    for (const auto& q : r.queues)
        j["queues"].push_back({{"approach", q.name}, {"meanLength", estimateJson(q.meanLength)}, {"maxLength", estimateJson(q.maxLength)}});
    if (!r.sections.empty()) { // M5.4: only with a section, so other projects keep their bytes
        j["sections"] = Json::array();
        for (const auto& m : r.sections)
            j["sections"].push_back({{"section", m.name}, {"vehicles", estimateJson(m.vehicles)},
                                     {"meanDelay", estimateJson(m.meanDelay)}, {"meanTravelTime", estimateJson(m.meanTravelTime)},
                                     {"unfinished", estimateJson(m.unfinished)},
                                     {"controlType", m.controlType ? Json(*m.controlType) : Json(nullptr)},
                                     {"los", losCell(m.meanDelay.mean, m.controlType, r.los).empty() ? Json(nullptr)
                                                                                                    : Json(losCell(m.meanDelay.mean, m.controlType, r.los))}});
        addLosJson(j, losInputs(r), r.los); // M5.5
    }
    j["meanDelay"] = estimateJson(r.meanDelay); j["completed"] = estimateJson(r.completed);
    j["pending"] = estimateJson(r.pending); j["safetyClamps"] = estimateJson(r.safetyClamps);
    j["movementsWithUnfinished"] = movementsWithUnfinished(r);
    if (!runs.empty()) {
        const auto& first = runs.front().report;
        j["evaluationPeriod"] = {{"warmup", first.warmup}, {"end", first.evaluationEnd}};
        if (first.cooldown) j["evaluationPeriod"]["cooldown"] = *first.cooldown; // M5.9
    }
    j["perSeed"] = Json::array();
    for (const auto& run : bySeed(runs))
        j["perSeed"].push_back({{"seed", run.seed}, {"generated", run.generated}, {"completed", run.report.completed},
                                {"active", run.report.active}, {"pending", run.report.pending},
                                {"safetyClamps", run.report.safetyClamps}, {"overloaded", run.overloaded},
                                {"meanDelay", value(run.report.meanDelay)}});
    return j;
}
std::string batchCsv(const BatchReport& r, const std::vector<SeedRun>& runs) {
    std::ostringstream out;
    out << "# TrafficSim - not yet validated. " << measure(r.seeds.size()) << ".\n";
    if (!r.overloadedSeeds.empty())
        out << kOverloadedWarning << ' ' << seedList(r.overloadedSeeds) << '\n';
    const auto stuck = movementsWithUnfinished(r);
    if (!stuck.empty()) {
        out << kUnfinishedWarning;
        for (const auto& name : stuck) out << ' ' << csvQuoted(name);
        out << '\n';
    }
    if (!runs.empty())
        out << "# Evaluation period: " << csvNumber(runs.front().report.warmup) << " s to "
            << csvNumber(runs.front().report.evaluationEnd) << " s\n";
    if (!runs.empty() && runs.front().report.cooldown) // M5.9
        out << "# Cool-down: " << csvNumber(*runs.front().report.cooldown) << " s after the demand ends\n";
    out << "movement,n,meanDelay_s,ci95_s,sd_s,vehicles_mean,meanTravelTime_s,unfinished_mean\n";
    for (const auto& m : r.movements)
        out << csvQuoted(m.name) << ',' << m.meanDelay.n << ',' << csvNumber(m.meanDelay.mean) << ','
            << csvNumber(m.meanDelay.halfWidth95) << ',' << csvNumber(m.meanDelay.sd) << ','
            << csvNumber(m.vehicles.mean) << ',' << csvNumber(m.meanTravelTime.mean) << ','
            << csvNumber(m.unfinished.mean) << '\n';
    out << "\napproach,n,meanQueue_m,ci95_m,maxQueue_m,maxQueue_ci95_m\n";
    for (const auto& q : r.queues)
        out << csvQuoted(q.name) << ',' << q.meanLength.n << ',' << csvNumber(q.meanLength.mean) << ','
            << csvNumber(q.meanLength.halfWidth95) << ',' << csvNumber(q.maxLength.mean) << ','
            << csvNumber(q.maxLength.halfWidth95) << '\n';
    if (!r.sections.empty()) { // M5.4
        out << "\nsection,n,meanDelay_s,ci95_s,sd_s,vehicles_mean,meanTravelTime_s,unfinished_mean,controlType,los\n";
        for (const auto& m : r.sections)
            out << csvQuoted(m.name) << ',' << m.meanDelay.n << ',' << csvNumber(m.meanDelay.mean) << ','
                << csvNumber(m.meanDelay.halfWidth95) << ',' << csvNumber(m.meanDelay.sd) << ','
                << csvNumber(m.vehicles.mean) << ',' << csvNumber(m.meanTravelTime.mean) << ','
                << csvNumber(m.unfinished.mean) << ',' << m.controlType.value_or("") << ','
                << losCell(m.meanDelay.mean, m.controlType, r.los) << '\n';
        writeLosCsv(out, losInputs(r), r.los); // M5.5
    }
    out << "\nseed,generated,completed,active,pending,safetyClamps,overloaded,meanDelay_s\n";
    for (const auto& run : bySeed(runs))
        out << run.seed << ',' << run.generated << ',' << run.report.completed << ',' << run.report.active << ','
            << run.report.pending << ',' << run.report.safetyClamps << ',' << (run.overloaded ? "yes" : "no") << ','
            << csvNumber(run.report.meanDelay) << '\n';
    return out.str();
}
}
