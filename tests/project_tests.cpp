#include "test.hpp"
#include "../src/project/document.hpp"
#include <fstream>
#include <tuple>

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
// The reported failure: an editor project opened in the simulation window used to surface
// "[json.exception.type_error.304] cannot use at() with null" because a drawn network is
// legitimately saved with "definition": null. It must name the file kind instead.
TEST(project, file_kind_is_named_not_thrown_as_a_parser_error) {
    const auto directory = std::filesystem::temp_directory_path() / "trafficsim-project-tests";
    std::filesystem::create_directories(directory);
    const auto data = test::root() / "data";
    const auto write = [&](const std::string& name, const Json& value) {
        const auto path = directory / name;
        std::ofstream(path) << value.dump(2);
        return path;
    };
    ProjectDocument drawn;
    drawn.network = test::demo().network;
    const auto project = documentJson(drawn);
    CHECK(project.at("definition").is_null()); // the shape that produced the report
    for (const auto& [name, value, code] : std::vector<std::tuple<std::string, Json, std::string>>{
            {"network.traffic.json", project, "SCENARIO_IS_PROJECT"},
            {"no-definition.json", Json{{"network", project.at("network")}}, "SCENARIO_NO_DEFINITION"},
            {"null-network.json", Json{{"network", nullptr}, {"definition", Json::object()}}, "SCENARIO_NO_NETWORK"},
            {"array.json", Json::array(), "SCENARIO_NOT_JSON_OBJECT"}}) {
        const auto path = write(name, value);
        try {
            loadScenario(path, data);
            throw std::runtime_error("Expected a load failure for " + name);
        } catch (const ScenarioLoadError& error) {
            CHECK(error.code == code);
            CHECK(std::string(error.what()).find("json.exception") == std::string::npos);
        }
    }
    std::filesystem::remove_all(directory);
}
TEST(project, editor_rejects_null_sections_with_named_codes) {
    auto project = documentJson(ProjectDocument{});
    CHECK(parseDocument(project).network.links.empty()); // the round trip still works
    auto broken = project; broken["background"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_BACKGROUND_INVALID");
    broken = project; broken["background"]["metresPerPixel"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_BACKGROUND_INVALID");
    broken = project; broken["network"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_NO_NETWORK");
    broken = project; broken["nextId"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_ID_LIMIT");
    broken = project; broken["format"] = nullptr;
    test::throws([&] { parseDocument(broken); }, "EDIT_VERSION");
    test::throws([&] { parseDocument(Json(nullptr)); }, "EDIT_VERSION");
}
// No load path may surface an nlohmann exception: a null anywhere in a scenario must produce
// a sentence naming the field, not "cannot use at() with null" / "type must be ... but is null".
TEST(project, no_null_field_leaks_a_parser_exception) {
    std::ifstream file(test::root() / "data/scenarios/crossing.json");
    const auto source = Json::parse(file);
    const auto nulled = [&](const std::function<void(Json&)>& breakIt) {
        auto copy = source; breakIt(copy);
        try { parseNetwork(copy.at("network")); parseDefinition(copy.at("definition")); }
        catch (const std::exception& error) {
            CHECK(std::string(error.what()).find("json.exception") == std::string::npos);
            return;
        }
        throw std::runtime_error("Expected a rejection");
    };
    nulled([](Json& j) { j["network"]["links"][0]["id"] = nullptr; });
    nulled([](Json& j) { j["network"]["links"][0]["lanes"][0]["width"] = nullptr; });
    nulled([](Json& j) { j["network"]["connectors"][0]["from"] = nullptr; });
    nulled([](Json& j) { j["network"]["signalHeads"][0]["lane"]["laneId"] = nullptr; });
    nulled([](Json& j) { j["definition"]["routes"][0]["segmentIds"][0] = nullptr; });
    nulled([](Json& j) { j["definition"]["signalPrograms"][0]["phases"] = nullptr; });
    nulled([](Json& j) { j["definition"]["inputs"][0]["vehiclesPerHour"] = nullptr; });
}
// Root-level metadata is where classification itself reads, and it was the one place a
// parser exception still escaped: value("format", std::string{}) throws when the key is
// present but not a string. Drive these through loadScenario, not the section parsers.
TEST(project, file_kind_classification_survives_broken_metadata) {
    const auto directory = std::filesystem::temp_directory_path() / "trafficsim-metadata-tests";
    std::filesystem::create_directories(directory);
    std::ifstream source(test::root() / "data/scenarios/crossing.json");
    const auto scenario = Json::parse(source);
    for (const auto& metadata : {Json(nullptr), Json(42), Json(Json::array()), Json("TrafficSim"), Json("other")})
        for (const auto* key : {"format", "schemaVersion"}) {
            auto broken = scenario; broken.erase("definition"); broken[key] = metadata;
            const auto path = directory / "broken.json";
            std::ofstream(path) << broken.dump(2);
            try {
                loadScenario(path, test::root() / "data");
                throw std::runtime_error("Expected a load failure");
            } catch (const ScenarioLoadError& error) {
                CHECK(std::string(error.what()).find("json.exception") == std::string::npos);
                CHECK(error.code == "SCENARIO_IS_PROJECT" || error.code == "SCENARIO_NO_DEFINITION");
            }
        }
    std::filesystem::remove_all(directory);
}
