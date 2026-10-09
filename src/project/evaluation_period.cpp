#include "evaluation_period.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <cmath>

namespace trafficsim {
namespace {
[[noreturn]] void invalid() { throw ValidationError({{"EVALUATION_PERIOD_INVALID", "definition.evaluation"}}); }
}
void rejectEvaluationPeriodBefore22(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("evaluation"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.evaluation"}});
}
std::optional<EvaluationPeriod> parseEvaluationPeriod(const Json& definition) {
    if (!definition.contains("evaluation")) return std::nullopt;
    const auto& e = definition.at("evaluation");
    if (!e.is_object() || !e.contains("warmup") || !e.at("warmup").is_number()) invalid();
    for (const auto& [key, value] : e.items())
        if (key != "warmup" && (key != "end" || !value.is_number())) invalid();
    EvaluationPeriod period{e.at("warmup").get<double>(), {}};
    if (e.contains("end")) period.end = e.at("end").get<double>();
    return period;
}
void addEvaluationPeriodJson(const AuthoringDefinition& d, Json& definition) {
    if (!d.evaluation) return;
    Json e = {{"warmup", d.evaluation->warmup}};
    if (d.evaluation->end) e["end"] = *d.evaluation->end;
    definition["evaluation"] = e;
}
void validateEvaluationPeriod(const AuthoringDefinition& d) {
    if (!d.evaluation) return;
    const auto warmup = d.evaluation->warmup;
    const auto end = d.evaluation->end.value_or(d.duration);
    if (!std::isfinite(warmup) || !std::isfinite(end) || warmup < 0 || !(end > warmup) || end > d.duration) invalid();
}
ProjectDocument newProjectDocument() {
    ProjectDocument d;
    d.definition.emplace();
    d.definition->duration = 4500; d.definition->timeStep = 0.1;
    d.definition->evaluation = EvaluationPeriod{900, {}};
    return d;
}
}
