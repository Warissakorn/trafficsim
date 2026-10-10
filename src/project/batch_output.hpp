#pragma once
#include "json.hpp"
#include "../runner/batch.hpp"

namespace trafficsim {
// The CSV warning lines a batch and a comparison share (BATCH §3, §5), so both describe a flag alike.
inline constexpr const char* kOverloadedWarning =
    "# WARNING: overloaded seeds (pending over 5% of generated) are included in the means:";
inline constexpr const char* kUnfinishedWarning = "# WARNING: unfinished trips over 5% of the movement; its delay reads low:";
// Batch output (M5.2): JSON for tools and a CSV whose first line is the not-validated marker.
// Both carry every seed's accounting and name the overloaded seeds the means include (D131).
Json batchJson(const BatchReport&, const std::vector<SeedRun>& runs);
std::string batchCsv(const BatchReport&, const std::vector<SeedRun>& runs);
}
