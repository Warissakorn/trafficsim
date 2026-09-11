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
void verify(std::uint32_t seed) {
    std::ifstream file(test::root() / "tests/reference" / ("seed-" + std::to_string(seed) + ".json"));
    CHECK(file.good()); const auto expected = Json::parse(file);
    auto state = createSimulation(test::demo().scenario, seed);
    Json events = Json::array(), checkpoints = Json::array();
    SummaryAccumulator summary;
    const auto consume = [&] {
        for (const auto& e : state.events) {
            summary.add(e);
            if (!std::holds_alternative<MovedEvent>(e)) events.push_back(eventJson(e));
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
}
}
TEST(reference, typescript_seed_0) { verify(0); }
TEST(reference, typescript_seed_42) { verify(42); }
TEST(reference, typescript_seed_43) { verify(43); }
TEST(reference, typescript_seed_uint32_max) { verify(4294967295U); }
