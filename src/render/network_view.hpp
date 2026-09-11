#pragma once
#include "../model/network/network.hpp"
#include <QWidget>
#include <map>

namespace trafficsim {
class NetworkView : public QWidget {
public:
    explicit NetworkView(QWidget* parent = nullptr);
    void setNetwork(const Network& network);
    void setFrame(const SimState& state);
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    Network network_;
    SimState frame_;
    std::map<std::string, std::vector<Point>> geometry_;
    QRectF bounds_;
};
}
