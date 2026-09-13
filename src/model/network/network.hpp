#pragma once
#include "../../core/types.hpp"

namespace trafficsim {
struct Point { double x{}, y{}; bool operator==(const Point&) const = default; };
struct Lane { std::string id; double width{}; };
struct Link { std::string id; std::vector<Point> geometry; std::vector<Lane> lanes; };
struct LaneReference { std::string linkId, laneId; };
struct Connector { std::string id; LaneReference from, to; std::vector<Point> geometry; };
struct NetworkSignalHead { std::string id; LaneReference lane; double position{}; std::string programId; };
enum class DrivingSide { left, right };
struct Network {
    std::string id;
    DrivingSide drivingSide{DrivingSide::left};
    std::vector<Link> links;
    std::vector<Connector> connectors;
    std::vector<NetworkSignalHead> signalHeads;
};
double polylineLength(const std::vector<Point>& points);
Point pointAlong(const std::vector<Point>& points, double distance);
std::vector<Point> laneGeometry(const Link& link, const std::string& laneId, DrivingSide side);
std::vector<ValidationIssue> validateNetwork(const Network& network);
void assertValidNetwork(const Network& network);
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition);
}
