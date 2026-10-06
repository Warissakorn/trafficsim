#include "../src/editor/canvas.hpp"
#include "../src/editor/canvas_style.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/simulation.hpp"
#include <QApplication>
#include <QFocusEvent>
#include <QGraphicsSimpleTextItem>
#include <QTest>
#include <cmath>
#include <iostream>

using namespace trafficsim;
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
QGraphicsItem* drawn(EditorCanvas& c,const char* kind) {
    for(auto* item:c.scene()->items())if(item->data(0).toString()==kind)return item;
    return nullptr;
}
QPoint middle(EditorCanvas& c,const Network& n,const std::string& id) {
    const auto g=objectGeometry(n,id);
    const auto p=pointAlong(g,polylineLength(g)/2);
    const auto at=c.mapFromScene(p.x,p.y);
    require(c.viewport()->rect().contains(at),"Fixture point outside viewport");return at;
}
void click(EditorCanvas& c,QPoint p) {QTest::mouseClick(c.viewport(),Qt::LeftButton,{},p);}
void move(EditorCanvas& c,QPoint p) {QTest::mouseMove(c.viewport(),p);QApplication::processEvents();}
void geometry() {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        ProjectDocument d;d.network.drivingSide=side;
        const auto a=addLink(d,{{0,0},{30,0},{80,25}},3,3.5);
        const auto& l=d.network.links.front();
        NetworkSignalHead h;h.lane={a,l.lanes[1].id};h.position=45;
        const auto bar=signalCrossbar(d.network,h);require(bar.has_value(),"Curved lane has no crossbar");
        const auto lane=laneGeometry(l,h.lane.laneId,side);
        const auto at=pointAlong(lane,h.position),u=directionAlong(lane,h.position,false);
        for(const auto p:{bar->first,bar->second})
            require(std::abs((p.x-at.x)*u.x+(p.y-at.y)*u.y)<1e-6,"Crossbar is not normal to traffic");
        require(std::abs(std::hypot(bar->first.x-bar->second.x,bar->first.y-bar->second.y)-3.5)<1e-6,
            "Signal crossbar extends outside its lane");
        const auto start=objectCrossbar(d.network,a,false);require(start.has_value(),"No road start bar");
        require(std::abs(std::hypot(start->first.x-start->second.x,start->first.y-start->second.y)-10.5)<1e-6,
            "Input/route crossbar does not span the carriageway");
        for(const double scale:{.25,4.,20.}) {
            RoadCrossbarItem item(*bar,canvasStyle::active(),scale);
            const QPointF centre((bar->first.x+bar->second.x)/2,(bar->first.y+bar->second.y)/2);
            require(item.pen().isCosmetic() && item.pen().widthF()==editorDesign::crossbarPixels,"Stroke scales with zoom");
            require(item.shape().contains(centre+QPointF(u.x*4/scale,u.y*4/scale)),"Hit target shrinks at low zoom");
            const QPointF edge(bar->first.x,bar->first.y);
            const auto outward=edge-centre;
            require(!item.shape().contains(edge+outward*.01),"Hit target spills into the adjacent lane");
        }
        const auto fromLane=l.lanes.front().id;
        const auto b=addLink(d,{{110,65},{180,65}},1,3.5);
        addConnector(d,{a,fromLane},{b,d.network.links.back().lanes.front().id});
        const auto path=connectorPaths(d.network,d.network.connectors.front()).front();
        h.lane={};h.connectorId=path.id;
        for(const double station:{0.,connectorPathLength(path)/2,connectorPathLength(path)}) {
            h.position=station;
            require(signalCrossbar(d.network,h).has_value(),"Connector-mounted head lost its crossbar");
        }
    }
}
void trace(EditorCanvas& c) {
    ProjectDocument d;
    const auto start=addLink(d,{{-100,0},{-30,0}},1,3.5);
    const auto upper=addLink(d,{{5,40},{60,40}},1,3.5);
    const auto lower=addLink(d,{{5,-40},{60,-40}},1,3.5);
    const auto end=addLink(d,{{100,0},{160,0}},1,3.5);
    const auto isolated=addLink(d,{{100,75},{160,75}},1,3.5);
    const auto lane=[&](int k){return d.network.links[k].lanes.front().id;};
    const auto a=addConnector(d,{start,lane(0)},{upper,lane(1)});
    addConnector(d,{start,lane(0)},{lower,lane(2)});
    const auto b=addConnector(d,{upper,lane(1)},{end,lane(3)});
    addConnector(d,{lower,lane(2)},{end,lane(3)});
    c.setDocument(&d);c.fitNetwork();c.setTool(EditorCanvas::Tool::route);
    int commits=0,rejects=0;std::vector<std::string> saved;
    c.routeDraftCommitted=[&](auto ids){++commits;saved=std::move(ids);};c.creationRejected=[&]{++rejects;};
    const auto from=middle(c,d.network,start),to=middle(c,d.network,end);
    click(c,from);require(c.routeDraft()==std::vector<std::string>{start},"Click did not start trace");
    move(c,to);require(!routeChainTo(d.network,{start},end).size(),"Ambiguity fixture was not ambiguous");
    click(c,to);require(commits==0 && rejects==1,"Ambiguous destination guessed a branch");
    move(c,middle(c,d.network,upper));
    require(c.routePreview()==std::vector<std::string>({start,a,upper}),"Hover did not remember the chosen branch");
    move(c,to);require(c.routePreview()==std::vector<std::string>({start,a,upper,b,end}),"Preview missed the destination chain");
    require(drawn(c,"route-start") && drawn(c,"route-end") && !drawn(c,"route-band"),"Preview is not road crossbars");
    require(commits==0 && !d.definition,"Hover mutated the authored document");
    move(c,middle(c,d.network,isolated));click(c,middle(c,d.network,isolated));
    require(commits==0 && rejects==2,"Unreachable destination committed");
    move(c,to);QTest::keyClick(&c,Qt::Key_Backspace);
    require(c.routePreview()==std::vector<std::string>({start,a,upper,b}),"Backspace did not go back");
    click(c,to);require(commits==1 && saved==std::vector<std::string>({start,a,upper,b,end}),"Final click did not commit once");
    require(c.routeDraft().empty() && c.routePreview().empty(),"Commit retained trace state");
    click(c,from);move(c,middle(c,d.network,upper));QTest::keyClick(&c,Qt::Key_Escape);
    require(commits==1 && c.routePreview().empty(),"Escape committed or retained preview");
    click(c,from);QFocusEvent blur(QEvent::FocusOut);QApplication::sendEvent(&c,&blur);
    require(c.routeDraft().empty(),"Focus loss retained trace");
    click(c,from);c.setTool(EditorCanvas::Tool::input);
    require(c.routeDraft().empty() && c.routePreview().empty(),"Tool change retained trace");
    c.routeDraftCommitted={};c.creationRejected={};c.setDocument(nullptr);
}
void signalsAndInputs(EditorCanvas& c,const char* preview) {
    ProjectDocument d;
    const auto link=addLink(d,{{-70,0},{30,0}},3,3.5);
    const auto route=putRoute(d,{{},{link}});
    VehicleInput input;input.routeId=route;input.vehicleTypeId="car";input.vehiclesPerHour=900;input.endTime=60;input.laneShares={1,0,1};
    const auto id=putInput(d,input);
    const auto program=putProgram(d,{{},0,{{10,SignalColor::green},{3,SignalColor::amber},{10,SignalColor::red}}});
    const auto head=putSignalHead(d,{{},{link,d.network.links.front().lanes[1].id},50,program,{}});
    c.setDocument(&d);c.fitNetwork();c.setDemandLabels("veh/h","%1 periods");
    int inputs=0;
    for(auto* item:c.scene()->items())if(item->data(0).toString()=="input-marker") {
        auto* bar=dynamic_cast<RoadCrossbarItem*>(item);require(bar,"Input still uses a chevron");++inputs;
        require(std::abs(bar->line().length()-3.5)<1e-6,"Input includes its excluded lane");
        const auto found=c.demandObjectAt(c.mapFromScene(bar->line().center()));
        require(found==std::pair<std::string,std::string>{"input",id},"Crossbar is not selectable");
    }
    require(inputs==2,"Non-contiguous input lanes were not separate bars");
    auto* stop=dynamic_cast<RoadCrossbarItem*>(drawn(c,"stop-line"));require(stop,"Head still uses a dot");
    const auto original=stop->line();
    Scenario scenario;scenario.duration=60;scenario.timeStep=.1;
    scenario.signalPrograms=d.definition->signalPrograms;scenario.signalHeads={{head,"unused",50,program}};
    SimState state;state.scenario=std::make_shared<const Scenario>(scenario);
    c.setRunNetwork(d.network);
    for(const auto& [time,colour]:{std::pair{0.,canvasStyle::ok()},std::pair{11.,canvasStyle::warning()},std::pair{14.,canvasStyle::error()}}) {
        state.time=time;c.setRunFrame(state);
        auto* bar=dynamic_cast<RoadCrossbarItem*>(drawn(c,"run-signal"));require(bar,"Run signal still uses a dot");
        require(bar->line()==original && bar->pen().color()==colour,"Run bar position or colour disagrees with the signal");
    }
    c.clearRunFrame();require(drawn(c,"stop-line") && !drawn(c,"run-signal"),"Reset did not restore authored heads");
    c.setHighlightedRoute(route);
    if(preview)require(c.grab().save(QString::fromLocal8Bit(preview)),"Could not save visual review image");
    c.setVisibleLevel(1);require(!drawn(c,"input-marker") && !drawn(c,"stop-line"),"Hidden level leaked crossbars");
    c.setVisibleLevel({});c.setDocument(nullptr);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        geometry();EditorCanvas canvas;canvas.resize(1000,700);canvas.show();QTest::qWait(20);
        trace(canvas);signalsAndInputs(canvas,argc>1?argv[1]:nullptr);std::cout<<"road crossbars ok\n";
    } catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
