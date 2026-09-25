#include "evaluation.hpp"
#include "json.hpp"
#include "../model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
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
    // sections belong to, first and last, grouped and named exactly as an authored one.
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
        if (route.segmentIds.empty()) continue;
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
    std::set<std::string> measured; // heads an authored counter measures: their Link's derived row goes
    std::vector<QueueCounter> authored;
    for (const auto& c : network.queueCounters) {
        QueueCounter counter{c.name.empty() ? c.id : c.name, {}};
        for (const auto& l : c.lines) {
            std::optional<CounterLine> line;
            if (l.point) {
                if (const auto at = locateControlPoint(network, table, *l.point)) line = CounterLine{at->segment, at->position};
            } else if ((line = headLine(l.referenceId))) {
                measured.insert(l.referenceId);
            } else {
                for (const auto& w : network.rightOfWay.waitingLines)
                    if (w.id == l.referenceId)
                        if (const auto at = locateControlPoint(network, table, w.point)) line = CounterLine{at->segment, at->position};
            }
            if (line) counter.lines.push_back(*line);
        }
        // A line that no longer resolves measures nothing; a counter with none is not a row.
        if (!counter.lines.empty()) authored.push_back(std::move(counter));
    }
    for (const auto& link : network.links) {
        QueueCounter counter{link.name.empty() ? link.id : link.name, {}};
        bool replaced = false;
        for (const auto& head : network.signalHeads)
            if (head.connectorId.empty() && head.lane.linkId == link.id) {
                replaced = replaced || measured.contains(head.id);
                if (const auto line = headLine(head.id)) counter.lines.push_back(*line);
            }
        // One row per approach (A23): an authored counter over any of these heads replaces it.
        if (!counter.lines.empty() && !replaced) spec.counters.push_back(std::move(counter));
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
        << "\nactive," << r.active << "\nsafetyClamps," << r.safetyClamps << '\n';
    return out.str();
}
}
