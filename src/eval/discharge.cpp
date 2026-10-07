#include "discharge.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace trafficsim {
void validateDischargeSpec(const DischargeSpec& s) {
    if(!std::isfinite(s.windowStart)||!std::isfinite(s.windowEnd)||!std::isfinite(s.warmup)||
       s.windowStart<0||s.windowEnd<=s.windowStart||s.warmup<0||s.warmup>=s.windowEnd||
       s.steadyFirst<2||s.steadyLast<s.steadyFirst||s.startupLast==0||s.startupLast>=s.steadyFirst)
        throw std::invalid_argument("Invalid discharge window or rank ranges");
}
DischargeEstimate estimateDischarge(const DischargeCycle& c,const DischargeSpec& s) {
    validateDischargeSpec(s);
    const auto unavailable=[](const std::string& why){return DischargeEstimate{why,0,{},{},{}};};
    if(!c.unavailable.empty())return unavailable(c.unavailable);
    if(!c.complete)return unavailable("partial_cycle");
    if(!std::isfinite(c.go)||!std::isfinite(c.end)||c.end<=c.go||
       !std::isfinite(c.timeStep)||c.timeStep<=0)return unavailable("invalid_cycle");
    if(c.go<std::max(s.windowStart,s.warmup)||c.end>s.windowEnd)return unavailable("outside_window");
    std::set<std::uint64_t> seen;
    double previous=c.go;
    for(const auto& v:c.crossings) {
        if(!std::isfinite(v.time)||v.time<c.go||v.time>=c.end||v.time<previous||
           v.vehicleTypeId.empty()||!seen.insert(v.vehicleId).second)return unavailable("invalid_crossings");
        previous=v.time;
    }
    if(c.crossings.size()<s.steadyLast)return unavailable("insufficient_crossings");
    for(std::size_t i=0;i<s.steadyLast;++i)
        if(!c.crossings[i].queuedAtGo)return unavailable("queue_not_sustained");
    double total=0;
    for(auto rank=s.steadyFirst;rank<=s.steadyLast;++rank) {
        const double gap=c.crossings[rank-1].time-c.crossings[rank-2].time;
        if(gap<=0)return unavailable("nonpositive_headway");
        total+=gap;
    }
    const auto n=s.steadyLast-s.steadyFirst+1;
    const double h=total/static_cast<double>(n);
    const double rate=3600/h, startup=c.crossings[s.startupLast-1].time-c.go-s.startupLast*h;
    if(!std::isfinite(h)||!std::isfinite(rate)||!std::isfinite(startup))return unavailable("nonfinite_estimate");
    return {{},n,h,rate,startup};
}

DischargeAccumulator::DischargeAccumulator(DischargeSpec s,QueueDefinition q):spec_(s),queue_(q) {
    validateDischargeSpec(s);
    if(!std::isfinite(q.beginSpeed)||!std::isfinite(q.maxGap)||q.beginSpeed<0||q.maxGap<=0)
        throw std::invalid_argument("Invalid discharge queue definition");
}
void DischargeAccumulator::observe(const SimState& state) {
    if(!state.scenario||!state.index)throw std::invalid_argument("Discharge needs a simulation snapshot");
    if(scenario_&&scenario_!=state.scenario)throw std::invalid_argument("Discharge Scenario changed");
    if(tick_&&*tick_==state.tick)return;
    if(tick_&&state.tick!=*tick_+1)throw std::invalid_argument("Discharge needs consecutive snapshots");
    if(!scenario_) {
        scenario_=state.scenario;
        for(const auto& head:scenario_->signalHeads) {
            Head h; h.atRoute.assign(scenario_->routes.size(),std::numeric_limits<double>::quiet_NaN());
            for(std::size_t r=0;r<scenario_->routes.size();++r)
                for(const auto& p:state.index->parts[r])if(p.segmentId==head.segmentId) {
                    h.atRoute[r]=p.start+head.position; break;
                }
            heads_.push_back(std::move(h));
        }
    }
    for(std::size_t i=0;i<heads_.size();++i) {
        auto& h=heads_[i]; const auto& head=scenario_->signalHeads[i];
        std::map<std::uint64_t,double> nextUpstream;
        for(const auto& v:state.vehicles) {
            const auto at=h.atRoute.at(v.routeIndex);
            if(std::isfinite(at)&&v.distance<at)nextUpstream.emplace(v.id,at-v.distance);
        }
        const auto record=[&](std::uint64_t id,std::size_t type) {
            if(h.cycle)h.cycle->crossings.push_back({id,scenario_->vehicleTypes.at(type).id,
                                                    state.time,h.queue.contains(id)});
        };
        // Remaps cannot be mistaken for motion through a head. Conservatively invalidate
        // the affected cycle when a vehicle tracked upstream changes lane or route.
        for(const auto& e:state.events) {
            std::visit([&](const auto& event) {
                using T=std::decay_t<decltype(event)>;
                if constexpr(std::is_same_v<T,LaneChangeEvent>||std::is_same_v<T,RoutingEvent>)
                    if(h.cycle&&h.upstream.contains(event.vehicleId))h.cycle->unavailable="route_or_lane_change";
            },e);
        }
        // A source can be downstream of a head, or pass it within its insertion tick.
        // Without the source station/type snapshot we cannot distinguish those cases.
        for(const auto& e:state.events)if(const auto* departed=std::get_if<DepartedEvent>(&e)) {
            const auto route=std::find_if(scenario_->routes.begin(),scenario_->routes.end(),
                [&](const auto& r){return r.id==departed->routeId;});
            if(h.cycle&&route!=scenario_->routes.end()&&
               std::isfinite(h.atRoute[route-scenario_->routes.begin()])&&
               !nextUpstream.contains(departed->vehicleId))
                h.cycle->unavailable="untracked_source_passage";
        }
        for(const auto& v:state.vehicles)
            if(h.upstream.contains(v.id)&&!nextUpstream.contains(v.id)&&
               std::isfinite(h.atRoute.at(v.routeIndex)))record(v.id,v.typeIndex);
        // At a route sink no survivor snapshot exists. The Arrived event and previously
        // tracked head establish passage, rather than dropping the last vehicle.
        for(const auto& e:state.events)if(const auto* a=std::get_if<ArrivedEvent>(&e))
            if(h.upstream.contains(a->vehicleId)) {
                const auto type=types_.find(a->vehicleId);
                if(type!=types_.end())record(a->vehicleId,type->second);
            }
        for(const auto& e:state.events)if(const auto* signal=std::get_if<SignalEvent>(&e)) {
            if(signal->signalId!=head.id)continue;
            if(signal->color!=SignalColor::green&&h.cycle) {
                h.cycle->end=state.time; h.cycle->complete=h.cycle->go>=0;
                // Crossing at the right boundary is outside this half-open green.
                std::erase_if(h.cycle->crossings,[&](const auto& c){return c.time>=state.time;});
                closed_.push_back(std::move(*h.cycle)); h.cycle.reset();
            } else if(signal->color==SignalColor::green&&!h.cycle) {
                h.cycle=DischargeCycle{head.id,head.segmentId,state.time,state.time,
                                      scenario_->timeStep,false,{}, {}};
                if(!tick_)h.cycle->unavailable="initial_partial_cycle";
                h.queue.clear();
                struct Waiting {std::uint64_t id; double gap,length,speed;};
                std::vector<Waiting> waiting;
                for(const auto& v:state.vehicles)if(nextUpstream.contains(v.id))
                    waiting.push_back({v.id,nextUpstream.at(v.id),scenario_->vehicleTypes.at(v.typeIndex).length,v.speed});
                std::sort(waiting.begin(),waiting.end(),[](auto a,auto b){return a.gap!=b.gap?a.gap<b.gap:a.id<b.id;});
                double rear=0;
                for(const auto& v:waiting) {
                    if(v.speed>=queue_.beginSpeed||v.gap-rear>queue_.maxGap)break;
                    h.queue.insert(v.id); rear=v.gap+v.length;
                }
            }
        }
        h.upstream=std::move(nextUpstream);
        if(h.cycle)h.cycle->end=state.time;
    }
    types_.clear(); for(const auto& v:state.vehicles)types_.emplace(v.id,v.typeIndex);
    tick_=state.tick;
}
std::vector<DischargeCycle> DischargeAccumulator::report() const {
    auto result=closed_;
    for(const auto& h:heads_)if(h.cycle)result.push_back(*h.cycle);
    std::stable_sort(result.begin(),result.end(),[](const auto& a,const auto& b){
        return a.headId!=b.headId?a.headId<b.headId:a.go<b.go;
    });
    return result;
}
}
