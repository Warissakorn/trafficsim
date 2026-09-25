#pragma once
#include "../project/document.hpp"
#include "../model/network/display.hpp"
#include <QGraphicsView>
#include <QPainterPath>
#include <functional>
#include <optional>
#include <map>

class QTimer;

namespace trafficsim {
class EditorCanvas : public QGraphicsView {
public:
    enum class Tool { select, draw, split, measure, calibrate, connect, route, input, head, conflict };
    explicit EditorCanvas(QWidget* parent = nullptr);
    void setDisplayCatalog(DisplayCatalog catalog) { display_=std::move(catalog); redraw(); }
    void setVisibleLevel(std::optional<int> level);
    // Explicit selection from a table/inspector reveals hidden objects. Changing the
    // level filter instead drops hidden selections before they can be edited.
    std::function<void(std::optional<int>)> visibleLevelChanged;
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
    void cancel();                                        // forget the gesture AND repaint
    void finishDrawing();
    void removeVertex();
    bool snap{true};
    double grid{1};
    std::function<void(const std::vector<Point>&)> createLinkGesture;
    std::function<void(LaneReference,LaneReference,const std::vector<Point>&)> createRangeGesture;
    // The head tool's click: the lane or Connector path under the pointer and the station on
    // it, which is where the stop line goes. A drag of a selected head slides it along its lane.
    std::function<void(const HeadPlacement&)> headPlaced;
    std::function<void(const std::string&, double)> headMoved;
    // A route drawn by pointer: the segments, in travel order, exactly as the dialog would
    // have stored them. The canvas never writes to the document itself.
    std::function<void(std::vector<std::string>)> routeDraftCommitted;
    std::function<void(std::string)> inputPlaced;
    const std::vector<std::string>& routeDraft() const { return routeDraft_; }
    void commitRouteDraft();
    void dropLastRouteSegment();
    // Which route is drawn on the canvas. The shell owns table selection, so it says.
    void setHighlightedRoute(std::string id);
    // Begin a route at this segment without a click: the input tool offers it when no route
    // starts where the author wants an input.
    void startRouteDraft(const std::string& objectId);
    // The demand object drawn under this viewport position, for the context menu. Returns the
    // id of a vehicle input or of the drawn route, and an empty string for anything else.
    std::pair<std::string,std::string> demandObjectAt(QPoint viewportPosition) const;
    std::function<void(QPoint)> contextMenuRequested;
    const std::string& highlightedRoute() const { return highlightedRoute_; }
    // M3.2.4: the conflict area the shell's table has selected, drawn outlined. Areas are shown
    // green where a side has priority, red where it gives way and amber while undetermined.
    void setHighlightedConflict(std::string id);
    const std::string& highlightedConflict() const { return highlightedConflict_; }
    // M3.2.4b, the Conflict area tool (docs/VISSIM_PARITY.md §2b). Only that tool hit-tests areas,
    // so a click at a junction under Select still selects the Link. A click picks an area; a click
    // on the highlighted one asks to cycle its priority; a drag on a waiting line slides it along
    // its own path. The canvas never writes to the document itself.
    std::function<void(const std::string&)> conflictPicked, conflictCycled;
    std::function<void(const std::string&, double)> waitingLineMoved;
    std::vector<std::string> conflictsAt(Point) const;   // areas whose drawn side contains it, by id
    // Paint state only, advanced by a timer. Tests set it directly: waiting on wall clock for
    // an animation is how a suite becomes flaky, and no measured number depends on it.
    void setAnimationPhase(int phase);
    int animationPhase() const { return animationPhase_; }
    std::function<void(Point)> duplicateRequested;
    std::function<void(Point)> translateRequested;
    std::function<void(Point,double)> rotateRequested;
    std::optional<Point> rotationPivot() const;
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
    void focusOutEvent(QFocusEvent*) override;
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
    // A drag on a multi-selection moves the whole of it. groupDrag_ is armed on the press;
    // groupDragging_ turns on once the pointer has travelled far enough to be a drag and not
    // a click, which is what keeps a plain click on a selected object from moving anything.
    bool groupDrag_{}, groupDragging_{};
    Point groupOffset_{};
    std::optional<Point> rotationPivot_;
    Point rotationStart_{};
    double rotationDegrees_{};
    bool rotationDragging_{};
    void startRotation(QPoint);
    void updateRotation(QPoint, bool angleSnap);
    void drawRotationPreview();
    void drawDemandOverlay();
    void drawConflicts();
    std::string highlightedConflict_;
    std::string waitingLineAt(Point) const;
    bool conflictPress(QMouseEvent*);
    void updateLineDrag(QPoint);
    void finishLineDrag(QPoint);
    struct LineDrag { std::string id; std::vector<Point> polyline; double original{}, station{}; bool moved{}; };
    std::optional<LineDrag> lineDrag_;
    void drawRouteArrows(const std::vector<Point>&, QColor);
    std::string objectAt(Point) const;
    bool isLink(const std::string& objectId) const;
    std::vector<std::string> routeDraftWith(const std::string& target) const;
    bool demandPress(QMouseEvent*);
    bool demandHover(QMouseEvent*);
    void clearRouteDraft();
    void reject();
    void animate();
    bool animating() const;
    std::vector<std::string> routeDraft_;
    std::vector<std::vector<Point>> pulseGeometry_;
    std::string hoverSegment_, highlightedRoute_;
    bool hoverReachable_{};
    Point hoverPoint_{};
    int animationPhase_{}, commitPulse_{}, rejectPulse_{};
    QTimer* animation_{};
    // Connector geometry is the most expensive thing a frame does, and a frame recomputed all
    // of it even when nothing had moved. What `connectorPaths` and `connectorBoundaries` read is
    // exactly the Connector, the two Links it names and the driving side -- nothing else in the
    // Network can change their answer -- so those values ARE the cache key, compared by value
    // rather than trusted from a revision counter. A preview Connector simply misses.
    struct CachedConnector {
        Connector connector; Link from, to; DrivingSide side{};
        std::optional<std::vector<ConnectorPath>> paths;
        std::optional<std::vector<std::vector<Point>>> boundaries;
        std::optional<std::vector<ConnectorMarking>> markings;
    };
    mutable std::map<std::string,CachedConnector> connectorCache_;
    CachedConnector& connectorEntry(const Connector&) const;
    void pruneConnectorCache();
    const std::vector<ConnectorPath>& cachedPaths(const Connector&) const;
    const std::vector<std::vector<Point>>& cachedBoundaries(const Connector&) const;
    const std::vector<ConnectorMarking>& cachedMarkings(const Connector&) const;
    void drawCopyPreview();
    QPainterPath objectShape(const std::string&) const;
    std::optional<std::pair<Point,int>> headPosition(const NetworkSignalHead&) const;
    struct HeadGeometry { std::vector<Point> points; double width{}; int level{}; };
    std::optional<HeadGeometry> headGeometry(const NetworkSignalHead&) const;
    std::optional<HeadPlacement> headAt(Point) const;
    QPainterPath headShape(const NetworkSignalHead&) const;
    bool headPress(QMouseEvent*);
    bool headHover(QMouseEvent*);
    bool startHeadDrag(const std::string& id, QPoint press);
    void updateHeadDrag(QPoint);
    void finishHeadDrag(QPoint);
    void drawHeads();
    struct HeadDrag { std::string id; std::vector<Point> geometry; double original{}, station{}; bool moved{}; };
    std::optional<HeadDrag> headDrag_;
    std::optional<HeadPlacement> hoverHead_;
    std::optional<LaneReference> gestureFrom_;
    int rangeCorner_{}, previewFromCount_{1}, previewToCount_{1};
    Point lastPick_{};
    bool levelVisible(int level) const { return !visibleLevel_ || *visibleLevel_==level; }
    std::optional<int> objectLevel(const std::string&) const;
    bool mouseGestureActive() const;
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
    QPoint panStart_, panPress_, dragPress_;
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
    // cancel() without the repaint, for callers that redraw for their own reasons anyway.
    void resetGesture();
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
