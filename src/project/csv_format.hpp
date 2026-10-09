#pragma once
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>

namespace trafficsim {
// The cell formats every results CSV shares, so a single run and a batch read alike: names quoted
// with doubled quotes, numbers C-locale with two decimals, a missing value an empty cell.
inline std::string csvQuoted(const std::string& text) {
    std::string out = "\"";
    for (const char c : text) { if (c == '"') out += '"'; out += c; }
    return out + '"';
}
inline std::string csvNumber(const std::optional<double>& value) {
    if (!value) return "";
    std::ostringstream s; s.imbue(std::locale::classic()); s << std::fixed << std::setprecision(2) << *value; return s.str();
}
}
