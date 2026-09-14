#pragma once
#include "../project/document.hpp"
#include <QGraphicsView>
#include <functional>
#include <optional>

namespace trafficsim {
class EditorCanvas : public QGraphicsView {
public:
    enum class Tool { select, draw, split, measure, calibrate, connect };
    explicit EditorCanvas(QWidget* parent = nullptr);
    void setDocument(const ProjectDocument* document);
    void setTool(Tool tool);
    // One object is "primary": the last one added. Property edits act on it alone, so every
    // single-object gesture behaves exactly as it did before multi-selection existed.
    void select(const std::string& id);                   // replaces the selection with this object
    void setSelection(std::vector<std::string> ids);      // replaces; notifies once
    void toggle(const std::string& id);                   // Ctrl/Shift-click semantics
    void frame(const std::string& id);                    // centre it, zooming only if it does not fit
    const std::vector<std::string>& selection() const { return selection_; }
    std::string selected() const { return selection_.empty() ? std::string{} : selection_.back(); }
    bool isSelected(const std::string& id) const;
    std::vector<std::string> inRectangle(Point a, Point b) const;
    const Connector* selectedConnector() const;
    bool pickingConnectorTarget() const { return connectorFrom_.has_value(); }
    void redraw();
    void fitNetwork();
    void cancel();
    void finishDrawing();
    void removeVertex();
    bool snap{true};
    double grid{1};
    std::function<void()> selectionChanged;
    std::function<void(const std::vector<Point>&)> createLink;
    std::function<void(const LaneReference&, const LaneReference&)> createConnector;
    std::function<void(const LaneReference&)> connectorSourcePicked;
    std::function<void()> connectorDraftChanged;
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
    std::vector<std::string> selection_;
    std::optional<QRectF> band_;
    bool additive_{};
    std::vector<Point> draft_, preview_, original_;
    std::optional<LaneReference> connectorFrom_, connectorHover_;
    int vertex_{-1};
    bool dragging_{}, panning_{};
    QPoint panStart_;
    Point dragStart_{};
    Point world(QPoint position, bool snapped = true) const;
    const Link* selectedLink() const;
    const std::vector<Point>* selectedGeometry() const;
    std::pair<std::string, double> hit(Point p, bool connectors = true) const;
    int vertexAt(QPoint position) const;
    std::optional<LaneReference> hitLaneEnd(Point p, bool outgoing) const;
    void pickConnector(Point p);
    void notifySelection();
    void drawConnectors();
};
}
