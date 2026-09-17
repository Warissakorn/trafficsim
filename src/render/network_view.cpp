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
    // Keyed by SECTION id, from the same runtimeSections call the scenario was compiled from.
    // paintEvent resolves a head and a vehicle with geometry_.at(), so this map has to cover
    // every segment id the scenario can name -- and it does, because both come from this table.
    for (const auto& section : runtimeSections(network).sections) geometry_[section.id] = section.geometry;
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
    const auto road=[&](const std::vector<std::vector<Point>>& boundaries,
                        const std::vector<ConnectorMarking>& markings,const QColor& color) {
        QPolygonF surface;
        for(const auto& p:boundaries.front())surface<<QPointF(p.x,p.y);
        for(auto it=boundaries.back().rbegin();it!=boundaries.back().rend();++it)surface<<QPointF(it->x,it->y);
        // Winding, so a ribbon that overlaps itself on a tight turn stays road instead of
        // punching the overlap out as a hole.
        painter.setPen(Qt::NoPen);painter.setBrush(color);
        painter.drawPolygon(surface,Qt::WindingFill);painter.setBrush(Qt::NoBrush);
        for(const auto& marking:markings) {
            // An outer edge is always solid; an interior divider draws the MarkingType the
            // Connector carries, which defaults to the dashed line it always was.
            QPen pen(QColor("#d9e5eb"),1,
                     marking.edge||marking.type==MarkingType::solid?Qt::SolidLine:Qt::DashLine);
            pen.setCosmetic(true);
            painter.setPen(pen);QPolygonF line;
            for(const auto& p:marking.geometry)line<<QPointF(p.x,p.y);
            painter.drawPolyline(line);
        }
    };
    for(const auto& link:network_.links) {
        std::vector<std::vector<Point>> boundaries;std::vector<ConnectorMarking> markings;
        for(std::size_t i=0;i<=link.lanes.size();++i)
            boundaries.push_back(trimSelfIntersections(laneBoundaryGeometry(link,i,network_.drivingSide)));
        for(std::size_t i=0;i<boundaries.size();++i) {
            const bool edge=i==0 || i+1==boundaries.size();
            markings.push_back({boundaries[i],edge,edge?MarkingType::solid:MarkingType::dashed});
        }
        road(boundaries,markings,QColor("#536c7c"));
    }
    for(const auto& connector:network_.connectors)
        road(connectorBoundaries(network_,connector),connectorMarkings(network_,connector),QColor("#386b78"));
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
