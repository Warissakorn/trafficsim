#include "network.hpp"
#include <cmath>
#include <stdexcept>

namespace trafficsim {
std::vector<Point> connectorCurve(const Network& network, const LaneReference& from, const LaneReference& to) {
    const auto resolve = [&](const LaneReference& ref) {
        for (const auto& link : network.links) if (link.id == ref.linkId)
            return laneGeometry(link, ref.laneId, network.drivingSide);
        throw std::invalid_argument("UNKNOWN_LANE");
    };
    const auto source = resolve(from), target = resolve(to);
    if (source.size() < 2 || target.size() < 2) throw std::invalid_argument("INVALID_GEOMETRY");
    const auto a = source.back(), b = target.front();
    const double gap = std::hypot(b.x-a.x, b.y-a.y);
    if (!std::isfinite(gap) || gap < 1e-6) throw std::invalid_argument("EDIT_CONNECTOR_GAP");
    const auto control = [&](Point origin, Point previous, Point next, double sign) {
        const double dx = next.x-previous.x, dy = next.y-previous.y, length = std::hypot(dx, dy);
        if (!std::isfinite(length) || length <= 0) throw std::invalid_argument("INVALID_GEOMETRY");
        return Point{origin.x + sign*dx/length*gap/3, origin.y + sign*dy/length*gap/3};
    };
    const auto c1 = control(a, source[source.size()-2], a, 1);
    const auto c2 = control(b, b, target[1], -1);
    std::vector<Point> points{a};
    for (int i = 1; i < 12; ++i) {
        const double t = i/12.0, s = 1-t;
        points.push_back({s*s*s*a.x + 3*s*s*t*c1.x + 3*s*t*t*c2.x + t*t*t*b.x,
                          s*s*s*a.y + 3*s*s*t*c1.y + 3*s*t*t*c2.y + t*t*t*b.y});
    }
    points.push_back(b);
    return points;
}
}
