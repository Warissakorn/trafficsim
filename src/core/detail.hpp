#pragma once
#include "types.hpp"
#include <algorithm>
#include <stdexcept>

namespace trafficsim::detail {
template<class T>
const T& byId(const std::vector<T>& items, const std::string& id) {
    const auto found = std::find_if(items.begin(), items.end(),
                                    [&](const T& item) { return item.id == id; });
    if (found == items.end()) throw std::logic_error("Unknown runtime ID: " + id);
    return *found;
}
double random(std::uint32_t& state);
void initializeInputs(SimState& state);
void generateArrivals(SimState& state);
}
