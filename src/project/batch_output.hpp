#pragma once
#include "json.hpp"
#include "../runner/runner.hpp"
#include <string>

namespace trafficsim {
// M5.2 (D136): a batch report as the CLI writes it. Both carry the not-validated statement first,
// every Estimate with its n, and one accounting row per seed (completed, still in the network,
// waiting to enter): a seed that ends congested is shown, never averaged away silently.
Json batchJson(const BatchReport&);
std::string batchCsv(const BatchReport&);
}
