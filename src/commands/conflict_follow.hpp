#pragma once
#include "../project/document.hpp"

namespace trafficsim {
// D85: an authored conflict area is a setting on an overlap, not a place. After an edit that
// changed the drawing (Links, Connectors, driving side), each area's extents are re-derived from
// the overlap its pair has now -- the piece nearest where it was -- and its waiting lines keep
// their distance to it. An area whose pair no longer overlaps, or whose merge is gone, is removed
// with its rule, Stop/Yield and unused lines, as Vissim removes it. Geometry the coverage cannot
// measure leaves the area as it was, for the resolver to report. `before` is the drawing the
// edit started from; an edit that left it alone changes nothing.
void followGeometry(ProjectDocument&, const Network& before);
}
