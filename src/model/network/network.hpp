#pragma once
#include "../../core/types.hpp"

namespace trafficsim {
struct Point { double x{}, y{}; bool operator==(const Point&) const = default; };
struct Lane { std::string id; double width{}; };
struct Link { std::string id; std::vector<Point> geometry; std::vector<Lane> lanes; };
struct LaneReference { std::string linkId, laneId; bool operator==(const LaneReference&) const = default; };
struct Connector { std::string id; LaneReference from, to; std::vector<Point> geometry; };
struct NetworkSignalHead { std::string id; LaneReference lane; double position{}; std::string programId; std::string connectorId; };
enum class DrivingSide { left, right };
struct Network {
    std::string id;
    DrivingSide drivingSide{DrivingSide::left};
    std::vector<Link> links;
    std::vector<Connector> connectors;
    std::vector<NetworkSignalHead> signalHeads;
};
double polylineLength(const std::vector<Point>& points);
double stationOfClosestPoint(const std::vector<Point>&, Point);
std::string signalSegment(const NetworkSignalHead&);
Point pointAlong(const std::vector<Point>& points, double distance);
std::vector<Point> laneGeometry(const Link& link, const std::string& laneId, DrivingSide side);
// A sampled cubic between lane endpoints, aligned with their travel directions.
// The returned polyline is the editable/persisted geometry; no second curve is stored.
std::vector<Point> connectorCurve(const Network&, const LaneReference& from, const LaneReference& to);
std::vector<ValidationIssue> validateNetwork(const Network& network);
void assertValidNetwork(const Network& network);
// Unchecked assembly, for diagnostics that must not throw. Requires an already-valid network.
Scenario buildScenario(const Network& network, const ScenarioDefinition& definition);
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition);
}
