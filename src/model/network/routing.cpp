#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <optional>

namespace trafficsim {
namespace {
// What an author names in a route: Links and Connectors, never a lane and never a path. A
// Connector's lane count is a drawing decision the author keeps changing, so a route that named
// one would stop meaning anything the moment the Connector was narrowed.
struct RouteObject { std::string id; std::vector<std::string> next; };
std::vector<RouteObject> routeObjects(const Network& network) {
    std::vector<RouteObject> objects;
    for (const auto& link : network.links) objects.push_back({link.id, {}});
    for (const auto& connector : network.connectors) objects.push_back({connector.id, {}});
    const auto add = [&](const std::string& from, const std::string& to) {
        for (auto& object : objects) if (object.id == from)
            if (std::find(object.next.begin(), object.next.end(), to) == object.next.end())
                object.next.push_back(to);
    };
    // Two Links are joined only through a Connector, which is the whole point of the object:
    // Vissim's Link is one carriageway and every turn between two of them is a Connector.
    for (const auto& connector : network.connectors) {
        std::vector<ConnectorPath> paths;
        try { paths = connectorPaths(network, connector); } catch (const std::exception&) { continue; }
        for (const auto& path : paths) {
            add(path.from.linkId, connector.id);
            add(connector.id, path.to.linkId);
        }
    }
    return objects;
}
std::vector<std::string> continuations(const std::vector<RouteObject>& objects,
                                       const std::vector<std::string>& authored) {
    std::vector<std::string> result;
    for (const auto& object : objects) {
        // An empty route may start anywhere; otherwise only where the tail leads.
        bool allowed = authored.empty();
        if (!allowed) for (const auto& last : objects) if (last.id == authored.back())
            allowed = std::find(last.next.begin(), last.next.end(), object.id) != last.next.end();
        // A route that revisited an object would loop. The dialog refused that and the pointer
        // gesture refuses the same thing, or the two disagree about one network.
        if (allowed && std::find(authored.begin(), authored.end(), object.id) == authored.end())
            result.push_back(object.id);
    }
    return result;
}
const Connector* connectorById(const Network& network, const std::string& id) {
    for (const auto& connector : network.connectors) if (connector.id == id) return &connector;
    return nullptr;
}
const Link* linkById(const Network& network, const std::string& id) {
    for (const auto& link : network.links) if (link.id == id) return &link;
    return nullptr;
}
}
std::vector<std::string> routeContinuations(const Network& network,
                                            const std::vector<std::string>& authored) {
    return continuations(routeObjects(network), authored);
}
std::vector<std::string> routeChainTo(const Network& network,
                                      const std::vector<std::string>& authored,
                                      const std::string& target) {
    if (target.empty()) return {};
    const auto objects = routeObjects(network);
    // Breadth first over the same continuation rule, so the shortest chain wins and a longer
    // detour never hides it. Bounded: one click must not walk a large network forever.
    constexpr std::size_t kMaxChain = 20;
    std::deque<std::vector<std::string>> level{{}};
    for (std::size_t depth = 0; depth < kMaxChain && !level.empty(); ++depth) {
        std::deque<std::vector<std::string>> next;
        std::vector<std::string> found;
        for (const auto& chain : level) {
            auto path = authored; path.insert(path.end(), chain.begin(), chain.end());
            for (const auto& candidate : continuations(objects, path)) {
                auto extended = chain; extended.push_back(candidate);
                if (candidate != target) { next.push_back(std::move(extended)); continue; }
                // Two ways to reach the target at the same depth is an ambiguity only the author
                // can settle, by clicking an intermediate object. Guessing one of them would
                // author a route they never chose.
                if (!found.empty() && found != extended) return {};
                found = std::move(extended);
            }
        }
        if (!found.empty()) return found;
        level = std::move(next);
    }
    return {};
}
std::vector<std::vector<std::string>> routeShortestChains(const Network& network, const std::string& from,
                                                          const std::string& target) {
    // The object graph is built once per search: rebuilding it per chain, through
    // routeContinuations, was 9% of an M2.6 Run.
    const auto objects = routeObjects(network);
    constexpr std::size_t kMaxDepth = 20, kMaxLevel = 4096;
    std::vector<std::vector<std::string>> level{{from}};
    for (std::size_t depth = 0; depth < kMaxDepth && !level.empty(); ++depth) {
        std::vector<std::vector<std::string>> next, found;
        for (const auto& chain : level)
            for (const auto& step : continuations(objects, chain)) {
                auto extended = chain; extended.push_back(step);
                (step == target ? found : next).push_back(std::move(extended));
            }
        if (!found.empty()) return found;
        if (next.size() > kMaxLevel) return {};
        level = std::move(next);
    }
    return {};
}
std::vector<FamilyChain> routeLaneFamily(const Network& network, const std::vector<std::string>& objectIds,
                                         std::vector<std::string>* ambiguous) {
    if (objectIds.empty()) return {};
    // A chain in progress: the lane-level ids so far, where the vehicle currently is, the lane it
    // started on, and whether it has stopped on a lane the next object does not leave (M3.2.8b).
    struct Chain { std::vector<std::string> ids; LaneReference at; bool onConnector{}; std::size_t lane{}; bool stub{}; };
    std::vector<Chain> chains;
    const auto* firstLink = linkById(network, objectIds.front());
    const auto pathsOf = [&](const Connector& connector) {
        std::vector<ConnectorPath> paths;
        try { paths = connectorPaths(network, connector); } catch (const std::exception&) { return paths; }
        return paths;
    };
    // The route covers EVERY lane of the Link it starts on. That is the authored meaning: a
    // routing decision belongs to the carriageway, not to one lane of it.
    if (firstLink) {
        for (std::size_t k = 0; k < firstLink->lanes.size(); ++k)
            chains.push_back({{firstLink->lanes[k].id}, {firstLink->id, firstLink->lanes[k].id}, false, k});
    } else if (const auto* connector = connectorById(network, objectIds.front())) {
        const auto paths = pathsOf(*connector);
        for (std::size_t k = 0; k < paths.size(); ++k) chains.push_back({{paths[k].id}, paths[k].to, true, k});
    } else return {};
    // A chain on a lane that the next object does not leave stops there as a stub, when the route
    // starts on a Link: its vehicles change lanes to a chain that goes on (contract §2). On a
    // Connector, or at an ambiguous step, it is dropped as it always was.
    const auto stop = [&](std::vector<Chain>& carried, Chain& chain) {
        if (firstLink && !chain.onConnector) { chain.stub = true; carried.push_back(std::move(chain)); }
    };
    for (std::size_t i = 1; i < objectIds.size(); ++i) {
        std::vector<Chain> carried;
        for (auto& chain : chains) {
            if (chain.stub) { carried.push_back(std::move(chain)); continue; }
            if (const auto* connector = connectorById(network, objectIds[i])) {
                // A lane with no path onward contributes no full chain. The route itself is
                // untouched: narrowing a Connector changes how many lanes reach its end, nothing else.
                if (chain.onConnector) continue;
                bool found = false;
                for (const auto& path : pathsOf(*connector)) if (path.from.laneId == chain.at.laneId) {
                    auto ids = chain.ids; ids.push_back(path.id);
                    carried.push_back({std::move(ids), path.to, true, chain.lane});
                    found = true;
                    break; // One path may leave a given lane; the first is the one drawn.
                }
                if (!found) stop(carried, chain);
            } else if (const auto* link = linkById(network, objectIds[i])) {
                if (chain.onConnector) {
                    if (chain.at.linkId != link->id) continue;
                    auto ids = chain.ids; ids.push_back(chain.at.laneId);
                    carried.push_back({std::move(ids), chain.at, false, chain.lane});
                    continue;
                }
                // Two Links named one after the other: the Connector between them is implied,
                // and each lane finds its own. That is what lets a Link be split -- the split
                // makes one bridging Connector per lane, and no single one of them could stand
                // in a route. Ambiguity is refused rather than guessed: where two Connectors
                // serve the same lane pair, the author has to name the one they mean.
                std::optional<ConnectorPath> bridge;
                bool twoWays = false;
                for (const auto& connector : network.connectors)
                    for (const auto& path : pathsOf(connector))
                        if (path.from.laneId == chain.at.laneId && path.to.linkId == link->id) {
                            if (bridge && bridge->id != path.id) twoWays = true;
                            if (!bridge) bridge = path;
                        }
                if (twoWays && ambiguous) ambiguous->push_back(chain.at.laneId);
                if (twoWays) continue;
                if (!bridge) { stop(carried, chain); continue; }
                auto ids = chain.ids; ids.push_back(bridge->id); ids.push_back(bridge->to.laneId);
                carried.push_back({std::move(ids), bridge->to, false, chain.lane});
            }
        }
        chains = std::move(carried);
        if (std::all_of(chains.begin(), chains.end(), [](const auto& c) { return c.stub; })) return {};
    }
    // A stub is kept only when a run of neighbouring lanes, each with a chain, leads from it to a
    // full chain on the first Link. Every chain covers that Link from its start, so each step of the
    // run is a change a vehicle can make there. Anything else could only wait at its dead end.
    std::vector<int> kind(firstLink ? firstLink->lanes.size() : 0, 0); // 0 none, 1 stub, 2 full
    if (firstLink) for (const auto& chain : chains) kind[chain.lane] = chain.stub ? 1 : 2;
    const auto reaches = [&](std::size_t k) {
        for (const std::ptrdiff_t step : {-1, 1})
            for (auto j = static_cast<std::ptrdiff_t>(k) + step; j >= 0 && j < static_cast<std::ptrdiff_t>(kind.size()); j += step) {
                if (kind[static_cast<std::size_t>(j)] == 0) break;
                if (kind[static_cast<std::size_t>(j)] == 2) return true;
            }
        return false;
    };
    std::vector<FamilyChain> result;
    for (auto& chain : chains)
        if (!chain.stub || reaches(chain.lane)) result.push_back({std::move(chain.ids), chain.lane, chain.stub});
    return result;
}
std::vector<std::vector<std::string>> routeLaneChains(const Network& network,
                                                      const std::vector<std::string>& objectIds,
                                                      std::vector<std::string>* ambiguous) {
    std::vector<std::vector<std::string>> result;
    for (auto& chain : routeLaneFamily(network, objectIds, ambiguous))
        if (!chain.stub) result.push_back(std::move(chain.ids));
    return result;
}
std::vector<Point> objectGeometry(const Network& network, const std::string& objectId) {
    if (const auto* link = linkById(network, objectId))
        try { return linkCentreline(*link, network.drivingSide); } catch (const std::exception&) { return {}; }
    if (const auto* connector = connectorById(network, objectId))
        try { return connectorCentreline(network, *connector); } catch (const std::exception&) { return {}; }
    return {};
}
std::vector<std::vector<Point>> routeGeometries(const Network& network,
                                                const std::vector<std::string>& objectIds) {
    const auto table = runtimeSections(network);
    std::vector<std::vector<Point>> result;
    for (const auto& chain : routeLaneChains(network, objectIds)) {
        std::vector<Point> drawn;
        const auto append = [&](const std::vector<Point>& part) {
            for (const auto& point : part)
                if (drawn.empty() || std::hypot(point.x - drawn.back().x, point.y - drawn.back().y) > 1e-9)
                    drawn.push_back(point);
        };
        // The COMPILED chain, not the whole lane: a Connector arriving part way along a Link
        // means the vehicle never travels the stretch upstream of the arrival, and drawing that
        // stretch put a line on the road running against the traffic on it.
        for (const auto& id : expandRouteSegments(table, chain)) {
            for (const auto& section : table.sections) if (section.id == id) append(section.geometry);
            for (const auto& path : table.paths) if (path.id == id) append(path.geometry);
        }
        if (drawn.size() > 1) result.push_back(std::move(drawn));
    }
    return result;
}
}
