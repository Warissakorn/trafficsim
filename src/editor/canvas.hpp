#pragma once
#include "../project/document.hpp"
#include <QGraphicsView>
#include <functional>

namespace trafficsim {
class EditorCanvas : public QGraphicsView {
public:
    enum class Tool { select, draw, split, measure, calibrate };
    explicit EditorCanvas(QWidget* parent = nullptr);
    void setDocument(const ProjectDocument* document);
    void setTool(Tool tool);
    void select(const std::string& id);
    std::string selected() const { return selected_; }
    void redraw();
    void fitNetwork();
    void cancel();
    void finishDrawing();
    void removeVertex();
    bool snap{true};
    double grid{1};
    std::function<void()> selectionChanged;
    std::function<void(const std::vector<Point>&)> createLink;
    std::function<void(const std::string&, const std::vector<Point>&)> editGeometry;
    std::function<void(const std::string&, double)> splitAt;
    std::function<void(Point, Point, bool)> measured;
    std::function<void(Point)> cursorMoved;
protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void drawBackground(QPainter*, const QRectF&) override;
private:
    const ProjectDocument* document_{};
    QGraphicsScene scene_;
    std::shared_ptr<const std::string> cachedImage_;
    QPixmap image_;
    Tool tool_{Tool::select};
    std::string selected_;
    std::vector<Point> draft_, preview_, original_;
    int vertex_{-1};
    bool dragging_{}, panning_{};
    QPoint panStart_;
    Point dragStart_{};
    Point world(QPoint position, bool snapped = true) const;
    const Link* selectedLink() const;
    std::pair<std::string, double> hit(Point p) const;
};
}
