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
    return scenario; // All fields are owned values, independent of the editor model.
}
std::vector<ValidationIssue> connectorRuntimeIssues(const Network& network) {
    std::vector<ValidationIssue> issues;
    const auto table=runtimeSections(network);
    for(std::size_t i=0;i<network.connectors.size();++i) {
        const auto& c=network.connectors[i];
        const auto path="connectors["+std::to_string(i)+"]";
        // A Connector ARRIVING inside a lane body makes two paths feed the same place: the
        // section upstream of the arrival and the Connector itself. That is a merge, and
        // arbitrating it is right-of-way, which the engine does not have yet. Reported against
        // the Connector here so the author gets a row that selects the object, rather than the
        // core's generic segments.<id> row about a merge they did not know they had drawn.
        if(!attachedAtLinkEnd(network,c.to,false))
            issues.push_back({"UNSUPPORTED_ATTACHED_TARGET",path});
        // A Connector LEAVING a lane body is a diverge, which the sectioned runtime traverses.
        // The one case it still cannot is a cut with no room for a section either side of it,
        // since a zero-length segment is not a thing the core accepts.
        else if(std::find(table.unsectionable.begin(),table.unsectionable.end(),c.id)!=
                table.unsectionable.end())
            issues.push_back({"UNSUPPORTED_CONNECTOR_POSITION",path});
    }
    return issues;
}
std::vector<ValidationIssue> connectorShapeIssues(const Network& network) {
    std::vector<ValidationIssue> issues;
    for(std::size_t i=0;i<network.connectors.size();++i) {
        const auto& c=network.connectors[i];
        double width=0,radius=std::numeric_limits<double>::infinity();
        std::vector<ConnectorPath> paths;
        try { paths=connectorPaths(network,c); } catch(const std::exception&) { continue; }
        for(const auto& link:network.links)for(const auto& lane:link.lanes)
            if(lane.id==paths.front().from.laneId)width+=lane.width;
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
    if(!issues.empty())throw ValidationError(std::move(issues));
    auto scenario = buildScenario(network, definition);
    assertValidScenario(scenario);
    return scenario;
}
}
