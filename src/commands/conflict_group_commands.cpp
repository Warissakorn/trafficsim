#include "right_of_way_commands.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace trafficsim {
namespace {
ConflictGroup groupOf(const Network& n,const std::string& id) {
    for(const auto& g:conflictGroups(n,automaticConflicts(n)))if(conflictGroupContains(g,id))return g;
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
ConflictArea areaOf(const Network& n,const std::string& id) {
    for(const auto& a:n.rightOfWay.conflictAreas)if(a.id==id)return a;
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
ConflictPriority priorityFor(const ConflictArea& a,const std::string& yielding) {
    if(yielding.empty())return ConflictPriority::undetermined;
    return conflictOwner(a.first.path)==yielding?ConflictPriority::firstYields:ConflictPriority::secondYields;
}
}
std::string authorConflictGroup(ProjectDocument& d,const std::string& key,const PriorityDefaults& defaults) {
    const auto g=groupOf(d.network,key);const auto automatic=automaticConflicts(d.network);
    std::string first=g.areaIds.empty()?std::string{}:g.areaIds.front();std::set<std::string> merges;
    for(const auto& id:g.automaticKeys) {
        const auto at=std::find_if(automatic.begin(),automatic.end(),[&](const auto& a){return a.key==id;});
        if(at==automatic.end())throw std::invalid_argument("EDIT_NO_CROSSING");
        if(at->kind==ConflictKind::merge && !merges.insert(at->mergeSection).second)continue;
        const auto created=authorAutomaticConflict(d,*at,defaults);if(first.empty())first=created;
    }
    return first;
}
void setConflictGroupControl(ProjectDocument& d,const std::string& id,const std::string& name,
                             ConflictPriority priority,double gap,double headway) {
    const auto representative=areaOf(d.network,id);
    const std::string yielding=priority==ConflictPriority::undetermined?std::string{}:
        conflictOwner(priority==ConflictPriority::firstYields?representative.first.path:representative.second.path);
    const auto key=authorConflictGroup(d,id,{gap,headway});
    const auto g=groupOf(d.network,key);
    for(const auto& member:g.areaIds) {
        const auto a=areaOf(d.network,member);
        setConflictControl(d,member,name,priorityFor(a,yielding),gap,headway);
    }
}
void cycleConflictGroupPriority(ProjectDocument& d,const std::string& id,const PriorityDefaults& defaults) {
    const auto representative=areaOf(d.network,id);
    const auto next=representative.priority==ConflictPriority::firstYields?ConflictPriority::secondYields:
        representative.priority==ConflictPriority::secondYields?ConflictPriority::undetermined:ConflictPriority::firstYields;
    const std::string yielding=next==ConflictPriority::undetermined?std::string{}:
        conflictOwner(next==ConflictPriority::firstYields?representative.first.path:representative.second.path);
    const auto key=authorConflictGroup(d,id,defaults);const auto g=groupOf(d.network,key);
    for(const auto& member:g.areaIds) {
        const auto a=areaOf(d.network,member);
        const auto& rules=d.network.rightOfWay.priorityRules;
        const auto r=std::find_if(rules.begin(),rules.end(),[&](const auto& r){return r.conflictAreaId==member;});
        const double gap=r==rules.end()?defaults.gapTime:r->gapTime,headway=r==rules.end()?defaults.headway:r->headway;
        setConflictControl(d,member,a.name,priorityFor(a,yielding),gap,headway);
    }
}
void setConflictGroupStopControl(ProjectDocument& d,const std::string& id,std::optional<StopMode> mode) {
    const auto g=groupOf(d.network,id);std::set<std::string> lines;
    for(const auto& member:g.areaIds) {
        const auto a=areaOf(d.network,member);
        if(a.priority==ConflictPriority::undetermined)throw std::invalid_argument("EDIT_UNDETERMINED_PRIORITY");
        const auto& line=(a.priority==ConflictPriority::firstYields?a.first:a.second).waitingLineId;
        if(lines.insert(line).second)setAreaControl(d,member,mode);
    }
}
void removeConflictGroup(ProjectDocument& d,const std::string& id) {
    const auto g=groupOf(d.network,id);for(const auto& member:g.areaIds)removeConflictArea(d,member);
}
}
