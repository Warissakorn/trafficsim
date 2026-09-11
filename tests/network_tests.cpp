#include "test.hpp"
#include <algorithm>
#include <limits>

using namespace trafficsim;
TEST(network, compiled_lengths_and_midlink_heads) {
    const auto loaded = test::demo();
    CHECK(validateNetwork(loaded.network).empty()); CHECK(loaded.scenario.segments.size() == 6);
    for (const auto& s : loaded.scenario.segments) {
        if (s.id == "west-1") { CHECK(s.length == 150); CHECK(s.next == std::vector<std::string>{"west-east"}); }
        if (s.id == "west-east") { CHECK(s.length == 20); CHECK(s.next == std::vector<std::string>{"east-1"}); }
    }
    CHECK(loaded.scenario.signalHeads[0].position == 148);
}
TEST(network, interpolation) {
    const std::vector<Point> p{{0, 0}, {3, 4}, {3, 14}};
    CHECK(polylineLength(p) == 15); CHECK(pointAlong(p, 2.5) == Point{1.5, 2});
    CHECK(pointAlong(p, 10) == Point{3, 9}); CHECK(pointAlong(p, -1) == p.front());
    CHECK(pointAlong(p, 100) == p.back());
    test::throws([&] { pointAlong(p, std::numeric_limits<double>::quiet_NaN()); });
    test::throws([&] { pointAlong({}, 0); });
}
TEST(network, driving_side_offsets) {
    const Link link{"link", {{0, 0}, {10, 0}}, {{"curb", 4}, {"inner", 4}}};
    CHECK(laneGeometry(link, "curb", DrivingSide::left) == std::vector<Point>({{0, 2}, {10, 2}}));
    CHECK(laneGeometry(link, "curb", DrivingSide::right) == std::vector<Point>({{0, -2}, {10, -2}}));
    Network network{"two-lane", DrivingSide::right, {link}, {}, {}};
    auto definition = test::straight(); definition.inputs.clear(); definition.routes = {{"r", {"curb"}}};
    const auto compiled = compileScenario(network, definition);
    CHECK(compiled.segments[0].length == 10); CHECK(compiled.segments[1].length == 10);
}
TEST(network, invalid_geometry_ids_and_references) {
    auto n = test::demo().network;
    n.links.push_back(n.links[0]); n.connectors.resize(1);
    n.connectors[0].to = {"missing", "bad"}; n.connectors[0].geometry = {{0, 0}, {0, 0}};
    const auto issues = validateNetwork(n);
    for (const auto* code : {"DUPLICATE_ID", "INVALID_GEOMETRY", "UNKNOWN_LANE", "DISCONNECTED_GEOMETRY"})
        CHECK(std::any_of(issues.begin(), issues.end(), [&](const auto& issue) { return issue.code == code; }));
    test::throws([&] { compileScenario(n, test::straight()); }, "UNKNOWN_LANE");
}
TEST(network, invalid_width_position_and_side) {
    auto n = test::demo().network;
    n.links[0].lanes[0].width = std::numeric_limits<double>::quiet_NaN(); n.signalHeads[0].position = -1;
    test::throws([&] { assertValidNetwork(n); }, "INVALID_WIDTH");
    test::throws([&] { assertValidNetwork(n); }, "INVALID_POSITION");
    n.drivingSide = static_cast<DrivingSide>(99);
    test::throws([&] { assertValidNetwork(n); }, "INVALID_DRIVING_SIDE");
}
TEST(network, compile_detaches_authoring_data) {
    auto loaded = test::demo();
    const auto compiled = compileScenario(loaded.network, loaded.scenario);
    loaded.scenario.inputs[0].vehiclesPerHour = 1; loaded.network.links[0].geometry[0].x = 999;
    CHECK(compiled.inputs[0].vehiclesPerHour != 1); CHECK(compiled.segments[0].length == 150);
}
