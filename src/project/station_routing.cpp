#include "station_routing.hpp"
#include "demand_time_types.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace trafficsim {
namespace {
struct Gate { double at{},begin{},end{};std::vector<std::string> prefix; };
std::optional<Gate> gateOn(const Network& n,const RuntimeSections& table,const Route& route,
                           const RoutingDecision& decision) {
    const auto link=std::find_if(n.links.begin(),n.links.end(),[&](const auto& x){return x.id==decision.linkId;});
    if(link==n.links.end() || !decision.position)return {};
    Gate gate;
    for(const auto& id:route.segmentIds) {
        gate.prefix.push_back(id);
        for(const auto& section:table.sections)if(section.id==id) {
            if(section.linkId==link->id) {
                const auto lane=laneGeometry(*link,section.laneId,n.drivingSide);
                const double station=matchedStation(link->geometry,lane,*decision.position);
                if(station>=section.start-1e-9 && station<section.end-1e-9) {
                    gate.begin=gate.at;gate.end=gate.at+section.end-section.start;
                    gate.at+=station-section.start;return gate;
                }
            }
            gate.at+=section.end-section.start;
        }
        for(const auto& path:table.paths)if(path.id==id)gate.at+=connectorPathLength(path);
    }
    return {};
}
std::string familyOf(const AuthoringDefinition& d,const RoutingDecision& decision,std::size_t index) {
    if(decision.position)return decision.id+">route-"+std::to_string(index);
    const auto& entry=decision.routes[index];std::string end=entry.destinationLinkId;
    if(end.empty())for(const auto& r:d.routes)if(r.id==entry.routeId && !r.segmentIds.empty())end=r.segmentIds.back();
    return decision.id+">"+end;
}
bool member(const StationRoute& route,const std::string& family) {
    return std::any_of(route.families.begin(),route.families.end(),[&](const auto& x){return x.name==family;});
}
}
void appendStationRouting(const Network& n,const AuthoringDefinition& d,const std::string& type,
                          const std::vector<StationRoute>& routes,ScenarioDefinition& resolved) {
    const auto table=runtimeSections(n);
    std::map<std::pair<std::string,std::string>,Gate> gates;
    for(const auto& decision:d.routingDecisions)if(decision.position) {
        for(const auto& source:routes) {
            const bool tagged=std::any_of(source.families.begin(),source.families.end(),[&](const auto& tag){
                return tag.name.starts_with(decision.id+">");});
            if(!tagged)continue;
            const auto gate=gateOn(n,table,source.route,decision);
            if(!gate)throw ValidationError({{"ROUTING_POSITION_AFTER_DIVERGENCE",decision.id+".position"}});
            gates[{source.route.id,decision.id}]=*gate;
            RouteDecision runtime{decision.id,source.route.id,gate->at,{},{}};
            for(const auto& p:decision.intervals)runtime.intervals.push_back({p.startTime,p.endTime,0});
            for(std::size_t k=0;k<decision.routes.size();++k) {
                const auto family=familyOf(d,decision,k);
                // One representative per destination on this physical prefix; later decisions
                // select their own suffixes. Path order is the deterministic walk's order.
                for(const auto& target:routes)if(member(target,family)) {
                    bool compatible=true;
                    // Legacy downstream choices remain booked at scheduled demand time.
                    // Recognition may select this destination, not rebook those choices.
                    for(const auto& booked:d.routingDecisions)if(!booked.position && booked.id!=decision.id) {
                        bool applies=false,same=false;
                        for(const auto& tag:source.families)if(tag.name.starts_with(booked.id+">")) {
                            applies=true;same=same || member(target,tag.name);
                        }
                        if(applies && !same)compatible=false;
                    }
                    if(!compatible)continue;
                    const auto there=gateOn(n,table,target.route,decision);
                    if(!there || there->prefix!=gate->prefix || std::abs(there->at-gate->at)>1e-7)continue;
                    RoutingChoice choice{target.route.id,typeDecisionFlowAt(decision,k,-1,type),{}};
                    for(const auto& p:decision.intervals)
                        choice.intervalWeights.push_back(typeDecisionFlowAt(decision,k,(p.startTime+p.endTime)/2,type));
                    const auto same=std::find_if(runtime.choices.begin(),runtime.choices.end(),[&](const auto& c){return c.routeId==choice.routeId;});
                    if(same==runtime.choices.end())runtime.choices.push_back(std::move(choice));
                    else {
                        same->weight+=choice.weight;
                        for(std::size_t i=0;i<choice.intervalWeights.size();++i)same->intervalWeights[i]+=choice.intervalWeights[i];
                    }
                    break;
                }
            }
            if(runtime.choices.empty())throw ValidationError({{"ROUTING_DECISION_UNREACHABLE",decision.id}});
            resolved.routeDecisions.push_back(std::move(runtime));
        }
    }
    // Restrict only spans on the decision's physical section, preserving changes needed
    // between an earlier decision and a later one. Both lane stations determine the trim.
    std::erase_if(resolved.laneChanges,[&](auto& span) {
        double fraction=0;
        for(const auto& decision:d.routingDecisions)if(decision.position) {
            const auto a=gates.find({span.fromRouteId,decision.id}),b=gates.find({span.toRouteId,decision.id});
            if(a==gates.end() || b==gates.end())continue;
            if(span.fromEnd<a->second.begin || span.fromStart>a->second.end)continue;
            if(span.fromEnd<=a->second.at || span.toEnd<=b->second.at)return true;
            if(span.fromStart<a->second.at)fraction=std::max(fraction,(a->second.at-span.fromStart)/(span.fromEnd-span.fromStart));
            if(span.toStart<b->second.at)fraction=std::max(fraction,(b->second.at-span.toStart)/(span.toEnd-span.toStart));
        }
        span.fromStart+=(span.fromEnd-span.fromStart)*fraction;
        span.toStart+=(span.toEnd-span.toStart)*fraction;return false;
    });
}
}
