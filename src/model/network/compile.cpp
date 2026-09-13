#include "network.hpp"
#include "../../core/validate.hpp"

namespace trafficsim {
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition) {
    assertValidNetwork(network);
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
        scenario.signalHeads.push_back({head.id, head.lane.laneId, head.position, head.programId});
    assertValidScenario(scenario);
    return scenario; // All fields are owned values, independent of the editor model.
}
}
