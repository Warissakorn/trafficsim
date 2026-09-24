#include "network.hpp"
#include "../../core/validate.hpp"
#include <cmath>
#include <map>
#include <algorithm>
#include <limits>
#include <vector>

namespace trafficsim {
namespace {
// A route already given in lane or section ids, rather than the Links and Connectors an author
// names. Compiling an already-compiled Scenario has to be a no-op -- callers do it -- so such a
// route passes through untouched instead of being reported as one no lane can travel.
bool routeAlreadyExpanded(const RuntimeSections& table, const std::vector<std::string>& ids) {
    if (ids.empty()) return false;
    for (const auto& id : ids) {
        const bool known =
            std::any_of(table.sections.begin(), table.sections.end(),
                        [&](const auto& s) { return s.id == id; }) ||
            std::any_of(table.paths.begin(), table.paths.end(),
                        [&](const auto& p) { return p.id == id; });
        if (!known) return false;
    }
    return true;
}
}
Scenario buildScenario(const Network& network, const ScenarioDefinition& definition) {
    Scenario scenario;
    static_cast<ScenarioDefinition&>(scenario) = definition;
    // One pass over the drawing produces the lane sections, the connector paths and where each
    // path arrives. Deriving any of it a second time here is what made an earlier version cubic,
    // and buildScenario runs on every model change.
    const auto table = runtimeSections(network);
    for (const auto& section : table.sections)
        scenario.segments.push_back({section.id, section.end - section.start, section.next});
    for (std::size_t p = 0; p < table.paths.size(); ++p)
        scenario.segments.push_back({table.paths[p].id, polylineLength(table.paths[p].geometry),
                                     {table.pathNext[p]}});
    // A route is authored on LINKS AND CONNECTORS, because that is the object an author places
    // and keeps editing. The runtime travels one lane, so each authored route expands here into
    // one core route per lane the drawing actually carries -- inside buildScenario, which is the
    // one place every caller goes through, including the save-time demand validation.
    //
    // An expansion of exactly one lane keeps the authored id. That is load-bearing: every
    // reference network is single-lane, so the four frozen baselines still name the same routes
    // and replay the same trajectories. Only a genuinely multi-lane route gains "/lane-k".
    std::vector<Route> routes;
    std::vector<VehicleInput> inputs;
    std::map<std::string, std::vector<std::string>> expanded; // authored route id -> core ids
    for (const auto& route : scenario.routes) {
        if (routeAlreadyExpanded(table, route.segmentIds)) {
            routes.push_back(route); expanded[route.id].push_back(route.id); continue;
        }
        const auto chains = routeLaneChains(network, route.segmentIds);
        for (std::size_t k = 0; k < chains.size(); ++k) {
            const auto id = chains.size() == 1 ? route.id : route.id + "/lane-" + std::to_string(k + 1);
            routes.push_back({id, expandRouteSegments(table, chains[k])});
            expanded[route.id].push_back(id);
        }
    }
    for (const auto& input : scenario.inputs) {
        const auto found = expanded.find(input.routeId);
        // A route no lane can travel emits no core route, and its inputs go with it rather than
        // dangling as UNKNOWN_ROUTE. That is not silence: routeRuntimeIssues names the route,
        // the Problems panel shows it and Run refuses, while the EDIT still goes through -- an
        // author must be able to move a Connector without the document rejecting the change.
        const bool authored = std::any_of(definition.routes.begin(), definition.routes.end(),
                                          [&](const auto& r) { return r.id == input.routeId; });
        if (found == expanded.end() || found->second.empty()) {
            if (!authored) inputs.push_back(input);
            continue;
        }
        const auto& lanes = found->second;
        // laneShares are weights in the same order as `lanes`. A stale size -- the network was
        // edited since they were set -- degrades to the equal split rather than landing on the
        // wrong lane (M1.26.1); so does any non-positive weight, which would otherwise divide by
        // a zero or negative sum.
        bool useShares = input.laneShares.size() == lanes.size();
        double weightSum = 0;
        if (useShares)
            for (double w : input.laneShares) { if (!(w > 0)) { useShares = false; break; } weightSum += w; }
        for (std::size_t k = 0; k < lanes.size(); ++k) {
            auto share = input;
            share.id = lanes.size() == 1 ? input.id : input.id + "/lane-" + std::to_string(k + 1);
            share.routeId = lanes[k];
            share.laneShares.clear();
            // The authored volume is the LINK total, divided across the lanes it reaches -- equally
            // by default, or by the authored weights. It is an authoring convenience, not a
            // lane-choice model: the engine has no lane changing, so nothing here claims that this
            // is how traffic really distributes itself.
            share.vehiclesPerHour = useShares ? input.vehiclesPerHour * (input.laneShares[k] / weightSum)
                                              : input.vehiclesPerHour / static_cast<double>(lanes.size());
            inputs.push_back(std::move(share));
        }
    }
    scenario.routes = std::move(routes);
    scenario.inputs = std::move(inputs);
    for (const auto& head : network.signalHeads) scenario.signalHeads.push_back(rebaseHead(table, head));
    // Appended, not assigned: an authored rule keeps its own two numbers, and a derived one is
    // added for each merge the drawing creates. Order follows path order, so it is reproducible.
    for (auto& rule : derivedPriorityRules(table, definition.priorityDefaults))
        scenario.priorityRules.push_back(std::move(rule));
    return scenario; // All fields are owned values, independent of the editor model.
}
std::vector<ValidationIssue> connectorRuntimeIssues(const Network& network) {
    std::vector<ValidationIssue> issues;
    const auto table=runtimeSections(network);
    for(std::size_t i=0;i<network.connectors.size();++i) {
        const auto& c=network.connectors[i];
        const auto path="connectors["+std::to_string(i)+"]";
        // Both directions are runnable now: a Connector leaving a lane body is a diverge, and one
        // arriving on it is a merge that M3.1 arbitrates with a derived priority rule. The one
        // case that still cannot run is a cut with no room for a section either side of it, since
        // a zero-length segment is not a thing the core accepts.
        if(std::find(table.unsectionable.begin(),table.unsectionable.end(),c.id)!=
           table.unsectionable.end())
            issues.push_back({"UNSUPPORTED_CONNECTOR_POSITION",path});
    }
    return issues;
}
std::vector<ValidationIssue> routeRuntimeIssues(const Network& network,
                                                const ScenarioDefinition& definition) {
    std::vector<ValidationIssue> issues;
    const auto table = runtimeSections(network);
    for (std::size_t i = 0; i < definition.routes.size(); ++i) {
        const auto& route = definition.routes[i];
        if (route.segmentIds.empty() || routeAlreadyExpanded(table, route.segmentIds) ||
            !routeLaneChains(network, route.segmentIds).empty()) continue;
        // The objects named do not join up for a single lane, so nothing can travel this route.
        // UNSUPPORTED_ prefix on purpose: authoring tolerates it (D18b), Run does not.
        issues.push_back({"UNSUPPORTED_ROUTE_TOPOLOGY", "routes[" + std::to_string(i) + "]"});
    }
    return issues;
}
std::vector<ValidationIssue> routeAmbiguityIssues(const Network& network,
                                                  const ScenarioDefinition& definition) {
    std::vector<ValidationIssue> issues;
    const auto table = runtimeSections(network);
    for (std::size_t i = 0; i < definition.routes.size(); ++i) {
        const auto& route = definition.routes[i];
        if (route.segmentIds.empty() || routeAlreadyExpanded(table, route.segmentIds)) continue;
        std::vector<std::string> dropped;
        // Only a route that still runs: one that does not is UNSUPPORTED_ROUTE_TOPOLOGY already,
        // and a second row about the same route would say less than the first.
        if (routeLaneChains(network, route.segmentIds, &dropped).empty() || dropped.empty()) continue;
        issues.push_back({"AMBIGUOUS_ROUTE_STEP", "routes[" + std::to_string(i) + "]"});
    }
    return issues;
}
std::vector<ValidationIssue> priorityDefaultsIssues(const Network& network,
                                                    const PriorityDefaults& defaults) {
    // A merge the drawing creates is arbitrated by a DERIVED rule, and a rule with a zero gap
    // time and headway is a merge nobody gives way at. So a network that needs one cannot run
    // without the numbers from data/priority-rules/. One helper, two callers: compileScenario
    // throws on it and runtimeDiagnostics reports it, so Run and the panel cannot disagree.
    if(defaults.gapTime>0 && defaults.headway>0)return {};
    std::vector<ValidationIssue> issues;
    const auto table=runtimeSections(network);
    for(std::size_t i=0;i<network.connectors.size();++i)
        if(!attachedAtLinkEnd(network,network.connectors[i].to,false) &&
           std::find(table.unsectionable.begin(),table.unsectionable.end(),network.connectors[i].id)==
           table.unsectionable.end())
            issues.push_back({"EDIT_NO_PRIORITY_DEFAULTS","connectors["+std::to_string(i)+"]"});
    return issues;
}
std::vector<ValidationIssue> connectorShapeIssues(const Network& network) {
    std::vector<ValidationIssue> issues;
    for(std::size_t i=0;i<network.connectors.size();++i) {
        const auto& c=network.connectors[i];
        double width=0,radius=std::numeric_limits<double>::infinity();
        if(polylineLength(c.geometry)<5)
            issues.push_back({"WARN_SHORT_CONNECTOR","connectors["+std::to_string(i)+"]"});
        std::vector<ConnectorPath> paths;
        try { paths=connectorPaths(network,c); } catch(const std::exception&) { continue; }
        try {
            const auto fit=connectorMouthFit(network,c);
            if(fit.source.squareFallback || fit.target.squareFallback ||
               fit.source.residual>.01 || fit.target.residual>.01)
                issues.push_back({"WARN_CONNECTOR_ALIGNMENT","connectors["+std::to_string(i)+"]"});
        } catch(const std::exception&) { continue; }
        // The same widths connectorBoundaries draws from, so an authored width is measured
        // against rather than silently ignored here (hard rule 3).
        try { for(const double w:connectorLaneWidths(network,c).source)width+=w; }
        catch(const std::exception&) { continue; }
        for(const auto& path:paths)for(std::size_t j=1;j+1<path.geometry.size();++j) {
            const auto a=path.geometry[j-1],b=path.geometry[j],d=path.geometry[j+1];
            // Radius of the circle through three consecutive points: the side lengths over
            // twice the triangle area. Collinear points give an infinite radius, as they should.
            const double ab=std::hypot(b.x-a.x,b.y-a.y),bd=std::hypot(d.x-b.x,d.y-b.y),ad=std::hypot(d.x-a.x,d.y-a.y);
            const double area=std::abs((b.x-a.x)*(d.y-a.y)-(b.y-a.y)*(d.x-a.x))/2;
            if(area>1e-12)radius=std::min(radius,ab*bd*ad/(4*area));
        }
        if(width>0 && radius<width/2)issues.push_back({"TIGHT_CONNECTOR_RADIUS","connectors["+std::to_string(i)+"]"});
    }
    return issues;
}
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition) {
    // Order is load-bearing: the network pass reports UNKNOWN_LANE before laneGeometry can throw.
    assertValidNetwork(network);
    auto issues=connectorRuntimeIssues(network);
    for(auto& issue:routeRuntimeIssues(network,definition))
        issues.push_back(std::move(issue));
    for(auto& issue:priorityDefaultsIssues(network,definition.priorityDefaults))
        issues.push_back(std::move(issue));
    if(!issues.empty())throw ValidationError(std::move(issues));
    auto scenario = buildScenario(network, definition);
    assertValidScenario(scenario);
    return scenario;
}
}
