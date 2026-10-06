#include "conflict_display.hpp"
#include "connector_lane_mapping.hpp"
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
    return result;
}
ConflictPolygons conflictAreaPolygons(const Network& n,ConflictKind kind,const ConflictSide& a,const ConflictSide& b) {
    return conflictAreaGeometry(n,kind,a,b).polygons;
}
}
