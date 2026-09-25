#pragma once
#include "../model/demand/definition.hpp"
#include <QWidget>

namespace trafficsim {
// M2.7b. The timing-bar diagram a signal controller dialog shows: one row per signal group across
// one cycle, in cycle seconds, painted from the same colour rule the run's expanded program
// follows (signalGroupColorAt), so the picture cannot disagree with the simulation.
class SignalTimingView : public QWidget {
public:
    explicit SignalTimingView(QWidget* parent = nullptr);
    void setController(SignalController controller);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    SignalController controller_;
};
}
