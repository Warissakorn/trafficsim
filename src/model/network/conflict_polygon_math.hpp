#pragma once
#include "right_of_way.hpp"
namespace trafficsim {
// Inverse bilinear cross-section map, shared by body overlap and mouth station measurement.
double stationInLaneQuad(Point l0,Point l1,Point r0,Point r1,Point p,double start,double length);
double polygonSignedArea(const std::vector<Point>&);
std::vector<Point> intersectConvexPolygons(std::vector<Point>,const std::vector<Point>&);
ConflictPolygons triangulatePolygon(std::vector<Point>);
}
