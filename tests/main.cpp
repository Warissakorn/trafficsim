#include "test.hpp"
#include <iostream>

std::vector<test::Case>& test::cases() { static std::vector<Case> value; return value; }
int main(int argc, char** argv) {
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
