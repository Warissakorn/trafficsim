#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsItem>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace trafficsim {
void EditorCanvas::previewLinkLanes(Link& link) const {
    if(link.id!=selected() || !laneResize_ || (laneResize_->kind!=4 && laneResize_->kind!=8))return;
    const bool leading=laneResize_->kind==8;auto lanes=link.lanes;
    const double width=(leading?lanes.front():lanes.back()).width;
    while(static_cast<int>(lanes.size())<previewLinkCount_)
        lanes.insert(leading?lanes.begin():lanes.end(),{"preview-"+std::to_string(lanes.size()),width});
    while(static_cast<int>(lanes.size())>previewLinkCount_)lanes.erase(leading?lanes.begin():lanes.end()-1);
    replaceLaneBundle(link,std::move(lanes),leading);
}
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
        const auto tangent=directionAlong(geometry,at,base==1);
        const double sign=(side==DrivingSide::left?1.:-1.)*(leading?-1.:1.);
        const Point direction{sign*tangent.y,-sign*tangent.x};
        const auto index=static_cast<std::size_t>(std::distance(link.lanes.begin(),first))+(leading?0:count);
        const auto boundary=laneBoundaryGeometry(link,index,side);
        const auto anchor=pointAlong(boundary,matchedStation(link.geometry,boundary,
            ref.station.value_or(base==1?polylineLength(link.geometry):0.)));
        const double offset=canvasStyle::laneTabDepth/2/std::abs(transform().m11());
        return LaneHandle{{anchor.x+direction.x*offset,anchor.y+direction.y*offset},anchor,
                          direction,lane.width,kind,count,leading?static_cast<int>(std::distance(link.lanes.begin(),first))+count:available};
    };
    if(const auto* original=selectedLink();original && levelVisible(original->level)) {
        auto preview=*original;previewLinkLanes(preview);const auto* link=&preview;
        std::vector<LaneHandle> result;
        const double length=polylineLength(link->geometry);
        for(int location=0;location<3;++location) {
            const LaneReference ref{link->id,link->lanes.front().id,length*location/2};
            for(int kind:{4,8}) {
                auto h=handle(*link,ref,static_cast<int>(link->lanes.size()),kind);
                h.maximum=12;h.location=location;
                const auto boundary=laneBoundaryGeometry(*link,kind==8?0:link->lanes.size(),side);
                const double railLength=polylineLength(boundary);
                const double scale=std::abs(transform().m11());
                h.tabLength=std::min(canvasStyle::laneTabLength,railLength*scale/3);
                const double inset=h.tabLength/2/scale;
                const double station=location==0?inset:location==1?railLength/2:railLength-inset;
                h.anchor=pointAlong(boundary,station);
                const auto& lane=kind==8?link->lanes.front():link->lanes.back();
                const auto geometry=laneGeometry(*link,lane.id,side);
                const double onLink=matchedStation(boundary,link->geometry,station);
                const double onLane=matchedStation(link->geometry,geometry,onLink);
                const auto tangent=directionAlong(geometry,onLane,false);
                const double sign=(side==DrivingSide::left?1.:-1.)*(kind==8?-1.:1.);
                h.direction={sign*tangent.y,-sign*tangent.x};
                const double offset=canvasStyle::laneTabDepth/2/scale;
                h.position={h.anchor.x+h.direction.x*offset,h.anchor.y+h.direction.y*offset};
                result.push_back(h);
            }
        }
        return result;
    }
    if(const auto* original=selectedConnector();original && levelVisible(original->level)) {
        auto preview=*original;
        if(rangeCorner_)resizeConnectorEdges(document_->network,preview,previewFromCount_,previewToCount_,rangeCorner_>4,true);
        const auto* connector=&preview;
        const Link *from=nullptr,*to=nullptr;
        for(const auto& link:document_->network.links) {
            if(link.id==connector->from.linkId)from=&link;
            if(link.id==connector->to.linkId)to=&link;
        }
        if(!from || !to)return {};
        std::vector<LaneHandle> result;
        const auto& paths=cachedPaths(*connector);
        const auto& boundaries=cachedBoundaries(*connector);
        const auto widths=connectorLaneWidths(document_->network,*connector);
        const double offset=canvasStyle::laneTabDepth/2/std::abs(transform().m11());
        const auto attachEnd=[&](LaneHandle& h,bool source) {
            // The non-leading tabs follow the far rail; leading tabs follow the kerb rail,
            // matching the corresponding Connector-body tab at midspan.
            const bool leading=h.kind>4;
            const auto& edge=leading?boundaries.front():boundaries.back();
            const auto& other=leading?boundaries.back():boundaries.front();
            const double length=polylineLength(edge);
            const double scale=std::abs(transform().m11());
            h.tabLength=std::min(canvasStyle::laneTabLength,length*scale/2);
            const double inset=std::min(h.tabLength/2/scale,length/2);
            const double station=source?inset:length-inset;
            h.anchor=pointAlong(edge,station);
            const auto tangent=directionAlong(edge,station,false);
            const auto opposite=pointAlong(other,matchedStation(edge,other,station));
            Point normal{h.anchor.x-opposite.x,h.anchor.y-opposite.y};
            double along=normal.x*tangent.x+normal.y*tangent.y;
            normal.x-=along*tangent.x;normal.y-=along*tangent.y;
            double magnitude=std::hypot(normal.x,normal.y);
            if(magnitude<=1e-9) {
                normal=h.direction;along=normal.x*tangent.x+normal.y*tangent.y;
                normal.x-=along*tangent.x;normal.y-=along*tangent.y;
                magnitude=std::hypot(normal.x,normal.y);
            }
            if(magnitude>1e-9)h.direction={normal.x/magnitude,normal.y/magnitude};
            h.position={h.anchor.x+h.direction.x*offset,h.anchor.y+h.direction.y*offset};
        };
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
            attachEnd(a,true);attachEnd(b,false);
            // One drag step is one lane: the outer path may be a taper, zero wide at one end,
            // and averaging that in would make half a lane's drag add or drop a whole one.
            const double width=std::max(widths.source[index],widths.target[index]);
            LaneHandle body{{anchor.x+direction.x*offset,anchor.y+direction.y*offset},anchor,
                            direction,width,3+extra,std::max(a.count,b.count),std::min(a.maximum,b.maximum)};
            result.insert(result.end(),{a,b,body});
        }
        return result;
    }
    return {};
}
QPainterPath EditorCanvas::laneHandlePath(const LaneHandle& h,double padding) const {
    const double scale=std::abs(transform().m11());
    const double length=(h.tabLength>0?h.tabLength:canvasStyle::laneTabLength)/2/scale+padding;
    const double depth=canvasStyle::laneTabDepth/2/scale+padding;
    const Point tangent{-h.direction.y,h.direction.x};
    QPainterPath shape;bool first=true;
    for(const auto [along,outward]:{std::pair{-length,-depth},std::pair{length,-depth},
                                   std::pair{length,depth},std::pair{-length,depth}}) {
        const QPointF p(h.position.x+tangent.x*along+h.direction.x*outward,
                        h.position.y+tangent.y*along+h.direction.y*outward);
        if(first){shape.moveTo(p);first=false;}else shape.lineTo(p);
    }
    shape.closeSubpath();return shape;
}
std::optional<EditorCanvas::LaneHandle> EditorCanvas::laneHandleAt(QPoint position) const {
    std::optional<LaneHandle> picked;double best=1e300;
    for(const auto& h:laneHandles()) {
        if(!laneHandlePath(h,2/std::abs(transform().m11())).contains(mapToScene(position)))continue;
        const auto delta=mapFromScene(h.position.x,h.position.y)-position;
        const double distance=QPoint::dotProduct(delta,delta);
        if(distance<best){best=distance;picked=h;}
    }
    // At a distant zoom, a lane tab can overlap a geometry grip. The nearest centre wins.
    if(picked)if(const int vertex=vertexAt(position);vertex>=0) {
        const auto point=handleGeometry()[static_cast<std::size_t>(vertex)];
        const auto delta=mapFromScene(point.x,point.y)-position;
        if(QPoint::dotProduct(delta,delta)<=best)return {};
    }
    return picked;
}
bool EditorCanvas::startLaneResize(QPoint position) {
    const auto picked=laneHandleAt(position);
    if(!picked)return false;
    laneResize_=picked;dragStart_=world(position,false);
    if(picked->kind==4 || picked->kind==8)previewLinkCount_=picked->count;
    else {
        const auto* c=selectedConnector();previewFromCount_=c->fromLaneCount;previewToCount_=c->toLaneCount;
        rangeCorner_=picked->kind;
    }
    redraw(); // the resize cursor set on hover stays for the drag
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
    // One lane added or dropped per side at most (D73): the tab stops at a two-lane difference.
    else if(kind==1)previewFromCount_=std::clamp(count,std::max(1,previewToCount_-2),previewToCount_+2);
    else if(kind==2)previewToCount_=std::clamp(count,std::max(1,previewFromCount_-2),previewFromCount_+2);
    else previewFromCount_=previewToCount_=count;
    redraw();
}
void EditorCanvas::drawLaneHandles() {
    if(movingGeometry())return; // skips laneHandles() too: they reappear on release
    for(auto h:laneHandles()) {
        const bool held=laneResize_ && laneResize_->kind==h.kind && laneResize_->location==h.location;
        const bool hovered=hoverLaneKind_==h.kind && hoverLaneLocation_==h.location;
        const QColor colour=held?canvasStyle::active():canvasStyle::selection();
        QPen border(colour,held || hovered?2:1);border.setCosmetic(true);
        auto* item=scene_.addPath(laneHandlePath(h),border,QBrush(held || hovered?colour:editorDesign::role(QPalette::Base)));
        item->setZValue(200011);item->setData(0,QStringLiteral("lane-resize"));item->setData(1,h.kind);
        item->setData(2,h.location);
    }
}
}
