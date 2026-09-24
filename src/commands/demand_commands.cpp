#include "demand_commands.hpp"
#include "detail.hpp"
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
    const auto id=value.id;
    // A route may name only objects that exist. Objects that exist but do not join up are a
    // different matter -- tolerated while authoring, reported by routeRuntimeIssues, refused by
    // Run -- because an author must be able to move a Connector without the edit being rejected.
    for (const auto& named : value.segmentIds) {
        const bool known =
            std::any_of(d.network.links.begin(), d.network.links.end(),
                        [&](const auto& l) {
                            if (l.id == named) return true;
                            return std::any_of(l.lanes.begin(), l.lanes.end(),
                                               [&](const auto& lane) { return lane.id == named; });
                        }) ||
            std::any_of(d.network.connectors.begin(), d.network.connectors.end(),
                        [&](const auto& c) {
                            if (c.id == named) return true;
                            for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i)
                                if (connectorPathId(c, i) == named) return true;
                            return false;
                        });
        if (!known) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    }
    auto& routes=demand(d).routes;
    put(routes,std::move(value));
    // A route names Links and Connectors (M1.26). A caller that named a lane or a Connector path
    // meant the object it belongs to, so it is stored that way here rather than refused: this is
    // the same mapping a schema-7 file gets on load, and it keeps one meaning in the document.
    migrateRoutesToObjects(d.network,demand(d));
    return id;
}
std::string putInput(ProjectDocument& d, VehicleInput value) {
    if (value.id.empty()) value.id=allocateId(d,"input");
    deriveInputTotals(value); // M2.2: intervals, when given, are the source of the scalars.
    const auto id=value.id; put(demand(d).inputs,std::move(value)); return id;
}
std::string putProgram(ProjectDocument& d, SignalProgram value) {
    if (value.id.empty()) value.id=allocateId(d,"program");
    const auto id=value.id; put(demand(d).signalPrograms,std::move(value)); return id;
}
void deleteRoute(ProjectDocument& d, const std::string& id) {
    auto& values=demand(d); remove(values.routes,id);
    std::erase_if(values.inputs,[&](const auto& i){return i.routeId==id;});
    detail::pruneRoutingDecisions(values);
}
std::string putRoutingDecision(ProjectDocument& d, RoutingDecision value) {
    if (value.id.empty()) value.id=allocateId(d,"decision");
    const auto id=value.id; put(demand(d).routingDecisions,std::move(value)); return id;
}
void deleteRoutingDecision(ProjectDocument& d, const std::string& id) {
    auto& values=demand(d); remove(values.routingDecisions,id);
    std::erase_if(values.inputs,[&](const auto& i){return i.routingDecisionId==id;});
}
void detail::pruneRoutingDecisions(AuthoringDefinition& values) {
    // A decision entry for a route that is gone goes with it, as the route's own inputs do; a
    // decision left with no routes cannot split anything, so it and its inputs go too.
    for (auto& decision : values.routingDecisions)
        std::erase_if(decision.routes,[&](const auto& entry){
            return std::none_of(values.routes.begin(),values.routes.end(),[&](const auto& r){return r.id==entry.routeId;});});
    std::vector<std::string> emptied;
    for (const auto& decision : values.routingDecisions) if (decision.routes.empty()) emptied.push_back(decision.id);
    std::erase_if(values.routingDecisions,[](const auto& x){return x.routes.empty();});
    std::erase_if(values.inputs,[&](const auto& i){
        return std::find(emptied.begin(),emptied.end(),i.routingDecisionId)!=emptied.end();});
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
