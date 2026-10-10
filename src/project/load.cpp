#include "load.hpp"
#include "input_manifest.hpp"
#include "json.hpp"
#include "run.hpp"
#include "demand_catalog.hpp"
#include "demand_time_types.hpp"
#include <set>
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace trafficsim {
namespace {
Json readJson(const std::filesystem::path& file,InputManifest* manifest=nullptr) {
    return readInputJson(file,"catalog/"+file.parent_path().filename().generic_string()+"/"+
                              file.filename().generic_string(),manifest);
}
std::vector<Json> catalog(const std::filesystem::path& directory,InputManifest* manifest=nullptr) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    std::vector<Json> result;
    for (const auto& file : files) result.push_back(readJson(file,manifest));
    return result;
}
}
std::vector<Composition> loadCompositions(const std::filesystem::path& dataDirectory,InputManifest* manifest) {
    std::vector<Composition> result;
    // A data directory with no compositions is legal: nothing may name one, and that is checked
    // where one is named. A composition file that does not parse is a broken catalog.
    if (!std::filesystem::is_directory(dataDirectory / "compositions")) return result;
    try {
        for (const auto& item : catalog(dataDirectory / "compositions",manifest)) result.push_back(parseComposition(item));
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    return result;
}
namespace {
bool validComposition(const Composition& c, const std::vector<VehicleType>& types) {
    if (c.types.empty()) return false;
    double total=0;std::set<std::string> ids;
    for (const auto& t : c.types) {
        total+=t.share;if(!ids.insert(t.vehicleTypeId).second)return false;
        if (!(std::isfinite(t.share) && t.share > 0)) return false;
        if (std::none_of(types.begin(), types.end(), [&](const auto& v) { return v.id == t.vehicleTypeId; }))
            return false;
    }
    return std::isfinite(total) && total>0;
}
const Composition* findComposition(const std::vector<Composition>& all, const std::string& id) {
    for (const auto& c : all) if (c.id == id) return &c;
    return nullptr;
}
}
std::vector<ValidationIssue> compositionIssues(const AuthoringDefinition& authored,
                                               const std::filesystem::path& dataDirectory,InputManifest* manifest) {
    std::vector<ValidationIssue> issues;
    if (std::none_of(authored.inputs.begin(), authored.inputs.end(),
                     [](const auto& i) { return !i.compositionId.empty(); }) && authored.externalCompositions && !hasTypeRouting(authored)) return issues;
    const bool used=std::any_of(authored.inputs.begin(),authored.inputs.end(),[](const auto& i){return !i.compositionId.empty();});
    const auto resolved=resolveDemandCatalog(authored,dataDirectory,used,manifest);
    const auto& all=resolved.compositions;const auto& types=resolved.vehicleTypes;
    auto owned=authored;owned.vehicleTypes=types;owned.externalVehicleTypes=false;
    issues=timeTypeIssues(owned);
    if(!owned.externalCompositions) {
        const auto catalogIssues=ownedCatalogIssues(owned);issues.insert(issues.end(),catalogIssues.begin(),catalogIssues.end());
    }
    for (std::size_t i = 0; i < authored.inputs.size(); ++i) {
        const auto& id = authored.inputs[i].compositionId;
        if (id.empty()) continue;
        const auto path = "inputs[" + std::to_string(i) + "].compositionId";
        const auto* c = findComposition(all, id);
        if (!c) { issues.push_back({"UNKNOWN_COMPOSITION", path}); continue; }
        if(!c->intervals.empty()) {
            auto single=owned;single.compositions={*c};single.inputs.clear();single.externalCompositions=false;
            const auto timed=ownedCatalogIssues(single);issues.insert(issues.end(),timed.begin(),timed.end());
        }
        if (c->types.empty() || std::any_of(c->types.begin(), c->types.end(),
                [](const auto& t) { return !(std::isfinite(t.share) && t.share > 0); }))
            issues.push_back({"INVALID_SHARE", path});
        else if (!validComposition(*c, types)) issues.push_back({"UNKNOWN_VEHICLE_TYPE", path});
    }
    return issues;
}
DemandCatalog resolveDemandCatalog(const AuthoringDefinition& authored,const std::filesystem::path& dataDirectory,bool includeCompositions,InputManifest* manifest) {
    DemandCatalog result{authored.vehicleTypes,authored.behaviours,authored.compositions,authored.vehicleTypeNames};
    try {
        if (authored.externalVehicleTypes) {
            result.vehicleTypes.clear();result.vehicleTypeNames.clear();
            for (const auto& item : catalog(dataDirectory / "vehicle-types",manifest)) {
                result.vehicleTypes.push_back(parseVehicleType(item));
                if(item.contains("name")) {
                    if(!item.at("name").is_string())throw std::invalid_argument("EDIT_CATALOG_READ");
                    const auto name=item.at("name").get<std::string>();
                    if(!name.empty())result.vehicleTypeNames[result.vehicleTypes.back().id]=name;
                }
            }
        }
        if (authored.externalBehaviours) {
            result.behaviours.clear();
            for (const auto& item : catalog(dataDirectory / "driver-behaviour",manifest))
                result.behaviours.push_back(parseBehaviour(item));
        }
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    // M4.2 (D147): an M0 scenario keeps amber as red (AMBER.md §2).
    if (authored.provenance.legacyAmber) for (auto& b : result.behaviours) b.amberDeceleration.reset();
    if(includeCompositions && authored.externalCompositions)result.compositions=loadCompositions(dataDirectory,manifest);
    return result;
}
ScenarioDefinition resolveCatalogs(const AuthoringDefinition& authored, const std::filesystem::path& dataDirectory,InputManifest* manifest) {
    const auto resolved=resolveDemandCatalog(authored,dataDirectory,
        std::any_of(authored.inputs.begin(),authored.inputs.end(),[](const auto& i){return !i.compositionId.empty();}),manifest);
    // Legacy ordering/IDs/draw stream are frozen. Type overrides need a type before routing.
    AuthoringDefinition expanded=authored;
    if(hasTypeRouting(authored)) {
        expanded.inputs=expandCompositions(authored.inputs,resolved.compositions);
        expanded=withRoutingDecisions(std::move(expanded));
    } else {
        expanded=withRoutingDecisions(std::move(expanded));
        expanded.inputs=expandCompositions(expanded.inputs,resolved.compositions);
    }
    ScenarioDefinition definition=expanded;
    definition.vehicleTypes=resolved.vehicleTypes;definition.behaviours=resolved.behaviours;
    // Best-effort, and deliberately NOT inside the block above: a document carrying its own
    // vehicle types and behaviours must stay portable to a machine with no data directory, which
    // is a contract a test already pins. These numbers are only needed when a priority rule has
    // to be DERIVED from the drawing, so a missing file is not an error here -- it is an error at
    // the point of use, where the alternative would be a zero gap time, a merge nobody gives way
    // at, invented in silence.
    try {
        const auto defaults = catalog(dataDirectory / "priority-rules",manifest);
        if (!defaults.empty()) definition.priorityDefaults = parsePriorityDefaults(defaults.front());
        else if(manifest)manifest->fallback("catalog/priority-rules");
    } catch (const std::exception&) {
        if(manifest)manifest->fallback("catalog/priority-rules"); // existing best-effort default
    }
    return definition;
}
ScenarioLoadError::ScenarioLoadError(std::filesystem::path path, std::string errorCode, const std::string& text)
    : std::runtime_error(path.string() + ": " + text), file(std::move(path)), code(std::move(errorCode)), detail(text) {}
LoadedScenario loadScenario(const std::filesystem::path& file, const std::filesystem::path& dataDirectory) {
    std::string code = "SCENARIO_FILE_READ";
    try {
        const auto value = readJson(file);
        code.clear();
        // Two file kinds share the .json extension: M0 authoring scenarios, which this window
        // runs, and version-1 editor projects, which it cannot. Say which one this is before
        // any field is read, or a project's legitimate "definition": null reads as corruption.
        if (!value.is_object()) code = "SCENARIO_NOT_JSON_OBJECT";
        else if (!present(value, "network")) code = "SCENARIO_NO_NETWORK";
        else if (value.contains("schemaVersion")) code = "SCENARIO_IS_PROJECT";
        else if (!present(value, "definition"))
            // Not value("format", ...): that throws type_error.302 when the key is present but
            // not a string, which is the very leak this classification exists to prevent.
            code = value.contains("schemaVersion") ||
                   (value.contains("format") && value.at("format").is_string() && value.at("format") == "TrafficSim")
                ? "SCENARIO_IS_PROJECT" : "SCENARIO_NO_DEFINITION";
        if (!code.empty()) throw std::invalid_argument(code);
        const auto& declared = section(value, "definition");
        // An M0 scenario carries no schemaVersion, so it is read with the pre-5 meaning.
        auto network = parseNetwork(section(value, "network"), 0);
        auto authored = parseAuthoringDefinition(declared);
        authored.provenance.legacyAmber = true; // M4.2 (D147): the frozen TS baselines' file kind
        // An M0 scenario names lanes in its routes, like a schema-7 project; the same migration
        // runs here, or the CLI and the editor would compile the same file two different ways.
        migrateRoutesToObjects(network, authored);
        if (auto issues = compositionIssues(authored, dataDirectory); !issues.empty())
            throw ValidationError(std::move(issues));
        auto definition = resolveCatalogs(authored,dataDirectory);
        auto scenario = compileScenario(network, definition);
        return {std::move(network), std::move(scenario)};
    } catch (const std::exception& error) {
        throw ScenarioLoadError(file, code, error.what());
    }
}
std::uint32_t parseSeed(const std::string& text) {
    std::uint32_t seed{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), seed);
    if (text.empty() || error != std::errc{} || end != text.data() + text.size())
        throw std::invalid_argument("Seed must be an unsigned 32-bit integer (0..4294967295)");
    return seed;
}
std::vector<std::uint32_t> parseSeedList(const std::string& text) {
    const auto fail = [&](const std::string& why) { return std::invalid_argument("Invalid seed list '" + text + "': " + why); };
    constexpr std::size_t limit = 1000;
    std::vector<std::uint32_t> seeds;
    const auto one = [&](const std::string& item) {
        try { return parseSeed(item); } catch (const std::invalid_argument&) { throw fail("'" + item + "' is not a seed (0..4294967295)"); }
    };
    std::size_t start = 0;
    while (true) {
        const auto comma = text.find(',', start);
        const auto item = text.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        const auto dash = item.find('-');
        const auto first = one(dash == std::string::npos ? item : item.substr(0, dash));
        const auto last = dash == std::string::npos ? first : one(item.substr(dash + 1));
        if (last < first) throw fail("range " + item + " runs backwards");
        if (last - first >= limit || seeds.size() + (last - first) >= limit) throw fail("more than 1000 seeds");
        for (auto seed = first;; ++seed) { seeds.push_back(seed); if (seed == last) break; }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    auto sorted = seeds;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) throw fail("a seed is repeated");
    return seeds;
}
std::filesystem::path findDataDirectory(const std::filesystem::path& executable) {
    std::vector<std::filesystem::path> candidates;
    const auto add = [&](const std::filesystem::path& binary) {
        std::error_code error;
        const auto resolved = std::filesystem::weakly_canonical(binary, error);
        if (!error) candidates.push_back(resolved.parent_path() / "data");
    };
    if (executable.has_parent_path()) add(executable);
    else if (const char* environment = std::getenv("PATH")) {
        const std::string paths(environment);
#ifdef _WIN32
        constexpr char separator = ';';
#else
        constexpr char separator = ':';
#endif
        std::size_t start = 0;
        do {
            const auto end = paths.find(separator, start);
            const auto binary = std::filesystem::path(paths.substr(start, end - start)) / executable;
            if (std::filesystem::is_regular_file(binary)) { add(binary); break; }
            if (end == std::string::npos) break;
            start = end + 1;
        } while (start <= paths.size());
    }
    candidates.push_back(std::filesystem::current_path() / "data");
    for (const auto& path : candidates)
        if (std::filesystem::is_regular_file(path / "scenarios" / "crossing.json")) return path;
    throw std::runtime_error("Cannot locate data directory; pass --data-dir <directory>");
}
}
