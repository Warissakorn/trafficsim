#include "routeless.hpp"
#include <algorithm>
#include <map>
#include <optional>

namespace trafficsim {
namespace {
struct Walk {
    const Network& network;
    const std::vector<PlacedDecision>& decisions;
    std::vector<ConnectorPath> paths; // every connector path in the drawing, in connector order
    RoutelessResult result;
    bool stopped{};
    const Link* link(const std::string& id) const {
        for (const auto& l : network.links) if (l.id == id) return &l;
        return nullptr;
    }
    const ConnectorPath* path(const std::string& id) const {
        for (const auto& p : paths) if (p.id == id) return &p;
        return nullptr;
    }
    const PlacedDecision* decisionOn(const std::string& linkId) const {
        for (const auto& d : decisions) if (d.linkId == linkId) return &d;
        return nullptr;
    }
    void finish(std::vector<std::string> chain, std::size_t lane, double share) {
        if (result.chains.size() >= kMaxRoutelessPaths) {
            if (!stopped) result.issues.push_back({"ROUTELESS_TOO_MANY_PATHS", {}});
            stopped = true; return;
        }
        result.chains.push_back({std::move(chain), lane, share});
    }
    void cycle() {
        if (std::none_of(result.issues.begin(), result.issues.end(), [](const auto& i) { return i.code == "ROUTELESS_CYCLE"; }))
            result.issues.push_back({"ROUTELESS_CYCLE", {}});
    }
    // The vehicle is on `laneId` of `linkId`, having arrived at `arrived` (absent: the lane
    // start). `entered` is true when it has just come onto the Link, which is where a placed
    // decision acts (its station along the Link is not modelled, M2.1).
    void at(std::vector<std::string> chain, const std::string& linkId, const std::string& laneId,
            std::optional<double> arrived, bool entered, std::size_t lane, double share) {
        if (stopped) return;
        if (entered) if (const auto* d = decisionOn(linkId)) if (decide(*d, chain, laneId, lane, share)) return;
        free(std::move(chain), laneId, arrived, lane, share);
    }
    bool decide(const PlacedDecision& d, const std::vector<std::string>& chain, const std::string& laneId,
                std::size_t lane, double share) {
        // Only the destinations this lane can reach: with no lane changing, a vehicle in the
        // wrong lane for one cannot get there, so the others share its flow.
        std::vector<std::pair<std::vector<std::string>, double>> legs;
        double sum = 0;
        for (const auto& destination : d.destinations) {
            bool served = false;
            for (const auto& objects : destination.chains) {
                for (auto& leg : routeLaneChains(network, objects))
                    if (!leg.empty() && leg.front() == laneId) {
                        legs.push_back({std::move(leg), destination.weight}); sum += destination.weight; served = true; break;
                    }
                if (served) break;
            }
        }
        if (legs.empty() || !(sum > 0)) {
            if (std::none_of(result.advisories.begin(), result.advisories.end(),
                             [&](const auto& i) { return i.path == d.path; }))
                result.advisories.push_back({"ROUTING_DECISION_LANE_UNSERVED", d.path});
            return false; // it carries on as if the decision were not there
        }
        for (auto& [leg, weight] : legs) follow(chain, leg, d.linkId, lane, share * weight / sum);
        return true;
    }
    // Drive a decision's leg to its destination, then carry on without a route from there.
    void follow(std::vector<std::string> chain, const std::vector<std::string>& leg, const std::string& decisionLink,
                std::size_t lane, double share) {
        for (std::size_t k = 1; k < leg.size(); ++k) {
            if (std::find(chain.begin(), chain.end(), leg[k]) != chain.end()) { cycle(); return; }
            chain.push_back(leg[k]);
        }
        // The leg ends on a lane of the destination Link; where it arrived is the arriving path's.
        std::optional<double> arrived;
        std::string arrivedLink = decisionLink;
        if (leg.size() >= 2) if (const auto* p = path(leg[leg.size() - 2])) { arrived = p->to.station; arrivedLink = p->to.linkId; }
        at(std::move(chain), arrivedLink, leg.back(), arrived, leg.size() >= 2, lane, share);
    }
    // The entry Link carries the decision: route each destination's share to the lanes reaching it.
    bool entryDecision(const Link& start) {
        const auto* d = decisionOn(start.id);
        if (!d) return false;
        struct Served { double weight; std::vector<std::pair<std::size_t, std::vector<std::string>>> legs; };
        std::vector<Served> served;
        double sum = 0;
        for (const auto& destination : d->destinations) {
            Served s{destination.weight, {}};
            for (std::size_t k = 0; k < start.lanes.size(); ++k) {
                bool found = false;
                for (const auto& objects : destination.chains) {
                    for (auto& leg : routeLaneChains(network, objects))
                        if (!leg.empty() && leg.front() == start.lanes[k].id) { s.legs.push_back({k, std::move(leg)}); found = true; break; }
                    if (found) break;
                }
            }
            if (!s.legs.empty() && destination.weight > 0) { sum += destination.weight; served.push_back(std::move(s)); }
        }
        if (!(sum > 0)) return false;
        result.byDestination = true;
        for (const auto& s : served)
            for (const auto& [k, leg] : s.legs)
                follow({start.lanes[k].id}, leg, start.id, k, s.weight / sum / static_cast<double>(s.legs.size()));
        return true;
    }
    void free(std::vector<std::string> chain, const std::string& laneId, std::optional<double> arrived,
              std::size_t lane, double share) {
        std::vector<const ConnectorPath*> out;
        bool fromEnd = false;
        for (const auto& p : paths) {
            if (p.from.laneId != laneId) continue;
            // A way out upstream of where the vehicle came onto the lane is behind it.
            if (p.from.station && arrived && *p.from.station < *arrived - 1e-9) continue;
            if (!p.from.station) fromEnd = true;
            out.push_back(&p);
        }
        const std::size_t ways = out.size() + (fromEnd ? 0 : 1);
        const double each = share / static_cast<double>(ways);
        for (const auto* p : out) {
            if (std::find(chain.begin(), chain.end(), p->to.laneId) != chain.end()) { cycle(); continue; }
            auto next = chain; next.push_back(p->id); next.push_back(p->to.laneId);
            at(std::move(next), p->to.linkId, p->to.laneId, p->to.station, true, lane, each);
        }
        // Nothing leaves from the lane's end: the vehicle that reaches it leaves the network.
        if (!fromEnd) finish(std::move(chain), lane, each);
    }
};
}
RoutelessResult routelessChains(const Network& network, const std::string& linkId,
                                const std::vector<PlacedDecision>& decisions) {
    Walk walk{network, decisions, {}, {}, false};
    for (const auto& connector : network.connectors) {
        try { for (auto& p : connectorPaths(network, connector)) walk.paths.push_back(std::move(p)); }
        catch (const std::exception&) {} // a broken Connector is reported by its own diagnostics
    }
    const auto* start = walk.link(linkId);
    if (!start) { walk.result.issues.push_back({"UNKNOWN_LINK", {}}); return walk.result; }
    if (walk.entryDecision(*start)) return walk.result;
    for (std::size_t k = 0; k < start->lanes.size(); ++k)
        walk.at({start->lanes[k].id}, linkId, start->lanes[k].id, std::nullopt, true, k, 1.0);
    return walk.result;
}
}
