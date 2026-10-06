#pragma once
#include "../model/network/network.hpp"
#include <QGraphicsLineItem>
#include <QColor>

namespace trafficsim {
struct RoadCrossbar { Point first, second; int level{}; };
// Presentation only: intersect the road normal with its real rails at this station.
std::optional<RoadCrossbar> roadCrossbar(const std::vector<Point>& centre,
    const std::vector<Point>& firstRail, const std::vector<Point>& secondRail,
    double station, int level,
    const std::vector<Point>& firstContinuation = {}, const std::vector<Point>& secondContinuation = {});
std::optional<RoadCrossbar> objectCrossbar(const Network&, const std::string& id, bool end);
std::optional<RoadCrossbar> signalCrossbar(const Network&, const NetworkSignalHead&);
// The hit region is wider than the visible cosmetic stroke, at every zoom.
class RoadCrossbarItem : public QGraphicsLineItem {
public:
    RoadCrossbarItem(const RoadCrossbar&, QColor, double scale, bool dashed = false);
    QPainterPath shape() const override;
    QRectF boundingRect() const override;
private:
    double hitWidth_{};
};
}
