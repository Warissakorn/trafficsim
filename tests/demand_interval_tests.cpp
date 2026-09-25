#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
using namespace trafficsim;
// M2.2: an input's volume per period. Expanded at compile time into one core input per period,
// so the core, and every frozen fixture, is untouched.
namespace {
struct Built { ProjectDocument document; std::string route; };
Built road(int lanes) {
    Built b;
    const auto link = addLink(b.document, {{0, 0}, {400, 0}}, lanes, 3.5);
    b.route = putRoute(b.document, {"", {link}});
    changeRunSettings(b.document, 1800, 0.1);
    return b;
}
VehicleInput counted(const std::string& route) {
    VehicleInput input{"", route, "car", 0, 0, 0, {}, {}};
    input.intervals = {{0, 900, 0}, {900, 1800, 1200}};
    return input;
}
}
TEST(demand, intervals_expand_to_one_core_input_per_period) {
    auto b = road(1);
    const auto id = putInput(b.document, counted(b.route));
    // The scalars are derived: first start, last end, the mean over the span.
    const auto& stored = b.document.definition->inputs.front();
    CHECK(stored.startTime == 0); CHECK(stored.endTime == 1800); test::near(stored.vehiclesPerHour, 600);
    const auto inputs = compileDocument(b.document, test::root() / "data").scenario.inputs;
    CHECK(inputs.size() == 2);
    CHECK(inputs[0].id == id + "/int-1"); CHECK(inputs[1].id == id + "/int-2");
    CHECK(inputs[0].startTime == 0); CHECK(inputs[0].endTime == 900); CHECK(inputs[0].vehiclesPerHour == 0);
    CHECK(inputs[1].startTime == 900); CHECK(inputs[1].endTime == 1800); CHECK(inputs[1].vehiclesPerHour == 1200);
    for (const auto& i : inputs) CHECK(i.intervals.empty()); // The core never sees them.
}
TEST(demand, intervals_then_lanes_compose) {
    auto b = road(2);
    const auto id = putInput(b.document, counted(b.route));
    const auto inputs = compileDocument(b.document, test::root() / "data").scenario.inputs;
    CHECK(inputs.size() == 4);
    CHECK(inputs[2].id == id + "/int-2/lane-1"); CHECK(inputs[3].id == id + "/int-2/lane-2");
    CHECK(inputs[2].vehiclesPerHour == 600); CHECK(inputs[3].vehiclesPerHour == 600);
}
TEST(demand, a_zero_period_releases_nobody_and_the_next_does) {
    auto b = road(1);
    putInput(b.document, counted(b.route));
    const auto scenario = compileDocument(b.document, test::root() / "data").scenario;
    std::vector<double> scheduled;
    runSimulation(scenario, 42, [&](const SimEvent& e) {
        if (const auto* d = std::get_if<DepartedEvent>(&e)) scheduled.push_back(d->scheduledTime);
    }, false);
    CHECK(scheduled.size() > 100); // The forcing: the busy period really did release traffic.
    CHECK(std::all_of(scheduled.begin(), scheduled.end(), [](double t) { return t >= 900; }));
}
TEST(demand, one_interval_is_the_scalar_input_exactly) {
    auto a = road(1), b = road(1);
    VehicleInput scalar{"in", a.route, "car", 900, 0, 600, {}, {}};
    auto single = scalar; single.intervals = {{0, 600, 900}};
    putInput(a.document, scalar); putInput(b.document, single);
    const auto x = compileDocument(a.document, test::root() / "data").scenario;
    const auto y = compileDocument(b.document, test::root() / "data").scenario;
    CHECK(x.inputs == y.inputs); // Same id, same numbers: the same run, byte for byte.
}
TEST(demand, intervals_round_trip_and_stay_absent_when_unset) {
    auto b = road(1);
    putInput(b.document, counted(b.route));
    putInput(b.document, {"", b.route, "car", 300, 0, 600, {}, {}});
    const auto saved = documentJson(b.document);
    CHECK(saved["schemaVersion"] == 14);
    CHECK(saved["definition"]["inputs"][0].contains("intervals"));
    CHECK(!saved["definition"]["inputs"][1].contains("intervals"));
    const auto reopened = parseDocument(Json::parse(saved.dump()));
    CHECK(reopened.definition->inputs == b.document.definition->inputs);
    // The scalars in the file are a copy; the intervals win on read.
    auto edited = saved; edited["definition"]["inputs"][0]["vehiclesPerHour"] = 5;
    test::near(parseDocument(edited).definition->inputs.front().vehiclesPerHour, 600);
}
TEST(demand, overlapping_intervals_are_refused_and_roll_back) {
    History h; h.reset(road(1).document);
    const auto route = h.document().definition->routes.front().id;
    const auto before = documentJson(h.document());
    auto input = counted(route); input.intervals = {{0, 900, 100}, {600, 1200, 100}};
    // The forcing: these really do overlap, so the refusal is about that and nothing else.
    CHECK(input.intervals[1].startTime < input.intervals[0].endTime);
    test::throws([&] { h.execute("bad", [&](auto& d) { putInput(d, input); }); }, "INVALID_INTERVAL");
    CHECK(documentJson(h.document()) == before);
}
