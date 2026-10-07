#include "discharge_passage.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace trafficsim {
std::map<std::uint64_t,DischargeMotion> dischargeMotions(
    const SimState& state,const DischargePositions& previous,const DischargePending& pending) {
    const auto& s=*state.scenario;const auto& index=*state.index;
    constexpr auto unknown=std::numeric_limits<std::size_t>::max();
    const auto slot=[&](const std::string& id) {
        const auto it=std::find_if(s.routes.begin(),s.routes.end(),[&](const auto& r){return r.id==id;});
        return it==s.routes.end()?unknown:static_cast<std::size_t>(it-s.routes.begin());
    };
    const auto near=[](double a,double b){return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=1e-9;};
    std::map<std::uint64_t,DischargeMotion> result;
    for(const auto& [id,p]:previous)result[id].start=p;
    for(const auto& v:state.vehicles)result[v.id].end=DischargePosition{v.routeIndex,v.typeIndex,v.distance};
    for(const auto& e:state.events)if(const auto* a=std::get_if<ArrivedEvent>(&e)) {
        auto& m=result[a->vehicleId];const auto r=slot(a->routeId);
        if(m.end||r==unknown) {m.ambiguous=true;continue;}
        const auto& parts=index.parts[r];
        m.end=DischargePosition{r,unknown,parts.back().start+parts.back().length};
        if(!near(a->time,state.time))m.ambiguous=true;
    }
    for(const auto& e:state.events)if(const auto* d=std::get_if<DepartedEvent>(&e)) {
        auto& m=result[d->vehicleId];m.source=true;const auto r=slot(d->routeId);
        if(m.start||r==unknown) {m.ambiguous=true;continue;}
        auto type=m.end?m.end->type:unknown;
        if(const auto p=pending.find(d->vehicleId);p!=pending.end()) {
            const auto& v=p->second;
            // An exact fingerprint, never a route/input guess: a mismatch is contradictory evidence.
            if(v.routeIndex!=r||v.scheduledTime!=d->scheduledTime||v.desiredSpeed!=d->desiredSpeed||
               (type!=unknown&&type!=v.typeIndex))m.ambiguous=true;
            type=v.typeIndex;
        }
        m.start=DischargePosition{r,type,0};
        if(type==unknown||!near(d->time,state.time-s.timeStep))m.ambiguous=true;
        if(m.end&&m.end->type==unknown)m.end->type=type;
    }
    DischargePositions current;
    for(const auto& [id,m]:result)if(m.start)current[id]=*m.start;
    std::map<std::uint64_t,bool> endPhase;
    for(const auto& e:state.events)std::visit([&](const auto& event) {
        using T=std::decay_t<decltype(event)>;
        if constexpr(std::is_same_v<T,LaneChangeEvent>||std::is_same_v<T,RoutingEvent>) {
            auto& m=result[event.vehicleId];const auto from=slot(event.fromRouteId),to=slot(event.toRouteId);
            if(to==unknown) {m.ambiguous=true;return;}
            auto it=current.find(event.vehicleId);
            if(it==current.end()||from!=it->second.route) {
                m.ambiguous=true;m.remaps.push_back({{from,unknown,0},{to,unknown,0}});return;
            }
            auto before=it->second,after=before;after.route=to;
            if constexpr(std::is_same_v<T,LaneChangeEvent>) {
                std::optional<double> mapped;
                if(!near(event.time,state.time-s.timeStep)||endPhase[event.vehicleId]||before.type>=s.vehicleTypes.size())
                    m.ambiguous=true;
                else {
                    const double rear=before.distance-s.vehicleTypes[before.type].length;
                    const auto consider=[&](const auto& spans) {
                        for(const auto& c:spans)if(c.target==to&&rear>=c.fromStart-1e-9&&before.distance<=c.fromEnd+1e-9) {
                            const auto at=mappedOnto(c,before.distance);
                            if(mapped&&!near(*mapped,at))m.ambiguous=true;
                            else mapped=at;
                        }
                    };
                    consider(index.laneChangesOfRoute[from]);consider(index.discretionaryOfRoute[from]);
                }
                if(!mapped)m.ambiguous=true;
                after.distance=m.ambiguous?0:mapped.value_or(0); // ambiguous target: conservatively upstream
                m.start=after;
            } else {
                const auto d=std::find_if(s.routeDecisions.begin(),s.routeDecisions.end(),[&](const auto& x) {
                    return x.id==event.decisionId&&x.fromRouteId==event.fromRouteId;
                });
                const bool atEnd=near(event.time,state.time),atStart=near(event.time,state.time-s.timeStep);
                if(atEnd&&m.end)before.distance=after.distance=m.end->distance;
                if(d==s.routeDecisions.end()||(!atEnd&&!atStart)||(!atEnd&&endPhase[event.vehicleId])||
                   !near(before.distance,d==s.routeDecisions.end()?std::numeric_limits<double>::quiet_NaN():d->at)||
                   (d!=s.routeDecisions.end()&&std::none_of(d->choices.begin(),d->choices.end(),[&](const auto& c){return c.routeId==event.toRouteId;})))
                    m.ambiguous=true;
                if(atEnd)endPhase[event.vehicleId]=true;
                else m.start=after;
            }
            m.remaps.emplace_back(before,after);it->second=after;
        }
    },e);
    for(auto& [id,m]:result) {
        if(!m.start||!m.end) {m.ambiguous=true;continue;}
        if(m.end->type==unknown)m.end->type=m.start->type;
        const auto p=current.find(id);
        if(p==current.end()||p->second.route!=m.end->route||m.start->type!=m.end->type||
           m.end->type>=s.vehicleTypes.size()||!std::isfinite(m.start->distance)||
           !std::isfinite(m.end->distance)||m.end->distance<m.start->distance-1e-9)m.ambiguous=true;
    }
    return result;
}
}
