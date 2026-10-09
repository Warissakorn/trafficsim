#pragma once
#include "json.hpp"
#include "../runner/batch.hpp"

namespace trafficsim {
// Batch output (M5.2): JSON for tools and a CSV whose first line is the not-validated marker.
// Both carry every seed's accounting and name the overloaded seeds the means include (D131).
Json batchJson(const BatchReport&, const std::vector<SeedRun>& runs);
std::string batchCsv(const BatchReport&, const std::vector<SeedRun>& runs);
}
