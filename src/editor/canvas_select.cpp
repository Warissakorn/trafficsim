#include "canvas.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
// Segment/rectangle overlap: a link crossing the band counts even when no vertex is inside it.
bool crosses(Point a, Point b, const QRectF& box) {
    if (box.contains(a.x, a.y) || box.contains(b.x, b.y)) return true;
    const QLineF segment(a.x, a.y, b.x, b.y);
    const QPointF corners[4]{box.topLeft(), box.topRight(), box.bottomRight(), box.bottomLeft()};
    for (int i = 0; i < 4; ++i)
        if (segment.intersects(QLineF(corners[i], corners[(i + 1) % 4]), nullptr) == QLineF::BoundedIntersection)
            return true;
    return false;
}
bool touches(const std::vector<Point>& geometry, const QRectF& box) {
    for (std::size_t i = 1; i < geometry.size(); ++i) if (crosses(geometry[i - 1], geometry[i], box)) return true;
    return false;
}
}
bool EditorCanvas::isSelected(const std::string& id) const {
    return std::find(selection_.begin(), selection_.end(), id) != selection_.end();
}
void EditorCanvas::notifySelection() { redraw(); if (selectionChanged) selectionChanged(); }
void EditorCanvas::select(const std::string& id) {
    cancel(); selection_.clear();
    if (!id.empty()) selection_.push_back(id);
    vertex_ = -1; notifySelection();
}
void EditorCanvas::setSelection(std::vector<std::string> ids) {
    cancel(); selection_.clear();
    for (auto& id : ids) if (!id.empty() && !isSelected(id)) selection_.push_back(std::move(id));
    vertex_ = -1; notifySelection();
}
void EditorCanvas::toggle(const std::string& id) {
    cancel();
    if (id.empty()) return;
    const auto at = std::find(selection_.begin(), selection_.end(), id);
    // Re-adding moves the object to the end, so the last thing clicked is always the primary.
    if (at != selection_.end()) selection_.erase(at);
    else selection_.push_back(id);
    vertex_ = -1; notifySelection();
}
std::vector<std::string> EditorCanvas::inRectangle(Point a, Point b) const {
    std::vector<std::string> result;
    if (!document_) return result;
    const QRectF box = QRectF(QPointF(a.x, a.y), QPointF(b.x, b.y)).normalized();
    // Network order, links before connectors, so the same band always yields the same list.
    for (const auto& link : document_->network.links)
        if (touches(link.geometry, box)) result.push_back(link.id);
    for (const auto& connector : document_->network.connectors)
        if (touches(connector.geometry, box)) result.push_back(connector.id);
    return result;
}
void EditorCanvas::frame(const std::string& id) {
    if (!document_ || id.empty()) return;
    const std::vector<Point>* geometry = nullptr;
    for (const auto& link : document_->network.links) if (link.id == id) geometry = &link.geometry;
    if (!geometry) for (const auto& c : document_->network.connectors) if (c.id == id) geometry = &c.geometry;
    if (!geometry || geometry->empty()) return;
    QRectF bounds(QPointF(geometry->front().x, geometry->front().y), QSizeF(0, 0));
    for (const auto& p : *geometry) {
        bounds.setLeft(std::min(bounds.left(), p.x)); bounds.setRight(std::max(bounds.right(), p.x));
        bounds.setTop(std::min(bounds.top(), p.y)); bounds.setBottom(std::max(bounds.bottom(), p.y));
    }
    bounds = bounds.adjusted(-5, -5, 5, 5);
    const auto visible = mapToScene(viewport()->rect()).boundingRect();
    // Zoom only when the object does not fit, or is so small it would be invisible: a jump
    // should not silently throw away the zoom level the user chose.
    const double fit = std::min(visible.width() / bounds.width(), visible.height() / bounds.height());
    if (fit < 1 || fit > 10) {
        const double scale = std::clamp(std::abs(transform().m11()) * std::clamp(fit, 0.1, 10.0), 0.05, 100.0);
        setTransform(QTransform::fromScale(scale, -scale));
    }
    centerOn(bounds.center()); redraw();
}
}
