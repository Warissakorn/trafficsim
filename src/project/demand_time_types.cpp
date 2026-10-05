#include "demand_time_types.hpp"
#include "demand_paths.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace trafficsim {
const std::vector<CompositionShare>& compositionTypesAt(const Composition& c,double t) {
    for(const auto& p:c.intervals)if(t>=p.startTime && t<p.endTime)return p.types;
    return c.types;
}
bool hasTypeRouting(const AuthoringDefinition& d) {
    return std::any_of(d.routingDecisions.begin(),d.routingDecisions.end(),[](const auto& x){return !x.typeRules.empty();});
}
bool hasTimeTypeDemand(const AuthoringDefinition& d) {
    return hasTypeRouting(d) || std::any_of(d.compositions.begin(),d.compositions.end(),[](const auto& c){return !c.intervals.empty();});
}
double typeDecisionFlowAt(const RoutingDecision& d,std::size_t route,double t,const std::string& type) {
    for(const auto& rule:d.typeRules)if(rule.vehicleTypeId==type) {
        for(std::size_t k=0;k<d.intervals.size();++k)if(t>=d.intervals[k].startTime && t<d.intervals[k].endTime) {
            double sum=0;for(const auto& row:rule.intervalFlows)if(k<row.size())sum+=row[k];
            return sum>0 && route<rule.intervalFlows.size() && k<rule.intervalFlows[route].size()
                ?rule.intervalFlows[route][k] : route<rule.relativeFlows.size()?rule.relativeFlows[route]:0;
        }
        return route<rule.relativeFlows.size()?rule.relativeFlows[route]:0;
    }
    return decisionFlowAt(d,d.routes.at(route),t);
}
std::vector<ValidationIssue> timeTypeIssues(const AuthoringDefinition& d) {
    std::vector<ValidationIssue> issues;
    const auto known=[&](const std::string& id){return std::any_of(d.vehicleTypes.begin(),d.vehicleTypes.end(),[&](const auto& t){return t.id==id;});};
    for(std::size_t k=0;k<d.routingDecisions.size();++k) {
        const auto& x=d.routingDecisions[k];std::set<std::string> ids;
        for(std::size_t r=0;r<x.typeRules.size();++r) {
            const auto& rule=x.typeRules[r];const auto path="routingDecisions["+std::to_string(k)+"].typeRules["+std::to_string(r)+"]";
            if(rule.vehicleTypeId.empty())issues.push_back({"INVALID_ID",path+".vehicleTypeId"});
            if(!ids.insert(rule.vehicleTypeId).second)issues.push_back({"DUPLICATE_ID",path+".vehicleTypeId"});
            if(!d.externalVehicleTypes && !known(rule.vehicleTypeId))issues.push_back({"UNKNOWN_VEHICLE_TYPE",path+".vehicleTypeId"});
            if(rule.relativeFlows.size()!=x.routes.size() || rule.intervalFlows.size()!=x.routes.size())
                issues.push_back({"ROUTING_DECISION_INTERVALS",path});
            double sum=0;bool valid=true;
            for(double f:rule.relativeFlows){valid=valid && std::isfinite(f) && f>=0;sum+=f;}
            if(!valid || !std::isfinite(sum) || !(sum>0))issues.push_back({"INVALID_SHARE",path+".relativeFlows"});
            for(const auto& row:rule.intervalFlows) {
                if(row.size()!=x.intervals.size())issues.push_back({"ROUTING_DECISION_INTERVALS",path+".intervalFlows"});
                for(double f:row)if(!std::isfinite(f)||f<0){issues.push_back({"INVALID_SHARE",path+".intervalFlows"});break;}
            }
            for(std::size_t n=0;n<x.intervals.size();++n) {
                double at=0;for(const auto& row:rule.intervalFlows)if(n<row.size())at+=row[n];
                if(!std::isfinite(at))issues.push_back({"INVALID_SHARE",path+".intervalFlows"});
            }
        }
    }
    return issues;
}
std::vector<VehicleInput> expandCompositions(const std::vector<VehicleInput>& original,const std::vector<Composition>& all) {
    std::vector<VehicleInput> result;
    for(const auto& input:original) {
        if(input.compositionId.empty()){result.push_back(input);continue;}
        const auto at=std::find_if(all.begin(),all.end(),[&](const auto& c){return c.id==input.compositionId;});
        if(at==all.end())continue; // validation names it before Run
        const auto& c=*at;std::vector<std::string> ids;std::vector<double> points;
        const auto collect=[&](const auto& members){for(const auto& t:members)if(std::find(ids.begin(),ids.end(),t.vehicleTypeId)==ids.end())ids.push_back(t.vehicleTypeId);};
        collect(c.types);for(const auto& p:c.intervals){collect(p.types);points.push_back(p.startTime);points.push_back(p.endTime);}
        std::sort(points.begin(),points.end());points.erase(std::unique(points.begin(),points.end()),points.end());
        for(const auto& id:ids) {
            auto part=input;part.compositionId.clear();part.vehicleTypeId=id;
            if(ids.size()>1)part.id=input.id+"/type-"+id;
            if(c.intervals.empty()) {
                double sum=0,share=0;for(const auto& t:c.types){sum+=t.share;if(t.vehicleTypeId==id)share=t.share;}
                const double fraction=share/sum;part.vehiclesPerHour*=fraction;for(auto& p:part.intervals)p.vehiclesPerHour*=fraction;
            } else {
                part.intervals.clear();
                for(const auto& p:cutPeriods(input,points)) {
                    const auto& members=compositionTypesAt(c,p.startTime);double sum=0,share=0;
                    for(const auto& t:members){sum+=t.share;if(t.vehicleTypeId==id)share=t.share;}
                    if(share>0)part.intervals.push_back({p.startTime,p.endTime,p.vehiclesPerHour*(share/sum)});
                }
                if(part.intervals.empty())continue;
                deriveInputTotals(part);
            }
            result.push_back(std::move(part));
        }
    }
    return result;
}
}
