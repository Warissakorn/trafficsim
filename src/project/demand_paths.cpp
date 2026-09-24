#include "demand_paths.hpp"
#include <algorithm>
#include <map>

namespace trafficsim {
namespace {
bool hasLink(const Network& n, const std::string& id) {
    return std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
}
const RoutingDecision* decisionById(const AuthoringDefinition& d, const std::string& id) {
    for (const auto& x : d.routingDecisions) if (x.id == id) return &x;
    return nullptr;
}
// The Link an authored input enters on when it has no route: its own, or its placed decision's.
std::string routelessLink(const AuthoringDefinition& d, const VehicleInput& input) {
    if (!input.linkId.empty()) return input.linkId;
    if (const auto* x = decisionById(d, input.routingDecisionId); x && !x->linkId.empty()) return x->linkId;
    return {};
}
}
namespace {
// Every object chain from `from` to `target` of the shortest length, in continuation order.
// routeChainTo refuses a tie, because a drawn route must mean one thing; a decision's destination
// may be reached by several, since each lane of the Link follows whichever it can drive.
std::vector<std::vector<std::string>> shortestChains(const Network& network, const std::string& from,
                                                     const std::string& target) {
    constexpr std::size_t kMaxDepth = 20, kMaxLevel = 4096;
    std::vector<std::vector<std::string>> level{{from}};
    for (std::size_t depth = 0; depth < kMaxDepth && !level.empty(); ++depth) {
        std::vector<std::vector<std::string>> next, found;
        for (const auto& chain : level)
            for (const auto& step : routeContinuations(network, chain)) {
                auto extended = chain; extended.push_back(step);
                (step == target ? found : next).push_back(std::move(extended));
            }
        if (!found.empty()) return found;
        if (next.size() > kMaxLevel) return {};
        level = std::move(next);
    }
    return {};
}
}
std::string routelessRouteId(const std::string& linkId, std::size_t k, std::size_t n) {
    return n == 1 ? "link:" + linkId : "link:" + linkId + "/path-" + std::to_string(k + 1);
}
std::vector<PlacedDecision> placedDecisions(const Network& network, const AuthoringDefinition& d,
                                            std::vector<ValidationIssue>* issues, std::optional<double> time) {
    std::vector<PlacedDecision> result;
    const auto report = [&](std::string code, std::string path) { if (issues) issues->push_back({std::move(code), std::move(path)}); };
    for (std::size_t k = 0; k < d.routingDecisions.size(); ++k) {
        const auto& decision = d.routingDecisions[k];
        if (decision.linkId.empty()) continue;
        const auto path = "routingDecisions[" + std::to_string(k) + "]";
        if (!hasLink(network, decision.linkId)) { report("UNKNOWN_LINK", path + ".linkId"); continue; }
        PlacedDecision placed{decision.id, decision.linkId, path, {}};
        for (std::size_t j = 0; j < decision.routes.size(); ++j) {
            const auto& entry = decision.routes[j];
            const auto at = path + ".routes[" + std::to_string(j) + "]";
            std::vector<std::vector<std::string>> chains;
            if (!entry.destinationLinkId.empty()) {
                if (!hasLink(network, entry.destinationLinkId)) { report("UNKNOWN_LINK", at); continue; }
                chains = shortestChains(network, decision.linkId, entry.destinationLinkId);
            } else {
                const auto route = std::find_if(d.routes.begin(), d.routes.end(), [&](const auto& r) { return r.id == entry.routeId; });
                if (route == d.routes.end()) continue; // routingDecisionIssues names it
                chains = {route->segmentIds};
            }
            std::erase_if(chains, [&](const auto& c) { return routeLaneChains(network, c).empty(); });
            if (chains.empty()) { report("ROUTING_DECISION_UNREACHABLE", at); continue; }
            const double weight = time ? decisionFlowAt(decision, entry, *time) : entry.relativeFlow;
            if (weight > 0) placed.destinations.push_back({std::move(chains), weight});
        }
        result.push_back(std::move(placed));
    }
    return result;
}
namespace {
// M2.1.2: the times at which the placed decisions' flows change, and one representative time per
// slice between them (its midpoint; the first and last slices are open-ended). No decision with
// intervals: one slice and no time, which is exactly the M2.1.1 walk.
struct Slices { std::vector<double> points; std::vector<std::optional<double>> times; };
Slices slices(const AuthoringDefinition& d) {
    std::vector<const RoutingDecision*> timed;
    for (const auto& x : d.routingDecisions) if (!x.linkId.empty() && !x.intervals.empty()) timed.push_back(&x);
    Slices result{decisionBreakpoints(timed), {}};
    if (result.points.empty()) { result.times.push_back(std::nullopt); return result; }
    result.times.push_back(result.points.front() - 1);
    for (std::size_t k = 0; k + 1 < result.points.size(); ++k) result.times.push_back((result.points[k] + result.points[k + 1]) / 2);
    result.times.push_back(result.points.back() + 1);
    return result;
}
std::size_t sliceOf(const Slices& s, double t) {
    return static_cast<std::size_t>(std::upper_bound(s.points.begin(), s.points.end(), t) - s.points.begin());
}
}
std::vector<VolumeInterval> cutPeriods(const VehicleInput& input, const std::vector<double>& breakpoints) {
    std::vector<VolumeInterval> pieces;
    for (const auto& period : inputPeriods(input)) {
        double from = period.startTime;
        for (double b : breakpoints)
            if (b > from && b < period.endTime) { pieces.push_back({from, b, period.vehiclesPerHour}); from = b; }
        if (period.endTime > from) pieces.push_back({from, period.endTime, period.vehiclesPerHour});
    }
    return pieces;
}
RoutelessIssues routelessIssues(const Network& network, const AuthoringDefinition& d) {
    RoutelessIssues result;
    const auto cut = slices(d);
    // Reachability does not depend on the flows, so the static pass reports it once.
    std::vector<std::vector<PlacedDecision>> placed{placedDecisions(network, d, &result.blocking)};
    if (cut.times.front()) for (const auto& t : cut.times) placed.push_back(placedDecisions(network, d, nullptr, t));
    std::map<std::string, bool> walked;
    for (std::size_t i = 0; i < d.inputs.size(); ++i) {
        const auto link = routelessLink(d, d.inputs[i]);
        if (link.empty()) continue;
        const auto field = "inputs[" + std::to_string(i) + "]." + (d.inputs[i].linkId.empty() ? "routingDecisionId" : "linkId");
        for (const auto& decisions : placed) {
            const auto walk = routelessChains(network, link, decisions);
            for (const auto& issue : walk.issues)
                if (std::find(result.blocking.begin(), result.blocking.end(), ValidationIssue{issue.code, field}) == result.blocking.end())
                    result.blocking.push_back({issue.code, field});
            for (const auto& issue : walk.advisories)
                if (std::find(result.advisory.begin(), result.advisory.end(), issue) == result.advisory.end())
                    result.advisory.push_back(issue);
        }
    }
    return result;
}
ScenarioDefinition expandRouteless(const Network& network, const AuthoringDefinition& authored,
                                   ScenarioDefinition resolved) {
    if (std::none_of(resolved.inputs.begin(), resolved.inputs.end(), [](const auto& i) { return !i.linkId.empty(); }))
        return resolved; // nothing to do, and nothing to compute: every existing project
    const auto cut = slices(authored);
    std::vector<std::vector<PlacedDecision>> placed;
    for (const auto& t : cut.times) placed.push_back(placedDecisions(network, authored, nullptr, t));
    const auto table = runtimeSections(network);
    // Per Link: the union of every slice's paths, in order of first appearance, so one runtime
    // route serves a path in every slice; and each slice's share of each path.
    struct Walks { std::vector<std::vector<std::string>> paths; std::vector<std::size_t> lanes;
                   std::vector<std::vector<double>> share; bool byDestination{}; bool failed{}; };
    std::map<std::string, Walks> walks;
    std::vector<VehicleInput> inputs;
    for (const auto& input : resolved.inputs) {
        if (input.linkId.empty()) { inputs.push_back(input); continue; }
        auto [it, fresh] = walks.try_emplace(input.linkId);
        auto& w = it->second;
        if (fresh) {
            for (std::size_t s = 0; s < placed.size(); ++s) {
                const auto walk = routelessChains(network, input.linkId, placed[s]);
                if (!walk.issues.empty()) { w.failed = true; break; }
                w.byDestination = w.byDestination || walk.byDestination;
                for (const auto& c : walk.chains) {
                    auto at = std::find(w.paths.begin(), w.paths.end(), c.laneChain);
                    if (at == w.paths.end()) {
                        w.paths.push_back(c.laneChain); w.lanes.push_back(c.lane);
                        w.share.emplace_back(placed.size(), 0.0);
                        at = w.paths.end() - 1;
                    }
                    w.share[static_cast<std::size_t>(at - w.paths.begin())][s] += c.share;
                }
            }
            if (!w.failed)
                for (std::size_t k = 0; k < w.paths.size(); ++k)
                    resolved.routes.push_back({routelessRouteId(input.linkId, k, w.paths.size()),
                                               expandRouteSegments(table, w.paths[k])});
        }
        if (w.failed) continue; // routelessIssues names it
        // The Link total divided across its lanes, as for a route (M1.26, M1.26.1).
        std::size_t lanes = 0;
        for (const auto& l : network.links) if (l.id == input.linkId) lanes = l.lanes.size();
        bool weighted = input.laneShares.size() == lanes;
        double sum = 0;
        if (weighted) for (double v : input.laneShares) { if (!(v > 0)) { weighted = false; break; } sum += v; }
        const auto pieces = placed.size() > 1 ? cutPeriods(input, cut.points) : std::vector<VolumeInterval>{};
        for (std::size_t k = 0; k < w.paths.size(); ++k) {
            const double lane = w.byDestination ? 1.0
                : weighted ? input.laneShares[w.lanes[k]] / sum : 1.0 / static_cast<double>(lanes);
            auto part = input;
            part.linkId.clear(); part.laneShares.clear();
            part.routeId = routelessRouteId(input.linkId, k, w.paths.size());
            if (w.paths.size() > 1) part.id = input.id + "/path-" + std::to_string(k + 1);
            if (placed.size() == 1) {
                const double fraction = lane * w.share[k][0];
                if (!(fraction > 0)) continue;
                part.vehiclesPerHour *= fraction;
                for (auto& period : part.intervals) period.vehiclesPerHour *= fraction;
            } else { // M2.1.2: each piece of the input at its slice's share of this path
                part.intervals.clear();
                for (const auto& piece : pieces) {
                    const double fraction = lane * w.share[k][sliceOf(cut, (piece.startTime + piece.endTime) / 2)];
                    if (fraction > 0) part.intervals.push_back({piece.startTime, piece.endTime, piece.vehiclesPerHour * fraction});
                }
                if (part.intervals.empty()) continue;
                deriveInputTotals(part);
            }
            inputs.push_back(std::move(part));
        }
    }
    resolved.inputs = std::move(inputs);
    return resolved;
}
}
