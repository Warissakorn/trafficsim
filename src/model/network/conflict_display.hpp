#pragma once
#include "right_of_way.hpp"

namespace trafficsim {
// Measured geometry and its separate station spans along each participant's lane.
// Merge spans describe physical mouths; they do not replace the runtime admission extents.
struct ConflictAreaGeometry {
    ConflictPolygons polygons;
    std::vector<ConflictSide> first,second;
};
ConflictAreaGeometry conflictAreaGeometry(const Network&,ConflictKind,const ConflictSide&,const ConflictSide&);
}
