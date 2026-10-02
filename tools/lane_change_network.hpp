#pragma once
// The lane-change lab (M3.2.8c, D98): four small scenes, each isolating one lane-change situation,
// so a change to lane changing can be iterated on in seconds before A55 runs it on the four-leg
// and M2.6 drawings. It is a development bed, not an acceptance fixture: A55 is judged on those two.
// Built through the editor's own commands, like the other fixtures; committed as
// data/projects/lane-change-lab.traffic.json, and `lanelab` checks the file is this builder's
// output. Regenerate with trafficsim-lane-change-fixture, never by hand.
//
// The scenes share nothing and lie 60 m apart, so each is its own movement row:
//   Overtaking   one 2-lane Link: cars behind heavy vehicles, discretionary changes both ways (A47,
//                A53's back-and-forth).
//   Three lanes  one 3-lane Link, the same mix: a change can carry on to a third lane.
//   Lane drop    2 lanes, a 1-lane Connector, 2 lanes (D96): the upstream second lane is a stub, and
//                no route reaches the downstream second lane -- the limitation D95 does not close.
//   Diverge      3 lanes splitting into a 1-lane and a 2-lane exit: mandatory changes onto the
//                2-lane exit's lanes, then discretionary ones between them.
//
// Left-hand traffic, as the other fixtures. The D95 fields are not set here: the behaviour catalog
// decides, and trafficsim-lane-change-sweep sets them per variant. Volumes and dimensions are
// plausible round numbers, not counts: nothing measured here is a claim about a real road (rule 4).
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <stdexcept>
#include <string>

namespace trafficsim::fixture {
struct LaneChangeLab {
    ProjectDocument document;
    std::string overtaking, threeLanes;           // Links
    std::string dropUpstream, dropDownstream, drop; // Links and the 1-lane Connector
    std::string diverge, exitA, exitB;             // Links
    std::string overtakingRoute, threeLaneRoute, dropRoute, exitARoute, exitBRoute;
};

inline LaneChangeLab laneChangeLab() {
    constexpr double w = 3.5, gap = 60, length = 1000;
    const auto lane = [](const ProjectDocument& d, const std::string& id, std::size_t k) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(k).id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    LaneChangeLab t;
    auto& d = t.document;
    d.network.id = "lane-change-lab";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};

    t.overtaking = addLink(d, {{0, 0}, {length, 0}}, 2, w);
    editableLink(d, t.overtaking).name = "Overtaking";
    t.threeLanes = addLink(d, {{0, -gap}, {length, -gap}}, 3, w);
    editableLink(d, t.threeLanes).name = "Three lanes";

    t.dropUpstream = addLink(d, {{0, -2 * gap}, {300, -2 * gap}}, 2, w);
    t.dropDownstream = addLink(d, {{320, -2 * gap}, {820, -2 * gap}}, 2, w);
    editableLink(d, t.dropUpstream).name = "Lane drop upstream";
    editableLink(d, t.dropDownstream).name = "Lane drop downstream";
    t.drop = addConnector(d, {t.dropUpstream, lane(d, t.dropUpstream, 0)}, {t.dropDownstream, lane(d, t.dropDownstream, 0)});
    editableConnector(d, t.drop).name = "Lane drop";

    // The 1-lane exit leaves from lane 0, the kerb lane, on the kerb side (+y in left-hand traffic);
    // the 2-lane exit takes lanes 1-2 straight on, so the two Connectors do not cross.
    const double y = -3 * gap;
    t.diverge = addLink(d, {{0, y}, {600, y}}, 3, w);
    t.exitA = addLink(d, {{620, y + 2 * w}, {900, y + 2 * w}}, 1, w);
    t.exitB = addLink(d, {{620, y - w / 2}, {900, y - w / 2}}, 2, w);
    editableLink(d, t.diverge).name = "Diverge";
    editableLink(d, t.exitA).name = "Exit A";
    editableLink(d, t.exitB).name = "Exit B";
    const auto toA = addConnector(d, {t.diverge, lane(d, t.diverge, 0)}, {t.exitA, lane(d, t.exitA, 0)});
    const auto toB = addConnectorRange(d, {t.diverge, lane(d, t.diverge, 1)}, {t.exitB, lane(d, t.exitB, 0)}, 2, 2);
    editableConnector(d, toA).name = "To exit A";
    editableConnector(d, toB).name = "To exit B";

    // Veh/h; the inputs stop at 600 s and the run continues to 900 s, so the demand drains. An
    // input's volume is the Link total, split equally over the lanes its route enters on.
    t.overtakingRoute = putRoute(d, {"", {t.overtaking}});
    t.threeLaneRoute = putRoute(d, {"", {t.threeLanes}});
    t.dropRoute = putRoute(d, {"", {t.dropUpstream, t.drop, t.dropDownstream}});
    t.exitARoute = putRoute(d, {"", {t.diverge, toA, t.exitA}});
    t.exitBRoute = putRoute(d, {"", {t.diverge, toB, t.exitB}});
    putInput(d, {"", t.overtakingRoute, "car", 1000, 0, 600, {}});
    putInput(d, {"", t.overtakingRoute, "heavy-vehicle", 150, 0, 600, {}});
    putInput(d, {"", t.threeLaneRoute, "car", 1500, 0, 600, {}});
    putInput(d, {"", t.threeLaneRoute, "heavy-vehicle", 200, 0, 600, {}});
    putInput(d, {"", t.dropRoute, "car", 900, 0, 600, {}});
    putInput(d, {"", t.exitARoute, "car", 400, 0, 600, {}});
    putInput(d, {"", t.exitBRoute, "car", 1000, 0, 600, {}});
    changeRunSettings(d, 900, 0.1);
    return t;
}
}
