#include "network.hpp"
#include "connector_surface.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
namespace trafficsim {
namespace {
double laneWidthOf(const Network& n,const LaneReference& ref) {
    for(const auto& l:n.links)if(l.id==ref.linkId)
        for(const auto& lane:l.lanes)if(lane.id==ref.laneId)return lane.width;
    throw std::invalid_argument("UNKNOWN_LANE");
}
}
ConnectorLaneWidths connectorLaneWidths(const Network& n,const Connector& c) {
    const auto paths=connectorPaths(n,c);
    // A Connector carries lanes, not a ribbon that shrinks. Each lane keeps its width from end to
    // end; a lane the other end has no room for is the one that tapers, closing onto its neighbour
    // like a merge taper. The surplus path is the added or dropped lane itself (D73): the one the
    // lane shift puts outside the narrower end's range, so the taper closes at that side's edge.
    const std::size_t count=paths.size();
    const int shift=connectorLaneShift(n,c);
    const auto surplus=[&](std::size_t i,int lanes) {
        const int j=static_cast<int>(i)-shift;
        return lanes<static_cast<int>(count) && (j<0 || j>=lanes);
    };
    ConnectorLaneWidths widths{std::vector<double>(count),std::vector<double>(count)};
    for(std::size_t i=0;i<count;++i) {
        const bool surplusSource=surplus(i,c.fromLaneCount);
        const bool surplusTarget=surplus(i,c.toLaneCount);
        // An authored width replaces the width the Links give, at both ends, so the lane runs at
        // the metre value the author typed. It does NOT fill in a surplus end: that zero is a
        // consequence of the lane counts, not a width the author chose, and overriding it would
        // draw a taper as a full-width lane ending in mid-air.
        const bool authored=i<c.laneWidths.size();
        widths.source[i]=surplusSource?0:authored?c.laneWidths[i]:laneWidthOf(n,paths[i].from);
        widths.target[i]=surplusTarget?0:authored?c.laneWidths[i]:laneWidthOf(n,paths[i].to);
    }
    return widths;
}

// File geometry remains the first lane's authored path (schema 17). Rebase it onto the
// centre of the WHOLE lane range before constructing any edges. No edge feeds this axis.
// This preserves old files and runtime paths without mistaking one lane for the road centre.
std::vector<Point> connectorCentreline(const Network& n,const Connector& c) {
    auto axis=c.geometry;
    const auto a=connectorRangeCentre(n,c,true),b=connectorRangeCentre(n,c,false);
    if(!a || !b)throw std::invalid_argument("EDIT_LANE_RANGE");
    const auto from=laneAttachment(n,c.from,true),to=laneAttachment(n,c.to,false);
    const auto weights=connectorBlendWeights(c);
    for(std::size_t j=0;j<axis.size();++j) {
        const double t=weights[j];
        axis[j].x+=(a->x-from.x)*(1-t)+(b->x-to.x)*t;
        axis[j].y+=(a->y-from.y)*(1-t)+(b->y-to.y)*t;
    }
    return axis;
}
std::vector<std::vector<Point>> connectorBodyBoundaries(const Network& n,const Connector& c) {
    const auto axis=connectorCentreline(n,c);
    const auto widths=connectorLaneWidths(n,c);const auto weights=connectorBlendWeights(c);
    const auto count=widths.source.size();
    std::vector<std::vector<double>> offsets(count+1,std::vector<double>(axis.size()));
    // Boundary 0 is the kerb-side edge; direction is traffic handedness, never a dot
    // product with already adjusted rails. It cannot flip when an end passes 90 degrees.
    const double sign=n.drivingSide==DrivingSide::left?-1.:1.;
    for(std::size_t j=0;j<axis.size();++j) {
        const double t=weights[j];
        double total=0;for(std::size_t i=0;i<count;++i)total+=widths.source[i]*(1-t)+widths.target[i]*t;
        double across=-total/2;
        for(std::size_t i=0;i<=count;++i) {
            offsets[i][j]=sign*across;
            if(i<count)across+=widths.source[i]*(1-t)+widths.target[i]*t;
        }
    }
    std::vector<std::vector<Point>> result;
    for(const auto& offset:offsets)result.push_back(offsetGeometry(axis,offset));
    return result;
}
std::vector<Point> connectorGeometryWithGrip(const Network& n,const Connector& c,std::size_t index,Point target) {
    if(index==0 || index+1>=c.geometry.size())throw std::invalid_argument("INVALID_GEOMETRY");
    auto edit=c;edit.laneBlend.clear();
    const auto a=*connectorRangeCentre(n,c,true),b=*connectorRangeCentre(n,c,false);
    const auto from=laneAttachment(n,c.from,true),to=laneAttachment(n,c.to,false);
    const Point da{a.x-from.x,a.y-from.y},db{b.x-to.x,b.y-to.y};
    // Only the blend parameter is unknown. For any t, place geometry so its centre is target;
    // solve t = arclengthWeight(geometry) on [0,1]. Fixed bisection is deterministic and does
    // not reuse the pre-drag offset after the angle/segment lengths have changed.
    double low=0,high=1;
    for(int pass=0;pass<64;++pass) {
        const double t=std::midpoint(low,high);
        edit.geometry[index]={target.x-da.x*(1-t)-db.x*t,target.y-da.y*(1-t)-db.y*t};
        if(connectorBlendWeights(edit)[index]>t)low=t;else high=t;
    }
    return edit.geometry;
}
std::vector<std::vector<Point>> connectorBoundaries(const Network& n,const Connector& c) {
    return connectorSurface(n,c).boundaries;
}
std::vector<ConnectorMarking> connectorMarkings(const Network& n,const Connector& c) {
    return connectorMarkings(c,connectorBoundaries(n,c));
}
std::vector<ConnectorMarking> connectorMarkings(const Connector& c,const std::vector<std::vector<Point>>& boundaries) {
    std::vector<ConnectorMarking> result;
    result.push_back({trimSelfIntersections(boundaries.front()),true,MarkingType::solid});
    for(std::size_t i=1;i+1<boundaries.size();++i) {
        // Every interior boundary now sits on a real lane edge for its whole length, because the
        // cross-section is built from lane widths. The one case with nothing to divide is a lane
        // with no width anywhere -- a range drawn onto lanes that are not there.
        double widest=0;
        for(std::size_t j=0;j<boundaries[i].size();++j)
            widest=std::max(widest,std::hypot(boundaries[i+1][j].x-boundaries[i][j].x,
                                              boundaries[i+1][j].y-boundaries[i][j].y));
        // An authored MarkingType names what is painted on this interior divider; the default is
        // the dashed line a lane divider has always been drawn with. i-1 because laneMarkings is
        // indexed by divider, and boundary i is the divider after lane i-1.
        const auto type=i-1<c.laneMarkings.size()?c.laneMarkings[i-1]:MarkingType::dashed;
        if(widest>1e-9)result.push_back({trimSelfIntersections(boundaries[i]),false,type});
    }
    if(boundaries.size()>1)result.push_back({trimSelfIntersections(boundaries.back()),true,MarkingType::solid});
    return result;
}
}
