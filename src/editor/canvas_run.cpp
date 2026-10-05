#include "canvas.hpp"
#include "canvas_style.hpp"
#include "vehicle_shape.hpp"
#include "vehicle_pose.hpp"
#include "../core/routes.hpp"
#include "../core/simulation.hpp"
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <optional>
namespace trafficsim {
void EditorCanvas::setRunNetwork(const Network& network) {
    runAxlePaths_.clear();runLaneChangePaths_.clear();
    runGeometry_.clear();runEquations_.clear();runLevels_.clear();runStyles_.clear();
    // Keyed by SECTION, from the same table buildScenario compiled the scenario from, so every
    // segment a vehicle can be located on has geometry here. A lane with nothing attached to its
    // body is one section carrying the lane's own id, which is what the map held before.
    const auto table=runtimeSections(network);
    for(const auto& section:table.sections)for(const auto& l:network.links)if(l.id==section.linkId) {
        runGeometry_[section.id]=section.geometry;runLevels_[section.id]=l.level;runStyles_[section.id]=l.displayType;
    }
    for(const auto& c:network.connectors)for(const auto& p:connectorPaths(network,c)) {
        runGeometry_[p.id]=p.geometry;if(p.equation)runEquations_[p.id]=*p.equation;runLevels_[p.id]=c.level;runStyles_[p.id]=c.displayType;
    }
}
void EditorCanvas::setRunFrame(const SimState& frame) {
    // Scenarios are owned immutable snapshots, not mutable documents/revisions.
    if(frame.scenario!=runFrame_.scenario){runAxlePaths_.clear();runLaneChangePaths_.clear();}
    runFrame_=frame;drawRunItems();
}
// Clear all three together: marker() relies on the geometry, level and style maps holding
// the same keys, so dropping only the geometry would leave the others describing a run that
// no longer exists.
void EditorCanvas::clearRunFrame() {runAxlePaths_.clear();runLaneChangePaths_.clear();runFrame_={};runGeometry_.clear();runEquations_.clear();runLevels_.clear();runStyles_.clear();drawRunItems();}
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
        const auto curve=runEquations_.find(segment);
        const auto p=curve==runEquations_.end()?pointAlong(it->second,station):equationPoint(curve->second,equationParameter(curve->second,station));
        auto* item=scene_.addEllipse(p.x-size,p.y-size,size*2,size*2,QPen(Qt::NoPen),QBrush(color));
        item->setZValue(runLevels_.at(segment)*100.+layer);runItems_.push_back(item);
    };
    for(const auto& h:runFrame_.scenario->signalHeads)for(const auto& p:runFrame_.scenario->signalPrograms)if(p.id==h.programId) {
        const auto color=signalColorAt(p,runFrame_.time);
        marker(h.segmentId,h.position,color==SignalColor::red?canvasStyle::error():color==SignalColor::green?canvasStyle::ok():canvasStyle::warning(),radius*1.3,12);
    }
    // D97: each vehicle is its type's true length x width, front bumper at the located station.
    // Below a few pixels the size is floored so a vehicle never vanishes when zoomed far out, and
    // the windshield and cab gap appear only once the body is long enough on screen to show them.
    const double scale=std::abs(transform().m11());
    std::map<std::uint32_t,QPainterPath> shapes; // per type, shared by every item of that type
    std::map<std::uint32_t,std::vector<RoutePart>> partsOf;
    const auto partsForRoute=[&](std::uint32_t route)->const std::vector<RoutePart>& {
        auto parts=partsOf.find(route);
        if(parts==partsOf.end())parts=partsOf.emplace(route,routeParts(*runFrame_.scenario,runFrame_.scenario->routes.at(route))).first;
        return parts->second;
    };
    for(const auto& v:runFrame_.vehicles) {
        const auto& parts=partsForRoute(v.routeIndex);
        const auto location=locateOnParts(parts,v);
        const auto it=runGeometry_.find(location.segmentId);
        if(it==runGeometry_.end() || !levelVisible(runLevels_.at(location.segmentId)))continue;
        const auto& type=runFrame_.scenario->vehicleTypes.at(v.typeIndex);
        auto shape=shapes.find(v.typeIndex);
        if(shape==shapes.end())shape=shapes.emplace(v.typeIndex,vehicleShape(std::max(type.length,4/scale),
            std::max(type.width,2.5/scale),type.length*scale>=14)).first;
        const auto pose=vehiclePose(parts,v.distance,type.length,runGeometry_,runEquations_);
        if(!pose)continue;
        const auto key=std::pair{v.routeIndex,v.typeIndex};
        auto track=runAxlePaths_.find(key);
        if(track==runAxlePaths_.end())track=runAxlePaths_.try_emplace(key,parts,type,runGeometry_,runEquations_).first;
        auto axlePose=track->second.pose(v.distance,pose->front);
        if(!axlePose)continue;
        if(!v.laneChangeTrace.empty()) {
            auto changed=runLaneChangePaths_.find(v.id);
            if(changed!=runLaneChangePaths_.end() && !changed->second.matches(v)) {
                runLaneChangePaths_.erase(changed);changed=runLaneChangePaths_.end();
            }
            if(changed==runLaneChangePaths_.end())changed=runLaneChangePaths_.try_emplace(v.id,
                *runFrame_.scenario,v,runGeometry_,runEquations_).first;
            axlePose=changed->second.pose(v);
            if(!axlePose)continue;
        }
        const Point at=axlePose->front,heading=axlePose->heading;
        const auto colour=display_.vehicleColors.find(type.id);
        const auto axles=vehicleAxles(type);const double lever=axles.wheelbase+axles.frontOverhang;
        // Rear axle is local origin; even a minimum-size symbol keeps its nose at
        // the traffic front station. Lane-change guides also satisfy the rear rolling equation.
        auto body=shape->second;body.translate(lever,0);
        auto* item=new QGraphicsPathItem(body);
        item->setPen(Qt::NoPen);
        item->setBrush(QColor(QString::fromStdString(colour!=display_.vehicleColors.end()?colour->second:styleOf(location.segmentId).vehicleColor)));
        const double norm=std::hypot(heading.x,heading.y);
        item->setPos(at.x-lever*heading.x/norm,at.y-lever*heading.y/norm);
        item->setRotation(std::atan2(heading.y,heading.x)*180/std::numbers::pi);
        item->setData(3,lever); // local front-bumper coordinate, metres from rear axle
        item->setData(0,"run-vehicle");item->setData(1,QString::fromStdString(type.id));item->setData(2,QVariant::fromValue<qulonglong>(v.id));
        item->setZValue(runLevels_.at(location.segmentId)*100.+11);scene_.addItem(item);runItems_.push_back(item);
    }
    std::erase_if(runLaneChangePaths_,[&](const auto& entry) {
        return std::none_of(runFrame_.vehicles.begin(),runFrame_.vehicles.end(),
            [&](const Vehicle& v){return v.id==entry.first;});
    });
    // No viewport()->update() here: adding and removing items already invalidates their own
    // rectangles, and a whole-viewport repaint per frame was 7 of scenario-run-ui's 9.5 s.
}
}
