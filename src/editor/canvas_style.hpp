#pragma once
#include "../model/network/network.hpp"
#include <QColor>
#include <QPen>

namespace trafficsim::canvasStyle {
inline const QColor selection{"#167b98"};
inline const QColor hover{"#38a3c4"};
inline const QColor active{"#de8618"};
inline constexpr double markingWidth = 0.10; // metres, including each stroke of a double line
inline constexpr double laneTabLength = 24; // logical pixels along the road
inline constexpr double laneTabDepth = 8;   // logical pixels outside the road

inline QPen markingPen(const QColor& colour, MarkingType type) {
    QPen pen(colour, markingWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    // Qt expresses dash lengths in pen widths. Keep 3 m dashes/gaps as the view zooms.
    if (type == MarkingType::dashed) pen.setDashPattern({3 / markingWidth, 3 / markingWidth});
    return pen;
}
}
