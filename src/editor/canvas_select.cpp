#include "canvas.hpp"
#include <QLineF>
#include <algorithm>
#include <cmath>

namespace trafficsim {
std::optional<int> EditorCanvas::objectLevel(const std::string& id) const {
    if (!document_) return {};
    for (const auto& link : document_->network.links) if (link.id == id) return link.level;
    for (const auto& connector : document_->network.connectors) if (connector.id == id) return connector.level;
    for (const auto& head : document_->network.signalHeads) if (head.id == id)
        if (const auto at = headPosition(head)) return at->second;
    return {};
}
void EditorCanvas::setVisibleLevel(std::optional<int> level) {
    visibleLevel_ = level;
    std::erase_if(selection_, [&](const auto& id) {
        const auto at = objectLevel(id);
        return !at || !levelVisible(*at);
    });
    cancel();
    if (visibleLevelChanged) visibleLevelChanged(level);
    if (selectionChanged) selectionChanged();
}
bool EditorCanvas::isSelected(const std::string& id) const {
    return std::find(selection_.begin(), selection_.end(), id) != selection_.end();
}
void EditorCanvas::notifySelection() { redraw(); if (selectionChanged) selectionChanged(); }
void EditorCanvas::select(const std::string& id) {
    setSelection({id});
}
void EditorCanvas::setSelection(std::vector<std::string> ids) {
    cancel(); selection_.clear();
    bool reveal = false;
    for (auto& id : ids) if (const auto level = objectLevel(id); level && !isSelected(id)) {
        reveal = reveal || !levelVisible(*level);
        selection_.push_back(std::move(id));
    }
    // A deliberate table/diagnostic selection should be visible, not an invisible edit target.
    if (reveal) {
        visibleLevel_.reset();
        if (visibleLevelChanged) visibleLevelChanged({});
    }
    vertex_ = -1; notifySelection();
}
void EditorCanvas::toggle(const std::string& id) {
    auto ids = selection_;
    const auto at = std::find(ids.begin(), ids.end(), id);
    // Re-adding moves the object to the end, so the last thing clicked is always the primary.
    if (at != ids.end()) ids.erase(at);
    else ids.push_back(id);
    setSelection(std::move(ids));
}
std::vector<std::string> EditorCanvas::inRectangle(Point a, Point b) const {
    std::vector<std::string> result;
    if (!document_) return result;
    const QRectF box = QRectF(QPointF(a.x, a.y), QPointF(b.x, b.y)).normalized();
    // Network order, links before connectors, so the same band always yields the same list.
    for (const auto& link : document_->network.links)
        if (levelVisible(link.level) && objectShape(link.id).intersects(box)) result.push_back(link.id);
    for (const auto& connector : document_->network.connectors)
        if (levelVisible(connector.level) && objectShape(connector.id).intersects(box)) result.push_back(connector.id);
    for(const auto& h:document_->network.signalHeads)if(const auto at=headPosition(h))
        if(levelVisible(at->second) && objectShape(h.id).intersects(box))result.push_back(h.id);
    return result;
}
void EditorCanvas::frame(const std::string& id) {
    if (!document_ || id.empty()) return;
    auto bounds=objectShape(id).boundingRect();if(bounds.isEmpty())return;
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
