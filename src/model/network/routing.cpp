#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <deque>

namespace trafficsim {
namespace {
// Everything an author may name in a route: whole lanes, plus the Connector paths between them.
// A section id is deliberately absent -- it is derived data and a route is persisted.
std::vector<Segment> routeSegments(const RuntimeSections& table) {
    auto segments = authoringSegments(table);
    for (std::size_t p = 0; p < table.paths.size(); ++p)
        segments.push_back({table.paths[p].id, polylineLength(table.paths[p].geometry), {table.pathNext[p]}});
    // A path's successor is a SECTION; an author picks the lane it belongs to. The route dialog
    // did this rewrite inline; it lives here now so the canvas and the dialog cannot disagree.
    for (auto& segment : segments) for (auto& next : segment.next)
        for (const auto& section : table.sections) if (section.id == next) next = section.laneId;
    return segments;
}
// The rule itself, over an already-built segment list: what may follow `authored`.
std::vector<std::string> continuations(const std::vector<Segment>& segments,
                                       const std::vector<std::string>& authored) {
    std::vector<std::string> result;
    for (const auto& segment : segments) {
        // An empty route may start anywhere; otherwise only where the tail leads.
        bool allowed = authored.empty();
        if (!allowed) for (const auto& last : segments) if (last.id == authored.back())
            allowed = std::find(last.next.begin(), last.next.end(), segment.id) != last.next.end();
        // A route that revisited a segment would loop. The dialog refused that and the pointer
        // gesture refuses the same thing, or the two disagree about one network.
        if (allowed && std::find(authored.begin(), authored.end(), segment.id) == authored.end())
            result.push_back(segment.id);
    }
    return result;
}
}
std::vector<std::string> routeContinuations(const RuntimeSections& table,
                                            const std::vector<std::string>& authored) {
    return continuations(routeSegments(table), authored);
}
std::vector<std::string> routeContinuations(const Network& network,
                                            const std::vector<std::string>& authored) {
    return routeContinuations(runtimeSections(network), authored);
}
std::vector<std::string> routeChainTo(const RuntimeSections& table,
                                      const std::vector<std::string>& authored,
                                      const std::string& target) {
    if (target.empty()) return {};
    const auto segments = routeSegments(table);
    // Breadth first over the same continuation rule, so the shortest chain wins and a longer
    // detour never hides it. Bounded: one click must not walk a large network forever.
    constexpr std::size_t kMaxChain = 20;
    std::deque<std::vector<std::string>> level{{}};
    for (std::size_t depth = 0; depth < kMaxChain && !level.empty(); ++depth) {
        std::deque<std::vector<std::string>> next;
        std::vector<std::string> found;
        for (const auto& chain : level) {
            auto path = authored; path.insert(path.end(), chain.begin(), chain.end());
            for (const auto& candidate : continuations(segments, path)) {
                auto extended = chain; extended.push_back(candidate);
                if (candidate != target) { next.push_back(std::move(extended)); continue; }
                // Two ways to reach the target at the same depth is an ambiguity only the author
                // can settle, by clicking an intermediate segment. Guessing one of them would
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
std::vector<std::string> routeChainTo(const Network& network,
                                      const std::vector<std::string>& authored,
                                      const std::string& target) {
    return routeChainTo(runtimeSections(network), authored, target);
}
std::vector<Point> routeGeometry(const Network& network,
                                 const std::vector<std::string>& segmentIds) {
    std::vector<Point> result;
    const auto append = [&](const std::vector<Point>& part) {
        for (const auto& point : part)
            if (result.empty() || std::hypot(point.x - result.back().x, point.y - result.back().y) > 1e-9)
                result.push_back(point);
    };
    for (const auto& id : segmentIds) {
        bool drawn = false;
        for (const auto& link : network.links) for (const auto& lane : link.lanes)
            if (lane.id == id) { append(laneGeometry(link, lane.id, network.drivingSide)); drawn = true; }
        if (drawn) continue;
        // Not a lane: a Connector path. Derived per Connector rather than through the section
        // table, so a draft route still draws on a network the section table refuses.
        for (const auto& connector : network.connectors) {
            std::vector<ConnectorPath> paths;
            try { paths = connectorPaths(network, connector); } catch (const std::exception&) { continue; }
            for (const auto& path : paths) if (path.id == id) append(path.geometry);
        }
    }
    return result;
}
}
