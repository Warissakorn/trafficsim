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
struct WeightedChain {
    std::vector<std::string> laneChain; // lane and connector-path ids, as routeLaneChains gives
    std::size_t lane{};                 // index of the starting lane in the Link's lanes
    // Probability of this path for a vehicle in that lane -- or, when the result is
    // `byDestination`, the fraction of the Link's whole input.
    double share{};
};
struct RoutelessResult {
    std::vector<WeightedChain> chains;
    // Blocking: ROUTELESS_CYCLE / ROUTELESS_TOO_MANY_PATHS carry an empty path (the caller names
    // the input); UNKNOWN_LINK likewise. Advisory: ROUTING_DECISION_LANE_UNSERVED at a decision.
    std::vector<ValidationIssue> issues, advisories;
    // A decision placed on the entry Link itself chooses the lanes: each destination's flow goes
    // equally to the lanes that reach it, as drivers would sort themselves by lane changing before
    // the junction. So the typed proportions hold exactly, and the input's lane split is unused.
    bool byDestination{};
};
constexpr std::size_t kMaxRoutelessPaths = 256;
// Deterministic: the Link's lanes in order, then ways out in connector-path order, then
// destinations in decision order. For each starting lane the shares sum to 1 unless an issue
// cut a branch off.
RoutelessResult routelessChains(const Network&, const std::string& linkId,
                                const std::vector<PlacedDecision>& decisions);
}
