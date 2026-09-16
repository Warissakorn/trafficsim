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
// Vissim moves the one poly point that is attached to the Link, and leaves the rest of the
// Connector's poly points where the author put them. So does this. It is also what makes the
// result independent of the path taken: a link moved away and back puts that point back, and
// nothing else was ever touched. Carrying the whole curve rigidly, as this used to, dragged
// hand-placed points around a Link edit they had nothing to do with.
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
    if(!std::isfinite(from.x) || !std::isfinite(from.y) || !std::isfinite(to.x) || !std::isfinite(to.y))
        throw std::invalid_argument("INVALID_GEOMETRY");
    c.geometry.front()=from;c.geometry.back()=to;
}
namespace {
// The travel direction of a lane at the station a Connector attaches to it.
Point endDirection(const Network& network,const LaneReference& ref,bool outgoing) {
    for(const auto& link:network.links)if(link.id==ref.linkId) {
        const auto lane=laneGeometry(link,ref.laneId,network.drivingSide);
        if(lane.size()<2)throw std::invalid_argument("INVALID_GEOMETRY");
        const double length=polylineLength(lane);
        const double station=matchedStation(link.geometry,lane,attachmentStation(network,ref,outgoing));
        const auto a=pointAlong(lane,std::max(0.,station-0.01)),b=pointAlong(lane,std::min(length,station+0.01));
        const double dx=b.x-a.x,dy=b.y-a.y,span=std::hypot(dx,dy);
        if(!std::isfinite(span) || span<=0)throw std::invalid_argument("INVALID_GEOMETRY");
        return {dx/span,dy/span};
    }
    throw std::invalid_argument("UNKNOWN_LANE");
}
}
std::pair<Point,Point> connectorTangents(const Network& network,const LaneReference& from,const LaneReference& to) {
    return {endDirection(network,from,true),endDirection(network,to,false)};
}
std::vector<Point> connectorCurve(const Network& network, const LaneReference& from, const LaneReference& to,
                                  int intermediatePoints) {
    if(intermediatePoints<0 || intermediatePoints>40)throw std::invalid_argument("EDIT_CONNECTOR_POINTS");
    const auto a = laneAttachment(network,from,true), b = laneAttachment(network,to,false);
    const double gap = std::hypot(b.x-a.x, b.y-a.y);
    if (!std::isfinite(gap) || gap < 1e-6) throw std::invalid_argument("EDIT_CONNECTOR_GAP");
    const auto [entry,exit]=connectorTangents(network,from,to);
    const auto control = [&](Point origin, Point direction, double reach, double sign) {
        return Point{origin.x + sign*direction.x*reach, origin.y + sign*direction.y*reach};
    };
    // Reach each control point the way a circular arc would, reading each end on its own: for a
    // tangent that leaves the chord at alpha, the cubic approximating that arc uses
    // (2/3)*gap*tan(alpha/2)/sin(alpha). It tends to gap/3 as alpha tends to 0, the constant this
    // used for every turn, and for a symmetric turn alpha is half the turn at both ends, which is
    // the arc formula this used before -- so ordinary turns are unchanged to the last bit. What it
    // adds is the reverse curve, where the two tangents are parallel and the turn between them
    // says nothing, while each end still leaves its chord at a steep angle.
    const Point chord{(b.x-a.x)/gap,(b.y-a.y)/gap};
    const auto reach=[&](Point tangent) {
        const double alpha=std::abs(std::atan2(tangent.x*chord.y-tangent.y*chord.x,
                                               tangent.x*chord.x+tangent.y*chord.y));
        // The arc reach runs away as the tangent turns back down the chord: it is 0.67 of the
        // chord at a right angle, 1.33 at 120 degrees and 11.05 at 160, which is a curve that
        // leaves the junction altogether -- measured at 11.0 times its own chord on a Connector
        // drawn between two links that nearly touch. Hold it at the 120-degree value: every
        // ordinary turn, U-turn included, is unchanged to the last bit, and a hairpin stays
        // inside a corridor the author can see.
        return alpha<1e-6?gap/3:gap*std::min(2./3*std::tan(alpha/2)/std::sin(alpha),4./3);
    };
    const auto c1 = control(a, entry, reach(entry), 1);
    const auto c2 = control(b, exit, reach(exit), -1);
    // The points are laid on the arc; the Connector is drawn straight between them. More points
    // follow the arc more closely, which is the whole meaning of the count.
    const int spans=intermediatePoints+1;
    std::vector<Point> points{a};
    for (int i = 1; i < spans; ++i) {
        const double t = static_cast<double>(i)/spans, s = 1-t;
        points.push_back({s*s*s*a.x + 3*s*s*t*c1.x + 3*s*t*t*c2.x + t*t*t*b.x,
                          s*s*s*a.y + 3*s*s*t*c1.y + 3*s*t*t*c2.y + t*t*t*b.y});
    }
    points.push_back(b);
    return points;
}

}
