#include "load.hpp"
#include "json.hpp"
#include "run.hpp"
#include <algorithm>
#include <charconv>
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
ScenarioDefinition resolveCatalogs(const AuthoringDefinition& authored, const std::filesystem::path& dataDirectory) {
    ScenarioDefinition definition=authored;
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
        auto definition = resolveCatalogs(parseAuthoringDefinition(declared),dataDirectory);
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
