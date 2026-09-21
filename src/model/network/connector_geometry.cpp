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
void retargetConnector(const Network& network,Connector& c,LaneReference from,LaneReference to) {
    if(c.from==from && c.to==to)return;
    for(const auto& other:network.connectors)
        if(other.id!=c.id && other.from==from && other.to==to)
            throw std::invalid_argument("DUPLICATE_CONNECTION");
    auto moved=c;moved.from=std::move(from);moved.to=std::move(to);
    moved.fromLaneCount=std::min(c.fromLaneCount,lanesFromReference(network,moved.from));
    moved.toLaneCount=std::min(c.toLaneCount,lanesFromReference(network,moved.to));
    if(moved.fromLaneCount<1 || moved.toLaneCount<1)throw std::invalid_argument("UNKNOWN_LANE");
    if(std::max(moved.fromLaneCount,moved.toLaneCount)!=std::max(c.fromLaneCount,c.toLaneCount)) {
        moved.laneWidths.clear();moved.laneMarkings.clear();
    }
    // Keeping the old interior points makes the last leg run backwards when the attachment
    // crosses them. Retarget is a topology gesture: rebuild its directed turn, with Undo
    // retaining the complete old shape. Manual interior-point edits remain manual.
    if(c.geometry.size()<2)throw std::invalid_argument("INVALID_GEOMETRY");
    moved.geometry=connectorCurve(network,moved.from,moved.to,
        static_cast<int>(std::min<std::size_t>(c.geometry.size()-2,40)));
    // Imported polylines may have more points than the inspector's 0–40 editing range.
    // Retargeting them must not become a new rejection or silently drop their point count.
    if(c.geometry.size()>42) {
        const auto curve=moved.geometry;const double length=polylineLength(curve);
        moved.geometry={curve.front()};
        for(std::size_t i=1;i+1<c.geometry.size();++i)
            moved.geometry.push_back(pointAlong(curve,length*static_cast<double>(i)/(c.geometry.size()-1)));
        moved.geometry.push_back(curve.back());
    }
    moved.laneBlend.clear();
    (void)connectorPaths(network,moved);
    c=std::move(moved);
}
// Snap both ends onto the lanes they NAME, wherever those lanes are. This is what an edit that
// names the lanes wants -- creating a Connector, or moving an end onto another lane -- because
// there the reference is the author's input and the geometry follows it.
//
// It is NOT what a Link edit wants: see reanchorConnector below, where the Connector's own
// geometry is the author's input and the reference follows it.
void anchorConnectorEnds(const Network& network,Connector& c) {
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
bool laneContains(const Network& network,const LaneReference& ref,Point p) {
    for(const auto& link:network.links)if(link.id==ref.linkId)
        for(const auto& lane:link.lanes)if(lane.id==ref.laneId) {
            const auto geometry=laneGeometry(link,ref.laneId,network.drivingSide);
            if(geometry.size()<2)return false;
            const auto on=pointAlong(geometry,stationOfClosestPoint(geometry,p));
            // Half the lane's width, and nothing else: the carriageway is where the lane is, and
            // a tolerance on top of it would be a width the lane does not have. A point a little
            // past the end of the lane but on its line still counts, and snaps to the end -- an
            // overshoot along the road is not the same thing as being off the road.
            return std::hypot(p.x-on.x,p.y-on.y)<=lane.width/2+1e-9;
        }
    return false;
}
// THE CONNECTOR KEEPS ITS OWN POSITION. Its geometry is what the author drew, not something
// recomputed from its Links on every edit: an end that is still on the lane it names is snapped
// back onto that lane's middle and its station moved to wherever the author has put it, and an
// end that has come off the lane is left exactly where it is and reported. A Connector with an
// end off its Link has nothing to connect, so the caller deletes it -- `reanchorConnectors` in
// `connector_commands.cpp`, in the same undoable transaction as the edit that moved it.
//
// This is what replaced dragging both ends after the Links on every edit. That made a Link edit
// reach into a Connector the author had placed by hand and move it; it also meant a Connector
// could not be moved off a Link at all, because the next edit put it back.
bool reanchorConnector(const Network& network,Connector& c) {
    if(c.geometry.size()<2)throw std::invalid_argument("INVALID_GEOMETRY");
    const auto hold=[&](LaneReference& ref,Point& end,int count,bool outgoing) {
        if(!std::isfinite(end.x) || !std::isfinite(end.y))return false;
        for(const auto& link:network.links)if(link.id==ref.linkId) {
            // A Link can be shortened past an attachment. Clamp rather than reject the Link edit.
            if(ref.station)ref.station=std::clamp(*ref.station,0.,polylineLength(link.geometry));
            // Nothing moved under this end: leave the reference alone, to the last bit. Without
            // this, every Link edit anywhere would re-derive every station through a polyline
            // round trip and walk them by an ulp at a time -- and a station is an author's number,
            // not something an unrelated edit may rewrite.
            const auto standing=laneAttachment(network,ref,outgoing);
            if(std::hypot(end.x-standing.x,end.y-standing.y)<=1e-12)return true;
            // A LANE of this Link, not necessarily the one named. A lane bundle edit slides every
            // lane sideways by a whole lane width, and the end the author placed has not moved:
            // it is now on its neighbour, and that is the lane it attaches to. Only leaving the
            // Link's carriageway altogether detaches it. The named lane is tried first, so an end
            // that is still on its own lane never changes lane over rounding.
            const auto lanes=link.lanes;
            std::size_t found=lanes.size();
            if(laneContains(network,ref,end))
                for(std::size_t i=0;i<lanes.size();++i)if(lanes[i].id==ref.laneId)found=i;
            for(std::size_t i=0;found==lanes.size() && i<lanes.size();++i)
                if(laneContains(network,{link.id,lanes[i].id,ref.station},end))found=i;
            if(found==lanes.size())return false;
            // A range owns `count` lanes from here on, so it may not start past what is left.
            const auto first=std::min(found,lanes.size()-static_cast<std::size_t>(std::min<int>(count,
                static_cast<int>(lanes.size()))));
            ref.laneId=lanes[first].id;
            const auto lane=laneGeometry(link,ref.laneId,network.drivingSide);
            const double along=stationOfClosestPoint(lane,end),length=polylineLength(lane);
            const double reference=polylineLength(link.geometry);
            // The end and the start of a Link keep meaning "the end" and "the start": that is what
            // `attachedAtLinkEnd` reads and what the M0 whole-lane runtime can traverse, and an
            // attachment that stayed put must not drift off it. Read on the LANE, with a micron of
            // slack, because measuring a polyline's own length back off it is not exact and a
            // micron is not a position an author can mean.
            const bool atEnd=outgoing?length-along<=1e-6:along<=1e-6;
            ref.station=atEnd?std::optional<double>{}
                             :std::optional<double>{std::clamp(matchedStation(lane,link.geometry,along),0.,reference)};
            end=laneAttachment(network,ref,outgoing);
            return true;
        }
        return false;   // The Link itself is gone.
    };
    const bool source=hold(c.from,c.geometry.front(),c.fromLaneCount,true);
    const bool target=hold(c.to,c.geometry.back(),c.toLaneCount,false);
    return source && target;
}
namespace {
// The travel direction of a lane at the station a Connector attaches to it.
Point endDirection(const Network& network,const LaneReference& ref,bool outgoing) {
    for(const auto& link:network.links)if(link.id==ref.linkId) {
        const auto lane=laneGeometry(link,ref.laneId,network.drivingSide);
        if(lane.size()<2)throw std::invalid_argument("INVALID_GEOMETRY");
        const double station=matchedStation(link.geometry,lane,attachmentStation(network,ref,outgoing));
        return directionAlong(lane,station,outgoing);
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
