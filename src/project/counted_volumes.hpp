#pragma once
#include "document.hpp"

namespace trafficsim {
// D142 (schema 26): a vehicle input whose volume is its entry decision's turning counts, so a
// count sheet is typed once. Off, D46 holds: the input's volume is split by the counts as
// proportions. On, each counted interval enters sum(counts) * 3600 / length veh/h, and splitting
// that by the same counts sends each movement exactly the vehicles counted on it.

// The decision an input's counts come from: the one it names, or the placed decision on the Link
// it enters by. Null when it has none.
const RoutingDecision* countedDecision(const AuthoringDefinition&, const VehicleInput&);
// One period per decision interval: the base routes' counts summed, as veh/h. Type rules change
// how a type splits, never how many vehicles enter.
std::vector<VolumeInterval> countedVolumes(const RoutingDecision&);
// Rewrites every volumeFromCounts input's intervals and derived scalars from its decision. Runs
// on every read and every command, so editing the counts moves the volume in the same step.
void syncCountedVolumes(AuthoringDefinition&);
// INPUT_COUNTS_NO_DECISION, INPUT_COUNTS_POSITIONED (a D119 decision sits downstream of the
// source, so it cannot say what enters) and INPUT_COUNTS_EMPTY (no interval counts a vehicle).
std::vector<ValidationIssue> countedVolumeIssues(const AuthoringDefinition&);
bool usesCountedVolumes(const ProjectDocument&);
void rejectCountedVolumesBefore26(const Json& file);
}
