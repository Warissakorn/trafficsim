#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <stdexcept>
namespace trafficsim {
namespace {
// Use the lane's segment parameter, rather than boundary arclength, so a cross-section
// remains aligned at body attachments on a curved link.
Point edgeAt(const Network& n,const LaneReference& ref,int boundary,bool outgoing) {
    for(const auto& l:n.links)if(l.id==ref.linkId) {
        const auto lane=laneGeometry(l,ref.laneId,n.drivingSide);
        const auto edge=laneBoundaryGeometry(l,static_cast<std::size_t>(boundary),n.drivingSide);
        double remaining=matchedStation(l.geometry,lane,attachmentStation(n,ref,outgoing));
        for(std::size_t i=1;i<lane.size();++i) {
            const double length=std::hypot(lane[i].x-lane[i-1].x,lane[i].y-lane[i-1].y);
            if(length>0 && remaining<=length) {
                const double t=remaining/length;
                return {edge[i-1].x+(edge[i].x-edge[i-1].x)*t,edge[i-1].y+(edge[i].y-edge[i-1].y)*t};
            }
            remaining-=length;
        }
        return edge.back();
    }
    throw std::invalid_argument("UNKNOWN_LANE");
}
// The direction across the road at the end a reference attaches to: taken from the link's own
// lane edges, so the Connector's mouth meets the link flush instead of being pulled onto it.
Point endCross(const Network& n,const LaneReference& ref,int count,bool outgoing) {
    for(const auto& l:n.links)if(l.id==ref.linkId) {
        const auto first=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& lane){return lane.id==ref.laneId;});
        if(first==l.lanes.end() || std::distance(first,l.lanes.end())<count)throw std::invalid_argument("EDIT_LANE_RANGE");
        const int index=static_cast<int>(std::distance(l.lanes.begin(),first));
        auto last=ref;last.laneId=(first+count-1)->id;
        const auto a=edgeAt(n,ref,index,outgoing),b=edgeAt(n,last,index+count,outgoing);
        const double span=std::hypot(b.x-a.x,b.y-a.y);
        if(!std::isfinite(span) || span<=0)throw std::invalid_argument("INVALID_GEOMETRY");
        return {(b.x-a.x)/span,(b.y-a.y)/span};
    }
    throw std::invalid_argument("UNKNOWN_LANE");
}
double wrap(double angle) {
    while(angle>std::numbers::pi)angle-=2*std::numbers::pi;
    while(angle<-std::numbers::pi)angle+=2*std::numbers::pi;
    return angle;
}
double laneWidthOf(const Network& n,const LaneReference& ref) {
    for(const auto& l:n.links)if(l.id==ref.linkId)
        for(const auto& lane:l.lanes)if(lane.id==ref.laneId)return lane.width;
    throw std::invalid_argument("UNKNOWN_LANE");
}
// Where a boundary's own cut vertex sits is exact by construction -- fixed at the width the two
// links give it, measured along the link's own cross-section. That fixed-distance snap is only
// consistent with the polygon when the leg it lands at the end of is already headed roughly that
// way; at anything but a near-tangential merge angle it can imply an edge that runs backwards
// across the neighbouring boundary instead of towards it. Re-mitering that last leg against the
// link's cross-section -- extending it and cutting where it actually meets that line, the way
// offsetGeometry miters an interior corner against the next real segment -- keeps every boundary
// its own full width all the way to the link instead of pinching two of them together to dodge
// the fold, and does not move the cut at all when the fixed snap and the re-miter already agree.
std::optional<Point> legCrossing(Point a1,Point a2,Point b1,Point b2) {
    const double rx=a2.x-a1.x,ry=a2.y-a1.y,sx=b2.x-b1.x,sy=b2.y-b1.y;
    const double denominator=rx*sy-ry*sx;
    if(std::abs(denominator)<1e-12)return {};
    const double t=((b1.x-a1.x)*sy-(b1.y-a1.y)*sx)/denominator;
    const double u=((b1.x-a1.x)*ry-(b1.y-a1.y)*rx)/denominator;
    if(t<0||t>1||u<0||u>1)return {};
    return Point{a1.x+t*rx,a1.y+t*ry};
}
Point remiter(Point edgeFrom,Point edgeTo,Point crossAt,Point crossDir,Point fallback) {
    const double rx=edgeTo.x-edgeFrom.x,ry=edgeTo.y-edgeFrom.y;
    const double denominator=rx*crossDir.y-ry*crossDir.x;
    if(std::abs(denominator)<1e-9)return fallback;
    const double t=((crossAt.x-edgeFrom.x)*crossDir.y-(crossAt.y-edgeFrom.y)*crossDir.x)/denominator;
    return {edgeFrom.x+rx*t,edgeFrom.y+ry*t};
}
}
ConnectorLaneWidths connectorLaneWidths(const Network& n,const Connector& c) {
    const auto paths=connectorPaths(n,c);
    // A Connector carries lanes, not a ribbon that shrinks. Each lane keeps its width from end to
    // end; a lane the other end has no room for is the one that tapers, closing onto its neighbour
    // like a merge taper. Where two paths share a lane at one end, the second of them is the
    // surplus one, so its width there is zero.
    const std::size_t count=paths.size();
    ConnectorLaneWidths widths{std::vector<double>(count),std::vector<double>(count)};
    for(std::size_t i=0;i<count;++i) {
        const bool surplusSource=i && paths[i].from.laneId==paths[i-1].from.laneId;
        const bool surplusTarget=i && paths[i].to.laneId==paths[i-1].to.laneId;
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
std::vector<std::vector<Point>> connectorBoundaries(const Network& n,const Connector& c) {
    const auto paths=connectorPaths(n,c);const auto weights=connectorBlendWeights(c);
    const std::size_t count=paths.size();
    const auto widths=connectorLaneWidths(n,c);
    const auto& source=widths.source;const auto& target=widths.target;
    const auto from=endCross(n,c.from,c.fromLaneCount,true),to=endCross(n,c.to,c.toLaneCount,false);
    // Hang the cross-section on the last lane that is a real lane at both ends, and step out from
    // there in both directions. A lane added at the leading edge then cannot move the far edge,
    // and a lane that tapers is placed against its neighbour rather than on its own driving line,
    // which converges onto the lane it merges into and is no longer where that lane's edge is.
    std::size_t anchorLane=0;
    for(std::size_t i=0;i<count;++i)if(source[i]>0 && target[i]>0)anchorLane=i;
    // A constant offset from the axis, square to it at every point, is what a road is: the width
    // belongs to the Connector, not to the line between the links it joins. Interpolating the two
    // mouths' cross-sections through the body made the width depend on how far the path had swung
    // away from that line -- a lane drawn 1.06 m of its 3.50 m on a reverse curve, and 0.46 m once
    // a Link had been moved so the curve no longer left it straight. The correction onto each
    // link's own cross-section sits on the two end samples and goes no further in.
    const auto& spine=paths[anchorLane].geometry;
    const double entry=std::atan2(from.y,from.x),exit=std::atan2(to.y,to.x);
    // Which way a normal points is a convention; which way lane order runs is not. Take the
    // source mouth's word for it once, for the whole body, or the lanes come out mirrored.
    const auto raw=[&](std::size_t j) {
        const auto a=spine[j?j-1:0],b=spine[std::min(j+1,spine.size()-1)];
        return std::atan2(b.y-a.y,b.x-a.x)+std::numbers::pi/2;
    };
    const double sign=std::abs(wrap(entry-raw(0)))>std::numbers::pi/2?-1.:1.;
    // Each boundary is the axis offset by the lanes stacked up to it, mitered at every corner by
    // the same function a Link's own edges use -- so a lane is its full width square to the road
    // at every point, through a bend and past a poly point the author has dragged.
    std::vector<std::vector<double>> offsets(count+1,std::vector<double>(spine.size()));
    for(std::size_t j=0;j<spine.size();++j) {
        const double t=weights[j];
        const auto width=[&](std::size_t i){return source[i]+(target[i]-source[i])*t;};
        offsets[anchorLane][j]=-width(anchorLane)/2;
        for(std::size_t i=anchorLane;i-->0;)offsets[i][j]=offsets[i+1][j]-width(i);
        for(std::size_t i=anchorLane;i<count;++i)offsets[i+1][j]=offsets[i][j]+width(i);
        for(std::size_t i=0;i<=count;++i)offsets[i][j]*=sign;
    }
    std::vector<std::vector<Point>> result;
    for(std::size_t i=0;i<=count;++i)result.push_back(offsetGeometry(spine,offsets[i]));
    // The two ends belong to the links, not to the Connector: cut them on the link's own
    // cross-section, so a mouth is a wedge lying on its lane edges rather than a square end
    // standing clear of them. This is what Vissim draws -- confirmed against a screenshot of
    // a Connector arriving on a link body at an angle -- and squaring the ends to the Connector
    // instead left a step of 0.12-0.29 m between the mouth and the road. That fixed-distance cut
    // is exact and is kept whenever it leaves the cross-section's boundaries in their own order;
    // where two of them would cross instead (a merge angle far from tangential), every boundary
    // at that end re-miters its own last leg against the cross-section line instead, which keeps
    // each one its own full width rather than pinching two of them together to a shared point.
    // Both ends read their edge direction from this untouched copy, taken before either end is
    // cut, so a two-point spine -- its front and back leg being the same one segment -- reads
    // its own original direction at both ends rather than the other end's already-cut vertex.
    const auto pristine=result;
    const Point entryDir{std::cos(entry),std::sin(entry)},exitDir{std::cos(exit),std::sin(exit)};
    for(const bool front:{true,false}) {
        std::vector<Point> naive(count+1),cut(count+1);
        for(std::size_t i=0;i<=count;++i) {
            const double d=(front?offsets[i].front():offsets[i].back())*sign;
            naive[i]=front?Point{spine.front().x+entryDir.x*d,spine.front().y+entryDir.y*d}
                          :Point{spine.back().x+exitDir.x*d,spine.back().y+exitDir.y*d};
        }
        bool crosses=false;
        for(std::size_t i=0;!crosses && i<count;++i) {
            const auto& a=pristine[i];const auto& b=pristine[i+1];
            if(a.size()<2||b.size()<2)continue;
            // A tapering lane's cut point coincides exactly with its neighbour's by construction
            // (a surplus lane closes to zero width there) -- that shared endpoint is the taper
            // meeting cleanly, not a fold, and must not itself register as a crossing.
            if(std::hypot(naive[i].x-naive[i+1].x,naive[i].y-naive[i+1].y)<1e-9)continue;
            const Point a1=front?a[0]:a[a.size()-2],b1=front?b[0]:b[b.size()-2];
            if(legCrossing(a1,naive[i],b1,naive[i+1]))crosses=true;
        }
        // A re-miter is only as safe as the angle between the leg and the cross-section it is
        // extended to meet: near-parallel, the intersection can land many lane-widths away, a
        // spike shooting past the link instead of a mouth lying on it. Bound it to a few widths
        // of the whole cross-section, the same spirit as offsetGeometry's own miter clamp, and
        // fall back to the exact fixed-distance cut -- which can only leave the mild fold the
        // re-miter was trying to avoid, never an unbounded overshoot -- past that.
        const Point crossAt=front?spine.front():spine.back();
        const double limit=1.5*std::hypot(naive.back().x-naive.front().x,naive.back().y-naive.front().y);
        for(std::size_t i=0;i<=count;++i) {
            const auto& shape=pristine[i];
            if(!crosses||shape.size()<2){cut[i]=naive[i];continue;}
            const Point edgeFrom=front?shape[0]:shape[shape.size()-2],edgeTo=front?shape[1]:shape.back();
            cut[i]=remiter(edgeFrom,edgeTo,crossAt,front?entryDir:exitDir,naive[i]);
            if(limit>0 && std::hypot(cut[i].x-crossAt.x,cut[i].y-crossAt.y)>limit)cut[i]=naive[i];
        }
        for(std::size_t i=0;i<=count;++i)if(front)result[i].front()=cut[i];else result[i].back()=cut[i];
    }
    return result;
}
std::vector<ConnectorMarking> connectorMarkings(const Network& n,const Connector& c) {
    const auto boundaries=connectorBoundaries(n,c);
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
std::vector<Point> connectorCentreline(const Network& n,const Connector& c) {
    const auto boundaries=connectorBoundaries(n,c);
    std::vector<Point> result;
    for(std::size_t i=0;i<c.geometry.size();++i)
        result.push_back({(boundaries.front()[i].x+boundaries.back()[i].x)/2,
                          (boundaries.front()[i].y+boundaries.back()[i].y)/2});
    return result;
}
}
