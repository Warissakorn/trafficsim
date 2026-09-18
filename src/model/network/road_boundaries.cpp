#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
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
double cross(Point a,Point b){return a.x*b.y-a.y*b.x;}
// As kMiterLimit bounds a corner that would spike to infinity, this bounds a mouth. The slide
// runs away as the arrival turns to face along the Link's cross-section; four times the boundary's
// own offset is where it stops being a mouth and starts being a spike.
constexpr double kMouthShiftLimit=4.;
// How far the mouth's cross-section may be compressed to land on the Link's. 0.5 is a 60-degree
// arrival; below it no ribbon of finite width can carry the Link's lane spacing, because the
// Link's lanes stack along the Connector's own direction there.
// How many times the mouth offsets are re-solved against the boundary legs they actually produce.
// Measured on a two-lane 75-degree turn, worst lane middle off its Link lane's: 1.23 m with no
// pass, 0.49 m after one, 0.06 m after three -- and 6 cm is the worst case over the whole sweep,
// where an ordinary arrival is already below a millimetre.
constexpr int kMouthPasses=8;
// The least of its own width a mouth may be compressed to in order to meet the Link's lanes.
constexpr double kMouthSpanFloor=0.25;
Point unitStep(Point a,Point b) {
    const double length=std::hypot(a.x-b.x,a.y-b.y);
    return length>0?Point{(a.x-b.x)/length,(a.y-b.y)/length}:Point{0,0};
}
// The slide must stay monotone along each boundary, which needs |s| < zone. The margin is the
// slack that keeps the worst boundary strictly inside that, not on it.
constexpr double kMouthZoneMargin=1.25;
std::vector<double> stationsOf(const std::vector<Point>& p) {
    std::vector<double> result(p.size(),0.);
    for(std::size_t i=1;i<p.size();++i)result[i]=result[i-1]+std::hypot(p[i].x-p[i-1].x,p[i].y-p[i-1].y);
    return result;
}
// pointAlong with the ends extrapolated rather than clamped. At an oblique mouth about half the
// boundaries have to reach BEHIND the mouth to meet the Link, and the extension is straight, so
// the point it lands on is exactly on the Link's cross-section rather than near it.
Point alongExtended(const std::vector<Point>& p,const std::vector<double>& at,double station) {
    const auto lerp=[&](std::size_t a,std::size_t b,double t) {
        return Point{p[a].x+(p[b].x-p[a].x)*t,p[a].y+(p[b].y-p[a].y)*t};
    };
    const std::size_t last=p.size()-1;
    if(station<=0) {
        std::size_t b=1;while(b<last && at[b]<=0)++b;
        return at[b]>0?lerp(0,b,station/at[b]):p.front();
    }
    if(station>=at[last]) {
        std::size_t a=last;while(a>0 && at[last]-at[a-1]<=0)--a;
        if(a==0 && at[last]<=0)return p.back();
        const std::size_t previous=a>0?a-1:0;
        const double length=at[last]-at[previous];
        return length>0?lerp(previous,last,(station-at[previous])/length):p.back();
    }
    std::size_t i=1;while(i<last && at[i]<station)++i;
    const double length=at[i]-at[i-1];
    return length>0?lerp(i-1,i,(station-at[i-1])/length):p[i];
}
// The end of one boundary, the direction its station grows in there, and the Link's cross-section
// through the attachment. Everything the slide needs, read once per boundary per end.
struct MouthEnd { Point at,along; };
MouthEnd mouthEnd(const std::vector<Point>& b,bool start) {
    const Point p=start?b.front():b.back(),q=start?b[1]:b[b.size()-2];
    // The boundary's OWN end leg, never the spine's: where a lane tapers, its edge leans off the
    // ribbon by construction, and solving against the ribbon would leave that one boundary short.
    const Point step=start?Point{q.x-p.x,q.y-p.y}:Point{p.x-q.x,p.y-q.y};
    const double length=std::hypot(step.x,step.y);
    return {p,length>0?Point{step.x/length,step.y/length}:Point{0,0}};
}
}
namespace {
// The widths with the authored ones honoured (`authored=true`) or ignored. The mouth needs the
// second: what the Links themselves give, which is the only cross-section that can meet them.
ConnectorLaneWidths laneWidthsOf(const Network& n,const Connector& c,bool authored);
}
ConnectorLaneWidths connectorLaneWidths(const Network& n,const Connector& c) { return laneWidthsOf(n,c,true); }
namespace {
ConnectorLaneWidths laneWidthsOf(const Network& n,const Connector& c,bool useAuthored) {
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
        const bool authored=useAuthored && i<c.laneWidths.size();
        widths.source[i]=surplusSource?0:authored?c.laneWidths[i]:laneWidthOf(n,paths[i].from);
        widths.target[i]=surplusTarget?0:authored?c.laneWidths[i]:laneWidthOf(n,paths[i].to);
    }
    return widths;
}
// How far each boundary must slide ALONG ITS OWN offset curve for its end to land on the Link's
// cross-section, and how much of that slide there is room to spend.
//
// This is not M1.17's wedge. That moved the end vertices ACROSS the ribbon, onto the Link's lane
// edges, which re-aimed each boundary's last leg and let neighbouring boundaries cross -- the fold
// that closed the mouth to a point. A slide along the curve never changes a vertex's lateral
// offset, so the boundaries keep their order and cannot cross each other at all. What it costs is
// that the mouth spreads along the cross-section by |d|/|c.n| instead of |d|: the mouth is flush
// with the Link but wider than the lanes it feeds, and its lane edges land outside the Link's.
// That trade was the owner's, made with the numbers in front of them (M1.18).
ConnectorMouthFit fitMouth(const std::vector<std::vector<Point>>& boundaries,Point anchor,Point line,bool start) {
    ConnectorMouthFit fit;
    fit.shift.resize(boundaries.size());
    double worst=0;
    for(std::size_t i=0;i<boundaries.size();++i) {
        const auto end=mouthEnd(boundaries[i],start);
        // s solves ((at + s*along) - anchor) x line == 0. The denominator vanishes as the boundary
        // turns to face along the cross-section itself, where no finite slide reaches the line;
        // the limit below is what stops a mouth becoming a spike there.
        const double turn=cross(end.along,line);
        const double room=kMouthShiftLimit*std::hypot(end.at.x-anchor.x,end.at.y-anchor.y);
        const double reach=cross(Point{anchor.x-end.at.x,anchor.y-end.at.y},line);
        double s=std::abs(turn)>1e-9?reach/turn:(reach>=0?room:-room);
        // A degenerate end leg has no direction to slide along, so it does not move. This is a
        // lane tapered to nothing, whose end sits on its neighbour's and is carried by it.
        if(!std::isfinite(s) || (end.along.x==0 && end.along.y==0))s=0;
        fit.shift[i]=std::clamp(s,-room,room);
        worst=std::max(worst,std::abs(fit.shift[i]));
    }
    fit.zone=kMouthZoneMargin*worst;   // |s| < zone is what keeps the slide monotone.
    return fit;
}
// Spend what the Connector can actually afford. Every shift at one end scales by the same factor,
// so a mouth short of its Link is still one straight line -- it has simply not arrived yet, and
// `residual` says by how much rather than leaving it to be noticed.
void spendMouth(ConnectorMouthFit& fit,const std::vector<std::vector<Point>>& boundaries,Point anchor,
                Point line,bool start,double affordable) {
    const double scale=fit.zone>0?std::min(1.,affordable/fit.zone):1;
    fit.zone*=scale;
    for(std::size_t i=0;i<boundaries.size();++i) {
        fit.shift[i]*=scale;
        const auto end=mouthEnd(boundaries[i],start);
        // What still stands off the Link, square to its cross-section, in metres. Exact even where
        // the solve above was unbounded, because this never divides by the vanishing term.
        fit.residual=std::max(fit.residual,
            std::abs(cross(Point{anchor.x-end.at.x,anchor.y-end.at.y},line)-fit.shift[i]*cross(end.along,line)));
    }
}
void shearMouth(std::vector<std::vector<Point>>& boundaries,const ConnectorMouthFit& fit,bool start) {
    if(fit.zone<=0)return;
    // A lane tapered to nothing ends ON its neighbour and must leave on it too. The two share an
    // end point but not a curve, so sliding each along its own by the same distance still parts
    // them by millimetres and prises the closed lane back open. Noted here, re-closed below.
    std::vector<bool> closed(boundaries.size(),false);
    for(std::size_t i=1;i<boundaries.size();++i) {
        const auto a=mouthEnd(boundaries[i-1],start).at,b=mouthEnd(boundaries[i],start).at;
        closed[i]=std::hypot(b.x-a.x,b.y-a.y)<1e-9;
    }
    for(std::size_t i=0;i<boundaries.size();++i) {
        if(fit.shift[i]==0)continue;   // A parallel arrival is left bit for bit as it was.
        const auto at=stationsOf(boundaries[i]);
        const double length=at.back();
        std::vector<Point> moved(boundaries[i].size());
        for(std::size_t j=0;j<moved.size();++j) {
            const double from=start?at[j]:length-at[j];
            const double decay=std::clamp(1-from/fit.zone,0.,1.);
            moved[j]=alongExtended(boundaries[i],at,at[j]+fit.shift[i]*decay);
        }
        boundaries[i]=std::move(moved);
    }
    for(std::size_t i=1;i<boundaries.size();++i)if(closed[i]) {
        auto& edge=boundaries[i];
        (start?edge.front():edge.back())=start?boundaries[i-1].front():boundaries[i-1].back();
    }
}
// The ribbon before either mouth is corrected: parallel-sided, square to its own axis, which is
// what the body is and stays. Both public entry points below start here, so a reported number and
// a drawn edge can never come from two different ribbons.
struct SquareRibbon { std::vector<std::vector<Point>> boundaries; std::vector<Point> spine; };
SquareRibbon squareRibbon(const Network& n,const Connector& c) {
    const auto paths=connectorPaths(n,c);const auto weights=connectorBlendWeights(c);
    const std::size_t count=paths.size();
    const auto widths=connectorLaneWidths(n,c);
    const auto& source=widths.source;const auto& target=widths.target;
    const auto from=endCross(n,c.from,c.fromLaneCount,true);
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
    const double entry=std::atan2(from.y,from.x);
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
    const auto stack=[&](const ConnectorLaneWidths& w,double t) {
        std::vector<double> o(count+1);
        const auto width=[&](std::size_t i){return w.source[i]+(w.target[i]-w.source[i])*t;};
        o[anchorLane]=-width(anchorLane)/2;
        for(std::size_t i=anchorLane;i-->0;)o[i]=o[i+1]-width(i);
        for(std::size_t i=anchorLane;i<count;++i)o[i+1]=o[i]+width(i);
        for(std::size_t i=0;i<=count;++i)o[i]*=sign;
        return o;
    };
    // THE MOUTH'S CROSS-SECTION IS THE LINK'S OWN, READ ONTO THE CONNECTOR'S.
    //
    // M1.18 slid each boundary along its own curve until it met the Link's cross-section, which
    // made the mouth flush -- but every boundary kept its full offset square to the Connector, so
    // once resolved onto that oblique cut the lanes came out spread by 1/cos(arrival), and the
    // middle of each Connector lane landed BESIDE the middle of the Link lane it feeds. That is
    // what the owner saw in the editor, and it is what this fixes.
    //
    // The offset a boundary must leave the mouth at, for the slide to land it exactly on the
    // Link's own lane boundary, is that boundary's position PROJECTED onto the Connector's
    // cross-section: the slide moves along the Connector's direction, which adds nothing in the
    // normal direction, so the normal component is all that has to agree. One projection per
    // boundary, no solve, and it is exact for lanes of unequal width as well -- the Link's lane
    // spacing is read, not assumed. Lane middles coincide because every boundary does.
    //
    // The Link's widths are what the mouth is built from, never an authored one (M1.12.3): a
    // cross-section built from a width the Link does not have cannot coincide with the Link's
    // lanes, however it is slid. The authored width takes over through the body over the zone
    // below, so a Connector given a width still runs at it everywhere its own road is its own --
    // it simply starts and ends on the road it joins.
    const auto linkWidths=laneWidthsOf(n,c,false);
    const Point exit=endCross(n,c.to,c.toLaneCount,false);
    // WHERE EACH BOUNDARY MUST LAND, measured ALONG the Link's own cross-section.
    //
    // The middle of the Connector's lane k goes on the middle of the Link lane it joins, and its
    // edges on that lane's edges -- which is the whole of what the owner asked for. Along the cut
    // the lane is the Link's own width, because the cut is where the two roads meet; square to the
    // Connector it reads that times the cosine of the arrival, which is what a road crossing
    // another at an angle measures and not a narrowing.
    //
    // A lane the Link has no room for has no width at this end, so its middle sits on its
    // neighbour's and the divider between them closes there, which is what a taper is.
    struct Mouth { std::vector<double> offsets; std::vector<Point> at; };
    const auto mouthOffsets=[&](const LaneReference&,const std::vector<double>& link,
                                const std::vector<double>& own,Point at,Point along,Point cross) {
        const Point normal{-along.y,along.x};
        Mouth mouth{std::vector<double>(count+1),std::vector<Point>(count+1)};
        // Each lane's middle along the cross-section, from the first lane of the range's outer
        // edge, then rebased on the lane the spine is drawn down so the two agree at the anchor.
        std::vector<double> middle(count);
        double walked=0;
        for(std::size_t i=0;i<count;++i){middle[i]=walked+link[i]/2;walked+=link[i];}
        const double base=middle[anchorLane];   // Read once: the loop below moves it.
        for(std::size_t i=0;i<count;++i)middle[i]-=base;
        // The boundary between two lanes is where their edges meet -- one line, and the same line
        // either lane's word gives when the two widths agree, which is every Connector that has
        // not been given a width of its own.
        for(std::size_t i=0;i<=count;++i) {
            const double position=i==0?middle[0]-own[0]/2
                                 :i==count?middle[count-1]+own[count-1]/2
                                 :(middle[i-1]+own[i-1]/2+middle[i]-own[i]/2)/2;
            mouth.at[i]={at.x+cross.x*position,at.y+cross.y*position};
            mouth.offsets[i]=(mouth.at[i].x-at.x)*normal.x+(mouth.at[i].y-at.y)*normal.y;
        }
        return mouth;
    };
    const Point uSource=unitStep(spine[1],spine[0]),uTarget=unitStep(spine[spine.size()-1],spine[spine.size()-2]);
    // Both the middles and the widths at a mouth are the LINK'S, never an authored one. This is
    // M1.12.3: a lane laid at a width the Link does not have cannot have its middle on the Link's
    // lane AND its edges on the Link's edges -- the two only coincide when the widths do. The
    // Link wins where the two roads meet, and the authored width takes over through the body.
    const auto atSource=mouthOffsets(c.from,linkWidths.source,linkWidths.source,spine.front(),uSource,from);
    const auto atTarget=mouthOffsets(c.to,linkWidths.target,linkWidths.target,spine.back(),uTarget,exit);
    // The zone the mouth's cross-section opens out to the Connector's own over. Two carriageway
    // widths is the length a lane taper is drawn over; capped so the two zones cannot meet.
    const auto span=[&](const std::vector<double>& w) {
        double total=0;for(double v:w)total+=v;return total;
    };
    const double spineLength=polylineLength(spine);
    // The length the mouth's cross-section opens out to the Connector's own over. One carriageway
    // width is the shortest run that does not read as a kink in the edge, and a quarter of the
    // Connector each end leaves half of it running at its own width even on a short one.
    const double zone=std::min(0.25*spineLength,std::max(span(linkWidths.source),span(linkWidths.target)));
    const auto stations=stationsOf(spine);
    const auto table=[&](const std::vector<double>& source,const std::vector<double>& target) {
        std::vector<std::vector<double>> offsets(count+1,std::vector<double>(spine.size()));
        for(std::size_t j=0;j<spine.size();++j) {
            const double t=weights[j];
            const auto body=stack(widths,t);
            const double toSource=zone>0?std::clamp(1-stations[j]/zone,0.,1.):0;
            const double toTarget=zone>0?std::clamp(1-(stations.back()-stations[j])/zone,0.,1.):0;
            for(std::size_t i=0;i<=count;++i)
                offsets[i][j]=body[i]+(source[i]-body[i])*toSource+(target[i]-body[i])*toTarget;
        }
        return offsets;
    };
    const auto draw=[&](const std::vector<std::vector<double>>& offsets) {
        std::vector<std::vector<Point>> ribbon;
        for(std::size_t i=0;i<=count;++i)ribbon.push_back(offsetGeometry(spine,offsets[i]));
        return ribbon;
    };
    // The projection above assumed the slide runs along the spine's own end leg. It does not
    // quite: a boundary whose offset is still opening out from the mouth leans off the spine by
    // however fast it opens, and the slide follows THAT leg. So draw the ribbon once, read each
    // boundary's real end direction, and re-solve the offset that puts its end on the Link's
    // boundary when slid along it. One correction pass takes the worst lane middle from 1.23 m to
    // millimetres. A fixed number of passes, never a convergence test: the same drawing must come
    // out of the same network on every machine and every run (hard rule 2), and a loop that stops
    // when it is "close enough" is exactly the kind of thing that stops at a different place.
    const auto refine=[&](const std::vector<std::vector<Point>>& ribbon,const std::vector<double>& first,
                          Point anchor,Point along,bool start,const std::vector<Point>& target) {
        const Point normal{-along.y,along.x};
        auto result=first;
        for(std::size_t i=0;i<=count;++i) {
            const auto leg=mouthEnd(ribbon[i],start).along;
            const double turn=normal.x*leg.y-normal.y*leg.x;
            if(std::abs(turn)<1e-9)continue;   // No end leg to slide along; keep the projection.
            result[i]=((target[i].x-anchor.x)*leg.y-(target[i].y-anchor.y)*leg.x)/turn;
        }
        return result;
    };
    // As the arrival turns to face along the Link's cross-section, the Link's lane boundaries all
    // project onto the SAME point of the Connector's, and a mouth built from that projection is a
    // spike -- the failure kMiterLimit bounds at a corner and kMouthShiftLimit bounds for the
    // slide. Below half the Connector's own width the compression stops: past about 60 degrees the
    // mouth is as near the Link's lanes as a road of finite width gets, and stays a mouth.
    const auto bodySource=stack(widths,0),bodyTarget=stack(widths,1);
    const auto floored=[&](std::vector<double> o,const std::vector<double>& body) {
        // An ill-conditioned solve runs away instead of merely being large: at an arrival facing
        // along the Link's cross-section the offset that would reach it is unbounded, and one
        // reading of 3.6e7 metres was measured before this cap. Four times the Connector's own
        // half-width is where a mouth stops being a mouth, exactly as kMiterLimit bounds a corner.
        const double reach=kMouthShiftLimit*std::max(std::abs(body[0]),std::abs(body[count]));
        for(auto& v:o)v=std::isfinite(v)?std::clamp(v,-reach,reach):0;
        if((o[count]-o[0])*(body[count]-body[0])<0)return body;
        const double want=kMouthSpanFloor*std::abs(body[count]-body[0]),span=std::abs(o[count]-o[0]);
        if(span>=want)return o;
        if(span<=1e-12)return body;   // Collapsed altogether: the Connector's own cross-section.
        const double middle=(o[count]+o[0])/2,scale=want/span;
        for(auto& v:o)v=middle+(v-middle)*scale;
        return o;
    };
    auto mouthSource=floored(atSource.offsets,bodySource),mouthTarget=floored(atTarget.offsets,bodyTarget);
    for(int pass=0;pass<kMouthPasses;++pass) {
        const auto drawn=draw(table(mouthSource,mouthTarget));
        mouthSource=floored(refine(drawn,mouthSource,spine.front(),uSource,true,atSource.at),bodySource);
        mouthTarget=floored(refine(drawn,mouthTarget,spine.back(),uTarget,false,atTarget.at),bodyTarget);
    }
    const auto offsets=table(mouthSource,mouthTarget);
    std::vector<std::vector<Point>> result;
    for(std::size_t i=0;i<=count;++i)result.push_back(offsetGeometry(spine,offsets[i]));
    return {std::move(result),spine};
}
// Fit both mouths onto their Links and slide the ribbon into them. The two ends share the spine
// rather than taking half each, so an ordinary end beside a steep one still aligns exactly; when
// together they want more than there is, both shrink by the same factor. Either way the two zones
// sum to at most the whole length, so they meet at worst at a point and never overlap.
ConnectorMouthFits fitAndShear(const Network& n,const Connector& c,SquareRibbon& ribbon) {
    const Point entry=endCross(n,c.from,c.fromLaneCount,true),exit=endCross(n,c.to,c.toLaneCount,false);
    ConnectorMouthFits fits{fitMouth(ribbon.boundaries,ribbon.spine.front(),entry,true),
                            fitMouth(ribbon.boundaries,ribbon.spine.back(),exit,false)};
    const double length=polylineLength(ribbon.spine),wanted=fits.source.zone+fits.target.zone;
    const double scale=wanted>length?length/wanted:1;
    spendMouth(fits.source,ribbon.boundaries,ribbon.spine.front(),entry,true,fits.source.zone*scale);
    spendMouth(fits.target,ribbon.boundaries,ribbon.spine.back(),exit,false,fits.target.zone*scale);
    // Both fits are read off the square ribbon before either is applied, so the two ends cannot
    // influence each other's numbers -- then applied, in a fixed order, for reproducibility.
    shearMouth(ribbon.boundaries,fits.source,true);
    shearMouth(ribbon.boundaries,fits.target,false);
    return fits;
}
}
std::vector<std::vector<Point>> connectorBoundaries(const Network& n,const Connector& c) {
    auto ribbon=squareRibbon(n,c);
    (void)fitAndShear(n,c,ribbon);
    return std::move(ribbon.boundaries);
}
ConnectorMouthFits connectorMouthFit(const Network& n,const Connector& c) {
    // Measured off the same ribbon connectorBoundaries draws, by the same call, so a reported
    // number cannot describe a mouth other than the one on screen (hard rule 3).
    auto ribbon=squareRibbon(n,c);
    return fitAndShear(n,c,ribbon);
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
