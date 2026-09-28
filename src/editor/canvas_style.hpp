#pragma once
#include "../model/network/network.hpp"
#include "ui_design_tokens.hpp"
#include <QColor>
#include <QPen>
#include <cmath>

namespace trafficsim::canvasStyle {
inline const QColor selection{editorDesign::accent};
inline const QColor hover{editorDesign::accent};
inline const QColor active{editorDesign::accent};
inline constexpr double markingWidth = 0.10; // metres, including each stroke of a double line
inline constexpr double laneTabLength = 24; // logical pixels along the road
inline constexpr double laneTabDepth = 8;   // logical pixels outside the road

inline QColor connectorBoundaryColor(const QColor& surface) {
    const auto linear = [](double channel) {
        return channel <= .04045 ? channel / 12.92 : std::pow((channel + .055) / 1.055, 2.4);
    };
    const double luminance = .2126 * linear(surface.redF()) + .7152 * linear(surface.greenF()) +
                             .0722 * linear(surface.blueF());
    return luminance > .27 ? QColor(editorDesign::gray8) : QColor(editorDesign::gray0);
}

inline QPen markingPen(const QColor& colour, MarkingType type) {
    QPen pen(colour, markingWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    // Qt expresses dash lengths in pen widths. Keep 3 m dashes/gaps as the view zooms.
    if (type == MarkingType::dashed) pen.setDashPattern({3 / markingWidth, 3 / markingWidth});
    return pen;
}
}
