#include "test.hpp"
#include "../src/eval/discharge.hpp"
#include "../src/core/simulation.hpp"
using namespace trafficsim;
// M3.3.1b2b2 (D125): same-tick source-sink identity (BA05) and rank-scoped remap invalidation.
namespace {
Scenario signalled(double length,double head,double dt) {
    Scenario s;s.duration=30;s.timeStep=dt;
    s.segments={{"lane",length,{}}};s.routes={{"route",{"lane"}}};
    s.behaviours={{"driver",2,2,3,1.5,.2}};
    s.vehicleTypes={{"car",4,2,{10,10},2,3,6,"driver"}};
    s.signalPrograms={{"signal",0,{{dt,SignalColor::red},{20,SignalColor::green},{10-dt,SignalColor::red}}}};
    s.signalHeads={{"head","lane",head,"signal"}};return s;
}
// Two inputs on ONE route: the route cannot name the type; desired speed does, independently.
Scenario mixedSink(double dt) {
    auto s=signalled(.005,.004,dt);
    s.vehicleTypes.push_back({"bus",10,2.5,{5,5},1,2,5,"driver"});
    s.inputs={{"cars","route","car",3600,0,30},{"buses","route","bus",3600,0,30}};return s;
}
Scenario lateral() {
    auto s=signalled(20,19,.1);s.segments.push_back({"old",20,{}});
    s.routes.push_back({"stub",{"old"}});s.signalHeads.push_back({"old-head","old",19,"signal"});
    s.laneChanges={{"stub","route",0,20,0,20}};s.routeDeadEnds={{"stub",20}};return s;
}
void go(SimState& a,DischargeAccumulator& d) {
    a.tick=1;a.time=a.scenario->timeStep;a.events.clear();
    for(const auto& h:a.scenario->signalHeads)a.events.emplace_back(SignalEvent{a.time,h.id,SignalColor::green});
    d.observe(a);
}
std::uint32_t slot(const SimState& a,const std::string& id) {
    for(std::uint32_t i=0;i<a.scenario->routes.size();++i)if(a.scenario->routes[i].id==id)return i;
    throw std::runtime_error("Missing fixture route");
}
const DischargeCycle& row(const std::vector<DischargeCycle>& rows,const std::string& head) {
    for(const auto& r:rows)if(r.headId==head)return r;
    throw std::runtime_error("Missing fixture head");
}
}
TEST(discharge, upcoming_arrivals_replay_next_generation_without_mutation) {
    auto a=createSimulation(mixedSink(.1),42);bool generated=false;
    CHECK(upcomingArrivals(test::withVehicles(signalled(20,19,.1),{})).empty());
    while(a.tick+1<totalTicks(*a.scenario)) {
        const auto copy=a;const auto up=upcomingArrivals(a);
        CHECK(a.inputs==copy.inputs && a.randomState==copy.randomState && a.nextVehicleId==copy.nextVehicleId);
        const auto next=stepSimulation(SimState(a));
        CHECK(next.nextVehicleId==a.nextVehicleId+up.size());
        std::map<std::uint64_t,std::size_t> seen;
        for(const auto& input:next.inputs)for(const auto& v:input.queue)seen[v.id]=v.typeIndex;
        for(const auto& v:next.vehicles)seen[v.id]=v.typeIndex;
        for(const auto& v:up) {
            generated=true;CHECK(v.id>=a.nextVehicleId);
            if(seen.contains(v.id))CHECK(seen[v.id]==v.typeIndex);
        }
        a=next;
    }
    CHECK(generated);
}
TEST(discharge, same_tick_source_sink_types_come_from_exact_replay) {
    for(double dt:{.1,.2}) {
        auto a=createSimulation(mixedSink(dt),42);auto b=a;
        DischargeAccumulator d({0,30,0,3,5,2},{.5,1,10});d.observe(a);
        std::map<std::uint64_t,double> speedOf;std::set<std::uint64_t> sameTick;
        std::size_t inGreen=0;bool green=false;
        while(a.tick<totalTicks(*a.scenario)) {
            std::set<std::uint64_t> known;
            for(const auto& v:a.vehicles)known.insert(v.id);
            for(const auto& input:a.inputs)for(const auto& v:input.queue)known.insert(v.id);
            a=stepSimulation(std::move(a));b=stepSimulation(std::move(b));d.observe(a);
            CHECK(a.vehicles==b.vehicles && a.events==b.events && a.inputs==b.inputs);
            CHECK(a.randomState==b.randomState && a.nextVehicleId==b.nextVehicleId && a.completed==b.completed);
            std::set<std::uint64_t> departed;bool red=false,turnsGreen=false;
            for(const auto& e:a.events) {
                if(const auto* x=std::get_if<DepartedEvent>(&e)) {departed.insert(x->vehicleId);speedOf[x->vehicleId]=x->desiredSpeed;}
                if(const auto* x=std::get_if<SignalEvent>(&e))(x->color==SignalColor::green?turnsGreen:red)=true;
            }
            for(const auto& e:a.events)if(const auto* x=std::get_if<ArrivedEvent>(&e)) {
                if(departed.contains(x->vehicleId)&&!known.contains(x->vehicleId))sameTick.insert(x->vehicleId);
                if(green&&!red)++inGreen;
            }
            if(red)green=false;
            if(turnsGreen)green=true;
        }
        // The forcing worked: vehicles were generated, inserted and sunk within one tick, so
        // neither a survivor nor a previous queue entry can name their type.
        CHECK(sameTick.size()>=5);
        std::size_t crossings=0,cars=0,buses=0;
        for(const auto& r:d.report()) {
            if(r.complete)CHECK(r.unavailable.empty());
            for(const auto& c:r.crossings) {
                ++crossings;CHECK(speedOf.contains(c.vehicleId));
                const auto expected=speedOf[c.vehicleId]==10?"car":"bus";CHECK(c.vehicleTypeId==expected);
                (c.vehicleTypeId=="car"?cars:buses)+=sameTick.contains(c.vehicleId);
            }
        }
        CHECK(crossings==inGreen);CHECK(cars>0 && buses>0);
    }
}
TEST(discharge, contradictory_pending_fingerprint_stays_untracked) {
    for(double scheduled:{0.,.05}) {
        auto s=signalled(.005,.004,.1);s.inputs={{"input","route","car",0,0,30}};
        auto a=createSimulation(s,42);PendingVehicle p;p.id=1;p.inputIndex=0;
        p.routeIndex=slot(a,"route");p.typeIndex=0;p.desiredSpeed=10;
        a.inputs[0].queue.push_back(p);a.nextVehicleId=2;
        DischargeAccumulator d({0,30,0,3,5,2},{.5,1,10});d.observe(a);go(a,d);
        a.tick=2;a.time=.2;a.inputs[0].queue.clear();
        a.events={DepartedEvent{.1,1,"route",scheduled,10},ArrivedEvent{.2,1,"route",.1,0,0}};d.observe(a);
        const auto r=d.report()[0];
        if(scheduled==0) {CHECK(r.unavailable.empty());CHECK(r.crossings.size()==1);} // the control matches
        else {CHECK(r.crossings.empty());CHECK(r.unavailable=="untracked_source_passage");}
    }
}
TEST(discharge, unqueued_lane_change_keeps_both_cycles_available) {
    auto a=test::withVehicles(lateral(),{{1,"stub",5,10}});
    DischargeAccumulator d({0,30,0,3,5,2},{.5,1,10});d.observe(a);go(a,d);
    a.tick=2;a.time=.2;a.vehicles[0].routeIndex=slot(a,"route");a.vehicles[0].distance=5.5;
    a.events={LaneChangeEvent{.1,1,"stub","route"}};d.observe(a);
    const auto rows=d.report();
    for(const auto& r:rows) {CHECK(r.crossings.empty());CHECK(r.unavailable.empty());}
}
TEST(discharge, queued_lane_change_invalidates_only_while_ranks_are_open) {
    for(std::size_t last:{2,3}) {
        // Three vehicles queued at old-head; two cross, then the third changes lane away.
        auto a=test::withVehicles(lateral(),{{1,"stub",18.9,0},{2,"stub",14,0},{3,"stub",9,0}});
        DischargeAccumulator d({0,30,0,2,last,1},{.5,1,10});d.observe(a);go(a,d);
        a.tick=2;a.time=.2;a.vehicles[0].distance=19.5;a.vehicles[1].distance=19.2;a.events.clear();d.observe(a);
        CHECK(row(d.report(),"old-head").crossings.size()==2);
        a.tick=3;a.time=.3;a.vehicles[2].routeIndex=slot(a,"route");
        a.events={LaneChangeEvent{.2,3,"stub","route"}};d.observe(a);
        const auto old=row(d.report(),"old-head");CHECK(old.crossings.size()==2);
        CHECK(old.unavailable==(last==2?"":"route_or_lane_change"));
        CHECK(row(d.report(),"head").unavailable.empty()); // it was never queued there
    }
}
