#include "demand_paths.hpp"
#include "document.hpp"
#include "../model/demand/signal_control.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace trafficsim {
AuthoringDefinition parseAuthoringDefinition(const Json& j) {
    AuthoringDefinition d;
    static_cast<ScenarioDefinition&>(d) = parseDefinition(j);
    d.externalVehicleTypes = !j.contains("vehicleTypes");
    d.externalBehaviours = !j.contains("behaviours");
    if (j.contains("routingDecisions")) d.routingDecisions = parseRoutingDecisions(j);
    if (j.contains("signalControllers")) d.signalControllers = parseSignalControllers(j);
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
        if (!i.compositionId.empty()) input["compositionId"] = i.compositionId; // M2.3
        if (!i.routingDecisionId.empty()) input["routingDecisionId"] = i.routingDecisionId; // M2.4
        if (!i.linkId.empty()) input["linkId"] = i.linkId; // M2.1.1
        // M2.2, the same way: absent unless set. The scalars above are then derived from it.
        if (!i.intervals.empty()) {
            input["intervals"] = Json::array();
            for (const auto& p : i.intervals)
                input["intervals"].push_back({{"startTime",p.startTime},{"endTime",p.endTime},
                                              {"vehiclesPerHour",p.vehiclesPerHour}});
        }
        j["inputs"].push_back(std::move(input));
    }
    if (!d.routingDecisions.empty()) { // M2.4, absent unless authored
        j["routingDecisions"] = Json::array();
        for (const auto& x : d.routingDecisions) {
            Json routes = Json::array();
            for (const auto& r : x.routes) {
                routes.push_back({{"routeId",r.routeId},{"relativeFlow",r.relativeFlow}});
                if (!r.destinationLinkId.empty()) routes.back()["destinationLinkId"] = r.destinationLinkId; // M2.1.1
                if (!r.intervalFlows.empty()) routes.back()["intervalFlows"] = r.intervalFlows; // M2.1.2
            }
            Json decision = {{"id",x.id},{"routes",routes}};
            if (!x.name.empty()) decision["name"] = x.name;
            if (!x.linkId.empty()) decision["linkId"] = x.linkId; // M2.1.1
            if (!x.intervals.empty()) { // M2.1.2
                decision["intervals"] = Json::array();
                for (const auto& i : x.intervals) decision["intervals"].push_back({{"startTime",i.startTime},{"endTime",i.endTime}});
            }
            j["routingDecisions"].push_back(std::move(decision));
        }
    }
    for (const auto& p : d.signalPrograms) {
        Json phases = Json::array();
        for (const auto& f : p.phases) phases.push_back({{"duration",f.duration},
            {"color",f.color==SignalColor::green?"green":f.color==SignalColor::amber?"amber":"red"}});
        j["signalPrograms"].push_back({{"id",p.id},{"offset",p.offset},{"phases",phases}});
    }
    if (!d.signalControllers.empty()) { // M2.7b, schema 13, absent unless authored
        j["signalControllers"] = Json::array();
        for (const auto& c : d.signalControllers) {
            Json groups = Json::array();
            for (const auto& g : c.groups) {
                groups.push_back({{"number",g.number},{"greenStart",g.greenStart},{"greenEnd",g.greenEnd},{"amber",g.amber}});
                if (!g.name.empty()) groups.back()["name"] = g.name;
            }
            Json controller = {{"id",c.id},{"cycle",c.cycle},{"offset",c.offset},{"groups",groups}};
            if (!c.name.empty()) controller["name"] = c.name;
            j["signalControllers"].push_back(std::move(controller));
        }
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
std::vector<ValidationIssue> routingDecisionIssues(const AuthoringDefinition& d) {
    std::vector<ValidationIssue> issues;
    const auto route = [&](const std::string& id) -> const Route* {
        for (const auto& r : d.routes) if (r.id == id) return &r;
        return nullptr;
    };
    for (std::size_t k = 0; k < d.routingDecisions.size(); ++k) {
        const auto& decision = d.routingDecisions[k];
        const auto path = "routingDecisions[" + std::to_string(k) + "]";
        if (decision.routes.empty()) { issues.push_back({"INVALID_SHARE", path}); continue; }
        // M2.1.1: one decision per Link, since a vehicle reaching it cannot follow two.
        if (!decision.linkId.empty())
            for (std::size_t e = 0; e < k; ++e)
                if (d.routingDecisions[e].linkId == decision.linkId) issues.push_back({"ROUTING_DECISION_DUPLICATE_LINK", path + ".linkId"});
        for (std::size_t i = 0; i < decision.intervals.size(); ++i) { // M2.1.2, as for an input's
            const auto& p = decision.intervals[i];
            if (!std::isfinite(p.startTime) || !std::isfinite(p.endTime) || p.startTime < 0 || p.startTime >= p.endTime ||
                (i > 0 && p.startTime < decision.intervals[i - 1].endTime))
                issues.push_back({"INVALID_INTERVAL", path + ".intervals[" + std::to_string(i) + "]"});
            // An interval where every flow is zero is allowed: it uses the whole-period flows (D46).
        }
        // A placed decision's routes leave its Link; an unplaced one's leave the first route's.
        std::string origin = decision.linkId;
        for (std::size_t j = 0; j < decision.routes.size(); ++j) {
            const auto& entry = decision.routes[j];
            const auto at = path + ".routes[" + std::to_string(j) + "]";
            if (!(std::isfinite(entry.relativeFlow) && entry.relativeFlow > 0)) issues.push_back({"INVALID_SHARE", at});
            // M2.1.2: one flow per interval; zero is allowed there (a turn closed for a period).
            if (entry.intervalFlows.size() != decision.intervals.size()) issues.push_back({"ROUTING_DECISION_INTERVALS", at});
            for (double f : entry.intervalFlows)
                if (!(std::isfinite(f) && f >= 0)) { issues.push_back({"INVALID_SHARE", at + ".intervalFlows"}); break; }
            // A destination is reached from where the decision sits, so it needs a placed one,
            // and an entry names one destination or one route -- never both.
            if (!entry.destinationLinkId.empty()) {
                if (decision.linkId.empty()) issues.push_back({"ROUTING_DECISION_NEEDS_LINK", at});
                if (!entry.routeId.empty()) issues.push_back({"INPUT_TARGET_CONFLICT", at});
                continue; // reachability depends on the network: placedDecisions checks it
            }
            const auto* r = route(entry.routeId);
            if (!r) { issues.push_back({"UNKNOWN_ROUTE", at}); continue; }
            const auto start = r->segmentIds.empty() ? std::string{} : r->segmentIds.front();
            if (origin.empty()) origin = start;
            else if (start != origin) issues.push_back({"ROUTING_DECISION_MIXED_ORIGIN", at});
        }
    }
    for (std::size_t i = 0; i < d.inputs.size(); ++i) {
        const auto& id = d.inputs[i].routingDecisionId;
        if (!id.empty() && std::none_of(d.routingDecisions.begin(), d.routingDecisions.end(),
                                        [&](const auto& x) { return x.id == id; }))
            issues.push_back({"UNKNOWN_ROUTING_DECISION", "inputs[" + std::to_string(i) + "].routingDecisionId"});
        // An input enters by exactly one of a route, a decision or a Link (M2.1.1).
        const auto& input = d.inputs[i];
        if (int(!input.routeId.empty()) + int(!input.routingDecisionId.empty()) + int(!input.linkId.empty()) > 1)
            issues.push_back({"INPUT_TARGET_CONFLICT", "inputs[" + std::to_string(i) + "]"});
    }
    return issues;
}
AuthoringDefinition withRoutingDecisions(AuthoringDefinition d) {
    std::vector<VehicleInput> inputs;
    for (const auto& input : d.inputs) {
        if (input.routingDecisionId.empty()) { inputs.push_back(input); continue; }
        const auto decision = std::find_if(d.routingDecisions.begin(), d.routingDecisions.end(),
                                           [&](const auto& x) { return x.id == input.routingDecisionId; });
        if (decision == d.routingDecisions.end()) continue; // routingDecisionIssues names it
        // A PLACED decision (M2.1.1) acts where it sits: the input becomes a routeless input on
        // its Link, and expandRouteless meets the decision there on the network walk.
        if (!decision->linkId.empty()) {
            auto routeless = input; routeless.routingDecisionId.clear(); routeless.linkId = decision->linkId;
            inputs.push_back(std::move(routeless)); continue;
        }
        double sum = 0;
        for (const auto& entry : decision->routes) sum += entry.relativeFlow;
        if (!(sum > 0)) continue;
        if (!decision->intervals.empty()) { // M2.1.2: the fraction changes with the entry time
            const auto pieces = cutPeriods(input, decisionBreakpoints({&*decision}));
            for (const auto& entry : decision->routes) {
                auto part = input;
                part.routingDecisionId.clear(); part.routeId = entry.routeId; part.intervals.clear();
                if (decision->routes.size() > 1) { part.id = input.id + "/route-" + entry.routeId; part.laneShares.clear(); }
                for (const auto& piece : pieces) {
                    const double mid = (piece.startTime + piece.endTime) / 2;
                    double at = 0;
                    for (const auto& other : decision->routes) at += decisionFlowAt(*decision, other, mid);
                    const double fraction = at > 0 ? decisionFlowAt(*decision, entry, mid) / at : 0.0;
                    if (fraction > 0) part.intervals.push_back({piece.startTime, piece.endTime, piece.vehiclesPerHour * fraction});
                }
                if (part.intervals.empty()) continue;
                deriveInputTotals(part);
                inputs.push_back(std::move(part));
            }
            continue;
        }
        // Splitting a Poisson stream by fixed proportions is exact, as for a composition.
        for (const auto& entry : decision->routes) {
            auto part = input;
            part.routingDecisionId.clear();
            part.routeId = entry.routeId;
            // Weights for one route's lanes mean nothing on another's: each route splits its
            // lanes equally, the M1.26 default, and laneShares stay on single-route inputs.
            if (decision->routes.size() > 1) { part.id = input.id + "/route-" + entry.routeId; part.laneShares.clear(); }
            const double fraction = entry.relativeFlow / sum;
            part.vehiclesPerHour = input.vehiclesPerHour * fraction;
            for (auto& period : part.intervals) period.vehiclesPerHour *= fraction;
            inputs.push_back(std::move(part));
        }
    }
    d.inputs = std::move(inputs);
    return d;
}
void validateAuthoredDemand(const ProjectDocument& d) {
    if (!d.definition) return;
    // Checked on the authored intervals, before they are expanded: an overlap is a property of
    // the table the author typed, and would otherwise surface as two core inputs that each look fine.
    if (auto periods = inputIntervalIssues(*d.definition); !periods.empty()) throw ValidationError(std::move(periods));
    // An input drawing from a composition (M2.3) names no vehicle type of its own; its types are
    // checked against the catalog on Run, exactly as external vehicle types are. For the checks
    // here it borrows an embedded type, if the document carries any.
    if (auto decisions = routingDecisionIssues(*d.definition); !decisions.empty()) throw ValidationError(std::move(decisions));
    if (auto signals = signalControlIssues(d.network, *d.definition); !signals.empty()) throw ValidationError(std::move(signals));
    // Routeless inputs (M2.1.1) are walked here too; one whose walk fails is left out and named
    // by routelessIssues on Run, so an edit to the network is never refused because of it.
    auto checked = expandRouteless(d.network, *d.definition, withRoutingDecisions(*d.definition));
    for (auto& input : checked.inputs)
        if (!input.compositionId.empty() && !checked.vehicleTypes.empty())
            input.vehicleTypeId = checked.vehicleTypes.front().id;
    auto issues = validateScenario(buildScenario(d.network,checked));
    // Authoring supports topology beyond M0. Catalog references are checked on Run.
    std::erase_if(issues,[&](const auto& i) {
        return i.code=="EMPTY_NETWORK" || i.code.rfind("UNSUPPORTED_",0)==0 ||
            (i.code=="UNKNOWN_VEHICLE_TYPE" && d.definition->externalVehicleTypes) ||
            (i.code=="UNKNOWN_BEHAVIOUR" && d.definition->externalBehaviours);
    });
    if (!issues.empty()) throw ValidationError(std::move(issues));
}
}
