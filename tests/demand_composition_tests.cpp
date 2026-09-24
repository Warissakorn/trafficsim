#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <fstream>
#include <map>
using namespace trafficsim;
// M2.3: an input may draw its vehicles from a composition in data/compositions/. Resolving the
// catalog expands it into one input per type, so the core and the frozen fixtures are untouched.
namespace {
struct Built { ProjectDocument document; std::string route; };
Built road(double duration = 600) {
    Built b;
    const auto link = addLink(b.document, {{0, 0}, {400, 0}}, 1, 3.5);
    b.route = putRoute(b.document, {"", {link}});
    changeRunSettings(b.document, duration, 0.1);
    return b;
}
VehicleInput mixed(const std::string& route, double volume, double end) {
    VehicleInput input{"", route, "", volume, 0, end, {}, {}};
    input.compositionId = "urban-mixed";
    return input;
}
// A throwaway data directory holding only the catalogs a run reads, plus `compositions`.
std::filesystem::path dataWith(const std::string& name, const Json& composition) {
    const auto dir = std::filesystem::temp_directory_path() / ("trafficsim-compositions-" + name);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    for (const char* sub : {"vehicle-types", "driver-behaviour", "priority-rules"})
        std::filesystem::copy(test::root() / "data" / sub, dir / sub, std::filesystem::copy_options::recursive);
    std::filesystem::create_directories(dir / "compositions");
    std::ofstream(dir / "compositions" / "c.json") << composition.dump();
    return dir;
}
}
TEST(demand, a_composition_expands_to_one_input_per_type) {
    auto b = road();
    const auto id = putInput(b.document, mixed(b.route, 1000, 600));
    const auto inputs = compileDocument(b.document, test::root() / "data").scenario.inputs;
    CHECK(inputs.size() == 2);
    CHECK(inputs[0].id == id + "/type-car"); CHECK(inputs[0].vehicleTypeId == "car");
    CHECK(inputs[1].id == id + "/type-heavy-vehicle"); CHECK(inputs[1].vehicleTypeId == "heavy-vehicle");
    test::near(inputs[0].vehiclesPerHour, 950); test::near(inputs[1].vehiclesPerHour, 50);
    for (const auto& i : inputs) CHECK(i.compositionId.empty());
}
TEST(demand, a_single_type_composition_is_that_type_exactly) {
    auto a = road(), b = road();
    putInput(a.document, {"in", a.route, "car", 900, 0, 600, {}, {}});
    auto only = mixed(b.route, 900, 600); only.id = "in"; only.compositionId = "car-only";
    putInput(b.document, only);
    CHECK(compileDocument(a.document, test::root() / "data").scenario.inputs ==
          compileDocument(b.document, test::root() / "data").scenario.inputs);
}
TEST(demand, a_composition_scales_every_counted_interval) {
    auto b = road();
    auto input = mixed(b.route, 0, 0);
    input.intervals = {{0, 300, 400}, {300, 600, 800}};
    const auto id = putInput(b.document, input);
    const auto inputs = compileDocument(b.document, test::root() / "data").scenario.inputs;
    CHECK(inputs.size() == 4);
    std::map<std::string, double> volume;
    for (const auto& i : inputs) volume[i.id] = i.vehiclesPerHour;
    test::near(volume[id + "/type-heavy-vehicle/int-1"], 20);
    test::near(volume[id + "/type-heavy-vehicle/int-2"], 40);
    test::near(volume[id + "/type-car/int-2"], 760);
}
TEST(demand, sampled_heavy_share_matches_the_composition) {
    // 5% heavy vehicles at 1800 veh/h for an hour: about 1800 arrivals, sigma of the sampled share
    // sqrt(0.05 * 0.95 / 1800) = 0.0051. The seed is fixed, so this cannot flake; 3 sigma is the
    // stated tolerance for "the share the author asked for is the share the run released".
    auto b = road(3600);
    putInput(b.document, mixed(b.route, 1800, 3600));
    const auto scenario = compileDocument(b.document, test::root() / "data").scenario;
    std::map<std::uint64_t, std::uint32_t> typeOf;
    auto state = createSimulation(scenario, 42);
    while (state.time < scenario.duration - 1e-9) {
        state = stepSimulation(state);
        // Every released vehicle, entered or still queued at the entry: its type is drawn on release.
        for (const auto& v : state.vehicles) typeOf.emplace(v.id, v.typeIndex);
        for (const auto& input : state.inputs) for (const auto& v : input.queue) typeOf.emplace(v.id, v.typeIndex);
    }
    CHECK(typeOf.size() > 1500); // The forcing: enough vehicles for the tolerance to mean something.
    const auto heavy = std::count_if(typeOf.begin(), typeOf.end(), [&](const auto& e) {
        return state.scenario->vehicleTypes[e.second].id == "heavy-vehicle"; });
    const double share = static_cast<double>(heavy) / static_cast<double>(typeOf.size());
    CHECK(std::abs(share - 0.05) < 3 * 0.0051);
}
TEST(demand, an_unknown_composition_blocks_run_by_name) {
    auto b = road();
    auto input = mixed(b.route, 600, 600); input.compositionId = "nowhere";
    putInput(b.document, input); // Authoring accepts it: the catalog is checked on Run.
    const auto rows = runDiagnostics(b.document, test::root() / "data");
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) {
        return r.code == "UNKNOWN_COMPOSITION" && r.path == "inputs[0].compositionId"; }));
    test::throws([&] { compileDocument(b.document, test::root() / "data"); }, "UNKNOWN_COMPOSITION");
}
TEST(demand, a_broken_composition_is_named_not_run) {
    auto b = road();
    auto input = mixed(b.route, 600, 600); input.compositionId = "c";
    putInput(b.document, input);
    const auto zero = dataWith("zero", {{"id", "c"}, {"types", {{{"vehicleTypeId", "car"}, {"share", 0}}}}});
    test::throws([&] { compileDocument(b.document, zero); }, "INVALID_SHARE");
    const auto ghost = dataWith("ghost", {{"id", "c"}, {"types", {{{"vehicleTypeId", "ghost"}, {"share", 1}}}}});
    test::throws([&] { compileDocument(b.document, ghost); }, "UNKNOWN_VEHICLE_TYPE");
    // The forcing, last: the same document runs against a sound catalog of the same shape.
    const auto sound = dataWith("sound", {{"id", "c"}, {"types", {{{"vehicleTypeId", "car"}, {"share", 1}}}}});
    CHECK(compileDocument(b.document, sound).scenario.inputs.size() == 1);
}
TEST(demand, composition_round_trips_and_stays_absent_when_unset) {
    auto b = road();
    putInput(b.document, mixed(b.route, 600, 600));
    putInput(b.document, {"", b.route, "car", 300, 0, 600, {}, {}});
    const auto saved = documentJson(b.document);
    CHECK(saved["definition"]["inputs"][0]["compositionId"] == "urban-mixed");
    CHECK(!saved["definition"]["inputs"][1].contains("compositionId"));
    CHECK(parseDocument(Json::parse(saved.dump())).definition->inputs == b.document.definition->inputs);
}
