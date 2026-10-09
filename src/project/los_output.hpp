#pragma once
#include "input_manifest.hpp"
#include "../eval/los.hpp"
#include <filesystem>
#include <iosfwd>

namespace trafficsim {
// M5.5 (D134, docs/reference/LOS.md). data/los/hcm.json; EDIT_CATALOG_READ unless it has an id and
// five finite, positive, strictly increasing bounds for each of signalised and unsignalised, and
// nothing else.
LosPack loadLosPack(const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// A section's letter as written: the letter, or empty without a type, delay or pack.
std::string losCell(const std::optional<double>& delay, const std::optional<std::string>& controlType,
                    const std::optional<LosPack>&);
// With any section type: `los {pack, approaches, intersections}` in JSON, and in CSV the marker
// line and the `level,name,controlType,vehicles,delay_s,los` block. Nothing otherwise.
void addLosJson(Json&, const std::vector<LosInput>&, const std::optional<LosPack>&);
void writeLosCsv(std::ostream&, const std::vector<LosInput>&, const std::optional<LosPack>&);
}
