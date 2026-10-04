#include "canvas.hpp"
#include "canvas_style.hpp"
#include "vehicle_shape.hpp"
#include "../core/routes.hpp"
#include "../core/simulation.hpp"
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainter>
#include <cmath>
#include <limits>
#include <map>
#include <numbers>
#include <optional>
namespace trafficsim {
namespace {
// D102: how long a lane change is drawn, and how far off the lane it left may be found. Display
// values, not engine parameters and not measured against any driver.
constexpr double kLaneChangeShown=3,kLaneChangeReach=8,kLaneChangeYawSpeed=5;
}
void EditorCanvas::setRunNetwork(const Network& network) {
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
void EditorCanvas::setRunFrame(const SimState& frame) {runFrame_=frame;drawRunItems();}
// Clear all three together: marker() relies on the geometry, level and style maps holding
// the same keys, so dropping only the geometry would leave the others describing a run that
// no longer exists.
void EditorCanvas::clearRunFrame() {runFrame_={};runGeometry_.clear();runEquations_.clear();runLevels_.clear();runStyles_.clear();drawRunItems();}
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
    // D102, display only: for kLaneChangeShown seconds after a lane change the engine already made
    // (Vehicle::lastLaneChange), the body is drawn sliding from the lane it left to the one it is on,
    // eased in and out. The engine moved the vehicle in one tick and still does; nothing it decides
    // or any measurement reads changes. The lane it left is found as the nearest point on that
    // route's geometry, so no lane mapping is duplicated here. A second change inside the window
    // restarts the slide from the lane the second one left.
    std::map<std::uint32_t,std::vector<RoutePart>> partsOf;
    const auto onRoute=[&](std::uint32_t route,Point p)->std::optional<Point> {
        auto parts=partsOf.find(route);
        if(parts==partsOf.end())parts=partsOf.emplace(route,routeParts(*runFrame_.scenario,runFrame_.scenario->routes.at(route))).first;
        std::optional<Point> best;double bestDistance=std::numeric_limits<double>::infinity();
        for(const auto& part:parts->second) {
            const auto g=runGeometry_.find(part.segmentId);if(g==runGeometry_.end())continue;
            const auto curve=runEquations_.find(part.segmentId);
            const auto q=curve==runEquations_.end()?pointAlong(g->second,stationOfClosestPoint(g->second,p)):
                equationPoint(curve->second,equationParameter(curve->second,equationClosestStation(curve->second,p)));
            if(const double d=std::hypot(q.x-p.x,q.y-p.y);d<bestDistance){bestDistance=d;best=q;}
        }
        return best;
    };
    for(const auto& v:runFrame_.vehicles) {
        const auto location=locateVehicle(*runFrame_.scenario,v);
        const auto it=runGeometry_.find(location.segmentId);
        if(it==runGeometry_.end() || !levelVisible(runLevels_.at(location.segmentId)))continue;
        const auto& type=runFrame_.scenario->vehicleTypes.at(v.typeIndex);
        auto shape=shapes.find(v.typeIndex);
        if(shape==shapes.end())shape=shapes.emplace(v.typeIndex,vehicleShape(std::max(type.length,4/scale),
            std::max(type.width,2.5/scale),type.length*scale>=14)).first;
        const auto& g=it->second;const double station=location.position;
        const auto curve=runEquations_.find(location.segmentId);
        const auto point=[&](double at){return curve==runEquations_.end()?pointAlong(g,at):equationPoint(curve->second,equationParameter(curve->second,at));};
        const auto front=point(station);
        // The body is the chord from rear to front, as a real vehicle sits across a curve; a vehicle
        // whose rear is still on the previous segment takes the tangent at its front instead.
        Point heading=curve==runEquations_.end()?directionAlong(g,station,true):equationDerivative(curve->second,equationParameter(curve->second,station));
        if(station>=type.length) {
            const auto rear=point(station-type.length);
            if(std::hypot(front.x-rear.x,front.y-rear.y)>1e-6)heading={front.x-rear.x,front.y-rear.y};
        }
        Point at=front;
        if(const auto& last=v.lastLaneChange) {
            const double f=static_cast<double>(runFrame_.tick-last->tick)*runFrame_.scenario->timeStep/kLaneChangeShown;
            const double norm=std::hypot(heading.x,heading.y);
            // How far sideways the lane it left lies from the front: only the sideways part, since
            // past a stub's dead end the nearest point is behind the vehicle.
            const Point along{heading.x/norm,heading.y/norm},side{-along.y,along.x};
            std::optional<Point> left;
            if(f<1 && norm>1e-9)left=onRoute(last->fromRoute,front);
            const double offset=left?(left->x-front.x)*side.x+(left->y-front.y)*side.y:0;
            if(left && std::abs(offset)<=kLaneChangeReach) {
                const double rest=offset*(1-f*f*(3-2*f)); // smoothstep: leaves and arrives along the lane
                at={front.x+side.x*rest,front.y+side.y*rest};
                // The nose points along the path: sideways speed over forward speed, floored so a
                // queued changer does not swing across the lane.
                const double sidewaysSpeed=-offset*6*f*(1-f)/kLaneChangeShown,forward=std::max(v.speed,kLaneChangeYawSpeed);
                heading={along.x*forward+side.x*sidewaysSpeed,along.y*forward+side.y*sidewaysSpeed};
            }
        }
        const auto colour=display_.vehicleColors.find(type.id);
        auto* item=new QGraphicsPathItem(shape->second);
        item->setPen(Qt::NoPen);
        item->setBrush(QColor(QString::fromStdString(colour!=display_.vehicleColors.end()?colour->second:styleOf(location.segmentId).vehicleColor)));
        item->setPos(at.x,at.y);item->setRotation(std::atan2(heading.y,heading.x)*180/std::numbers::pi);
        item->setData(0,"run-vehicle");item->setData(1,QString::fromStdString(type.id));item->setData(2,QVariant::fromValue<qulonglong>(v.id));
        item->setZValue(runLevels_.at(location.segmentId)*100.+11);scene_.addItem(item);runItems_.push_back(item);
    }
    // No viewport()->update() here: adding and removing items already invalidates their own
    // rectangles, and a whole-viewport repaint per frame was 7 of scenario-run-ui's 9.5 s.
}
}
