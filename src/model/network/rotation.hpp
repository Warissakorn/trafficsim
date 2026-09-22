#pragma once
#include "network.hpp"
namespace trafficsim {
// Geometry-bearing selection, internal Connectors and heads carried by those roads, in
// network order. An independently selected head has no free geometry and is excluded.
std::vector<std::string> rotationObjects(const Network&, const std::vector<std::string>& ids);
// Centre of the affected road surfaces' world-axis bounds; heads do not affect the pivot.
// The same pivot and object set serve the dialog, gesture preview and command.
std::optional<Point> rotationCentre(const Network&, const std::vector<std::string>& ids);
// Positive degrees turn counter-clockwise in world coordinates. Full turns are exact no-ops.
// Reject non-finite input/output rather than putting invalid coordinates into a document.
Point rotatePoint(Point point, Point pivot, double degrees);
}
