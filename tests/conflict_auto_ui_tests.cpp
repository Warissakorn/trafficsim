#include "../src/shell/editor_window.hpp"
#include "../tools/t_junction_network.hpp"
#include "../src/model/network/right_of_way.hpp"
#include "../src/model/network/conflict_display.hpp"
#include "../src/model/network/connector_surface.hpp"
#include "../src/commands/connector_commands.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <set>
using namespace trafficsim;
// M3.2.4c (D68): automatic conflict areas in the editor, as the owner asked -- they appear where
// roads overlap with no Add step; the Conflict area tool only sets priority. A passive crossing is
// authored by its first click (or P on its row) and made passive again by Delete; a derived merge is
// taken over by a click. Each is one Undo step, and nothing automatic is ever saved.
namespace {
// Printed before the throw: a window left with unsaved edits can abort while it unwinds.
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a, name); return a; }
QPoint pixel(EditorWindow& w, Point p) {
    const auto q = w.canvas()->mapFromScene(p.x, p.y);
    require(w.canvas()->viewport()->rect().contains(q), "Point outside the viewport");
    return q;
}
void click(EditorWindow& w, Point p) {
    QTest::qWait(600); // never let two clicks become a double-click
    QTest::mouseClick(w.canvas()->viewport(), Qt::LeftButton, {}, pixel(w, p)); QApplication::processEvents();
}
// D84: Ctrl+right-click authors or changes; a left click only selects.
void author(EditorWindow& w, Point p) {
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::RightButton, Qt::ControlModifier, pixel(w, p)); QApplication::processEvents();
}
void key(EditorWindow& w, Qt::Key k) { w.canvas()->setFocus(); QTest::keyClick(w.canvas(), k); QApplication::processEvents(); }
int drawn(EditorWindow& w, const char* kind) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == "auto-conflict" && i->data(2).toString() == kind;
    return n;
}
Point centre(const std::vector<Point>& outline) {
    Point c{};
    for (const auto& p : outline) { c.x += p.x / outline.size(); c.y += p.y / outline.size(); }
    return c;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        fixture::TJunctionOptions o; o.authoredControls = false;
        const auto t = fixture::tJunction(o);
        require(t.document.network.rightOfWay.empty(), "The drawing already has authored controls");
        const auto file = temp.filePath("bare.traffic.json");
        {
            QFile f(file); require(f.open(QIODevice::WriteOnly), "Fixture open failed");
            const auto bytes = QByteArray::fromStdString(documentJson(t.document).dump());
            require(f.write(bytes) == bytes.size(), "Fixture write failed");
        }
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        c->fitNetwork(); c->centerOn(0, 0); c->scale(3, 3); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorConflictTable"); require(table, "No conflict table");
        const auto& network = [&]() -> const Network& { return w.history().document().network; };
        const auto all = automaticConflicts(network());
        const auto passive = *std::find_if(all.begin(), all.end(), [](const auto& a) { return a.kind == ConflictKind::crossing; });
        const auto merge = *std::find_if(all.begin(), all.end(), [](const auto& a) { return a.kind == ConflictKind::merge; });
        // Click where only the crossing is: the middle of its Link side (the turn's side is the same square).
        const auto onLink = passive.first.path.linkId.empty() ? passive.second : passive.first;
        const auto crossingPoint = centre(conflictSideOutline(network(), onLink));
        require(!merge.polygons.empty(), "The merge has no measured mouth geometry");
        const auto mergePoint = centre(merge.polygons.front());

        // Select draws no automatic area and still picks a road there.
        require(drawn(w, "passive") == 0 && drawn(w, "merge") == 0, "Automatic areas drawn outside the Conflict area tool");
        click(w, crossingPoint);
        require(!c->selected().empty(), "Select did not pick a road at the crossing");

        // The Conflict area tool shows what the drawing implies, with no Add step.
        key(w, Qt::Key_A);
        require(drawn(w, "passive") == 2 && drawn(w, "merge") == 4, "The passive crossing and the two merges are not drawn");
        int automatic = 0;
        for (int r = 0; r < table->rowCount(); ++r) automatic += table->item(r, 6)->text().startsWith("Passive") || table->item(r, 6)->text().startsWith("Automatic");
        require(automatic == 3 && table->rowCount() == static_cast<int>(conflictGroups(network(),all).size()),
                "The table does not list crossings, merges and branching separately");
        const auto branch=std::find_if(all.begin(),all.end(),[](const auto& a){return a.kind==ConflictKind::branching;});
        require(branch!=all.end() && !branch->polygons.empty(),"The shared-source branching mouth is missing");
        const auto branchPoint=centre(branch->polygons.front());
        const auto beforeBranch=w.history().revision();
        click(w,branchPoint);
        require(!c->highlightedConflict().empty(),"A branching area cannot be selected");
        require(!act(w,"editorCyclePriority")->isEnabled() && !act(w,"editorEditConflict")->isEnabled(),
                "A branching area offers a separate priority control");
        require(table->item(table->currentRow(),2)->text()=="Branching","The branching kind is not shown");
        author(w,branchPoint);key(w,Qt::Key_P);
        require(network().rightOfWay.empty() && w.history().revision()==beforeBranch,"Branching authored a priority");

        // First click: the passive crossing gets a priority -- the turn gives way to the road.
        const auto revision = w.history().revision();
        click(w, crossingPoint);
        require(network().rightOfWay.empty() && w.history().revision() == revision, "A left click authored the crossing");
        author(w, crossingPoint);
        require(network().rightOfWay.conflictAreas.size() == 1, "The click did not author the crossing");
        const auto authored = network().rightOfWay.conflictAreas.front();
        const auto& yielding = authored.priority == ConflictPriority::firstYields ? authored.first : authored.second;
        require(yielding.path.connectorId == t.crossingTurn, "The turn does not give way to the road");
        require(drawn(w, "passive") == 0, "The authored crossing is still drawn as passive");
        require(c->highlightedConflict() == authored.id, "The new area is not selected");
        // A second click cycles it, as on any authored area.
        author(w, crossingPoint);
        require(network().rightOfWay.conflictAreas.front().priority != authored.priority, "A second click did not cycle");
        act(w, "editorUndo")->trigger(); act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().rightOfWay.empty() && w.history().revision() == revision, "Authoring was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();

        // Delete makes it passive again.
        require(network().rightOfWay.conflictAreas.size() == 1, "Redo did not restore the crossing");
        click(w, crossingPoint); // select the restored area, independently of the row selected during Undo
        require(w.findChild<QAction*>("editorDeleteConflict")->isEnabled(), "Delete is disabled on the authored crossing");
        act(w, "editorDeleteConflict")->trigger(); QApplication::processEvents();
        require(network().rightOfWay.empty() && drawn(w, "passive") == 2, "Delete did not make the crossing passive");

        // A click on a merge takes it over, one step, with the priority it already ran with.
        const auto beforeMerge = w.history().revision();
        author(w, mergePoint);
        require(network().rightOfWay.conflictAreas.size() == 1 && network().rightOfWay.conflictAreas.front().kind == ConflictKind::merge,
                "The click did not take the merge over");
        require(network().rightOfWay.conflictAreas.front().priority == merge.priority, "The take-over changed the priority");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().revision() == beforeMerge, "The take-over was not one Undo step");

        // Keyboard: select the passive row and press P.
        int row = -1;
        for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == passive.key) row = r;
        require(row >= 0, "The passive crossing has no row");
        table->setFocus(); table->selectRow(row); QApplication::processEvents();
        require(!w.findChild<QAction*>("editorDeleteConflict")->isEnabled(), "Delete is enabled on an automatic row");
        QTest::keyClick(table, Qt::Key_P); QApplication::processEvents();
        require(network().rightOfWay.conflictAreas.size() == 1 && network().rightOfWay.conflictAreas.front().kind == ConflictKind::crossing,
                "P on the passive row did not author it");

        // Thai: the automatic statuses are translated.
        auto* language = w.findChild<QComboBox*>("editorLanguage");
        int automaticRow = -1;
        for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 0)->text() == QStringLiteral("—")) automaticRow = r;
        require(automaticRow >= 0, "No automatic row left to translate");
        const auto english = table->item(automaticRow, 6)->text();
        language->setCurrentIndex(1); QApplication::processEvents();
        for (int r = 0; r < table->rowCount(); ++r)
            if (table->item(r, 0)->text() == QStringLiteral("—")) require(table->item(r, 6)->text() != english, "An automatic status is not translated");
        language->setCurrentIndex(0); QApplication::processEvents();

        // Save and reopen: only the authored crossing is in the file; the merges reappear on their own.
        bool asked = false;
        QTimer::singleShot(0, [&] { if (auto* m = QApplication::activeModalWidget()) { asked = true; m->close(); } });
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        require(!asked, "Save asked for a file name");
        {
            QFile f(file); require(f.open(QIODevice::ReadOnly), "Saved file unreadable");
            const auto saved = nlohmann::json::parse(f.readAll().toStdString());
            require(saved["network"]["rightOfWay"]["conflictAreas"].size() == 1, "The file holds more than the authored area");
        }
        w.openFile(file); QApplication::processEvents();
        key(w, Qt::Key_A);
        require(network().rightOfWay.conflictAreas.size() == 1 && drawn(w, "merge") == 4 && drawn(w, "passive") == 0,
                "Reopening did not derive the same automatic areas");
        // A 3 x 3 crossing is one selectable suggestion, with nine independent reservations.
        ProjectDocument multi;
        addLink(multi,{{0,0},{200,0}},3,3.5);addLink(multi,{{100,-60},{100,60}},3,3.5);
        const auto multiFile=temp.filePath("multi.traffic.json");
        {QFile f(multiFile);require(f.open(QIODevice::WriteOnly),"Multi fixture open failed");
            f.write(QByteArray::fromStdString(documentJson(multi).dump()));}
        w.openFile(multiFile);QApplication::processEvents();key(w,Qt::Key_A);
        require(table->rowCount()==1 && drawn(w,"passive")==18,"Nine pairs did not retain their individual sides");
        std::set<QString> pairIds,groupIds;
        for(auto* item:w.canvas()->scene()->items())if(item->data(0).toString()=="auto-conflict") {
            pairIds.insert(item->data(3).toString());groupIds.insert(item->data(1).toString());
        }
        require(pairIds.size()==9 && groupIds.size()==1,"Separate outlines did not share one selectable control group");
        table->setFocus();table->selectRow(0);QApplication::processEvents();
        const auto beforeMulti=w.history().revision();
        QTest::keyClick(table,Qt::Key_P);QApplication::processEvents();
        require(network().rightOfWay.conflictAreas.size()==9 && table->rowCount()==1,"P did not author all nine pairs");
        const auto grouped=network().rightOfWay;
        act(w,"editorUndo")->trigger();QApplication::processEvents();
        require(network().rightOfWay.empty() && w.history().revision()==beforeMulti,"Group authoring was not one Undo");
        act(w,"editorRedo")->trigger();QApplication::processEvents();
        require(network().rightOfWay==grouped,"Redo lost a group member");
        act(w,"editorDeleteConflict")->trigger();QApplication::processEvents();
        require(network().rightOfWay.empty() && drawn(w,"passive")==18,"Delete missed a group member");
        act(w,"editorUndo")->trigger();QApplication::processEvents();
        require(network().rightOfWay==grouped,"Group delete was not one Undo");
        // Leave no unsaved document for window teardown.
        act(w,"editorUndo")->trigger();QApplication::processEvents();
        // Oblique crossing: separate rail-inset bands follow each driving direction, and
        // picking agrees with the visible bands both before and after authoring.
        for(const double laneWidth:{3.5,0.5}) {
        ProjectDocument oblique;
        addLink(oblique,{{0,0},{200,0}},1,laneWidth);addLink(oblique,{{0,-100},{200,100}},1,laneWidth);
        const auto obliqueFile=temp.filePath("oblique-"+QString::number(laneWidth)+".traffic.json");
        {QFile f(obliqueFile);require(f.open(QIODevice::WriteOnly),"Oblique fixture open failed");
            f.write(QByteArray::fromStdString(documentJson(oblique).dump()));}
        w.openFile(obliqueFile);QApplication::processEvents();key(w,Qt::Key_A);
        require(table->rowCount()==1 && drawn(w,"passive")==2,"Opening a new document retained cached conflicts");
        const auto crossing=automaticConflicts(network()).front();
        const auto path=[](const std::vector<Point>& points) {
            QPainterPath p;for(std::size_t i=0;i<points.size();++i)
                if(i)p.lineTo(points[i].x,points[i].y);else p.moveTo(points[i].x,points[i].y);
            p.closeSubpath();return p;
        };
        const auto firstShape=path(conflictSideOutline(network(),crossing.first));
        const auto secondShape=path(conflictSideOutline(network(),crossing.second));
        const auto inset=[](std::vector<Point> p) {
            require(p.size()==4,"The straight probe is not a quad");
            for(int i=0;i<2;++i) {
                const auto a=p[i],b=p[3-i];const double width=std::hypot(b.x-a.x,b.y-a.y);
                const double t=std::min(0.5/width,0.2);
                p[i]={a.x+t*(b.x-a.x),a.y+t*(b.y-a.y)};
                p[3-i]={b.x+t*(a.x-b.x),b.y+t*(a.y-b.y)};
            }
            return p;
        };
        const auto firstBand=path(inset(conflictSideOutline(network(),crossing.first)));
        const auto secondBand=path(inset(conflictSideOutline(network(),crossing.second)));
        const auto expected=firstBand.united(secondBand);
        const auto pathArea=[](const QPainterPath& p) {
            double total=0;
            for(const auto& polygon:p.toSubpathPolygons()) {
                double sum=0;
                for(int i=0;i<polygon.size();++i) {
                    const auto a=polygon[i],b=polygon[(i+1)%polygon.size()];
                    sum+=a.x()*b.y()-a.y()*b.x();
                }
                total+=std::abs(sum)/2;
            }
            return total;
        };
        int firstCount=0,secondCount=0;
        const auto difference=[&](const QPainterPath& a,const QPainterPath& b) {
            return pathArea(a.subtracted(b))+pathArea(b.subtracted(a));
        };
        for(auto* item:c->scene()->items())if(item->data(0).toString()=="auto-conflict") {
            auto* shape=dynamic_cast<QGraphicsPathItem*>(item);require(shape,"Conflict is not a path");
            // Qt and model clipping can differ at the last floating-point bits of an edge.
            firstCount+=difference(shape->path(),firstBand)<1e-7;
            secondCount+=difference(shape->path(),secondBand)<1e-7;
        }
        require(firstCount==1 && secondCount==1,"Conflict bands do not separately follow both driving directions");
        QPointF outside;bool foundOutside=false;const auto bounds=firstShape.boundingRect();
        for(int x=1;x<20 && !foundOutside;++x)for(int y=1;y<20 && !foundOutside;++y) {
            const QPointF point{bounds.left()+bounds.width()*x/20,bounds.top()+bounds.height()*y/20};
            if(firstShape.contains(point) && !expected.contains(point)){outside=point;foundOutside=true;}
        }
        require(foundOutside,"No point in the rail offset was found");
        require(c->automaticAt({outside.x(),outside.y()}).empty(),"Picking includes the blank rail offset");
        c->fitNetwork();c->scale(4,4);QApplication::processEvents();
        c->centerOn(expected.boundingRect().center());QApplication::processEvents();
        author(w,centre(crossing.polygons.front()));
        require(network().rightOfWay.conflictAreas.size()==1,"The exact intersection cannot be authored");
        require(c->conflictsAt({outside.x(),outside.y()}).empty(),"Authored picking includes the blank rail offset");
        act(w,"editorUndo")->trigger();QApplication::processEvents();
        }
        // One real Link/Connector site contains branching, crossing and merge lane pairs.
        // Its Link continuation must be painted/pickable through the actual P3-P4 station span.
        for(const auto handed:{DrivingSide::left,DrivingSide::right}) {
            ProjectDocument mouth;mouth.network.drivingSide=handed;
            const auto road=addLink(mouth,{{0,0},{200,0}},3,3.5);
            const auto& l=mouth.network.links.front();
            const auto connector=addConnector(mouth,{road,l.lanes.front().id,100},{road,l.lanes.back().id,110});
            const auto original=mouth.network.connectors.front();
            changeConnectorGeometry(mouth,connector,{laneAttachment(mouth.network,original.from,true),laneAttachment(mouth.network,original.to,false)});
            const auto file=temp.filePath("mouth-"+QString::number(static_cast<int>(handed))+".traffic.json");
            {QFile f(file);require(f.open(QIODevice::WriteOnly),"Mouth fixture open failed");f.write(QByteArray::fromStdString(documentJson(mouth).dump()));}
            w.openFile(file);QApplication::processEvents();key(w,Qt::Key_A);
            const auto automatic=automaticConflicts(network());const auto groups=conflictGroups(network(),automatic);
            require(groups.size()==1 && groups.front().kinds.size()==3,"The three real kinds were not grouped");
            require(table->rowCount()==1 && table->item(0,2)->text().count(" / ")==2,"The mixed row does not list all three kinds");
            const auto& connectorRef=network().connectors.front();
            const ControlPathRef pathRef{"","",connectorRef.id,connectorRef.from.laneId,connectorRef.to.laneId};
            const auto caps=conflictMouthBands(network(),pathRef,false,true);
            require(!caps.empty(),"No target mouth continuation");
            const auto surface=connectorSurface(network(),connectorRef);require(surface.target.has_value(),"No target P3-P4");
            const auto& link=network().links.front();
            const double at=(stationOfClosestPoint(link.geometry,surface.target->points[2])+stationOfClosestPoint(link.geometry,surface.target->points[3]))/2;
            const auto lane=laneGeometry(link,connectorRef.to.laneId,handed);
            const auto probe=pointAlong(lane,matchedStation(link.geometry,lane,at));
            bool painted=false;
            for(auto* item:c->scene()->items())if(item->data(0).toString()=="auto-conflict" &&
                item->data(4).toString()=="merge" && item->data(5).toString()==QString::fromStdString(connectorRef.id)) {
                const auto* band=dynamic_cast<QGraphicsPathItem*>(item);
                painted|=band && band->path().contains(QPointF(probe.x,probe.y));
            }
            require(painted && c->automaticAt(probe)==groups.front().key,"P3-P4 Link continuation display/picking disagree");
            table->selectRow(0);QApplication::processEvents();
            require(act(w,"editorCyclePriority")->isEnabled(),"Mixed site priority action is disabled");
            act(w,"editorCyclePriority")->trigger();QApplication::processEvents();
            const auto branches=std::count_if(automatic.begin(),automatic.end(),[](const auto& a){return a.kind==ConflictKind::branching;});
            require(network().rightOfWay.conflictAreas.size()==automatic.size()-static_cast<std::size_t>(branches),"Mixed site did not author all editable members");
            for(const auto& a:network().rightOfWay.conflictAreas)require(a.kind!=ConflictKind::branching,"A branching control was authored");
            require(!c->conflictsAt(probe).empty(),"The authored P3-P4 continuation is not pickable");
            act(w,"editorUndo")->trigger();QApplication::processEvents();
            require(w.history().document()==mouth,"Mixed site authoring is not one Undo");
        }
        std::cout << "conflict auto ui tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
