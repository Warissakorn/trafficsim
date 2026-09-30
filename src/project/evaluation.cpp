#include "evaluation.hpp"
#include "json.hpp"
#include "../model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <optional>
#include <set>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace trafficsim {
QueueDefinition loadQueueDefinition(const std::filesystem::path& dataDirectory) {
    try {
        std::ifstream stream(dataDirectory / "evaluation" / "queue-counter.json");
        if (!stream) throw std::runtime_error("missing");
        const auto j = Json::parse(stream);
        QueueDefinition q{j.at("beginSpeed").get<double>() / 3.6, j.at("endSpeed").get<double>() / 3.6,
                          j.at("maxHeadway").get<double>()};
        for (const double v : {q.beginSpeed, q.endSpeed, q.maxGap})
            if (!(std::isfinite(v) && v >= 0)) throw std::runtime_error("range");
        if (q.endSpeed < q.beginSpeed) throw std::runtime_error("order");
        return q;
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
}
namespace {
std::string linkLabel(const Network& network, const std::string& id) {
    for (const auto& link : network.links)
        if (link.id == id) return link.name.empty() ? id : link.name;
    return id;
}
}
std::string queueRowName(const AuthoredQueueCounter& c) { return c.name.empty() ? c.id : c.name; }
std::vector<std::string> replacedApproaches(const Network& network, const AuthoredQueueCounter& c) {
    std::vector<std::string> links;
    for (const auto& link : network.links) {
        const bool measured = std::any_of(c.lines.begin(), c.lines.end(), [&](const auto& l) {
            return !l.point && std::any_of(network.signalHeads.begin(), network.signalHeads.end(), [&](const auto& h) {
                return h.id == l.referenceId && h.connectorId.empty() && h.lane.linkId == link.id; });
        });
        if (measured) links.push_back(link.id);
    }
    return links;
}
EvaluationSpec evaluationSpec(const ProjectDocument& document, const RunSnapshot& snapshot,
                              const std::filesystem::path& dataDirectory) {
    EvaluationSpec spec;
    spec.queue = loadQueueDefinition(dataDirectory);
    std::map<std::string, std::size_t> movementOfAuthored;
    if (document.definition) {
        std::map<std::pair<std::string, std::string>, std::size_t> movementOfPair;
        for (const auto& route : document.definition->routes) {
            if (route.segmentIds.empty()) continue;
            const std::pair key{route.segmentIds.front(), route.segmentIds.back()};
            auto [it, added] = movementOfPair.try_emplace(key, spec.movementNames.size());
            if (added)
                spec.movementNames.push_back(linkLabel(document.network, key.first) + " → " +
                                             linkLabel(document.network, key.second));
            movementOfAuthored[route.id] = it->second;
        }
    }
    // A route no author drew -- a routeless path (M2.1.1) -- is a movement by the Links its lane
    // sections belong to, first and last, grouped and named exactly as an authored one. A stub
    // (M3.2.8b) is never one: no vehicle arrives on it, so it would only be an empty row.
    const auto stub = [&](const std::string& id) {
        return std::any_of(snapshot.scenario.routeDeadEnds.begin(), snapshot.scenario.routeDeadEnds.end(),
                           [&](const auto& d) { return d.routeId == id; });
    };
    const auto table = runtimeSections(snapshot.network);
    const auto linkOf = [&](const std::string& segment) -> std::string {
        for (const auto& section : table.sections) if (section.id == segment) return section.linkId;
        return {};
    };
    std::map<std::pair<std::string, std::string>, std::size_t> movementOfLinks;
    if (document.definition)
        for (const auto& route : document.definition->routes)
            if (!route.segmentIds.empty())
                movementOfLinks.try_emplace({route.segmentIds.front(), route.segmentIds.back()}, movementOfAuthored[route.id]);
    for (const auto& route : snapshot.scenario.routes) {
        if (const auto it = movementOfAuthored.find(route.id.substr(0, route.id.find('/')));
            it != movementOfAuthored.end()) { spec.movementOfRoute[route.id] = it->second; continue; }
        if (route.segmentIds.empty() || stub(route.id)) continue;
        const std::pair key{linkOf(route.segmentIds.front()), linkOf(route.segmentIds.back())};
        if (key.first.empty() || key.second.empty()) continue;
        auto [it, added] = movementOfLinks.try_emplace(key, spec.movementNames.size());
        if (added)
            spec.movementNames.push_back(linkLabel(document.network, key.first) + " \u2192 " +
                                         linkLabel(document.network, key.second));
        spec.movementOfRoute[route.id] = it->second;
    }
    // Counter lines are places on runtime segments (M3.2.6b). A head's is where the core stops
    // traffic for it, read from the compiled scenario, so a head-derived counter measures exactly
    // the line it always did.
    const auto& network = snapshot.network;
    const auto headLine = [&](const std::string& id) -> std::optional<CounterLine> {
        for (const auto& h : snapshot.scenario.signalHeads) if (h.id == id) return CounterLine{h.segmentId, h.position};
        return std::nullopt;
    };
    std::set<std::string> replaced; // Links an authored counter measures heads on: their derived row goes
    std::vector<QueueCounter> authored;
    for (const auto& c : network.queueCounters) {
        QueueCounter counter{queueRowName(c), {}};
        for (const auto& l : c.lines) {
            std::optional<CounterLine> line;
            if (l.point) {
                if (const auto at = locateControlPoint(network, table, *l.point)) line = CounterLine{at->segment, at->position};
            } else if (!(line = headLine(l.referenceId))) {
                for (const auto& w : network.rightOfWay.waitingLines)
                    if (w.id == l.referenceId)
                        if (const auto at = locateControlPoint(network, table, w.point)) line = CounterLine{at->segment, at->position};
            }
            if (line) counter.lines.push_back(*line);
        }
        // A line that no longer resolves measures nothing; a counter with none is not a row.
        if (counter.lines.empty()) continue;
        for (auto& link : replacedApproaches(network, c)) replaced.insert(std::move(link));
        authored.push_back(std::move(counter));
    }
    for (const auto& link : network.links) {
        // One row per approach (A23): an authored counter over any of its heads replaces it.
        if (replaced.contains(link.id)) continue;
        QueueCounter counter{link.name.empty() ? link.id : link.name, {}};
        for (const auto& head : network.signalHeads)
            if (head.connectorId.empty() && head.lane.linkId == link.id)
                if (const auto line = headLine(head.id)) counter.lines.push_back(*line);
        if (!counter.lines.empty()) spec.counters.push_back(std::move(counter));
    }
    for (auto& c : authored) spec.counters.push_back(std::move(c));
    return spec;
}
namespace {
std::string quoted(const std::string& text) {
    std::string out = "\"";
    for (const char c : text) { if (c == '"') out += '"'; out += c; }
    return out + '"';
}
std::string number(const std::optional<double>& value) {
    if (!value) return "";
    std::ostringstream s; s << std::fixed << std::setprecision(2) << *value; return s.str();
}
}
Json movementJson(const MovementReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = "simulated movement delay, not HCM control delay; one run";
    j["movements"] = Json::array();
    for (const auto& m : r.movements)
        j["movements"].push_back({{"movement", m.name}, {"vehicles", m.vehicles},
                                  {"meanDelay", m.meanDelay ? Json(*m.meanDelay) : Json(nullptr)},
                                  {"meanTravelTime", m.meanTravelTime ? Json(*m.meanTravelTime) : Json(nullptr)}});
    j["queues"] = Json::array();
    for (const auto& q : r.queues)
        j["queues"].push_back({{"approach", q.name}, {"meanLength", q.meanLength}, {"maxLength", q.maxLength}});
    j["completed"] = r.completed; j["notInMovement"] = r.unassigned; j["meanDelay"] = r.meanDelay ? Json(*r.meanDelay) : Json(nullptr);
    j["pending"] = r.pending; j["active"] = r.active; j["safetyClamps"] = r.safetyClamps; j["time"] = r.time;
    j["laneChanges"] = r.laneChanges;
    return j;
}
Json laneChangeJson(const LaneChangeReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = "mandatory lane changes and dead-end waits, by the movement a vehicle arrived on; one run";
    j["rows"] = Json::array();
    for (const auto& row : r.rows) {
        // Nearest-rank quantiles, and the share of changes at most 20 m from the reference.
        const auto spread = [](std::vector<double> values) {
            if (values.empty()) return Json(nullptr);
            std::sort(values.begin(), values.end());
            const auto rank = [&](double q) {
                const auto k = static_cast<std::size_t>(std::ceil(q * static_cast<double>(values.size())));
                return values[std::clamp<std::size_t>(k, 1, values.size()) - 1];
            };
            const auto near = std::count_if(values.begin(), values.end(), [](double d) { return d <= 20; });
            return Json{{"min", rank(0)}, {"p10", rank(0.1)}, {"median", rank(0.5)}, {"p90", rank(0.9)},
                        {"max", rank(1)}, {"within20m", static_cast<double>(near) / static_cast<double>(values.size())}};
        };
        j["rows"].push_back({{"movement", row.name}, {"changes", row.changes}, {"changedVehicles", row.changedVehicles},
                             {"positions", row.beforeDeadEnd.size()}, {"unplaced", row.unplaced},
                             {"beforeDeadEnd", spread(row.beforeDeadEnd)}, {"fromNetworkEdge", spread(row.atDistance)},
                             {"waitingVehicles", row.waitingVehicles}, {"waitSeconds", row.waitSeconds},
                             {"longestWait", row.longestWait}});
    }
    return j;
}
Json segmentTimeJson(const SegmentTimeReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = "mean time from departure to entering each runtime segment, by the movement a vehicle arrived on; one run";
    j["unassigned"] = r.unassigned; j["undeparted"] = r.undeparted;
    j["rows"] = Json::array();
    for (const auto& row : r.rows) {
        Json segments = Json::array();
        for (const auto& s : row.segments)
            segments.push_back({{"segmentId", s.segmentId}, {"vehicles", s.vehicles}, {"meanSinceDeparture", s.meanSinceDeparture}});
        j["rows"].push_back({{"movement", row.name}, {"vehicles", row.vehicles},
                             {"meanDepartureDelay", row.meanDepartureDelay}, {"meanTravelTime", row.meanTravelTime},
                             {"meanFreeFlowTime", row.meanFreeFlowTime}, {"segments", std::move(segments)}});
    }
    return j;
}
Json stopLineJson(const StopLineReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = "stop-line discharge per signal head: standing upstream in red and green, greens held through; one run";
    j["rows"] = Json::array();
    for (const auto& h : r.rows)
        j["rows"].push_back({{"head", h.headId}, {"crossed", h.crossed}, {"greens", h.greens},
                             {"meanStandRed", h.meanStandRed}, {"meanStandGreen", h.meanStandGreen},
                             {"heldShare", h.heldShare}, {"meanHeld", h.meanHeld},
                             {"meanDischarged", h.meanDischarged}, {"meanHeadway", h.meanHeadway},
                             {"meanResidual", h.meanResidual}});
    return j;
}
std::string movementCsv(const MovementReport& r) {
    std::ostringstream out;
    out << "# TrafficSim - not yet validated. Simulated movement delay, not HCM control delay; one run.\n";
    out << "movement,vehicles,meanDelay_s,meanTravelTime_s\n";
    for (const auto& m : r.movements)
        out << quoted(m.name) << ',' << m.vehicles << ',' << number(m.meanDelay) << ',' << number(m.meanTravelTime) << '\n';
    out << "\napproach,meanQueue_m,maxQueue_m\n";
    for (const auto& q : r.queues)
        out << quoted(q.name) << ',' << number(q.meanLength) << ',' << number(q.maxLength) << '\n';
    out << "\ncompleted," << r.completed << "\nnotInMovement," << r.unassigned << "\npending," << r.pending
        << "\nactive," << r.active << "\nsafetyClamps," << r.safetyClamps << "\nlaneChanges," << r.laneChanges << '\n';
    return out.str();
}
}
