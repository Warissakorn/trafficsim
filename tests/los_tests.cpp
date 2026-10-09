#include "test.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/los_output.hpp"
#include "../src/project/run.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>
using namespace trafficsim;
// M5.5 (D134): LOS letters from section delay, rows L1-L6 of docs/reference/LOS.md §6.
namespace {
Json fourLegJson() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return j;
}
// A data directory holding the queue conditions and the given pack, so evaluationSpec can read it.
std::filesystem::path dataWith(const std::string& name, const Json& pack) {
    const auto dir = std::filesystem::temp_directory_path() / ("trafficsim-los-" + name);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir / "evaluation"); std::filesystem::create_directories(dir / "los");
    std::filesystem::copy_file(test::root() / "data/evaluation/queue-counter.json", dir / "evaluation/queue-counter.json");
    std::ofstream(dir / "los/hcm.json") << pack.dump();
    return dir;
}
Json hcmJson() { std::ifstream file(test::root() / "data/los/hcm.json"); Json j; file >> j; return j; }
ProjectDocument controlled(std::optional<SectionControl> type = SectionControl::signalised) {
    auto d = parseDocument(fourLegJson());
    putTravelTimeSection(d, {"", "West through", {"link-1", 20}, {"link-20", 50}, type});
    putTravelTimeSection(d, {"", "West left", {"link-1", 20}, {"link-44", 50}, type});
    validateDocument(d);
    return d;
}
}
TEST(los, every_bound_on_both_sides) { // L1
    const auto pack = loadLosPack(test::root() / "data");
    CHECK(pack.id == "hcm");
    const std::map<std::string, std::array<double, 5>> expected{{"signalised", {10, 20, 35, 55, 80}}, {"unsignalised", {10, 15, 25, 35, 50}}};
    CHECK(pack.bounds == expected);
    for (const auto& [type, bounds] : expected) {
        CHECK(losLetter(0, type, pack) == 'A');
        for (std::size_t i = 0; i < 5; ++i) {
            CHECK(losLetter(bounds[i], type, pack) == static_cast<char>('A' + i));        // equal: the better letter
            CHECK(losLetter(bounds[i] + 0.01, type, pack) == static_cast<char>('B' + i)); // just above: the next
        }
        CHECK(losLetter(1e6, type, pack) == 'F');
    }
    CHECK(!losLetter(5, "roundabout", pack));
}
TEST(los, a_swapped_pack_changes_letters_without_code) { // L2
    const auto d = controlled();
    const auto real = test::root() / "data";
    auto strict = hcmJson(); strict["id"] = "strict"; strict["controlTypes"]["signalised"] = {1, 2, 3, 4, 5};
    const auto swapped = dataWith("strict", strict);
    const auto snapshot = compileDocument(d, real);
    const auto a = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, real), 42).report;
    const auto b = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, swapped), 42).report;
    // The forcing: the same delay, measured once; only the bounds differ.
    CHECK(a.sections[0].meanDelay == b.sections[0].meanDelay); CHECK(*a.sections[0].meanDelay > 5);
    const auto ja = movementJson(a), jb = movementJson(b);
    CHECK(jb["sections"][0]["los"] == "F"); CHECK(ja["sections"][0]["los"] != jb["sections"][0]["los"]);
    CHECK(ja["los"]["pack"] == "hcm"); CHECK(jb["los"]["pack"] == "strict");
}
TEST(los, bad_packs_are_refused) { // L3
    std::vector<Json> bad;
    auto j = hcmJson(); j["controlTypes"].erase("unsignalised"); bad.push_back(j);
    j = hcmJson(); j["controlTypes"]["signalised"] = {10, 20, 35, 55}; bad.push_back(j);
    j = hcmJson(); j["controlTypes"]["signalised"] = {10, 20, 20, 55, 80}; bad.push_back(j);
    j = hcmJson(); j["controlTypes"]["unsignalised"] = {-1, 15, 25, 35, 50}; bad.push_back(j);
    j = hcmJson(); j["controlTypes"]["unsignalised"] = {10, 15, "25", 35, 50}; bad.push_back(j);
    j = hcmJson(); j["controlTypes"]["roundabout"] = {10, 15, 25, 35, 50}; bad.push_back(j);
    j = hcmJson(); j["vcRule"] = true; bad.push_back(j);
    j = hcmJson(); j.erase("id"); bad.push_back(j);
    for (std::size_t i = 0; i < bad.size(); ++i) {
        const auto dir = dataWith("bad-" + std::to_string(i), bad[i]);
        test::throws([&] { loadLosPack(dir); }, "EDIT_CATALOG_READ");
    }
    test::throws([] { loadLosPack(std::filesystem::temp_directory_path() / "trafficsim-los-none"); }, "EDIT_CATALOG_READ");
}
TEST(los, groups_weight_by_vehicles) { // L4
    const auto pack = loadLosPack(test::root() / "data");
    const std::vector<LosInput> sections{
        {"West", "signalised", 30, 10.0},   // 300
        {"West", "signalised", 10, 50.0},   // 500 -> West: 800 / 40 = 20 s, B
        {"North", "signalised", 20, 40.0},  // 800 -> North: 40 s, D
        {"North", "signalised", 0, std::nullopt}, // no vehicle: left out
        {"North", std::nullopt, 99, 99.0},  // no type: left out
        {"East", "unsignalised", 5, 16.0},  // 16 s unsignalised: C
    };
    const auto g = losGroups(sections, pack);
    CHECK(g.approaches.size() == 3);
    CHECK(g.approaches[0].name == "West"); test::near(*g.approaches[0].delay, 20); CHECK(g.approaches[0].los == 'B');
    CHECK(g.approaches[0].vehicles == 40);
    CHECK(g.approaches[1].name == "North"); test::near(*g.approaches[1].delay, 40); CHECK(g.approaches[1].los == 'D');
    CHECK(g.approaches[2].los == 'C');
    CHECK(g.intersections.size() == 2);
    test::near(*g.intersections[0].delay, 1600.0 / 60); CHECK(g.intersections[0].los == 'C'); // 26.67 s signalised
    CHECK(g.intersections[1].controlType == "unsignalised"); CHECK(g.intersections[1].los == 'C');
    // A group whose sections have no vehicle has no delay and no letter.
    const auto empty = losGroups({{"South", "signalised", 0, std::nullopt}}, pack);
    CHECK(empty.approaches.size() == 1); CHECK(!empty.approaches[0].delay); CHECK(!empty.approaches[0].los);
}
TEST(los, schema24_codec) { // L5
    const auto d = controlled();
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 24);
    CHECK(j["network"]["travelTimeSections"][0]["controlType"] == "signalised");
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d); CHECK(documentJson(reopened) == j);
    // Without a type the sections stay schema 23 and write no key.
    const auto plain = documentJson(controlled(std::nullopt));
    CHECK(plain["schemaVersion"] == 23); CHECK(!plain["network"]["travelTimeSections"][0].contains("controlType"));
    auto older = j; older["schemaVersion"] = 23;
    test::throws([&] { parseDocument(older); }, "EDIT_UNSUPPORTED_FIELD");
    auto other = j; other["network"]["travelTimeSections"][0]["controlType"] = "roundabout";
    test::throws([&] { parseDocument(other); }, "INVALID_ENUM");
    auto newer = j; newer["schemaVersion"] = 28;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
}
TEST(los, outputs_letter_and_mark_the_rows) { // L6
    const auto data = test::root() / "data";
    const auto d = controlled();
    const auto snapshot = compileDocument(d, data);
    InputManifest read;
    const auto spec = evaluationSpec(d, snapshot, data, &read);
    CHECK(read.json().dump().find("los/hcm.json") != std::string::npos);
    CHECK(spec.sections[0].approach == "West approach"); CHECK(spec.sections[0].controlType == "signalised");
    const auto r = runSeed(snapshot.scenario, spec, 42).report;
    const auto j = movementJson(r);
    const auto letter = j["sections"][0]["los"].get<std::string>();
    CHECK(letter == std::string(1, *losLetter(*r.sections[0].meanDelay, "signalised", *r.los)));
    CHECK(j["los"]["approaches"].size() == 1); CHECK(j["los"]["approaches"][0]["name"] == "West approach");
    CHECK(j["los"]["intersections"][0]["controlType"] == "signalised");
    const auto csv = movementCsv(r);
    CHECK(csv.find("\nsection,vehicles,meanTravelTime_s,meanDelay_s,unfinished,controlType,los\n\"West through\",") != std::string::npos);
    CHECK(csv.find(",signalised," + letter + "\n") != std::string::npos);
    CHECK(csv.find("\n# LOS (pack hcm) from simulated section delay, not HCM control delay; not validated (M6)\nlevel,name,controlType,vehicles,delay_s,los\napproach,\"West approach\",signalised,") != std::string::npos);
    // No type: no letter, no LOS block, and the pack is never read.
    const auto untyped = controlled(std::nullopt);
    InputManifest none;
    const auto plainSpec = evaluationSpec(untyped, compileDocument(untyped, data), data, &none);
    CHECK(!plainSpec.los); CHECK(none.json().dump().find("los/hcm.json") == std::string::npos);
    const auto p = runSeed(snapshot.scenario, plainSpec, 42).report;
    CHECK(!movementJson(p).contains("los")); CHECK(movementJson(p)["sections"][0]["los"].is_null());
    CHECK(movementCsv(p).find("# LOS") == std::string::npos);
    // Batch: the letter of the mean over seeds, weighted by the mean vehicles.
    SeedRun a, b; a.seed = 1; b.seed = 2;
    a.report.sections = {{"s", 10, 30.0, 18.0, 0, "signalised", "West"}};
    b.report.sections = {{"s", 30, 34.0, 24.0, 0, "signalised", "West"}};
    a.report.los = b.report.los = r.los;
    const auto batch = aggregate({a, b});
    CHECK(batch.sections[0].controlType == "signalised"); CHECK(batch.los == r.los);
    const auto bj = batchJson(batch, {a, b});
    CHECK(bj["sections"][0]["los"] == "C"); // mean 21 s
    CHECK(bj["los"]["approaches"][0]["vehicles"] == 20.0); CHECK(bj["los"]["approaches"][0]["los"] == "C");
    const auto bc = batchCsv(batch, {a, b});
    CHECK(bc.find("unfinished_mean,controlType,los\n\"s\",2,21.00,") != std::string::npos);
    CHECK(bc.find(",signalised,C\n") != std::string::npos);
    CHECK(bc.find("# LOS (pack hcm)") != std::string::npos);
}
