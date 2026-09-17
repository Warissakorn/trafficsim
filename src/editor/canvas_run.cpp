#include "canvas.hpp"
#include "../core/routes.hpp"
#include "../core/simulation.hpp"
#include <QGraphicsEllipseItem>
#include <QPainter>
#include <cmath>
namespace trafficsim {
void EditorCanvas::setRunNetwork(const Network& network) {
    runGeometry_.clear();runLevels_.clear();runStyles_.clear();
    // Keyed by SECTION, from the same table buildScenario compiled the scenario from, so every
    // segment a vehicle can be located on has geometry here. A lane with nothing attached to its
    // body is one section carrying the lane's own id, which is what the map held before.
    const auto table=runtimeSections(network);
    for(const auto& section:table.sections)for(const auto& l:network.links)if(l.id==section.linkId) {
        runGeometry_[section.id]=section.geometry;runLevels_[section.id]=l.level;runStyles_[section.id]=l.displayType;
    }
    for(const auto& c:network.connectors)for(const auto& p:connectorPaths(network,c)) {
        runGeometry_[p.id]=p.geometry;runLevels_[p.id]=c.level;runStyles_[p.id]=c.displayType;
    }
}
void EditorCanvas::setRunFrame(const SimState& frame) {runFrame_=frame;drawRunItems();}
// Clear all three together: marker() relies on the geometry, level and style maps holding
// the same keys, so dropping only the geometry would leave the others describing a run that
// no longer exists.
void EditorCanvas::clearRunFrame() {runFrame_={};runGeometry_.clear();runLevels_.clear();runStyles_.clear();drawRunItems();}
void EditorCanvas::drawRunItems() {
    for(auto* item:runItems_){scene_.removeItem(item);delete item;}runItems_.clear();
    if(!runFrame_.scenario)return;
    const double radius=3/std::abs(transform().m11());
    // style() already falls back to the default type, so resolve without at(): a segment
    // missing here must degrade like one missing from marker()'s geometry map, not throw
    // out of a paint callback.
    const auto styleOf=[&](const std::string& segment)->const DisplayType& {
        const auto it=runStyles_.find(segment);
        return style(it==runStyles_.end()?std::string{}:it->second);
    };
    const auto marker=[&](const std::string& segment,double station,QColor color,double size,int layer){
        const auto it=runGeometry_.find(segment);if(it==runGeometry_.end() || !levelVisible(runLevels_.at(segment)))return;
        const auto p=pointAlong(it->second,station);
        auto* item=scene_.addEllipse(p.x-size,p.y-size,size*2,size*2,QPen(Qt::NoPen),QBrush(color));
        item->setZValue(runLevels_.at(segment)*100.+layer);runItems_.push_back(item);
    };
    for(const auto& h:runFrame_.scenario->signalHeads)for(const auto& p:runFrame_.scenario->signalPrograms)if(p.id==h.programId) {
        const auto color=signalColorAt(p,runFrame_.time);
        marker(h.segmentId,h.position,color==SignalColor::red?QColor("#dc2626"):color==SignalColor::green?QColor("#16a34a"):QColor("#f59e0b"),radius*1.3,12);
    }
    for(const auto& v:runFrame_.vehicles) {
        const auto location=locateVehicle(*runFrame_.scenario,v);
        marker(location.segmentId,location.position,QColor(QString::fromStdString(styleOf(location.segmentId).vehicleColor)),radius,11);
    }
    viewport()->update();
}
}
