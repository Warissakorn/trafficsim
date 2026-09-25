#include "test.hpp"
#include "right_of_way_fixture.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/eval/movement.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace trafficsim;
using namespace rowfixture;
// M3.2.6b, A22 and A23 of docs/M3_ACCEPTANCE.md: a queue counter is a set of places on the road,
// independent of signal heads; one mechanism measures both the counters derived from heads and the
// ones an author places, so the two agree and an approach is never listed twice.
namespace {
constexpr double kmh = 1 / 3.6;
ProjectDocument fourLeg() {
    std::ifstream f(test::root() / "data/projects/four-leg-signalised.traffic.json");
    return parseDocument(Json::parse(f));
}
MovementReport report(const ProjectDocument& d, std::uint32_t seed = 42) {
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    MovementAccumulator m(evaluationSpec(d, snapshot, data));
    auto state = createSimulation(snapshot.scenario, seed);
    m.observe(state);
    while (state.tick < totalTicks(snapshot.scenario)) { state = stepSimulation(state); m.observe(state); }
    return m.report(state);
}
const QueueRow* row(const MovementReport& r, const std::string& name) {
    for (const auto& q : r.queues) if (q.name == name) return &q;
    return nullptr;
}
}
TEST(queue_counter, a22_a_known_standing_queue_at_a_line_with_no_signal_head) {
    auto s = test::straight();
    CHECK(s.signalHeads.empty()); // the forcing: nothing here is a signal
    EvaluationSpec spec;
    spec.counters = {{"approach", {{"road", 100}}}};
    spec.queue = {5 * kmh, 10 * kmh, 20};
    // Fronts 5 m and 12 m short of the line, standing; a third 60 m back, past the 20 m gap.
    const auto state = test::withVehicles(s, {test::vehicle(1, 95, 0), test::vehicle(2, 88, 0), test::vehicle(3, 40, 0)});
    MovementAccumulator m(spec);
    m.observe(state);
    const auto length = 12 + s.vehicleTypes.front().length; // to the second vehicle's rear
    test::near(m.report(state).queues.front().maxLength, length, 1e-9);
}
TEST(queue_counter, a23_an_authored_counter_over_an_approachs_heads_replaces_its_row_with_the_same_numbers) {
    auto d = fourLeg();
    const auto before = report(d);
    const auto& link = *std::find_if(d.network.links.begin(), d.network.links.end(),
                                     [&](const auto& l) { return l.id == d.network.signalHeads.front().lane.linkId; });
    const auto name = link.name.empty() ? link.id : link.name;
    const auto* derived = row(before, name);
    CHECK(derived != nullptr); // the forcing: that approach has a derived row
    AuthoredQueueCounter counter{"", "Counted west", {}};
    for (const auto& h : d.network.signalHeads)
        if (h.connectorId.empty() && h.lane.linkId == link.id) counter.lines.push_back({h.id, std::nullopt});
    CHECK(!counter.lines.empty());
    putQueueCounter(d, counter);
    validateDocument(d);
    const auto after = report(d);
    CHECK(after.queues.size() == before.queues.size()); // no duplicate approach row
    CHECK(row(after, name) == nullptr);
    const auto* authored = row(after, "Counted west");
    CHECK(authored != nullptr);
    test::near(authored->meanLength, derived->meanLength, 0); test::near(authored->maxLength, derived->maxLength, 0);
    CHECK(after.movements == before.movements); // a counter moves no vehicle
    CHECK(compileDocument(d, test::root() / "data").scenario == compileDocument(fourLeg(), test::root() / "data").scenario);
    // Shared CLI/editor output names the authored counter.
    const auto json = movementJson(after);
    CHECK(std::any_of(json["queues"].begin(), json["queues"].end(), [](const auto& q) { return q["approach"] == "Counted west"; }));
}
TEST(queue_counter, a22_a_counter_at_a_stop_line_and_the_same_explicit_point_agree) {
    // A Stop with no signal anywhere: its waiting line, referenced or given as a point.
    ProjectDocument d;
    const auto major = addLink(d, {{0, 0}, {200, 0}}, 1, 3.5);
    const auto minor = addLink(d, {{100, -100}, {100, 100}}, 1, 3.5);
    const auto area = addCrossingAreas(d, major, minor, minor, kDefaults).front();
    setAreaControl(d, area, StopMode::stop);
    putInput(d, {"", putRoute(d, {"", {major}}), "car", 900, 0, 540, {}});
    putInput(d, {"", putRoute(d, {"", {minor}}), "car", 500, 0, 540, {}});
    changeRunSettings(d, 600, 0.1);
    CHECK(d.network.signalHeads.empty());
    const auto& line = *std::find_if(d.network.rightOfWay.waitingLines.begin(), d.network.rightOfWay.waitingLines.end(),
        [&](const auto& w) { return w.id == d.network.rightOfWay.stopControls.front().waitingLineId; });
    putQueueCounter(d, {"", "at the line", {{line.id, std::nullopt}}});
    putQueueCounter(d, {"", "at the point", {{"", line.point}}});
    validateDocument(d);
    const auto r = report(d);
    CHECK(r.queues.size() == 2);
    CHECK(row(r, "at the line")->maxLength > 0); // a Stop line builds a queue
    CHECK(*row(r, "at the line") == QueueRow{"at the line", row(r, "at the point")->meanLength, row(r, "at the point")->maxLength});
}
TEST(queue_counter, schema_16_round_trips_and_bad_lines_are_refused) {
    auto d = fourLeg();
    const auto head = d.network.signalHeads.front().id;
    putQueueCounter(d, {"", "west", {{head, std::nullopt}, {"", ControlPoint{{d.network.links[0].id, d.network.links[0].lanes[0].id, "", "", ""}, 10}}}});
    validateDocument(d);
    auto file = documentJson(d);
    CHECK(file["schemaVersion"] == 16);
    CHECK(parseDocument(file).network == d.network);
    auto older = file; older["schemaVersion"] = 15;
    test::throws([&] { parseDocument(older); }, "");
    CHECK(!documentJson(fourLeg())["network"].contains("queueCounters")); // absent when unused
    const auto issues = [&](MeasurementLine l) {
        auto bad = d; bad.network.queueCounters.front().lines = {std::move(l)};
        return validateNetwork(bad.network);
    };
    CHECK(has(issues({"", std::nullopt}), "INVALID_MEASUREMENT_LINE"));                       // neither
    CHECK(has(issues({head, ControlPoint{{"x", "y", "", "", ""}, 1}}), "INVALID_MEASUREMENT_LINE")); // both
    CHECK(has(issues({"nothing", std::nullopt}), "UNKNOWN_MEASUREMENT_REFERENCE"));
}
TEST(queue_counter, a_counter_line_follows_its_head_its_road_and_a_split) {
    auto d = fourLeg();
    const auto& h = d.network.signalHeads.front();
    const auto link = h.lane.linkId;
    const auto lane = h.lane.laneId;
    putQueueCounter(d, {"", "by head", {{h.id, std::nullopt}}});
    putQueueCounter(d, {"", "by point", {{"", ControlPoint{{link, lane, "", "", ""}, 5}}}});
    // Deleting the head takes its line, and with it the counter that measured nothing else.
    auto noHead = d; deleteSignalHead(noHead, h.id);
    CHECK(noHead.network.queueCounters.size() == 1 && noHead.network.queueCounters.front().name == "by point");
    validateDocument(noHead);
    // Deleting the road takes both.
    auto noRoad = d; deleteLink(noRoad, link);
    CHECK(noRoad.network.queueCounters.empty()); validateDocument(noRoad);
    // A lane a counter measures on cannot be removed, as a head's cannot.
    ProjectDocument two;
    const auto road = addLink(two, {{0, 0}, {50, 0}}, 2, 3.5);
    putQueueCounter(two, {"", "", {{"", ControlPoint{{road, two.network.links[0].lanes[1].id, "", "", ""}, 10}}}});
    test::throws([&] { changeLanes(two, road, {3.5}); }, "EDIT_REFERENCED_LANE");
}
TEST(queue_counter, a_split_moves_an_explicit_point_onto_the_link_that_now_holds_it) {
    ProjectDocument d;
    const auto link = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto lane = d.network.links.front().lanes.front().id;
    const auto id = putQueueCounter(d, {"", "", {{"", ControlPoint{{link, lane, "", "", ""}, 70}}}});
    const auto before = pointAlong(d.network.links.front().geometry, 70);
    History h; h.reset(d);
    std::string downstream;
    CHECK(h.execute("split", [&](auto& m) { downstream = splitLink(m, link, 40); }));
    const auto& point = *h.document().network.queueCounters.front().lines.front().point;
    CHECK(point.path.linkId == downstream); // the forcing: the point now lies past the cut
    const auto& moved = *std::find_if(h.document().network.links.begin(), h.document().network.links.end(), [&](const auto& l) { return l.id == downstream; });
    const auto after = pointAlong(moved.geometry, point.station);
    test::near(after.x, before.x, 1e-9); test::near(after.y, before.y, 1e-9);
    // Through the cut itself is refused, as a waiting line's is.
    test::throws([&] { h.execute("x", [&](auto& m) { splitLink(m, downstream, point.station); }); }, "EDIT_SPLIT_CONTROL");
    (void)id;
}
