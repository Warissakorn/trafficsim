#pragma once
// M3.2.8b: mandatory lane changes (docs/M3_8_CONTRACT.md §2). A vehicle on a stub route -- one
// whose lane cannot reach its movement's end -- moves to an adjacent chain of the same movement
// when both the gap ahead and the gap behind are safe there. Everything is read off the tick's
// pre-step snapshot and decided in vehicle-id order, with no random draw, so replay is exact.
#include "routes.hpp"

namespace trafficsim {
// Validation of the compiled spans and dead ends; called by validateScenario.
std::vector<ValidationIssue> laneChangeIssues(const Scenario&);
// Fills the index's lane-change fields. Needs `index.parts` already built.
void indexLaneChanges(const Scenario&, ScenarioIndex&);
// How many changes each route still needs: 0 for a route without a dead end, else one more than
// the best target reachable through its spans. UINT32_MAX when no chain of spans reaches a full
// route. Shared by the index and by validation, so the two cannot disagree.
std::vector<std::uint32_t> laneChangesRemaining(const Scenario&);
struct LaneChange { std::size_t vehicle{}; std::uint32_t route{}; double distance{}; };
// This tick's accepted changes, in vehicle order. `stopService` is the previous tick's table: a
// vehicle serving a Stop line does not change lanes, since its line is a distance on its route.
std::vector<LaneChange> decideLaneChanges(const Scenario&, const ScenarioIndex&, const std::vector<Vehicle>&,
                                          const std::vector<VehicleRefs>&, const std::vector<OccupiedSpan>&,
                                          const SpanBuckets&, const std::vector<StopService>& stopService);
// Cooperation (contract §2): for each vehicle, the gap to a place it holds back from so that a
// stub vehicle waiting at its dead end can change in ahead of it; +infinity for none. The nearest
// vehicle behind the waiting one's target place that can stop there comfortably and without a
// clamp holds back; any nearer pass first. Read off the snapshot; nothing is stored.
std::vector<double> courtesyHolds(const Scenario&, const ScenarioIndex&, const std::vector<Vehicle>&,
                                  const std::vector<VehicleRefs>&, const std::vector<OccupiedSpan>&, const SpanBuckets&);
}
