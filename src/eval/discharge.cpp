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
    if(s.vehicleTypeIds.contains(""))throw std::invalid_argument("Empty discharge vehicle type ID");
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
    std::vector<std::size_t> ranks;
    for(auto rank=s.steadyFirst;rank<=s.steadyLast;++rank) {
        const double gap=c.crossings[rank-1].time-c.crossings[rank-2].time;
        if(gap<=0)return unavailable("nonpositive_headway");
        if(s.vehicleTypeIds.empty()||s.vehicleTypeIds.contains(c.crossings[rank-1].vehicleTypeId)) {
            total+=gap;ranks.push_back(rank);
        }
    }
    const auto n=ranks.size();
    if(n==0)return unavailable("no_matching_headways");
    const double h=total/static_cast<double>(n);
    const double rate=3600/h, startup=c.crossings[s.startupLast-1].time-c.go-s.startupLast*h;
    if(!std::isfinite(h)||!std::isfinite(rate)||!std::isfinite(startup))return unavailable("nonfinite_estimate");
    DischargeEstimate result{{},n,h,rate,startup,std::move(ranks),{}};
    if(!s.vehicleTypeIds.empty())
        for(std::size_t i=0;i<s.startupLast;++i)
            if(!s.vehicleTypeIds.contains(c.crossings[i].vehicleTypeId)) {
                result.startupLostTime.reset();result.startupUnavailableReason="mixed_type_startup_prefix";break;
            }
    return result;
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
        for(const auto& id:spec_.vehicleTypeIds)
            if(std::none_of(state.scenario->vehicleTypes.begin(),state.scenario->vehicleTypes.end(),
                            [&](const auto& v){return v.id==id;}))
                throw std::invalid_argument("Unknown discharge vehicle type: "+id);
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
    const auto motions=tick_?dischargeMotions(state,previous_,pending_):std::map<std::uint64_t,DischargeMotion>{};
    for(std::size_t i=0;i<heads_.size();++i) {
        auto& h=heads_[i]; const auto& head=scenario_->signalHeads[i];
        std::map<std::uint64_t,double> nextUpstream;
        for(const auto& v:state.vehicles) {
            const auto at=h.atRoute.at(v.routeIndex);
            if(std::isfinite(at)&&v.distance<at)nextUpstream.emplace(v.id,at-v.distance);
        }
        const auto upstream=[&](const DischargePosition& p) {
            return p.route<h.atRoute.size()&&std::isfinite(h.atRoute[p.route])&&p.distance<h.atRoute[p.route];
        };
        // D125: only ranks 1..steadyLast feed an estimate, and estimateDischarge already rejects a
        // vehicle there that was not queued at Go. Once those ranks are recorded nothing can change
        // them; before that, only a QUEUED vehicle leaving can silently shift them.
        const bool ranksOpen=h.cycle&&h.cycle->crossings.size()<spec_.steadyLast;
        for(const auto& [id,m]:motions) {
            bool affected=(m.start&&upstream(*m.start));
            for(const auto& [from,to]:m.remaps) {
                const bool changesMembership=upstream(from)||upstream(to);
                // Identical physical prefixes and equal stations keep the same lane membership.
                bool samePrefix=from.distance==to.distance;
                if(from.route>=scenario_->routes.size()||to.route>=scenario_->routes.size())samePrefix=false;
                else {
                    const auto& a=scenario_->routes[from.route].segmentIds;
                    const auto& b=scenario_->routes[to.route].segmentIds;bool found=false;
                    for(std::size_t j=0;samePrefix&&j<a.size();++j) {
                        if(j>=b.size()||a[j]!=b[j]) {samePrefix=false;break;}
                        if(a[j]==head.segmentId) {found=true;break;}
                    }
                    samePrefix&=found;
                }
                affected|=changesMembership;
                if(ranksOpen&&h.queue.contains(id)&&changesMembership&&!samePrefix)h.cycle->unavailable="route_or_lane_change";
            }
            if(m.ambiguous) {
                // Missing start evidence can hide a crossing on the terminal route.
                affected|=m.end&&(upstream(*m.end)||
                    ((!m.start||m.start->route!=m.end->route)&&m.end->route<h.atRoute.size()&&std::isfinite(h.atRoute[m.end->route])));
                if(ranksOpen&&affected)h.cycle->unavailable=m.source?"untracked_source_passage":"route_or_lane_change";
                continue;
            }
            if(!h.cycle||!m.start||!m.end||!upstream(*m.start)||m.end->distance<h.atRoute[m.start->route])continue;
            if(std::any_of(h.cycle->crossings.begin(),h.cycle->crossings.end(),[&](const auto& c){return c.vehicleId==id;})) {
                h.cycle->unavailable="repeated_head_passage";continue;
            }
            h.cycle->crossings.push_back({id,scenario_->vehicleTypes.at(m.start->type).id,state.time,h.queue.contains(id)});
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
        if(h.cycle)h.cycle->end=state.time;
    }
    previous_.clear();for(const auto& v:state.vehicles)previous_.emplace(v.id,DischargePosition{v.routeIndex,v.typeIndex,v.distance});
    pending_.clear();for(const auto& input:state.inputs)for(const auto& v:input.queue)pending_.emplace(v.id,v);
    for(const auto& v:upcomingArrivals(state))pending_.emplace(v.id,v); // same-tick source sinks (D125)
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
