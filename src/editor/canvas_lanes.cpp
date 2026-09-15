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
        const auto& lane=*(first+count-1);
        const auto geometry=laneGeometry(link,lane.id,side);
        const double length=polylineLength(geometry),fraction=ref.fraction.value_or(kind==1?1.:0.);
        const auto p=pointAlong(geometry,length*fraction);
        const auto a=pointAlong(geometry,std::max(0.,length*fraction-.01));
        const auto b=pointAlong(geometry,std::min(length,length*fraction+.01));
        const double norm=std::hypot(b.x-a.x,b.y-a.y),sign=side==DrivingSide::left?1.:-1.;
        const Point direction{sign*(b.y-a.y)/norm,-sign*(b.x-a.x)/norm};
        // Keep resize handles visibly outside geometry handles at every zoom level.
        const double offset=lane.width/2+10/std::abs(transform().m11());
        return LaneHandle{{p.x+direction.x*offset,p.y+direction.y*offset},direction,lane.width,kind,count,available};
    };
    if(const auto* link=selectedLink();link && levelVisible(link->level)) {
        auto h=handle(*link,{link->id,link->lanes.front().id,.5},static_cast<int>(link->lanes.size()),4);
        h.maximum=12;return {h};
    }
    if(const auto* connector=selectedConnector();connector && levelVisible(connector->level)) {
        const Link *from=nullptr,*to=nullptr;
        for(const auto& link:document_->network.links) {
            if(link.id==connector->from.linkId)from=&link;
            if(link.id==connector->to.linkId)to=&link;
        }
        if(!from || !to)return {};
        auto a=handle(*from,connector->from,connector->fromLaneCount,1);
        auto b=handle(*to,connector->to,connector->toLaneCount,2);
        const auto paths=connectorPaths(document_->network,*connector);
        const auto& outer=paths.back().geometry;const double length=polylineLength(outer);
        const auto mid=pointAlong(outer,length/2),ahead=pointAlong(outer,std::min(length,length/2+.01));
        const double norm=std::hypot(ahead.x-mid.x,ahead.y-mid.y),sign=side==DrivingSide::left?1.:-1.;
        const Point direction{sign*(ahead.y-mid.y)/norm,-sign*(ahead.x-mid.x)/norm};
        const double width=(a.width+b.width)/2,offset=width/2+10/std::abs(transform().m11());
        LaneHandle body{{mid.x+direction.x*offset,mid.y+direction.y*offset},direction,width,3,
                        std::max(a.count,b.count),std::min(a.maximum,b.maximum)};
        return {a,b,body};
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
    if(picked->kind==4)previewLinkCount_=picked->count;
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
    if(h.kind==3 && std::round(lateral/h.width)==0) {
        const auto* c=selectedConnector();previewFromCount_=c->fromLaneCount;previewToCount_=c->toLaneCount;redraw();return;
    }
    const int count=std::clamp(h.count+static_cast<int>(std::round(lateral/h.width)),1,h.maximum);
    if(h.kind==4)previewLinkCount_=count;
    else if(h.kind==1)previewFromCount_=count;
    else if(h.kind==2)previewToCount_=count;
    else previewFromCount_=previewToCount_=count;
    redraw();
}
void EditorCanvas::drawLaneHandles() {
    const double radius=4/std::abs(transform().m11());
    for(auto h:laneHandles()) {
        int count=h.count;
        if(laneResize_ && laneResize_->kind==h.kind) {
            count=h.kind==4?previewLinkCount_:h.kind==2?previewToCount_:previewFromCount_;
            // Link lanes are centred; an extra lane moves the outer edge by half its width.
            const double shift=(count-h.count)*h.width*(h.kind==4?.5:1.);
            h.position.x+=h.direction.x*shift;h.position.y+=h.direction.y*shift;
        }
        QPen pen(QColor("#9a5b00"));pen.setCosmetic(true);
        auto* item=scene_.addRect(h.position.x-radius,h.position.y-radius,2*radius,2*radius,pen,QBrush("#ffb454"));
        item->setZValue(200010);item->setData(0,QStringLiteral("lane-resize"));item->setData(1,h.kind);
        auto* label=scene_.addSimpleText(QString::number(count));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);label->setPos(h.position.x+radius,h.position.y);
        label->setZValue(200010);
    }
}
}
