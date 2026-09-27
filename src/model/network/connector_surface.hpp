#pragma once
#include "network.hpp"
#include <array>

namespace trafficsim {
// Rendering only. P1/P4 intersect the local Connector and Link edges; P2 is the centre
// of the attached lane range; P3 is the nearest point to P2 on the far LINK edge.
// Near/far are measured from the Connector body, not fixed left/right labels.
struct ConnectorMouth {
    std::array<Point,4> points;
    bool firstBoundaryNear{};
    // M3.2.9b: one point on P1->P2->P3->P4 per Link lane boundary of the attached range, in the
    // order of the Connector's own boundaries: [0] is where the first rail ends, back() the last.
    // Interior Link boundaries cross P1->P2->P3; the two range edges are P1 and P4.
    std::vector<Point> boundaries;
};
struct ConnectorSurface {
    std::vector<Point> outline; // Closed implicitly, in perimeter order.
    std::vector<ConnectorMarking> markings;
    std::optional<ConnectorMouth> source, target;
};
// One surface for paint, picking and previews. Parallel or unusable local intersections
// retain the existing cap. Runtime paths and conflict strips keep their existing geometry.
ConnectorSurface connectorSurface(const Network&, const Connector&);
}
