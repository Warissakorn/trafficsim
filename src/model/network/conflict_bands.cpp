#include "conflict_display.hpp"
#include "conflict_surface.hpp"
#include <algorithm>
#include <cmath>
namespace trafficsim {
std::vector<Point> conflictBandOutline(const Network& n,const ConflictSide& side) {
    try {
        const auto s=conflictSurface(n,side.path);if(!s)return {};
        if(s->left.size()<2 || s->left.size()!=s->right.size() || s->left.size()!=s->base.size())return {};
        const auto inset=[&](const auto& edge,const auto& other) {
            std::vector<double> offsets;double station=0;
            for(std::size_t i=0;i<edge.size();++i) {
                if(i)station+=std::hypot(edge[i].x-edge[i-1].x,edge[i].y-edge[i-1].y);
                const auto u=directionAlong(edge,station,i+1==edge.size());
                const double across=u.x*(other[i].y-edge[i].y)-u.y*(other[i].x-edge[i].x);
                offsets.push_back(std::copysign(std::min(kConflictRailOffset,std::abs(across)*0.2),across));
            }
            return offsetGeometry(edge,offsets);
        };
        const auto left=inset(s->left,s->right),right=inset(s->right,s->left);
        auto out=polylineSpan(left,matchedStation(s->base,left,side.entryStation),matchedStation(s->base,left,side.exitStation));
        const auto other=polylineSpan(right,matchedStation(s->base,right,side.entryStation),matchedStation(s->base,right,side.exitStation));
        out.insert(out.end(),other.rbegin(),other.rend());return out;
    }catch(const std::exception&){return {};}
}
}
