#include "routeless.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <set>

namespace trafficsim {
namespace {
using Fixed = std::set<std::pair<std::string, std::size_t>>; // (family, lane on its decision Link)
// A destination's chain from `laneId`: full if the lane reaches it, else its stub; null if neither.
const FamilyChain* legFrom(const std::vector<std::vector<FamilyChain>>& families, const std::string& laneId, bool stub) {
    for (const auto& family : families)
        for (const auto& chain : family)
            if (chain.stub == stub && !chain.ids.empty() && chain.ids.front() == laneId) return &chain;
    return nullptr;
}
std::string familyName(const PlacedDecision& d, const PlacedDecision::Destination& destination) {
    if(d.position)return d.id+">route-"+std::to_string(&destination-d.destinations.data());
    const auto& objects = destination.chains.front();
    return d.id + ">" + (objects.empty() ? std::string{} : objects.back());
}
struct Walk {
    const Network& network;
    const std::vector<PlacedDecision>& decisions;
    const std::vector<ConnectorPath>& paths; // every connector path in the drawing, in connector order
    std::string start;                       // the Link the walk enters on
    const Fixed& fixed;                      // D93 rule 4: downstream stubs not kept
    RoutelessResult result;
    bool stopped{};
    std::vector<FamilyTag> families;               // the families being walked (M3.2.8b, D93)
    std::map<std::string, std::string> decisionOf; // family -> its decision's path
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
    void finish(std::vector<std::string> chain, std::size_t lane, double share, bool stub = false) {
        if (result.chains.size() >= kMaxRoutelessPaths) {
            if (!stopped) result.issues.push_back({"ROUTELESS_TOO_MANY_PATHS", {}});
            stopped = true; return;
        }
        result.chains.push_back({std::move(chain), lane, share, stub, families});
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
        if (entered) if (const auto* d = decisionOn(linkId)) if ((!d->position || !arrived || *arrived<=*d->position) && decide(*d, chain, laneId, lane, share)) return;
        free(std::move(chain), laneId, arrived, lane, share);
    }
    // D93 (contract §2, "Downstream routing decisions"): a decision past the entry Link draws
    // among the destinations the arrival lane serves -- with its full chain, or with a stub its
    // vehicles change lanes from on this Link -- by relative flow, as an entry decision does.
    bool decideDownstream(const PlacedDecision& d, const std::vector<std::string>& chain, const std::string& laneId,
                          std::size_t lane, double share) {
        const auto* on = link(d.linkId);
        std::size_t k = 0;
        while (on && k < on->lanes.size() && on->lanes[k].id != laneId) ++k;
        struct Leg { FamilyChain chain; double weight; std::string name; };
        std::vector<Leg> legs;
        double sum = 0;
        for (const auto& destination : d.destinations) {
            std::vector<std::vector<FamilyChain>> family;
            for (const auto& objects : destination.chains) family.push_back(routeLaneFamily(network, objects));
            const auto name = familyName(d, destination);
            decisionOf[name] = d.path;
            const auto* leg = legFrom(family, laneId, false);
            if (!leg && !fixed.contains({name, k})) leg = legFrom(family, laneId, true);
            if (leg) { legs.push_back({*leg, destination.weight, name}); sum += destination.weight; }
        }
        if (legs.empty() || !(sum > 0)) {
            if (std::none_of(result.advisories.begin(), result.advisories.end(),
                             [&](const auto& i) { return i.path == d.path; }))
                result.advisories.push_back({"ROUTING_DECISION_LANE_UNSERVED", d.path});
            return false; // it carries on as if the decision were not there
        }
        for (const auto& leg : legs) {
            families.push_back({leg.name, d.linkId, k});
            const double each = share * leg.weight / sum;
            if (!leg.chain.stub) follow(chain, leg.chain.ids, d.linkId, lane, each);
            else {
                auto stub = chain;
                bool looped = false;
                for (std::size_t i = 1; i < leg.chain.ids.size() && !looped; ++i) {
                    looped = std::find(stub.begin(), stub.end(), leg.chain.ids[i]) != stub.end();
                    stub.push_back(leg.chain.ids[i]);
                }
                if (looped) cycle(); else finish(std::move(stub), lane, each, true);
            }
            families.pop_back();
        }
        return true;
    }
    bool decide(const PlacedDecision& d, const std::vector<std::string>& chain, const std::string& laneId,
                std::size_t lane, double share) {
        if (d.linkId != start) return decideDownstream(d, chain, laneId, lane, share);
        // On the entry Link, reached only when entryDecision served nothing: only the
        // destinations this lane can reach, as before M3.2.8b.
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
    // The entry Link carries the decision: each destination's share goes equally to every lane of
    // the Link. A lane that reaches it takes its full chain; one that does not takes the stub its
    // vehicles change lanes from (M3.2.8b). A lane with neither -- no neighbouring chain leads to a
    // full one -- takes no share, as before.
    bool entryDecision(const Link& start) {
        const auto* d = decisionOn(start.id);
        if (!d) return false;
        struct Leg { std::size_t lane; FamilyChain chain; };
        struct Served { double weight; std::string family; std::vector<Leg> legs; };
        std::vector<Served> served;
        double sum = 0;
        for (const auto& destination : d->destinations) {
            Served s{destination.weight, familyName(*d, destination), {}};
            std::vector<std::vector<FamilyChain>> family;
            for (const auto& objects : destination.chains) family.push_back(routeLaneFamily(network, objects));
            for (std::size_t k = 0; k < start.lanes.size(); ++k) {
                const auto* leg = legFrom(family, start.lanes[k].id, false);
                if (!leg) leg = legFrom(family, start.lanes[k].id, true);
                if (leg) s.legs.push_back({k, *leg});
            }
            if (!s.legs.empty() && destination.weight > 0) { sum += destination.weight; served.push_back(std::move(s)); }
        }
        if (!(sum > 0)) return false;
        result.byDestination = true;
        for (const auto& s : served) {
            for (const auto& leg : s.legs) {
                families.push_back({s.family, start.id, leg.lane});
                const double share = s.weight / sum / static_cast<double>(s.legs.size());
                if (leg.chain.stub) finish(leg.chain.ids, leg.lane, share, true);
                else follow({start.lanes[leg.lane].id}, leg.chain.ids, start.id, leg.lane, share);
                families.pop_back();
            }
        }
        return true;
    }
    void free(std::vector<std::string> chain, const std::string& laneId, std::optional<double> arrived,
              std::size_t lane, double share) {
        std::vector<const ConnectorPath*> out;
        bool fromEnd = false;
        double end = 0;
        for (const auto& l : network.links)
            for (const auto& ln : l.lanes) if (ln.id == laneId) end = polylineLength(l.geometry);
        for (const auto& p : paths) {
            if (p.from.laneId != laneId) continue;
            // A way out upstream of where the vehicle came onto the lane is behind it.
            if (p.from.station && arrived && *p.from.station < *arrived - 1e-9) continue;
            // D44: a path leaving within kRoutelessStubLength of the end leaves "from the end" --
            // the remainder cannot hold a vehicle, so it is not a way out of the network.
            if (!p.from.station || *p.from.station >= end - kRoutelessStubLength) fromEnd = true;
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
// D93 rule 4: a downstream stub is kept only when a run of adjacent lanes of its decision Link,
// each with a path of its family, leads from its lane to one with a full path of the family.
Fixed unkeptStubs(const RoutelessResult& r, const std::string& start) {
    std::map<std::string, std::pair<std::set<std::size_t>, std::set<std::size_t>>> lanes; // any, full
    for (const auto& c : r.chains)
        for (std::size_t i = 0; i < c.families.size(); ++i) {
            const auto& tag = c.families[i];
            if (tag.linkId == start) continue;
            lanes[tag.name].first.insert(tag.lane);
            if (!(c.stub && i + 1 == c.families.size())) lanes[tag.name].second.insert(tag.lane);
        }
    Fixed unkept;
    for (const auto& c : r.chains) {
        if (!c.stub || c.families.empty() || c.families.back().linkId == start) continue;
        const auto& tag = c.families.back();
        const auto& [any, full] = lanes[tag.name];
        bool kept = false;
        for (std::size_t k = tag.lane; !kept && any.contains(k); ++k) kept = full.contains(k);
        for (std::size_t k = tag.lane; !kept && k > 0 && any.contains(k - 1); --k) kept = full.contains(k - 1);
        if (!kept) unkept.insert({tag.name, tag.lane});
    }
    return unkept;
}
}
RoutelessResult routelessChains(const Network& network, const std::string& linkId,
                                const std::vector<PlacedDecision>& decisions) {
    std::vector<ConnectorPath> paths;
    for (const auto& connector : network.connectors) {
        try { for (auto& p : connectorPaths(network, connector)) paths.push_back(std::move(p)); }
        catch (const std::exception&) {} // a broken Connector is reported by its own diagnostics
    }
    // Walk, drop the downstream stubs rule 4 does not keep, and walk again until none is dropped.
    // Each pass drops at least one, so this ends; with no downstream decision it is one pass.
    Fixed fixed;
    for (;;) {
        Walk walk{network, decisions, paths, linkId, fixed, {}, false, {}, {}};
        const auto* start = walk.link(linkId);
        if (!start) { walk.result.issues.push_back({"UNKNOWN_LINK", {}}); return walk.result; }
        if (!walk.entryDecision(*start))
            for (std::size_t k = 0; k < start->lanes.size(); ++k)
                walk.at({start->lanes[k].id}, linkId, start->lanes[k].id, std::nullopt, true, k, 1.0);
        const auto unkept = walk.result.issues.empty() ? unkeptStubs(walk.result, linkId) : Fixed{};
        if (unkept.empty()) {
            for (const auto& [name, lane] : fixed) {
                const auto& path = walk.decisionOf[name];
                if (std::none_of(walk.result.advisories.begin(), walk.result.advisories.end(), [&](const auto& i) {
                        return i.code == "ROUTING_DECISION_LANE_FIXED" && i.path == path; }))
                    walk.result.advisories.push_back({"ROUTING_DECISION_LANE_FIXED", path});
            }
            return walk.result;
        }
        fixed.insert(unkept.begin(), unkept.end());
    }
}
}
