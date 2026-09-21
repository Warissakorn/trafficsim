#include "canvas.hpp"
#include <QGraphicsItem>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace trafficsim {
std::vector<EditorCanvas::LaneHandle> EditorCanvas::laneHandles() const {
    if(!document_ || tool_!=Tool::select || selection_.size()!=1)return {};
    const auto side=document_->network.drivingSide;
    const auto handle=[&](const Link& link,const LaneReference& ref,int count,int kind) {
        auto first=std::find_if(link.lanes.begin(),link.lanes.end(),[&](const auto& l){return l.id==ref.laneId;});
        const int available=static_cast<int>(std::distance(first,link.lanes.end()));
        const bool leading=kind>4;const int base=leading?kind-4:kind;
        const auto& lane=*(first+(leading?0:count-1));
        const auto geometry=laneGeometry(link,lane.id,side);
        const double at=matchedStation(link.geometry,geometry,
            ref.station.value_or(base==1?polylineLength(link.geometry):0.));
        const auto p=pointAlong(geometry,at);
        const auto tangent=directionAlong(geometry,at,base==1);
        const double sign=(side==DrivingSide::left?1.:-1.)*(leading?-1.:1.);
        const Point direction{sign*tangent.y,-sign*tangent.x};
        // Keep resize handles visibly outside geometry handles at every zoom level, and
        // remember where the road edge is so the grip can be drawn attached to it.
        const double edge=lane.width/2,offset=edge+16/std::abs(transform().m11());
        return LaneHandle{{p.x+direction.x*offset,p.y+direction.y*offset},{p.x+direction.x*edge,p.y+direction.y*edge},
                          direction,lane.width,kind,count,leading?static_cast<int>(std::distance(link.lanes.begin(),first))+count:available};
    };
    if(const auto* link=selectedLink();link && levelVisible(link->level)) {
        // Mid-link, clear of the endpoint grips. A link station says that once for both tabs,
        // whichever lane `handle` measures the offset from.
        const LaneReference middle{link->id,link->lanes.front().id,polylineLength(link->geometry)/2};
        auto h=handle(*link,middle,static_cast<int>(link->lanes.size()),4);
        auto other=handle(*link,middle,static_cast<int>(link->lanes.size()),8);
        h.maximum=other.maximum=12;return {h,other};
    }
    if(const auto* connector=selectedConnector();connector && levelVisible(connector->level)) {
        const Link *from=nullptr,*to=nullptr;
        for(const auto& link:document_->network.links) {
            if(link.id==connector->from.linkId)from=&link;
            if(link.id==connector->to.linkId)to=&link;
        }
        if(!from || !to)return {};
        std::vector<LaneHandle> result;
        const auto paths=connectorPaths(document_->network,*connector);
        const auto boundaries=connectorBoundaries(document_->network,*connector);
        const auto widths=connectorLaneWidths(document_->network,*connector);
        for(bool leading:{false,true}) {
            const int extra=leading?4:0;
            auto a=handle(*from,connector->from,connector->fromLaneCount,1+extra);
            auto b=handle(*to,connector->to,connector->toLaneCount,2+extra);
            const auto& outer=(leading?paths.front():paths.back()).geometry;const double length=polylineLength(outer);
            const auto tangent=directionAlong(outer,length/2,false);
            const double sign=(side==DrivingSide::left?1.:-1.)*(leading?-1.:1.);
            const Point direction{sign*tangent.y,-sign*tangent.x};
            const auto index=leading?0:paths.size()-1;
            const auto& boundary=leading?boundaries.front():boundaries.back();
            const auto anchor=polylineLength(boundary)>0
                ?pointAlong(boundary,matchedStation(outer,boundary,length/2)):boundary.front();
            const double width=(widths.source[index]+widths.target[index])/2,offset=16/std::abs(transform().m11());
            LaneHandle body{{anchor.x+direction.x*offset,anchor.y+direction.y*offset},anchor,
                            direction,width,3+extra,std::max(a.count,b.count),std::min(a.maximum,b.maximum)};
            result.insert(result.end(),{a,b,body});
        }
        return result;
    }
    return {};
}
bool EditorCanvas::startLaneResize(QPoint position) {
    auto handles=laneHandles();int best=10;std::optional<LaneHandle> picked;
    for(const auto& h:handles) {
        const int distance=(mapFromScene(h.position.x,h.position.y)-position).manhattanLength();
        if(distance<best){best=distance;picked=h;}
    }
    if(!picked)return false;
    laneResize_=picked;dragStart_=world(position,false);
    if(picked->kind==4 || picked->kind==8)previewLinkCount_=picked->count;
    else {
        const auto* c=selectedConnector();previewFromCount_=c->fromLaneCount;previewToCount_=c->toLaneCount;
        rangeCorner_=picked->kind;
    }
    return true;
}
void EditorCanvas::updateLaneResize(QPoint position) {
    if(!laneResize_)return;
    const auto& h=*laneResize_;const auto p=world(position,false);
    const double lateral=(p.x-dragStart_.x)*h.direction.x+(p.y-dragStart_.y)*h.direction.y;
    const int kind=(h.kind-1)%4+1;
    if(kind==3 && (std::round(lateral/h.width)==0 || (h.count>h.maximum && lateral>0))) {
        const auto* c=selectedConnector();previewFromCount_=c->fromLaneCount;previewToCount_=c->toLaneCount;redraw();return;
    }
    const int count=static_cast<int>(std::clamp(h.count+std::round(lateral/h.width),1.,static_cast<double>(h.maximum)));
    if(kind==4)previewLinkCount_=count;
    else if(kind==1)previewFromCount_=count;
    else if(kind==2)previewToCount_=count;
    else previewFromCount_=previewToCount_=count;
    redraw();
}
void EditorCanvas::drawLaneHandles() {
    // Vissim draws a lane grip as a tab on the edge of the carriageway carrying the lane
    // count, not as a loose dot with a number beside it. One shape says what it edits and
    // what the result will be, which is why the number lives inside the grip.
    const double scale=std::abs(transform().m11()),half=9/scale,corner=3/scale;
    for(auto h:laneHandles()) {
        int count=h.count;
        const bool held=laneResize_ && laneResize_->kind==h.kind;
        if(laneResize_) {
            const int kind=(h.kind-1)%4+1;
            count=kind==4?previewLinkCount_:kind==2?previewToCount_:kind==1?previewFromCount_:std::max(previewFromCount_,previewToCount_);
            if(held) {
                const double shift=(count-h.count)*h.width;
                h.position.x+=h.direction.x*shift;h.position.y+=h.direction.y*shift;
                h.anchor.x+=h.direction.x*shift;h.anchor.y+=h.direction.y*shift;
            }
        }
        QPen stem(QColor("#9a5b00"),1);stem.setCosmetic(true);
        scene_.addLine(h.anchor.x,h.anchor.y,h.position.x,h.position.y,stem)->setZValue(200009);
        QPainterPath grip;
        grip.addRoundedRect(QRectF(h.position.x-half,h.position.y-half,2*half,2*half),corner,corner);
        QPen border(QColor("#ffffff"),2);border.setCosmetic(true);
        auto* item=scene_.addPath(grip,border,QBrush(held?QColor("#e08a00"):QColor("#ffb454")));
        item->setZValue(200010);item->setData(0,QStringLiteral("lane-resize"));item->setData(1,h.kind);
        auto* label=scene_.addSimpleText(QString::number(count));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setBrush(QBrush(QColor("#3a2200")));label->setPos(h.position.x,h.position.y);
        const auto box=label->boundingRect();
        // The label ignores the view transform, so centre it in device pixels around the grip.
        label->setTransform(QTransform::fromTranslate(-box.width()/2,-box.height()/2));
        label->setZValue(200011);
    }
}
}
