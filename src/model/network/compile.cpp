#include "network.hpp"
#include "../../core/validate.hpp"
#include <cmath>
#include <limits>
#include <map>
#include <vector>

namespace trafficsim {
Scenario buildScenario(const Network& network, const ScenarioDefinition& definition) {
    Scenario scenario;
    static_cast<ScenarioDefinition&>(scenario) = definition;
    // Connector paths depend only on the network, so derive them once. Resolving them per
    // lane per connector made this cubic, and buildScenario runs on every model change.
    std::vector<ConnectorPath> paths;
    for (const auto& connector : network.connectors)
        for (auto& path : connectorPaths(network, connector)) paths.push_back(std::move(path));
    std::map<std::string, std::vector<const ConnectorPath*>> outgoing;
    for (const auto& path : paths) outgoing[path.from.laneId].push_back(&path);
    for (const auto& link : network.links) for (const auto& lane : link.lanes) {
        Segment segment{lane.id, polylineLength(laneGeometry(link, lane.id, network.drivingSide)), {}};
        if (const auto it = outgoing.find(lane.id); it != outgoing.end())
            for (const auto* path : it->second) segment.next.push_back(path->id);
        scenario.segments.push_back(std::move(segment));
    }
    for (const auto& path : paths)
        scenario.segments.push_back({path.id, polylineLength(path.geometry), {path.to.laneId}});
    for (const auto& head : network.signalHeads)
        scenario.signalHeads.push_back({head.id, signalSegment(head), head.position, head.programId});
    return scenario; // All fields are owned values, independent of the editor model.
}
std::vector<ValidationIssue> connectorRuntimeIssues(const Network& network) {
    std::vector<ValidationIssue> issues;
    for(std::size_t i=0;i<network.connectors.size();++i) {
        const auto& c=network.connectors[i];
        if(!attachedAtLinkEnd(network,c.from,true) || !attachedAtLinkEnd(network,c.to,false))
            issues.push_back({"UNSUPPORTED_CONNECTOR_POSITION","connectors["+std::to_string(i)+"]"});
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
