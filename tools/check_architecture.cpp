#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>

namespace {
// Deliberately restricted include syntax: literal same-directory headers and a
// reviewed set of standard headers. Macro includes and modules fail closed.
bool check(const std::string& source, const std::filesystem::path& directory) {
    static const std::set<std::string> standard{
        "algorithm", "cmath", "cstdint", "functional", "limits", "map", "memory", "numbers",
        "optional", "set", "stdexcept", "string", "utility", "variant", "vector", "type_traits"};
    const std::regex directive(R"(^\s*#\s*(include|include_next)\b.*)");
    const std::regex literal(R"(^\s*#\s*include\s*([<"])([^>"\r\n]+)[>"]\s*(//.*)?$)");
    const std::regex forbidden(R"(\b(import|random_device|rand|srand|system_clock|steady_clock|unordered_map|unordered_set|__TIME__|__DATE__)\b)");
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        // Comments are not executable; full C++ parsing is intentionally not claimed.
        const auto code = line.substr(0, line.find("//"));
        if (std::regex_search(code, forbidden)) return false;
        if (!std::regex_match(line, directive)) continue;
        std::smatch match;
        if (!std::regex_match(line, match, literal)) return false;
        const std::string header = match[2];
        if (match[1] == "<") { if (!standard.contains(header)) return false; }
        else if (header.find_first_of("/\\") != std::string::npos ||
                 std::filesystem::path(header).extension() != ".hpp" ||
                 !std::filesystem::is_regular_file(directory / header)) return false;
    }
    return true;
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    if (std::string(argv[1]) == "--self-test") {
        for (const auto* bad : {"#include <QWidget>", "#include <fstream>", "#include <random>",
                "#include \"../model/network.hpp\"", "#include HEADER", "#include_next <vector>",
                "import bad.module;", "std::rand();", "auto x = __TIME__;"})
            if (check(bad, {})) { std::cerr << "Guard accepted: " << bad << '\n'; return 1; }
        return check("#include <vector>\n#include <cmath>\n", {}) ? 0 : 1;
    }
    bool valid = true;
    for (const auto* module : {"core", "eval"}) {
        const auto directory = std::filesystem::path(argv[1]) / "src" / module;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.path().extension() != ".hpp" && entry.path().extension() != ".cpp") continue;
            std::ifstream file(entry.path());
            std::string source((std::istreambuf_iterator<char>(file)), {});
            // eval is allowed the core contract, never model/project/Qt.
            if (std::string(module) == "eval") {
                const std::string allowed = "#include \"../core/types.hpp\"";
                for (auto pos = source.find(allowed); pos != std::string::npos; pos = source.find(allowed))
                    source.replace(pos, allowed.size(), "#include <vector>");
            }
            if (!file || !check(source, directory)) { std::cerr << "Boundary violation: " << entry.path() << '\n'; valid = false; }
        }
    }
    return valid ? 0 : 1;
}
