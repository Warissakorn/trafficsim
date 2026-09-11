#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: trafficsim-check-file-sizes <repository>\n"; return 2; }
    const std::set<std::string> extensions{".cpp", ".hpp", ".h", ".md", ".cmake"};
    bool valid = true;
    for (auto it = std::filesystem::recursive_directory_iterator(argv[1]); it != std::filesystem::recursive_directory_iterator(); ++it) {
        const auto name = it->path().filename().string();
        if (it->is_directory()) {
            if (name.starts_with('.') || name.starts_with("build") || name == "node_modules" || name == "dist" || name == "out")
                it.disable_recursion_pending();
            continue;
        }
        if (!extensions.contains(it->path().extension().string()) && name != "CMakeLists.txt") continue;
        std::ifstream file(it->path());
        std::string line; unsigned count = 0;
        while (std::getline(file, line)) ++count;
        if (count > 500) { std::cerr << it->path() << ": " << count << " lines (limit 500)\n"; valid = false; }
    }
    return valid ? 0 : 1;
}
