#pragma once
// M3.2.2: what the authored right-of-way controls (control.hpp) mean. Two kinds of check, kept
// apart on purpose (docs/M3_CONTRACT.md §2):
//   - structural issues reject the edit or the load -- the document stays as it was;
//   - runtime issues leave a savable draft that Run refuses, each naming the object at fault.
#include "network.hpp"

namespace trafficsim {
// Duplicate or blank ids, a reference to a Link/Connector/waiting line/area that does not
// exist, a non-finite station, entry not before exit, a rule whose gap time or headway is not
// finite and positive, two rules on one area, a malformed path (neither or both kinds set).
// Paths are "rightOfWay.<collection>[i]..." so the editor's Problems rows can name them.
std::vector<ValidationIssue> rightOfWayStructuralIssues(const Network&);

// The runtime segment a path reference names, or empty when it no longer resolves to exactly
// one: a lane gone from its Link, a Connector lane pair that now matches zero or several paths.
// Resolution is by id, never by ordinal, so a lane-count change cannot retarget a control.
std::string resolveControlPath(const Network&, const RuntimeSections&, const ControlPathRef&,
                               double station);

// M3.2.2c. Where two lane surfaces really overlap, as an interval of authored stations on each
// path's own polyline (a Link's reference polyline, a Connector's base polyline) -- the numbers a
// crossing area's extents must contain. A surface is the quad strip between the lane's two
// boundaries, vertex for vertex with that polyline, so a station names the same cross-section
// matchedStation does. `none` is no overlap of positive area (a shared edge is not a crossing);
// `unsupported` names what this resolver cannot measure honestly -- two overlaps (the paths
// cross twice), a strip folded on a tight bend, boundaries that do not correspond -- and
// `unresolved` a path that does not name exactly one lane or Connector path.
struct StationInterval { double from{}, to{}; };
struct SurfaceOverlap {
    enum class Status { overlap, none, unsupported, unresolved } status{Status::unresolved};
    StationInterval first, second;
};
SurfaceOverlap surfaceOverlap(const Network&, const ControlPathRef& first, const ControlPathRef& second);
// M3.2.4: what the editor draws, from the same lane strips. A side's area is the strip between
// its entry and exit as a closed outline; a waiting line is a bar across its lane. Empty when the
// reference does not resolve -- the editor draws nothing rather than a guess.
std::vector<Point> conflictSideOutline(const Network&, const ConflictSide&);
std::optional<std::pair<Point, Point>> waitingLineBar(const Network&, const ControlPoint&);

// One merge: the incoming segments that arrive on one section, in drawing order -- the order
// derivedPriorityRules ranks them in. `explicitControl` is set when an authored area covers two
// of them; the authored areas then own the whole group (§3, policy 2).
struct MergeGroup {
    std::string section;
    std::vector<std::string> incoming;
    bool explicitControl{};
};
std::vector<MergeGroup> mergeGroups(const Network&, const RuntimeSections&);
// M3.2.4, for the editor's two named actions. The merge sections any path of this Connector
// arrives on ("Take over merge"), and the one an authored merge area covers ("Restore automatic
// priority"); empty when there is none.
std::vector<std::string> mergeSectionsOf(const Network&, const RuntimeSections&, const std::string& connectorId);
std::string mergeSectionOfArea(const Network&, const RuntimeSections&, const ConflictArea&);

// The one resolver the compiler and the diagnostics share. `issues` are runtime (Run-blocking)
// issues; with none, `rules` and `zones` are what compiles. With no authored controls `rules` is
// exactly derivedPriorityRules -- the frozen fixtures and seed 42 depend on it.
// `rules` are the derived rules of every merge nobody overrode. `zones` are the authored areas the
// admission solver runs: each crossing with nothing reported against it (M3.2.3a/b), and each
// area of a complete, acyclic merge group (M3.2.3c) -- an authored merge no longer compiles to a
// rule, because the solver also holds the major side for an admitted minor vehicle.
struct RightOfWayResolution {
    std::vector<PriorityRule> rules;
    std::vector<ConflictZone> zones;
    std::vector<ValidationIssue> issues;
};
RightOfWayResolution resolveRightOfWay(const Network&, const RuntimeSections&, const PriorityDefaults&);
}
