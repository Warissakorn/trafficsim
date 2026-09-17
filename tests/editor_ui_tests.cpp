#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QBuffer>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QInputDialog>
#include <QTimer>
#include <QMessageBox>
#include <QAbstractButton>
#include <QWheelEvent>
#include <iostream>
#include <cmath>
using namespace trafficsim;
namespace {
void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
template<class T> T* item(EditorWindow& w,const char* name){auto* p=w.findChild<T*>(name);require(p,"Missing widget");return p;}
void action(EditorWindow& w,const char* name){item<QAction>(w,name)->trigger();QApplication::processEvents();}
void click(EditorCanvas* c,double x,double y){QTest::mouseClick(c->viewport(),Qt::LeftButton,{},c->mapFromScene(x,y));}
void write(const QString& file,const Json& data){QFile f(file);require(f.open(QIODevice::WriteOnly),"fixture write failed");f.write(QByteArray::fromStdString(data.dump()));}
}
int main(int argc,char** argv){
    QApplication app(argc,argv);
    try{
        require(argc>=2,"Expected data directory");QTemporaryDir directory;require(directory.isValid(),"temp directory");
        EditorWindow w{std::filesystem::path(argv[1])};w.show();QTest::qWait(40);
        auto* c=w.canvas();auto* tool=item<QComboBox>(w,"editorTool");
        tool->setCurrentIndex(1);click(c,-60,0);click(c,60,0);QTest::keyClick(c,Qt::Key_Return);
        require(w.history().document().network.links.size()==1,"Draw did not create link");
        const auto original=documentJson(w.history().document());const auto id=c->selected();
        require(w.history().dirty(),"Draw must mark dirty");
        const auto file=directory.path()+QString::fromUtf8("/ทางแยก.traffic.json");w.saveFile(file);require(!w.history().dirty(),"Save must mark clean");
        tool->setCurrentIndex(0);
        const auto start=c->mapFromScene(60,0), end=c->mapFromScene(60,20);
        QTest::mousePress(c->viewport(),Qt::LeftButton,{},start);QTest::mouseMove(c->viewport(),end);QTest::mouseRelease(c->viewport(),Qt::LeftButton,{},end);
        require(w.history().revision()==2,"Drag should be exactly one command");
        require(w.history().document().network.links[0].geometry.back().y==20,"Drag did not move endpoint");
        action(w,"editorUndo");require(!w.history().dirty(),"Undo to save must be clean");require(documentJson(w.history().document())==original,"Undo changed document");
        action(w,"editorRedo");require(w.history().dirty(),"Redo lost dirty state");
        bool failed=false;try{w.saveFile(directory.path());}catch(const std::exception&){failed=true;}
        require(failed&&w.history().dirty(),"Failed save lost dirty state");
        QFile saved(file);require(saved.open(QIODevice::ReadOnly),"saved file missing");
        require(Json::parse(saved.readAll().toStdString())==original,"Failed save changed existing file");saved.close();
        action(w,"editorUndo");
        QTest::mouseDClick(c->viewport(),Qt::LeftButton,{},c->mapFromScene(0,0));
        require(w.history().document().network.links[0].geometry.size()==3,"Insert vertex failed");
        click(c,0,0);QTest::keyClick(c,Qt::Key_Delete,Qt::ControlModifier);
        require(w.history().document().network.links[0].geometry.size()==2,"Remove vertex failed");
        // Whole-link drag, cancellation, pan and zoom exercise input handling beyond commands.
        const auto beforeMove=documentJson(w.history().document());
        QTest::mousePress(c->viewport(),Qt::LeftButton,{},c->mapFromScene(0,0));
        const auto moveTo=c->mapFromScene(10,5);QTest::mouseMove(c->viewport(),moveTo);QTest::mouseRelease(c->viewport(),Qt::LeftButton,{},moveTo);
        require(w.history().document().network.links[0].geometry.front().x==-50,"Whole link drag failed");
        action(w,"editorUndo");require(documentJson(w.history().document())==beforeMove,"Whole link undo failed");
        tool->setCurrentIndex(1);click(c,0,30);QTest::keyClick(c,Qt::Key_Escape);QTest::keyClick(c,Qt::Key_Return);
        require(w.history().document().network.links.size()==1,"Cancelled draft committed");tool->setCurrentIndex(0);
        const auto centre=c->viewport()->rect().center();const auto beforePan=c->mapToScene(centre);
        QTest::mousePress(c->viewport(),Qt::RightButton,{},centre);QTest::mouseMove(c->viewport(),centre+QPoint(30,20));QTest::mouseRelease(c->viewport(),Qt::RightButton,{},centre+QPoint(30,20));
        require(c->mapToScene(centre)!=beforePan,"Pan did not change viewport");
        const double scale=c->transform().m11();QWheelEvent wheel(QPointF(centre),QPointF(c->viewport()->mapToGlobal(centre)),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(c->viewport(),&wheel);require(c->transform().m11()>scale&&c->transform().m22()<0,"Zoom lost world orientation");
        item<QSpinBox>(w,"editorLaneCount")->setValue(3);item<QLineEdit>(w,"editorLaneWidths")->setText("3, 3.5, 4");action(w,"editorApplyLanes");
        require(w.history().document().network.links[0].lanes.size()==3,"Lane count not applied");
        require(w.history().document().network.links[0].lanes[2].width==4,"Per-lane width lost");
        // Vissim's Name: type it, press Return, and the object carries it -- in the model, in
        // the object list, and after a reopen.
        auto* name=item<QLineEdit>(w,"editorName");
        require(name->isEnabled() && name->text().isEmpty(),"Name field should start enabled and empty");
        const auto beforeName=w.history().revision();
        QTest::keyClick(name,Qt::Key_Return);
        require(w.history().revision()==beforeName,"Committing an unchanged name added a history entry");
        name->setText(QString::fromUtf8("ถนนสุขุมวิท"));QTest::keyClick(name,Qt::Key_Return);QApplication::processEvents();
        require(w.history().document().network.links[0].name==QString::fromUtf8("ถนนสุขุมวิท").toStdString(),"Name did not reach the model");
        require(item<QTableWidget>(w,"editorLinkTable")->item(0,1)->text()==QString::fromUtf8("ถนนสุขุมวิท"),"Name column did not follow");
        action(w,"editorUndo");
        require(w.history().document().network.links[0].name.empty(),"Undo did not take the name back");
        require(name->text().isEmpty(),"Name field did not follow Undo");
        action(w,"editorRedo");
        item<QDoubleSpinBox>(w,"editorSplitDistance")->setValue(60);action(w,"editorPocket");
        require(w.history().document().network.links.size()==2,"Pocket split missing");
        require(w.history().document().network.links.back().lanes.size()==4,"Pocket extra lane missing");
        require(w.history().document().network.connectors.size()==3,"Split continuity missing");
        action(w,"editorOpposite");require(w.history().document().network.links.size()==3,"Opposite link missing");
        item<QComboBox>(w,"editorDrivingSide")->setCurrentIndex(1);
        require(w.history().document().network.drivingSide==DrivingSide::right,"Driving side not applied");
        const auto beforeFail=documentJson(w.history().document());const auto revision=w.history().revision();
        item<QLineEdit>(w,"editorLaneWidths")->setText("-1");action(w,"editorApplyLanes");
        require(w.history().revision()==revision && documentJson(w.history().document())==beforeFail,"Invalid edit mutated state");
        require(!item<QLabel>(w,"editorError")->text().isEmpty(),"Invalid edit needs visible feedback");
        const auto selectedForDelete=w.history().document().network.links.front().id;c->select(selectedForDelete);
        QTimer::singleShot(10,[]{for(auto* top:QApplication::topLevelWidgets())if(auto* box=qobject_cast<QMessageBox*>(top))box->button(QMessageBox::Yes)->click();});
        action(w,"editorDeleteLink");require(w.history().document().network.links.size()==2,"Confirmed delete failed");
        action(w,"editorUndo");require(documentJson(w.history().document())==beforeFail,"Delete undo lost related objects");
        QTimer::singleShot(10,[]{for(auto* top:QApplication::topLevelWidgets())if(auto* box=qobject_cast<QMessageBox*>(top))box->button(QMessageBox::Cancel)->click();});
        action(w,"editorNew");require(documentJson(w.history().document())==beforeFail,"Cancel discarded unsaved work");
        w.saveFile(file);w.openFile(file);require(documentJson(w.history().document())==beforeFail,"Open roundtrip failed");
        const auto broken=directory.path()+"/broken.json";auto bad=beforeFail;bad["schemaVersion"]=999;write(broken,bad);
        failed=false;try{w.openFile(broken);}catch(const std::exception&){failed=true;}
        require(failed&&documentJson(w.history().document())==beforeFail,"Failed load discarded the document");
        auto imageDoc=w.history().document();QImage image(300,200,QImage::Format_RGB32);image.fill(QColor("#cbd5be"));
        QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);require(image.save(&buffer,"PNG"),"image fixture");
        imageDoc.background.pngBase64=std::make_shared<const std::string>(png.toBase64().toStdString());
        write(file,documentJson(imageDoc));w.openFile(file);
        item<QDoubleSpinBox>(w,"editorImageScale")->setValue(0.5);item<QDoubleSpinBox>(w,"editorImageRotation")->setValue(15);action(w,"editorApplyImage");
        require(w.history().document().background.metresPerPixel==0.5,"Image scale not applied");
        require(w.history().document().background.rotation==15,"Image angle not applied");
        action(w,"editorUndo");require(w.history().document().background.rotation==0,"Image undo failed");
        tool->setCurrentIndex(4);click(c,0,0);
        QTimer::singleShot(20,[]{for(auto* widget:QApplication::topLevelWidgets())if(auto* dialog=qobject_cast<QInputDialog*>(widget)){dialog->setDoubleValue(20);dialog->accept();}});
        click(c,40,0);require(std::abs(w.history().document().background.metresPerPixel-0.5)<0.02,"Two-point calibration failed");
        tool->setCurrentIndex(3);click(c,0,0);click(c,40,0);
        require(item<QLabel>(w,"editorError")->text().contains("Distance"),"Measurement feedback missing");
        action(w,"editorRemoveImage");require(w.history().document().background.pngBase64->empty(),"Remove image failed");
        action(w,"editorUndo");require(!w.history().document().background.pngBase64->empty(),"Undo must restore image");
        item<QComboBox>(w,"editorLanguage")->setCurrentIndex(1);
        require(item<QAction>(w,"editorSave")->text()==QString::fromUtf8("บันทึก"),"Thai editor translation missing");
        w.resize(1000,760);action(w,"editorFit");QTest::qWait(30);
        require(c->viewport()->width()>300,"Inspector leaves too little canvas space");
        if(argc>2)require(w.grab().save(QString::fromUtf8(argv[2])),"Screenshot failed");
        w.saveFile(file);w.openFile(file);require(!w.history().dirty(),"Final save/load dirty");
        std::cout<<"Editor draw, drag, history, lanes, pocket, opposite, save/load, image calibration and Thai UI passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
