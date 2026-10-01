#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace trafficsim {
namespace {
// One lane section of a compiled route, with the route distance it starts at.
struct Stretch { const LaneSection* section{}; double at{}; };
std::vector<Stretch> stretches(const RuntimeSections& table, const std::map<std::string, const LaneSection*>& sections,
                               const std::vector<std::string>& segments) {
    std::vector<Stretch> result;
    double at = 0;
    for (const auto& id : segments) {
        if (const auto s = sections.find(id); s != sections.end()) {
            result.push_back({s->second, at});
            at += s->second->end - s->second->start;
            continue;
        }
        // A Connector path: the same length buildScenario gives its segment.
        for (const auto& path : table.paths) if (path.id == id) { at += polylineLength(path.geometry); break; }
    }
    return result;
}
double routeLength(const RuntimeSections& table, const std::map<std::string, const LaneSection*>& sections,
                   const std::vector<std::string>& segments) {
    double at = 0;
    for (const auto& id : segments) {
        if (const auto s = sections.find(id); s != sections.end()) { at += s->second->end - s->second->start; continue; }
        for (const auto& path : table.paths) if (path.id == id) { at += polylineLength(path.geometry); break; }
    }
    return at;
}
struct LaneAt { const Link* link{}; std::size_t index{}; std::vector<Point> geometry; };
}
std::size_t routeLaneShareCount(const Network& network, const std::vector<std::string>& objectIds) {
    if (!objectIds.empty())
        for (const auto& link : network.links) if (link.id == objectIds.front()) return link.lanes.size();
    return routeLaneFamily(network, objectIds).size();
}
void appendLaneChanges(const Network& network, const RuntimeSections& table, const std::vector<FamilyRoute>& family,
                       std::vector<LaneChangeSpan>& spans, std::vector<RouteDeadEnd>& deadEnds) {
    if (std::none_of(family.begin(), family.end(), [](const auto& r) { return r.stub; })) return;
    std::map<std::string, const LaneSection*> sections;
    for (const auto& section : table.sections) sections[section.id] = &section;
    std::map<std::string, LaneAt> lanes;
    const auto laneOf = [&](const std::string& laneId) -> const LaneAt& {
        if (const auto it = lanes.find(laneId); it != lanes.end()) return it->second;
        LaneAt found;
        for (const auto& link : network.links)
            for (std::size_t k = 0; k < link.lanes.size(); ++k)
                if (link.lanes[k].id == laneId) found = {&link, k, laneGeometry(link, laneId, network.drivingSide)};
        return lanes.emplace(laneId, std::move(found)).first->second;
    };
    std::vector<std::vector<Stretch>> routes;
    std::vector<double> from; // where each route reaches its family's decision Link (D93 rule 5)
    for (const auto& route : family) {
        routes.push_back(stretches(table, sections, route.segments));
        double at = 0;
        if (!route.after.empty())
            for (const auto& stretch : routes.back())
                if (stretch.section->linkId == route.after) { at = stretch.at; break; }
        from.push_back(at);
    }
    for (std::size_t s = 0; s < family.size(); ++s) {
        if (!family[s].stub) continue;
        const auto first = spans.size();
        for (std::size_t t = 0; t < family.size(); ++t) {
            if (t == s) continue;
            std::vector<LaneChangeSpan> pieces;
            for (const auto& a : routes[s])
                for (const auto& b : routes[t]) {
                    if (a.section->linkId != b.section->linkId) continue;
                    if (a.at < from[s] - 1e-9 || b.at < from[t] - 1e-9) continue; // before the decision
                    const auto& la = laneOf(a.section->laneId);
                    const auto& lb = laneOf(b.section->laneId);
                    if (!la.link || !lb.link || (la.index + 1 != lb.index && lb.index + 1 != la.index)) continue;
                    // Both lanes in the Link's reference stations, so a cross-section is square on a curve.
                    const auto& reference = la.link->geometry;
                    const double r0 = std::max(matchedStation(la.geometry, reference, a.section->start),
                                               matchedStation(lb.geometry, reference, b.section->start));
                    const double r1 = std::min(matchedStation(la.geometry, reference, a.section->end),
                                               matchedStation(lb.geometry, reference, b.section->end));
                    if (r1 - r0 <= 1e-6) continue;
                    pieces.push_back({family[s].id, family[t].id,
                                      a.at + matchedStation(reference, la.geometry, r0) - a.section->start,
                                      a.at + matchedStation(reference, la.geometry, r1) - a.section->start,
                                      b.at + matchedStation(reference, lb.geometry, r0) - b.section->start,
                                      b.at + matchedStation(reference, lb.geometry, r1) - b.section->start});
                }
            // Pieces that continue each other on both routes -- a section cut on either lane -- are
            // one span, so a vehicle straddling the cut is still wholly inside one.
            std::sort(pieces.begin(), pieces.end(), [](const auto& x, const auto& y) { return x.fromStart < y.fromStart; });
            for (const auto& piece : pieces) {
                if (spans.size() > first && spans.back().toRouteId == piece.toRouteId &&
                    std::abs(spans.back().fromEnd - piece.fromStart) < 1e-6 && std::abs(spans.back().toEnd - piece.toStart) < 1e-6) {
                    spans.back().fromEnd = piece.fromEnd; spans.back().toEnd = piece.toEnd;
                } else spans.push_back(piece);
            }
        }
        double deadEnd = -1;
        for (auto k = first; k < spans.size(); ++k) deadEnd = std::max(deadEnd, spans[k].fromEnd);
        deadEnds.push_back({family[s].id, deadEnd >= 0 ? deadEnd : routeLength(table, sections, family[s].segments)});
    }
}
}
