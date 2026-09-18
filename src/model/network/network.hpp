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
// What is painted on one boundary line. Vissim's MarkingType, reduced to the two kinds this
// editor draws; the renderer maps it to a pen style.
enum class MarkingType { solid, dashed };
struct Connector {
    std::string id; LaneReference from, to; std::vector<Point> geometry;
    int fromLaneCount{1}, toLaneCount{1}, level{};
    std::string displayType{"default"};
    std::vector<double> laneBlend{}; // Frozen interpolation weights when rebasing the first lane.
    std::string name; // Vissim's Name. See Link::name.
    // Vissim's Lanes tab, schema 6. Both are ordered last, after `name`, for the reason stated on
    // Link::name: every existing brace-initialisation keeps meaning what it says.
    //
    // `laneWidths` is one metre value per lane path, or EMPTY meaning "take the width from the
    // Links each end joins", which is what every Connector drawn before schema 6 does and what a
    // Connector whose lanes were never given a width must keep doing.
    std::vector<double> laneWidths{};
    // `laneMarkings` is one entry per INTERIOR divider, so `paths - 1` of them, or empty for the
    // default. The two outer edges are always solid: they are the edge of the carriageway, not a
    // lane divider, and nothing in this milestone makes them authorable.
    std::vector<MarkingType> laneMarkings{};
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
// The polyline between two stations, keeping every original vertex that lies between them.
// polylineSpan(g, 0, polylineLength(g)) returns g itself, which is what lets a lane with no
// interior attachment keep its geometry and its length bit for bit.
std::vector<Point> polylineSpan(const std::vector<Point>&, double from, double to);
void replaceLaneBundle(Link&, std::vector<Lane> lanes, bool leading);
std::vector<double> connectorBlendWeights(const Connector&);
void resizeConnectorEdges(const Network&, Connector&, int fromCount, int toCount, bool leading);
// The width of each lane path at the Connector's two ends: the authored width where the Connector
// carries one, and the width of the Link lane that end joins otherwise. Zero at an end where the
// path is a surplus lane, which is what makes it taper closed rather than run at full width.
// The ONE place a Connector's width is decided -- connectorBoundaries draws from it and
// connectorShapeIssues measures from it, so authoring a width cannot make the two disagree.
struct ConnectorLaneWidths { std::vector<double> source, target; };
ConnectorLaneWidths connectorLaneWidths(const Network&, const Connector&);
std::vector<std::vector<Point>> connectorBoundaries(const Network&, const Connector&);
// What the mouth correction did at each end, in metres, so it is a number rather than a look.
// `shift` is how far each boundary slid ALONG its own offset curve to land on the Link's
// cross-section, `zone` the length over which that slide decays back to nothing, and `residual`
// the part of the slide a Connector too short to carry it did not get -- the distance its mouth
// still stands off the Link. A residual above zero is the honest report that the arrival is too
// oblique for the room available, not a defect.
struct ConnectorMouthFit { std::vector<double> shift; double zone{},residual{}; };
struct ConnectorMouthFits { ConnectorMouthFit source,target; };
ConnectorMouthFits connectorMouthFit(const Network&, const Connector&);
// The same idea for a connector: the middle of its whole width, point for point with its
// stored geometry, which is the first lane's path.
std::vector<Point> connectorCentreline(const Network&, const Connector&);
// What to draw on a Connector: its two outer edges, plus an interior divider for each pair of
// adjacent lane paths, trimmed to the stretch where those two lanes are genuinely side by side.
// Where a range merges, the divider stops instead of running down the middle of the single lane
// the paths have converged into, which is not a place a marking belongs.
struct ConnectorMarking { std::vector<Point> geometry; bool edge{}; MarkingType type{MarkingType::solid}; };
std::vector<ConnectorMarking> connectorMarkings(const Network&, const Connector&);
// Lanes from this reference to the last lane of its link; 0 when the reference is unknown.
int lanesFromReference(const Network&, const LaneReference&);
// Snap both ends onto the lanes they NAME. For an edit whose input IS the reference: creating a
// Connector, or moving one of its ends onto another lane.
void anchorConnectorEnds(const Network&, Connector&);
// True when `p` lies on the carriageway of this lane -- within half its width of the lane's own
// middle. The ONE test for whether a Connector end is still on its Link.
bool laneContains(const Network&, const LaneReference&, Point);
// A Connector keeps its own position. Each end that is still on the lane it names is snapped back
// onto that lane's middle, with its station moved to wherever the author has put the end; an end
// that has come off is left where it is. FALSE means an end is off its Link, and a Connector with
// an end off its Link is deleted by the caller -- it has nothing left to connect.
bool reanchorConnector(const Network&, Connector&);
Point laneAttachment(const Network&, const LaneReference&, bool outgoing);
// The station a reference resolves to, filling in the end/start its absent value means.
double attachmentStation(const Network&, const LaneReference&, bool outgoing);
// True when this end sits exactly at the start or the end of its link, which is the only
// case the M0 whole-lane runtime can traverse.
bool attachedAtLinkEnd(const Network&, const LaneReference&, bool outgoing);
std::vector<ValidationIssue> connectorRuntimeIssues(const Network&);
// Blocks Run when the drawing creates a merge but the numbers that arbitrate it were not read
// from data/priority-rules/. Separate from connectorRuntimeIssues because it needs the resolved
// definition, and shared with runtimeDiagnostics so the panel and Run agree.
std::vector<ValidationIssue> priorityDefaultsIssues(const Network&, const PriorityDefaults&);
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
                                  int intermediatePoints=kDefaultIntermediatePoints);
// The travel directions a Connector's two ends leave and arrive on, which clamp its spline.
std::pair<Point,Point> connectorTangents(const Network&, const LaneReference& from, const LaneReference& to);

// One runnable piece of one authored lane. A lane is cut wherever a Connector attaches to its
// body, because the engine's Segment is a whole traversable length and a vehicle that leaves
// part way along one travels only part of it. `start` and `end` are metres along the LANE
// polyline, which is the same coordinate a signal head's position is already in.
//
// Derived, never persisted: sectioning changes no authored id, so it is a pure function of the
// drawing. That is what separates it from splitLink, which must rewrite routes because the ids
// an author stored really do change there.
struct LaneSection {
    std::string id; // laneId for the first section; sectionId(laneId, n) after that
    std::string linkId, laneId;
    double start{}, end{};
    std::vector<Point> geometry;    // the lane polyline clipped to [start, end]
    std::vector<std::string> next;  // the following section, then the paths leaving at its end
};
// Everything the runtime needs derived from the drawing, computed in one pass. Connector paths
// are carried here because the table needs them anyway and buildScenario must not derive them a
// second time -- see the cubic-cost note in compile.cpp.
struct RuntimeSections {
    std::vector<ConnectorPath> paths;
    std::vector<std::string> pathNext;   // parallel to paths: the section each path arrives on
    std::vector<LaneSection> sections;   // link order, then lane order, then station order
    // Connector ids whose interior attachment leaves no runnable section, so Run must still
    // refuse them. Recorded here rather than recomputed, so the gate and the table cannot
    // disagree about which attachments were usable.
    std::vector<std::string> unsectionable;
};
// A lane's first section keeps the lane's own id. That is load-bearing: a lane with no interior
// attachment compiles to exactly the Segment it always did, which is what keeps the four frozen
// baselines valid and keeps UNSUPPORTED_INTERNAL_INPUT meaningful, since a route still begins on
// the lane id and that section still has no predecessor.
std::string sectionId(const std::string& laneId, int index);
RuntimeSections runtimeSections(const Network&);
// The section a station on a lane falls in. A station exactly on a cut belongs to the section
// UPSTREAM of it, which is the convention splitLink already uses for a head sitting on a cut.
const LaneSection& sectionForStation(const RuntimeSections&, const std::string& laneId, double laneStation);
// The section that STARTS at a station: where a vehicle ARRIVING there continues. The mirror of
// sectionForStation, which resolves upstream because that is what a head standing on a cut wants.
const LaneSection& sectionStartingAt(const RuntimeSections&, const std::string& laneId, double laneStation);
// Authored routes name whole lanes; the runtime needs the chain of sections that carries them.
// Where the route leaves the lane part way along, the chain stops at the section that carries
// the Connector it leaves by, so the vehicle travels the drawn distance and no more.
std::vector<std::string> expandRouteSegments(const RuntimeSections&, const std::vector<std::string>&);
// One whole-lane segment per lane, collapsed back from the same table: what an author may name
// and store in a route. Never used to run anything -- offering a derived section id as something
// to persist would put a copy of derived data in the project file.
std::vector<Segment> authoringSegments(const RuntimeSections&);
// One priority rule per Connector arriving inside a lane body: the arriving path gives way to the
// section upstream of the arrival, which is the merge that sectioning creates. Derived from the
// drawing, never authored or persisted. Throws EDIT_NO_PRIORITY_DEFAULTS rather than deriving a
// rule with a zero gap time, which would be a merge nobody gives way at.
std::vector<PriorityRule> derivedPriorityRules(const RuntimeSections&, const PriorityDefaults&);
// A head's compiled form, with its segment and position rebased onto the section it sits on.
SignalHead rebaseHead(const RuntimeSections&, const NetworkSignalHead&);
// Zero-length segments are invalid in the core, and an attachment closer than this to a lane end
// or to another attachment leaves no section between them. 0.2 m is the span splitLink already
// uses for the same question.
inline constexpr double kMinSectionLength = 0.2;

std::vector<ValidationIssue> validateNetwork(const Network& network);
void assertValidNetwork(const Network& network);
// Unchecked assembly, for diagnostics that must not throw. Requires an already-valid network.
Scenario buildScenario(const Network& network, const ScenarioDefinition& definition);
Scenario compileScenario(const Network& network, const ScenarioDefinition& definition);
}
