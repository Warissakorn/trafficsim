#include "load.hpp"
#include "json.hpp"
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
LoadedScenario loadScenario(const std::filesystem::path& file, const std::filesystem::path& dataDirectory) {
    try {
        const auto value = readJson(file);
        auto network = parseNetwork(value.at("network"));
        auto definition = parseDefinition(value.at("definition"));
        if (!value.at("definition").contains("vehicleTypes"))
            for (const auto& item : catalog(dataDirectory / "vehicle-types")) definition.vehicleTypes.push_back(parseVehicleType(item));
        if (!value.at("definition").contains("behaviours"))
            for (const auto& item : catalog(dataDirectory / "driver-behaviour")) definition.behaviours.push_back(parseBehaviour(item));
        auto scenario = compileScenario(network, definition);
        return {std::move(network), std::move(scenario)};
    } catch (const std::exception& error) {
        throw std::runtime_error(file.string() + ": " + error.what());
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
