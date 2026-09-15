#include "network_view.hpp"
#include "../core/routes.hpp"
#include "../core/simulation.hpp"
#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <limits>

namespace trafficsim {
NetworkView::NetworkView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(280, 260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setObjectName("networkView");
}
void NetworkView::setNetwork(const Network& network) {
    network_ = network; geometry_.clear();
    double minX = std::numeric_limits<double>::infinity(), minY = minX;
    double maxX = -minX, maxY = maxX;
    for (const auto& link : network.links)
        for (const auto& lane : link.lanes) geometry_[lane.id] = laneGeometry(link, lane.id, network.drivingSide);
    for (const auto& connector : network.connectors)for(const auto& path:connectorPaths(network,connector))geometry_[path.id]=path.geometry;
    for (const auto& [id, points] : geometry_) for (const auto& p : points) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    }
    bounds_ = QRectF(minX, minY, std::max(1.0, maxX - minX), std::max(1.0, maxY - minY));
    update();
}
void NetworkView::setFrame(const SimState& state) { frame_ = state; update(); }
void NetworkView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#14222d"));
    if (geometry_.empty() || !frame_.scenario) return;
    painter.setRenderHint(QPainter::Antialiasing);
    const double scale = std::min((width() - 64.0) / bounds_.width(), (height() - 64.0) / bounds_.height());
    QTransform transform;
    transform.translate(width() / 2.0, height() / 2.0);
    transform.scale(scale, -scale);
    transform.translate(-bounds_.center().x(), -bounds_.center().y());
    painter.setTransform(transform);
    const auto draw = [&](const std::vector<Point>& points, double width, const QColor& color) {
        QPolygonF polygon;
        for (const auto& p : points) polygon << QPointF(p.x, p.y);
        painter.setPen(QPen(color, width, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter.drawPolyline(polygon);
    };
    for (const auto& link : network_.links)
        for (const auto& lane : link.lanes) draw(geometry_.at(lane.id), lane.width, QColor("#536c7c"));
    for (const auto& connector : network_.connectors)for(const auto& p:connectorPaths(network_,connector))draw(p.geometry, 2.5, QColor("#386b78"));
    for (const auto& head : frame_.scenario->signalHeads) {
        const auto found = std::find_if(frame_.scenario->signalPrograms.begin(), frame_.scenario->signalPrograms.end(),
            [&](const auto& p) { return p.id == head.programId; });
        if (found == frame_.scenario->signalPrograms.end()) continue;
        const auto color = signalColorAt(*found, frame_.time);
        const auto position = pointAlong(geometry_.at(head.segmentId), head.position);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color == SignalColor::green ? QColor("#63e6be") :
                         color == SignalColor::amber ? QColor("#ffd166") : QColor("#ff6b78"));
        painter.drawEllipse(QPointF(position.x, position.y), 3.4 / scale, 3.4 / scale);
    }
    for (const auto& vehicle : frame_.vehicles) {
        const auto location = locateVehicle(*frame_.scenario, vehicle);
        const auto position = pointAlong(geometry_.at(location.segmentId), location.position);
        painter.setPen(Qt::NoPen); painter.setBrush(QColor("#f2f5d7"));
        painter.drawEllipse(QPointF(position.x, position.y), 2.6 / scale, 2.6 / scale);
    }
}
}
