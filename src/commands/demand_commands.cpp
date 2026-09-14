#include "demand_commands.hpp"
#include <algorithm>

namespace trafficsim {
AuthoringDefinition& demand(ProjectDocument& d) {
    if (!d.definition) d.definition.emplace();
    return *d.definition;
}
namespace {
template<class T> void put(std::vector<T>& values, T value) {
    for (auto& existing : values) if (existing.id==value.id) { existing=std::move(value); return; }
    values.push_back(std::move(value));
}
template<class T> void remove(std::vector<T>& values, const std::string& id) {
    const auto count=std::erase_if(values,[&](const auto& v){return v.id==id;});
    if (!count) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
}
std::string putRoute(ProjectDocument& d, Route value) {
    if (value.id.empty()) value.id=allocateId(d,"route");
    const auto id=value.id; put(demand(d).routes,std::move(value)); return id;
}
std::string putInput(ProjectDocument& d, VehicleInput value) {
    if (value.id.empty()) value.id=allocateId(d,"input");
    const auto id=value.id; put(demand(d).inputs,std::move(value)); return id;
}
std::string putProgram(ProjectDocument& d, SignalProgram value) {
    if (value.id.empty()) value.id=allocateId(d,"program");
    const auto id=value.id; put(demand(d).signalPrograms,std::move(value)); return id;
}
void deleteRoute(ProjectDocument& d, const std::string& id) {
    auto& values=demand(d); remove(values.routes,id);
    std::erase_if(values.inputs,[&](const auto& i){return i.routeId==id;});
}
void deleteInput(ProjectDocument& d, const std::string& id) { remove(demand(d).inputs,id); }
void deleteProgram(ProjectDocument& d, const std::string& id) {
    for (const auto& h:d.network.signalHeads) if (h.programId==id)
        throw std::invalid_argument("EDIT_REFERENCED_PROGRAM");
    remove(demand(d).signalPrograms,id);
}
void changeRunSettings(ProjectDocument& d, double duration, double timeStep) {
    auto& def=demand(d); def.duration=duration; def.timeStep=timeStep;
}
std::string putSignalHead(ProjectDocument& d, NetworkSignalHead value) {
    if (value.id.empty()) value.id=allocateId(d,"head");
    const auto id=value.id; put(d.network.signalHeads,std::move(value)); return id;
}
void deleteSignalHead(ProjectDocument& d, const std::string& id) { remove(d.network.signalHeads,id); }
}
