#pragma once
#include "json.hpp"
#include "../runner/compare.hpp"

namespace trafficsim {
// Scenario comparison output (M5.8a, D148, BATCH §7). Each side is one project's batch, named by
// its file, with its runs so the output can state that side's overloaded seeds and unfinished
// movements. Same marker-first CSV conventions as batchCsv.
struct ComparedBatch {
    std::string name;
    BatchReport report;
    std::vector<SeedRun> runs;
};
// Refused before any run: a difference over two evaluation windows is not the scenario's effect.
// The window is the one MovementReport states -- warm-up, end (the run's end when none is set)
// and cool-down. `duration` is each compiled scenario's.
void requireSameEvaluationPeriod(const EvaluationSpec& base, double baseDuration,
                                 const EvaluationSpec& alternative, double alternativeDuration);
Json comparisonJson(const Comparison&, const ComparedBatch& base, const ComparedBatch& alternative);
std::string comparisonCsv(const Comparison&, const ComparedBatch& base, const ComparedBatch& alternative);
}
