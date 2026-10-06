#include "conflict_display.hpp"
#include "conflict_surface.hpp"
#include "conflict_polygon_math.hpp"
#include "connector_surface.hpp"
#include "connector_lane_mapping.hpp"
#include <algorithm>
#include <cmath>
namespace trafficsim {
namespace {
std::vector<ConflictMouthBand> atMouth(const Network& n,const LaneReference& attachment,const ConnectorMouth& mouth) {
    std::vector<ConflictMouthBand> result;
    const ControlPathRef ref{attachment.linkId,attachment.laneId,"","",""};
    const auto s=conflictSurface(n,ref);if(!s || s->base.size()<2)return result;
    const auto cap=triangulatePolygon({mouth.points.begin(),mouth.points.end()});
    double station=0;
    for(std::size_t j=1;j<s->base.size();++j) {
        const auto l0=s->left[j-1],r0=s->right[j-1],l1=s->left[j],r1=s->right[j];
        const auto lane=triangulatePolygon({l0,l1,r1,r0});
        const double length=std::hypot(s->base[j].x-s->base[j-1].x,s->base[j].y-s->base[j-1].y);
        ConflictPolygons clips;double from=INFINITY,to=-INFINITY;
        for(const auto& c:cap)for(const auto& q:lane) {
            auto p=intersectConvexPolygons(c,q);
            if(p.size()<3 || polygonSignedArea(p)<1e-6)continue;
            for(const auto& point:p) {
                const double at=stationInLaneQuad(l0,l1,r0,r1,point,station,length);
                from=std::min(from,at);to=std::max(to,at);
            }
            clips.push_back(std::move(p));
        }
        if(!clips.empty() && to>from)result.push_back({{ref,from,to,""},std::move(clips)});
        station+=length;
    }
    return result;
}
}
std::vector<ConflictMouthBand> conflictMouthBands(const Network& n,const ControlPathRef& ref,bool source,bool target) {
    for(const auto& c:n.connectors)if(c.id==ref.connectorId) {
        const auto surface=connectorSurface(n,c);if(surface.selfIntersecting)return {};
        for(const auto& pair:connectorLanePairs(n,c))if(pair.from.laneId==ref.fromLaneId && pair.to.laneId==ref.toLaneId) {
            std::vector<ConflictMouthBand> result;
            if(source && surface.source)result=atMouth(n,pair.from,*surface.source);
            if(target && surface.target) {
                auto parts=atMouth(n,pair.to,*surface.target);result.insert(result.end(),parts.begin(),parts.end());
            }
            return result;
        }
    }
    return {};
}
}
