#include "canvas.hpp"
#include "../core/routes.hpp"
#include "../core/simulation.hpp"
#include <QPainter>
#include <algorithm>
#include <cmath>
namespace trafficsim {
void EditorCanvas::setRunNetwork(const Network& network) {
    runGeometry_.clear();
    for(const auto& l:network.links)for(const auto& lane:l.lanes)
        runGeometry_[lane.id]=laneGeometry(l,lane.id,network.drivingSide);
    for(const auto& c:network.connectors)runGeometry_[c.id]=c.geometry;
}
void EditorCanvas::setRunFrame(const SimState& frame) {runFrame_=frame;viewport()->update();}
void EditorCanvas::clearRunFrame() {runFrame_={};runGeometry_.clear();viewport()->update();}
void EditorCanvas::drawForeground(QPainter* painter,const QRectF&) {
    if(!runFrame_.scenario)return;
    const double radius=3/std::abs(transform().m11());
    painter->setPen(Qt::NoPen);
    for(const auto& h:runFrame_.scenario->signalHeads) {
        const auto it=runGeometry_.find(h.segmentId);if(it==runGeometry_.end())continue;
        for(const auto& p:runFrame_.scenario->signalPrograms)if(p.id==h.programId) {
            const auto color=signalColorAt(p,runFrame_.time);
            painter->setBrush(color==SignalColor::red?QColor("#dc2626"):color==SignalColor::green?QColor("#16a34a"):QColor("#f59e0b"));
            const auto pos=pointAlong(it->second,h.position);painter->drawEllipse(QPointF(pos.x,pos.y),radius*1.3,radius*1.3);
        }
    }
    painter->setBrush(QColor("#facc15"));
    for(const auto& v:runFrame_.vehicles) {
        const auto location=locateVehicle(*runFrame_.scenario,v);
        const auto it=runGeometry_.find(location.segmentId);if(it==runGeometry_.end())continue;
        const auto pos=pointAlong(it->second,location.position);
        painter->drawEllipse(QPointF(pos.x,pos.y),radius,radius);
    }
}
}
