#include "lane_changes.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace trafficsim {
namespace {
void fold(LaneChangeRow& row, const auto& tally) {
    row.changes += tally.changes;
    row.changedVehicles += tally.changes > 0;
    row.beforeDeadEnd.insert(row.beforeDeadEnd.end(), tally.before.begin(), tally.before.end());
    row.atDistance.insert(row.atDistance.end(), tally.at.begin(), tally.at.end());
    row.unplaced += tally.unplaced;
    row.waitingVehicles += tally.wait > 0;
    row.waitSeconds += tally.wait;
    row.longestWait = std::max(row.longestWait, tally.longest);
    row.discretionaryChanges += tally.discretionary;
    row.quickRepeats += tally.quickRepeats;
    row.quickBack += tally.quickBack;
    row.quickAfterMandatory += tally.quickAfterMandatory;
    row.quickOnward += tally.quickOnward;
}
}
LaneChangeAccumulator::LaneChangeAccumulator(const EvaluationSpec& spec)
    : movementOfRoute_(spec.movementOfRoute) {
    for (const auto& name : spec.movementNames) rows_.push_back({name});
    rows_.push_back({"unassigned"});
}
void LaneChangeAccumulator::bind(const SimState& state) {
    if (bound_ == state.scenario.get()) return;
    bound_ = state.scenario.get();
    slotOfRoute_.clear();
    for (std::size_t r = 0; r < bound_->routes.size(); ++r) slotOfRoute_[bound_->routes[r].id] = r;
}
void LaneChangeAccumulator::observe(const SimState& state) {
    bind(state);
    const auto& s = *state.scenario;
    const auto& index = *state.index;
    for (const auto& event : state.events) {
        if (const auto* change = std::get_if<LaneChangeEvent>(&event)) {
            auto& tally = open_[change->vehicleId];
            const auto previousChange = std::exchange(tally.lastChange, change->time);
            const auto previousFrom = std::exchange(tally.lastFrom, change->fromRouteId);
            const auto from = slotOfRoute_.find(change->fromRouteId);
            const bool discretionary = from != slotOfRoute_.end() && !std::isfinite(index.deadEndOfRoute[from->second]);
            const bool previousMandatory = std::exchange(tally.lastMandatory, !discretionary);
            // From a full route, no dead end: a change by choice (D95), counted apart. A repeat
            // within 3 s counts only when this change is one: a stub's chain of mandatory changes
            // is not a back-and-forth. Each repeat is one of three kinds: straight back to the
            // route the previous change left, right after a mandatory change, or onward.
            if (discretionary) {
                ++tally.discretionary;
                if (previousChange && change->time - *previousChange < 3 - 1e-9) {
                    ++tally.quickRepeats;
                    if (change->toRouteId == previousFrom) ++tally.quickBack;
                    else if (previousMandatory) ++tally.quickAfterMandatory;
                    else ++tally.quickOnward;
                }
                continue;
            }
            ++tally.changes;
            const auto was = previous_.find(change->vehicleId);
            if (was == previous_.end() || from == slotOfRoute_.end()) { ++tally.unplaced; continue; }
            tally.before.push_back(index.deadEndOfRoute[from->second] - was->second.second);
            tally.at.push_back(was->second.second);
        } else if (const auto* arrived = std::get_if<ArrivedEvent>(&event)) {
            const auto tally = open_.find(arrived->vehicleId);
            if (tally == open_.end()) continue;
            const auto m = movementOfRoute_.find(arrived->routeId);
            fold(m == movementOfRoute_.end() ? rows_.back() : rows_[m->second], tally->second);
            open_.erase(tally);
        }
    }
    previous_.clear();
    for (const auto& v : state.vehicles) {
        previous_[v.id] = {v.routeIndex, v.distance};
        const auto& behaviour = s.behaviours[index.behaviourOfType[v.typeIndex]];
        if (!waitingAtDeadEnd(index, v.routeIndex, v, behaviour)) {
            if (const auto tally = open_.find(v.id); tally != open_.end()) tally->second.run = 0;
            continue;
        }
        auto& tally = open_[v.id];
        tally.wait += s.timeStep; tally.run += s.timeStep;
        tally.longest = std::max(tally.longest, tally.run);
    }
}
LaneChangeReport LaneChangeAccumulator::report() const {
    LaneChangeReport r;
    r.rows.assign(rows_.begin(), rows_.end() - 1);
    LaneChangeRow unfinished{"unfinished"};
    for (const auto& [id, tally] : open_) fold(unfinished, tally);
    r.rows.push_back(std::move(unfinished));
    r.rows.push_back(rows_.back());
    return r;
}
}
