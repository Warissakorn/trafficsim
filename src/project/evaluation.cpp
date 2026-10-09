#include "evaluation.hpp"
#include "csv_format.hpp"
#include "input_manifest.hpp"
#include "los_output.hpp"
#include "json.hpp"
#include "../model/network/right_of_way.hpp"
#include "../model/network/travel_time.hpp"
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
QueueDefinition loadQueueDefinition(const std::filesystem::path& dataDirectory,InputManifest* manifest) {
    try {
        const auto j=readInputJson(dataDirectory / "evaluation" / "queue-counter.json",
                                   "evaluation/queue-counter.json",manifest);
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
                              const std::filesystem::path& dataDirectory,InputManifest* manifest) {
    EvaluationSpec spec;
    spec.queue = loadQueueDefinition(dataDirectory,manifest);
    if (document.definition && document.definition->evaluation) {
        spec.warmup = document.definition->evaluation->warmup;
        spec.end = document.definition->evaluation->end; // none: the end of the run, unbounded
        // M5.9 (D146): the run goes on past the duration, so the window's end is the authored one.
        if (const auto cooldown = document.definition->evaluation->cooldown; cooldown > 0) {
            spec.cooldown = cooldown;
            if (!spec.end) spec.end = document.definition->duration;
        }
    }
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
    // M5.4 (D133): every authored section is a row, even one whose lines no longer resolve.
    for (const auto& section : network.travelTimeSections) {
        SectionSpec s{section.name.empty() ? section.id : section.name, {}, {}, {}, linkLabel(document.network, section.start.linkId)};
        if (section.controlType) s.controlType = sectionControlName(*section.controlType); // M5.5
        for (const auto& at : locateSectionLine(network, table, section.start)) s.start.push_back({at.segment, at.position});
        for (const auto& at : locateSectionLine(network, table, section.end)) s.end.push_back({at.segment, at.position});
        spec.sections.push_back(std::move(s));
    }
    // M5.5 (D134): the pack is read only when a letter can be given, so other projects' input
    // manifests are unchanged.
    if (std::any_of(spec.sections.begin(), spec.sections.end(), [](const auto& s) { return s.controlType.has_value(); }))
        spec.los = loadLosPack(dataDirectory, manifest);
    return spec;
}
std::vector<LosInput> losInputs(const MovementReport& r) {
    std::vector<LosInput> inputs;
    for (const auto& s : r.sections) inputs.push_back({s.approach, s.controlType, static_cast<double>(s.vehicles), s.meanDelay});
    return inputs;
}
Json movementJson(const MovementReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = "simulated movement delay, not HCM control delay; one run";
    j["movements"] = Json::array();
    for (const auto& m : r.movements)
        j["movements"].push_back({{"movement", m.name}, {"vehicles", m.vehicles},
                                  {"meanDelay", m.meanDelay ? Json(*m.meanDelay) : Json(nullptr)},
                                  {"meanTravelTime", m.meanTravelTime ? Json(*m.meanTravelTime) : Json(nullptr)},
                                  {"unfinished", m.unfinished}});
    j["queues"] = Json::array();
    for (const auto& q : r.queues)
        j["queues"].push_back({{"approach", q.name}, {"meanLength", q.meanLength}, {"maxLength", q.maxLength}});
    j["completed"] = r.completed; j["notInMovement"] = r.unassigned; j["meanDelay"] = r.meanDelay ? Json(*r.meanDelay) : Json(nullptr);
    j["pending"] = r.pending; j["active"] = r.active; j["safetyClamps"] = r.safetyClamps; j["time"] = r.time;
    j["laneChanges"] = r.laneChanges;
    j["evaluationPeriod"] = {{"warmup", r.warmup}, {"end", r.evaluationEnd}}; // M5.3
    if (r.cooldown) j["evaluationPeriod"]["cooldown"] = *r.cooldown; // M5.9: only with one
    if (!r.sections.empty()) { // M5.4: only with a section, so other projects keep their bytes
        j["sections"] = Json::array();
        for (const auto& row : r.sections)
            j["sections"].push_back({{"section", row.name}, {"vehicles", row.vehicles},
                                     {"meanTravelTime", row.meanTravelTime ? Json(*row.meanTravelTime) : Json(nullptr)},
                                     {"meanDelay", row.meanDelay ? Json(*row.meanDelay) : Json(nullptr)},
                                     {"unfinished", row.unfinished},
                                     {"controlType", row.controlType ? Json(*row.controlType) : Json(nullptr)},
                                     {"los", losCell(row.meanDelay, row.controlType, r.los).empty() ? Json(nullptr)
                                                                                                  : Json(losCell(row.meanDelay, row.controlType, r.los))}});
        addLosJson(j, losInputs(r), r.los); // M5.5
    }
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
        // D95, written only when there is one, so a run without discretionary changes reads as before.
        if (row.discretionaryChanges) j["rows"].back()["discretionaryChanges"] = row.discretionaryChanges;
        if (row.quickRepeats) {
            j["rows"].back()["quickRepeats"] = row.quickRepeats;
            j["rows"].back()["quickRepeatsByKind"] = {{"back", row.quickBack}, {"afterMandatory", row.quickAfterMandatory},
                                                      {"onward", row.quickOnward}};
        }
    }
    return j;
}
Json waitCauseJson(const WaitCauseReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = "dead-end waits by the cause found at each wait's start, by the movement a vehicle arrived on; one run";
    j["rows"] = Json::array();
    for (const auto& row : r.rows) {
        Json causes = Json::object();
        for (std::size_t c = 0; c < kWaitCauses; ++c)
            causes[waitCauseName(static_cast<WaitCause>(c))] = {{"waits", row.waits[c]}, {"seconds", row.seconds[c]}};
        j["rows"].push_back({{"movement", row.name}, {"causes", causes}});
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
Json arrivalPhaseJson(const ArrivalPhaseReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = "cycle phase (time mod cycle) at which vehicles entered each segment and first stood on it, by movement; one run";
    j["cycle"] = r.cycle; j["binWidth"] = r.binWidth; j["unassigned"] = r.unassigned;
    j["rows"] = Json::array();
    for (const auto& row : r.rows)
        j["rows"].push_back({{"movement", row.movement}, {"segmentId", row.segmentId}, {"entered", row.entered},
                             {"stopped", row.stopped}, {"enteredAt", row.enteredAt}, {"firstStopAt", row.firstStopAt}});
    return j;
}
std::string movementCsv(const MovementReport& r) {
    std::ostringstream out;
    out << "# TrafficSim - not yet validated. Simulated movement delay, not HCM control delay; one run.\n";
    out << "movement,vehicles,meanDelay_s,meanTravelTime_s,unfinished\n";
    for (const auto& m : r.movements)
        out << csvQuoted(m.name) << ',' << m.vehicles << ',' << csvNumber(m.meanDelay) << ',' << csvNumber(m.meanTravelTime) << ',' << m.unfinished << '\n';
    out << "\napproach,meanQueue_m,maxQueue_m\n";
    for (const auto& q : r.queues)
        out << csvQuoted(q.name) << ',' << csvNumber(q.meanLength) << ',' << csvNumber(q.maxLength) << '\n';
    if (!r.sections.empty()) { // M5.4
        out << "\nsection,vehicles,meanTravelTime_s,meanDelay_s,unfinished,controlType,los\n";
        for (const auto& row : r.sections)
            out << csvQuoted(row.name) << ',' << row.vehicles << ',' << csvNumber(row.meanTravelTime) << ','
                << csvNumber(row.meanDelay) << ',' << row.unfinished << ',' << row.controlType.value_or("") << ','
                << losCell(row.meanDelay, row.controlType, r.los) << '\n';
        writeLosCsv(out, losInputs(r), r.los); // M5.5
    }
    out << "\ncompleted," << r.completed << "\nnotInMovement," << r.unassigned << "\npending," << r.pending
        << "\nactive," << r.active << "\nsafetyClamps," << r.safetyClamps << "\nlaneChanges," << r.laneChanges
        << "\nevaluationPeriod_s," << csvNumber(r.warmup) << ',' << csvNumber(r.evaluationEnd) << '\n';
    if (r.cooldown) out << "cooldown_s," << csvNumber(*r.cooldown) << '\n'; // M5.9: only with one
    return out.str();
}
}
