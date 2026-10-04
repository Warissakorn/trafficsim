#include "connector_lane_mapping.hpp"
#include "connector_surface.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <stdexcept>
namespace trafficsim {
std::string connectorPathId(const Connector& c,int index) {
    return index==0?c.id:c.id+"/lane-"+std::to_string(index+1);
}
std::vector<double> connectorBlendWeights(const Connector& c) {
    if(!c.laneBlend.empty()) {
        if(c.laneBlend.size()!=c.geometry.size() || c.laneBlend.front()!=0 || c.laneBlend.back()!=1)
            throw std::invalid_argument("INVALID_GEOMETRY");
        double previous=0;
        for(double t:c.laneBlend) {
            if(!std::isfinite(t) || t<previous || t>1)throw std::invalid_argument("INVALID_GEOMETRY");
            previous=t;
        }
        return c.laneBlend;
    }
    std::vector<double> result(c.geometry.size());const double length=polylineLength(c.geometry);
    double station=0;
    for(std::size_t i=1;i<result.size();++i) {
        station+=std::hypot(c.geometry[i].x-c.geometry[i-1].x,c.geometry[i].y-c.geometry[i-1].y);
        result[i]=length>0?station/length:0;
    }
    return result;
}
void resizeConnectorEdges(const Network& n,Connector& c,int fromCount,int toCount,bool leading,bool fromTab) {
    auto resized=c;
    if(leading) {
        const auto shift=[&](LaneReference& ref,int delta) {
            for(const auto& l:n.links)if(l.id==ref.linkId) {
                const auto it=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
                const auto index=std::distance(l.lanes.begin(),it)-delta;
                if(it==l.lanes.end() || index<0 || index>=static_cast<int>(l.lanes.size()))break;
                ref.laneId=l.lanes[static_cast<std::size_t>(index)].id;return;
            }
            throw std::invalid_argument("EDIT_LANE_RANGE");
        };
        shift(resized.from,fromCount-c.fromLaneCount);shift(resized.to,toCount-c.toLaneCount);
        resized.laneBlend=connectorBlendWeights(c);
        const auto a=laneAttachment(n,resized.from,true),b=laneAttachment(n,resized.to,false);
        for(std::size_t j=0;j<c.geometry.size();++j) {
            const double t=resized.laneBlend[j];
            resized.geometry[j].x+=(a.x-c.geometry.front().x)*(1-t)+(b.x-c.geometry.back().x)*t;
            resized.geometry[j].y+=(a.y-c.geometry.front().y)*(1-t)+(b.y-c.geometry.back().y)*t;
        }
        resized.geometry.front()=a;resized.geometry.back()=b;
    }
    resized.fromLaneCount=fromCount;resized.toLaneCount=toCount;
    // A side chosen for a one-lane difference says nothing about any other difference.
    const int before=std::abs(c.fromLaneCount-c.toLaneCount),after=std::abs(fromCount-toCount);
    if(after!=before) {
        resized.laneChangeSide.reset();
        // A tab adds or drops its lane at its own edge: the kerb edge for a leading tab, the far
        // edge otherwise. From equal counts that lane is the one that tapers; from a two-lane
        // difference it pairs up, and the taper left is the other edge's (D83).
        if(fromTab && after==1 && (before==0)!=leading)
            resized.laneChangeSide=n.drivingSide==DrivingSide::left?LaneSide::right:LaneSide::left;
    }
    // Authored widths and markings are indexed by lane path, so a resize that changes how many
    // paths there are leaves them describing lanes that no longer exist. Dropped rather than
    // padded: a width the author never typed is not a width they chose, and the derived one is
    // the honest fallback. Same reasoning as clearing laneBlend when the geometry changes.
    if(connectorPaths(n,resized).size()!=connectorPaths(n,c).size())
        { resized.laneWidths.clear();resized.laneMarkings.clear(); }
    (void)connectorPaths(n,resized);c=std::move(resized);
}
int connectorLaneShift(const Network& n,const Connector& c) {
    const int difference=std::abs(c.fromLaneCount-c.toLaneCount);
    // One lane added or dropped per side at most (D73): a 2 -> 5 Connector is not a road.
    if(difference>2 || (c.laneChangeSide && difference!=1))throw std::invalid_argument("EDIT_LANE_RANGE");
    if(difference!=1)return difference/2;
    // Lane 0 is the kerb lane on both driving sides, so the kerb is the driver's right in
    // right-hand traffic and the driver's left in left-hand traffic.
    const auto kerb=n.drivingSide==DrivingSide::left?LaneSide::left:LaneSide::right;
    return c.laneChangeSide.value_or(kerb)==kerb?1:0;
}
void fitLaneDifference(Connector& c) {
    c.fromLaneCount=std::min(c.fromLaneCount,c.toLaneCount+2);
    c.toLaneCount=std::min(c.toLaneCount,c.fromLaneCount+2);
    if(std::abs(c.fromLaneCount-c.toLaneCount)!=1)c.laneChangeSide.reset();
}
std::size_t centredLaneRange(std::size_t focus,int count,std::size_t laneCount) {
    const long long lanes=static_cast<long long>(laneCount),run=std::max(1,count);
    return static_cast<std::size_t>(std::clamp(static_cast<long long>(focus)-(run-1)/2,0LL,std::max(0LL,lanes-run)));
}
std::vector<ConnectorLanePair> connectorLanePairs(const Network& n,const Connector& c) {
    const auto range=[&](const LaneReference& ref,int count) {
        std::vector<LaneReference> result;
        if(count<1 || count>12)throw std::invalid_argument("EDIT_LANE_RANGE");
        for(const auto& l:n.links)if(l.id==ref.linkId) {
            const auto it=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
            if(it==l.lanes.end() || std::distance(it,l.lanes.end())<count)break;
            for(int i=0;i<count;++i)result.push_back({l.id,(it+i)->id,ref.station});
            return result;
        }
        throw std::invalid_argument("EDIT_LANE_RANGE");
    };
    const auto from=range(c.from,c.fromLaneCount),to=range(c.to,c.toLaneCount);
    const int count=std::max(c.fromLaneCount,c.toLaneCount);
    const int shift=connectorLaneShift(n,c);
    const auto narrow=[&](int i,int lanes){return lanes==count?i:std::clamp(i-shift,0,lanes-1);};
    std::vector<ConnectorLanePair> result;
    for(int i=0;i<count;++i)result.push_back({from[narrow(i,c.fromLaneCount)],to[narrow(i,c.toLaneCount)]});
    return result;
}
std::vector<ConnectorPath> connectorPaths(const Network& n,const Connector& c) {
    const auto pairs=connectorLanePairs(n,c);
    std::vector<ConnectorPath> result;
    if(c.geometry.size()<2) {
        for(std::size_t i=0;i<pairs.size();++i)
            result.push_back({connectorPathId(c,static_cast<int>(i)),pairs[i].from,pairs[i].to,c.geometry});
        return result; // Keep draft validation readable, including empty geometry.
    }
    const auto rails=connectorSurface(n,c).boundaries;
    for(std::size_t i=0;i<pairs.size();++i) {
        std::vector<Point> shape;shape.reserve(c.geometry.size());
        for(std::size_t j=0;j<c.geometry.size();++j) {
            Point p{std::midpoint(rails[i][j].x,rails[i+1][j].x),
                    std::midpoint(rails[i][j].y,rails[i+1][j].y)};
            // Symmetric single-lane surfaces differ from their authoring axis only by round-off.
            // Keep those vertices exactly, preserving the frozen core trajectory fixtures.
            if(std::hypot(p.x-c.geometry[j].x,p.y-c.geometry[j].y)<=1e-12)p=c.geometry[j];
            shape.push_back(p);
        }
        // Mouth cuts can be longitudinally displaced, and a zero-width taper ends on an edge.
        // Runtime enters/leaves at the named Link lane centre, not at either of those cuts.
        // The terminal polyline legs connect that centre to the first/last interior midpoint.
        shape.front()=laneAttachment(n,pairs[i].from,true);
        shape.back()=laneAttachment(n,pairs[i].to,false);
        result.push_back({connectorPathId(c,static_cast<int>(i)),pairs[i].from,pairs[i].to,std::move(shape)});
    }
    return result;
}
}
