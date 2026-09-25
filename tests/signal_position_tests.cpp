#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include <cmath>
using namespace trafficsim;
// M3.2.6a, A21 of docs/M3_ACCEPTANCE.md: a Signal head keeps the place it was drawn through
// section cuts and Link edits, on both driving sides, and the three readings of that place agree
// -- the command's station, the canvas's slot (headSlot, what canvas_heads.cpp draws) and the
// runtime section the core stops traffic on (rebaseHead).
namespace {
ProjectDocument road(DrivingSide side) {
    ProjectDocument d; d.network.drivingSide = side;
    d.network.links = {{"a", {{0, 0}, {40, 0}, {100, 12}}, {{"a1", 3.5}, {"a2", 3.5}, {"a3", 3.5}}},
                       {"b", {{60, 60}, {160, 60}}, {{"b1", 3.5}}}};
    return d;
}
std::string program(ProjectDocument& d) { return putProgram(d, {"", 0, {{60, SignalColor::red}}}); }
const NetworkSignalHead& head(const ProjectDocument& d, const std::string& id) {
    for (const auto& h : d.network.signalHeads) if (h.id == id) return h;
    throw std::invalid_argument("no head");
}
// Where the canvas draws the stop line.
Point drawn(const ProjectDocument& d, const std::string& id) {
    const auto slot = headSlot(d.network, head(d, id));
    CHECK(slot.has_value());
    return pointAlong(slot->geometry, head(d, id).position);
}
// Where the runtime stops traffic: the compiled segment's polyline at the compiled position.
Point run(const ProjectDocument& d, const std::string& id, std::string* segment = nullptr) {
    const auto table = runtimeSections(d.network);
    const auto compiled = rebaseHead(table, head(d, id));
    if (segment) *segment = compiled.segmentId;
    for (const auto& s : table.sections) if (s.id == compiled.segmentId) return pointAlong(s.geometry, compiled.position);
    for (const auto& p : table.paths) if (p.id == compiled.segmentId) return pointAlong(p.geometry, compiled.position);
    throw std::invalid_argument("no runtime segment");
}
void same(Point a, Point b, double tolerance = 1e-6) { test::near(a.x, b.x, tolerance); test::near(a.y, b.y, tolerance); }
}
TEST(signal_position, a_head_before_on_and_after_a_cut_stands_where_it_was_drawn) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto d = road(side);
        addConnector(d, {"a", "a1", 25}, {"b", "b1", 0}); // cuts a1 at reference station 25
        const auto cut = [&] {
            for (const auto& s : runtimeSections(d.network).sections) if (s.laneId == "a1" && s.start > 0) return s.start;
            return -1.0;
        }();
        CHECK(cut > 0); // the forcing: a1 really is in two sections
        const auto p = program(d);
        const auto before = putSignalHead(d, {"", {"a", "a1"}, cut - 15, p, {}});
        const auto on = putSignalHead(d, {"", {"a", "a1"}, cut, p, {}});
        const auto after = putSignalHead(d, {"", {"a", "a1"}, cut + 30, p, {}});
        std::string sBefore, sOn, sAfter;
        same(run(d, before, &sBefore), drawn(d, before));
        same(run(d, on, &sOn), drawn(d, on));
        same(run(d, after, &sAfter), drawn(d, after));
        CHECK(sBefore == "a1"); CHECK(sOn == "a1"); // exactly on the cut belongs upstream (contract §5)
        CHECK(sAfter != "a1" && sAfter.rfind("a1", 0) == 0);
        const auto table = runtimeSections(d.network);
        test::near(rebaseHead(table, head(d, on)).position, cut, 1e-9); // at the upstream section's end
        test::near(rebaseHead(table, head(d, after)).position, 30, 1e-9);
    }
}
TEST(signal_position, every_lane_of_a_link_holds_its_own_head) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto d = road(side);
        const auto p = program(d);
        for (const auto* lane : {"a1", "a2", "a3"}) {
            const auto id = putSignalHead(d, {"", {"a", lane}, 70, p, {}});
            std::string segment;
            same(run(d, id, &segment), drawn(d, id));
            CHECK(segment == lane);
        }
        // Three distinct stop lines, one lane width apart across the carriageway.
        const auto first = drawn(d, d.network.signalHeads[0].id), second = drawn(d, d.network.signalHeads[1].id);
        test::near(std::hypot(first.x - second.x, first.y - second.y), 3.5, 0.2);
    }
}
TEST(signal_position, stretching_keeps_the_station_and_the_drawn_place) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto d = road(side);
        const auto id = putSignalHead(d, {"", {"a", "a2"}, 30, program(d), {}});
        const auto before = drawn(d, id);
        History h; h.reset(d);
        // Only the far end moves, so the first 40 m of the Link are the same road.
        CHECK(h.execute("stretch", [](auto& m) { changeGeometry(m, "a", {{0, 0}, {40, 0}, {180, 30}}); }));
        CHECK(head(h.document(), id).position == 30);
        same(drawn(h.document(), id), before);
        same(run(h.document(), id), before);
    }
}
TEST(signal_position, a_split_before_at_or_after_a_head_keeps_its_world_point) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        for (const double at : {20.0, 30.0, 45.0}) { // head at lane station 30: split after, through, before it
            auto d = road(side);
            d.network.links[0].geometry = {{0, 0}, {100, 0}}; // straight: lane and reference stations agree
            const auto id = putSignalHead(d, {"", {"a", "a2"}, 30, program(d), {}});
            const auto before = drawn(d, id);
            History h; h.reset(d);
            std::string downstream;
            CHECK(h.execute("split", [&](auto& m) { downstream = splitLink(m, "a", at); }));
            const auto& moved = head(h.document(), id);
            // The owner: the upstream Link, the bridging Connector, or the new downstream Link.
            if (at > 30.1) CHECK(moved.lane.linkId == "a");
            else if (at < 29.9) CHECK(moved.lane.linkId == downstream);
            else CHECK(!moved.connectorId.empty());
            same(drawn(h.document(), id), before);
            same(run(h.document(), id), before);
        }
    }
}
TEST(signal_position, copying_a_link_moves_its_head_by_exactly_the_offset) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto d = road(side);
        const auto id = putSignalHead(d, {"", {"a", "a3"}, 55, program(d), {}});
        const auto before = drawn(d, id);
        const auto copies = duplicateObjects(d, {"a"}, {5, 200});
        const auto copy = std::find_if(d.network.signalHeads.begin(), d.network.signalHeads.end(),
                                       [&](const auto& h) { return h.id != id; });
        CHECK(copies.size() >= 1); CHECK(copy != d.network.signalHeads.end());
        CHECK(copy->lane.linkId != "a");
        same(drawn(d, copy->id), {before.x + 5, before.y + 200});
        same(run(d, copy->id), {before.x + 5, before.y + 200});
        same(drawn(d, id), before); // the original stays put
    }
}
#include "../src/project/run.hpp"
#include "../src/core/routes.hpp"
TEST(signal_position, a_red_head_past_a_cut_holds_traffic_at_the_drawn_line) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto d = road(side);
        d.network.links[0].geometry = {{0, 0}, {100, 0}};
        d.network.links[0].lanes = {{"a1", 3.5}}; // one lane, so every vehicle meets the head
        addConnector(d, {"a", "a1", 25}, {"b", "b1", 0});
        const auto id = putSignalHead(d, {"", {"a", "a1"}, 60, program(d), {}}); // red for the whole run
        putInput(d, {"", putRoute(d, {"", {"a"}}), "car", 600, 0, 50, {}});
        changeRunSettings(d, 60, 0.1);
        std::string segment;
        const auto line = run(d, id, &segment);
        CHECK(segment != "a1"); // the forcing: the head is on the section after the cut
        const auto s = compileDocument(d, test::root() / "data").scenario;
        auto state = createSimulation(s, 42);
        double furthest = -1;
        while (state.tick < totalTicks(s)) {
            state = stepSimulation(state);
            for (const auto& v : state.vehicles) {
                const auto at = locateVehicle(*state.scenario, v, *state.index);
                if (at.segmentId != segment) continue;
                furthest = std::max(furthest, at.position);
            }
        }
        CHECK(furthest > 0); // traffic reached the head's section
        const auto table = runtimeSections(d.network);
        const auto stop = rebaseHead(table, head(d, id)).position;
        CHECK(furthest <= stop + 1e-9);
        test::near(pointAlong(std::find_if(table.sections.begin(), table.sections.end(), [&](const auto& x) { return x.id == segment; })->geometry, stop).x,
                   line.x, 1e-9);
    }
}
