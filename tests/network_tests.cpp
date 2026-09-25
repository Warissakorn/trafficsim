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
    Network network{"two-lane", DrivingSide::right, {link}, {}, {}, {}};
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
// A bend used to pinch the carriageway: each edge was offset by the full width along the
// average normal instead of the miter, leaving it width/2*cos(theta/2) from the centreline.
TEST(network, bends_keep_their_full_carriageway_width) {
    const Link link{"bent",{{0,0},{40,0},{40,40}},{{"l1",3.5},{"l2",3.5},{"l3",3.5}}};
    // The forcing: these two legs really do turn a right angle.
    const Point first{1,0},second{0,1};
    CHECK(first.x*second.x+first.y*second.y==0);
    for(auto side:{DrivingSide::left,DrivingSide::right}) {
        const auto centre=linkCentreline(link,side);
        const auto left=laneBoundaryGeometry(link,0,side),right=laneBoundaryGeometry(link,3,side);
        // Measured across each leg, the road is 10.5 m wide at the corner, not 7.4 m.
        for(const auto* direction:{&first,&second}) {
            const Point across{-direction->y,direction->x};
            const double width=(left[1].x-right[1].x)*across.x+(left[1].y-right[1].y)*across.y;
            test::near(std::abs(width),10.5,1e-9);
        }
        // Every lane keeps its own width across the corner, and the centreline stays central.
        for(std::size_t boundary=0;boundary<3;++boundary) {
            const auto inner=laneBoundaryGeometry(link,boundary,side),outer=laneBoundaryGeometry(link,boundary+1,side);
            test::near(std::hypot(inner[1].x-outer[1].x,inner[1].y-outer[1].y),3.5*std::sqrt(2.),1e-9);
        }
        test::near(centre[1].x,(left[1].x+right[1].x)/2,1e-12);
        test::near(centre[1].y,(left[1].y+right[1].y)/2,1e-12);
    }
    // A straight polyline is untouched by the miter, so no existing geometry moved.
    const Link straight{"straight",{{0,0},{10,0}},{{"s1",4}}};
    CHECK(laneGeometry(straight,"s1",DrivingSide::left)==std::vector<Point>({{0,0},{10,0}}));
}
TEST(network, a_hairpin_is_clamped_instead_of_spiking) {
    // The forcing: this vertex doubles back on itself, where an unclamped miter diverges.
    const std::vector<Point> hairpin{{0,0},{10,0},{0,0.001}};
    const auto offsets=offsetGeometry(hairpin,3.5);
    CHECK(offsets.size()==3);
    for(const auto& p:offsets)CHECK(std::isfinite(p.x) && std::isfinite(p.y));
    // Four times the offset is the documented miter limit.
    test::near(std::hypot(offsets[1].x-hairpin[1].x,offsets[1].y-hairpin[1].y),4*3.5,1e-9);
}
