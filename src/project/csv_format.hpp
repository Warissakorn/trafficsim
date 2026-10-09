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
// M5.6 Copy: one of the CSVs above as tab-separated values, so a paste into a spreadsheet or a
// Word table lands column by column. A `#` comment or blank line is kept as written; any other
// line splits at its unquoted commas and loses the quotes (a doubled quote becomes one). A tab
// inside a name would start a new column, and a line break a new row, so each becomes a space.
inline std::string csvToTsv(const std::string& csv) {
    std::string out;
    bool quoted = false, comment = false, lineStart = true;
    for (std::size_t i = 0; i < csv.size(); ++i) {
        const char c = csv[i];
        if (lineStart) { comment = c == '#'; lineStart = false; }
        if (c == '\n' && !quoted) { out += c; lineStart = true; continue; }
        if (comment) { out += c; continue; }
        if (c == '"') {
            if (quoted && i + 1 < csv.size() && csv[i + 1] == '"') { out += '"'; ++i; }
            else quoted = !quoted;
        } else if (c == ',' && !quoted) out += '\t';
        else out += c == '\t' || c == '\n' ? ' ' : c; // a line break inside a quoted name too
    }
    return out;
}
}
