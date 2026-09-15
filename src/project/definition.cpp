#include "document.hpp"
#include "../core/validate.hpp"
#include <algorithm>

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
    for (const auto& i : d.inputs)
        j["inputs"].push_back({{"id",i.id},{"routeId",i.routeId},{"vehicleTypeId",i.vehicleTypeId},
            {"vehiclesPerHour",i.vehiclesPerHour},{"startTime",i.startTime},{"endTime",i.endTime}});
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
void validateAuthoredDemand(const ProjectDocument& d) {
    if (!d.definition) return;
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
