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
                                            std::vector<ValidationIssue>* issues) {
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
            if (entry.relativeFlow > 0) placed.destinations.push_back({std::move(chains), entry.relativeFlow});
        }
        result.push_back(std::move(placed));
    }
    return result;
}
RoutelessIssues routelessIssues(const Network& network, const AuthoringDefinition& d) {
    RoutelessIssues result;
    const auto placed = placedDecisions(network, d, &result.blocking);
    std::map<std::string, RoutelessResult> walks;
    for (std::size_t i = 0; i < d.inputs.size(); ++i) {
        const auto link = routelessLink(d, d.inputs[i]);
        if (link.empty()) continue;
        auto [it, fresh] = walks.try_emplace(link);
        if (fresh) it->second = routelessChains(network, link, placed);
        const auto field = "inputs[" + std::to_string(i) + "]." + (d.inputs[i].linkId.empty() ? "routingDecisionId" : "linkId");
        for (const auto& issue : it->second.issues) result.blocking.push_back({issue.code, field});
        for (const auto& issue : it->second.advisories)
            if (std::find(result.advisory.begin(), result.advisory.end(), issue) == result.advisory.end())
                result.advisory.push_back(issue);
    }
    return result;
}
ScenarioDefinition expandRouteless(const Network& network, const AuthoringDefinition& authored,
                                   ScenarioDefinition resolved) {
    if (std::none_of(resolved.inputs.begin(), resolved.inputs.end(), [](const auto& i) { return !i.linkId.empty(); }))
        return resolved; // nothing to do, and nothing to compute: every existing project
    const auto placed = placedDecisions(network, authored);
    const auto table = runtimeSections(network);
    std::map<std::string, RoutelessResult> walks;
    std::vector<VehicleInput> inputs;
    for (const auto& input : resolved.inputs) {
        if (input.linkId.empty()) { inputs.push_back(input); continue; }
        auto [it, fresh] = walks.try_emplace(input.linkId);
        const auto& walk = it->second;
        if (fresh) {
            it->second = routelessChains(network, input.linkId, placed);
            if (it->second.issues.empty())
                for (std::size_t k = 0; k < walk.chains.size(); ++k)
                    resolved.routes.push_back({routelessRouteId(input.linkId, k, walk.chains.size()),
                                               expandRouteSegments(table, walk.chains[k].laneChain)});
        }
        if (!walk.issues.empty()) continue; // routelessIssues names it
        // The Link total divided across its lanes, as for a route (M1.26, M1.26.1).
        std::size_t lanes = 0;
        for (const auto& l : network.links) if (l.id == input.linkId) lanes = l.lanes.size();
        bool weighted = input.laneShares.size() == lanes;
        double sum = 0;
        if (weighted) for (double w : input.laneShares) { if (!(w > 0)) { weighted = false; break; } sum += w; }
        for (std::size_t k = 0; k < walk.chains.size(); ++k) {
            const auto& chain = walk.chains[k];
            const double lane = walk.byDestination ? 1.0
                : weighted ? input.laneShares[chain.lane] / sum : 1.0 / static_cast<double>(lanes);
            const double fraction = lane * chain.share;
            if (!(fraction > 0)) continue;
            auto part = input;
            part.linkId.clear(); part.laneShares.clear();
            part.routeId = routelessRouteId(input.linkId, k, walk.chains.size());
            if (walk.chains.size() > 1) part.id = input.id + "/path-" + std::to_string(k + 1);
            part.vehiclesPerHour *= fraction;
            for (auto& period : part.intervals) period.vehiclesPerHour *= fraction;
            inputs.push_back(std::move(part));
        }
    }
    resolved.inputs = std::move(inputs);
    return resolved;
}
}
