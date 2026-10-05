#pragma once
#include "rear_axle_pose.hpp"

namespace trafficsim {
// A complete display journey from the first change, including overlapping changes
// and the subsequent settling of body heading. Progress is longitudinal travel,
// not elapsed time: a stopped vehicle holds its entire pose. Engine lane occupancy
// remains instantaneous. Missing/inconsistent traces or geometry have no pose.
class LaneChangePath {
public:
    LaneChangePath(const Scenario&, const Vehicle&,
        const std::map<std::string,std::vector<Point>>& geometry,
        const std::map<std::string,ConnectorEquation>& equations, double maxStep=.1);
    bool matches(const Vehicle&) const;
    std::optional<AxleVehiclePose> pose(const Vehicle&) const;
private:
    struct Sample {double travel, arc;Point front;};
    std::vector<LaneChangeTrace> trace_;
    std::uint32_t type_{};
    double offset_{};
    std::vector<Sample> samples_;
    std::optional<RearAxlePath> rolling_;
};
}
