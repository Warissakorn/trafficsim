#include "vehicle_shape.hpp"
#include <algorithm>

namespace trafficsim {
namespace {
constexpr double kCabLength = 2.4;   // front bumper to the cab gap, metres
constexpr double kTrailerFrom = 7.0; // a vehicle longer than this draws a separate cab
}
QPainterPath vehicleShape(double length, double width, bool detailed) {
    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);
    const double corner = std::min(.35, width * .2);
    path.addRoundedRect(-length, -width / 2, length, width, corner, corner);
    if (!detailed) return path;
    // Proportions of a car's windshield: it starts behind the bonnet and is inset from the sides.
    // A tractor's sits closer to its flat front.
    const bool cab = length > kTrailerFrom;
    const double inset = std::min(.2, width * .12), depth = std::min(.55, length * .15);
    const double from = cab ? .35 : std::min(.9, length * .22);
    path.addRoundedRect(-from - depth, -width / 2 + inset, depth, width - 2 * inset, depth * .3, depth * .3);
    if (cab) path.addRect(-kCabLength - .3, -width / 2, .3, width);
    return path;
}
}
