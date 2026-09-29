#include "signal_timing_view.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../model/demand/signal_control.hpp"
#include <QPainter>
#include <cmath>
#include <utility>

namespace trafficsim {
SignalTimingView::SignalTimingView(QWidget* parent) : QWidget(parent) {
    setObjectName("editorSignalTiming");
    setMinimumHeight(60);
}
void SignalTimingView::setController(SignalController controller) {
    controller_ = std::move(controller);
    updateGeometry(); update();
}
void SignalTimingView::setStatusLabels(QString green,QString amber,QString red) {
    greenLabel_=std::move(green);amberLabel_=std::move(amber);redLabel_=std::move(red);update();
}
QSize SignalTimingView::sizeHint() const {
    return {560, 48 + 22 * static_cast<int>(std::max<std::size_t>(1, controller_.groups.size()))};
}
void SignalTimingView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setFont(editorDesign::numericFont());
    p.fillRect(rect(), palette().base());
    const int label = 90, top = 4, row = 22, axis = 20;
    const int width = std::max(1, this->width() - label - 8);
    if (!(controller_.cycle > 0) || !std::isfinite(controller_.cycle)) return;
    // Cycle seconds, not simulation time: the offset shifts when the cycle starts, not its shape.
    auto shape = controller_; shape.offset = 0;
    const auto colour = [](SignalColor c) {
        using editorDesign::Semantic;
        return editorDesign::semantic(c == SignalColor::green ? Semantic::ok : c == SignalColor::amber ? Semantic::warning : Semantic::error);
    };
    for (std::size_t k = 0; k < shape.groups.size(); ++k) {
        const auto& g = shape.groups[k];
        const int y = top + static_cast<int>(k) * row;
        p.setPen(palette().text().color());
        p.drawText(QRect(0, y, label - 6, row - 4), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(g.number) + (g.name.empty() ? QString() : "  " + QString::fromStdString(g.name)));
        // One sample per pixel column, merged into runs: exact to a pixel at any cycle length.
        int start = 0; SignalColor current = signalGroupColorAt(shape, g, 0);
        for (int x = 1; x <= width; ++x) {
            const auto next = x == width ? current : signalGroupColorAt(shape, g, controller_.cycle * (x + .5) / width);
            if (next != current || x == width) {
                p.fillRect(label + start, y + 2, x - start, row - 6, colour(current));
                start = x; current = next;
            }
        }
    }
    const int y = top + static_cast<int>(shape.groups.size()) * row;
    p.setPen(palette().mid().color());
    p.drawLine(label, y, label + width, y);
    double step = 5;
    while (controller_.cycle / step > 12) step *= 2;
    p.setPen(palette().text().color());
    for (double s = 0; s <= controller_.cycle + 1e-9; s += step) {
        const int x = label + static_cast<int>(std::round(s / controller_.cycle * width));
        p.drawLine(x, y, x, y + 4);
        p.drawText(QRect(x - 20, y + 4, 40, axis - 4), Qt::AlignHCenter | Qt::AlignTop, QString::number(s));
    }
    const int legendY=y+24;
    const QString symbols[]={QStringLiteral("G"),QStringLiteral("Y"),QStringLiteral("R")};
    const QString labels[]={greenLabel_,amberLabel_,redLabel_};
    const QColor status[]={editorDesign::semantic(editorDesign::Semantic::ok),editorDesign::semantic(editorDesign::Semantic::warning),
                           editorDesign::semantic(editorDesign::Semantic::error)};
    for(int i=0;i<3;++i) {
        const int x=label+i*96;p.fillRect(QRect(x,legendY+3,8,8),status[i]);
        p.setPen(editorDesign::role(QPalette::Text));
        p.drawText(QRect(x+12,legendY,80,14),Qt::AlignLeft|Qt::AlignVCenter,symbols[i]+" · "+labels[i]);
    }
}
}
