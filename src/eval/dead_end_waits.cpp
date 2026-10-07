#include "dead_end_waits.hpp"
#include <algorithm>
#include <limits>

namespace trafficsim {
namespace {
void fold(WaitCauseRow& row, const auto& tally) {
    for (std::size_t c = 0; c < kWaitCauses; ++c) { row.waits[c] += tally.waits[c]; row.seconds[c] += tally.seconds[c]; }
}
}
const char* waitCauseName(WaitCause cause) {
    switch (cause) {
    case WaitCause::noMovingApproach: return "noMovingApproach";
    case WaitCause::outsideSpan: return "outsideSpan";
    case WaitCause::targetStanding: return "targetStanding";
    case WaitCause::movingStream: return "movingStream";
    }
    return "";
}
WaitCause waitCause(const SimState& state, std::size_t v, bool unhelped) {
    if (unhelped) return WaitCause::noMovingApproach;
    const auto& s = *state.scenario;
    const auto& index = *state.index;
    const auto& vehicle = state.vehicles[v];
    const double length = s.vehicleTypes[vehicle.typeIndex].length;
    const auto* change = laneChangeTargetOf(index, vehicle.routeIndex, vehicle.distance, vehicle.distance - length);
    if (!change) return WaitCause::outsideSpan;
    const double at = mappedOnto(*change, vehicle.distance), atRear = at - length;
    const auto& target = index.parts[change->target];
    bool standing = false;
    double behindFront = -std::numeric_limits<double>::infinity();
    bool behindStanding = false;
    for (std::size_t w = 0; w < state.vehicles.size(); ++w) {
        if (w == v) continue;
        const auto& other = state.vehicles[w];
        const auto& parts = index.parts[other.routeIndex];
        const auto part = std::find_if(parts.begin(), parts.end(), [&](const RoutePart& p) {
            return other.distance >= p.start && other.distance <= p.start + p.length; });
        if (part == parts.end()) continue;
        const auto on = std::find_if(target.begin(), target.end(),
                                     [&](const RoutePart& p) { return p.segmentIndex == part->segmentIndex; });
        if (on == target.end()) continue;
        const double front = on->start + (other.distance - part->start);
        const double rear = front - s.vehicleTypes[other.typeIndex].length;
        const bool still = other.speed < kWaitingSpeed;
        if (front > atRear && rear < at) standing |= still;            // alongside
        else if (front <= atRear && front > behindFront) { behindFront = front; behindStanding = still; }
    }
    return standing || behindStanding ? WaitCause::targetStanding : WaitCause::movingStream;
}
DeadEndWaitAccumulator::DeadEndWaitAccumulator(const EvaluationSpec& spec) : movementOfRoute_(spec.movementOfRoute) {
    for (const auto& name : spec.movementNames) rows_.push_back({name});
    rows_.push_back({"unassigned"});
}
void DeadEndWaitAccumulator::observe(const SimState& state) {
    const auto& s = *state.scenario;
    const auto& index = *state.index;
    for (const auto& event : state.events) {
        const auto* arrived = std::get_if<ArrivedEvent>(&event);
        if (!arrived) continue;
        const auto tally = open_.find(arrived->vehicleId);
        if (tally == open_.end()) continue;
        const auto m = movementOfRoute_.find(arrived->routeId);
        fold(m == movementOfRoute_.end() ? rows_.back() : rows_[m->second], tally->second);
        open_.erase(tally);
    }
    for (std::size_t v = 0; v < state.vehicles.size(); ++v) {
        const auto& vehicle = state.vehicles[v];
        if (index.remainingOfRoute[vehicle.routeIndex] == 0) {
            if (const auto tally = open_.find(vehicle.id); tally != open_.end()) tally->second.waiting.reset();
            continue;
        }
        auto& tally = open_[vehicle.id];
        if (tally.route != vehicle.routeIndex) { tally.route = vehicle.routeIndex; tally.unhelped = true; tally.waiting.reset(); }
        const auto& type = s.vehicleTypes[vehicle.typeIndex];
        const auto& behaviour = s.behaviours[effectiveBehaviour(index, vehicle)]; // as the engine selects it
        if (!waitingAtDeadEnd(index, vehicle.routeIndex, vehicle, behaviour)) {
            tally.waiting.reset();
            if (vehicle.speed >= kWaitingSpeed && deadEndGoverns(index, vehicle.routeIndex, vehicle, type, behaviour) &&
                laneChangeTargetOf(index, vehicle.routeIndex, vehicle.distance, vehicle.distance - type.length))
                tally.unhelped = false;
            continue;
        }
        if (!tally.waiting) {
            tally.waiting = waitCause(state, v, tally.unhelped);
            ++tally.waits[static_cast<std::size_t>(*tally.waiting)];
        }
        tally.seconds[static_cast<std::size_t>(*tally.waiting)] += s.timeStep;
    }
}
WaitCauseReport DeadEndWaitAccumulator::report() const {
    WaitCauseReport r;
    r.rows.assign(rows_.begin(), rows_.end() - 1);
    WaitCauseRow unfinished{"unfinished"};
    for (const auto& [id, tally] : open_) fold(unfinished, tally);
    r.rows.push_back(std::move(unfinished));
    r.rows.push_back(rows_.back());
    return r;
}
}
