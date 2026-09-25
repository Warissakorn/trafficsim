#pragma once
// The four-leg signalised intersection with turn pockets that M1's done-condition asks an engineer
// to draw and M2's done-condition asks the engine to run. Built through the same commands the
// editor issues, so what it produces is a document an author could have made by hand.
//
// It is ONE drawing, committed as data/projects/four-leg-signalised.traffic.json. The file is the
// thing the editor opens; this builder is its source, and `four-leg` checks the two are identical
// so neither drifts (hard rule 3). Regenerate with trafficsim-four-leg-fixture, never by hand.
//
// Left-hand traffic, as in Thailand: lane 0 of every Link is the kerb lane, the LEFT turn is the
// short kerbside turn, and the RIGHT turn is the one that crosses opposing traffic and has the
// pocket. Volumes, timings and dimensions are plausible round numbers, not counted data -- this is
// a fixture, and nothing measured here is a claim about any real intersection (rule 4).
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <array>
#include <string>

namespace trafficsim::fixture {
struct FourLegOptions {
    // Where each turning Connector joins its exit Link, in metres along it. Zero -- the default,
    // and how an engineer draws it -- meets the exit at its start, where the turns from three
    // approaches merge and are ordered by M3.1's derived rule in drawing order (M2.0.1, D35).
    // A positive value joins part way along the body instead: the staggered drawing that was
    // needed before M2.0.1, kept as a variant so both shapes stay runnable.
    double leftArrival = 0, rightArrival = 0;
    // Metres upstream of the kerb lane's head at which the LEFT turn leaves it. Zero -- the
    // default -- leaves at the stop line, so the left turn waits for green. A positive value is
    // the Thai left turn at all times: the Connector leaves before the head and never sees it.
    double leftBypass = 0;
};
struct FourLeg {
    ProjectDocument document;
    std::vector<std::string> routes;  // 12 movements, three per approach
};

namespace detail {
struct Frame { Point u, n; };  // u: direction of travel INTO the junction; n: its left normal
inline Point at(const Frame& f, double along, double across) {
    // `along` is distance from the centre measured against travel (positive = upstream);
    // `across` is offset to the approaching driver's left.
    return {-f.u.x * along + f.n.x * across, -f.u.y * along + f.n.y * across};
}
inline const Link& link(const ProjectDocument& d, const std::string& id) {
    for (const auto& l : d.network.links) if (l.id == id) return l;
    throw std::invalid_argument("UNKNOWN_LINK");
}
inline std::string lane(const ProjectDocument& d, const std::string& linkId, std::size_t index) {
    return link(d, linkId).lanes.at(index).id;
}
}

inline FourLeg fourLegIntersection(const FourLegOptions& options = {}) {
    using namespace detail;
    constexpr double w = 3.5;           // lane width
    constexpr double stopLine = 16;     // approach ends / exit starts, metres from the centre
    constexpr double pocketStart = 60;  // start of the 3-lane approach Link
    constexpr double taperStart = 80;   // end of the 2-lane upstream Link
    constexpr double far = 300;         // upstream end of every leg
    // Across-offsets to the approaching driver's left, measured from the leg's axis. The pocket
    // lane is carved out of a 4 m median; the exit carriageway lies on the other side of it.
    constexpr double upstreamCentre = 0.25 + w + w;  // lanes 0-1 over [3.75, 10.75]
    constexpr double pocketCentre = 0.25 + 1.5 * w;  // lanes 0-2 over [0.25, 10.75]
    constexpr double exitCentre = -0.25 - w;         // lanes 0-1 over [-7.25, -0.25], mirrored

    FourLeg result;
    auto& d = result.document;
    d.network.id = "four-leg-signalised";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};

    // West, east, south and north approaches: the direction traffic travels INTO the junction.
    const std::array<const char*, 4> names{"West", "East", "South", "North"};
    const std::array<Frame, 4> frames{{{{1, 0}, {0, 1}}, {{-1, 0}, {0, -1}},
                                       {{0, 1}, {-1, 0}}, {{0, -1}, {1, 0}}}};
    std::array<std::string, 4> upstream, pocket, exit, taper, pocketEntry;
    for (std::size_t k = 0; k < 4; ++k) {
        const auto& f = frames[k];
        const std::string name = names[k];
        upstream[k] = addLink(d, {at(f, far, upstreamCentre), at(f, taperStart, upstreamCentre)}, 2, w);
        pocket[k] = addLink(d, {at(f, pocketStart, pocketCentre), at(f, stopLine, pocketCentre)}, 3, w);
        // The exit leaves along -u, so its own left normal is -n: the kerb lane stays outermost.
        exit[k] = addLink(d, {at(f, stopLine, exitCentre), at(f, far, exitCentre)}, 2, w);
        editableLink(d, upstream[k]).name = name + " approach";
        editableLink(d, pocket[k]).name = name + " approach, right-turn pocket";
        editableLink(d, exit[k]).name = name + " exit";
        // The taper: both through lanes carry on, and the inner one also feeds the pocket. With
        // no lane changing yet, this diverge is the only way a vehicle reaches the pocket lane.
        taper[k] = addConnectorRange(d, {upstream[k], lane(d, upstream[k], 0)},
                                     {pocket[k], lane(d, pocket[k], 0)}, 2, 2);
        pocketEntry[k] = addConnector(d, {upstream[k], lane(d, upstream[k], 1)},
                                      {pocket[k], lane(d, pocket[k], 2)});
        editableConnector(d, taper[k]).name = name + " taper";
        editableConnector(d, pocketEntry[k]).name = name + " pocket entry";
    }
    // Receiving leg for each movement, in LHT: left is the kerbside turn, right crosses.
    // West→(through east, left north, right south) and so on round the junction.
    const std::array<std::size_t, 4> through{1, 0, 3, 2}, left{3, 2, 0, 1}, right{2, 3, 1, 0};
    const auto arrival = [](double station) {
        return station > 0 ? std::optional<double>(station) : std::nullopt;
    };
    std::array<std::array<std::string, 3>, 4> movement;
    for (std::size_t k = 0; k < 4; ++k) {
        const std::string name = names[k];
        const auto to = [&](std::size_t leg, std::size_t index, std::optional<double> station) {
            return LaneReference{exit[leg], lane(d, exit[leg], index), station};
        };
        movement[k][0] = addConnectorRange(d, {pocket[k], lane(d, pocket[k], 0)},
                                           to(through[k], 0, std::nullopt), 2, 2);
        std::optional<double> leftFrom;
        if (options.leftBypass > 0) leftFrom = pocketStart - stopLine - 1 - options.leftBypass;
        movement[k][1] = addConnector(d, {pocket[k], lane(d, pocket[k], 0), leftFrom},
                                      to(left[k], 0, arrival(options.leftArrival)));
        movement[k][2] = addConnector(d, {pocket[k], lane(d, pocket[k], 2)},
                                      to(right[k], 1, arrival(options.rightArrival)));
        editableConnector(d, movement[k][0]).name = name + " through";
        editableConnector(d, movement[k][1]).name = name + " left";
        editableConnector(d, movement[k][2]).name = name + " right";
    }
    // Split phasing, one approach at a time, 120 s cycle: green, 3 s amber, 2 s all-red. With
    // no crossing-conflict model before M3, this is the only phasing the engine can run honestly:
    // no two conflicting movements ever have green together. One controller, one signal group
    // per approach (M2.7b), exactly as the timing sheet reads.
    const std::array<double, 4> green{30, 30, 20, 20};
    SignalController controller{"", "Four-leg", 120, 0, {}};
    double start = 0;
    for (std::size_t k = 0; k < 4; ++k) {
        controller.groups.push_back({static_cast<int>(k) + 1, names[k], start, start + green[k], 3});
        start += green[k] + 5;
    }
    const auto controllerId = putSignalController(d, controller);
    for (std::size_t k = 0; k < 4; ++k) {
        for (std::size_t index = 0; index < 3; ++index) {
            NetworkSignalHead head;
            head.lane = {pocket[k], lane(d, pocket[k], index)};
            head.position = polylineLength(laneGeometry(link(d, pocket[k]), head.lane.laneId,
                                                        d.network.drivingSide)) - 1;
            head.controllerId = controllerId; head.groupNumber = static_cast<int>(k) + 1;
            head.name = std::string(names[k]) + " signal, lane " + std::to_string(index + 1);
            putSignalHead(d, head);
        }
    }
    // Four programs took four ids here before M2.7b; one controller takes one. Skipping the other
    // three keeps every later id -- and so the order vehicles are drawn in, and every four-leg
    // number already published (M2.5, D39) -- exactly as it was. The signal colours are identical.
    d.nextId += 3;
    // Link-total volumes per movement, veh/h: a busier east-west main road, a quieter side road.
    const std::array<std::array<double, 3>, 4> volume{{{500, 120, 100}, {450, 110, 90},
                                                      {250, 60, 50}, {220, 50, 40}}};
    constexpr double duration = 900;
    // Each route NAMES the Connector into the pocket Link. Naming only the two Links would imply
    // it, but the inner upstream lane has two ways into that Link -- on through, or into the
    // pocket -- and an implied bridge that is ambiguous for a lane drops that lane from the route
    // (routeLaneChains refuses to guess). Through and left go by the taper, right by the entry.
    for (std::size_t k = 0; k < 4; ++k)
        for (std::size_t m = 0; m < 3; ++m) {
            const auto& entry = m == 2 ? pocketEntry[k] : taper[k];
            const auto route = putRoute(d, {"", {upstream[k], entry, pocket[k], movement[k][m],
                                                 exit[m == 0 ? through[k] : m == 1 ? left[k] : right[k]]}});
            putInput(d, {"", route, "car", volume[k][m], 0, duration, {}});
            result.routes.push_back(route);
        }
    changeRunSettings(d, duration, 0.1);
    return result;
}
}
