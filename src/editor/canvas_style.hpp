#pragma once
#include "../model/network/network.hpp"
#include "ui_design_tokens.hpp"
#include <QColor>
#include <QPen>
#include <cmath>

namespace trafficsim::canvasStyle {
// Functions, not namespace-scope QColors: the palette must not be built before QApplication.
inline QColor selection() { return editorDesign::accent(); }
inline QColor hover() { return editorDesign::accent(); }
inline QColor active() { return editorDesign::accent(); }
inline QColor error() { return editorDesign::semantic(editorDesign::Semantic::error); }
inline QColor warning() { return editorDesign::semantic(editorDesign::Semantic::warning); }
inline QColor advisory() { return editorDesign::semantic(editorDesign::Semantic::advisory); }
inline QColor ok() { return editorDesign::semantic(editorDesign::Semantic::ok); }
// Neutral overlay ink: linework a tool draws that is not state (draft, band, waiting line).
inline QColor ink() { return editorDesign::role(QPalette::Dark); }
inline constexpr double markingWidth = 0.10; // metres, including each stroke of a double line
inline constexpr double laneTabLength = 24; // logical pixels along the road
inline constexpr double laneTabDepth = 8;   // logical pixels outside the road

inline QColor connectorBoundaryColor(const QColor& surface) {
    const auto linear = [](double channel) {
        return channel <= .04045 ? channel / 12.92 : std::pow((channel + .055) / 1.055, 2.4);
    };
    const double luminance = .2126 * linear(surface.redF()) + .7152 * linear(surface.greenF()) +
                             .0722 * linear(surface.blueF());
    return editorDesign::role(luminance > .27 ? QPalette::Text : QPalette::Base);
}

inline QPen markingPen(const QColor& colour, MarkingType type) {
    QPen pen(colour, markingWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    // Qt expresses dash lengths in pen widths. Keep 3 m dashes/gaps as the view zooms.
    if (type == MarkingType::dashed) pen.setDashPattern({3 / markingWidth, 3 / markingWidth});
    return pen;
}
}
