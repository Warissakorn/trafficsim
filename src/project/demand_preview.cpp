#include "demand_preview.hpp"
#include "demand_paths.hpp"
#include <algorithm>
namespace trafficsim {
DemandPreview previewDemand(const ProjectDocument& d,const std::filesystem::path& data) {
    const auto snapshot=compileDocument(d,data);
    DemandPreview result;result.revision=snapshot.revision;
    result.advisories=demandAdvisories(d.network,*d.definition);
    const auto sections=runtimeSections(snapshot.network);
    for(const auto& input:snapshot.scenario.inputs) {
        DemandPreviewRow row;row.inputId=input.id;row.vehicleTypeId=input.vehicleTypeId;row.routeId=input.routeId;
        for(const auto& route:snapshot.scenario.routes)if(route.id==input.routeId && !route.segmentIds.empty()) {
            for(const auto& section:sections.sections) {
                if(section.id==route.segmentIds.front()){row.entryLinkId=section.linkId;row.entryLaneId=section.laneId;}
                if(section.id==route.segmentIds.back())row.lastLinkId=section.linkId;
            }
        }
        row.deferredRouting=std::any_of(snapshot.scenario.routeDecisions.begin(),snapshot.scenario.routeDecisions.end(),[&](const auto& x){return x.fromRouteId==input.routeId;});
        if(row.deferredRouting)row.lastLinkId.clear();
        for(const auto& period:inputPeriods(input)) {
            row.startTime=std::max(0.0,period.startTime);row.endTime=std::min(snapshot.scenario.duration,period.endTime);
            if(row.endTime<=row.startTime)continue;
            row.vehiclesPerHour=period.vehiclesPerHour;
            row.expectedVehicles=period.vehiclesPerHour*(row.endTime-row.startTime)/3600;
            result.expectedVehicles+=row.expectedVehicles;result.rows.push_back(row);
        }
    }
    return result;
}
}
