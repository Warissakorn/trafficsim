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
        const auto json=documentJson(d);CHECK(json["schemaVersion"]==16);
        CHECK(documentJson(parseDocument(Json::parse(json.dump())))==json);
        History h;h.reset(d);h.execute("move",[](auto& m){changeGeometry(m,"a",{{0,10},{30,10},{80,20}});});
        // A Connector keeps its own position, so moving a Link does not drag it: the Link goes and
        // the Connector stands where the author drew it. 10 m across a 3 m lane leaves its end off
        // the road altogether, and a Connector with an end off its Link has nothing to connect --
        // it is deleted in the same transaction, so one Undo brings both back. `connector_tests`
        // carries the other half: a Link edit the end survives, where the station follows it.
        CHECK(h.document().network.connectors.empty());
        attached(h.document());
        h.undo();CHECK(documentJson(h.document())==json);
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
// Was `draft_and_run_diagnostics_do_not_silently_run_wrong_lane_lengths`, which asserted that an
// interior attachment of either kind is refused, then briefly asserted the target half was still
// refused. M3.1 supplied the arbitration, so both halves run and this pins the whole thing: a
// Connector arriving inside a lane body compiles, and the merge it makes carries a derived
// priority rule rather than being accepted silently.
TEST(attachments, an_interior_target_attachment_runs_and_its_merge_is_arbitrated) {
    auto d=roads(DrivingSide::left);const auto id=addConnector(d,{"a","a1",at(d,"a",.5)},{"b","b1",at(d,"b",.5)});
    // The forcing: the target really is inside the body. A connector quietly snapped to the link
    // start would make everything below true without a merge existing at all.
    CHECK(!attachedAtLinkEnd(d.network,connector(d,id).to,false));
    CHECK(connectorRuntimeIssues(d.network).empty());
    const auto route=putRoute(d,{"",{"a1",id,"b1"}});putInput(d,{"",route,"car",600,0,60});
    validateDocument(d);
    const auto snapshot=compileDocument(d,test::root()/"data");
    // b1 is cut, and the arriving path continues DOWNSTREAM of the cut -- the vehicle joins at
    // the drawn metre and never travels the stretch above it.
    const auto segment=[&](const std::string& sid)->const Segment& {
        for(const auto& s:snapshot.scenario.segments)if(s.id==sid)return s;
        throw std::invalid_argument("UNKNOWN_SEGMENT: "+sid);
    };
    CHECK(segment(id).next==std::vector<std::string>({"b1/sec-2"}));
    // The merge is arbitrated, not waved through: the Connector gives way to the lane it joins.
    CHECK(snapshot.scenario.priorityRules.size()==1);
    const auto& rule=snapshot.scenario.priorityRules.front();
    CHECK(rule.yieldSegmentId==id);CHECK(rule.conflictSegmentId=="b1");
    test::near(rule.gapTime,3.0,1e-12);test::near(rule.headway,7.0,1e-12);
    // And the merge would NOT have validated without it -- which is what makes the rule load
    // bearing rather than decorative.
    auto unarbitrated=snapshot.scenario;unarbitrated.priorityRules.clear();
    test::throws([&]{assertValidScenario(unarbitrated);},"UNSUPPORTED_MERGE");
    CHECK(validateScenario(snapshot.scenario).empty());
    // A route arriving part way along b1 travels only the part below the arrival.
    for(const auto& r:snapshot.scenario.routes)if(r.id==route)
        CHECK(r.segmentIds==std::vector<std::string>({"a1",id,"b1/sec-2"}));
    const auto before=documentJson(d);History h;h.reset(d);
    // M1.26: moving the station a routed Connector attaches at is an ordinary edit. The route
    // names the Connector, so it still says the same thing and still expands to one lane.
    h.execute("move station",[&](auto& m){changeConnectorEndpoints(m,id,{"a","a1",at(d,"a",.3)},{"b","b1",at(d,"b",.5)});});
    CHECK(h.document().definition->routes.size()==1);
    CHECK(routeLaneChains(h.document().network,h.document().definition->routes.front().segmentIds).size()==1);
    CHECK(routeRuntimeIssues(h.document().network,*h.document().definition).empty());
    h.undo();CHECK(documentJson(h.document())==before);
}
// The numbers that arbitrate a derived merge are data. Their absence must block Run -- a zero gap
// time is a merge nobody gives way at -- but it must NOT block an edit, which is D18b's boundary.
TEST(attachments, a_merge_without_its_data_catalog_blocks_run_but_not_editing) {
    auto d=roads(DrivingSide::left);const auto id=addConnector(d,{"a","a1",at(d,"a",.5)},{"b","b1",at(d,"b",.5)});
    const auto route=putRoute(d,{"",{"a1",id,"b1"}});putInput(d,{"",route,"car",600,0,60});
    // The forcing: this network really does need a derived rule, and really does have one when
    // the catalog is there.
    CHECK(compileDocument(d,test::root()/"data").scenario.priorityRules.size()==1);
    const auto rows=priorityDefaultsIssues(d.network,{});
    CHECK(rows.size()==1);CHECK(rows.front().code=="EDIT_NO_PRIORITY_DEFAULTS");
    CHECK(rows.front().path=="connectors[0]");
    // Editing is untouched: a missing catalog is not a reason to refuse a drawing.
    History h;h.reset(d);
    h.execute("shape",[&](auto& m){changeGeometry(m,"a",{{0,1},{30,1},{80,11}});});
    CHECK(h.canUndo());h.undo();
    validateDocument(d);
    // And a network with no merge needs no numbers at all.
    auto plain=roads(DrivingSide::left);addConnector(plain,{"a","a1",at(plain,"a",.5)},{"b","b1",0});
    CHECK(priorityDefaultsIssues(plain.network,{}).empty());
    (void)id;
}
// M1.11.1's done-condition, the half M3.1 unblocked: a vehicle ENTERS at the drawn station.
TEST(attachments, a_vehicle_enters_a_lane_at_the_drawn_station_and_gives_way) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);
        d.network.links[0].geometry={{0,0},{100,0}};d.network.links[1].geometry={{140,0},{240,0}};
        const auto id=addConnector(d,{"a","a1",100},{"b","b1",40});
        const auto table=runtimeSections(d.network);
        // The forcing: the arrival cut really is at the drawn metre, so "enters there" has a
        // place to mean. Without this the distance below could be right by coincidence.
        const auto& joined=sectionStartingAt(table,"b1",40);
        test::near(joined.start,40,1e-9);
        CHECK(joined.id=="b1/sec-2");
        test::near(joined.end-joined.start,60,1e-9);
        // The upstream part of b1 is the major approach, and it keeps its own 40 m.
        test::near(sectionForStation(table,"b1",0).end,40,1e-9);
        const auto route=putRoute(d,{"",{"a1",id,"b1"}});putInput(d,{"",route,"car",600,0,60});
        const auto snapshot=compileDocument(d,test::root()/"data");
        // The arriving route's total is 100 m of a1, the connector, and 60 m of b1 -- not 140.
        double total=0;
        for(const auto& r:snapshot.scenario.routes)if(r.id==route)
            for(const auto& sid:r.segmentIds)for(const auto& seg:snapshot.scenario.segments)
                if(seg.id==sid)total+=seg.length;
        const double connectorLength=[&]{
            for(const auto& seg:snapshot.scenario.segments)if(seg.id==id)return seg.length;
            throw std::invalid_argument("UNKNOWN_SEGMENT");}();
        test::near(total,100+connectorLength+60,1e-9);
        // It runs, it replays, and vehicles get through.
        CHECK(runSimulation(snapshot.scenario,42).completed>0);
        const auto stream=[&](std::uint32_t seed){
            std::vector<SimEvent> out;
            runSimulation(snapshot.scenario,seed,[&](const auto& e){out.push_back(e);},true);
            return out;};
        CHECK(stream(42)==stream(42));
    }
}
// The structural half of the merge is not the point of it. This drives the compiled scenario and
// watches an arriving vehicle actually hold at the end of the Connector while the lane it is
// joining is occupied.
TEST(attachments, an_arriving_vehicle_holds_at_the_connector_while_the_lane_is_occupied) {
    auto d=roads(DrivingSide::left);
    d.network.links[0].geometry={{0,0},{100,0}};d.network.links[1].geometry={{140,0},{240,0}};
    const auto id=addConnector(d,{"a","a1",100},{"b","b1",40});
    const auto arriving=putRoute(d,{"arriving",{"a1",id,"b1"}});
    // Link b has three lanes, so a route on it expands to three: the major traffic this test
    // places is the one on b1, the lane the Connector arrives on.
    const auto major=putRoute(d,{"major",{"b1"}});
    const std::string majorLane=major+"/lane-1";
    putInput(d,{"",arriving,"car",600,0,60});
    const auto scenario=compileDocument(d,test::root()/"data").scenario;
    const auto length=[&](const std::string& sid){
        for(const auto& seg:scenario.segments)if(seg.id==sid)return seg.length;
        throw std::invalid_argument("UNKNOWN_SEGMENT");};
    // Route coordinates: the arriving vehicle's stop line is at 100 m of a1 plus the Connector.
    const double stopLine=100+length(id);
    const auto place=[&](std::uint64_t vid,const std::string& route,double distance,double speed){
        return test::Placement{vid,route,distance,speed};};
    // A major vehicle 20 m short of the conflict point at 40 m, moving slowly enough that it
    // stays inside the three-second gap time for several seconds.
    auto state=test::withVehicles(scenario,{place(1,arriving,stopLine-3,0),
                                            place(2,majorLane,20,5)});
    const auto distanceOf=[](const SimState& st,std::uint64_t vid){
        for(const auto& v:st.vehicles)if(v.id==vid)return v.distance;
        return -1.0;};
    // The forcing: the major vehicle really is approaching the conflict point, and really is on
    // the major approach rather than already past it.
    CHECK(distanceOf(state,2)<40);
    bool majorWasApproaching=false;
    for(int i=0;i<40;++i) {
        state=stepSimulation(state);
        const double behind=distanceOf(state,2);
        if(behind>0 && behind<40) {
            majorWasApproaching=true;
            // Held at the stop line, never over it, while the lane it joins is occupied.
            CHECK(distanceOf(state,1)<=stopLine+1e-9);
        }
    }
    CHECK(majorWasApproaching);
    // And with the lane clear, the same vehicle from the same place crosses into it.
    // And with the lane clear, the same vehicle from the same place crosses into it. Checked
    // DURING the run, not after: it goes on to finish the route and leave the network, and a
    // departed vehicle has no distance to compare.
    auto clear=test::withVehicles(scenario,{place(1,arriving,stopLine-3,0)});
    bool crossed=false;
    for(int i=0;i<80 && !crossed;++i) {
        clear=stepSimulation(clear);
        if(distanceOf(clear,1)>stopLine)crossed=true;
    }
    CHECK(crossed);
}

// M1.11.1. A Connector leaving a lane body cuts that lane in two, because the engine's Segment is
// a whole traversable length: a vehicle turning off at 25 m must travel 25 m of the lane, not 100.
TEST(attachments, an_interior_source_attachment_compiles_to_two_sections_of_the_drawn_lengths) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=roads(side);d.network.links[0].geometry={{0,0},{100,0}};
        const auto id=addConnector(d,{"a","a1",25},{"b","b1",0});
        // The forcing: the gate really opened. Without this the length assertions below would
        // pass just as happily on a network that Run still refuses to compile.
        CHECK(connectorRuntimeIssues(d.network).empty());
        CHECK(!attachedAtLinkEnd(d.network,connector(d,id).from,true));
        const auto scenario=buildScenario(d.network,{});
        const auto segment=[&](const std::string& sid)->const Segment& {
            for(const auto& s:scenario.segments)if(s.id==sid)return s;
            throw std::invalid_argument("UNKNOWN_SEGMENT: "+sid);
        };
        test::near(segment("a1").length,25,1e-9);
        test::near(segment("a1/sec-2").length,75,1e-9);
        test::near(segment("a1").length+segment("a1/sec-2").length,
                   polylineLength(laneGeometry(d.network.links[0],"a1",side)),1e-9);
        // The following section first, then what leaves at the cut -- the order the whole-lane
        // compiler emitted, so an uncut lane is unchanged by this rewrite.
        CHECK(segment("a1").next==std::vector<std::string>({"a1/sec-2",id}));
        CHECK(segment("a1/sec-2").next.empty());
        // The connector arrives at the start of b1, which is that lane's only section.
        CHECK(segment(id).next==std::vector<std::string>({"b1"}));
        // A lane with nothing attached to its body is still one segment of its whole length.
        test::near(segment("a2").length,polylineLength(laneGeometry(d.network.links[0],"a2",side)),1e-9);
        // The negative: the cut is read from the station, not from anywhere else. Moving it must
        // move both lengths, or the two numbers above are coincidences.
        auto moved=roads(side);moved.network.links[0].geometry={{0,0},{100,0}};
        addConnector(moved,{"a","a1",50},{"b","b1",0});
        const auto other=buildScenario(moved.network,{});
        const auto length=[&](const Scenario& sc,const std::string& sid) {
            for(const auto& s:sc.segments)if(s.id==sid)return s.length;
            throw std::invalid_argument("UNKNOWN_SEGMENT");
        };
        test::near(length(other,"a1"),50,1e-9);test::near(length(other,"a1/sec-2"),50,1e-9);
    }
}
// A route is authored on whole lanes, because that is what the author draws and stores. What runs
// is the chain of sections it actually travels, and where it leaves part way along, the chain has
// to stop there.
TEST(attachments, a_route_authored_on_whole_lanes_expands_to_the_sections_it_travels) {
    auto d=roads(DrivingSide::left);d.network.links[0].geometry={{0,0},{100,0}};
    const auto turn=addConnector(d,{"a","a1",25},{"b","b1",0});
    const auto ahead=addConnector(d,{"a","a1",100},{"b","b2",0});
    // The forcing: a1 really is in more than one piece, so "expands" has something to expand.
    const auto table=runtimeSections(d.network);
    CHECK(std::count_if(table.sections.begin(),table.sections.end(),
                        [](const auto& s){return s.laneId=="a1";})==2);
    putRoute(d,{"turning",{"a1",turn,"b1"}});
    putRoute(d,{"through",{"a1",ahead,"b2"}});
    const auto scenario=buildScenario(d.network,demand(d));
    const auto ids=[&](const std::string& rid) {
        for(const auto& r:scenario.routes)if(r.id==rid)return r.segmentIds;
        throw std::invalid_argument("UNKNOWN_ROUTE");
    };
    // Turning off at 25 m travels ONLY the first section. The through section being absent is the
    // whole point: carrying it would drive the vehicle 75 m it never drove.
    CHECK(ids("turning")==std::vector<std::string>({"a1",turn,"b1"}));
    // Going straight on travels both sections, in order.
    CHECK(ids("through")==std::vector<std::string>({"a1","a1/sec-2",ahead,"b2"}));
    CHECK(validateScenario(scenario).empty());
    // The negative, in the terms a route is now authored in: objects that do not join up are
    // not quietly stitched together. Naming them is allowed -- an author must be able to draw
    // in any order -- but nothing can travel the route, and Run says so by name.
    auto other=d;putRoute(other,{"bogus",{"b",turn,"a"}});
    const auto issues=routeRuntimeIssues(other.network,demand(other));
    CHECK(issues.size()==1);CHECK(issues.front().code=="UNSUPPORTED_ROUTE_TOPOLOGY");
    CHECK(issues.front().path=="routes[2]");
    test::throws([&]{compileScenario(other.network,demand(other));},"UNSUPPORTED_ROUTE_TOPOLOGY");
}
// A zero-length segment is not something the core accepts, so a cut with no room either side of it
// is the one interior source attachment that still cannot run.
TEST(attachments, an_attachment_too_close_to_a_lane_end_still_blocks_run) {
    auto d=roads(DrivingSide::left);d.network.links[0].geometry={{0,0},{100,0}};
    const auto id=addConnector(d,{"a","a1",0.05},{"b","b1",0});
    // The forcing: 0.05 m is genuinely an interior station, not one that attachedAtLinkEnd has
    // already rounded to the link start. Without this the row below would prove nothing.
    CHECK(!attachedAtLinkEnd(d.network,connector(d,id).from,true));
    const auto rows=documentDiagnostics(d);
    CHECK(std::any_of(rows.begin(),rows.end(),[&](const auto& r){return r.code=="UNSUPPORTED_CONNECTOR_POSITION" && r.selectId==id;}));
    const auto route=putRoute(d,{"",{"a1",id,"b1"}});putInput(d,{"",route,"car",600,0,60});
    test::throws([&]{compileDocument(d,test::root()/"data");},"UNSUPPORTED_CONNECTOR_POSITION");
    // The negative: this is the minimum-length rule, not a blanket refusal of interior sources.
    auto fine=roads(DrivingSide::left);fine.network.links[0].geometry={{0,0},{100,0}};
    addConnector(fine,{"a","a1",5},{"b","b1",0});
    CHECK(connectorRuntimeIssues(fine.network).empty());
    // And two cuts too close to EACH OTHER block, while the same pair further apart does not.
    auto near=roads(DrivingSide::left);near.network.links[0].geometry={{0,0},{100,0}};
    addConnector(near,{"a","a1",30},{"b","b1",0});
    const auto second=addConnector(near,{"a","a1",30.1},{"b","b2",0});
    CHECK(std::any_of(near.network.connectors.begin(),near.network.connectors.end(),
                      [](const auto& c){return true;}));
    const auto blocked=connectorRuntimeIssues(near.network);
    CHECK(std::any_of(blocked.begin(),blocked.end(),[](const auto& i){return i.code=="UNSUPPORTED_CONNECTOR_POSITION";}));
    auto apart=roads(DrivingSide::left);apart.network.links[0].geometry={{0,0},{100,0}};
    addConnector(apart,{"a","a1",30},{"b","b1",0});addConnector(apart,{"a","a1",31},{"b","b2",0});
    CHECK(connectorRuntimeIssues(apart.network).empty());
    (void)second;
}
// A head's position is metres along its lane. Once that lane is in pieces, the head belongs to the
// piece it stands on, at its distance from THAT piece's start -- or it stops traffic in the wrong
// place, which is the kind of error a simulation reports as a plausible number.
TEST(attachments, a_signal_head_on_a_sectioned_lane_stops_traffic_where_it_was_drawn) {
    auto d=roads(DrivingSide::left);d.network.links[0].geometry={{0,0},{100,0}};
    addConnector(d,{"a","a1",25},{"b","b1",0});
    const auto program=putProgram(d,{"",0,{{10,SignalColor::green}}});
    const auto head=putSignalHead(d,{"",{"a","a1"},60,program,{}});
    const auto compiled=buildScenario(d.network,demand(d));
    const auto found=[&](const std::string& hid)->const SignalHead& {
        for(const auto& h:compiled.signalHeads)if(h.id==hid)return h;
        throw std::invalid_argument("UNKNOWN_HEAD");
    };
    // The forcing: the lane really was cut upstream of the head, so a rebase is required.
    CHECK(found(head).segmentId=="a1/sec-2");
    test::near(found(head).position,35,1e-9);
    // The negative: a head UPSTREAM of the cut keeps its own position. An unconditional
    // subtraction, or a rebase onto the wrong section, breaks exactly this line.
    auto early=d;const auto low=putSignalHead(early,{"",{"a","a1"},10,program,{}});
    const auto other=buildScenario(early.network,demand(early));
    const auto at10=std::find_if(other.signalHeads.begin(),other.signalHeads.end(),
                                 [&](const auto& h){return h.id==low;});
    CHECK(at10!=other.signalHeads.end());
    CHECK(at10->segmentId=="a1");test::near(at10->position,10,1e-9);
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
TEST(attachments, shortening_a_link_past_an_attachment_deletes_the_connector_that_hung_off_it) {
    auto d=roads(DrivingSide::left);d.network.links[0].geometry={{0,0},{100,0}};
    const auto id=addConnector(d,{"a","a1",80},{"b","b1",at(d,"b",.5)});
    History h;h.reset(d);
    h.execute("shorten",[](auto& m){changeGeometry(m,"a",{{0,0},{40,0}});});
    // The forcing: the new link really is shorter than the station that was stored, so the end
    // the author placed is 40 m past where the road now stops.
    CHECK(polylineLength(h.document().network.links.front().geometry)==40);
    // A Connector keeps its own position, so the end does not slide back to the new end of the
    // Link: it stands where it was, which is off the Link, and a Connector with an end off its
    // Link has nothing to connect. It goes -- inside the same transaction, so one Undo brings
    // back both the Link and the Connector. This replaced clamping the station, which moved a
    // Connector the author had placed to somewhere they had not.
    test::throws([&]{(void)connector(h.document(),id);},"EDIT_UNKNOWN_CONNECTOR");
    CHECK(h.document().network.connectors.empty());
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
    CHECK(documentJson(migrated)["schemaVersion"]==16);
    // The unit is decided by the version, never by which key happens to be present.
    auto mixed=documentJson(d);mixed["network"]["connectors"][0]["from"]["fraction"]=.4;
    test::throws([&]{parseDocument(mixed);},"EDIT_VERSION");
    auto stale=legacy;stale["network"]["connectors"][0]["from"]["station"]=10;
    test::throws([&]{parseDocument(stale);},"EDIT_VERSION");
}
// A route is stored in the project file. Offering a derived section id as something to store would
// put a copy of derived data in there, and the next edit that re-sections the lane would leave the
// route naming something that no longer exists. So the author picks whole lanes -- but the interior
// diverge still has to be reachable, or the turn cannot be authored at all.
TEST(attachments, the_authoring_view_offers_lanes_and_interior_connectors_but_never_sections) {
    auto d=roads(DrivingSide::left);d.network.links[0].geometry={{0,0},{100,0}};
    const auto turn=addConnector(d,{"a","a1",25},{"b","b1",0});
    const auto table=runtimeSections(d.network);
    // The forcing: a1 really is sectioned, so there is a section id available to leak.
    CHECK(std::count_if(table.sections.begin(),table.sections.end(),
                        [](const auto& s){return s.laneId=="a1";})==2);
    const auto offered=authoringSegments(table);
    for(const auto& segment:offered) {
        CHECK(segment.id.find("/sec-")==std::string::npos);
        for(const auto& next:segment.next)CHECK(next.find("/sec-")==std::string::npos);
    }
    const auto lane=[&](const std::string& id)->const Segment& {
        for(const auto& s:offered)if(s.id==id)return s;
        throw std::invalid_argument("UNKNOWN_SEGMENT");
    };
    // One row per authored lane, carrying the whole lane's length -- what the author drew.
    CHECK(offered.size()==d.network.links[0].lanes.size()+d.network.links[1].lanes.size());
    test::near(lane("a1").length,100,1e-9);
    // And the interior connector is selectable from the lane, which is the union that makes a
    // turn off the middle of a link authorable. Without it the diverge would be invisible.
    CHECK(std::find(lane("a1").next.begin(),lane("a1").next.end(),turn)!=lane("a1").next.end());
}
