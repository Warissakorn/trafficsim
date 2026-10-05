#include "demand_catalog.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace trafficsim {
std::vector<ValidationIssue> ownedCatalogIssues(const AuthoringDefinition& d) {
    std::vector<ValidationIssue> issues;
    const auto typeExists=[&](const std::string& id){return std::any_of(d.vehicleTypes.begin(),d.vehicleTypes.end(),[&](const auto& t){return t.id==id;});};
    if(!d.externalVehicleTypes)for(const auto& [id,name]:d.vehicleTypeNames)
        if(!typeExists(id))issues.push_back({"UNKNOWN_VEHICLE_TYPE","vehicleTypeNames."+id});
    if(d.externalCompositions)return issues;
    std::set<std::string> ids;
    for(std::size_t k=0;k<d.compositions.size();++k) {
        const auto& c=d.compositions[k];const auto path="compositions["+std::to_string(k)+"]";
        if(c.id.find_first_not_of(" \t\r\n")==std::string::npos)issues.push_back({"INVALID_ID",path+".id"});
        else if(!ids.insert(c.id).second)issues.push_back({"DUPLICATE_ID",path+".id"});
        const auto validateMembers=[&](const std::vector<CompositionShare>& members,const std::string& atPath) {
            std::set<std::string> types;double sum=0;bool valid=!members.empty();
            for(std::size_t n=0;n<members.size();++n) {
                const auto& member=members[n];const auto at=atPath+"["+std::to_string(n)+"]";
                valid=valid && std::isfinite(member.share) && member.share>0;sum+=member.share;
                if(!types.insert(member.vehicleTypeId).second)issues.push_back({"DUPLICATE_ID",at+".vehicleTypeId"});
                if(!d.externalVehicleTypes && !typeExists(member.vehicleTypeId))issues.push_back({"UNKNOWN_VEHICLE_TYPE",at+".vehicleTypeId"});
            }
            if(!valid || !std::isfinite(sum) || !(sum>0))issues.push_back({"INVALID_SHARE",atPath});
        };
        validateMembers(c.types,path+".types");double end=0;
        for(std::size_t n=0;n<c.intervals.size();++n) {
            const auto& p=c.intervals[n];const auto at=path+".intervals["+std::to_string(n)+"]";
            if(!std::isfinite(p.startTime)||!std::isfinite(p.endTime)||p.startTime<end||p.startTime>=p.endTime)
                issues.push_back({"INVALID_INTERVAL",at});
            end=p.endTime;validateMembers(p.types,at+".types");
        }
    }
    for(std::size_t k=0;k<d.inputs.size();++k) {
        const auto& id=d.inputs[k].compositionId;
        if(!id.empty() && !ids.contains(id))issues.push_back({"UNKNOWN_COMPOSITION","inputs["+std::to_string(k)+"].compositionId"});
    }
    return issues;
}
}
