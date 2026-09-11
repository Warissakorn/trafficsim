#pragma once
#include "types.hpp"

namespace trafficsim {
struct RoutePart { std::string segmentId; double start{}, length{}; };
struct VehicleLocation { std::string segmentId; double position{}; };
struct OccupiedSpan {
    std::uint64_t vehicleId{}; std::string segmentId; double rear{}, front{}, speed{};
};
std::vector<RoutePart> routeParts(const Scenario& scenario, const Route& route);
VehicleLocation locateVehicle(const Scenario& scenario, const Vehicle& vehicle);
std::vector<OccupiedSpan> occupiedSpans(const Scenario& scenario, const std::vector<Vehicle>& vehicles);
}
