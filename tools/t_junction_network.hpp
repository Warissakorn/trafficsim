#pragma once
// The T-junction of docs/M3_ACCEPTANCE.md §2 (M3.2.7, A26): a two-way major road, one minor
// approach, no signals, and a minor turn that CROSSES the near major stream and then MERGES into
// the far one across a 1 m median -- a crossing area and a separate downstream merge, so a merge-only drawing cannot
// pretend to exercise crossing control. Built through the editor's own commands, like the four-leg
// fixture; the right-hand Yield base is committed as data/projects/t-junction-priority.traffic.json
// and `tjunction` checks the file is this builder's output. Regenerate with
// trafficsim-t-junction-fixture, never by hand.
//
// Right-hand traffic (the base): the major road runs along x, eastbound on the south side (y < 0),
// westbound on the north. The minor road arrives from the south heading north. Its LEFT turn
// crosses eastbound and merges into westbound; its right turn merges into eastbound only.
//
// Left-hand traffic is the mirror in y: eastbound on the north side, the minor road from the
// north heading south. The movement that crosses and then merges is then the minor RIGHT turn,
// and the near-side kerb turn is the left. The mirror exercises the same crossing with the other
// hand; it is not a claim that a left-hand near-side turn crosses anything.
//
// Volumes and dimensions are plausible round numbers, not counts: nothing measured here is a claim
// about a real junction (rule 4). One lane per Link keeps every lane on its Link's reference line.
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include <algorithm>
#include <optional>
#include <string>

namespace trafficsim::fixture {
struct TJunctionOptions {
    DrivingSide side = DrivingSide::right;
    // What the minor approach does at its lines: Yield (the gap test alone), Stop (a full stop
    // first), or no control at all (the rule's gap test still holds; there is simply no sign).
    std::optional<StopMode> control = StopMode::yield;
    // A permanently red head on the far major lane past the merge: its queue fills the lane the
    // crossing movement must enter, which is what receiving-space control exists for.
    bool blockedExit = false;
    // M3.2.7c. A fixed-time head on the minor Link 1 m before its end -- upstream of both of the
    // minor road's waiting lines, which are on the Connectors -- showing these phases. Empty: none.
    std::vector<SignalPhase> minorSignal;
    // M3.2.7c. A fixed-time head on eastbound 16 m past the crossing, short of the near-turn merge. Its queue
    // backs up through the junction and discharges slowly, so slow and standing major vehicles
    // stand near the crossing entry -- the traffic in which `headway`, not `gapTime`, decides.
    bool congestedMajor = false;
    // M3.2.4c. False draws the roads, routes and demand only: no area, line, rule, control or
    // counter, so what remains is exactly what the editor derives on its own (automaticConflicts).
    bool authoredControls = true;
    // The rule on every area the minor road gives way at (contract §1 parameter names).
    double gapTime = 5, headway = 7;
};
struct TJunction {
    ProjectDocument document;
    std::string eastbound, westbound, minor;          // Links
    std::string crossingTurn, nearTurn;               // Connectors: crosses-then-merges, merges only
    std::string crossingArea, crossingMerge, nearMerge; // conflict areas
    std::string crossingRoute, nearRoute, eastRoute, westRoute;
    std::string counter;
    std::string minorHead; // M3.2.7c, when TJunctionOptions::minorSignal is set
};

inline TJunction tJunction(const TJunctionOptions& options = {}) {
    constexpr double w = 3.5, median = 1, far = 150, minorEnd = 12, join = 20;
    constexpr double centre = (w + median) / 2; // each carriageway's lane centre from the axis
    // y of the right-hand base; the left-hand mirror flips it.
    const double m = options.side == DrivingSide::right ? 1 : -1;
    const auto p = [&](double x, double y) { return Point{x, m * y}; };
    const auto lane0 = [](const ProjectDocument& d, const std::string& id) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.front().id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    TJunction t;
    auto& d = t.document;
    d.network.id = "t-junction-priority";
    d.network.drivingSide = options.side;
    d.definition = AuthoringDefinition{};
    t.eastbound = addLink(d, {p(-far, -centre), p(far, -centre)}, 1, w);
    t.westbound = addLink(d, {p(far, centre), p(-far, centre)}, 1, w);
    t.minor = addLink(d, {p(0, -far), p(0, -minorEnd)}, 1, w);
    editableLink(d, t.eastbound).name = "Major eastbound";
    editableLink(d, t.westbound).name = "Major westbound";
    editableLink(d, t.minor).name = "Minor approach";
    // Both turns join their receiving Link part way along, `join` metres past the minor axis.
    t.crossingTurn = addConnector(d, {t.minor, lane0(d, t.minor)}, {t.westbound, lane0(d, t.westbound), far + join});
    t.nearTurn = addConnector(d, {t.minor, lane0(d, t.minor)}, {t.eastbound, lane0(d, t.eastbound), far + join});
    editableConnector(d, t.crossingTurn).name = "Minor crossing turn";
    editableConnector(d, t.nearTurn).name = "Minor near turn";

    // M3.2.4c: without them, the drawing alone -- the automatic areas the editor derives from it.
    if (options.authoredControls) {
        const PriorityDefaults defaults{options.gapTime, options.headway};
        const auto crossings = addCrossingAreas(d, t.crossingTurn, t.eastbound, t.crossingTurn, defaults);
        if (crossings.size() != 1) throw std::logic_error("T_JUNCTION_CROSSING");
        t.crossingArea = crossings.front();
        // A taken-over merge keeps the drawing-order fallback; the minor road gives way at both.
        const auto yieldingMerge = [&](const std::string& connector, const std::string& name) {
            const auto areas = takeOverMergesOf(d, connector, defaults);
            if (areas.size() != 1) throw std::logic_error("T_JUNCTION_MERGE");
            const auto& a = *std::find_if(d.network.rightOfWay.conflictAreas.begin(), d.network.rightOfWay.conflictAreas.end(),
                                          [&](const auto& x) { return x.id == areas.front(); });
            const auto yields = a.first.path.connectorId == connector ? ConflictPriority::firstYields : ConflictPriority::secondYields;
            setConflictControl(d, a.id, name, yields, options.gapTime, options.headway);
            return a.id;
        };
        setConflictControl(d, t.crossingArea, "Minor crosses eastbound", ConflictPriority::firstYields, options.gapTime, options.headway);
        t.crossingMerge = yieldingMerge(t.crossingTurn, "Minor joins westbound");
        t.nearMerge = yieldingMerge(t.nearTurn, "Minor joins eastbound");
        // The sign stands where the minor road first gives way on each turn; the crossing turn's merge
        // line comes after a crossing already made, where a second stop would be no sign anyone posts.
        if (options.control) {
            setAreaControl(d, t.crossingArea, options.control);
            setAreaControl(d, t.nearMerge, options.control);
        }
        // The minor approach's queue, measured at both lines it forms behind.
        AuthoredQueueCounter counter{"", "Minor approach queue", {}};
        for (const auto* area : {&t.crossingArea, &t.nearMerge})
            for (const auto& a : d.network.rightOfWay.conflictAreas) if (a.id == *area)
                counter.lines.push_back({(a.first.path.connectorId.empty() ? a.second : a.first).waitingLineId, std::nullopt});
        t.counter = putQueueCounter(d, counter);
    }
    if (options.blockedExit) {
        const auto red = putProgram(d, {"", 0, {{60, SignalColor::red}}});
        putSignalHead(d, {"", {t.westbound, lane0(d, t.westbound)}, far + join + 50, red, {}});
    }
    if (!options.minorSignal.empty()) {
        const auto program = putProgram(d, {"", 0, options.minorSignal});
        t.minorHead = putSignalHead(d, {"", {t.minor, lane0(d, t.minor)}, far - minorEnd - 1, program, {}});
    }
    if (options.congestedMajor) {
        const auto program = putProgram(d, {"", 0, {{30, SignalColor::green}, {3, SignalColor::amber}, {27, SignalColor::red}}});
        putSignalHead(d, {"", {t.eastbound, lane0(d, t.eastbound)}, far + 15, program, {}});
    }
    // Veh/h; the inputs stop at 900 s and the run continues to 1500 s, so finite demand drains.
    t.eastRoute = putRoute(d, {"", {t.eastbound}});
    t.westRoute = putRoute(d, {"", {t.westbound}});
    t.crossingRoute = putRoute(d, {"", {t.minor, t.crossingTurn, t.westbound}});
    t.nearRoute = putRoute(d, {"", {t.minor, t.nearTurn, t.eastbound}});
    putInput(d, {"", t.eastRoute, "car", 450, 0, 900, {}});
    putInput(d, {"", t.westRoute, "car", 450, 0, 900, {}});
    putInput(d, {"", t.crossingRoute, "car", 100, 0, 900, {}});
    putInput(d, {"", t.nearRoute, "car", 100, 0, 900, {}});
    changeRunSettings(d, 1500, 0.1);
    return t;
}
}
