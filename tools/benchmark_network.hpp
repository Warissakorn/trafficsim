#pragma once
// The one network both benchmarks measure. Shared so the editor and the engine are timed on the
// same fixture and neither drifts from the other (hard rule 3). Not a test fixture: nothing here
// asserts, and no measured claim depends on the shape beyond it being deterministic.
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <string>
#include <vector>

namespace trafficsim::benchmark {
constexpr int kLanes = 3;
// A corridor of signalised crossings: at each one an eastbound carriageway meets a northbound
// approach, and both turn into the next crossing's eastbound link. Two Links, two Connectors and
// one signal head per intersection, three lanes throughout -- the shape a corridor study draws,
// and the shape whose cost a frame and a tick both pay.
struct Corridor {
    ProjectDocument document;
    std::vector<std::string> eastbound, northbound;
    std::vector<std::string> through;  // eastbound[k] -> eastbound[k+1]
    std::vector<std::string> entering; // northbound[k] -> eastbound[k+1]
};
// `enteringStation` is where the northbound approach joins the next eastbound link. 0 means the
// link's start, which is a plain end attachment; a positive value is a mid-body arrival, which
// cuts the link into runtime sections and derives the M3.1 priority rule a merge needs to RUN.
// The editor benchmark keeps 0 so the frame times published for M1.27.1 stay reproducible from
// this tool; the engine benchmark needs a runnable merge, so it passes a station.
inline Corridor corridor(int intersections, double enteringStation = 0) {
    Corridor result;
    auto& d = result.document;
    for (int k = 0; k < intersections; ++k) {
        const double x = 200. * k;
        result.eastbound.push_back(addLink(d, {{x, 0}, {x + 90, 0}}, kLanes, 3.5));
        result.northbound.push_back(addLink(d, {{x + 95, -100}, {x + 95, -10}}, kLanes, 3.5));
    }
    const auto firstLane = [&](const std::string& link) {
        for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
        return std::string{};
    };
    for (int k = 0; k + 1 < intersections; ++k) {
        const std::string lane = firstLane(result.eastbound[k + 1]);
        const LaneReference start{result.eastbound[k + 1], lane};
        LaneReference joined{result.eastbound[k + 1], lane};
        if (enteringStation > 0) joined.station = enteringStation;
        result.through.push_back(addConnectorRange(
            d, {result.eastbound[k], firstLane(result.eastbound[k])}, start, kLanes, kLanes));
        result.entering.push_back(addConnectorRange(
            d, {result.northbound[k], firstLane(result.northbound[k])}, joined, kLanes, kLanes));
    }
    // One program for the whole corridor rather than a progression: the benchmark measures cost,
    // and offsets per crossing would change which vehicles queue without changing the work done
    // per tick. Every northbound approach carries a head; the eastbound through movement does
    // not, so the corridor has both a signalised and an unsignalised stream.
    const auto program = putProgram(d, {"", 0, {{30, SignalColor::green},
                                                {3, SignalColor::amber},
                                                {27, SignalColor::red}}});
    for (int k = 0; k < intersections; ++k) {
        NetworkSignalHead head;
        head.lane = {result.northbound[k], firstLane(result.northbound[k])};
        head.position = 45;
        head.programId = program;
        putSignalHead(d, head);
    }
    return result;
}
}
