#include "test.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/eval/movement.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>
using namespace trafficsim;
// M5.4 (D133): travel-time sections, rows TT1-TT10 of docs/reference/TRAVEL_TIME_SECTIONS.md §6.
namespace {
Json fourLegJson() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return j;
}
// A 400 m road and a section between two places on it.
Scenario road() { auto s = test::straight(); s.segments = {{"road", 400, {}}}; s.duration = 60; return s; }
EvaluationSpec section(double from, double to, double warmup = 0, std::optional<double> end = std::nullopt) {
    EvaluationSpec spec; spec.warmup = warmup; spec.end = end;
    spec.sections = {{"s", {{"road", from}}, {{"road", to}}}};
    return spec;
}
// Runs a placed fleet to the end, observing every state, and returns the report.
MovementReport observed(SimState state, const EvaluationSpec& spec, std::vector<ArrivedEvent>* arrivals = nullptr) {
    MovementAccumulator m(spec); m.observe(state);
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(std::move(state)); m.observe(state);
        if (arrivals) for (const auto& e : state.events) if (const auto* a = std::get_if<ArrivedEvent>(&e)) arrivals->push_back(*a);
    }
    return m.report(state);
}
ProjectDocument withSection() {
    auto d = parseDocument(fourLegJson());
    putTravelTimeSection(d, {"", "West through", {"link-1", 20}, {"link-20", 50}});
    validateDocument(d);
    return d;
}
}
TEST(traveltime, lone_vehicle_at_desired_speed_has_no_delay) { // TT1
    // Placed at its desired speed (15 m/s) on a free road: it never accelerates, so the 100 m
    // section takes exactly 100/15 s, whatever the ticks.
    const auto r = observed(test::withVehicles(road(), {test::vehicle(1, 0, 15)}), section(200, 300));
    CHECK(r.sections.size() == 1); CHECK(r.sections[0].vehicles == 1);
    test::near(*r.sections[0].meanTravelTime, 100.0 / 15, 1e-9);
    test::near(*r.sections[0].meanDelay, 0, 1e-9);
    CHECK(r.sections[0].unfinished == 0);
}
TEST(traveltime, a_whole_route_section_is_the_trip) { // TT2
    std::vector<ArrivedEvent> arrivals;
    const auto r = observed(test::withVehicles(road(), {test::vehicle(1, 0, 0)}), section(0, 400), &arrivals);
    CHECK(arrivals.size() == 1); CHECK(r.sections[0].vehicles == 1);
    const auto& trip = arrivals[0];
    // Same free-flow time, travel time within one step of the trip's (the trip ends on a tick).
    test::near(*r.sections[0].meanTravelTime - *r.sections[0].meanDelay, trip.freeFlowTime, 1e-9);
    CHECK(*r.sections[0].meanTravelTime <= trip.travelTime + 1e-9);
    CHECK(*r.sections[0].meanTravelTime > trip.travelTime - 0.1);
}
TEST(traveltime, a_red_of_known_length_sets_the_delay) { // TT3
    // Red until 30 s at 200 m; the section is 100-300 m. Reaching the line no earlier than 30 s
    // and then moving no faster than 15 m/s, the vehicle cannot lose less than the hold below.
    auto s = road();
    s.signalPrograms = {{"p", 0, {{30, SignalColor::red}, {600, SignalColor::green}}}};
    s.signalHeads = {{"h", "road", 200, "p"}};
    const auto r = observed(test::withVehicles(s, {test::vehicle(1, 0, 15)}), section(100, 300));
    CHECK(r.sections[0].vehicles == 1);
    const double start = 100.0 / 15, hold = 30 - start - 100.0 / 15;
    const double delay = *r.sections[0].meanDelay;
    CHECK(delay >= hold - 1e-9);
    // Stopping and restarting cost more on top (here 3.3 s); constant-rate braking and acceleration
    // would cost about v/2b + v/2a, and the bound allows v/a_max for the model's softer approach.
    const auto& car = s.vehicleTypes.front();
    const double loss = 15 / car.maxAcceleration;
    CHECK(delay <= hold + loss);
}
TEST(traveltime, a_trip_counts_by_its_end_crossing) { // TT4
    // The lone vehicle of TT1 crosses the end line at exactly 300/15 = 20 s.
    const auto run = [](double warmup, std::optional<double> end) {
        return observed(test::withVehicles(road(), {test::vehicle(1, 0, 15)}), section(200, 300, warmup, end)).sections[0].vehicles;
    };
    CHECK(run(20, std::nullopt) == 1); CHECK(run(20.1, std::nullopt) == 0);
    CHECK(run(0, 20) == 1); CHECK(run(0, 19.9) == 0);
    // A start before warm-up does not matter: only the end crossing is tested.
    CHECK(run(15, std::nullopt) == 1);
}
TEST(traveltime, a_lane_change_inside_the_section_is_timed_once) { // TT5
    // Two parallel lanes as two routes; the section spans both at 10 m and 90 m. The vehicle
    // crosses the start on lane a, changes to lane b (distance mapped), crosses the end there.
    auto s = test::straight(); s.inputs.clear();
    s.segments = {{"a", 100, {}}, {"b", 100, {}}}; s.routes = {{"ra", {"a"}}, {"rb", {"b"}}};
    EvaluationSpec spec; spec.sections = {{"s", {{"a", 10}, {"b", 10}}, {{"a", 90}, {"b", 90}}}};
    MovementAccumulator m(spec);
    auto state = createSimulation(s, 1);
    const auto at = [&](double time, std::uint32_t route, double distance) {
        state.time = time; state.events.clear();
        Vehicle v; v.id = 1; v.routeIndex = route; v.distance = distance; v.speed = 10; v.desiredSpeed = 20;
        state.vehicles = {v}; m.observe(state);
    };
    const auto ra = state.scenario->routes[0].id == "ra" ? 0u : 1u, rb = 1 - ra;
    at(0, ra, 5); at(1, ra, 15); at(2, rb, 50); at(3, rb, 95);
    const auto r = m.report(state);
    CHECK(r.sections[0].vehicles == 1);
    // Start at 0.5 s (5 of 10 m), end at 2 + 40/45 s; free flow 80 m / 20 m/s = 4 s.
    test::near(*r.sections[0].meanTravelTime, 2 + 40.0 / 45 - 0.5, 1e-12);
    test::near(*r.sections[0].meanDelay, 0, 1e-12); // 2.39 s is under the 4 s free-flow time
}
TEST(traveltime, a_section_no_route_passes_in_order_is_an_empty_row) { // TT6
    const auto r = observed(test::withVehicles(road(), {test::vehicle(1, 0, 15)}), section(300, 200));
    CHECK(r.sections.size() == 1); CHECK(r.sections[0].vehicles == 0); CHECK(!r.sections[0].meanDelay);
    CHECK(r.sections[0].unfinished == 0);
    auto d = parseDocument(fourLegJson());
    test::throws([&] { putTravelTimeSection(d, {"", "", {"link-1", 50}, {"link-1", 50}}); validateDocument(d); }, "INVALID_SECTION_ORDER");
}
TEST(traveltime, a_vehicle_held_inside_is_unfinished) { // TT7
    auto s = road();
    s.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 200, "p"}};
    const auto r = observed(test::withVehicles(s, {test::vehicle(1, 0, 15)}), section(100, 300));
    CHECK(r.sections[0].vehicles == 0); CHECK(r.sections[0].unfinished == 1);
    CHECK(r.active == 1);
}
TEST(traveltime, schema23_codec) { // TT8
    const auto plain = parseDocument(fourLegJson());
    const auto d = withSection();
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 23);
    CHECK(j["network"]["travelTimeSections"][0] == Json({{"id", d.network.travelTimeSections[0].id}, {"name", "West through"},
        {"start", {{"linkId", "link-1"}, {"station", 20.0}}}, {"end", {{"linkId", "link-20"}, {"station", 50.0}}}}));
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d); CHECK(documentJson(reopened) == j);
    // Without a section the file keeps its schema and keys.
    CHECK(documentJson(plain)["schemaVersion"] == 17); CHECK(!documentJson(plain)["network"].contains("travelTimeSections"));
    auto older = j; older["schemaVersion"] = 22;
    test::throws([&] { parseDocument(older); }, "network.travelTimeSections");
    auto extra = j; extra["network"]["travelTimeSections"][0]["controlType"] = "signal";
    test::throws([&] { parseDocument(extra); }, "EDIT_UNSUPPORTED_FIELD");
    auto lineKey = j; lineKey["network"]["travelTimeSections"][0]["start"]["laneId"] = "lane-2";
    test::throws([&] { parseDocument(lineKey); }, "EDIT_UNSUPPORTED_FIELD");
    auto unknown = j; unknown["network"]["travelTimeSections"][0]["end"]["linkId"] = "link-404";
    test::throws([&] { parseDocument(unknown); }, "UNKNOWN_SECTION_LINK");
    auto negative = j; negative["network"]["travelTimeSections"][0]["start"]["station"] = -1;
    test::throws([&] { parseDocument(negative); }, "INVALID_POSITION");
    auto clash = j; clash["network"]["travelTimeSections"][0]["id"] = "link-1";
    test::throws([&] { parseDocument(clash); }, "DUPLICATE_ID");
    auto newer = j; newer["schemaVersion"] = 26;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
}
TEST(traveltime, commands_and_cascades) { // TT9
    History h; h.reset(parseDocument(fourLegJson()));
    std::string id;
    CHECK(h.execute("add", [&](auto& d) { id = putTravelTimeSection(d, {"", "", {"link-1", 20}, {"link-20", 50}}); }));
    CHECK(id.starts_with("section-")); CHECK(h.document().network.travelTimeSections.size() == 1);
    CHECK(h.execute("rename", [&](auto& d) { auto s = d.network.travelTimeSections[0]; s.name = "W"; putTravelTimeSection(d, s); }));
    CHECK(h.document().network.travelTimeSections[0].name == "W");
    h.undo(); CHECK(h.document().network.travelTimeSections[0].name.empty());
    // A refused edit leaves the document as it was.
    const auto before = h.document();
    test::throws([&] { h.execute("bad", [&](auto& d) { putTravelTimeSection(d, {"", "", {"link-404", 1}, {"link-20", 5}}); }); }, "UNKNOWN_SECTION_LINK");
    CHECK(h.document() == before);
    // Reversing a Link a section names would flip its stations: refused.
    test::throws([&] { h.execute("reverse", [&](auto& d) { reverseLink(d, "link-20"); }); }, "EDIT_REFERENCED_LINK");
    // Splitting moves a line beyond the cut to the downstream child; a line in the cut refuses.
    std::string downstream;
    CHECK(h.execute("split", [&](auto& d) { downstream = splitLink(d, "link-20", 30, false); }));
    const auto& moved = h.document().network.travelTimeSections[0].end;
    CHECK(moved.linkId == downstream); test::near(moved.station, 50 - 30.1, 1e-9);
    test::throws([&] { h.execute("split", [&](auto& d) { splitLink(d, "link-1", 20.05, false); }); }, "EDIT_SPLIT_CONTROL");
    // Deleting either Link deletes the section; Undo brings it back.
    CHECK(h.execute("delete", [&](auto& d) { deleteLink(d, "link-1"); }));
    CHECK(h.document().network.travelTimeSections.empty());
    h.undo(); CHECK(h.document().network.travelTimeSections.size() == 1);
    CHECK(h.execute("remove", [&](auto& d) { deleteTravelTimeSection(d, id); }));
    CHECK(h.document().network.travelTimeSections.empty());
    test::throws([&] { h.execute("remove", [&](auto& d) { deleteTravelTimeSection(d, id); }); }, "EDIT_UNKNOWN_OBJECT");
}
TEST(traveltime, a_project_section_runs_and_writes_rows) { // TT10
    const auto data = test::root() / "data";
    const auto d = withSection();
    const auto snapshot = compileDocument(d, data);
    const auto spec = evaluationSpec(d, snapshot, data);
    CHECK(spec.sections.size() == 1); CHECK(spec.sections[0].name == "West through");
    CHECK(spec.sections[0].start.size() == 2); CHECK(spec.sections[0].end.size() == 2); // both lanes
    const auto r = runSeed(snapshot.scenario, spec, 42).report;
    CHECK(r.sections[0].vehicles > 0); CHECK(*r.sections[0].meanDelay >= 0);
    CHECK(movementJson(r)["sections"][0]["section"] == "West through");
    CHECK(movementCsv(r).find("\nsection,vehicles,meanTravelTime_s,meanDelay_s,unfinished,controlType,los\n\"West through\",") != std::string::npos);
    // The movement rows are exactly those of the same project without the section.
    const auto plain = parseDocument(fourLegJson());
    const auto plainSnapshot = compileDocument(plain, data);
    const auto without = runSeed(plainSnapshot.scenario, evaluationSpec(plain, plainSnapshot, data), 42).report;
    auto stripped = r; stripped.sections.clear();
    CHECK(stripped == without);
    CHECK(!movementJson(without).contains("sections"));
    CHECK(movementCsv(without).find("section,") == std::string::npos);
}
TEST(traveltime, batches_aggregate_sections) { // TT10
    SeedRun a, b; a.seed = 1; b.seed = 2;
    a.report.sections = {{"s", 10, 30.0, 10.0, 1}}; b.report.sections = {{"s", 20, 34.0, 14.0, 3}};
    const auto r = aggregate({b, a});
    CHECK(r.sections.size() == 1); CHECK(r.sections[0].meanDelay.n == 2);
    test::near(*r.sections[0].meanDelay.mean, 12); test::near(*r.sections[0].meanTravelTime.mean, 32);
    test::near(*r.sections[0].vehicles.mean, 15); test::near(*r.sections[0].unfinished.mean, 2);
    const std::vector<SeedRun> runs{a, b};
    CHECK(batchCsv(r, runs).find("\nsection,n,meanDelay_s,ci95_s,sd_s,vehicles_mean,meanTravelTime_s,unfinished_mean,controlType,los\n\"s\",2,12.00,") != std::string::npos);
    CHECK(batchJson(r, runs)["sections"][0]["meanDelay"]["mean"] == 12.0);
    auto other = b; other.report.sections[0].name = "t";
    test::throws([&] { aggregate({a, other}); }, "different");
    const auto none = aggregate({SeedRun{}});
    CHECK(!batchJson(none, {}).contains("sections")); CHECK(batchCsv(none, {}).find("section,") == std::string::npos);
}
