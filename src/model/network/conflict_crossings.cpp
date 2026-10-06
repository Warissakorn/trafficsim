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
        // P1-P4 cuts can meet the named Link lane before/after its attachment station.
        // A measured piece touching this Connector's terminal cross-section is its
        // mouth on that lane; a separate interior crossing remains a crossing.
        const auto& g=c.connector->geometry;
        if(g.size()<2)return false;
        const double firstLeg=std::hypot(g[1].x-g[0].x,g[1].y-g[0].y);
        const double lastLeg=std::hypot(g.back().x-g[g.size()-2].x,g.back().y-g[g.size()-2].y);
        // At a finite Link end, a longitudinal mouth cut can extend beyond the Link.
        // Its overlap then ends before the Connector terminal, but still reaches the
        // named attachment on the Link and lies wholly on the terminal Connector leg.
        const bool source=at(ci,0) || (ci.to<=firstLeg+kMouthStationTolerance && at(li,c.fromStation));
        const bool target=at(ci,c.length) || (ci.from>=c.length-lastLeg-kMouthStationTolerance && at(li,c.toStation));
        return (c.from.linkId==l.linkId && c.from.laneId==l.laneId && source) ||
               (c.to.linkId==l.linkId && c.to.laneId==l.laneId && target);
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
