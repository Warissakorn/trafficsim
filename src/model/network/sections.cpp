#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

namespace trafficsim {
namespace {
// A path leaving a lane body, with the station on that lane it leaves at.
struct Departure { double station{}; std::size_t path{}; };
// The boundary a departure hangs off: the smallest section end at or after its station. For a
// path attached at the lane end that is the last section, and for one whose cut was rejected it
// is the section the station falls inside -- Run refuses that Connector either way, and the
// graph still has to be well formed for the diagnostics that read it.
std::size_t sectionOf(const std::vector<double>& boundaries, double station) {
    for (std::size_t i = 1; i < boundaries.size(); ++i)
        if (station <= boundaries[i] + 1e-9) return i - 1;
    return boundaries.size() - 2;
}
}
std::string sectionId(const std::string& laneId, int index) {
    return index == 0 ? laneId : laneId + "/sec-" + std::to_string(index + 1);
}
RuntimeSections runtimeSections(const Network& network) {
    RuntimeSections table;
    // Which connector each path came from, so a rejected cut can be reported by object.
    std::vector<std::size_t> owner;
    for (std::size_t c = 0; c < network.connectors.size(); ++c) {
        std::vector<ConnectorPath> paths;
        // connectorPaths throws on a range that no longer fits its link. A caller past the draft
        // validation never sees that, but diagnostics must not throw, so skip rather than fail.
        try { paths = connectorPaths(network, network.connectors[c]); } catch (const std::exception&) { continue; }
        for (auto& path : paths) { table.paths.push_back(std::move(path)); owner.push_back(c); }
    }
    for (const auto& link : network.links) for (const auto& lane : link.lanes) {
        // Verbatim the expression buildScenario used for a whole lane, so an uncut lane's length
        // is the same double it always was, not merely the same value to within a tolerance.
        const auto geometry = laneGeometry(link, lane.id, network.drivingSide);
        const double full = polylineLength(geometry);
        std::vector<Departure> departures;
        std::vector<Departure> cuts;
        for (std::size_t p = 0; p < table.paths.size(); ++p) {
            const auto& from = table.paths[p].from;
            if (from.laneId != lane.id) continue;
            if (attachedAtLinkEnd(network, from, true)) { departures.push_back({full, p}); continue; }
            // The station is metres along the link's REFERENCE polyline; matchedStation names the
            // same cross-section on this lane, which is the coordinate a section lives in.
            const double station = matchedStation(link.geometry, geometry,
                                                  attachmentStation(network, from, true));
            departures.push_back({station, p});
            cuts.push_back({station, p});
        }
        // A path ARRIVING inside this lane's body cuts it too, so the vehicle it delivers joins
        // at the drawn metre rather than at the start of the lane. It is not a departure: nothing
        // leaves the lane here, and the merge it creates is arbitrated by a priority rule.
        for (std::size_t p = 0; p < table.paths.size(); ++p) {
            const auto& to = table.paths[p].to;
            if (to.laneId != lane.id || attachedAtLinkEnd(network, to, false)) continue;
            cuts.push_back({matchedStation(link.geometry, geometry,
                                           attachmentStation(network, to, false)), p});
        }
        // Stable, so two attachments at the same station keep the order their connectors were
        // authored in. No unordered container touches this: replay depends on it.
        std::stable_sort(cuts.begin(), cuts.end(),
                         [](const auto& a, const auto& b) { return a.station < b.station; });
        std::vector<double> boundaries{0};
        for (const auto& cut : cuts) {
            if (cut.station < boundaries.back() + kMinSectionLength || cut.station > full - kMinSectionLength) {
                const auto& id = network.connectors[owner[cut.path]].id;
                if (std::find(table.unsectionable.begin(), table.unsectionable.end(), id) ==
                    table.unsectionable.end()) table.unsectionable.push_back(id);
                continue;
            }
            boundaries.push_back(cut.station);
        }
        boundaries.push_back(full);
        const auto first = table.sections.size();
        for (std::size_t i = 0; i + 1 < boundaries.size(); ++i) {
            const double start = boundaries[i], end = boundaries[i + 1];
            LaneSection section{sectionId(lane.id, static_cast<int>(i)), link.id, lane.id, start, end,
                                boundaries.size() == 2 ? geometry : polylineSpan(geometry, start, end), {}};
            if (i + 2 < boundaries.size()) section.next.push_back(sectionId(lane.id, static_cast<int>(i) + 1));
            table.sections.push_back(std::move(section));
        }
        // Departures in the order the paths were derived, which for an uncut lane reproduces the
        // exact `next` vector the whole-lane compiler built.
        for (const auto& departure : departures)
            table.sections[first + sectionOf(boundaries, departure.station)].next
                .push_back(table.paths[departure.path].id);
    }
    // Resolved after the sections exist, because a path arrives ON one of them.
    table.pathNext.reserve(table.paths.size());
    for (const auto& path : table.paths) {
        double station = 0;
        for (const auto& link : network.links) if (link.id == path.to.linkId) {
            const auto geometry = laneGeometry(link, path.to.laneId, network.drivingSide);
            station = matchedStation(link.geometry, geometry, attachmentStation(network, path.to, false));
        }
        // The section that STARTS here, not the one that ends here. An arriving vehicle continues
        // downstream of the cut; sectionForStation resolves upstream, which is right for a signal
        // head standing on the cut and wrong for a vehicle joining at it.
        table.pathNext.push_back(sectionStartingAt(table, path.to.laneId, station).id);
    }
    return table;
}
const LaneSection& sectionForStation(const RuntimeSections& table, const std::string& laneId,
                                     double laneStation) {
    const LaneSection* last = nullptr;
    for (const auto& section : table.sections) {
        if (section.laneId != laneId) continue;
        // `end` inclusive, so a station sitting exactly on a cut resolves upstream of it.
        if (laneStation <= section.end + 1e-9) return section;
        last = &section;
    }
    if (last) return *last;
    throw std::invalid_argument("UNKNOWN_LANE");
}
const LaneSection& sectionStartingAt(const RuntimeSections& table, const std::string& laneId,
                                     double laneStation) {
    const LaneSection* last = nullptr;
    for (const auto& section : table.sections) {
        if (section.laneId != laneId) continue;
        if (section.start >= laneStation - 1e-9) return section;
        last = &section;
    }
    if (last) return *last;
    throw std::invalid_argument("UNKNOWN_LANE");
}
namespace {
// The section of `laneId` that `fromId` leads to, or empty when it leads nowhere on that lane.
std::string successorOnLane(const RuntimeSections& table, const std::string& fromId,
                            const std::string& laneId) {
    const auto onLane = [&](const std::string& id) {
        return std::any_of(table.sections.begin(), table.sections.end(),
                           [&](const auto& s) { return s.id == id && s.laneId == laneId; });
    };
    for (std::size_t p = 0; p < table.paths.size(); ++p)
        if (table.paths[p].id == fromId && onLane(table.pathNext[p])) return table.pathNext[p];
    for (const auto& section : table.sections)
        if (section.id == fromId)
            for (const auto& next : section.next) if (onLane(next)) return next;
    return {};
}
}
std::vector<std::string> expandRouteSegments(const RuntimeSections& table,
                                             const std::vector<std::string>& authored) {
    std::vector<std::string> result;
    for (std::size_t i = 0; i < authored.size(); ++i) {
        const auto start = std::find_if(table.sections.begin(), table.sections.end(),
                                        [&](const auto& s) { return s.id == authored[i]; });
        // Not a lane's first section: a connector path, or an id the core will report as unknown.
        if (start == table.sections.end() || start->id != start->laneId) {
            result.push_back(authored[i]);
            continue;
        }
        // A route ARRIVING part way along a lane joins at that section, not at the lane's start:
        // the vehicle never travels the stretch upstream of where it came in. Ask the segment
        // just emitted where it leads on this lane, and begin there.
        auto from = start;
        if (!result.empty()) {
            const auto joined = successorOnLane(table, result.back(), start->laneId);
            if (!joined.empty())
                from = std::find_if(table.sections.begin(), table.sections.end(),
                                    [&](const auto& s) { return s.id == joined; });
        }
        for (auto it = from; it != table.sections.end() && it->laneId == start->laneId; ++it) {
            result.push_back(it->id);
            if (i + 1 == authored.size()) continue; // The route ends on this lane: travel all of it.
            // Stop where the route leaves. Running off the end instead leaves the last section's
            // `next` without the following authored id, which is DISCONNECTED_ROUTE -- reported by
            // the core guard that already owns that question, rather than by a second check here.
            if (std::find(it->next.begin(), it->next.end(), authored[i + 1]) != it->next.end()) break;
        }
    }
    return result;
}
std::vector<Segment> authoringSegments(const RuntimeSections& table) {
    std::vector<Segment> result;
    for (const auto& section : table.sections) {
        if (section.id == section.laneId) result.push_back({section.laneId, 0, {}});
        auto& segment = result.back();
        segment.length += section.end - section.start;
        // The union across the lane's sections, minus the lane's own sections: an interior
        // diverge has to be selectable, and a section id must never be.
        for (const auto& next : section.next) {
            const auto owned = std::find_if(table.sections.begin(), table.sections.end(),
                [&](const auto& s) { return s.id == next && s.laneId == section.laneId; });
            if (owned != table.sections.end()) continue;
            if (std::find(segment.next.begin(), segment.next.end(), next) == segment.next.end())
                segment.next.push_back(next);
        }
    }
    return result;
}
std::vector<PriorityRule> derivedPriorityRules(const RuntimeSections& table,
                                               const PriorityDefaults& defaults) {
    std::vector<PriorityRule> rules;
    for (std::size_t p = 0; p < table.paths.size(); ++p) {
        // An arriving path whose successor section does not start at 0 arrived inside the body,
        // so it and the section upstream of the arrival both feed that section: a merge.
        const auto joined = std::find_if(table.sections.begin(), table.sections.end(),
            [&](const auto& s) { return s.id == table.pathNext[p]; });
        if (joined == table.sections.end() || joined->start <= 0) continue;
        const auto upstream = std::find_if(table.sections.begin(), table.sections.end(),
            [&](const auto& s) { return s.laneId == joined->laneId && s.end == joined->start; });
        if (upstream == table.sections.end()) continue;
        // Deliberately does NOT throw when the defaults are unset: buildScenario is documented as
        // unchecked assembly for diagnostics that must not throw, and a missing data catalog must
        // not block an EDIT (D18b). The zero gap time that would result -- a merge nobody gives
        // way at -- is refused on the Run path instead, by priorityDefaultsIssues.
        // The arriving Connector gives way to the traffic already on the lane. Its stop line is
        // at its own downstream end; the conflict point is where the upstream section ends, which
        // is the drawn station.
        rules.push_back({"give-way/" + table.paths[p].id,
                         table.paths[p].id, polylineLength(table.paths[p].geometry),
                         upstream->id, upstream->end - upstream->start,
                         defaults.gapTime, defaults.headway});
    }
    return rules;
}
SignalHead rebaseHead(const RuntimeSections& table, const NetworkSignalHead& head) {
    // A connector-mounted head names a path, which is never sectioned, so it passes through.
    if (!head.connectorId.empty()) return {head.id, head.connectorId, head.position, head.programId};
    const auto& section = sectionForStation(table, head.lane.laneId, head.position);
    // Both are metres along the same lane polyline, so this is a subtraction, not a conversion.
    return {head.id, section.id, head.position - section.start, head.programId};
}
}
