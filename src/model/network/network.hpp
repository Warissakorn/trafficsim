#pragma once
#include "../../core/types.hpp"

namespace trafficsim {
struct Point { double x{}, y{}; bool operator==(const Point&) const = default; };
struct Lane { std::string id; double width{}; bool operator==(const Lane&) const = default; };
struct Link {
    std::string id; std::vector<Point> geometry; std::vector<Lane> lanes;
    int level{}; std::string displayType{"default"};
    double laneOffset{}; // Bundle offset from reference geometry, in lane-order coordinates.
    // Vissim's Name: the author's own label for this object, free text, never a key. Empty is
    // normal and means the object is referred to by its id alone. Ordered last so that every
    // existing brace-initialisation of a Link keeps meaning what it says.
    std::string name;
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
    std::string name; // Vissim's Name. See Link::name.
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
    std::string name; // Vissim's Name. See Link::name.
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
// The same miter-joined offset with a distance that varies point by point, which is how a road
// that gains or drops a lane along its length keeps every other lane at its own full width.
std::vector<Point> offsetGeometry(const std::vector<Point>&, const std::vector<double>& offsets);
// The same polyline with any self-crossing loop cut out and closed at the crossing point.
// Drawing only: the loop an offset makes on a tight bend is a notch in the line round a
// surface that is filled correctly without it.
std::vector<Point> trimSelfIntersections(const std::vector<Point>&);
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
// What to draw on a Connector: its two outer edges, plus an interior divider for each pair of
// adjacent lane paths, trimmed to the stretch where those two lanes are genuinely side by side.
// Where a range merges, the divider stops instead of running down the middle of the single lane
// the paths have converged into, which is not a place a marking belongs.
struct ConnectorMarking { std::vector<Point> geometry; bool edge{}; };
std::vector<ConnectorMarking> connectorMarkings(const Network&, const Connector&);
// Lanes from this reference to the last lane of its link; 0 when the reference is unknown.
int lanesFromReference(const Network&, const LaneReference&);
// Move a connector onto its current attachments the way Vissim does: the one poly point that
// is attached to each Link moves, and the points the author placed stay where they are.
void reanchorConnector(const Network&, Connector&);
Point laneAttachment(const Network&, const LaneReference&, bool outgoing);
// The middle of the whole lane range a Connector attaches to, at its station -- the point its
// own geometry starts and ends at. For a one-lane range this is laneAttachment; for a wider one
// it sits (N-1)/2 lane widths away from it. A Connector's stored line is its own centre, the way
// a Link's reference line is, and not the path of whichever lane happens to be first.
Point connectorAttachment(const Network&, const LaneReference&, int laneCount, bool outgoing);
// The station a reference resolves to, filling in the end/start its absent value means.
double attachmentStation(const Network&, const LaneReference&, bool outgoing);
// True when this end sits exactly at the start or the end of its link, which is the only
// case the M0 whole-lane runtime can traverse.
bool attachedAtLinkEnd(const Network&, const LaneReference&, bool outgoing);
std::vector<ValidationIssue> connectorRuntimeIssues(const Network&);
// Advisory only, and deliberately not part of connectorRuntimeIssues, which blocks Run: a turn
// tighter than the Connector's own half-width is undrivable but still a legal drawing.
std::vector<ValidationIssue> connectorShapeIssues(const Network&);
// The default shape between two lane attachments: the two attachments and
// kDefaultIntermediatePoints intermediate points along the arc-like cubic that joins them,
// aligned with each lane's local travel direction. A Connector is drawn straight between its
// points and mitered at each one, exactly as a Link is -- what the count buys is how closely the
// polygon follows the turn, which is what Vissim's Intermediate points field does.
inline constexpr int kDefaultIntermediatePoints=3;
std::vector<Point> connectorCurve(const Network&, const LaneReference& from, const LaneReference& to,
                                  int fromLaneCount, int toLaneCount,
                                  int intermediatePoints=kDefaultIntermediatePoints);
// The travel directions a Connector's two ends leave and arrive on, which clamp its spline.
std::pair<Point,Point> connectorTangents(const Network&, const LaneReference& from, const LaneReference& to);

std::vector<ValidationIssue> validateNetwork(const Network& network);
void assertValidNetwork(const Network& network);
// Unchecked assembly, for diagnostics that must not throw. Requires an already-valid network.
Scenario buildScenario(const Network& network, const ScenarioDefinition& definition);
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition);
}
