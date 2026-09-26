#include "t_junction_support.hpp"
#include <fstream>
#include <sstream>
using namespace trafficsim;
using namespace tjunction;
// M3.2.7a, A26 of docs/M3_ACCEPTANCE.md: the T-junction fixture (§2). One drawing through the
// editor's commands, a crossing and a separate downstream merge on the minor turn, both driving
// sides, Yield and Stop, and a blocked receiving lane.
namespace {
const auto kFile = "data/projects/t-junction-priority.traffic.json";
Json committed() { std::ifstream f(test::root() / kFile); Json j; f >> j; return j; }
// Structure and strings exactly, numbers to 1e-9 m: Connector curves are computed geometry (as
// `fourleg` compares its file).
bool same(const Json& a, const Json& b) {
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it) if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) { for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false; return true; }
    return a == b;
}
fixture::TJunction variant(DrivingSide side, std::optional<StopMode> control = StopMode::yield, bool blocked = false) {
    fixture::TJunctionOptions o; o.side = side; o.control = control; o.blockedExit = blocked;
    return fixture::tJunction(o);
}
// Movement names are the Links' names, first and last (evaluationSpec).
const std::string kCrossing = "Minor approach → Major westbound", kNear = "Minor approach → Major eastbound";
}
TEST(tjunction, committed_file_is_the_builder_output) {
    const auto built = documentJson(fixture::tJunction().document);
    CHECK(same(committed(), built));
    auto moved = built; moved["network"]["links"][0]["geometry"][0]["x"] = built["network"]["links"][0]["geometry"][0]["x"].get<double>() + 0.01;
    CHECK(!same(committed(), moved)); // the comparison can fail
    CHECK(documentJson(parseDocument(committed())) == committed()); // and the file reopens exactly
}
TEST(tjunction, every_variant_resolves_to_a_crossing_and_separate_merges) {
    for (const auto side : {DrivingSide::right, DrivingSide::left})
        for (const auto control : {std::optional{StopMode::yield}, std::optional{StopMode::stop}}) {
            const auto t = variant(side, control);
            validateDocument(t.document);
            const auto r = resolveRightOfWay(t.document.network, runtimeSections(t.document.network), {5, 7});
            CHECK(r.issues.empty());
            const auto s = compiled(t);
            CHECK(s.conflictZones.size() == 3);
            const auto& crossing = zoneOn(s, t.crossingTurn);
            // The crossing turn meets the crossing first and the merge after it, separately: the
            // merge's zone starts past the crossing's exit on the same Connector.
            const auto merge = std::find_if(s.conflictZones.begin(), s.conflictZones.end(), [&](const auto& z) {
                return &z != &crossing && z.minor.segmentIds.front() == t.crossingTurn; });
            CHECK(merge != s.conflictZones.end());
            CHECK(merge->minor.entry > crossing.minor.exit);
            CHECK(crossing.major.segmentIds.front() != merge->major.segmentIds.front()); // two different streams
            CHECK(crossing.waitPosition < crossing.minor.entry);
            test::near(crossing.gapTime, 5, 0); test::near(crossing.headway, 7, 0);
            CHECK((crossing.control == ZoneControl::stop) == (control == StopMode::stop));
            // Sink clearance for the longest vehicle in the catalog, on every route through a zone.
            double longest = 0;
            for (const auto& type : s.vehicleTypes) longest = std::max(longest, type.length);
            CHECK(longest >= 12); // the forcing: the heavy vehicle is in the catalog
            for (const auto& z : s.conflictZones)
                for (const auto& route : s.routes) {
                    double start = 0, end = 0; bool through = false;
                    for (const auto& id : route.segmentIds) {
                        const auto& seg = *std::find_if(s.segments.begin(), s.segments.end(), [&](const auto& x) { return x.id == id; });
                        if (id == z.minor.segmentIds.back()) { through = true; start = end + z.minor.exit; }
                        end += seg.length;
                    }
                    if (through) CHECK(end - start >= longest);
                }
        }
}
TEST(tjunction, every_variant_runs_drains_and_never_puts_both_streams_in_the_crossing) {
    for (const auto side : {DrivingSide::right, DrivingSide::left})
        for (const auto control : {std::optional{StopMode::yield}, std::optional{StopMode::stop}}) {
            const auto t = variant(side, control);
            const auto r = run(t);
            for (const auto& m : r.report.movements) CHECK(m.vehicles > 0);
            CHECK(r.report.pending == 0); CHECK(r.report.active == 0); // finite demand drained
            CHECK(!r.bothSidesInside);
            CHECK(r.majorClamps == 0); // the major road never brakes hard for the minor one
            CHECK(r.report.queues.size() == 1 && r.report.queues.front().name == "Minor approach queue");
            CHECK(r.report.queues.front().maxLength > 0);
        }
}
TEST(tjunction, the_mirror_runs_the_same_junction_with_the_other_hand) {
    // Mirrored geometry and the same demand: the same trips, on the same seed.
    const auto right = run(variant(DrivingSide::right)), left = run(variant(DrivingSide::left));
    CHECK(right.report.movements == left.report.movements);
    CHECK(right.report.queues == left.report.queues);
}
TEST(tjunction, stop_delays_the_minor_road_more_than_yield) {
    const auto yield = run(variant(DrivingSide::right)), stop = run(variant(DrivingSide::right, StopMode::stop));
    for (const auto& name : {kCrossing, kNear}) {
        CHECK(movement(yield.report, name).vehicles > 0); // the forcing: minor trips completed in both
        CHECK(*movement(stop.report, name).meanDelay > *movement(yield.report, name).meanDelay);
    }
    // The major road is not what changed.
    const auto major = "Major eastbound → Major eastbound";
    CHECK(movement(stop.report, major).vehicles == movement(yield.report, major).vehicles);
}
TEST(tjunction, a_full_receiving_lane_keeps_the_crossing_turn_out_of_the_crossing) {
    const auto t = variant(DrivingSide::right, StopMode::yield, true);
    const auto r = run(t);
    // The forcing: the far lane really filled back past the merge -- nothing reached the end of it.
    CHECK(movement(r.report, "Major westbound → Major westbound").vehicles == 0);
    CHECK(movement(r.report, kCrossing).vehicles == 0);
    CHECK(!r.bothSidesInside);
    // A turn with nowhere to go waits at its line, so the stream it would cross keeps moving. Two
    // guards hold it there, each enough alone: the crossing and the merge are one chain (A15; the
    // 6.8 m between them is less than the heavy vehicle's waiting room), so it waits at the
    // crossing line for both; and past a chain's end a standing leader must leave room for the
    // whole vehicle. Disabling both, and only both, fails this test (M3.2.7a, D66).
    const auto open = run(variant(DrivingSide::right));
    CHECK(movement(r.report, "Major eastbound → Major eastbound").vehicles ==
          movement(open.report, "Major eastbound → Major eastbound").vehicles);
    CHECK(r.majorClamps == 0);
}
#include "../tools/t_junction_sweep.hpp"
TEST(tjunction, the_archived_sweep_metadata_still_describes_the_fixture) {
    // M3.2.7b: docs/evidence/m3.2.7-sweep-metadata.json was committed before the sweep ran. Its
    // `build` line records where it ran and is not compared; everything else must still hold, or
    // the archived rows describe a drawing that no longer exists.
    std::ifstream f(test::root() / "docs/evidence/m3.2.7-sweep-metadata.json");
    CHECK(f.good());
    auto archived = nlohmann::ordered_json::parse(f);
    CHECK(archived.contains("build")); // the forcing: the file really is the tool's output
    archived.erase("build");
    CHECK(archived == sweep::fixtureMetadata(test::root()));
    // A changed catalog would be caught: the hash reads the file, not its name.
    CHECK(sweep::fnv1a(test::root() / "data/vehicle-types/car.json") != sweep::fnv1a(test::root() / "data/vehicle-types/heavy-vehicle.json"));
}
TEST(tjunction, commitment_removes_the_minor_clamps_the_sweep_archived_and_leaves_the_major_road_alone) {
    // M3.2.8a (D69), measured against the M3.2.7b rows at the base rule (gapTime 5 s, headway 7 m):
    // every clamp there was a minor driver too close to stop when its line closed.
    std::ifstream f(test::root() / "docs/evidence/m3.2.7-sweep.csv");
    std::string line; std::getline(f, line);
    int archived = 0;
    while (std::getline(f, line)) {
        std::vector<std::string> cells; std::stringstream row(line);
        for (std::string cell; std::getline(row, cell, ',');) cells.push_back(cell);
        if (cells[1] == "5" && cells[2] == "7") archived += std::stoi(cells.back());
    }
    CHECK(archived > 0); // the forcing: the archive holds clamps to remove
    int minor = 0;
    for (const auto seed : sweep::kSeeds) {
        fixture::TJunctionOptions o; o.gapTime = 5; o.headway = 7;
        const auto r = run(fixture::tJunction(o), seed);
        CHECK(r.report.pending == 0 && r.report.active == 0);
        CHECK(r.majorClamps == 0); CHECK(!r.bothSidesInside);
        minor += r.minorClamps;
    }
    CHECK(minor < archived);
}
