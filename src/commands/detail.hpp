#pragma once
#include "../project/document.hpp"
#include <set>

namespace trafficsim::detail {
void removeRoutesUsingSegments(ProjectDocument&, const std::set<std::string>& segments);
}
