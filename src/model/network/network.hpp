#pragma once
#include "../../core/types.hpp"

namespace trafficsim {
struct Point { double x{}, y{}; bool operator==(const Point&) const = default; };
struct Lane { std::string id; double width{}; bool operator==(const Lane&) const = default; };
struct Link {
    std::string id; std::vector<Point> geometry; std::vector<Lane> lanes;
    int level{}; std::string displayType{"default"};
    double laneOffset{}; // Bundle offset from reference geometry, in lane-order coordinates.
    bool operator==(const Link&) const = default;
};
struct LaneReference {
    std::string linkId, laneId;
    // Distance in metres along the link's reference polyline, as Vissim stores a position.
    // One station names one cross-section, so every lane of a range attaches square on a
    // curve, and stretching a link no longer slides what is attached part-way along it.
    // Absent means the source end / target start, whatever the link's length becomes.
    std::optional<double> station{};
    bool operator==(const LaneReference&) const = default;
};
struct Connector {
    std::string id; LaneReference from, to; std::vector<Point> geometry;
    int fromLaneCount{1}, toLaneCount{1}, level{};
    std::string displayType{"default"};
    std::vector<double> laneBlend{}; // Frozen interpolation weights when rebasing the first lane.
    bool operator==(const Connector&) const = default;
};
// One authored connector owns a contiguous range at each end. Individual runtime
// paths are derived, with stable ids; they are never stored as duplicate objects.
struct Network;
struct ConnectorPath { std::string id; LaneReference from, to; std::vector<Point> geometry; };
std::string connectorPathId(const Connector&, int index);
std::vector<ConnectorPath> connectorPaths(const Network&, const Connector&);
struct NetworkSignalHead {
    std::string id; LaneReference lane; double position{};
    std::string programId, connectorId;
    bool operator==(const NetworkSignalHead&) const = default;
};
enum class DrivingSide { left, right };
struct Network {
    std::string id;
    DrivingSide drivingSide{DrivingSide::left};
    std::vector<Link> links;
    std::vector<Connector> connectors;
    std::vector<NetworkSignalHead> signalHeads;
    bool operator==(const Network&) const = default;
};
double polylineLength(const std::vector<Point>& points);
double stationOfClosestPoint(const std::vector<Point>&, Point);
std::string signalSegment(const NetworkSignalHead&);
Point pointAlong(const std::vector<Point>& points, double distance);
std::vector<Point> laneGeometry(const Link& link, const std::string& laneId, DrivingSide side);
// Boundary 0 is before the first lane; boundary N is after the last.
std::vector<Point> laneBoundaryGeometry(const Link&, std::size_t boundary, DrivingSide);
std::vector<Point> offsetGeometry(const std::vector<Point>&, double offset);
// The centreline of the whole lane bundle: the reference polyline shifted by laneOffset.
// Grips, labels and direction markers belong here, never on the reference polyline, which
// sits at an arbitrary edge once lanes have been added to one side.
std::vector<Point> linkCentreline(const Link&, DrivingSide);
// Polylines derived from one reference share a vertex for vertex correspondence, because
// offsetGeometry emits one point per input point. A station on one therefore names a
// cross-section on the other: this is what keeps the mouth of a multi-lane Connector square
// on a curve, where the outer lane is the longer one. Stations outside `from` are clamped.
double matchedStation(const std::vector<Point>& from, const std::vector<Point>& to, double station);
void replaceLaneBundle(Link&, std::vector<Lane> lanes, bool leading);
std::vector<double> connectorBlendWeights(const Connector&);
void resizeConnectorEdges(const Network&, Connector&, int fromCount, int toCount, bool leading);
std::vector<std::vector<Point>> connectorBoundaries(const Network&, const Connector&);
// The same idea for a connector: the middle of its whole width, point for point with its
// stored geometry, which is the first lane's path.
std::vector<Point> connectorCentreline(const Network&, const Connector&);
// Lanes from this reference to the last lane of its link; 0 when the reference is unknown.
int lanesFromReference(const Network&, const LaneReference&);
// Move a connector onto its current attachments, carrying the interior points with the
// similarity transform that maps the old endpoint chord onto the new one.
void reanchorConnector(const Network&, Connector&);
Point laneAttachment(const Network&, const LaneReference&, bool outgoing);
// The station a reference resolves to, filling in the end/start its absent value means.
double attachmentStation(const Network&, const LaneReference&, bool outgoing);
// True when this end sits exactly at the start or the end of its link, which is the only
// case the M0 whole-lane runtime can traverse.
bool attachedAtLinkEnd(const Network&, const LaneReference&, bool outgoing);
std::vector<ValidationIssue> connectorRuntimeIssues(const Network&);
// A sampled cubic between lane attachments, aligned with their local travel directions.
// The returned polyline is the editable/persisted geometry; no second curve is stored.
std::vector<Point> connectorCurve(const Network&, const LaneReference& from, const LaneReference& to);
std::vector<ValidationIssue> validateNetwork(const Network& network);
void assertValidNetwork(const Network& network);
// Unchecked assembly, for diagnostics that must not throw. Requires an already-valid network.
Scenario buildScenario(const Network& network, const ScenarioDefinition& definition);
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition);
}
