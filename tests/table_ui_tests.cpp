#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QAction>
#include <QAbstractButton>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* msg) { if (!ok) throw std::runtime_error(msg); }
template<class T> T* item(EditorWindow& w,const char* name) {
    auto* p=w.findChild<T*>(name);require(p,"Missing widget");return p;
}
void action(EditorWindow& w,const char* name) { item<QAction>(w,name)->trigger();QApplication::processEvents(); }
QPoint pixel(EditorCanvas* c,Point p) {
    const auto result=c->mapFromScene(p.x,p.y);
    if (!c->viewport()->rect().contains(result)) {
        const auto r=c->viewport()->rect();
        throw std::runtime_error("Gesture outside viewport: world "+std::to_string(p.x)+","+std::to_string(p.y)+
            " -> pixel "+std::to_string(result.x())+","+std::to_string(result.y())+
            " viewport "+std::to_string(r.width())+"x"+std::to_string(r.height()));
    }
    return result;
}
void click(EditorCanvas* c,Point p,Qt::KeyboardModifiers keys={}) {
    QTest::mouseClick(c->viewport(),Qt::LeftButton,keys,pixel(c,p));
}
void band(EditorCanvas* c,Point from,Point to,Qt::KeyboardModifiers keys={}) {
    const auto a=pixel(c,from),b=pixel(c,to);
    QTest::mousePress(c->viewport(),Qt::LeftButton,keys,a);
    QTest::mouseMove(c->viewport(),b);
    QTest::mouseRelease(c->viewport(),Qt::LeftButton,keys,b);
    QApplication::processEvents();
}
void answer(QMessageBox::StandardButton choice) {
    QTimer::singleShot(10,[choice]{for (auto* top : QApplication::topLevelWidgets())
        if (auto* box=qobject_cast<QMessageBox*>(top)) box->button(choice)->click();});
}
int rowFor(QTableWidget* view,const QString& id) {
    for (int row=0;row<view->rowCount();++row) if (view->item(row,0) && view->item(row,0)->text()==id) return row;
    return -1;
}
bool lists(QTableWidget* view,const QString& code) {
    for (int row=0;row<view->rowCount();++row) if (view->item(row,1) && view->item(row,1)->text()==code) return true;
    return false;
}
Point midpoint(const std::vector<Point>& geometry) {
    return pointAlong(geometry,polylineLength(geometry)/2);
}
// Band corners are derived from what is actually on screen, so the gestures stay valid
// whatever the dock layout does to the viewport. Fractions, never pixels.
Point inset(EditorCanvas* c,double fx,double fy) {
    const auto box=c->mapToScene(c->viewport()->rect()).boundingRect();
    return {box.left()+fx*box.width(),box.top()+fy*box.height()};
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>=2,"Expected data directory");
        EditorWindow w{std::filesystem::path(argv[1])};w.resize(1280,900);w.show();QTest::qWait(30);
        auto* c=w.canvas();auto* tool=item<QComboBox>(w,"editorTool");
        auto* links=item<QTableWidget>(w,"editorLinkTable");
        auto* connectors=item<QTableWidget>(w,"editorConnectorTable");
        auto* heads=item<QTableWidget>(w,"editorSignalTable");
        auto* problems=item<QTableWidget>(w,"editorProblemTable");
        auto* tabs=item<QTabWidget>(w,"editorObjectTabs");
        require(links->rowCount()==0,"Empty document should list no links");

        // --- tables follow the document -------------------------------------------------
        const auto draw=[&](Point a,Point b){tool->setCurrentIndex(1);click(c,a);click(c,b);QTest::keyClick(c,Qt::Key_Return);};
        draw({-70,-40},{-20,-40});draw({-70,20},{-20,20});draw({20,-40},{70,-40});
        action(w,"editorFit");tool->setCurrentIndex(0);
        const auto drawn=w.history().document().network.links;
        require(drawn.size()==3,"Road drawing failed");
        require(links->rowCount()==3,"Link table did not follow the document");
        for (const auto& link : drawn) require(rowFor(links,QString::fromStdString(link.id))>=0,"Link missing from table");
        require(links->item(0,1)->text()=="2","Lane count column wrong");
        require(links->item(0,2)->text()=="50.00","Length column wrong");
        action(w,"editorUndo");require(links->rowCount()==2,"Table did not follow Undo");
        action(w,"editorRedo");require(links->rowCount()==3,"Table did not follow Redo");

        // --- a table row selects and frames its object ----------------------------------
        const int row=rowFor(links,QString::fromStdString(drawn[2].id));
        links->selectRow(row);QApplication::processEvents();
        require(c->selected()==drawn[2].id,"Table row did not select the object");
        require(c->selection().size()==1,"Table row selected more than its object");
        require(pixel(c,midpoint(drawn[2].geometry)).x()>0,"Framed object is outside the viewport");
        action(w,"editorFit"); // Framing moved the view onto one road; bring them all back.

        // --- Ctrl-click and rubber band select several ---------------------------------
        click(c,midpoint(drawn[0].geometry));
        require(c->selection().size()==1 && c->selected()==drawn[0].id,"Plain click did not replace selection");
        click(c,midpoint(drawn[1].geometry),Qt::ControlModifier);
        require(c->selection().size()==2,"Ctrl-click did not add to the selection");
        require(c->selected()==drawn[1].id,"Last object clicked is not primary");
        require(links->selectedItems().size()==8,"Table did not mirror the canvas selection");
        require(item<QLineEdit>(w,"editorId")->text()==QString::fromStdString(drawn[1].id),"Inspector lost the primary");
        click(c,midpoint(drawn[1].geometry),Qt::ControlModifier);
        require(c->selection().size()==1,"Ctrl-click did not toggle off");
        // The two western roads lie left of x = 0; the eastern one does not.
        const auto west=[&]{return std::pair{Point{inset(c,0.02,0.05).x,inset(c,0,0.05).y},Point{0,inset(c,0,0.95).y}};};
        band(c,west().first,west().second);
        require(c->selection().size()==2,"Rubber band did not select both western roads");
        require(c->isSelected(drawn[0].id) && c->isSelected(drawn[1].id),"Rubber band selected the wrong roads");
        band(c,inset(c,0.80,0.02),inset(c,0.98,0.10));
        require(c->selection().empty(),"Empty band did not clear the selection");

        // --- delete many, restored by one Undo ------------------------------------------
        tool->setCurrentIndex(5);
        const auto source=laneGeometry(drawn[0],drawn[0].lanes[0].id,DrivingSide::left).back();
        const auto target=laneGeometry(drawn[2],drawn[2].lanes[0].id,DrivingSide::left).front();
        click(c,source);click(c,target);
        require(w.history().document().network.connectors.size()==1,"Connector not created");
        require(connectors->rowCount()==1,"Connector table did not follow the document");
        tool->setCurrentIndex(0);
        const auto before=documentJson(w.history().document());const auto revision=w.history().revision();
        // The connector now crosses the band too, so the selection also names an object that a
        // link's own cascade will remove first: deleteObjects must skip it rather than fail.
        band(c,west().first,west().second);
        require(c->isSelected(drawn[0].id) && c->isSelected(drawn[1].id),"Band lost the roads");
        require(c->isSelected(w.history().document().network.connectors[0].id),"Band missed the connector");
        answer(QMessageBox::No);action(w,"editorDeleteSelected");
        require(documentJson(w.history().document())==before,"Cancelled delete changed the document");
        answer(QMessageBox::Yes);action(w,"editorDeleteSelected");
        require(w.history().document().network.links.size()==1,"Delete-selected did not remove both roads");
        require(w.history().document().network.connectors.empty(),"Attached connector was not cascaded");
        require(w.history().revision()!=revision,"Delete-selected did not commit");
        action(w,"editorUndo");
        require(documentJson(w.history().document())==before,"One Undo did not restore roads and connector");
        require(links->rowCount()==3 && connectors->rowCount()==1,"Tables did not follow the Undo");

        // --- a rejected edit fills Problems and can be jumped to ------------------------
        c->select(drawn[0].id);
        item<QLineEdit>(w,"editorLaneWidths")->setText("-1, 3");action(w,"editorApplyLanes");
        require(documentJson(w.history().document())==before,"Rejected edit changed the document");
        require(!item<QLabel>(w,"editorError")->text().isEmpty(),"Rejected edit gave no message");
        require(tabs->currentIndex()==3,"Rejected edit did not open Problems");
        int problem=-1;
        for (int i=0;i<problems->rowCount();++i)
            if (problems->item(i,2) && problems->item(i,2)->text()==QString::fromStdString(drawn[0].lanes[0].id)) problem=i;
        require(problem>=0,"Problem row does not name the offending lane");
        require(!problems->item(problem,0)->text().isEmpty(),"Severity column empty");
        require(!problems->item(problem,1)->text().isEmpty(),"Problem column empty");
        c->select("");problems->selectRow(problem);QApplication::processEvents();
        require(c->selected()==drawn[0].id,"Jump-to-object did not select the link");
        // A committed edit clears the rejection; the list is about the document, not the past.
        item<QLineEdit>(w,"editorLaneWidths")->setText("3, 3");action(w,"editorApplyLanes");
        require(w.history().revision()!=revision,"Valid lane edit did not commit");
        action(w,"editorRecheck");
        require(!lists(problems,QString()),"Recheck produced blank rows");

        // --- an authored merge is reported but never blocks authoring -------------------
        action(w,"editorFit"); // The jump above framed one link; bring the whole drawing back.
        tool->setCurrentIndex(5);
        const auto merged=w.history().document().network.links;
        const auto second=laneGeometry(merged[1],merged[1].lanes[0].id,DrivingSide::left).back();
        click(c,second);click(c,laneGeometry(merged[2],merged[2].lanes[0].id,DrivingSide::left).front());
        require(w.history().document().network.connectors.size()==2,"Merging connector was rejected");
        require(item<QLabel>(w,"editorError")->text().isEmpty(),"Authoring a merge reported an error");
        tool->setCurrentIndex(0);
        action(w,"editorRecheck");
        require(tabs->currentIndex()==3,"Recheck did not open Problems");
        require(item<QLabel>(w,"editorError")->text().isEmpty(),"Runnability check reported an edit error");
        int unsupported=-1;
        for (int i=0;i<problems->rowCount();++i)
            if (problems->item(i,3) && problems->item(i,3)->text().startsWith("segments.")) unsupported=i;
        require(unsupported>=0,"UNSUPPORTED_MERGE was not listed");
        require(!problems->item(unsupported,2)->text().isEmpty(),"Merge row names no object");
        problems->selectRow(unsupported);QApplication::processEvents();
        require(!c->selected().empty(),"Jump from a runtime row selected nothing");
        // Authoring still works after the check: a runnability finding is not a rejection.
        c->select(merged[0].id);
        item<QLineEdit>(w,"editorLaneWidths")->setText("3.25, 3.25");action(w,"editorApplyLanes");
        require(w.history().document().network.links[0].lanes[0].width==3.25,"Edit blocked after a runnability finding");

        // --- signal heads, and Thai -----------------------------------------------------
        w.openFile(QString::fromUtf8(argv[1])+"/scenarios/crossing.json");
        require(heads->rowCount()==2,"Signal head table did not load");
        const int head=rowFor(heads,"west-head");require(head>=0,"Signal head missing from table");
        heads->selectRow(head);QApplication::processEvents();
        require(c->selected()=="west","Signal head row did not select its link");
        item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);
        for (int i=0;i<4;++i) require(!tabs->tabText(i).isEmpty(),"Untranslated object tab");
        require(tabs->tabText(3)==QString::fromUtf8("ปัญหา"),"Thai Problems tab missing");
        for (int column=0;column<4;++column)
            require(!links->horizontalHeaderItem(column)->text().isEmpty(),"Untranslated column header");
        require(links->horizontalHeaderItem(1)->text()==QString::fromUtf8("จำนวนเลน"),"Thai column header missing");
        action(w,"editorRecheck");
        require(lists(problems,QString::fromUtf8("ยังไม่ได้ตรวจการอ้างอิงประเภทยานพาหนะและพฤติกรรมผู้ขับ เพราะแคตตาล็อกเหล่านั้นอยู่ใน data/ ไม่ได้อยู่ในไฟล์โครงการ")),
            "Thai runnability finding missing");
        if (argc>2) require(w.grab().save(QString::fromUtf8(argv[2])),"Screenshot failed");
        std::cout<<"Object tables, multi-selection, delete-many, diagnostics and jump-to-object passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
