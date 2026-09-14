#include "network.hpp"
#include "../../core/validate.hpp"

namespace trafficsim {
Scenario buildScenario(const Network& network, const ScenarioDefinition& definition) {
    Scenario scenario;
    static_cast<ScenarioDefinition&>(scenario) = definition;
    for (const auto& link : network.links) for (const auto& lane : link.lanes) {
        Segment segment{lane.id, polylineLength(laneGeometry(link, lane.id, network.drivingSide)), {}};
        for (const auto& connector : network.connectors)
            if (connector.from.laneId == lane.id) segment.next.push_back(connector.id);
        scenario.segments.push_back(std::move(segment));
    }
    for (const auto& connector : network.connectors)
        scenario.segments.push_back({connector.id, polylineLength(connector.geometry), {connector.to.laneId}});
    for (const auto& head : network.signalHeads)
        scenario.signalHeads.push_back({head.id, signalSegment(head), head.position, head.programId});
    return scenario; // All fields are owned values, independent of the editor model.
}
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition) {
    // Order is load-bearing: the network pass reports UNKNOWN_LANE before laneGeometry can throw.
    assertValidNetwork(network);
    auto scenario = buildScenario(network, definition);
    assertValidScenario(scenario);
    return scenario;
}
}
