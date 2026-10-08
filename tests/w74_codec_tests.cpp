#include "test.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/behaviour_library.hpp"
#include "../src/project/run.hpp"
using namespace trafficsim;
// M3.3.3a (D131): row BA29 of docs/plans/DRIVING_BEHAVIOUR.md -- the schema-22 codec of a `w74`
// behaviour (docs/reference/W74.md §5, §9) and the interim Run refusal of one in use.
namespace {
const W74Parameters kP{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0.5, .cxAdd = 16,
    .cxMult = 4, .opdvAdd = 0.5, .opdvMult = 0.5, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0.5, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
// Owned catalogs from data/, two Links joined by a Connector, one routed input of cars.
ProjectDocument owned() {
    ProjectDocument d; const auto a = addLink(d, {{0, 0}, {200, 0}}, 1, 3.5), b = addLink(d, {{220, 0}, {400, 0}}, 1, 3.5);
    addConnector(d, {a, d.network.links[0].lanes[0].id, {}}, {b, d.network.links[1].lanes[0].id, {}});
    const auto route = putRoute(d, {"route", {a, d.network.connectors[0].id, b}}); changeRunSettings(d, 60, .1);
    putDemandCatalog(d, resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data"));
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return d;
}
// The same project with an unused w74 behaviour named "urban-w74".
ProjectDocument withW74() {
    auto d = owned();
    DriverBehaviour w74{"urban-w74"}; w74.w74 = kP; w74.maxDecelerationCooperativeBraking = 3;
    putBehaviour(d, w74, "Urban W74");
    validateDocument(d);
    return d;
}
// Index of the w74 behaviour in the written file.
std::size_t w74Index(const Json& j) {
    const auto& list = j["definition"]["behaviours"];
    for (std::size_t i = 0; i < list.size(); ++i) if (list[i]["id"] == "urban-w74") return i;
    throw std::runtime_error("no w74 behaviour");
}
void refused(const Json& j, const std::string& code, const std::string& path) {
    test::throws([&] { parseDocument(j); }, code);
    try { parseDocument(j); } catch (const ValidationError& e) {
        CHECK(std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; }));
    }
}
}

TEST(w74codec, schema22_round_trips_only_when_a_w74_behaviour_is_owned) { // BA29
    const auto plain = documentJson(owned());
    CHECK(plain["schemaVersion"] == 18);
    const auto d = withW74(); const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 22);
    const auto& b = j["definition"]["behaviours"][w74Index(j)];
    CHECK(b["model"] == "w74"); CHECK(b["name"] == "Urban W74"); CHECK(b["maxDecelerationCooperativeBraking"] == 3);
    for (const auto& key : w74ParameterKeys()) CHECK(b.at(key.name) == kP.*key.member);
    for (const char* key : {"standstillDistance", "followingTime", "speedThreshold"}) CHECK(!b.contains(key));
    for (const auto& other : j["definition"]["behaviours"]) if (other["id"] != "urban-w74") CHECK(other["model"] == "prototype");
    const auto back = parseDocument(j);
    CHECK(documentJson(back) == j);
    CHECK(back.definition->behaviours == d.definition->behaviours);
    CHECK(documentJson(back).dump() == j.dump());
    // Without the w74 behaviour the file is exactly the plain one again.
    auto removed = d; std::erase_if(removed.definition->behaviours, [](const auto& x) { return x.w74.has_value(); });
    removed.definition->behaviourNames.clear();
    CHECK(documentJson(removed).dump() == plain.dump());
}

TEST(w74codec, missing_non_numeric_and_out_of_range_keys_are_refused_with_their_path) { // BA29
    const auto j = documentJson(withW74()); const auto i = w74Index(j);
    const auto path = "behaviours[" + std::to_string(i) + "]";
    for (const auto& key : w74ParameterKeys()) {
        auto missing = j; missing["definition"]["behaviours"][i].erase(key.name);
        refused(missing, "INVALID_BEHAVIOUR_PARAMETER", path + "." + key.name);
        auto text = j; text["definition"]["behaviours"][i][key.name] = "1";
        refused(text, "INVALID_BEHAVIOUR_PARAMETER", path + "." + key.name);
    }
    // One value just outside each kind of range; the behaviour is unused, and still refused.
    for (const auto& [key, value] : std::vector<std::pair<std::string, double>>{
             {"ax", 0}, {"bxAdd", 0}, {"bxMult", -0.1}, {"exAdd", 0.5}, {"cxAdd", 0}, {"opdvAdd", 0},
             {"dMax", 0}, {"bMaxAdd", 0}, {"bNullAdd", 0}, {"bMinAdd", 1}, {"leaderAccelerationWeight", 1.5},
             {"emergencyLeaderWeight", -0.5}}) {
        auto bad = j; bad["definition"]["behaviours"][i][key] = value;
        refused(bad, "INVALID_BEHAVIOUR_PARAMETER", path + "." + key);
    }
    // The boundaries themselves are inside the range.
    auto edge = j; auto& e = edge["definition"]["behaviours"][i];
    e["exAdd"] = 1; e["bxMult"] = 0; e["bMinAdd"] = 0; e["leaderAccelerationWeight"] = 1; e["emergencyLeaderWeight"] = 0;
    CHECK(parseDocument(edge).definition->behaviours.size() == j["definition"]["behaviours"].size());
}

TEST(w74codec, each_models_keys_are_refused_on_the_other_and_unknown_models_too) { // BA29
    const auto j = documentJson(withW74()); const auto i = w74Index(j);
    const auto path = "behaviours[" + std::to_string(i) + "]";
    for (const char* key : {"followingTime", "standstillDistance", "discretionaryLaneChangeThreshold"}) {
        auto extra = j; extra["definition"]["behaviours"][i][key] = 1;
        refused(extra, "EDIT_UNSUPPORTED_FIELD", path + "." + key);
    }
    const std::size_t prototype = i == 0 ? 1 : 0;
    auto onPrototype = j; onPrototype["definition"]["behaviours"][prototype]["ax"] = 2;
    refused(onPrototype, "EDIT_UNSUPPORTED_FIELD", "behaviours[" + std::to_string(prototype) + "].ax");
    auto unknown = j; unknown["definition"]["behaviours"][i]["cc0"] = 1.5;
    refused(unknown, "EDIT_UNSUPPORTED_FIELD", path + ".cc0");
    auto w99 = j; w99["definition"]["behaviours"][i]["model"] = "w99";
    refused(w99, "UNSUPPORTED_BEHAVIOUR_MODEL", path + ".model");
}

TEST(w74codec, w74_below_schema_22_and_a_future_schema_are_refused) { // BA29
    const auto j = documentJson(withW74()); const auto i = w74Index(j);
    auto old = j; old["schemaVersion"] = 21;
    refused(old, "UNSUPPORTED_BEHAVIOUR_MODEL", "behaviours[" + std::to_string(i) + "].model");
    auto future = j; future["schemaVersion"] = 23;
    test::throws([&] { parseDocument(future); }, "EDIT_VERSION");
}

TEST(w74codec, external_catalog_entries_read_the_model_key_and_default_to_the_prototype) { // §9
    Json item{{"id", "x"}, {"model", "w74"}};
    for (const auto& key : w74ParameterKeys()) item[key.name] = kP.*key.member;
    const auto w74 = parseBehaviour(item);
    CHECK(w74.w74 == kP);
    Json legacy{{"id", "p"}, {"standstillDistance", 2}, {"additiveSafetyDistance", 2},
        {"multiplicativeSafetyDistance", 3}, {"followingTime", 1.5}, {"speedThreshold", .2}};
    CHECK(!parseBehaviour(legacy).w74);
    legacy["model"] = "prototype"; CHECK(!parseBehaviour(legacy).w74);
    item.erase("ax"); test::throws([&] { parseBehaviour(item); }, "INVALID_BEHAVIOUR_PARAMETER");
}

TEST(w74codec, a_w74_behaviour_in_use_runs_and_an_unused_one_changes_nothing) { // D131, D133
    const auto data = test::root() / "data";
    // Unused: the run is the plain project's, vehicle for vehicle.
    const auto plain = compileDocument(owned(), data).scenario, unused = compileDocument(withW74(), data).scenario;
    CHECK(unused.behaviours.size() == plain.behaviours.size() + 1);
    const auto a = runSimulation(plain, 42), b = runSimulation(unused, 42);
    // Equal but for the hashed traits (D132), which only a scenario holding w74 carries.
    auto traitless = b.vehicles;
    for (auto& v : traitless) { CHECK(v.w74Traits.has_value()); CHECK(!v.w74State); v.w74Traits.reset(); }
    CHECK(!a.vehicles.empty()); CHECK(a.vehicles == traitless); CHECK(a.time == b.time);
    // In use, by a vehicle type or by a road: D131's interim Run refusal is lifted (D133).
    auto byType = withW74();
    for (auto& t : byType.definition->vehicleTypes) if (t.id == "car") t.behaviourId = "urban-w74";
    validateDocument(byType);
    CHECK(parseDocument(documentJson(byType)).definition->vehicleTypes == byType.definition->vehicleTypes);
    auto byRoad = withW74();
    putLinkBehaviourType(byRoad, {"urban", "Urban", "urban-w74", {}});
    assignBehaviourType(byRoad, byRoad.network.links[0].id, "urban");
    validateDocument(byRoad);
    for (const auto* d : {&byType, &byRoad}) {
        auto state = createSimulation(compileDocument(*d, data).scenario, 42);
        bool sawState = false;
        while (state.tick < totalTicks(*state.scenario)) {
            state = stepSimulation(std::move(state));
            for (const auto& v : state.vehicles) sawState = sawState || v.w74State.has_value();
        }
        CHECK(sawState);
    }
}
