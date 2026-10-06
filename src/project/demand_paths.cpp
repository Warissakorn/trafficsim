#include "demand_paths.hpp"
#include "demand_time_types.hpp"
#include "station_routing.hpp"
#include "../model/demand/signal_control.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

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
InputLanePolicy inputLanePolicy(const Network& n,const AuthoringDefinition& d,const VehicleInput& input) {
    std::string link=input.linkId;
    if(const auto* decision=decisionById(d,input.routingDecisionId)) {
        if(!decision->linkId.empty())link=decision->linkId;
        else if(decision->routes.size()>1)return {0,false};
        else if(!decision->routes.empty()) {
            auto routed=input;routed.routingDecisionId.clear();routed.routeId=decision->routes.front().routeId;
            return inputLanePolicy(n,d,routed);
        }
    }
    if(!link.empty())for(const auto& l:n.links)if(l.id==link) {
        const bool decided=std::any_of(d.routingDecisions.begin(),d.routingDecisions.end(),[&](const auto& x){return x.linkId==link && !x.position;});
        return {l.lanes.size(),!decided};
    }
    for(const auto& route:d.routes)if(route.id==input.routeId)
        return {routeLaneShareCount(n,route.segmentIds),true};
    return {};
}
std::vector<ValidationIssue> demandAdvisories(const Network& n,const AuthoringDefinition& d) {
    std::vector<ValidationIssue> result;
    for(std::size_t k=0;k<d.inputs.size();++k) {
        const auto& input=d.inputs[k];if(input.laneShares.empty())continue;
        const auto policy=inputLanePolicy(n,d,input);
        const auto path="inputs["+std::to_string(k)+"].laneShares";
        if(!policy.acceptsShares)result.push_back({"DEMAND_LANE_SHARES_IGNORED",path});
        else if(policy.lanes && policy.lanes!=input.laneShares.size())result.push_back({"DEMAND_LANE_SHARES_STALE",path});
    }
    return result;
}
std::string routelessRouteId(const std::string& linkId, std::size_t k, std::size_t n) {
    return n == 1 ? "link:" + linkId : "link:" + linkId + "/path-" + std::to_string(k + 1);
}
std::vector<PlacedDecision> placedDecisions(const Network& network, const AuthoringDefinition& d,
                                            std::vector<ValidationIssue>* issues, std::optional<double> time,const std::string& type) {
    std::vector<PlacedDecision> result;
    const auto report = [&](std::string code, std::string path) { if (issues) issues->push_back({std::move(code), std::move(path)}); };
    for (std::size_t k = 0; k < d.routingDecisions.size(); ++k) {
        const auto& decision = d.routingDecisions[k];
        if (decision.linkId.empty()) continue;
        const auto path = "routingDecisions[" + std::to_string(k) + "]";
        if (!hasLink(network, decision.linkId)) { report("UNKNOWN_LINK", path + ".linkId"); continue; }
        if(decision.position)for(const auto& link:network.links)if(link.id==decision.linkId)
            if(!std::isfinite(*decision.position) || *decision.position<0 || *decision.position>=polylineLength(link.geometry))
                report("INVALID_POSITION",path+".position");
        PlacedDecision placed{decision.id, decision.linkId, path, {},decision.position};
        for (std::size_t j = 0; j < decision.routes.size(); ++j) {
            const auto& entry = decision.routes[j];
            const auto at = path + ".routes[" + std::to_string(j) + "]";
            std::vector<std::vector<std::string>> chains;
            if (!entry.destinationLinkId.empty()) {
                if (!hasLink(network, entry.destinationLinkId)) { report("UNKNOWN_LINK", at); continue; }
                chains = routeShortestChains(network, decision.linkId, entry.destinationLinkId);
            } else {
                const auto route = std::find_if(d.routes.begin(), d.routes.end(), [&](const auto& r) { return r.id == entry.routeId; });
                if (route == d.routes.end()) continue; // routingDecisionIssues names it
                chains = {route->segmentIds};
            }
            std::erase_if(chains, [&](const auto& c) { return routeLaneChains(network, c).empty(); });
            if (chains.empty()) { report("ROUTING_DECISION_UNREACHABLE", at); continue; }
            const double weight = decision.position?1.:typeDecisionFlowAt(decision,j,time.value_or(-1),type);
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
    for (const auto& x : d.routingDecisions) if (!x.linkId.empty() && !x.position && !x.intervals.empty()) timed.push_back(&x);
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
    std::set<std::string> typeIds;
    for(const auto& x:d.routingDecisions)for(const auto& rule:x.typeRules)typeIds.insert(rule.vehicleTypeId);
    for(const auto& type:typeIds)for(const auto& t:cut.times)placed.push_back(placedDecisions(network,d,nullptr,t,type));
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
    // M2.7b: every signal group becomes an ordinary core program here, the one compile step
    // every Run, diagnostic and validation already goes through, so core never sees a controller.
    for (auto& program : signalGroupPrograms(authored.signalControllers)) resolved.signalPrograms.push_back(std::move(program));
    if (std::none_of(resolved.inputs.begin(), resolved.inputs.end(), [](const auto& i) { return !i.linkId.empty(); }))
        return resolved; // nothing to do, and nothing to compute: every existing project
    const auto cut = slices(authored);
    const bool typed=hasTypeRouting(authored);
    const auto table = runtimeSections(network);
    // Per Link: the union of every slice's paths, in order of first appearance, so one runtime
    // route serves a path in every slice; and each slice's share of each path.
    // M3.2.8b: `stub` and `family` per path, so an entry decision's destination gets its lateral
    // spans; a stub is a path of its own family only, never merged with a full one.
    // D93: a path can belong to several families (an entry decision's, then a downstream one's).
    struct Walks { std::vector<std::vector<std::string>> paths; std::vector<std::size_t> lanes;
                   std::vector<std::vector<double>> share; std::vector<bool> stub; std::vector<std::vector<FamilyTag>> families;
                   bool byDestination{}; bool failed{}; };
    std::map<std::pair<std::string,std::string>, Walks> walks;
    std::map<std::string,std::vector<std::vector<PlacedDecision>>> placements;
    std::vector<VehicleInput> inputs;
    for (const auto& input : resolved.inputs) {
        if (input.linkId.empty()) { inputs.push_back(input); continue; }
        const auto type=typed?input.vehicleTypeId:std::string{};
        auto [context, newType]=placements.try_emplace(type);
        if(newType)for(const auto& t:cut.times)context->second.push_back(placedDecisions(network,authored,nullptr,t,type));
        const auto& placed=context->second;
        const auto routeId=[&](std::size_t k,std::size_t n){
            const auto base=routelessRouteId(input.linkId,k,n);
            return typed?base+"/type-"+std::to_string(type.size())+":"+type:base;
        };
        auto [it, fresh] = walks.try_emplace(std::pair{input.linkId,type});
        auto& w = it->second;
        if (fresh) {
            for (std::size_t s = 0; s < placed.size(); ++s) {
                const auto walk = routelessChains(network, input.linkId, placed[s]);
                if (!walk.issues.empty()) { w.failed = true; break; }
                w.byDestination = w.byDestination || walk.byDestination;
                for (const auto& c : walk.chains) {
                    std::size_t at = 0;
                    while (at < w.paths.size() && !(w.paths[at] == c.laneChain && w.stub[at] == c.stub &&
                                                     (!c.stub || w.families[at] == c.families))) ++at;
                    if (at == w.paths.size()) {
                        w.paths.push_back(c.laneChain); w.lanes.push_back(c.lane);
                        w.share.emplace_back(placed.size(), 0.0);
                        w.stub.push_back(c.stub); w.families.push_back(c.families);
                    } else // a merged full path is a full member of every family either belongs to
                        for (const auto& tag : c.families)
                            if (std::find(w.families[at].begin(), w.families[at].end(), tag) == w.families[at].end())
                                w.families[at].push_back(tag);
                    w.share[at][s] += c.share;
                }
            }
            if (!w.failed) {
                std::vector<StationRoute> stationRoutes;
                std::map<std::string, std::vector<FamilyRoute>> families;
                std::vector<DiscretionaryRoute> full; // D95: every full path, with all its families
                for (std::size_t k = 0; k < w.paths.size(); ++k) {
                    Route route{routeId(k,w.paths.size()), expandRouteSegments(table, w.paths[k])};
                    if (!w.stub[k]) full.push_back({route.id, route.segmentIds, {}, {}});
                    for (std::size_t f = 0; f < w.families[k].size(); ++f) {
                        const auto& tag = w.families[k][f];
                        const bool stub = w.stub[k] && f + 1 == w.families[k].size();
                        const auto after = tag.linkId == input.linkId ? std::string{} : tag.linkId;
                        families[tag.name].push_back({route.id, route.segmentIds, stub, after});
                        if (!w.stub[k]) {
                            full.back().names.push_back(tag.name);
                            if (!after.empty()) full.back().after.push_back(after);
                        }
                    }
                    stationRoutes.push_back({route,w.families[k]});
                    resolved.routes.push_back(std::move(route));
                }
                for (const auto& [name, members] : families)
                    appendLaneChanges(network, table, members, resolved.laneChanges, resolved.routeDeadEnds);
                appendDiscretionaryLaneChanges(network, table, full, resolved.laneChanges);
                appendStationRouting(network,authored,type,stationRoutes,resolved);
            }
        }
        if (w.failed) continue; // routelessIssues names it
        // The Link total divided across its lanes, as for a route (M1.26, M1.26.1).
        std::size_t lanes = 0;
        for (const auto& l : network.links) if (l.id == input.linkId) lanes = l.lanes.size();
        // Weights are per Link lane here, so every lane is a "chain" (laneSplit): a path takes
        // the fraction of the lane it starts on.
        std::vector<std::size_t> everyLane(lanes);
        for (std::size_t k = 0; k < lanes; ++k) everyLane[k] = k;
        const auto split = laneSplit(everyLane, lanes, input.laneShares);
        const auto pieces = placed.size() > 1 ? cutPeriods(input, cut.points) : std::vector<VolumeInterval>{};
        for (std::size_t k = 0; k < w.paths.size(); ++k) {
            const bool positionedEntry=std::any_of(authored.routingDecisions.begin(),authored.routingDecisions.end(),[&](const auto& x){return x.linkId==input.linkId && x.position;});
            const double lane = w.byDestination && !positionedEntry ? 1.0
                : w.lanes[k] < split.fraction.size() ? split.fraction[w.lanes[k]] : 1.0 / static_cast<double>(lanes);
            auto part = input;
            part.linkId.clear(); part.laneShares.clear();
            part.routeId = routeId(k,w.paths.size());
            if (w.paths.size() > 1) part.id = input.id + "/path-" + std::to_string(k + 1);
            if (placed.size() == 1) {
                const double fraction = lane * w.share[k][0]*(positionedEntry?static_cast<double>(lanes):1.);
                if (!(fraction > 0)) continue;
                part.vehiclesPerHour *= fraction;
                for (auto& period : part.intervals) period.vehiclesPerHour *= fraction;
            } else { // M2.1.2: each piece of the input at its slice's share of this path
                part.intervals.clear();
                for (const auto& piece : pieces) {
                    const double fraction = lane * w.share[k][sliceOf(cut, (piece.startTime + piece.endTime) / 2)]*(positionedEntry?static_cast<double>(lanes):1.);
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
