#include "network.hpp"
#include "../../core/validate.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <vector>

namespace trafficsim {
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
    // A route is authored on whole lanes, because that is what an author draws and stores. The
    // runtime travels sections, so the chain is resolved here -- inside buildScenario, which is
    // the one place every caller goes through, including the save-time demand validation.
    for (auto& route : scenario.routes) route.segmentIds = expandRouteSegments(table, route.segmentIds);
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
        std::vector<ConnectorPath> paths;
        try { paths=connectorPaths(network,c); } catch(const std::exception&) { continue; }
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
    for(auto& issue:priorityDefaultsIssues(network,definition.priorityDefaults))
        issues.push_back(std::move(issue));
    if(!issues.empty())throw ValidationError(std::move(issues));
    auto scenario = buildScenario(network, definition);
    assertValidScenario(scenario);
    return scenario;
}
}
