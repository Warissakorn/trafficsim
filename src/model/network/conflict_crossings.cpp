#include "right_of_way.hpp"
#include "connector_lane_mapping.hpp"
#include <algorithm>
#include <cmath>
namespace trafficsim {
namespace {
constexpr double kMouthStationTolerance=1e-7; // authored metres, numerical only
struct Mouth {
    const Connector* connector{};
    LaneReference from,to;
    double fromStation{},toStation{},length{};
};
Mouth mouthOf(const Network& n,const ControlPathRef& ref) {
    for(const auto& c:n.connectors)if(c.id==ref.connectorId)
        for(const auto& pair:connectorLanePairs(n,c))
            if(pair.from.laneId==ref.fromLaneId && pair.to.laneId==ref.toLaneId)
                return {&c,pair.from,pair.to,attachmentStation(n,pair.from,true),
                        attachmentStation(n,pair.to,false),polylineLength(c.geometry)};
    return {};
}
bool at(StationInterval i,double station) {
    return i.from<=station+kMouthStationTolerance && i.to>=station-kMouthStationTolerance;
}
bool same(const LaneReference& a,double sa,const LaneReference& b,double sb) {
    return a.linkId==b.linkId && a.laneId==b.laneId && std::abs(sa-sb)<=kMouthStationTolerance;
}
bool sharedMouth(const ControlPathRef& a,const Mouth& ma,const ControlPathRef& b,const Mouth& mb,const SurfaceOverlap& o) {
    if(ma.connector && mb.connector)
        return (same(ma.from,ma.fromStation,mb.from,mb.fromStation) && at(o.first,0) && at(o.second,0)) ||
               (same(ma.to,ma.toStation,mb.to,mb.toStation) && at(o.first,ma.length) && at(o.second,mb.length));
    const auto joins=[](const Mouth& c,const ControlPathRef& l,StationInterval ci,StationInterval li) {
        return (c.from.linkId==l.linkId && c.from.laneId==l.laneId && at(ci,0) && at(li,c.fromStation)) ||
               (c.to.linkId==l.linkId && c.to.laneId==l.laneId && at(ci,c.length) && at(li,c.toStation));
    };
    return ma.connector && !mb.connector?joins(ma,b,o.first,o.second):
           mb.connector && !ma.connector?joins(mb,a,o.second,o.first):false;
}
}
std::vector<SurfaceOverlap> crossingOverlaps(const Network& n,const ControlPathRef& a,const ControlPathRef& b) {
    auto pieces=surfaceOverlaps(n,a,b);
    if(pieces.front().status!=SurfaceOverlap::Status::overlap)return pieces;
    const auto ma=mouthOf(n,a),mb=mouthOf(n,b);
    std::erase_if(pieces,[&](const auto& o){return sharedMouth(a,ma,b,mb,o);});
    if(pieces.empty())return {{SurfaceOverlap::Status::none,{},{}}};
    return pieces;
}
}
