#include "diagnostics.hpp"
#include "../../core/validate.hpp"
#include <algorithm>

namespace trafficsim {
namespace {
// Reads "name[index]" off the front of a path. Returns false for anything else,
// including a trailing "[" with no digits, so a malformed path never indexes a vector.
bool token(const std::string& path, std::size_t& at, std::string& name, std::size_t& index) {
    const auto open = path.find('[', at);
    if (open == std::string::npos) return false;
    name = path.substr(at, open - at);
    const auto close = path.find(']', open);
    if (close == std::string::npos || close == open + 1) return false;
    index = 0;
    for (auto i = open + 1; i < close; ++i) {
        if (path[i] < '0' || path[i] > '9') return false;
        index = index * 10 + static_cast<std::size_t>(path[i] - '0');
        if (index > 1000000) return false; // A path this large cannot name a real object.
    }
    at = close + 1;
    if (at < path.size() && path[at] == '.') ++at;
    return true;
}
}
bool blocksDraft(const std::string& code) { return code != "EMPTY_NETWORK"; }
std::string objectIdForPath(const Network& network, const std::string& path) {
    std::size_t at = 0, index = 0;
    std::string name;
    if (!token(path, at, name, index)) return {};
    if (name == "links") {
        if (index >= network.links.size()) return {};
        const auto& link = network.links[index];
        std::size_t lane = 0;
        std::string inner;
        // A lane names itself; anything else under the link names the link.
        if (token(path, at, inner, lane) && inner == "lanes")
            return lane < link.lanes.size() ? link.lanes[lane].id : std::string{};
        return link.id;
    }
    if (name == "connectors") return index < network.connectors.size() ? network.connectors[index].id : std::string{};
    if (name == "signalHeads") return index < network.signalHeads.size() ? network.signalHeads[index].id : std::string{};
    return {};
}
std::string selectableFor(const Network& network, const std::string& objectId) {
    if (objectId.empty()) return {};
    for (const auto& link : network.links) {
        if (link.id == objectId) return link.id;
        for (const auto& lane : link.lanes) if (lane.id == objectId) return link.id;
    }
    for (const auto& connector : network.connectors)
        for(int i=0;i<std::max(connector.fromLaneCount,connector.toLaneCount);++i)
            if(connectorPathId(connector,i)==objectId)return connector.id;
    for(const auto& head:network.signalHeads)if(head.id==objectId)return head.id;
    return {};
}
namespace {
// Demand objects are not canvas-selectable, but naming them still beats an index path.
std::string definitionId(const ScenarioDefinition& definition, const std::string& path) {
    std::size_t at = 0, index = 0;
    std::string name;
    if (!token(path, at, name, index)) return {};
    const auto pick = [&](const auto& items) -> std::string {
        return index < items.size() ? items[index].id : std::string{};
    };
    if (name == "routes") return pick(definition.routes);
    if (name == "inputs") return pick(definition.inputs);
    if (name == "vehicleTypes") return pick(definition.vehicleTypes);
    if (name == "behaviours") return pick(definition.behaviours);
    if (name == "signalPrograms") return pick(definition.signalPrograms);
    return {};
}
Diagnostic resolve(const Network& network, const ValidationIssue& issue, DiagnosticSeverity severity) {
    // "segments.<id>" is the one path a validator emits with a real id in it (the merge check).
    const auto prefix = std::string("segments.");
    auto object = issue.path.rfind(prefix, 0) == 0 ? issue.path.substr(prefix.size())
                                                   : objectIdForPath(network, issue.path);
    auto select = selectableFor(network, object);
    return {issue.code, issue.path, std::move(object), std::move(select), severity};
}
}
std::vector<Diagnostic> networkDiagnostics(const Network& network) {
    std::vector<Diagnostic> result;
    for (const auto& issue : validateNetwork(network))
        result.push_back(resolve(network, issue, DiagnosticSeverity::draft));
    return result;
}
std::vector<Diagnostic> runtimeDiagnostics(const Network& network, const ScenarioDefinition& definition) {
    // laneGeometry throws on an unknown lane or an invalid driving side, and buildScenario calls
    // it, so the draft pass must clear before anything is assembled. Reporting the skip keeps
    // "no runtime problems" from being claimed about a network that was never compiled.
    for (const auto& issue : validateNetwork(network))
        if (blocksDraft(issue.code))
            return {{"EDIT_RUNTIME_SKIPPED", {}, {}, {}, DiagnosticSeverity::runtime}};
    std::vector<Diagnostic> result;
    for(const auto& issue:connectorRuntimeIssues(network))
        result.push_back(resolve(network,issue,DiagnosticSeverity::runtime));
    // A route is not a network object, so there is nothing on the canvas to select for it;
    // resolve() would look one up and find none. The row names the route's path instead.
    for(const auto& issue:routeRuntimeIssues(network,definition))
        result.push_back({issue.code,issue.path,{},{},DiagnosticSeverity::runtime});
    for(const auto& issue:priorityDefaultsIssues(network,definition.priorityDefaults))
        result.push_back(resolve(network,issue,DiagnosticSeverity::runtime));
    // Shape advisories sit beside the runtime rows: visible and selectable, but they never
    // reach compileScenario, so neither Run nor saving is blocked by one.
    for(const auto& issue:connectorShapeIssues(network))
        result.push_back(resolve(network,issue,DiagnosticSeverity::advisory));
    try {
        const auto scenario = buildScenario(network, definition);
        for (const auto& issue : validateScenario(scenario)) {
            auto row = resolve(network, issue, DiagnosticSeverity::runtime);
            if (row.objectId.empty()) row.objectId = definitionId(definition, issue.path);
            result.push_back(std::move(row));
        }
    } catch (const std::exception& error) {
        return {{error.what(), {}, {}, {}, DiagnosticSeverity::runtime}};
    }
    return result;
}
}
