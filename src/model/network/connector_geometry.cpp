#include "network.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace trafficsim {
double attachmentStation(const Network& network,const LaneReference& ref,bool outgoing) {
    for(const auto& link:network.links)if(link.id==ref.linkId) {
        const double reference=polylineLength(link.geometry);
        if(!ref.station)return outgoing?reference:0.;
        if(!std::isfinite(*ref.station) || *ref.station<0 || *ref.station>reference)
            throw std::invalid_argument("EDIT_CONNECTOR_POSITION");
        return *ref.station;
    }
    throw std::invalid_argument("UNKNOWN_LANE");
}
bool attachedAtLinkEnd(const Network& network,const LaneReference& ref,bool outgoing) {
    for(const auto& link:network.links)if(link.id==ref.linkId)
        return !ref.station || *ref.station==(outgoing?polylineLength(link.geometry):0.);
    return false;
}
Point laneAttachment(const Network& network, const LaneReference& ref, bool outgoing) {
    const double station=attachmentStation(network,ref,outgoing);
    for(const auto& link:network.links)if(link.id==ref.linkId) {
        const auto geometry=laneGeometry(link,ref.laneId,network.drivingSide);
        // The ends are exact: station 0 and the reference length map to the lane's own ends.
        if(station<=0)return geometry.front();
        if(station>=polylineLength(link.geometry))return geometry.back();
        return pointAlong(geometry,matchedStation(link.geometry,geometry,station));
    }
    throw std::invalid_argument("UNKNOWN_LANE");
}
int lanesFromReference(const Network& network,const LaneReference& ref) {
    for(const auto& link:network.links)if(link.id==ref.linkId) {
        const auto lane=std::find_if(link.lanes.begin(),link.lanes.end(),[&](const auto& l){return l.id==ref.laneId;});
        return lane==link.lanes.end()?0:static_cast<int>(std::distance(lane,link.lanes.end()));
    }
    return 0;
}
// Writing each interior point as the complex number z = (p-a)/(b-a) makes z an invariant of
// the curve's shape, so the transform depends only on where the endpoints are now, never on
// how they got there: moving a link away and back restores the curve exactly. A displacement
// blend relative to the current geometry cannot do this — it composes, so a round trip
// through two edits silently deformed hand-tuned curves.
void reanchorConnector(const Network& network,Connector& c) {
    // A link can be shortened past an attachment. Clamp rather than reject the link edit: the
    // Connector survives at the new end, which is where the author can see and move it.
    const auto clamp=[&](LaneReference& ref) {
        if(!ref.station)return;
        for(const auto& link:network.links)if(link.id==ref.linkId)
            ref.station=std::clamp(*ref.station,0.,polylineLength(link.geometry));
    };
    clamp(c.from);clamp(c.to);
    const auto from=laneAttachment(network,c.from,true), to=laneAttachment(network,c.to,false);
    if(c.geometry.size()<2)throw std::invalid_argument("INVALID_GEOMETRY");
    const auto old=c.geometry;
    const auto a=old.front(), b=old.back();
    if(from==a && to==b)return;
    const double vx=b.x-a.x, vy=b.y-a.y, chord=vx*vx+vy*vy;
    const double wx=to.x-from.x, wy=to.y-from.y;
    if(!std::isfinite(chord) || chord<=0)throw std::invalid_argument("INVALID_GEOMETRY");
    for(std::size_t i=1;i+1<old.size();++i) {
        const double px=old[i].x-a.x, py=old[i].y-a.y;
        const double zr=(px*vx+py*vy)/chord, zi=(py*vx-px*vy)/chord;
        c.geometry[i]={from.x+zr*wx-zi*wy, from.y+zr*wy+zi*wx};
    }
    c.geometry.front()=from;c.geometry.back()=to;
}
std::vector<Point> connectorCurve(const Network& network, const LaneReference& from, const LaneReference& to) {
    const auto resolve = [&](const LaneReference& ref) {
        for (const auto& link : network.links) if (link.id == ref.linkId)
            return laneGeometry(link, ref.laneId, network.drivingSide);
        throw std::invalid_argument("UNKNOWN_LANE");
    };
    const auto source = resolve(from), target = resolve(to);
    if (source.size() < 2 || target.size() < 2) throw std::invalid_argument("INVALID_GEOMETRY");
    const auto a = laneAttachment(network,from,true), b = laneAttachment(network,to,false);
    const double gap = std::hypot(b.x-a.x, b.y-a.y);
    if (!std::isfinite(gap) || gap < 1e-6) throw std::invalid_argument("EDIT_CONNECTOR_GAP");
    const auto unit = [](Point previous, Point next) {
        const double dx = next.x-previous.x, dy = next.y-previous.y, length = std::hypot(dx, dy);
        if (!std::isfinite(length) || length <= 0) throw std::invalid_argument("INVALID_GEOMETRY");
        return Point{dx/length, dy/length};
    };
    const auto control = [&](Point origin, Point direction, double reach, double sign) {
        return Point{origin.x + sign*direction.x*reach, origin.y + sign*direction.y*reach};
    };
    const auto direction = [&](const std::vector<Point>& geometry,const LaneReference& ref,bool outgoing) {
        const double length=polylineLength(geometry);
        double station=length;
        for(const auto& link:network.links)if(link.id==ref.linkId)
            station=matchedStation(link.geometry,geometry,attachmentStation(network,ref,outgoing));
        return std::pair{pointAlong(geometry,std::max(0.,station-0.01)),
                         pointAlong(geometry,std::min(length,station+0.01))};
    };
    const auto [s0,s1]=direction(source,from,true);
    const auto [t0,t1]=direction(target,to,false);
    const auto entry=unit(s0,s1), exit=unit(t0,t1);
    // Reach each control point the way a circular arc would, reading each end on its own: for a
    // tangent that leaves the chord at alpha, the cubic approximating that arc uses
    // (2/3)*gap*tan(alpha/2)/sin(alpha). It tends to gap/3 as alpha tends to 0, the constant this
    // used for every turn, and for a symmetric turn alpha is half the turn at both ends, which is
    // the arc formula this used before -- so ordinary turns are unchanged to the last bit. What it
    // adds is the reverse curve, where the two tangents are parallel and the turn between them
    // says nothing, while each end still leaves its chord at a steep angle.
    const Point chord{(b.x-a.x)/gap,(b.y-a.y)/gap};
    const auto reach=[&](Point tangent) {
        // Beyond this the tangent points back down the chord and the arc length runs away.
        const double alpha=std::min(std::abs(std::atan2(tangent.x*chord.y-tangent.y*chord.x,
                                                        tangent.x*chord.x+tangent.y*chord.y)),2.8);
        return alpha<1e-6?gap/3:2./3*gap*std::tan(alpha/2)/std::sin(alpha);
    };
    const auto c1 = control(a, entry, reach(entry), 1);
    const auto c2 = control(b, exit, reach(exit), -1);
    std::vector<Point> points{a};
    for (int i = 1; i < 12; ++i) {
        const double t = i/12.0, s = 1-t;
        points.push_back({s*s*s*a.x + 3*s*s*t*c1.x + 3*s*t*t*c2.x + t*t*t*b.x,
                          s*s*s*a.y + 3*s*s*t*c1.y + 3*s*t*t*c2.y + t*t*t*b.y});
    }
    points.push_back(b);
    return points;
}
}
