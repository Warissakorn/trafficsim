#include "conflict_surface.hpp"
#include "connector_lane_mapping.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trafficsim {
namespace {
// Metres, not calibration. Refine centre and both edges to this chord error; a fixed initial
// partition and quarter checks also catch S bends whose midpoint lies on their chord.
constexpr double kSurfaceError = .002;
struct Sample { double t; Point centre, left, right; };
double distance(Point a, Point b) { return std::hypot(a.x-b.x,a.y-b.y); }
Point middle(Point a, Point b, double t) { return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t}; }
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
            const auto curve=connectorEquation(n,pairs[*index].from,pairs[*index].to);
            const auto widths=connectorLaneWidths(n,c);
            const auto sample=[&](double t) {
                const auto p=equationPoint(curve,t),d=equationDerivative(curve,t);
                const double length=std::hypot(d.x,d.y);
                if(length<1e-12)throw std::invalid_argument("CONFLICT_GEOMETRY_UNSUPPORTED");
                const double half=(widths.source[*index]*(1-t)+widths.target[*index]*t)/2;
                const Point normal{-d.y/length,d.x/length};
                return Sample{t,p,{p.x+normal.x*half,p.y+normal.y*half},{p.x-normal.x*half,p.y-normal.y*half}};
            };
            std::vector<Sample> samples{sample(0)};
            const auto refine=[&](auto&& self,const Sample& a,const Sample& b,int depth)->void {
                double error=0;
                for(double f:{.25,.5,.75}) {
                    const auto s=sample(a.t+(b.t-a.t)*f);
                    error=std::max({error,distance(s.centre,middle(a.centre,b.centre,f)),
                        distance(s.left,middle(a.left,b.left,f)),distance(s.right,middle(a.right,b.right,f))});
                }
                if(error>kSurfaceError) {
                    if(depth==16)throw std::invalid_argument("CONFLICT_GEOMETRY_UNSUPPORTED");
                    const auto m=sample((a.t+b.t)/2);self(self,a,m,depth+1);self(self,m,b,depth+1);
                }else samples.push_back(b);
            };
            for(int i=0;i<8;++i)refine(refine,sample(i/8.),sample((i+1)/8.),0);
            ConflictSurface s;s.connector=&c;
            for(const auto& p:samples) {
                s.base.push_back(p.centre);s.left.push_back(p.left);s.right.push_back(p.right);
                s.stations.push_back(p.t);
            }
            return s;
        }
    }catch(const std::exception&) {
        // A resolved but non-regular surface is unsupported, not a guessed overlap.
        return ConflictSurface{};
    }
    return std::nullopt;
}
std::vector<Point> conflictRuntimeOutline(const Network& n,const ConflictSide& side) {
    const auto s=conflictSurface(n,side.path);if(!s || s->base.size()<2)return {};
    const auto span=[&](const std::vector<Point>& edge) {
        std::vector<Point> out;
        const auto point=[&](double station) {
            if(s->connector)station=connectorDrawingParameter(*s->connector,station);
            const auto upper=std::upper_bound(s->stations.begin(),s->stations.end(),station);
            const auto j=std::clamp<std::size_t>(static_cast<std::size_t>(upper-s->stations.begin()),1,s->stations.size()-1);
            const double length=s->stations[j]-s->stations[j-1];
            return middle(edge[j-1],edge[j],length>0?std::clamp((station-s->stations[j-1])/length,0.,1.):0);
        };
        out.push_back(point(side.entryStation));
        for(std::size_t j=1;j+1<s->stations.size();++j)if((s->connector?connectorDrawingStation(*s->connector,s->stations[j]):s->stations[j])>side.entryStation &&
                (s->connector?connectorDrawingStation(*s->connector,s->stations[j]):s->stations[j])<side.exitStation)out.push_back(edge[j]);
        out.push_back(point(side.exitStation));return out;
    };
    auto out=span(s->left);const auto right=span(s->right);out.insert(out.end(),right.rbegin(),right.rend());return out;
}
namespace {
struct StationMap {std::vector<Point> reference,lane;const Connector* connector{};ConnectorPath path;};
StationMap stationMap(const Network& n,const ControlPathRef& ref) {
    if(!ref.linkId.empty())for(const auto& l:n.links)if(l.id==ref.linkId)
        return {l.geometry,laneGeometry(l,ref.laneId,n.drivingSide)};
    for(const auto& c:n.connectors)if(c.id==ref.connectorId)
        for(const auto& pair:connectorLanePairs(n,c))if(pair.from.laneId==ref.fromLaneId && pair.to.laneId==ref.toLaneId)
            return {{},{},&c,ConnectorPath{"",pair.from,pair.to,{},connectorEquation(n,pair.from,pair.to)}};
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
