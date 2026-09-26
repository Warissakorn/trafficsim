#include "editor_window.hpp"
#include <QAbstractButton>
#include <QComboBox>
#include <QMenu>
#include <QMessageBox>
#include <QLabel>
#include <QStatusBar>
#include <QTableWidget>
#include <algorithm>

namespace trafficsim {
namespace {
// The route a vehicle input placed on this Link would feed: one that starts there. An input
// belongs to a route in this engine, so placing one by pointer still has to name a route.
std::string routeStartingOn(const AuthoringDefinition& definition,const std::string& linkId) {
    for(const auto& route:definition.routes)
        if(!route.segmentIds.empty() && route.segmentIds.front()==linkId)return route.id;
    return {};
}
}
void EditorWindow::buildRouting() {
    canvas_->routeDraftCommitted=[this](const auto& segments){commitDrawnRoute(segments);};
    canvas_->inputPlaced=[this](const auto& link){placeInputOnLink(link);};
    canvas_->contextMenuRequested=[this](QPoint position){showDemandMenu(position);};
    // The pointer tools are gestures with no dialog to explain them, so the status bar says
    // what a click will do while one of them is chosen.
    toolHint_=new QLabel(this);toolHint_->setObjectName("editorToolHint");
    toolHint_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    statusBar()->addWidget(toolHint_,1);
    connect(tool_,&QComboBox::currentIndexChanged,this,[this]{refreshToolHint();});
    refreshToolHint();
}
void EditorWindow::refreshToolHint() {
    if(!toolHint_)return;
    const int index=tool_->currentIndex();
    toolHint_->setText(index==6?text("editorRouteClickHelp"):index==7?text("editorInputPlaceHelp"):index==8?text("editorHeadPlaceHelp"):index==10?text("editorCounterPlaceHelp"):QString{});
    toolHint_->setToolTip(toolHint_->text());
}
void EditorWindow::commitDrawnRoute(const std::vector<std::string>& segmentIds) {
    if(segmentIds.empty())return;
    std::string created;
    // The same command the dialog commits through, so a route drawn by pointer and a route
    // typed into the dialog are the same object with the same history entry.
    if(execute("editorEditRoute",[&](auto& d){created=putRoute(d,Route{{},segmentIds});})) {
        selectDemand(created);canvas_->setHighlightedRoute(created);
    }
}
void EditorWindow::placeInputOnLink(const std::string& linkId) {
    const auto existing=history_.document().definition
        ?routeStartingOn(*history_.document().definition,linkId):std::string{};
    if(!existing.empty()) {editInput({},existing);return;}
    // No route starts here: as in Vissim, the input needs none (M2.1.1). Its vehicles follow the
    // network from this Link -- equal shares at each branch, a placed routing decision's flows
    // where they meet one -- so the dialog opens on exactly that.
    editInput({},{},linkId);
}
void EditorWindow::syncHighlightedRoute() {
    if(!routeTable_)return;
    std::string route;
    const auto* cell=routeTable_->item(routeTable_->currentRow(),0);
    if(cell && routeTable_->selectionModel() && routeTable_->selectionModel()->hasSelection())
        route=cell->data(Qt::UserRole).toString().toStdString();
    if(route.empty() && inputTable_) {
        // An input row highlights the route it feeds: that is the thing drawn on the canvas.
        const auto* input=inputTable_->item(inputTable_->currentRow(),0);
        if(input && inputTable_->selectionModel() && inputTable_->selectionModel()->hasSelection()) {
            const auto id=input->data(Qt::UserRole).toString().toStdString();
            if(history_.document().definition)
                for(const auto& value:history_.document().definition->inputs)
                    if(value.id==id)route=value.routeId;
        }
    }
    canvas_->setHighlightedRoute(route);
}
void EditorWindow::showDemandMenu(QPoint position) {
    const auto [kind,id]=canvas_->demandObjectAt(position);
    if(kind.empty())return;
    QMenu menu(this);menu.setObjectName("editorDemandMenu");
    auto* edit=menu.addAction(text(kind=="input"?"editorEditInput":kind=="decision"?"editorEditDecision":"editorEditRoute"));
    edit->setObjectName("editorDemandMenuEdit");
    auto* remove=menu.addAction(text(kind=="input"?"editorDeleteInput":kind=="decision"?"editorDeleteDecision":"editorDeleteRoute"));
    remove->setObjectName("editorDemandMenuDelete");
    auto* reveal=menu.addAction(text("editorDemandMenuReveal"));
    reveal->setObjectName("editorDemandMenuReveal");
    const auto* chosen=menu.exec(canvas_->viewport()->mapToGlobal(position));
    if(chosen==edit) {if(kind=="input")editInput(id);else if(kind=="decision")editDecision(id);else editRoute(id);}
    else if(chosen==remove)deleteDemand(kind,id);
    else if(chosen==reveal)selectDemand(id);
}
}
