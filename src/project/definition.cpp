#include "document.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <vector>

namespace trafficsim {
AuthoringDefinition parseAuthoringDefinition(const Json& j) {
    AuthoringDefinition d;
    static_cast<ScenarioDefinition&>(d) = parseDefinition(j);
    d.externalVehicleTypes = !j.contains("vehicleTypes");
    d.externalBehaviours = !j.contains("behaviours");
    return d;
}
Json definitionJson(const AuthoringDefinition& d) {
    Json j = {{"duration", d.duration}, {"timeStep", d.timeStep},
        {"routes", Json::array()}, {"inputs", Json::array()}, {"signalPrograms", Json::array()}};
    for (const auto& r : d.routes) j["routes"].push_back({{"id",r.id},{"segmentIds",r.segmentIds}});
    for (const auto& i : d.inputs) {
        Json input = {{"id",i.id},{"routeId",i.routeId},{"vehicleTypeId",i.vehicleTypeId},
            {"vehiclesPerHour",i.vehiclesPerHour},{"startTime",i.startTime},{"endTime",i.endTime}};
        // Omitted rather than an empty array when unset, so an unedited input round-trips through
        // an older reader unchanged and the equal-split default never appears in the file.
        if (!i.laneShares.empty()) input["laneShares"] = i.laneShares;
        // M2.2, the same way: absent unless set. The scalars above are then derived from it.
        if (!i.intervals.empty()) {
            input["intervals"] = Json::array();
            for (const auto& p : i.intervals)
                input["intervals"].push_back({{"startTime",p.startTime},{"endTime",p.endTime},
                                              {"vehiclesPerHour",p.vehiclesPerHour}});
        }
        j["inputs"].push_back(std::move(input));
    }
    for (const auto& p : d.signalPrograms) {
        Json phases = Json::array();
        for (const auto& f : p.phases) phases.push_back({{"duration",f.duration},
            {"color",f.color==SignalColor::green?"green":f.color==SignalColor::amber?"amber":"red"}});
        j["signalPrograms"].push_back({{"id",p.id},{"offset",p.offset},{"phases",phases}});
    }
    if (!d.externalVehicleTypes) {
        j["vehicleTypes"] = Json::array();
        for (const auto& t : d.vehicleTypes)
            j["vehicleTypes"].push_back({{"id",t.id},{"length",t.length},{"width",t.width},
                {"desiredSpeed",{{"min",t.desiredSpeed.min},{"max",t.desiredSpeed.max}}},
                {"maxAcceleration",t.maxAcceleration},{"comfortableDeceleration",t.comfortableDeceleration},
                {"maxDeceleration",t.maxDeceleration},{"behaviourId",t.behaviourId}});
    }
    if (!d.externalBehaviours) {
        j["behaviours"] = Json::array();
        for (const auto& b : d.behaviours)
            j["behaviours"].push_back({{"id",b.id},{"standstillDistance",b.standstillDistance},
                {"additiveSafetyDistance",b.additiveSafetyDistance},{"multiplicativeSafetyDistance",b.multiplicativeSafetyDistance},
                {"followingTime",b.followingTime},{"speedThreshold",b.speedThreshold}});
    }
    return j;
}
void migrateRoutesToObjects(const Network& network, AuthoringDefinition& definition) {
    const auto owner = [&](const std::string& id) {
        for (const auto& link : network.links) {
            if (link.id == id) return id;
            for (const auto& lane : link.lanes) if (lane.id == id) return link.id;
        }
        for (const auto& connector : network.connectors) {
            if (connector.id == id) return id;
            for (int i = 0; i < std::max(connector.fromLaneCount, connector.toLaneCount); ++i)
                if (connectorPathId(connector, i) == id) return connector.id;
        }
        return id; // Unknown ids are left for validation to name; this is a rename, not a check.
    };
    for (auto& route : definition.routes) {
        std::vector<std::string> objects;
        for (const auto& id : route.segmentIds) {
            auto mapped = owner(id);
            // Several lanes of one Link collapse to that Link once, which is what makes the
            // mapping idempotent: running it again finds the object ids and keeps them.
            if (objects.empty() || objects.back() != mapped) objects.push_back(std::move(mapped));
        }
        route.segmentIds = std::move(objects);
    }
}
void validateAuthoredDemand(const ProjectDocument& d) {
    if (!d.definition) return;
    // Checked on the authored intervals, before they are expanded: an overlap is a property of
    // the table the author typed, and would otherwise surface as two core inputs that each look fine.
    if (auto periods = inputIntervalIssues(*d.definition); !periods.empty()) throw ValidationError(std::move(periods));
    auto issues = validateScenario(buildScenario(d.network,*d.definition));
    // Authoring supports topology beyond M0. Catalog references are checked on Run.
    std::erase_if(issues,[&](const auto& i) {
        return i.code=="EMPTY_NETWORK" || i.code.rfind("UNSUPPORTED_",0)==0 ||
            (i.code=="UNKNOWN_VEHICLE_TYPE" && d.definition->externalVehicleTypes) ||
            (i.code=="UNKNOWN_BEHAVIOUR" && d.definition->externalBehaviours);
    });
    if (!issues.empty()) throw ValidationError(std::move(issues));
}
}
