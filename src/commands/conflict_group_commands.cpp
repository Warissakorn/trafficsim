#include "right_of_way_commands.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
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
std::string authorConflictGroupImpl(ProjectDocument& d,const std::string& key,const PriorityDefaults& defaults) {
    const auto g=groupOf(d.network,key);
    if(g.geometryKind==ConflictGeometryKind::branching)throw std::invalid_argument("EDIT_BRANCHING_PRIORITY");
    const auto automatic=automaticConflicts(d.network);
    std::string first=g.areaIds.empty()?std::string{}:g.areaIds.front();std::set<std::string> merges;
    for(const auto& id:g.automaticKeys) {
        const auto at=std::find_if(automatic.begin(),automatic.end(),[&](const auto& a){return a.key==id;});
        if(at==automatic.end())throw std::invalid_argument("EDIT_NO_CROSSING");
        if(at->kind==ConflictKind::branching)continue;
        if(at->kind==ConflictKind::merge && !merges.insert(at->mergeSection).second)continue;
        const auto created=authorAutomaticConflict(d,*at,defaults);if(first.empty())first=created;
    }
    // A newly authored mixed site takes one owner-relative decision, including merge defaults.
    if(g.kinds.size()>1 && !first.empty()) {
        const auto representative=areaOf(d.network,first);
        const std::string yielding=representative.priority==ConflictPriority::undetermined?std::string{}:
            conflictOwner(representative.priority==ConflictPriority::firstYields?representative.first.path:representative.second.path);
        for(const auto& id:groupOf(d.network,first).areaIds) {
            if(std::find(g.areaIds.begin(),g.areaIds.end(),id)!=g.areaIds.end())continue;
            const auto a=areaOf(d.network,id);
            setConflictControl(d,id,a.name,priorityFor(a,yielding),defaults.gapTime,defaults.headway);
        }
    }
    return first;
}
void setConflictGroupControlImpl(ProjectDocument& d,const std::string& id,const std::string& name,
                             ConflictPriority priority,double gap,double headway) {
    const auto representative=areaOf(d.network,id);
    const std::string yielding=priority==ConflictPriority::undetermined?std::string{}:
        conflictOwner(priority==ConflictPriority::firstYields?representative.first.path:representative.second.path);
    const auto key=authorConflictGroupImpl(d,id,{gap,headway});
    const auto g=groupOf(d.network,key);
    for(const auto& member:g.areaIds) {
        const auto a=areaOf(d.network,member);
        setConflictControl(d,member,name,priorityFor(a,yielding),gap,headway);
    }
}
void cycleConflictGroupPriorityImpl(ProjectDocument& d,const std::string& id,const PriorityDefaults& defaults) {
    const auto representative=areaOf(d.network,id);
    const auto next=representative.priority==ConflictPriority::firstYields?ConflictPriority::secondYields:
        representative.priority==ConflictPriority::secondYields?ConflictPriority::undetermined:ConflictPriority::firstYields;
    const std::string yielding=next==ConflictPriority::undetermined?std::string{}:
        conflictOwner(next==ConflictPriority::firstYields?representative.first.path:representative.second.path);
    const auto key=authorConflictGroupImpl(d,id,defaults);const auto g=groupOf(d.network,key);
    for(const auto& member:g.areaIds) {
        const auto a=areaOf(d.network,member);
        const auto& rules=d.network.rightOfWay.priorityRules;
        const auto r=std::find_if(rules.begin(),rules.end(),[&](const auto& r){return r.conflictAreaId==member;});
        const double gap=r==rules.end()?defaults.gapTime:r->gapTime,headway=r==rules.end()?defaults.headway:r->headway;
        setConflictControl(d,member,a.name,priorityFor(a,yielding),gap,headway);
    }
}
void setConflictGroupStopControlImpl(ProjectDocument& d,const std::string& id,std::optional<StopMode> mode) {
    const auto g=groupOf(d.network,id);std::set<std::string> lines;
    for(const auto& member:g.areaIds) {
        const auto a=areaOf(d.network,member);
        if(a.priority==ConflictPriority::undetermined)throw std::invalid_argument("EDIT_UNDETERMINED_PRIORITY");
        const auto& line=(a.priority==ConflictPriority::firstYields?a.first:a.second).waitingLineId;
        if(lines.insert(line).second)setAreaControl(d,member,mode);
    }
}
void removeConflictGroupImpl(ProjectDocument& d,const std::string& id) {
    const auto g=groupOf(d.network,id);for(const auto& member:g.areaIds)removeConflictArea(d,member);
}
// Group commands are atomic even when invoked outside History. Refuse new merge-order cycles.
std::set<std::string> cycles(const ProjectDocument& d) {
    std::set<std::string> result;
    for(const auto& issue:resolveRightOfWay(d.network,runtimeSections(d.network),{1,1}).issues)
        if(issue.code=="CONFLICT_PRIORITY_CYCLE") {
            const auto left=issue.path.find('['),right=issue.path.find(']',left);
            if(left!=std::string::npos && right!=std::string::npos) {
                const auto index=std::stoul(issue.path.substr(left+1,right-left-1));
                if(index<d.network.rightOfWay.conflictAreas.size())result.insert(d.network.rightOfWay.conflictAreas[index].id);
            }
        }
    return result;
}
template<class Edit> auto atomicGroupEdit(ProjectDocument& d,Edit edit) {
    auto candidate=d;const auto before=cycles(d);
    const auto verify=[&] {
        validateDocument(candidate);
        for(const auto& path:cycles(candidate))if(!before.contains(path))throw std::invalid_argument("CONFLICT_PRIORITY_CYCLE");
    };
    if constexpr(std::is_void_v<std::invoke_result_t<Edit,ProjectDocument&>>) {
        edit(candidate);verify();d=std::move(candidate);
    }else {
        auto result=edit(candidate);verify();d=std::move(candidate);return result;
    }
}
}
std::string authorConflictGroup(ProjectDocument& d,const std::string& key,const PriorityDefaults& defaults) {
    return atomicGroupEdit(d,[&](auto& candidate){return authorConflictGroupImpl(candidate,key,defaults);});
}
void setConflictGroupControl(ProjectDocument& d,const std::string& id,const std::string& name,ConflictPriority priority,double gap,double headway) {
    atomicGroupEdit(d,[&](auto& candidate){setConflictGroupControlImpl(candidate,id,name,priority,gap,headway);});
}
void cycleConflictGroupPriority(ProjectDocument& d,const std::string& id,const PriorityDefaults& defaults) {
    atomicGroupEdit(d,[&](auto& candidate){cycleConflictGroupPriorityImpl(candidate,id,defaults);});
}
void setConflictGroupStopControl(ProjectDocument& d,const std::string& id,std::optional<StopMode> mode) {
    atomicGroupEdit(d,[&](auto& candidate){setConflictGroupStopControlImpl(candidate,id,mode);});
}
void removeConflictGroup(ProjectDocument& d,const std::string& id) {
    atomicGroupEdit(d,[&](auto& candidate){removeConflictGroupImpl(candidate,id);});
}
}
