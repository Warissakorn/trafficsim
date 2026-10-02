#pragma once
#include <QPainterPath>

namespace trafficsim {
// The plan view of one vehicle in the Run view (D97). Local frame, metres: the front bumper at the
// origin, the body along -x, centred on y = 0, so a caller places it at the located station and
// rotates it to the heading. One path with odd-even holes -- windshield, and a cab gap on a long
// vehicle -- so the road shows through and every vehicle stays a single one-colour scene item.
// `detailed` false gives the plain rounded body, for zooms where the holes would be sub-pixel.
QPainterPath vehicleShape(double length, double width, bool detailed);
}
