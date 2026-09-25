#include "editor_window.hpp"
#include "signal_timing_view.hpp"
#include "../model/demand/signal_control.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cmath>

namespace trafficsim {
// M2.7b. A signal controller the way a timing sheet reads: a cycle, an offset, and per signal
// group the second its green starts and ends and its amber. The table is the input; the bars
// under it are the check. Nothing here computes a colour of its own -- the diagram and the run
// both follow signalGroupColorAt.
namespace {
enum Column { number, name, start, end, amber, green, columns };
// Green, 3 s amber and 2 s all-red per phase, one after another round the cycle.
SignalController fromTemplate(int phases, double green, const std::function<std::string(int)>& label) {
    SignalController c{{}, {}, phases * (green + 5), 0, {}};
    for (int k = 0; k < phases; ++k)
        c.groups.push_back({k + 1, label(k + 1), k * (green + 5), k * (green + 5) + green, 3});
    return c;
}
}
std::string EditorWindow::editController(const std::string& id) {
    const auto& def=history_.document().definition;
    const auto label=[this](int k){return text("editorGroupDefaultName").arg(k).toStdString();};
    SignalController value=fromTemplate(2,25,label);
    bool existing=false;
    if(def)for(const auto& c:def->signalControllers)if(c.id==id){value=c;existing=true;}
    QDialog dialog(this);dialog.setObjectName("editorControllerDialog");
    dialog.setWindowTitle(text(existing?"editorEditController":"editorAddController"));
    auto* layout=new QVBoxLayout(&dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* templates=new QComboBox(&dialog);templates->setObjectName("editorControllerTemplate");
    templates->addItem(text("editorTemplateTwoPhase"));templates->addItem(text("editorTemplateFourPhase"));
    templates->addItem(text("editorTemplateBlank"));
    if(!existing)form->addRow(text("editorTemplate"),templates);else templates->hide();
    auto* title=new QLineEdit(QString::fromStdString(value.name),&dialog);title->setObjectName("editorControllerName");
    auto* cycle=new QDoubleSpinBox(&dialog);cycle->setObjectName("editorControllerCycle");cycle->setRange(1,3600);cycle->setDecimals(1);
    auto* offset=new QDoubleSpinBox(&dialog);offset->setObjectName("editorControllerOffset");offset->setRange(0,3600);offset->setDecimals(1);
    form->addRow(text("editorControllerName"),title);form->addRow(text("editorControllerCycle"),cycle);
    form->addRow(text("editorControllerOffset"),offset);
    auto* groups=new QTableWidget(0,columns,&dialog);groups->setObjectName("editorSignalGroups");
    groups->setHorizontalHeaderLabels({text("editorGroupNumber"),text("editorGroupName"),text("editorGreenStart"),
        text("editorGreenEnd"),text("editorAmber"),text("editorGreenTime")});
    groups->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    groups->horizontalHeader()->setSectionResizeMode(name,QHeaderView::Stretch);
    groups->verticalHeader()->hide();layout->addWidget(groups);
    auto* buttonsRow=new QHBoxLayout;layout->addLayout(buttonsRow);
    auto* add=new QPushButton(text("editorAddGroup"),&dialog);add->setObjectName("editorAddGroup");
    auto* remove=new QPushButton(text("editorRemoveGroup"),&dialog);remove->setObjectName("editorRemoveGroup");
    buttonsRow->addWidget(add);buttonsRow->addWidget(remove);buttonsRow->addStretch();
    layout->addWidget(new QLabel(text("editorTimingDiagram"),&dialog));
    auto* diagram=new SignalTimingView(&dialog);layout->addWidget(diagram);
    auto* status=new QLabel(&dialog);status->setObjectName("editorControllerStatus");status->setWordWrap(true);layout->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);

    bool loading=false;
    const auto second=[&](double v){
        auto* f=new QDoubleSpinBox(groups);f->setRange(0,3600);f->setDecimals(1);f->setValue(v);return f;
    };
    // What the table says, read back into a controller. The dialog never keeps a second copy.
    const auto read=[&]{
        auto c=value;c.name=title->text().toStdString();c.cycle=cycle->value();c.offset=offset->value();c.groups.clear();
        for(int r=0;r<groups->rowCount();++r)c.groups.push_back({
            qobject_cast<QSpinBox*>(groups->cellWidget(r,number))->value(),
            qobject_cast<QLineEdit*>(groups->cellWidget(r,name))->text().toStdString(),
            qobject_cast<QDoubleSpinBox*>(groups->cellWidget(r,start))->value(),
            qobject_cast<QDoubleSpinBox*>(groups->cellWidget(r,end))->value(),
            qobject_cast<QDoubleSpinBox*>(groups->cellWidget(r,amber))->value()});
        return c;
    };
    std::function<void()> update;
    const auto append=[&](const SignalGroup& g){
        const int r=groups->rowCount();groups->insertRow(r);
        auto* n=new QSpinBox(groups);n->setRange(1,999);n->setValue(g.number);groups->setCellWidget(r,number,n);
        auto* title=new QLineEdit(QString::fromStdString(g.name),groups);groups->setCellWidget(r,name,title);
        groups->setCellWidget(r,start,second(g.greenStart));groups->setCellWidget(r,end,second(g.greenEnd));
        groups->setCellWidget(r,amber,second(g.amber));
        auto* derived=new QTableWidgetItem;derived->setFlags(Qt::ItemIsEnabled);groups->setItem(r,green,derived);
        connect(n,qOverload<int>(&QSpinBox::valueChanged),&dialog,[&]{update();});
        connect(title,&QLineEdit::textChanged,&dialog,[&]{update();});
        for(int col:{start,end,amber})
            connect(qobject_cast<QDoubleSpinBox*>(groups->cellWidget(r,col)),qOverload<double>(&QDoubleSpinBox::valueChanged),&dialog,[&]{update();});
    };
    const auto load=[&](const SignalController& c){
        loading=true;groups->setRowCount(0);cycle->setValue(c.cycle);offset->setValue(c.offset);
        for(const auto& g:c.groups)append(g);
        loading=false;update();
    };
    // Every change redraws the bars and re-checks the timing, so a group that does not fit the
    // cycle is named before OK, not after.
    update=[&]{
        if(loading)return;
        const auto c=read();
        for(int r=0;r<groups->rowCount();++r)
            groups->item(r,green)->setText(QString::number(signalGroupGreen(c,c.groups[static_cast<std::size_t>(r)]),'f',1));
        diagram->setController(c);
        AuthoringDefinition probe;probe.timeStep=def?def->timeStep:0.1;probe.signalControllers={c};
        if(probe.signalControllers[0].id.empty())probe.signalControllers[0].id="new";
        QStringList problems;
        for(const auto& issue:signalControlIssues(Network{},probe))
            problems<<text(issue.code)+" ("+QString::fromStdString(issue.path.substr(issue.path.find('.')+1))+")";
        status->setText(problems.isEmpty()?text("editorTimingOk"):problems.join("\n"));
        status->setStyleSheet(problems.isEmpty()?QString{}:"color: #b91c1c;");
        buttons->button(QDialogButtonBox::Ok)->setEnabled(problems.isEmpty());
    };
    connect(cycle,qOverload<double>(&QDoubleSpinBox::valueChanged),&dialog,[&]{update();});
    connect(offset,qOverload<double>(&QDoubleSpinBox::valueChanged),&dialog,[&]{update();});
    connect(title,&QLineEdit::textChanged,&dialog,[&]{update();});
    connect(templates,qOverload<int>(&QComboBox::currentIndexChanged),&dialog,[&](int index){
        load(index==0?fromTemplate(2,25,label):index==1?fromTemplate(4,25,label):SignalController{{},{},90,0,{}});
    });
    connect(add,&QPushButton::clicked,&dialog,[&]{
        // The next free number, green right after the last group's amber and 2 s all-red.
        const auto c=read();int next=1;double from=0;
        for(const auto& g:c.groups){next=std::max(next,g.number+1);from=std::fmod(g.greenEnd+g.amber+2,c.cycle);}
        append({next,label(next),from,std::fmod(from+10,c.cycle),3});update();
    });
    connect(remove,&QPushButton::clicked,&dialog,[&]{
        const int r=groups->currentRow()>=0?groups->currentRow():groups->rowCount()-1;
        if(r>=0){groups->removeRow(r);update();}
    });
    load(value);
    dialog.resize(640,480);
    if(dialog.exec()!=QDialog::Accepted)return {};
    auto result=read();std::string created;
    if(execute("editorEditController",[&](auto& d){created=putSignalController(d,result);}))selectDemand(created);
    return created;
}
// What a head shows, in words: "Main · 2 East" for a signal group, "Legacy program p" otherwise.
QString EditorWindow::signalLabel(const std::string& controllerId,int groupNumber,const std::string& programId) const {
    if(controllerId.empty())return text("editorLegacyProgram").arg(QString::fromStdString(programId));
    QString controller=QString::fromStdString(controllerId),group;
    if(const auto& def=history_.document().definition)for(const auto& c:def->signalControllers)if(c.id==controllerId) {
        if(!c.name.empty())controller=QString::fromStdString(c.name);
        for(const auto& g:c.groups)if(g.number==groupNumber)group=QString::fromStdString(g.name);
    }
    return controller+" \u00b7 "+QString::number(groupNumber)+(group.isEmpty()?QString():" "+group);
}
}
