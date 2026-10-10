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
void rejectCooldownBefore27(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("evaluation") &&
        j.at("definition").at("evaluation").is_object() && j.at("definition").at("evaluation").contains("cooldown"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.evaluation.cooldown"}});
}
std::optional<EvaluationPeriod> parseEvaluationPeriod(const Json& definition) {
    if (!definition.contains("evaluation")) return std::nullopt;
    const auto& e = definition.at("evaluation");
    if (!e.is_object() || !e.contains("warmup") || !e.at("warmup").is_number()) invalid();
    for (const auto& [key, value] : e.items())
        if (key != "warmup" && ((key != "end" && key != "cooldown") || !value.is_number())) invalid();
    EvaluationPeriod period{e.at("warmup").get<double>(), {}};
    if (e.contains("end")) period.end = e.at("end").get<double>();
    if (e.contains("cooldown")) {
        period.cooldown = e.at("cooldown").get<double>();
        if (!(period.cooldown > 0)) invalid(); // written only above 0, so a stored 0 is not ours
    }
    return period;
}
void addEvaluationPeriodJson(const AuthoringDefinition& d, Json& definition) {
    if (!d.evaluation) return;
    Json e = {{"warmup", d.evaluation->warmup}};
    if (d.evaluation->end) e["end"] = *d.evaluation->end;
    if (d.evaluation->cooldown > 0) e["cooldown"] = d.evaluation->cooldown; // M5.9
    definition["evaluation"] = e;
}
void validateEvaluationPeriod(const AuthoringDefinition& d) {
    if (!d.evaluation) return;
    const auto warmup = d.evaluation->warmup;
    const auto end = d.evaluation->end.value_or(d.duration);
    if (!std::isfinite(warmup) || !std::isfinite(end) || warmup < 0 || !(end > warmup) || end > d.duration) invalid();
    // M5.9: the run becomes duration + cooldown, which the engine requires on the time grid.
    const auto cooldown = d.evaluation->cooldown;
    if (!std::isfinite(cooldown) || cooldown < 0 || !onTimeGrid(cooldown, d.timeStep)) invalid();
}
ProjectDocument newProjectDocument() {
    ProjectDocument d;
    d.definition.emplace();
    d.definition->duration = 4500; d.definition->timeStep = 0.1;
    d.definition->evaluation = EvaluationPeriod{900, {}, 900};
    return d;
}
}
