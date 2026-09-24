#include "load.hpp"
#include "json.hpp"
#include "run.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace trafficsim {
namespace {
Json readJson(const std::filesystem::path& file) {
    std::ifstream stream(file);
    if (!stream) throw std::runtime_error("Cannot read JSON: " + file.string());
    // Parsing the entire stream rejects trailing garbage as well as malformed JSON.
    return Json::parse(stream);
}
std::vector<Json> catalog(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    std::vector<Json> result;
    for (const auto& file : files) result.push_back(readJson(file));
    return result;
}
}
std::vector<Composition> loadCompositions(const std::filesystem::path& dataDirectory) {
    std::vector<Composition> result;
    // A data directory with no compositions is legal: nothing may name one, and that is checked
    // where one is named. A composition file that does not parse is a broken catalog.
    if (!std::filesystem::is_directory(dataDirectory / "compositions")) return result;
    try {
        for (const auto& item : catalog(dataDirectory / "compositions")) result.push_back(parseComposition(item));
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    return result;
}
namespace {
bool validComposition(const Composition& c, const std::vector<VehicleType>& types) {
    if (c.types.empty()) return false;
    for (const auto& t : c.types) {
        if (!(std::isfinite(t.share) && t.share > 0)) return false;
        if (std::none_of(types.begin(), types.end(), [&](const auto& v) { return v.id == t.vehicleTypeId; }))
            return false;
    }
    return true;
}
const Composition* findComposition(const std::vector<Composition>& all, const std::string& id) {
    for (const auto& c : all) if (c.id == id) return &c;
    return nullptr;
}
}
std::vector<ValidationIssue> compositionIssues(const AuthoringDefinition& authored,
                                               const std::filesystem::path& dataDirectory) {
    std::vector<ValidationIssue> issues;
    if (std::none_of(authored.inputs.begin(), authored.inputs.end(),
                     [](const auto& i) { return !i.compositionId.empty(); })) return issues;
    const auto all = loadCompositions(dataDirectory);
    const auto types = resolveCatalogs(AuthoringDefinition{authored}, dataDirectory).vehicleTypes;
    for (std::size_t i = 0; i < authored.inputs.size(); ++i) {
        const auto& id = authored.inputs[i].compositionId;
        if (id.empty()) continue;
        const auto path = "inputs[" + std::to_string(i) + "].compositionId";
        const auto* c = findComposition(all, id);
        if (!c) { issues.push_back({"UNKNOWN_COMPOSITION", path}); continue; }
        if (c->types.empty() || std::any_of(c->types.begin(), c->types.end(),
                [](const auto& t) { return !(std::isfinite(t.share) && t.share > 0); }))
            issues.push_back({"INVALID_SHARE", path});
        else if (!validComposition(*c, types)) issues.push_back({"UNKNOWN_VEHICLE_TYPE", path});
    }
    return issues;
}
ScenarioDefinition resolveCatalogs(const AuthoringDefinition& authored, const std::filesystem::path& dataDirectory) {
    // Routing decisions first (M2.4): after this every input names one route, which is what the
    // composition split below and buildScenario's period and lane splits all expect.
    ScenarioDefinition definition=withRoutingDecisions(authored);
    try {
        if (authored.externalVehicleTypes) {
            definition.vehicleTypes.clear();
            for (const auto& item : catalog(dataDirectory / "vehicle-types"))
                definition.vehicleTypes.push_back(parseVehicleType(item));
        }
        if (authored.externalBehaviours) {
            definition.behaviours.clear();
            for (const auto& item : catalog(dataDirectory / "driver-behaviour"))
                definition.behaviours.push_back(parseBehaviour(item));
        }
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    // M2.3. Splitting a Poisson stream by fixed shares gives independent Poisson streams, so one
    // input per type is the composition exactly, not an approximation of it. Read only when
    // an input names one, so a document without compositions never touches the directory.
    if (std::any_of(authored.inputs.begin(), authored.inputs.end(),
                    [](const auto& i) { return !i.compositionId.empty(); })) {
        const auto all = loadCompositions(dataDirectory);
        std::vector<VehicleInput> inputs;
        for (const auto& input : definition.inputs) {
            if (input.compositionId.empty()) { inputs.push_back(input); continue; }
            const auto* c = findComposition(all, input.compositionId);
            if (!c || !validComposition(*c, definition.vehicleTypes)) continue; // compositionIssues names it
            double sum = 0;
            for (const auto& t : c->types) sum += t.share;
            for (const auto& t : c->types) {
                auto part = input;
                part.compositionId.clear();
                part.vehicleTypeId = t.vehicleTypeId;
                if (c->types.size() > 1) part.id = input.id + "/type-" + t.vehicleTypeId;
                const double fraction = t.share / sum;
                part.vehiclesPerHour = input.vehiclesPerHour * fraction;
                for (auto& period : part.intervals) period.vehiclesPerHour *= fraction;
                inputs.push_back(std::move(part));
            }
        }
        definition.inputs = std::move(inputs);
    }
    // Best-effort, and deliberately NOT inside the block above: a document carrying its own
    // vehicle types and behaviours must stay portable to a machine with no data directory, which
    // is a contract a test already pins. These numbers are only needed when a priority rule has
    // to be DERIVED from the drawing, so a missing file is not an error here -- it is an error at
    // the point of use, where the alternative would be a zero gap time, a merge nobody gives way
    // at, invented in silence.
    try {
        const auto defaults = catalog(dataDirectory / "priority-rules");
        if (!defaults.empty()) definition.priorityDefaults = parsePriorityDefaults(defaults.front());
    } catch (const std::exception&) { /* Left at zero; refused where it is needed. */ }
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
