#pragma once
#include "document.hpp"

namespace trafficsim {
// M5.3 evaluation period codec and checks (D132). Written as `definition.evaluation` only when
// set, which is what makes a file schema 22; every other file keeps its version and bytes.
void rejectEvaluationPeriodBefore22(const Json& file);
// M5.9 (D146): `definition.evaluation.cooldown` is schema 27.
void rejectCooldownBefore27(const Json& file);
std::optional<EvaluationPeriod> parseEvaluationPeriod(const Json& definition);
void addEvaluationPeriodJson(const AuthoringDefinition&, Json& definition);
// EVALUATION_PERIOD_INVALID unless warm-up >= 0 and warm-up < end <= duration, all finite, and a
// cool-down is finite, >= 0 and on the time grid.
void validateEvaluationPeriod(const AuthoringDefinition&);
// The document a new project starts from: a 900 s warm-up, then one hour (4500 s in all), then a
// 900 s cool-down (D146).
ProjectDocument newProjectDocument();
}
