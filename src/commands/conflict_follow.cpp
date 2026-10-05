#include "conflict_follow.hpp"
#include "right_of_way_commands.hpp"
#include "../model/network/right_of_way.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <set>

// D86. See the header.
namespace trafficsim {
namespace {
struct Extents { double firstFrom, firstTo, secondFrom, secondTo; };
double shared(double a0, double a1, double b0, double b1) { return std::max(0.0, std::min(a1, b1) - std::max(a0, b0)); }
double mid(double a, double b) { return (a + b) / 2; }
std::string label(const ControlPathRef& r) { return r.linkId + "/" + r.laneId + "/" + r.connectorId + "/" + r.fromLaneId + ">" + r.toLaneId; }
// The piece of the pair's overlap this area was on: the most shared length with its old extents,
// then the nearest middle; pieces another area of the same pair already took are not offered.
std::optional<Extents> crossingNow(const Network& n, const ConflictArea& a, std::set<std::pair<std::string, std::size_t>>& taken,
                                   bool& gone) {
    const auto pieces = crossingOverlaps(n, a.first.path, a.second.path);
    if (pieces.front().status == SurfaceOverlap::Status::none) { gone = true; return std::nullopt; }
    if (pieces.front().status != SurfaceOverlap::Status::overlap) return std::nullopt; // the resolver says why
    const auto pair = std::min(label(a.first.path), label(a.second.path)) + "|" + std::max(label(a.first.path), label(a.second.path));
    std::optional<std::size_t> best;
    double bestShared = -1, bestDistance = INFINITY;
    for (std::size_t k = 0; k < pieces.size(); ++k) {
        if (taken.contains({pair, k})) continue;
        const auto& o = pieces[k];
        const double s = shared(a.first.entryStation, a.first.exitStation, o.first.from, o.first.to) +
                         shared(a.second.entryStation, a.second.exitStation, o.second.from, o.second.to);
        const double d = std::abs(mid(a.first.entryStation, a.first.exitStation) - mid(o.first.from, o.first.to)) +
                         std::abs(mid(a.second.entryStation, a.second.exitStation) - mid(o.second.from, o.second.to));
        if (s > bestShared || (s == bestShared && d < bestDistance)) { best = k; bestShared = s; bestDistance = d; }
    }
    if (!best) { gone = true; return std::nullopt; } // every piece is another area's already
    taken.insert({pair, *best});
    const auto& o = pieces[*best];
    return Extents{o.first.from, o.first.to, o.second.from, o.second.to};
}
// A taken-over merge: the same two paths arriving on one section, the join nearest the old one.
std::optional<Extents> mergeNow(const Network& n, const RuntimeSections& table, const ConflictArea& a, bool& gone) {
    std::optional<Extents> best;
    double bestDistance = INFINITY;
    for (const auto& g : mergeGroups(n, table)) {
        std::vector<MergeSide> sides;
        try { for (const auto& s : g.incoming) sides.push_back(mergeSide(n, table, s)); } catch (const std::exception&) { continue; }
        for (const auto& f : sides)
            for (const auto& s : sides) {
                if (&f == &s || !(f.path == a.first.path) || !(s.path == a.second.path)) continue;
                const double d = std::abs(f.exit - a.first.exitStation) + std::abs(s.exit - a.second.exitStation);
                if (d < bestDistance) { bestDistance = d; best = Extents{f.entry, f.exit, s.entry, s.exit}; }
            }
    }
    gone = !best;
    return best;
}
}
void followGeometry(ProjectDocument& d, const Network& before) {
    auto& n = d.network;
    auto& row = n.rightOfWay;
    if (row.conflictAreas.empty()) return;
    if (n.links == before.links && n.connectors == before.connectors && n.drivingSide == before.drivingSide) return;
    const auto was = row.conflictAreas;
    std::optional<RuntimeSections> table;
    std::set<std::pair<std::string, std::size_t>> taken;
    std::vector<std::string> gone;
    for (auto& a : row.conflictAreas) {
        bool lost = false;
        std::optional<Extents> now;
        if (a.kind == ConflictKind::crossing) now = crossingNow(n, a, taken, lost);
        else {
            if (!table) table = runtimeSections(n);
            now = mergeNow(n, *table, a, lost);
        }
        if (lost) { gone.push_back(a.id); continue; }
        if (!now) continue;
        a.first.entryStation = now->firstFrom; a.first.exitStation = now->firstTo;
        a.second.entryStation = now->secondFrom; a.second.exitStation = now->secondTo;
    }
    // A line stands before the first area it serves on its path (D63): it moves as that entry does.
    const auto firstEntry = [&](const std::vector<ConflictArea>& areas, const WaitingLine& w) {
        double entry = INFINITY;
        for (const auto& a : areas)
            if (std::find(gone.begin(), gone.end(), a.id) == gone.end())
                for (const auto* s : {&a.first, &a.second})
                    if (s->waitingLineId == w.id && s->path == w.point.path) entry = std::min(entry, s->entryStation);
        return entry;
    };
    for (auto& w : row.waitingLines) {
        const double from = firstEntry(was, w), to = firstEntry(row.conflictAreas, w);
        if (std::isfinite(from) && std::isfinite(to)) {
            try {
                const double gap=controlStationDistance(before,w.point.path,w.point.station,from);
                const double station=std::min(to,offsetControlStation(n,w.point.path,to,-gap));
                // Avoid introducing round-off-only edits (for example renaming a Link).
                if(w.point.station>to || std::abs(station-w.point.station)>1e-9)w.point.station=station;
            } catch(const std::exception&) {} // unresolved paths remain named resolver issues
        }
    }
    for (const auto& id : gone) removeConflictArea(d, id);
}
}
