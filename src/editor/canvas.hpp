#pragma once
#include "../project/document.hpp"
#include "../model/network/display.hpp"
#include <QGraphicsView>
#include <QPainterPath>
#include <functional>
#include <optional>
#include <map>

namespace trafficsim {
class EditorCanvas : public QGraphicsView {
public:
    enum class Tool { select, draw, split, measure, calibrate, connect, route, input, head };
    explicit EditorCanvas(QWidget* parent = nullptr);
    void setDisplayCatalog(DisplayCatalog catalog) { display_=std::move(catalog); redraw(); }
    void setVisibleLevel(std::optional<int> level) { visibleLevel_=level; cancel(); }
    void setBackgroundVisible(bool visible) { backgroundVisible_=visible; redraw(); }
    void cycleOverlap();
    std::vector<std::pair<std::string,double>> hitObjects(Point,bool connectors = true) const;
    void setDocument(const ProjectDocument* document);
    void setTool(Tool tool);
    // One object is "primary": the last one added. Property edits act on it alone, so every
    // single-object gesture behaves exactly as it did before multi-selection existed.
    void select(const std::string& id);                   // replaces the selection with this object
    void setSelection(std::vector<std::string> ids);      // replaces; notifies once
    void toggle(const std::string& id);                   // Shift-click selection semantics
    void frame(const std::string& id);                    // centre it, zooming only if it does not fit
    const std::vector<std::string>& selection() const { return selection_; }
    std::string selected() const { return selection_.empty() ? std::string{} : selection_.back(); }
    bool isSelected(const std::string& id) const;
    std::vector<std::string> inRectangle(Point a, Point b) const;
    const Connector* selectedConnector() const;
    bool pickingConnectorTarget() const { return connectorFrom_.has_value(); }
    void setRunNetwork(const Network&);
    void setRunFrame(const SimState&);
    void clearRunFrame();
    std::size_t renderedVehicles() const { return runFrame_.vehicles.size(); }
    std::function<void()> stopRequested;
    std::function<void()> deleteRequested;
    void redraw();
    void fitNetwork();
    void cancel();
    void finishDrawing();
    void removeVertex();
    bool snap{true};
    double grid{1};
    std::function<void(const std::vector<Point>&)> createLinkGesture;
    std::function<void(LaneReference,LaneReference,const std::vector<Point>&)> createRangeGesture;
    std::function<void(LaneReference,Tool)> createDemandGesture;
    std::function<void(Point)> duplicateRequested;
    std::function<void(int,int,bool)> resizeRangeRequested;
    std::function<void(int,bool)> resizeLinkRequested;
    std::function<void()> creationRejected;
    std::function<void()> selectionChanged;
    std::function<void(const std::vector<Point>&)> createLink;
    std::function<void(const LaneReference&, const LaneReference&)> createConnector;
    // Dragging a connector's end grip onto another lane or another position along it.
    std::function<void(bool leading, LaneReference)> moveConnectorEndpoint;
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
    bool focusNextPrevChild(bool) override;
private:
    DisplayCatalog display_;
    std::optional<int> visibleLevel_;
    bool backgroundVisible_{true}, creating_{};
    Tool creationTool_{Tool::draw};
    QPoint creationStart_, copyStart_;
    std::string copyPick_;
    bool copyArmed_{}, copyDragging_{};
    Point copyOffset_{};
    void drawCopyPreview();
    QPainterPath objectShape(const std::string&) const;
    std::optional<std::pair<Point,int>> headPosition(const NetworkSignalHead&) const;
    std::optional<LaneReference> gestureFrom_;
    int rangeCorner_{}, previewFromCount_{1}, previewToCount_{1};
    Point lastPick_{};
    bool levelVisible(int level) const { return !visibleLevel_ || *visibleLevel_==level; }
    const DisplayType& style(const std::string&) const;
    std::optional<LaneReference> nearestLane(Point) const;
    void insertVertex(Point);
    void drawRunItems();
    std::vector<QGraphicsItem*> runItems_;
    std::map<std::string,int> runLevels_;
    std::map<std::string,std::string> runStyles_;
    SimState runFrame_;
    std::map<std::string,std::vector<Point>> runGeometry_;
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
    QPoint panStart_, dragPress_;
    Point dragStart_{};
    Point world(QPoint position, bool snapped = true) const;
    const Link* selectedLink() const;
    const std::vector<Point>* selectedGeometry() const;
    std::pair<std::string, double> hit(Point p, bool connectors = true) const;
    // Where the primary object's geometry points are shown: the middle of the whole bundle,
    // point for point with the stored polyline. Dragging maps back through the same offsets.
    std::vector<Point> handleGeometry() const;
    Point handleOffset_{};
    std::optional<bool> endpointDrag_;          // set while a connector end grip is held
    std::optional<LaneReference> endpointDraft_; // where that end would land
    std::optional<LaneReference> connectorEndpointTarget(Point, bool leading) const;
    int vertexAt(QPoint position) const;
    std::optional<LaneReference> hitLanePosition(Point p, bool outgoing) const;
    void pickConnector(Point p);
    void notifySelection();
    void drawConnectors();
    struct LaneHandle { Point position, anchor, direction; double width; int kind, count, maximum; };
    std::vector<LaneHandle> laneHandles() const;
    bool startLaneResize(QPoint);
    void updateLaneResize(QPoint);
    void drawLaneHandles();
    std::optional<LaneHandle> laneResize_;
    int previewLinkCount_{};
};
}
