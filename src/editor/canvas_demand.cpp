#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
QPainterPath polylinePath(const std::vector<Point>& points) {
    QPainterPath path;
    if(points.empty())return path;
    path.moveTo(points.front().x,points.front().y);
    for(std::size_t i=1;i<points.size();++i)path.lineTo(points[i].x,points[i].y);
    return path;
}
}
// What a click means to the route and input tools: the Link or Connector under the pointer,
// which is what a route names. hitObjects already answers that question for every other tool,
// so the pick, the selection outline and the route agree about what was clicked.
std::string EditorCanvas::objectAt(Point p) const {
    if(!document_)return {};
    const auto hits=hitObjects(p);
    return hits.empty()?std::string{}:hits.front().first;
}
std::vector<std::string> EditorCanvas::routeDraftWith(const std::string& target) const {
    if(!document_ || target.empty())return {};
    if(routeDraft_.empty())return {target};
    return routeChainTo(document_->network,routeDraft_,target);
}
void EditorCanvas::startRouteDraft(const std::string& objectId) {
    if(!document_ || objectId.empty())return;
    if(routeLaneChains(document_->network,{objectId}).empty())return;
    routeDraft_={objectId};hoverSegment_.clear();redraw();
}
std::pair<std::string,std::string> EditorCanvas::demandObjectAt(QPoint viewportPosition) const {
    for(const auto* item:items(viewportPosition)) {
        const auto kind=item->data(0).toString();
        if(kind=="input-marker")return {"input",item->data(1).toString().toStdString()};
        if(kind=="decision-marker")return {"decision",item->data(1).toString().toStdString()};
        if(kind=="route-overlay")return {"route",item->data(1).toString().toStdString()};
    }
    return {};
}
bool EditorCanvas::demandPress(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    // Left-click, or Vissim's Ctrl+right-click on the link the decision sits on.
    const bool ctrlRight=e->button()==Qt::RightButton && (e->modifiers()&Qt::ControlModifier);
    if(e->button()!=Qt::LeftButton && !ctrlRight)return false;
    const auto p=world(e->pos(),false);
    const auto target=objectAt(p);
    if(target.empty()){clearSelection(false);return true;}
    if(tool_==Tool::input) {
        // A vehicle input is placed on a LINK, the way Vissim places one, and its volume is the
        // Link's total: the compiler divides it across the lanes the route actually reaches.
        if(isLink(target)) {if(inputPlaced)inputPlaced(target);}
        else reject();
        return true;
    }
    const auto chain=routeDraftWith(target);
    if(chain.empty()) {reject();return true;}
    // One click can append a whole chain: that is what makes "click the destination" work.
    routeDraft_.insert(routeDraft_.end(),chain.begin(),chain.end());
    redraw();
    return true;
}
bool EditorCanvas::isLink(const std::string& objectId) const {
    if(!document_ || objectId.empty())return false;
    for(const auto& link:document_->network.links)if(link.id==objectId)return true;
    return false;
}
bool EditorCanvas::demandHover(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    const auto p=world(e->pos(),false);
    const auto hovered=objectAt(p);
    const bool reachable=tool_==Tool::input?isLink(hovered):!routeDraftWith(hovered).empty();
    if(hovered!=hoverSegment_ || reachable!=hoverReachable_ || !routeDraft_.empty()) {
        hoverSegment_=hovered;hoverReachable_=reachable;hoverPoint_=p;redraw();
    }
    return true;
}
void EditorCanvas::commitRouteDraft() {
    if(routeDraft_.empty())return;
    const auto segments=routeDraft_;
    routeDraft_.clear();hoverSegment_.clear();
    if(routeDraftCommitted)routeDraftCommitted(segments);
    redraw();
}
void EditorCanvas::dropLastRouteSegment() {
    if(routeDraft_.empty())return;
    routeDraft_.pop_back();redraw();
}
void EditorCanvas::clearRouteDraft() {
    if(routeDraft_.empty() && hoverSegment_.empty())return;
    routeDraft_.clear();hoverSegment_.clear();
}
void EditorCanvas::reject() {
    if(creationRejected)creationRejected();
    redraw();
}
void EditorCanvas::setHighlightedRoute(std::string id) {
    if(highlightedRoute_==id)return;
    highlightedRoute_=std::move(id);redraw();
}
namespace {
QPen routePen(QColor colour,double width,bool draft) {
    QPen pen(colour,width);pen.setCosmetic(true);pen.setDashPattern({6,4});
    if(draft)pen.setDashPattern({3,3});
    return pen;
}
}
void EditorCanvas::drawDemandOverlay() {
    if(!document_)return;
    const double scale=std::abs(transform().m11());
    // Hover halo: nearestLane used to pick in silence, so the author learned where a click
    // landed only after the dialog opened.
    if((tool_==Tool::route || tool_==Tool::input) && !hoverSegment_.empty()) {
        const auto geometry=objectGeometry(document_->network,hoverSegment_);
        if(geometry.size()>1) {
            const QColor colour=hoverReachable_?canvasStyle::hover():canvasStyle::error();
            QPen outline(colour,1.5);outline.setCosmetic(true);
            QColor fill=colour;fill.setAlpha(24);
            auto* item=scene_.addPath(objectShape(hoverSegment_),outline,QBrush(fill));
            item->setZValue(200003);item->setData(0,QStringLiteral("demand-hover"));
            item->setData(1,QString::fromStdString(hoverSegment_));
            item->setData(2,hoverReachable_);
        }
    }
    // Every vehicle input draws where its traffic enters, with the volume beside it.
    if(document_->definition)for(const auto& input:document_->definition->inputs) {
        std::vector<std::string> segments;
        for(const auto& route:document_->definition->routes)if(route.id==input.routeId)segments=route.segmentIds;
        // M2.1.1: a routeless input enters on its own Link, across all of that Link's lanes.
        std::vector<std::vector<Point>> chains;
        if(!input.linkId.empty()) {
            chains={objectGeometry(document_->network,input.linkId)};
            for(const auto& link:document_->network.links)if(link.id==input.linkId)
                chains.resize(std::max<std::size_t>(1,link.lanes.size()),chains.front());
        } else if(!segments.empty()) chains=routeGeometries(document_->network,segments);
        if(chains.empty() || chains.front().size()<2)continue;
        const auto& geometry=chains.front();
        const bool lit=input.routeId==highlightedRoute_;
        const double r=7/scale;
        const auto at=pointAlong(geometry,0),ahead=pointAlong(geometry,std::min(0.5,polylineLength(geometry)));
        const double angle=std::atan2(ahead.y-at.y,ahead.x-at.x);
        QPolygonF chevron;
        for(double offset:{0.0,2.4,-2.4})chevron<<QPointF(at.x+r*std::cos(angle+offset),at.y+r*std::sin(angle+offset));
        QPen pen(canvasStyle::active(),lit?2:1);pen.setCosmetic(true);
        QColor markerFill=canvasStyle::active();markerFill.setAlpha(lit?72:28);
        auto* marker=scene_.addPolygon(chevron,pen,QBrush(markerFill));
        marker->setZValue(200005);marker->setData(0,QStringLiteral("input-marker"));
        marker->setData(1,QString::fromStdString(input.id));
        // The authored number is the Link total; the lanes it splits across is what the author
        // needs to see beside it, because that is what the run actually receives.
        auto* label=scene_.addSimpleText(chains.size()>1
            ?QString::number(input.vehiclesPerHour,'f',0)+" / "+QString::number(chains.size())
            :QString::number(input.vehiclesPerHour,'f',0));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setBrush(editorDesign::role(QPalette::Text));label->setPos(at.x+r,at.y+r);label->setZValue(200006);
        label->setData(0,QStringLiteral("input-volume"));
    }
    // M2.1.1: a routing decision placed on a Link draws as a diamond a little way along it, where
    // Vissim draws its decision marker; clicking it opens the decision.
    if(document_->definition)for(const auto& decision:document_->definition->routingDecisions) {
        if(decision.linkId.empty())continue;
        const auto geometry=objectGeometry(document_->network,decision.linkId);
        if(geometry.size()<2)continue;
        const double r=6/scale;
        const auto at=pointAlong(geometry,std::min(12.0,polylineLength(geometry)/2));
        QPolygonF diamond;diamond<<QPointF(at.x+r,at.y)<<QPointF(at.x,at.y+r)<<QPointF(at.x-r,at.y)<<QPointF(at.x,at.y-r);
        QPen pen(canvasStyle::active(),1);pen.setCosmetic(true);
        auto* marker=scene_.addPolygon(diamond,pen,QBrush(editorDesign::role(QPalette::Midlight)));
        marker->setZValue(200005);marker->setData(0,QStringLiteral("decision-marker"));
        marker->setData(1,QString::fromStdString(decision.id));
        auto* label=scene_.addSimpleText(QString::fromStdString(decision.name.empty()?decision.id:decision.name));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setBrush(editorDesign::role(QPalette::Text));label->setPos(at.x+r,at.y-r);label->setZValue(200006);
        label->setData(0,QStringLiteral("decision-label"));
    }
    // A committed route draws only while it is selected, so the canvas does not silt up.
    if(document_->definition && !highlightedRoute_.empty())
        for(const auto& route:document_->definition->routes)if(route.id==highlightedRoute_) {
            // One line per lane the route carries, each clipped to the sections the vehicles
            // travel -- drawing the whole lane put a line upstream of a mid-body arrival, which
            // read on screen as a route running against the traffic on that Link.
            for(const auto& geometry:routeGeometries(document_->network,route.segmentIds)) {
                auto* item=scene_.addPath(polylinePath(geometry),routePen(canvasStyle::active(),2,false));
                item->setZValue(200004);item->setData(0,QStringLiteral("route-overlay"));
                item->setData(1,QString::fromStdString(route.id));
                drawRouteArrows(geometry,canvasStyle::active());
            }
        }
    if(routeDraft_.empty())return;
    const auto drawn=routeGeometries(document_->network,routeDraft_);
    for(const auto& geometry:drawn) {
        if(geometry.size()<2)continue;
        auto* item=scene_.addPath(polylinePath(geometry),routePen(canvasStyle::active(),2,true));
        item->setZValue(200007);item->setData(0,QStringLiteral("route-draft"));
        drawRouteArrows(geometry,canvasStyle::active());
    }
    // The rubber band leaves the head of the draft -- the last lane it reached -- and says,
    // before the click, whether the click will be taken.
    if(!drawn.empty() && !drawn.front().empty()) {
        const QColor colour=hoverSegment_.empty()||hoverReachable_?canvasStyle::active():canvasStyle::error();
        QPen pen(colour,1,Qt::DashLine);pen.setCosmetic(true);
        const auto tail=drawn.front().back();
        auto* band=scene_.addLine(tail.x,tail.y,hoverPoint_.x,hoverPoint_.y,pen);
        band->setZValue(200007);band->setData(0,QStringLiteral("route-band"));
        band->setData(2,hoverSegment_.empty()||hoverReachable_);
    }
}
void EditorCanvas::drawRouteArrows(const std::vector<Point>& geometry,QColor colour) {
    const double length=polylineLength(geometry);
    if(!(length>0))return;
    // Spaced along the route rather than one per segment: a long lane needs more than one
    // arrow to read as a direction, and a short connector needs none of its own.
    for(double station=length/8;station<length;station+=length/4) {
        const auto at=pointAlong(geometry,station);
        const auto ahead=pointAlong(geometry,std::min(length,station+0.05));
        addArrowhead(at,std::atan2(ahead.y-at.y,ahead.x-at.x),6,colour,200008,QStringLiteral("route-arrow"));
    }
}
}
