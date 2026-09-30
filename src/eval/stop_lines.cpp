#include "stop_lines.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
StopLineAccumulator::StopLineAccumulator(const EvaluationSpec& spec) : beginSpeed_(spec.queue.beginSpeed) {}
void StopLineAccumulator::bind(const SimState& state) {
    if (bound_ == state.scenario.get()) return;
    bound_ = state.scenario.get();
    const auto& s = *bound_;
    heads_.clear();
    // The first part of each route on the head's segment, as MovementAccumulator binds a line.
    for (const auto& head : s.signalHeads) {
        Head bound{head.id, std::vector<double>(s.routes.size(), std::nan(""))};
        for (std::size_t r = 0; r < s.routes.size(); ++r)
            for (const auto& part : state.index->parts[r])
                if (part.segmentId == head.segmentId) { bound.atRoute[r] = part.start + head.position; break; }
        heads_.push_back(std::move(bound));
    }
}
void StopLineAccumulator::observe(const SimState& state) {
    bind(state);
    const double dt = state.scenario->timeStep;
    const auto standing = [&](std::size_t h) {
        std::uint64_t n = 0;
        for (const auto& v : state.vehicles) {
            const double at = heads_[h].atRoute[v.routeIndex];
            n += !std::isnan(at) && at > v.distance && v.speed < beginSpeed_;
        }
        return n;
    };
    for (const auto& event : state.events) {
        const auto* signal = std::get_if<SignalEvent>(&event);
        if (!signal) continue;
        for (std::size_t h = 0; h < heads_.size(); ++h) {
            auto& head = heads_[h];
            if (head.id != signal->signalId) continue;
            const bool green = signal->color == SignalColor::green;
            if (head.green && !green) {
                // Whoever is still standing upstream at the green's end was held through it.
                const auto left = standing(h);
                ++head.greenEnds; head.residual += left;
                for (auto& [key, tally] : open_)
                    if (key.second == h) {
                        const auto it = std::find_if(state.vehicles.begin(), state.vehicles.end(),
                                                     [&](const auto& v) { return v.id == key.first; });
                        if (it != state.vehicles.end() && it->speed < beginSpeed_) ++tally.held;
                    }
            }
            if (!head.green && green) { ++head.greens; head.lastCrossing = -1; }
            head.green = green;
        }
    }
    // Who is upstream of which head now; a vehicle that was, and still exists, has crossed.
    std::map<std::pair<std::uint64_t, std::size_t>, Tally> now;
    for (const auto& v : state.vehicles)
        for (std::size_t h = 0; h < heads_.size(); ++h) {
            const double at = heads_[h].atRoute[v.routeIndex];
            if (std::isnan(at)) continue;
            const auto key = std::pair{v.id, h};
            const auto was = open_.find(key);
            if (at > v.distance) {
                auto tally = was == open_.end() ? Tally{} : was->second;
                if (v.speed < beginSpeed_) (heads_[h].green ? tally.green : tally.red) += dt;
                now.emplace(key, tally);
            } else if (was != open_.end()) {
                auto& head = heads_[h];
                ++head.crossed; head.red += was->second.red; head.standGreen += was->second.green;
                head.held += was->second.held; head.heldAny += was->second.held > 0;
                if (head.green) {
                    ++head.discharged;
                    if (head.lastCrossing >= 0) { head.gapSum += state.time - head.lastCrossing; ++head.gaps; }
                    head.lastCrossing = state.time;
                }
            }
        }
    // A vehicle that left a head's route without crossing (arrived, or changed away) is dropped.
    open_ = std::move(now);
}
StopLineReport StopLineAccumulator::report() const {
    StopLineReport r;
    const auto ratio = [](double a, std::uint64_t b) { return b ? a / static_cast<double>(b) : 0.0; };
    for (const auto& h : heads_)
        r.rows.push_back({h.id, h.crossed, h.greens, ratio(h.red, h.crossed), ratio(h.standGreen, h.crossed),
                          ratio(static_cast<double>(h.heldAny), h.crossed), ratio(static_cast<double>(h.held), h.crossed),
                          ratio(static_cast<double>(h.discharged), h.greens), ratio(h.gapSum, h.gaps),
                          ratio(static_cast<double>(h.residual), h.greenEnds)});
    return r;
}
}
