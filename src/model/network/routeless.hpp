#pragma once
#include "network.hpp"

namespace trafficsim {
// M2.1.1: vehicles with no route, as Vissim runs them. A vehicle entering on a Link follows its
// lane; where the lane has several ways out each gets an equal share, and a routing decision
// placed on a Link it reaches sends it towards that decision's destinations by relative flow.
// Every choice is random, independent and fixed, so the whole tree is expanded at compile time
// into complete lane-level paths with a probability each -- the engine still sees static routes.
struct PlacedDecision {
    std::string id, linkId;
    std::string path; // where a problem with this decision is reported, e.g. "routingDecisions[2]"
    // Each destination as the Link/Connector chains a route would name, starting on linkId, and
    // its relative flow. Several chains when a destination is reached equally short by several
    // ways (a taper and a pocket entry, say): each lane takes the first one it can drive.
    struct Destination { std::vector<std::vector<std::string>> chains; double weight{}; };
    std::vector<Destination> destinations;
};
// M3.2.8b/D93: a decision's destination is a family -- one chain per lane of the decision's Link,
// and a lane that cannot reach it is a stub whose vehicles change lanes there. `name` is
// "<decision>><destination>", `linkId` the decision's Link and `lane` this path's lane on it.
struct FamilyTag {
    std::string name, linkId;
    std::size_t lane{};
    bool operator==(const FamilyTag&) const = default;
};
struct WeightedChain {
    std::vector<std::string> laneChain; // lane and connector-path ids, as routeLaneChains gives
    std::size_t lane{};                 // index of the starting lane in the Link's lanes
    // Probability of this path for a vehicle in that lane -- or, when the result is
    // `byDestination`, the fraction of the Link's whole input.
    double share{};
    // Every family the path belongs to, outermost first: an entry decision's full route can
    // also pass a downstream decision (D93). Empty for a free-walk path. `stub` is its role in
    // the last one; in every earlier one it is full.
    bool stub{};
    std::vector<FamilyTag> families;
};
struct RoutelessResult {
    std::vector<WeightedChain> chains;
    // Blocking: ROUTELESS_CYCLE / ROUTELESS_TOO_MANY_PATHS carry an empty path (the caller names
    // the input); UNKNOWN_LINK likewise. Advisory at a decision: ROUTING_DECISION_LANE_UNSERVED,
    // and ROUTING_DECISION_LANE_FIXED when a downstream stub could not be kept (D93 rule 4).
    std::vector<ValidationIssue> issues, advisories;
    // A decision placed on the entry Link itself splits each destination's flow equally over the
    // Link's lanes (M3.2.8b); a lane that cannot reach it enters on a stub and changes lanes. So
    // the typed proportions hold exactly, and the input's lane split is unused.
    bool byDestination{};
};
constexpr std::size_t kMaxRoutelessPaths = 256;
// D44: the lane end is a way out of the network only when the last way out along the lane leaves
// more than this before it. A Connector drawn a little short of the end (the author clicked near
// it) leaves a remainder shorter than the shortest shipped vehicle (car, 4.5 m), which no vehicle
// can mean to drive into. Measured on the Link's reference polyline, like every station.
constexpr double kRoutelessStubLength = 4.5;
// Deterministic: the Link's lanes in order, then ways out in connector-path order, then
// destinations in decision order. For each starting lane the shares sum to 1 unless an issue
// cut a branch off.
RoutelessResult routelessChains(const Network&, const std::string& linkId,
                                const std::vector<PlacedDecision>& decisions);
}
