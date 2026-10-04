#include "test.hpp"
#include "../src/core/following.hpp"
#include <algorithm>
using namespace trafficsim;
namespace {
SimState source(double room) {
    auto s=test::straight();s.timeStep=.5;s.inputs.front().vehiclesPerHour=0;
    s.vehicleTypes.erase(std::remove_if(s.vehicleTypes.begin(),s.vehicleTypes.end(),[](const auto& t){return t.id!="car";}),s.vehicleTypes.end());
    s.vehicleTypes.front().maxAcceleration=2;
    auto state=createSimulation(s,42);const auto& type=state.scenario->vehicleTypes.front();
    const auto b=std::find_if(state.scenario->behaviours.begin(),state.scenario->behaviours.end(),[&](const auto& b){return b.id==type.behaviourId;});
    Vehicle leader;leader.id=1;leader.desiredSpeed=15;leader.driverFactor=.5;leader.speed=12;
    leader.distance=type.length+b->standstillDistance+room;state.vehicles.push_back(leader);
    PendingVehicle p;p.id=2;p.inputIndex=0;p.desiredSpeed=15;p.driverFactor=.5;
    state.inputs.front().queue.push_back(p);state.nextVehicleId=3;return state;
}
bool clamped(const SimState& s){return std::any_of(s.events.begin(),s.events.end(),[](const auto& e){return std::holds_alternative<SafetyClampEvent>(e);});}
}
TEST(core, source_waits_when_positive_clearance_cannot_fit_its_first_step) {
    const auto before=source(.001);const auto snapshot=checkpointJson(before);
    const auto next=stepSimulation(before);
    CHECK(next.vehicles.size()==1);CHECK(next.inputs.front().queue==before.inputs.front().queue);
    CHECK(!clamped(next));CHECK(next.randomState==before.randomState);
    CHECK(checkpointJson(before)==snapshot);CHECK(checkpointJson(stepSimulation(before))==checkpointJson(next));
    CHECK(next.nextVehicleId-1==next.completed+next.vehicles.size()+next.inputs.front().queue.size());
    const auto released=stepSimulation(next);
    CHECK(released.inputs.front().queue.empty());CHECK(released.vehicles.size()==2);CHECK(!clamped(released));
    const auto event=std::find_if(released.events.begin(),released.events.end(),[](const auto& e){const auto* d=std::get_if<DepartedEvent>(&e);return d && d->vehicleId==2;});
    CHECK(event!=released.events.end());const auto& d=std::get<DepartedEvent>(*event);
    CHECK(d.time==before.scenario->timeStep && d.scheduledTime==0);
    CHECK(released.vehicles.back().distance>0 && released.vehicles.back().speed>0);
}
TEST(core, source_first_step_boundary_admits_without_extra_clearance) {
    // Exactly representable arithmetic: 0.5*a*dt² = .25 m; no new epsilon or buffer.
    const auto before=source(.25);const auto next=stepSimulation(before);
    CHECK(next.inputs.front().queue.empty());CHECK(next.vehicles.size()==2);CHECK(!clamped(next));
    test::near(next.vehicles.back().distance,.25,0);test::near(next.vehicles.back().speed,1,0);
    const auto below=stepSimulation(source(.25-1./1024));CHECK(below.vehicles.size()==1);CHECK(!clamped(below));
}
TEST(core, source_at_standstill_boundary_can_enter_at_rest_and_later_release) {
    const auto next=stepSimulation(source(0));CHECK(next.inputs.front().queue.empty());CHECK(next.vehicles.size()==2);
    CHECK(next.vehicles.back().distance==0 && next.vehicles.back().speed==0);CHECK(!clamped(next));
    const auto below=stepSimulation(source(-.001));CHECK(below.vehicles.size()==1);
}
