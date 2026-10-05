#include "conflict_surface.hpp"
#include "connector_lane_mapping.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trafficsim {
namespace {
double distance(Point a, Point b) { return std::hypot(a.x-b.x,a.y-b.y); }
}
std::optional<ConflictSurface> conflictSurface(const Network& n,const ControlPathRef& ref) {
    try {
        if(!ref.linkId.empty() && ref.connectorId.empty()) {
            for(const auto& l:n.links)if(l.id==ref.linkId)
                for(std::size_t i=0;i<l.lanes.size();++i)if(l.lanes[i].id==ref.laneId) {
                    ConflictSurface s{l.geometry,laneBoundaryGeometry(l,i,n.drivingSide),laneBoundaryGeometry(l,i+1,n.drivingSide),{0}};
                    for(std::size_t j=1;j<l.geometry.size();++j)s.stations.push_back(s.stations.back()+distance(l.geometry[j-1],l.geometry[j]));
                    return s;
                }
            return std::nullopt;
        }
        if(!ref.linkId.empty() || ref.connectorId.empty())return std::nullopt;
        for(const auto& c:n.connectors)if(c.id==ref.connectorId) {
            const auto pairs=connectorLanePairs(n,c);std::optional<std::size_t> index;
            for(std::size_t i=0;i<pairs.size();++i)if(pairs[i].from.laneId==ref.fromLaneId && pairs[i].to.laneId==ref.toLaneId) {
                if(index)return std::nullopt;
                index=i;
            }
            if(!index)return std::nullopt;
            const auto rails=connectorBoundaries(n,c);
            if(c.geometry.size()<2 || rails.size()<*index+2)return ConflictSurface{};
            ConflictSurface s{c.geometry,rails[*index],rails[*index+1],{0}};
            for(std::size_t j=1;j<c.geometry.size();++j)
                s.stations.push_back(s.stations.back()+distance(c.geometry[j-1],c.geometry[j]));
            return s;
        }
    }catch(const std::exception&) {
        // A resolved but non-regular surface is unsupported, not a guessed overlap.
        return ConflictSurface{};
    }
    return std::nullopt;
}
std::vector<Point> conflictRuntimeOutline(const Network& n,const ConflictSide& side) {
    return conflictSideOutline(n,side);
}
namespace {
struct StationMap {std::vector<Point> reference,lane;const Connector* connector{};ConnectorPath path;};
StationMap stationMap(const Network& n,const ControlPathRef& ref) {
    if(!ref.linkId.empty())for(const auto& l:n.links)if(l.id==ref.linkId)
        return {l.geometry,laneGeometry(l,ref.laneId,n.drivingSide)};
    for(const auto& c:n.connectors)if(c.id==ref.connectorId)
        for(const auto& path:connectorPaths(n,c))if(path.from.laneId==ref.fromLaneId && path.to.laneId==ref.toLaneId)
            return {{},{},&c,path};
    throw std::invalid_argument("CONFLICT_UNRESOLVED_PATH");
}
double metres(const StationMap& m,double station) {
    return m.connector?connectorRuntimeStation(*m.connector,m.path,station):matchedStation(m.reference,m.lane,station);
}
}
double controlStationDistance(const Network& n,const ControlPathRef& ref,double from,double to) {
    const auto m=stationMap(n,ref);return metres(m,to)-metres(m,from);
}
double offsetControlStation(const Network& n,const ControlPathRef& ref,double station,double offset) {
    const auto m=stationMap(n,ref);const double length=m.connector?connectorPathLength(m.path):polylineLength(m.lane);
    const double at=std::clamp(metres(m,station)+offset,0.,length);
    return m.connector?connectorAuthoringStation(*m.connector,m.path,at):matchedStation(m.lane,m.reference,at);
}

}
