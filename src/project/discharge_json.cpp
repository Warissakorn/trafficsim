#include "evaluation.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
Json dischargeJson(const std::vector<DischargeCycle>& cycles,const DischargeSpec& spec) {
    validateDischargeSpec(spec);
    Json rows=Json::array();
    for(const auto& c:cycles) {
        const auto r=estimateDischarge(c,spec);
        Json crossings=Json::array();
        for(const auto& v:c.crossings)crossings.push_back({{"vehicleId",v.vehicleId},
            {"vehicleTypeId",v.vehicleTypeId},{"time",v.time},{"queuedAtGo",v.queuedAtGo}});
        rows.push_back({{"headId",c.headId},{"laneId",c.laneId},{"go",c.go},{"end",c.end},
            {"timeStep",c.timeStep},{"complete",c.complete},{"crossings",crossings},
            {"unavailableReason",r.reason},{"samples",r.samples},
            {"meanHeadwaySeconds",r.meanHeadway?Json(*r.meanHeadway):Json(nullptr)},
            {"dischargeVehiclesPerHour",r.dischargeVehiclesPerHour?Json(*r.dischargeVehiclesPerHour):Json(nullptr)},
            {"startupLostTimeSeconds",r.startupLostTime?Json(*r.startupLostTime):Json(nullptr)}});
    }
    return {{"label","not yet validated; observed queue discharge, not calibrated capacity or PCU"},
        {"windowStart",spec.windowStart},{"windowEnd",spec.windowEnd},{"warmup",spec.warmup},
        {"steadyFirstRank",spec.steadyFirst},{"steadyLastRank",spec.steadyLast},
        {"startupLastRank",spec.startupLast},{"classFilter","all vehicle types; no PCU conversion"},
        {"timestampConvention","end-of-tick front crossing; half-open green intervals"},{"cycles",rows}};
}
}
