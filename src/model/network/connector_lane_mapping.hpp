#pragma once
#include "network.hpp"

namespace trafficsim {
// Topology only: shared by width derivation and runtime paths. Never reads a surface or path
// geometry, so building the surface cannot recurse through connectorPaths.
struct ConnectorLanePair { LaneReference from, to; };
std::vector<ConnectorLanePair> connectorLanePairs(const Network&, const Connector&);
}
