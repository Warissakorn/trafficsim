#include "right_of_way.hpp"
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
ConflictPolygons receivingMouth(const Network& n,const ControlPathRef& ref) {
    ConflictPolygons result;
    for(const auto& c:n.connectors)if(c.id==ref.connectorId)
        for(const auto& pair:connectorLanePairs(n,c))
            if(pair.from.laneId==ref.fromLaneId && pair.to.laneId==ref.toLaneId) {
                const ControlPathRef lane{pair.to.linkId,pair.to.laneId,"","",""};
                for(const auto& o:classifiedOverlaps(n,ref,lane))
                    if(o.status==SurfaceOverlap::Status::overlap && o.geometryKind==ConflictGeometryKind::merge)
                        append(result,o.polygons);
            }
    return result;
}
}
ConflictPolygons conflictAreaPolygons(const Network& n,ConflictKind kind,const ConflictSide& a,const ConflictSide& b) {
    ConflictPolygons result;
    const auto wanted=kind==ConflictKind::crossing?ConflictGeometryKind::crossing:ConflictGeometryKind::merge;
    for(const auto& o:classifiedOverlaps(n,a.path,b.path)) {
        if(o.status!=SurfaceOverlap::Status::overlap || o.geometryKind!=wanted)continue;
        if(kind==ConflictKind::crossing && (!holds(a,o.first) || !holds(b,o.second)))continue;
        append(result,o.polygons);
    }
    if(kind==ConflictKind::merge && result.empty()) {
        const auto ta=targetOf(n,a.path),tb=targetOf(n,b.path);
        if(ta && tb && ta->linkId==tb->linkId && ta->laneId==tb->laneId &&
           std::abs(attachmentStation(n,*ta,false)-attachmentStation(n,*tb,false))<=1e-7) {
            append(result,receivingMouth(n,a.path));
            append(result,receivingMouth(n,b.path));
        }
    }
    return result;
}
}
