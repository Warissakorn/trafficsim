#include "conflict_display.hpp"
#include "connector_lane_mapping.hpp"
#include "conflict_surface.hpp"
#include "conflict_polygon_math.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
void append(ConflictPolygons& to,const ConflictPolygons& from) {
    to.insert(to.end(),from.begin(),from.end());
}
bool holds(const ConflictSide& side,StationInterval interval) {
    const double middle=(interval.from+interval.to)/2;
    return side.entryStation<=middle && middle<=side.exitStation;
}
// Two arriving Connectors can meet only on an edge. Their receiving-lane mouth overlaps
// still identify the physical merge. Never replace absent geometry with a square/strip.
std::optional<LaneReference> targetOf(const Network& n,const ControlPathRef& ref) {
    for(const auto& c:n.connectors)if(c.id==ref.connectorId)
        for(const auto& pair:connectorLanePairs(n,c))
            if(pair.from.laneId==ref.fromLaneId && pair.to.laneId==ref.toLaneId)return pair.to;
    return std::nullopt;
}
void receivingMouth(const Network& n,const ConflictSide& side,ConflictPolygons& polygons,std::vector<ConflictSide>& spans) {
    const auto& ref=side.path;
    for(const auto& c:n.connectors)if(c.id==ref.connectorId)
        for(const auto& pair:connectorLanePairs(n,c))
            if(pair.from.laneId==ref.fromLaneId && pair.to.laneId==ref.toLaneId) {
                const ControlPathRef lane{pair.to.linkId,pair.to.laneId,"","",""};
                for(const auto& o:classifiedOverlaps(n,ref,lane))
                    if(o.status==SurfaceOverlap::Status::overlap && o.geometryKind==ConflictGeometryKind::merge)
                        { append(polygons,o.polygons);spans.push_back({ref,o.first.from,o.first.to,side.waitingLineId}); }
            }
}
std::vector<ConflictMouthBand> mouthsFor(const Network& n,ConflictKind kind,const ControlPathRef& ref,std::vector<ConflictSide>& spans) {
    if(ref.connectorId.empty() || spans.empty())return {};
    const double length=polylineLength(controlPathPolyline(n,ref));
    const bool source=kind==ConflictKind::branching || spans.front().entryStation<=1e-7;
    const bool target=kind==ConflictKind::merge || spans.back().exitStation>=length-1e-7;
    auto parts=conflictMouthBands(n,ref,source,target);
    if(!parts.empty()) {
        if(source)spans.front().entryStation=0;
        if(target)spans.back().exitStation=length;
    }
    return parts;
}
void appendMouthOverlap(const Network& n,const std::vector<ConflictMouthBand>& mouths,const ControlPathRef& other,ConflictPolygons& polygons) {
    const auto s=conflictSurface(n,other);if(!s)return;
    for(const auto& mouth:mouths)for(const auto& c:mouth.clips)for(std::size_t j=1;j<s->base.size();++j)
        for(const auto& q:triangulatePolygon({s->left[j-1],s->left[j],s->right[j],s->right[j-1]})) {
            auto p=intersectConvexPolygons(c,q);
            if(p.size()>=3 && polygonSignedArea(p)>=1e-6)polygons.push_back(std::move(p));
        }
}

}
ConflictAreaGeometry conflictAreaGeometry(const Network& n,ConflictKind kind,const ConflictSide& a,const ConflictSide& b) {
    ConflictAreaGeometry result;
    const auto wanted=kind==ConflictKind::crossing?ConflictGeometryKind::crossing:
        kind==ConflictKind::branching?ConflictGeometryKind::branching:ConflictGeometryKind::merge;
    for(const auto& o:classifiedOverlaps(n,a.path,b.path)) {
        if(o.status!=SurfaceOverlap::Status::overlap || o.geometryKind!=wanted)continue;
        if(kind!=ConflictKind::merge && (!holds(a,o.first) || !holds(b,o.second)))continue;
        append(result.polygons,o.polygons);
        result.first.push_back({a.path,o.first.from,o.first.to,a.waitingLineId});
        result.second.push_back({b.path,o.second.from,o.second.to,b.waitingLineId});
    }
    if(kind==ConflictKind::merge && result.polygons.empty()) {
        const auto ta=targetOf(n,a.path),tb=targetOf(n,b.path);
        if(ta && tb && ta->linkId==tb->linkId && ta->laneId==tb->laneId &&
           std::abs(attachmentStation(n,*ta,false)-attachmentStation(n,*tb,false))<=1e-7) {
            receivingMouth(n,a,result.polygons,result.first);
            receivingMouth(n,b,result.polygons,result.second);
        }
    }
    result.firstMouth=mouthsFor(n,kind,a.path,result.first);
    result.secondMouth=mouthsFor(n,kind,b.path,result.second);
    appendMouthOverlap(n,result.firstMouth,b.path,result.polygons);
    appendMouthOverlap(n,result.secondMouth,a.path,result.polygons);
    for(const auto& first:result.firstMouth)for(const auto& second:result.secondMouth)
        for(const auto& x:first.clips)for(const auto& y:second.clips) {
            auto p=intersectConvexPolygons(x,y);
            if(p.size()>=3 && polygonSignedArea(p)>=1e-6)result.polygons.push_back(std::move(p));
        }
    return result;
}
ConflictPolygons conflictAreaPolygons(const Network& n,ConflictKind kind,const ConflictSide& a,const ConflictSide& b) {
    return conflictAreaGeometry(n,kind,a,b).polygons;
}
}
