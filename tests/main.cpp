#include "test.hpp"
#include <algorithm>
#include <iostream>

std::vector<test::Case>& test::cases() { static std::vector<Case> value; return value; }
int main(int argc, char** argv) {
    // `--check-groups a b c` runs nothing: it fails when a registered group is missing from the
    // list, so a new group can never silently fall out of the ctest list (previously "points").
    if (argc > 1 && std::string(argv[1]) == "--check-groups") {
        const std::vector<std::string> listed(argv + 2, argv + argc);
        unsigned missing = 0;
        std::vector<std::string> seen;
        for (const auto& item : test::cases()) {
            if (std::find(seen.begin(), seen.end(), item.group) != seen.end()) continue;
            seen.push_back(item.group);
            if (std::find(listed.begin(), listed.end(), item.group) == listed.end()) {
                ++missing; std::cerr << "UNLISTED group " << item.group << '\n';
            }
        }
        std::cout << seen.size() << " groups, " << missing << " unlisted\n";
        return missing || seen.empty() ? 1 : 0;
    }
    unsigned passed = 0, failed = 0;
    for (const auto& item : test::cases()) {
        if (argc > 1 && item.group != argv[1]) continue;
        try { item.run(); ++passed; std::cout << "PASS " << item.group << "." << item.name << '\n'; }
        catch (const std::exception& error) {
            ++failed; std::cerr << "FAIL " << item.group << "." << item.name << ": " << error.what() << '\n';
        }
    }
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed || !passed ? 1 : 0;
}
