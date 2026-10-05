#pragma once
#include "right_of_way.hpp"
namespace trafficsim {
// Calculation only. Connector samples use equation parameters, independent of editable drawing
// vertices. Overlap results are converted to schema-17 authoring stations at the boundary.
struct ConflictSurface {
    std::vector<Point> base, left, right;
    std::vector<double> stations; // Link metres, or Connector equation parameters
    const Connector* connector{};
};
std::optional<ConflictSurface> conflictSurface(const Network&, const ControlPathRef&);
std::vector<Point> conflictRuntimeOutline(const Network&, const ConflictSide&);
}
