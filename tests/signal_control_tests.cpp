#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/model/demand/signal_control.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
using namespace trafficsim;
// M2.7: a Signal head is its stop line, placed where it is clicked (D47), and it shows a signal
// group of a fixed-time controller, expanded at compile time into a core program (D48).
namespace {
SignalController controller() {
    // Offset 7, one plain group, one wrapping past the cycle's end, one with no amber and one
    // with no red: every shape the expansion has to rotate.
    return {"c", "Main", 90, 7, {{1, "A", 0, 30, 3}, {2, "B", 80, 10, 4}, {3, "C", 40, 60, 0}, {4, "D", 10, 87, 3}}};
}
ProjectDocument road() {
    ProjectDocument d; d.definition = AuthoringDefinition{};
    const auto link = addLink(d, {{0, 0}, {100, 0}}, 2, 3.5);
    // Compiling needs traffic to compile.
    putInput(d, {"", putRoute(d, {"", {link}}), "car", 600, 0, 60});
    return d;
}
}
TEST(signals, a_group_expands_to_a_program_showing_the_same_colour_at_every_tick) {
    const auto c = controller();
    // The forcing first: the authored numbers mean what the dialog says. At t=0 the controller
    // is at cycle second 7, so A (green 0..30) and B (green 80..10, wrapping) are green and
    // C (green 40..60) is red.
    CHECK(signalGroupColorAt(c, c.groups[0], 0) == SignalColor::green);
    CHECK(signalGroupColorAt(c, c.groups[1], 0) == SignalColor::green);
    CHECK(signalGroupColorAt(c, c.groups[1], 4) == SignalColor::amber); // cycle second 11
    CHECK(signalGroupColorAt(c, c.groups[2], 0) == SignalColor::red);
    CHECK(signalGroupColorAt(c, c.groups[0], 24) == SignalColor::amber); // cycle second 31
    test::near(signalGroupGreen(c, c.groups[1]), 20);
    for (const auto& g : c.groups) {
        const auto program = signalGroupProgram(c, g);
        CHECK(program.id == "c#" + std::to_string(g.number));
        double cycle = 0; for (const auto& phase : program.phases) { CHECK(phase.duration > 0); cycle += phase.duration; }
        test::near(cycle, 90);
        for (int k = 0; k <= 2000; ++k) {
            const double t = k * 0.1;
            CHECK(signalColorAt(program, t) == signalGroupColorAt(c, g, t));
        }
    }
}
TEST(signals, timing_numbering_and_head_references_are_checked) {
    auto d = road();
    const auto codes = [&] {
        std::vector<std::string> out;
        for (const auto& i : signalControlIssues(d.network, *d.definition)) out.push_back(i.code + "@" + i.path);
        return out;
    };
    d.definition->signalControllers = {controller()};
    CHECK(codes().empty());
    auto& g = d.definition->signalControllers[0].groups;
    g[1].number = 1;                 // duplicate
    g[2].greenEnd = g[2].greenStart; // no green
    g[3].amber = 20;                 // green 77 + amber 20 overflows the 90 s cycle
    g[0].greenStart = 90;            // not a second of a 90 s cycle
    const auto found = codes();
    const auto has = [&](const std::string& s) { return std::find(found.begin(), found.end(), s) != found.end(); };
    CHECK(has("DUPLICATE_SIGNAL_GROUP@signalControllers[0].groups[1].number"));
    CHECK(has("INVALID_SIGNAL_TIMING@signalControllers[0].groups[2].greenEnd"));
    CHECK(has("INVALID_SIGNAL_TIMING@signalControllers[0].groups[3].amber"));
    CHECK(has("INVALID_SIGNAL_TIMING@signalControllers[0].groups[0].greenStart"));
    d.definition->signalControllers = {controller()};
    NetworkSignalHead head; head.lane = {d.network.links[0].id, d.network.links[0].lanes[0].id};
    head.position = 90; head.controllerId = "c"; head.groupNumber = 9;
    d.network.signalHeads = {head};
    CHECK(codes() == std::vector<std::string>{"UNKNOWN_SIGNAL_GROUP@signalHeads[0].groupNumber"});
}
TEST(signals, commands_refuse_to_orphan_a_head_and_a_run_uses_the_group) {
    History h; h.reset(road());
    std::string id;
    h.execute("controller", [&](auto& d) { id = putSignalController(d, controller()); });
    h.execute("head", [&](auto& d) {
        NetworkSignalHead head; head.lane = {d.network.links[0].id, d.network.links[0].lanes[0].id};
        head.position = 95; head.controllerId = id; head.groupNumber = 2; putSignalHead(d, head);
    });
    test::throws([&] { h.execute("delete", [&](auto& d) { deleteSignalController(d, id); }); }, "EDIT_REFERENCED_CONTROLLER");
    // Dropping the group a head shows is refused on commit, and the document is unchanged.
    const auto before = documentJson(h.document());
    test::throws([&] { h.execute("drop", [&](auto& d) {
        auto c = d.definition->signalControllers[0]; c.groups.erase(c.groups.begin() + 1); putSignalController(d, c); }); },
        "UNKNOWN_SIGNAL_GROUP");
    CHECK(documentJson(h.document()) == before);
    // Sliding the stop line is one command, validated against the lane's length.
    const auto headId = h.document().network.signalHeads[0].id;
    h.execute("move", [&](auto& d) { moveSignalHead(d, headId, 40); });
    test::near(h.document().network.signalHeads[0].position, 40);
    test::throws([&] { h.execute("far", [&](auto& d) { moveSignalHead(d, headId, 400); }); }, "INVALID_POSITION");
    h.undo(); test::near(h.document().network.signalHeads[0].position, 95);
    // The compiled head runs against its group's program.
    const auto snapshot = compileDocument(h.document(), test::root() / "data");
    CHECK(snapshot.scenario.signalHeads.size() == 1);
    CHECK(snapshot.scenario.signalHeads[0].programId == id + "#2");
    CHECK(std::any_of(snapshot.scenario.signalPrograms.begin(), snapshot.scenario.signalPrograms.end(),
                      [&](const auto& p) { return p.id == id + "#2"; }));
}
TEST(signals, schema_13_round_trips_and_schema_12_programs_migrate_colour_for_colour) {
    auto d = road();
    const auto link = d.network.links[0];
    // Legacy programs as schema 12 wrote them: one starting red, one with an offset and green
    // wrapping round the cycle's end, one that never turns green and one with two greens.
    const std::vector<SignalProgram> legacy{
        {"", 0, {{33, SignalColor::red}, {30, SignalColor::green}, {3, SignalColor::amber}, {24, SignalColor::red}}},
        {"", 12.5, {{10, SignalColor::green}, {4, SignalColor::amber}, {66, SignalColor::red}, {10, SignalColor::green}}},
        {"", 0, {{60, SignalColor::red}}},
        {"", 0, {{10, SignalColor::green}, {10, SignalColor::red}, {10, SignalColor::green}, {60, SignalColor::red}}}};
    std::vector<std::string> ids;
    for (const auto& p : legacy) ids.push_back(putProgram(d, p));
    for (std::size_t k = 0; k < ids.size(); ++k) {
        NetworkSignalHead head; head.lane = {link.id, link.lanes[k % 2].id}; head.position = 10.0 + 10 * k; head.programId = ids[k];
        putSignalHead(d, head);
    }
    const auto before = d;
    auto file = documentJson(d); CHECK(file["schemaVersion"] == 13);
    // Schema 13 keeps whatever it was given: a legacy program written today is not rewritten.
    CHECK(parseDocument(file) == d);
    file["schemaVersion"] = 12;
    const auto migrated = parseDocument(file);
    // The forcing: the migration ran -- two programs became groups of one 90 s controller.
    CHECK(migrated.definition->signalControllers.size() == 1);
    CHECK(migrated.definition->signalControllers[0].groups.size() == 2);
    CHECK(migrated.definition->signalPrograms.size() == 2); // never green, and two greens
    CHECK(migrated.network.signalHeads[0].programId.empty() && !migrated.network.signalHeads[0].controllerId.empty());
    CHECK(migrated.network.signalHeads[2].programId == ids[2]);
    // The consequence: every head shows the colour it showed before, at every tick.
    const auto a = compileDocument(before, test::root() / "data").scenario;
    const auto b = compileDocument(migrated, test::root() / "data").scenario;
    CHECK(a.signalHeads.size() == b.signalHeads.size());
    const auto program = [](const Scenario& s, const std::string& id) {
        return *std::find_if(s.signalPrograms.begin(), s.signalPrograms.end(), [&](const auto& p) { return p.id == id; });
    };
    for (std::size_t k = 0; k < a.signalHeads.size(); ++k)
        for (int tick = 0; tick <= 3000; ++tick)
            CHECK(signalColorAt(program(a, a.signalHeads[k].programId), tick * 0.1) ==
                  signalColorAt(program(b, b.signalHeads[k].programId), tick * 0.1));
    // And schema 13 reads back what it wrote.
    CHECK(parseDocument(documentJson(migrated)) == migrated);
}
TEST(signals, a_click_lands_on_the_lane_or_connector_path_under_it) {
    ProjectDocument d;
    const auto west = addLink(d, {{-100, 0}, {-10, 0}}, 2, 3.5);
    const auto east = addLink(d, {{10, 0}, {100, 0}}, 2, 3.5);
    addConnectorRange(d, {west, d.network.links[0].lanes[0].id}, {east, d.network.links[1].lanes[0].id}, 2, 2);
    const auto second = laneGeometry(d.network.links[0], d.network.links[0].lanes[1].id, d.network.drivingSide);
    const auto at = pointAlong(second, 37.5);
    const auto onLane = nearestHeadSlot(d.network, {at.x, at.y + 0.5}, 0);
    CHECK(onLane && onLane->slot.lane.laneId == d.network.links[0].lanes[1].id && onLane->slot.connectorId.empty());
    test::near(onLane->station, 37.5, 1e-6);
    const auto path = connectorPaths(d.network, d.network.connectors[0])[1];
    const auto mid = pointAlong(path.geometry, polylineLength(path.geometry) / 2);
    const auto onPath = nearestHeadSlot(d.network, mid, 0);
    CHECK(onPath && onPath->slot.connectorId == path.id && onPath->slot.lane.laneId.empty());
    CHECK(!nearestHeadSlot(d.network, {0, 40}, 0));   // off every lane
    CHECK(!nearestHeadSlot(d.network, at, 1));         // another level
}
