#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/diagnostics.hpp"
#include "../src/project/run.hpp"
#include <limits>
using namespace trafficsim;
namespace {
ProjectDocument roads(DrivingSide side) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={
        {"a",{{0,0},{30,0},{80,10}},{{"a1",3},{"a2",4},{"a3",3.5}}},
        {"b",{{100,20},{140,20},{160,60}},{{"b1",4},{"b2",3},{"b3",3.5}}}};
    return d;
}
void attached(const ProjectDocument& d) {
    CHECK(validateNetwork(d.network).empty());
    for(const auto& c:d.network.connectors)for(const auto& path:connectorPaths(d.network,c)) {
        const auto a=laneAttachment(d.network,path.from,true),b=laneAttachment(d.network,path.to,false);
        test::near(path.geometry.front().x,a.x,1e-9);test::near(path.geometry.front().y,a.y,1e-9);
        test::near(path.geometry.back().x,b.x,1e-9);test::near(path.geometry.back().y,b.y,1e-9);
    }
}
}
TEST(attachments, interior_positions_ranges_persistence_edits_and_history) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);const auto id=addConnectorRange(d,{"a","a1",.4},{"b","b1",.6},3,3);
        attached(d);const auto& c=d.network.connectors.front();
        CHECK(c.geometry.front()!=d.network.links.front().geometry.back());
        const auto json=documentJson(d);CHECK(json["schemaVersion"]==3);
        CHECK(documentJson(parseDocument(Json::parse(json.dump())))==json);
        History h;h.reset(d);h.execute("move",[](auto& m){changeGeometry(m,"a",{{0,10},{30,10},{80,20}});});
        attached(h.document());CHECK(h.document().network.connectors.front().from.fraction==.4);
        h.undo();CHECK(documentJson(h.document())==json);h.redo();attached(h.document());
        h.execute("lanes",[](auto& m){changeLanes(m,"b",{3,5,4,3});});attached(h.document());
        h.execute("side",[](auto& m){changeDrivingSide(m,DrivingSide::right);});attached(h.document());
        h.execute("copy",[](auto& m){duplicateObjects(m,{"a","b"},{0,100});});attached(h.document());
        CHECK(h.document().network.connectors.back().from.fraction==.4);
        CHECK(h.document().network.connectors.back().to.fraction==.6);
        auto copy=d;resetConnectorCurve(copy,id,true);attached(copy);CHECK(copy.network.connectors.front().geometry.size()==2);
    }
}
TEST(attachments, invalid_positions_and_duplicate_movements_roll_back) {
    auto d=roads(DrivingSide::left);addConnectorRange(d,{"a","a1",.4},{"b","b1",.6},3,3);
    History h;h.reset(d);const auto before=documentJson(d);
    test::throws([&]{h.execute("duplicate",[](auto& m){addConnector(m,{"a","a2",.4},{"b","b2",.6});});},"DUPLICATE_CONNECTION");
    CHECK(documentJson(h.document())==before);
    for(double fraction:{-.1,1.1,std::numeric_limits<double>::quiet_NaN()}) {
        test::throws([&]{h.execute("invalid",[&](auto& m){addConnector(m,{"a","a1",fraction},{"b","b1",.2});});},"EDIT_CONNECTOR_POSITION");
        CHECK(documentJson(h.document())==before);
    }
    h.execute("different station",[](auto& m){addConnector(m,{"a","a1",.2},{"b","b1",.3});});
    CHECK(h.document().network.connectors.size()==2);
}
TEST(attachments, split_remaps_both_source_and_target_and_rejects_cut_through_attachment) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);d.network.links[0].geometry={{0,0},{100,0}};
        addConnectorRange(d,{"a","a1",.25},{"b","b1",.5},2,2);
        addConnector(d,{"b","b3",.8},{"a","a3",.75});
        const auto before=d.network.connectors;History h;h.reset(d);
        test::throws([&]{h.execute("cut attachment",[](auto& m){splitLink(m,"a",25);});},"EDIT_SPLIT_ATTACHMENT");
        CHECK(h.document()==d);
        h.execute("split",[](auto& m){splitLink(m,"a",50);});attached(h.document());
        const auto& after=h.document().network.connectors;
        CHECK(after[0].from.linkId=="a");CHECK(after[1].to.linkId!="a");
        test::near(after[0].geometry.front().x,before[0].geometry.front().x);
        test::near(after[1].geometry.back().x,before[1].geometry.back().x);
        h.undo();CHECK(h.document()==d);
    }
}
TEST(attachments, draft_and_run_diagnostics_do_not_silently_run_wrong_lane_lengths) {
    auto d=roads(DrivingSide::left);const auto id=addConnector(d,{"a","a1",.5},{"b","b1",.5});
    const auto diagnostics=documentDiagnostics(d);
    CHECK(std::any_of(diagnostics.begin(),diagnostics.end(),[&](const auto& row){return row.code=="UNSUPPORTED_CONNECTOR_POSITION" && row.selectId==id;}));
    const auto route=putRoute(d,{"",{"a1",id,"b1"}});putInput(d,{"",route,"car",600,0,60});
    validateDocument(d);
    test::throws([&]{compileDocument(d,test::root()/"data");},"UNSUPPORTED_CONNECTOR_POSITION");
    const auto before=documentJson(d);History h;h.reset(d);
    test::throws([&]{h.execute("move station",[&](auto& m){changeConnectorEndpoints(m,id,{"a","a1",.3},{"b","b1",.5});});},"EDIT_REFERENCED_CONNECTOR");
    CHECK(documentJson(h.document())==before);
}
