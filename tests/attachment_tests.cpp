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
// Stations are metres along the link now. Naming the same places the old fractions did keeps
// the fixtures readable, and derives the metre value from the geometry rather than hardcoding it.
double at(const ProjectDocument& d,const std::string& link,double fraction) {
    for(const auto& l:d.network.links)if(l.id==link)return fraction*polylineLength(l.geometry);
    throw std::invalid_argument("UNKNOWN_LINK");
}
const Connector& connector(const ProjectDocument& d,const std::string& id) {
    for(const auto& c:d.network.connectors)if(c.id==id)return c;
    throw std::invalid_argument("EDIT_UNKNOWN_CONNECTOR");
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
        auto d=roads(side);const auto id=addConnectorRange(d,{"a","a1",at(d,"a",.4)},{"b","b1",at(d,"b",.6)},3,3);
        attached(d);const auto& c=d.network.connectors.front();
        CHECK(c.geometry.front()!=d.network.links.front().geometry.back());
        const auto json=documentJson(d);CHECK(json["schemaVersion"]==5);
        CHECK(documentJson(parseDocument(Json::parse(json.dump())))==json);
        History h;h.reset(d);h.execute("move",[](auto& m){changeGeometry(m,"a",{{0,10},{30,10},{80,20}});});
        attached(h.document());CHECK(h.document().network.connectors.front().from.station==at(d,"a",.4));
        h.undo();CHECK(documentJson(h.document())==json);h.redo();attached(h.document());
        h.execute("lanes",[](auto& m){changeLanes(m,"b",{3,5,4,3});});attached(h.document());
        h.execute("side",[](auto& m){changeDrivingSide(m,DrivingSide::right);});attached(h.document());
        h.execute("copy",[](auto& m){duplicateObjects(m,{"a","b"},{0,100});});attached(h.document());
        CHECK(h.document().network.connectors.back().from.station==at(d,"a",.4));
        CHECK(h.document().network.connectors.back().to.station==at(d,"b",.6));
        auto copy=d;resetConnectorCurve(copy,id,true);attached(copy);CHECK(copy.network.connectors.front().geometry.size()==2);
    }
}
TEST(attachments, invalid_positions_and_duplicate_movements_roll_back) {
    auto d=roads(DrivingSide::left);addConnectorRange(d,{"a","a1",at(d,"a",.4)},{"b","b1",at(d,"b",.6)},3,3);
    History h;h.reset(d);const auto before=documentJson(d);
    test::throws([&]{h.execute("duplicate",[&](auto& m){addConnector(m,{"a","a2",at(d,"a",.4)},{"b","b2",at(d,"b",.6)});});},"DUPLICATE_CONNECTION");
    CHECK(documentJson(h.document())==before);
    // Past the end of the link is the out-of-range case now; 1.1 is a perfectly good station.
    for(double station:{-.1,at(d,"a",1.)+1,std::numeric_limits<double>::quiet_NaN()}) {
        test::throws([&]{h.execute("invalid",[&](auto& m){addConnector(m,{"a","a1",station},{"b","b1",at(d,"b",.2)});});},"EDIT_CONNECTOR_POSITION");
        CHECK(documentJson(h.document())==before);
    }
    h.execute("different station",[&](auto& m){addConnector(m,{"a","a1",at(d,"a",.2)},{"b","b1",at(d,"b",.3)});});
    CHECK(h.document().network.connectors.size()==2);
}
TEST(attachments, split_remaps_both_source_and_target_and_rejects_cut_through_attachment) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);d.network.links[0].geometry={{0,0},{100,0}};
        addConnectorRange(d,{"a","a1",25},{"b","b1",at(d,"b",.5)},2,2);
        addConnector(d,{"b","b3",at(d,"b",.8)},{"a","a3",75});
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
    auto d=roads(DrivingSide::left);const auto id=addConnector(d,{"a","a1",at(d,"a",.5)},{"b","b1",at(d,"b",.5)});
    const auto diagnostics=documentDiagnostics(d);
    CHECK(std::any_of(diagnostics.begin(),diagnostics.end(),[&](const auto& row){return row.code=="UNSUPPORTED_CONNECTOR_POSITION" && row.selectId==id;}));
    const auto route=putRoute(d,{"",{"a1",id,"b1"}});putInput(d,{"",route,"car",600,0,60});
    validateDocument(d);
    test::throws([&]{compileDocument(d,test::root()/"data");},"UNSUPPORTED_CONNECTOR_POSITION");
    const auto before=documentJson(d);History h;h.reset(d);
    test::throws([&]{h.execute("move station",[&](auto& m){changeConnectorEndpoints(m,id,{"a","a1",at(d,"a",.3)},{"b","b1",at(d,"b",.5)});});},"EDIT_REFERENCED_CONNECTOR");
    CHECK(documentJson(h.document())==before);
}
// The reason M1.13 exists: a fraction of lane arclength slid every interior attachment when a
// link was stretched. A station is metres along the link, so the drawn place stays the drawn place.
TEST(attachments, stretching_a_link_leaves_an_interior_attachment_where_it_was_drawn) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);d.network.links[0].geometry={{0,0},{100,0}};
        const auto id=addConnectorRange(d,{"a","a1",30},{"b","b1",at(d,"b",.5)},2,2);
        const auto before=laneAttachment(d.network,d.network.connectors.front().from,true);
        History h;h.reset(d);
        h.execute("stretch",[](auto& m){changeGeometry(m,"a",{{0,0},{200,0}});});
        // The forcing: the link really did get longer.
        CHECK(polylineLength(h.document().network.links.front().geometry)==200);
        const auto& moved=connector(h.document(),id);
        CHECK(moved.from.station==30);
        const auto after=laneAttachment(h.document().network,moved.from,true);
        test::near(after.x,before.x,1e-9);test::near(after.y,before.y,1e-9);
        attached(h.document());
        // Lane edits keep the reference polyline, so they cannot move an attachment either.
        h.execute("lanes",[](auto& m){resizeLinkLanes(m,"a",5,true);});
        CHECK(connector(h.document(),id).from.station==30);attached(h.document());
    }
}
TEST(attachments, shortening_a_link_clamps_its_attachments_instead_of_rejecting_the_edit) {
    auto d=roads(DrivingSide::left);d.network.links[0].geometry={{0,0},{100,0}};
    const auto id=addConnector(d,{"a","a1",80},{"b","b1",at(d,"b",.5)});
    History h;h.reset(d);
    h.execute("shorten",[](auto& m){changeGeometry(m,"a",{{0,0},{40,0}});});
    // The forcing: the new link is shorter than the station that was stored.
    CHECK(polylineLength(h.document().network.links.front().geometry)==40);
    const auto& clamped=connector(h.document(),id);
    CHECK(clamped.from.station==40);
    const auto lane=laneGeometry(h.document().network.links.front(),"a1",DrivingSide::left);
    CHECK(laneAttachment(h.document().network,clamped.from,true)==lane.back());
    CHECK(validateNetwork(h.document().network).empty());attached(h.document());
    h.undo();CHECK(h.document()==d);
}
// A station names a cross-section, so the whole mouth of a range meets the link square, which
// a per-lane distance could not do where the outer lane is the longer one.
TEST(attachments, a_range_meets_a_curved_link_on_one_cross_section) {
    auto d=roads(DrivingSide::left);
    addConnectorRange(d,{"a","a1",at(d,"a",.5)},{"b","b1",at(d,"b",.5)},3,3);
    const auto paths=connectorPaths(d.network,d.network.connectors.front());
    CHECK(paths.size()==3);
    const auto& link=d.network.links.front();
    // The forcing: the attachment sits on the bent part, where the lanes differ in length.
    const auto inner=laneGeometry(link,"a1",DrivingSide::left),outer=laneGeometry(link,"a3",DrivingSide::left);
    CHECK(std::abs(polylineLength(inner)-polylineLength(outer))>.1);
    const auto a=paths[0].geometry.front(),b=paths[1].geometry.front(),c=paths[2].geometry.front();
    const double cross=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
    test::near(cross,0,1e-9); // collinear: one straight cross-section, not a staircase
    // Between two vertices the cross-section is slanted by the miter it is interpolating
    // towards, so compare the spacings to each other rather than to the nominal widths: the
    // three mouths are spaced exactly as the lane widths are.
    const double first=std::hypot(b.x-a.x,b.y-a.y),second=std::hypot(c.x-b.x,c.y-b.y);
    test::near(first/second,(link.lanes[0].width+link.lanes[1].width)/(link.lanes[1].width+link.lanes[2].width),1e-9);
    CHECK(first>3.4 && first<3.6);
}
// Schema 4 stored a fraction of lane arclength. Reading that number as metres would move every
// attachment, so the version decides the unit and the conversion keeps the world position.
TEST(attachments, schema_four_fractions_migrate_to_stations_at_the_same_place) {
    auto d=roads(DrivingSide::left);
    const auto id=addConnectorRange(d,{"a","a1",at(d,"a",.4)},{"b","b1",at(d,"b",.6)},2,2);
    const auto before=laneAttachment(d.network,connector(d,id).from,true);
    // Rewrite the file the way schema 4 stored it: the same place, as a fraction of the lane's
    // own arclength. The connector geometry is untouched, exactly as an older build left it.
    auto legacy=documentJson(d);legacy["schemaVersion"]=4;
    for(auto& c:legacy["network"]["connectors"])for(const auto* end:{"from","to"}) {
        const double station=c[end]["station"].get<double>();
        const std::string link=c[end]["linkId"],lane=c[end]["laneId"];
        double reference=0,arclength=0,along=0;
        for(const auto& l:d.network.links)if(l.id==link) {
            const auto geometry=laneGeometry(l,lane,d.network.drivingSide);
            reference=polylineLength(l.geometry);arclength=polylineLength(geometry);
            along=matchedStation(l.geometry,geometry,station);
        }
        // The forcing: on this bent link the lane is not the same length as its link, so a
        // fraction read as metres, or a lane distance read as a link station, lands elsewhere.
        CHECK(std::abs(reference-arclength)>.1);
        c[end].erase("station");c[end]["fraction"]=along/arclength;
    }
    const auto migrated=parseDocument(legacy);
    const auto after=laneAttachment(migrated.network,migrated.network.connectors.front().from,true);
    test::near(after.x,before.x,1e-6);test::near(after.y,before.y,1e-6);
    CHECK(documentJson(migrated)["schemaVersion"]==5);
    // The unit is decided by the version, never by which key happens to be present.
    auto mixed=documentJson(d);mixed["network"]["connectors"][0]["from"]["fraction"]=.4;
    test::throws([&]{parseDocument(mixed);},"EDIT_VERSION");
    auto stale=legacy;stale["network"]["connectors"][0]["from"]["station"]=10;
    test::throws([&]{parseDocument(stale);},"EDIT_VERSION");
}
