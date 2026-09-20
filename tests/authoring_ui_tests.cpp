#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGraphicsPathItem>
#include <QLabel>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
template<class T> T* item(EditorWindow& w,const char* name) {
    auto* result=w.findChild<T*>(name);require(result,"Missing authoring control");return result;
}
void action(EditorWindow& w,const char* name) {
    auto* a=item<QAction>(w,name);require(a->isEnabled(),"Action unexpectedly disabled");
    a->trigger();QApplication::processEvents();
}
void write(const QString& path,const ProjectDocument& d) {
    QFile f(path);require(f.open(QIODevice::WriteOnly),"Fixture open failed");
    const auto bytes=QByteArray::fromStdString(documentJson(d).dump());
    require(f.write(bytes)==bytes.size(),"Fixture write failed");
}
int roadStrokes(EditorWindow& w,const std::string& id) {
    int count=0;
    for(auto* p:w.canvas()->scene()->items())
        if(p->data(0).toString()=="road-marking" && p->data(1).toString()==QString::fromStdString(id))++count;
    return count;
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>1,"Expected data directory");
        QTemporaryDir temp;require(temp.isValid(),"Temporary directory unavailable");
        ProjectDocument d;const auto id=addLink(d,{{0,0},{20,0},{60,0}},2,3.5);
        const auto file=temp.filePath("authoring.traffic.json");write(file,d);
        EditorWindow w{std::filesystem::path(argv[1])};w.show();QTest::qWait(30);
        w.openFile(file);w.canvas()->select(id);QApplication::processEvents();
        auto* markings=item<QLineEdit>(w,"editorLinkMarkings");
        require(markings->isEnabled(),"Markings field not enabled for Link");
        const auto before=w.history().document();
        markings->setText("none, double, dashed");action(w,"editorApplyLinkMarkings");
        require(w.history().document().network.links[0].boundaryMarkings==
            std::vector<MarkingType>({MarkingType::none,MarkingType::doubleLine,MarkingType::dashed}),
            "Inspector markings did not reach model");
        require(roadStrokes(w,id)==3,"Double/none markings did not reach canvas");
        int solid=0,dashed=0;
        for(auto* p:w.canvas()->scene()->items())if(p->data(0).toString()=="road-marking") {
            auto* path=dynamic_cast<QGraphicsPathItem*>(p);require(path,"Marking is not a path");
            if(path->pen().style()==Qt::SolidLine)++solid;
            if(path->pen().style()==Qt::DashLine)++dashed;
        }
        require(solid==2 && dashed==1,"Authored outer dashed edge was forced solid");
        const auto authored=w.history().document();
        markings->setText("solid,,solid");action(w,"editorApplyLinkMarkings");
        require(w.history().document()==authored,"Invalid marking input committed");
        action(w,"editorUndo");require(w.history().document()==before,"Markings undo failed");
        action(w,"editorRedo");require(w.history().document()==authored,"Markings redo failed");
        w.saveFile(file);w.openFile(file);w.canvas()->select(id);
        require(markings->text()=="none, double, dashed","Reopen lost marking names");

        const auto saved=w.history().document();
        item<QDoubleSpinBox>(w,"editorLinkPointStation")->setValue(30);
        action(w,"editorInsertLinkPoint");
        require(w.history().document().network.links[0].geometry.size()==4,"Insert station did not add point");
        action(w,"editorUndo");require(w.history().document()==saved,"Insert undo failed");
        action(w,"editorAddLinkPoint");
        require(w.history().document().network.links[0].geometry[2]==Point{40,0},"Longest midpoint wrong");
        action(w,"editorStraightLink");
        require(w.history().document().network.links[0].geometry.size()==2,"Straighten action failed");
        const auto straight=w.history().document();action(w,"editorReverseLink");
        require(w.history().document().network.links[0].geometry.front()==Point{60,0},"Reverse action failed");
        action(w,"editorUndo");require(w.history().document()==straight,"Reverse undo failed");

        // Language changes must translate all new actions, and keep serialized enum names intact.
        auto* language=item<QComboBox>(w,"editorLanguage");
        language->setCurrentIndex(language->findData("th"));QApplication::processEvents();
        require(item<QAction>(w,"editorStraightLink")->text()==QString::fromUtf8("ปรับ Link ให้ตรง"),
                "New action was not translated");
        require(markings->text()=="none, double, dashed","Locale changed persisted parameter names");
        w.canvas()->select("");
        require(!item<QAction>(w,"editorReverseLink")->isEnabled(),"Reverse enabled without selection");
        require(!markings->isEnabled(),"Markings enabled without selection");
        // Opening unsupported target-spec fields must preserve the entire current document.
        auto bad=documentJson(d);bad["network"]["links"][0]["behavior"]={{"speedFactor",1.2}};
        QFile invalid(temp.filePath("unsupported.traffic.json"));require(invalid.open(QIODevice::WriteOnly),"Open invalid fixture");
        invalid.write(QByteArray::fromStdString(bad.dump()));invalid.close();
        const auto retained=w.history().document();bool rejected=false;
        try{w.openFile(invalid.fileName());}catch(const ValidationError& e){
            rejected=!e.issues.empty() && e.issues[0].code=="EDIT_UNSUPPORTED_FIELD";
        }
        require(rejected,"Unsupported field was silently accepted");
        require(w.history().document()==retained,"Rejected file replaced current document");
        w.saveFile(file);
        std::cout<<"PASS authoring inspector, canvas, persistence, undo, language and rejected open\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
