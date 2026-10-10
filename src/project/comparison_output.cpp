#include "comparison_output.hpp"
#include "csv_format.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace trafficsim {
namespace {
std::string measure(std::size_t runs) {
    return "Difference of means, alternative minus base: simulated delay and queues, not HCM control delay or LOS; " +
           std::to_string(runs) + " runs of each on the same seeds, independent (no common random numbers); "
           "95% CI by Welch's t";
}
Json value(const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); }
Json differenceJson(const Difference& d) {
    return {{"base", {{"n", d.nBase}, {"mean", value(d.base)}}},
            {"alternative", {{"n", d.nAlternative}, {"mean", value(d.alternative)}}},
            {"difference", value(d.difference)}, {"halfWidth95", value(d.halfWidth95)},
            {"degreesOfFreedom", value(d.degreesOfFreedom)}};
}
Json rowsJson(const std::vector<ComparisonRow>& rows, const char* key) {
    auto out = Json::array();
    for (const auto& row : rows) {
        auto j = differenceJson(row.value);
        j[key] = row.name;
        out.push_back(std::move(j));
    }
    return out;
}
Json unmatchedJson(const Unmatched& u) {
    return {{"baseOnly", u.baseOnly}, {"alternativeOnly", u.alternativeOnly}, {"ambiguous", u.ambiguous}};
}
Json sideJson(const ComparedBatch& side) {
    return {{"project", side.name}, {"overloadedSeeds", side.report.overloadedSeeds},
            {"movementsWithUnfinished", movementsWithUnfinished(side.report)}};
}
bool hasSections(const Comparison& c) { return !c.sections.empty() || !c.unmatchedSections.empty(); }
std::string seedList(const std::vector<std::uint32_t>& seeds) {
    std::string out;
    for (const auto s : seeds) out += ' ' + std::to_string(s);
    return out;
}
std::string nameList(const std::vector<std::string>& names) {
    std::string out;
    for (const auto& n : names) out += ' ' + csvQuoted(n);
    return out;
}
// "base 43 44; alternative 42" for the sides that have any.
template<class F> std::string perSide(const ComparedBatch& base, const ComparedBatch& alternative, F list) {
    std::string out;
    if (const auto b = list(base); !b.empty()) out += "base" + b;
    if (const auto a = list(alternative); !a.empty()) out += (out.empty() ? "" : "; ") + std::string("alternative") + a;
    return out;
}
void unmatchedCsv(std::string& out, const char* block, const Unmatched& u) {
    const auto part = [&](const char* kind, const std::vector<std::string>& names) {
        if (!names.empty()) out += (out.empty() ? "" : "; ") + std::string(block) + ' ' + kind + nameList(names);
    };
    part("base only", u.baseOnly);
    part("alternative only", u.alternativeOnly);
    part("ambiguous", u.ambiguous);
}
void rowCsv(std::ostringstream& out, const std::string& name, const Difference& d) {
    out << csvQuoted(name) << ',' << d.nBase << ',' << csvNumber(d.base) << ',' << d.nAlternative << ','
        << csvNumber(d.alternative) << ',' << csvNumber(d.difference) << ',' << csvNumber(d.halfWidth95) << ','
        << csvNumber(d.degreesOfFreedom) << '\n';
}
void blockCsv(std::ostringstream& out, const char* head, const char* quantity, const char* unit,
              const std::vector<ComparisonRow>& rows) {
    out << head << ",base_n,base_" << quantity << '_' << unit << ",alternative_n,alternative_" << quantity << '_' << unit
        << ",difference_" << unit << ",ci95_" << unit << ",df\n";
    for (const auto& row : rows) rowCsv(out, row.name, row.value);
}
}
void requireSameEvaluationPeriod(const EvaluationSpec& base, double baseDuration,
                                 const EvaluationSpec& alternative, double alternativeDuration) {
    if (base.warmup != alternative.warmup || base.end.value_or(baseDuration) != alternative.end.value_or(alternativeDuration) ||
        base.cooldown != alternative.cooldown)
        throw std::invalid_argument("compare: the projects have different evaluation periods (warm-up, end or cool-down)");
}
Json comparisonJson(const Comparison& c, const ComparedBatch& base, const ComparedBatch& alternative) {
    Json j;
    j["validated"] = false;
    j["measure"] = measure(c.seeds.size());
    j["seeds"] = c.seeds;
    j["base"] = sideJson(base);
    j["alternative"] = sideJson(alternative);
    if (!base.runs.empty()) {
        const auto& first = base.runs.front().report;
        j["evaluationPeriod"] = {{"warmup", first.warmup}, {"end", first.evaluationEnd}};
        if (first.cooldown) j["evaluationPeriod"]["cooldown"] = *first.cooldown;
    }
    j["movements"] = rowsJson(c.movements, "movement");
    if (hasSections(c)) j["sections"] = rowsJson(c.sections, "section");
    j["queues"] = rowsJson(c.queues, "approach");
    j["network"] = differenceJson(c.network);
    j["unmatched"] = {{"movements", unmatchedJson(c.unmatchedMovements)}, {"queues", unmatchedJson(c.unmatchedQueues)}};
    if (hasSections(c)) j["unmatched"]["sections"] = unmatchedJson(c.unmatchedSections);
    return j;
}
std::string comparisonCsv(const Comparison& c, const ComparedBatch& base, const ComparedBatch& alternative) {
    std::ostringstream out;
    out << "# TrafficSim - not yet validated. " << measure(c.seeds.size()) << ".\n";
    out << "# Base: " << csvQuoted(base.name) << "; alternative: " << csvQuoted(alternative.name) << '\n';
    if (const auto seeds = perSide(base, alternative, [](const ComparedBatch& s) { return seedList(s.report.overloadedSeeds); });
        !seeds.empty())
        out << "# WARNING: overloaded seeds (pending over 5% of generated) are included in the means: " << seeds << '\n';
    if (const auto stuck = perSide(base, alternative, [](const ComparedBatch& s) { return nameList(movementsWithUnfinished(s.report)); });
        !stuck.empty())
        out << "# WARNING: unfinished trips over 5% of the movement; its delay reads low: " << stuck << '\n';
    std::string unmatched;
    unmatchedCsv(unmatched, "movement", c.unmatchedMovements);
    unmatchedCsv(unmatched, "section", c.unmatchedSections);
    unmatchedCsv(unmatched, "approach", c.unmatchedQueues);
    if (!unmatched.empty()) out << "# Unmatched, not compared: " << unmatched << '\n';
    if (!base.runs.empty()) {
        const auto& first = base.runs.front().report;
        out << "# Evaluation period: " << csvNumber(first.warmup) << " s to " << csvNumber(first.evaluationEnd) << " s\n";
        if (first.cooldown) out << "# Cool-down: " << csvNumber(*first.cooldown) << " s after the demand ends\n";
    }
    blockCsv(out, "movement", "meanDelay", "s", c.movements);
    if (hasSections(c)) { out << '\n'; blockCsv(out, "section", "meanDelay", "s", c.sections); }
    out << '\n';
    blockCsv(out, "approach", "meanQueue", "m", c.queues);
    out << '\n';
    blockCsv(out, "network", "meanDelay", "s", {});
    rowCsv(out, "network", c.network);
    return out.str();
}
}
