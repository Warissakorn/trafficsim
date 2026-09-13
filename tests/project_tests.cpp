#include "test.hpp"
#include <fstream>

using namespace trafficsim;
TEST(project, strict_seed_parser) {
    CHECK(parseSeed("0") == 0); CHECK(parseSeed("42") == 42); CHECK(parseSeed("4294967295") == 4294967295U);
    for (const auto* invalid : {"", "-1", "+1", "2.5", "NaN", "Infinity", "4294967296", "1x", " 1"})
        test::throws([&] { parseSeed(invalid); });
}
TEST(project, strict_json_shapes) {
    std::ifstream file(test::root() / "data/scenarios/crossing.json");
    const auto source = Json::parse(file);
    auto n = source.at("network"); n["links"] = Json::object();
    test::throws([&] { parseNetwork(n); }, "Expected array");
    n = source.at("network"); n["links"][0]["lanes"][0]["width"] = true;
    test::throws([&] { parseNetwork(n); }, "Expected number");
    n = source.at("network"); n["drivingSide"] = "unknown";
    test::throws([&] { parseNetwork(n); }, "INVALID_DRIVING_SIDE");
    auto d = source.at("definition"); d["signalPrograms"][0]["phases"][0]["color"] = "blue";
    test::throws([&] { parseDefinition(d); }, "INVALID_SIGNAL_COLOR");
    d = source.at("definition"); d.erase("timeStep");
    test::throws([&] { parseDefinition(d); });
}
TEST(project, missing_file) {
    test::throws([&] { loadScenario(test::root() / "missing.json", test::root() / "data"); }, "Cannot read JSON");
}
TEST(project, catalog_and_translation_keys) {
    const auto s = test::demo().scenario;
    CHECK(s.vehicleTypes.size() == 1); CHECK(s.behaviours.size() == 1);
    std::ifstream english(test::root() / "data/locales/en.json"), thai(test::root() / "data/locales/th.json");
    CHECK(english.good()); CHECK(thai.good());
    const auto en = Json::parse(english), th = Json::parse(thai);
    CHECK(en.size() == th.size());
    for (const auto& [key, value] : en.items()) {
        CHECK(value.is_string()); CHECK(!value.get<std::string>().empty());
        CHECK(th.at(key).is_string()); CHECK(!th.at(key).get<std::string>().empty());
    }
}
