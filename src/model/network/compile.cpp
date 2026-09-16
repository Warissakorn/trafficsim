#include "network.hpp"
#include "../../core/validate.hpp"
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
