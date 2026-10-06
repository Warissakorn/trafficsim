#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QPainterPathStroker>
#include "../project/demand_paths.hpp"
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
    for(const auto& hit:hits) {
        if(isLink(hit.first))return hit.first;
        for(const auto& connector:document_->network.connectors)if(connector.id==hit.first)return hit.first;
    }
    return {};
}
std::vector<std::string> EditorCanvas::routeDraftWith(const std::string& target) const {
    if(!document_ || target.empty())return {};
    if(routeDraft_.empty())return {target};
    return routeChainTo(document_->network,routeDraft_,target);
}
void EditorCanvas::startRouteDraft(const std::string& objectId) {
    if(!document_ || objectId.empty())return;
    if(routeLaneChains(document_->network,{objectId}).empty())return;
    routeStartPosition_.reset();routeDraft_={objectId};routePreview_=routeDraft_;routeTracing_=true;hoverSegment_.clear();
    if(routeDraftChanged)routeDraftChanged();
    redraw();
}
std::pair<std::string,std::string> EditorCanvas::demandObjectAt(QPoint viewportPosition) const {
    std::pair<std::string,std::string> route;
    for(const auto* item:items(viewportPosition)) {
        const auto kind=item->data(0).toString();
        const auto id=item->data(1).toString().toStdString();
        if(id.empty())continue; // An unfinished Route is not an editable stored object.
        if(kind=="input-marker" || kind=="input-volume")return {"input",item->data(1).toString().toStdString()};
        if(kind=="decision-marker")return {"decision",item->data(1).toString().toStdString()};
        if(kind=="route-overlay" || kind=="route-start" || kind=="route-end")route={"route",id};
    }
    return route;
}
bool EditorCanvas::demandPress(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    const bool trace=tool_==Tool::route && e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier;
    const bool legacy=e->button()==Qt::RightButton && (e->modifiers()&Qt::ControlModifier);
    if(!trace && !legacy)return false;
    const auto target=objectAt(world(e->pos(),false));
    if(target.empty()){clearSelection(false);return true;}
    if(tool_==Tool::input) {
        if(isLink(target)) {if(inputPlaced)inputPlaced(target);}else reject();
        return true;
    }
    if(trace) {
        if(routeDraft_.empty()){
            clearSelection(false);startRouteDraft(target);
            for(const auto& link:document_->network.links)if(link.id==target) {
                routeStartPosition_=stationOfClosestPoint(link.geometry,world(e->pos(),false));
                if(document_->definition)for(const auto& decision:document_->definition->routingDecisions)
                    if(decision.linkId==target && decision.position &&
                       std::abs(*decision.position-*routeStartPosition_)*std::abs(transform().m11())<=10)routeStartPosition_=decision.position;
            }
            redraw();return true;
        }
        // Re-read the click: Qt can coalesce the final hover move.
        const auto base=routePreview_.empty()?routeDraft_:routePreview_;
        auto candidate=base;
        const auto previous=std::find(candidate.begin(),candidate.end(),target);
        if(previous!=candidate.end())candidate.erase(previous+1,candidate.end());
        else {
            const auto chain=routeChainTo(document_->network,base,target);
            if(chain.empty()){reject();return true;}
            candidate.insert(candidate.end(),chain.begin(),chain.end());
        }
        if(routeLaneChains(document_->network,candidate).empty()){reject();return true;}
        routeDraft_=std::move(candidate);routePreview_.clear();commitRouteDraft();return true;
    }
    const auto chain=routeDraftWith(target);
    if(chain.empty()){reject();return true;}
    routeTracing_=false;routeStartPosition_.reset();
    routeDraft_.insert(routeDraft_.end(),chain.begin(),chain.end());routePreview_=routeDraft_;
    if(routeDraftChanged)routeDraftChanged();
    redraw();return true;
}
bool EditorCanvas::isLink(const std::string& objectId) const {
    if(!document_ || objectId.empty())return false;
    for(const auto& link:document_->network.links)if(link.id==objectId)return true;
    return false;
}
bool EditorCanvas::demandHover(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    const auto hovered=objectAt(world(e->pos(),false));
    // Path finding and scene rebuilds happen on object changes, not on every mouse pixel.
    if(hovered==hoverSegment_)return true;
    hoverSegment_=hovered;
    if(tool_==Tool::input)hoverReachable_=isLink(hovered);
    else if(routeDraft_.empty())hoverReachable_=!routeDraftWith(hovered).empty();
    else {
        const auto base=routeTracing_ && !routePreview_.empty()?routePreview_:routeDraft_;
        auto candidate=base;
        const auto previous=std::find(candidate.begin(),candidate.end(),hovered);
        if(previous!=candidate.end())candidate.erase(previous+1,candidate.end());
        else {
            const auto chain=routeChainTo(document_->network,base,hovered);
            if(!chain.empty())candidate.insert(candidate.end(),chain.begin(),chain.end());
            else candidate.clear();
        }
        hoverReachable_=!candidate.empty() && !routeLaneChains(document_->network,candidate).empty();
        if(hoverReachable_)routePreview_=std::move(candidate);
    }
    redraw();return true;
}
void EditorCanvas::commitRouteDraft() {
    if(routeDraft_.empty())return;
    const auto segments=routeTracing_ && !routePreview_.empty()?routePreview_:routeDraft_;
    const auto position=routeStartPosition_;
    clearRouteDraft();
    if(positionedRouteCommitted)positionedRouteCommitted(segments,position);
    else if(routeDraftCommitted)routeDraftCommitted(segments);
    redraw();
}
void EditorCanvas::dropLastRouteSegment() {
    auto& path=routeTracing_?routePreview_:routeDraft_;
    if(path.empty())return;
    path.pop_back();
    if(routeTracing_ && path.empty()){routeDraft_.clear();routeStartPosition_.reset();}
    else if(!routeTracing_)routePreview_=routeDraft_;
    hoverSegment_.clear();
    if(routeDraftChanged)routeDraftChanged();
    redraw();
}
void EditorCanvas::clearRouteDraft() {
    routeDraft_.clear();routePreview_.clear();routeStartPosition_.reset();routeTracing_=false;hoverSegment_.clear();
    if(routeDraftChanged)routeDraftChanged();
}
void EditorCanvas::reject() {
    if(creationRejected)creationRejected();
    redraw();
}
void EditorCanvas::setHighlightedRoute(std::string id) {
    if(highlightedRoute_==id)return;
    highlightedRoute_=std::move(id);redraw();
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
    // Inputs use the same crossbar as route endpoints and signal heads.
    if(document_->definition)for(const auto& input:document_->definition->inputs) {
        const auto& def=*document_->definition;
        std::string origin=input.linkId;
        std::vector<std::string> segments;
        for(const auto& route:def.routes)if(route.id==input.routeId)segments=route.segmentIds;
        if(origin.empty() && !segments.empty())origin=segments.front();
        if(origin.empty())for(const auto& decision:def.routingDecisions)if(decision.id==input.routingDecisionId) {
            origin=decision.linkId;
            if(origin.empty())for(const auto& choice:decision.routes)
                for(const auto& route:def.routes)if(route.id==choice.routeId && !route.segmentIds.empty())origin=route.segmentIds.front();
        }
        const auto whole=objectCrossbar(document_->network,origin,false);
        if(!whole || !levelVisible(whole->level))continue;
        std::vector<RoadCrossbar> bars;
        for(const auto& link:document_->network.links)if(link.id==origin) {
            std::vector<bool> served(link.lanes.size(),segments.empty());
            if(!segments.empty())for(const auto& chain:routeLaneFamily(document_->network,segments))
                if(chain.lane<served.size())served[chain.lane]=true;
            const auto policy=inputLanePolicy(document_->network,def,input);
            if(policy.acceptsShares && input.laneShares.size()==served.size())
                for(std::size_t k=0;k<served.size();++k)served[k]=served[k] && input.laneShares[k]>0;
            for(std::size_t k=0;k<served.size();) {
                if(!served[k]){++k;continue;}
                const auto first=k;while(k<served.size() && served[k])++k;
                const auto a=laneBoundaryGeometry(link,first,document_->network.drivingSide);
                const auto b=laneBoundaryGeometry(link,k,document_->network.drivingSide);
                const auto centre=linkCentreline(link,document_->network.drivingSide);
                if(const auto bar=roadCrossbar(centre,a,b,0,link.level))bars.push_back(*bar);
            }
        }
        if(!isLink(origin))bars.push_back(*whole);
        if(bars.empty())continue;
        for(const auto& bar:bars) {
            auto* marker=new RoadCrossbarItem(bar,canvasStyle::active(),scale);
            scene_.addItem(marker);marker->setZValue(200005);
            marker->setData(0,QStringLiteral("input-marker"));marker->setData(1,QString::fromStdString(input.id));
        }
        const auto& at=bars.back().second;
        const auto value=input.intervals.empty()?input.vehiclesPerHour:0;
        const auto text=input.intervals.empty()?editorDesign::formatValue(value,0,inputRateLabel_)
            :inputPeriodsLabel_.arg(static_cast<int>(input.intervals.size()));
        auto* label=scene_.addSimpleText(text,editorDesign::numericFont());
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setBrush(editorDesign::role(QPalette::Text));
        label->setPos(at.x+editorDesign::space2/scale,at.y);label->setZValue(200006);
        label->setData(0,QStringLiteral("input-volume"));label->setData(1,QString::fromStdString(input.id));
    }
    drawPositionedDecisions();
    // M2.1.1: a routing decision placed on a Link draws as a diamond a little way along it, where
    // Vissim draws its decision marker; clicking it opens the decision.
    if(document_->definition)for(const auto& decision:document_->definition->routingDecisions) {
        if(decision.linkId.empty() || decision.position)continue;
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
    if(document_->definition && !highlightedRoute_.empty())
        for(const auto& route:document_->definition->routes)if(route.id==highlightedRoute_)
            drawRouteOverlay(route.segmentIds,route.id,false);
    if(!routeDraft_.empty())drawRouteOverlay(routePreview_.empty()?routeDraft_:routePreview_,{},true);
}
void EditorCanvas::drawRouteOverlay(const std::vector<std::string>& segments,const std::string& id,bool preview) {
    if(segments.empty())return;
    const auto& n=document_->network;
    auto position=preview?routeStartPosition_:std::optional<double>{};
    if(!preview && document_->definition)for(const auto& decision:document_->definition->routingDecisions)
        for(const auto& entry:decision.routes)if(entry.routeId==id && decision.position)position=decision.position;
    // Keep D96's compiled spans: tint never extends upstream of a mid-Link arrival.
    // Clip a lane-centre corridor to real road surfaces rather than inventing another road.
    QPainterPath roads;
    for(const auto& object:segments) {
        const auto level=objectLevel(object);
        if(level && levelVisible(*level))roads=roads.united(objectShape(object));
    }
    QPainterPath corridors;
    double width=0;
    for(const auto& l:n.links)for(const auto& lane:l.lanes)width=std::max(width,lane.width);
    for(const auto& c:n.connectors)for(const auto w:c.laneWidths)width=std::max(width,w);
    QPainterPathStroker stroke;stroke.setWidth(width);stroke.setCapStyle(Qt::FlatCap);
    for(auto geometry:routeGeometries(n,segments)) {
        if(position && !geometry.empty())for(const auto& link:n.links)if(link.id==segments.front())
            for(const auto& lane:link.lanes) {
                const auto origin=laneGeometry(link,lane.id,n.drivingSide);
                if(origin.empty() || std::hypot(geometry.front().x-origin.front().x,geometry.front().y-origin.front().y)>1e-7)continue;
                geometry=polylineSpan(geometry,matchedStation(link.geometry,origin,*position),polylineLength(geometry));break;
            }
        corridors=corridors.united(stroke.createStroke(polylinePath(geometry)));
    }
    QColor fill=canvasStyle::active();fill.setAlpha(preview?24:40);
    auto* overlay=scene_.addPath(roads.intersected(corridors),QPen(Qt::NoPen),QBrush(fill));
    overlay->setZValue(200004);overlay->setData(0,preview?QStringLiteral("route-draft"):QStringLiteral("route-overlay"));
    overlay->setData(1,QString::fromStdString(id));
    const auto mark=[&](const std::string& object,bool end) {
        auto bar=objectCrossbar(n,object,end);
        if(!end && position)for(const auto& link:n.links)if(link.id==object) {
            const auto axis=linkCentreline(link,n.drivingSide);
            bar=roadCrossbar(axis,laneBoundaryGeometry(link,0,n.drivingSide),laneBoundaryGeometry(link,link.lanes.size(),n.drivingSide),
                matchedStation(link.geometry,axis,*position),link.level);
        }
        if(!bar || !levelVisible(bar->level))return;
        auto* line=new RoadCrossbarItem(*bar,canvasStyle::active(),std::abs(transform().m11()),preview && end);
        scene_.addItem(line);line->setZValue(200007);
        line->setData(0,end?QStringLiteral("route-end"):QStringLiteral("route-start"));line->setData(1,QString::fromStdString(id));
    };
    mark(segments.front(),false);mark(segments.back(),true);
}
}
