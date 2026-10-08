#include "batch_output.hpp"
#include "evaluation.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

namespace trafficsim {
namespace {
Json value(const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); }
Json estimateJson(const Estimate& e) {
    return {{"n", e.n}, {"mean", value(e.mean)}, {"sd", value(e.sd)}, {"halfWidth95", value(e.halfWidth95)}};
}
std::string cells(const Estimate& e) {
    return std::to_string(e.n) + ',' + csvFigure(e.mean) + ',' + csvFigure(e.sd) + ',' + csvFigure(e.halfWidth95);
}
std::string measure(std::size_t seeds) {
    return "simulated movement delay, not HCM control delay or LOS; " + std::to_string(seeds) +
           " seeds; mean, SD and 95% Student-t half-width across seeds; completed trips only";
}
}
Json batchJson(const BatchReport& r) {
    Json j;
    j["validated"] = false;
    j["measure"] = measure(r.runs.size());
    j["seeds"] = Json::array();
    for (const auto& run : r.runs) j["seeds"].push_back(run.seed);
    j["movements"] = Json::array();
    for (const auto& m : r.movements)
        j["movements"].push_back({{"movement", m.name}, {"vehicles", estimateJson(m.vehicles)},
                                  {"meanDelay", estimateJson(m.meanDelay)}, {"meanTravelTime", estimateJson(m.meanTravelTime)}});
    j["queues"] = Json::array();
    for (const auto& q : r.queues)
        j["queues"].push_back({{"approach", q.name}, {"meanLength", estimateJson(q.meanLength)}, {"maxLength", estimateJson(q.maxLength)}});
    j["meanDelay"] = estimateJson(r.meanDelay);
    j["completed"] = estimateJson(r.completed);
    j["safetyClamps"] = estimateJson(r.safetyClamps);
    j["runs"] = Json::array();
    for (const auto& run : r.runs) {
        const auto& x = run.report;
        j["runs"].push_back({{"seed", run.seed}, {"generated", run.generated}, {"completed", x.completed},
            {"active", x.active}, {"pending", x.pending}, {"notInMovement", x.unassigned},
            {"safetyClamps", x.safetyClamps}, {"meanDelay", value(x.meanDelay)}, {"time", x.time}});
    }
    return j;
}
std::string batchCsv(const BatchReport& r) {
    std::ostringstream out;
    auto marker = measure(r.runs.size()); marker[0] = 'S'; // a sentence, as movementCsv's is
    out << "# TrafficSim - not yet validated. " << marker << ".\n";
    out << "movement,delay_n,meanDelay_s,meanDelay_sd_s,meanDelay_halfWidth95_s,"
           "travel_n,meanTravelTime_s,meanTravelTime_sd_s,meanTravelTime_halfWidth95_s,meanVehicles\n";
    for (const auto& m : r.movements)
        out << csvQuoted(m.name) << ',' << cells(m.meanDelay) << ',' << cells(m.meanTravelTime) << ','
            << csvFigure(m.vehicles.mean) << '\n';
    out << "\napproach,n,meanQueue_m,meanQueue_sd_m,meanQueue_halfWidth95_m,maxQueue_n,maxQueue_m,maxQueue_sd_m,maxQueue_halfWidth95_m\n";
    for (const auto& q : r.queues)
        out << csvQuoted(q.name) << ',' << cells(q.meanLength) << ',' << cells(q.maxLength) << '\n';
    out << "\nseed,generated,completed,active,pending,notInMovement,safetyClamps,meanDelay_s\n";
    for (const auto& run : r.runs) {
        const auto& x = run.report;
        out << run.seed << ',' << run.generated << ',' << x.completed << ',' << x.active << ',' << x.pending << ','
            << x.unassigned << ',' << x.safetyClamps << ',' << csvFigure(x.meanDelay) << '\n';
    }
    return out.str();
}
}
