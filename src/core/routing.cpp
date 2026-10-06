#include "routing.hpp"
#include "detail.hpp"
#include "conflicts.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace trafficsim {
namespace {
// The full physical prefix must agree, including segment lengths, through the station.
std::vector<std::string> prefixAt(const Scenario& s,const Route& r,double at) {
    std::vector<std::string> prefix;double length=0;
    for(const auto& id:r.segmentIds) {
        const auto segment=std::find_if(s.segments.begin(),s.segments.end(),[&](const auto& x){return x.id==id;});
        if(segment==s.segments.end())return {};
        prefix.push_back(id);length+=segment->length;
        if(length>at+1e-9)return prefix;
    }
    return {}; // a decision at the route's end cannot select a continuation
}
}
std::vector<ValidationIssue> routingIssues(const Scenario& s) {
    std::vector<ValidationIssue> issues;std::set<std::pair<std::string,std::string>> seen;
    for(std::size_t k=0;k<s.routeDecisions.size();++k) {
        const auto& d=s.routeDecisions[k];const auto path="routeDecisions["+std::to_string(k)+"]";
        const auto add=[&](const char* code){issues.push_back({code,path});};
        const auto route=std::find_if(s.routes.begin(),s.routes.end(),[&](const auto& x){return x.id==d.fromRouteId;});
        if(d.id.empty() || !seen.insert({d.id,d.fromRouteId}).second)add("DUPLICATE_ID");
        if(route==s.routes.end()){add("UNKNOWN_ROUTE");continue;}
        if(!std::isfinite(d.at) || d.at<0){add("INVALID_POSITION");continue;}
        const auto prefix=prefixAt(s,*route,d.at);
        if(prefix.empty())add("INVALID_POSITION");
        const bool known=std::all_of(route->segmentIds.begin(),route->segmentIds.end(),[&](const auto& id){
            return std::any_of(s.segments.begin(),s.segments.end(),[&](const auto& x){return x.id==id;});});
        if(known && !s.conflictZones.empty())for(const auto& zone:zoneIncidence(s,routeParts(s,*route))) {
            const double entry=zone.role==ZoneRole::minor?zone.waitAt:zone.entryAt;
            if(d.at>=entry-1e-9 && d.at<=zone.exitAt+1e-9)add("ROUTING_POSITION_IN_CONFLICT");
        }
        double total=0;std::set<std::string> targets;
        for(const auto& c:d.choices) {
            const auto target=std::find_if(s.routes.begin(),s.routes.end(),[&](const auto& x){return x.id==c.routeId;});
            if(target==s.routes.end())add("UNKNOWN_ROUTE");
            else if(prefixAt(s,*target,d.at)!=prefix)add("ROUTING_PREFIX_MISMATCH");
            if(!targets.insert(c.routeId).second)add("DUPLICATE_ID");
            if(!std::isfinite(c.weight) || c.weight<0)add("INVALID_SHARE");
            total+=c.weight;
            if(c.intervalWeights.size()!=d.intervals.size())add("ROUTING_DECISION_INTERVALS");
            for(double w:c.intervalWeights)if(!std::isfinite(w)||w<0)add("INVALID_SHARE");
        }
        if(!std::isfinite(total)||!(total>0))add("INVALID_SHARE");
        for(std::size_t i=0;i<d.intervals.size();++i) {
            const auto& p=d.intervals[i];
            if(!std::isfinite(p.startTime)||!std::isfinite(p.endTime)||p.startTime<0||p.endTime<=p.startTime ||
               (i && p.startTime<d.intervals[i-1].endTime))add("INVALID_INTERVAL");
        }
    }
    return issues;
}
const RouteDecision* nextRouteDecision(const Scenario& s,const Vehicle& v) {
    const RouteDecision* found=nullptr;
    const auto& id=s.routes[v.routeIndex].id;
    for(const auto& d:s.routeDecisions) {
        if(d.fromRouteId!=id || std::find(v.passedDecisions.begin(),v.passedDecisions.end(),d.id)!=v.passedDecisions.end())continue;
        if(d.at<v.distance-1e-9)continue; // an arrival downstream never crosses an upstream line
        if(!found || d.at<found->at || (d.at==found->at && d.id<found->id))found=&d;
    }
    return found;
}
void applyRouteDecision(const Scenario& s,Vehicle& v,const RouteDecision& d,double time,
                        std::uint32_t& randomState,std::vector<SimEvent>& events) {
    std::size_t interval=d.intervals.size();
    for(std::size_t i=0;i<d.intervals.size();++i)
        if(time>=d.intervals[i].startTime && time<d.intervals[i].endTime){interval=i;break;}
    double sum=0;
    if(interval<d.intervals.size())for(const auto& c:d.choices)sum+=c.intervalWeights[interval];
    const bool counted=sum>0 && interval<d.intervals.size();
    if(!counted){sum=0;for(const auto& c:d.choices)sum+=c.weight;}
    const double draw=detail::random(randomState)*sum;
    double cumulative=0;const RoutingChoice* selected=nullptr;
    for(const auto& c:d.choices) {
        const double weight=counted?c.intervalWeights[interval]:c.weight;
        if(weight<=0)continue;
        selected=&c;cumulative+=weight;if(draw<cumulative)break;
    }
    if(!selected)throw std::logic_error("Routing decision has no positive choice");
    const auto from=s.routes[v.routeIndex].id;
    const auto& route=detail::byId(s.routes,selected->routeId);
    v.routeIndex=static_cast<std::uint32_t>(&route-s.routes.data());
    v.passedDecisions.push_back(d.id);
    events.emplace_back(RoutingEvent{time,v.id,d.id,from,route.id});
}
}
