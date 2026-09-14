#include "test.hpp"
#include <fstream>

using namespace trafficsim;
namespace {
void compare(const Json& actual, const Json& expected, const std::string& path = "root") {
    if (actual.is_number() && expected.is_number()) {
        if (actual.is_number_integer() && expected.is_number_integer()) CHECK(actual == expected);
        else try { test::near(actual.get<double>(), expected.get<double>()); }
             catch (const std::exception&) { throw std::runtime_error("Baseline numeric difference at " + path); }
    } else if (expected.is_object()) {
        CHECK(actual.is_object()); CHECK(actual.size() == expected.size());
        for (const auto& [key, value] : expected.items()) compare(actual.at(key), value, path + "." + key);
    } else if (expected.is_array()) {
        CHECK(actual.is_array()); CHECK(actual.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) compare(actual[i], expected[i], path + "[" + std::to_string(i) + "]");
    } else if (actual != expected) throw std::runtime_error("Baseline value difference at " + path);
}
// Per-tick trajectory characterization. The frozen TypeScript baselines deliberately exclude
// MovedEvent, so positions between checkpoints are otherwise unpinned. These are weighted means
// (not sums) so magnitudes stay physical and the existing 1e-7 tolerance applies unchanged.
// They record what this engine currently does; they are not a fidelity claim.
struct TrajectoryDigest {
    std::uint64_t count{};
    double position{}, speed{}, acceleration{}, orderWeighted{}, segmentWeighted{}, vehicleWeighted{};
};
void accumulate(TrajectoryDigest& digest, const MovedEvent& moved, std::uint64_t index) {
    std::uint64_t segmentHash = 0;
    for (const char c : moved.segmentId) segmentHash = segmentHash * 131 + static_cast<unsigned char>(c);
    ++digest.count;
    digest.position += moved.position;
    digest.speed += moved.speed;
    digest.acceleration += moved.acceleration;
    // Weights make a reordering or a per-vehicle swap visible, which plain sums would hide.
    digest.orderWeighted += static_cast<double>(index % 97 + 1) * moved.position;
    digest.segmentWeighted += static_cast<double>(segmentHash % 83 + 1) * moved.speed;
    digest.vehicleWeighted += static_cast<double>(moved.vehicleId % 89 + 1) * moved.acceleration;
}
Json digestJson(const TrajectoryDigest& digest) {
    const double n = static_cast<double>(digest.count ? digest.count : 1);
    return Json{{"count", digest.count},
                {"meanPosition", digest.position / n},
                {"meanSpeed", digest.speed / n},
                {"meanAcceleration", digest.acceleration / n},
                {"meanOrderWeightedPosition", digest.orderWeighted / n},
                {"meanSegmentWeightedSpeed", digest.segmentWeighted / n},
                {"meanVehicleWeightedAcceleration", digest.vehicleWeighted / n}};
}
void verify(std::uint32_t seed) {
    std::ifstream file(test::root() / "tests/reference" / ("seed-" + std::to_string(seed) + ".json"));
    CHECK(file.good()); const auto expected = Json::parse(file);
    auto state = createSimulation(test::demo().scenario, seed);
    Json events = Json::array(), checkpoints = Json::array();
    SummaryAccumulator summary;
    TrajectoryDigest digest;
    std::uint64_t movedIndex = 0;
    const auto consume = [&] {
        for (const auto& e : state.events) {
            summary.add(e);
            if (const auto* moved = std::get_if<MovedEvent>(&e)) accumulate(digest, *moved, movedIndex++);
            else events.push_back(eventJson(e));
        }
    };
    consume();
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(state); consume();
        if (state.tick % 100 == 0) checkpoints.push_back(checkpointJson(state));
    }
    compare(events, expected.at("events"), "events");
    compare(checkpoints, expected.at("checkpoints"), "checkpoints");
    compare(summaryJson(summary.summary()), expected.at("summary"), "summary");

    const auto key = "seed-" + std::to_string(seed);
    const auto path = test::root() / "tests/reference/trajectory-digest.json";
    const auto actual = digestJson(digest);
    std::ifstream digestFile(path);
    if (!digestFile.good()) throw std::runtime_error("Missing " + path.string() + "; " + key + " is " + actual.dump());
    const auto recorded = Json::parse(digestFile);
    if (!recorded.contains(key)) throw std::runtime_error("Missing " + key + "; computed " + actual.dump());
    compare(actual, recorded.at(key), "trajectory." + key);
}
}
TEST(reference, typescript_seed_0) { verify(0); }
TEST(reference, typescript_seed_42) { verify(42); }
TEST(reference, typescript_seed_43) { verify(43); }
TEST(reference, typescript_seed_uint32_max) { verify(4294967295U); }
