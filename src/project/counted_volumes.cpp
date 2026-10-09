#include "counted_volumes.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace trafficsim {
const RoutingDecision* countedDecision(const AuthoringDefinition& d, const VehicleInput& input) {
    for (const auto& x : d.routingDecisions) {
        if (!input.routingDecisionId.empty() ? x.id == input.routingDecisionId
                                             : !input.linkId.empty() && x.linkId == input.linkId && !x.position)
            return &x;
    }
    return nullptr;
}
std::vector<VolumeInterval> countedVolumes(const RoutingDecision& decision) {
    std::vector<VolumeInterval> periods;
    for (std::size_t k = 0; k < decision.intervals.size(); ++k) {
        const auto& p = decision.intervals[k];
        if (!(p.endTime > p.startTime)) continue; // routingDecisionIssues names it
        double vehicles = 0;
        for (const auto& route : decision.routes)
            if (k < route.intervalFlows.size()) vehicles += route.intervalFlows[k];
        periods.push_back({p.startTime, p.endTime, vehicles * 3600 / (p.endTime - p.startTime)});
    }
    return periods;
}
void syncCountedVolumes(AuthoringDefinition& d) {
    for (auto& input : d.inputs) {
        if (!input.volumeFromCounts) continue;
        const auto* decision = countedDecision(d, input);
        input.intervals = decision && !decision->position ? countedVolumes(*decision) : std::vector<VolumeInterval>{};
        if (input.intervals.empty()) input.vehiclesPerHour = 0; // countedVolumeIssues says why
        else deriveInputTotals(input);
    }
}
std::vector<ValidationIssue> countedVolumeIssues(const AuthoringDefinition& d) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < d.inputs.size(); ++i) {
        if (!d.inputs[i].volumeFromCounts) continue;
        const auto path = "inputs[" + std::to_string(i) + "].volumeFromCounts";
        const auto* decision = countedDecision(d, d.inputs[i]);
        if (!decision) issues.push_back({"INPUT_COUNTS_NO_DECISION", path});
        else if (decision->position) issues.push_back({"INPUT_COUNTS_POSITIONED", path});
        else {
            const auto periods = countedVolumes(*decision);
            if (std::none_of(periods.begin(), periods.end(), [](const auto& p) { return p.vehiclesPerHour > 0; }))
                issues.push_back({"INPUT_COUNTS_EMPTY", path});
        }
    }
    return issues;
}
bool usesCountedVolumes(const ProjectDocument& d) {
    return d.definition && std::any_of(d.definition->inputs.begin(), d.definition->inputs.end(),
                                       [](const auto& i) { return i.volumeFromCounts; });
}
void rejectCountedVolumesBefore26(const Json& j) {
    if (!j.contains("definition") || !j.at("definition").is_object() || !j.at("definition").contains("inputs") ||
        !j.at("definition").at("inputs").is_array()) return;
    for (const auto& input : j.at("definition").at("inputs"))
        if (input.is_object() && input.contains("volumeFromCounts"))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.inputs.volumeFromCounts"}});
}
}
