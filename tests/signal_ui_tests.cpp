#include "../src/shell/editor_window.hpp"
#include "../src/project/run.hpp"
#include <nlohmann/json.hpp>
#include <QApplication>
#include <QAction>
#include <QGraphicsItem>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <cmath>
#include <iostream>
using namespace trafficsim;
// The Signal head is its stop line (M2.7): a click with the head tool places one exactly where
// the pointer is, on a Link lane or a Connector path, and a drag slides it along its lane.
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void write(const QString& path,const ProjectDocument& d) {
    QFile f(path);require(f.open(QIODevice::WriteOnly),"Fixture open failed");
    const auto bytes=QByteArray::fromStdString(documentJson(d).dump());
    require(f.write(bytes)==bytes.size(),"Fixture write failed");
}
QPoint at(EditorWindow& w,const std::vector<Point>& g,double station) {
    const auto p=pointAlong(g,station);return w.canvas()->mapFromScene(p.x,p.y);
}
std::vector<Point> lane(EditorWindow& w,std::size_t link,std::size_t index) {
    const auto& n=w.history().document().network;
    return laneGeometry(n.links[link],n.links[link].lanes[index].id,n.drivingSide);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>1,"Expected data directory");
        QTemporaryDir temp;require(temp.isValid(),"Temporary directory unavailable");
        ProjectDocument d;
        const auto west=addLink(d,{{-100,0},{-10,0}},2,3.5);
        const auto east=addLink(d,{{10,0},{100,0}},2,3.5);
        addConnectorRange(d,{west,d.network.links[0].lanes.front().id},{east,d.network.links[1].lanes.front().id},2,2);
        putProgram(d,{"",0,{{30,SignalColor::green},{3,SignalColor::amber},{30,SignalColor::red}}});
        putInput(d,{"",putRoute(d,{"",{west}}),"car",600,0,60}); // something to run
        const auto file=temp.filePath("signal.traffic.json");write(file,d);
        EditorWindow w{std::filesystem::path(argv[1])};w.show();QTest::qWait(30);
        w.openFile(file);QApplication::processEvents();
        w.canvas()->fitNetwork();QApplication::processEvents();
        auto* tool=w.findChild<QComboBox*>("editorTool");require(tool,"Missing tool combo");
        tool->setCurrentIndex(8);QApplication::processEvents();

        // A click 60 m along the second lane of the west Link: the dialog opens on THAT lane at
        // THAT station, where before it opened on the first lane at station 0.
        const auto second=lane(w,0,1);
        const auto secondId=w.history().document().network.links[0].lanes[1].id;
        bool opened=false;
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog && dialog->objectName()=="editorHeadDialog","Head dialog did not open");
            auto* laneBox=dialog->findChild<QComboBox*>("editorHeadLane");
            auto* station=dialog->findChild<QDoubleSpinBox*>("editorHeadPosition");
            require(laneBox && station,"Head dialog lost its fields");
            require(laneBox->currentData().toString().toStdString()==secondId,"Dialog did not open on the clicked lane");
            require(std::abs(station->value()-60)<1.,"Dialog did not open at the clicked station");
            require(std::abs(station->maximum()-polylineLength(second))<1e-6,"Station was not bounded by the lane length");
            opened=true;dialog->accept();
        });
        QTest::mouseClick(w.canvas()->viewport(),Qt::LeftButton,{},at(w,second,60));
        QApplication::processEvents();
        require(opened,"Clicking a lane with the head tool opened nothing");
        require(w.history().document().network.signalHeads.size()==1,"The click placed no head");
        const auto head=w.history().document().network.signalHeads.front();
        require(head.lane.laneId==secondId && std::abs(head.position-60)<1.,"The head is not where it was clicked");
        bool drawnLine=false;
        for(auto* i:w.canvas()->scene()->items())drawnLine=drawnLine || i->data(0).toString()=="stop-line";
        require(drawnLine,"The head drew no stop line");

        // A Connector path takes a head too: nearestLane ignored Connectors outright.
        const auto& n=w.history().document().network;
        const auto path=connectorPaths(n,n.connectors.front()).front();
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog,"Head dialog did not open on a Connector");
            require(dialog->findChild<QComboBox*>("editorHeadLane")->currentData().toString().toStdString()==path.id,
                "Dialog did not open on the clicked Connector path");
            dialog->accept();
        });
        QTest::mouseClick(w.canvas()->viewport(),Qt::LeftButton,{},at(w,path.geometry,polylineLength(path.geometry)/2));
        QApplication::processEvents();
        require(w.history().document().network.signalHeads.size()==2,"No head was placed on the Connector");
        require(w.history().document().network.signalHeads.back().connectorId==path.id,"Connector head stood on the wrong path");

        // A miss places nothing and opens nothing.
        QTest::mouseClick(w.canvas()->viewport(),Qt::LeftButton,{},w.canvas()->mapFromScene(0,60));
        QApplication::processEvents();
        require(!QApplication::activeModalWidget(),"A miss opened a dialog");
        require(w.history().document().network.signalHeads.size()==2,"A miss placed a head");

        // Drag: select the first head, slide its stop line to 30 m, one Undo step.
        tool->setCurrentIndex(0);QApplication::processEvents();
        w.canvas()->select(head.id);QApplication::processEvents();
        const auto from=at(w,second,60),to=at(w,second,30);
        QTest::mousePress(w.canvas()->viewport(),Qt::LeftButton,{},from);
        for(int k=1;k<=5;++k)QTest::mouseMove(w.canvas()->viewport(),from+(to-from)*k/5);
        QTest::mouseRelease(w.canvas()->viewport(),Qt::LeftButton,{},to);
        QApplication::processEvents();
        const auto moved=w.history().document().network.signalHeads.front();
        require(moved.lane.laneId==secondId,"Dragging moved the head to another lane");
        require(std::abs(moved.position-30)<1.,"Dragging did not slide the stop line");
        w.findChild<QAction*>("editorUndo")->trigger();QApplication::processEvents();
        require(std::abs(w.history().document().network.signalHeads.front().position-60)<1.,"Undo did not restore the head");

        // M2.7b: a controller from the 4-phase template, in one dialog. The table is the input,
        // the bars are drawn from it, and a time that does not fit the cycle disables OK.
        auto* add=w.findChild<QAction*>("editorAddController");require(add && add->isEnabled(),"No Add signal controller action");
        bool checked=false;
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog && dialog->objectName()=="editorControllerDialog","Controller dialog did not open");
            auto* templates=dialog->findChild<QComboBox*>("editorControllerTemplate");
            auto* groups=dialog->findChild<QTableWidget*>("editorSignalGroups");
            auto* ok=dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok);
            auto* status=dialog->findChild<QLabel*>("editorControllerStatus");
            require(templates && groups && ok && status && dialog->findChild<QWidget*>("editorSignalTiming"),"Controller dialog lost a control");
            templates->setCurrentIndex(1);QApplication::processEvents();
            require(groups->rowCount()==4,"The 4-phase template did not make four groups");
            require(dialog->findChild<QDoubleSpinBox*>("editorControllerCycle")->value()==120,"The 4-phase template's cycle is not 120 s");
            require(ok->isEnabled(),"A template's own timing was refused");
            auto* amber=qobject_cast<QDoubleSpinBox*>(groups->cellWidget(3,4));
            amber->setValue(200);QApplication::processEvents();
            require(!ok->isEnabled() && !status->text().isEmpty(),"A group overflowing the cycle was not flagged before OK");
            amber->setValue(3);QApplication::processEvents();
            require(ok->isEnabled(),"Fixing the time did not re-enable OK");
            checked=true;dialog->accept();
        });
        add->trigger();QApplication::processEvents();
        require(checked,"The controller dialog never ran");
        const auto& controllers=w.history().document().definition->signalControllers;
        require(controllers.size()==1 && controllers[0].groups.size()==4 && controllers[0].cycle==120,"The controller was not committed");
        const auto controllerId=controllers[0].id;

        // Placing heads for it: choose group 2 once, and the next click defaults to group 2.
        tool->setCurrentIndex(8);QApplication::processEvents();
        const auto first=lane(w,1,0),other=lane(w,1,1);
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            auto* signal=dialog->findChild<QComboBox*>("editorHeadSignal");
            const int at=signal->findData(QString::fromStdString("c:"+controllerId+"#2"));
            require(at>=0,"Group 2 is not offered for a head");
            signal->setCurrentIndex(at);dialog->accept();
        });
        QTest::mouseClick(w.canvas()->viewport(),Qt::LeftButton,{},at(w,first,20));QApplication::processEvents();
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog->findChild<QComboBox*>("editorHeadSignal")->currentData().toString().toStdString()=="c:"+controllerId+"#2",
                "The next head did not default to the group just used");
            dialog->accept();
        });
        QTest::mouseClick(w.canvas()->viewport(),Qt::LeftButton,{},at(w,other,20));QApplication::processEvents();
        const auto& heads=w.history().document().network.signalHeads;
        require(heads.size()==4,"Two more heads were not placed");
        for(std::size_t k=2;k<4;++k)
            require(heads[k].controllerId==controllerId && heads[k].groupNumber==2 && heads[k].programId.empty(),"A head does not show group 2");
        // And the run compiles every head against its group's program, with nothing to fix first.
        const auto snapshot=compileDocument(w.history().document(),argv[1]);
        require(snapshot.scenario.signalHeads[2].programId==controllerId+"#2","A head does not run against its group");
        std::cout<<"Signal UI tests passed\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
