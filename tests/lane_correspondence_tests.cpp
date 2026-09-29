#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include <tuple>
#include <utility>
using namespace trafficsim;
// M3.2.9a (D73): the lanes of a Connector's two ends pair one to one, and at most one lane is
// added or dropped on each side.
namespace {
ProjectDocument roads(DrivingSide side) {
    ProjectDocument d;d.network.drivingSide=side;
    d.network.links={
        {"a",{{0,0},{40,0}},{{"a1",3},{"a2",3},{"a3",3},{"a4",3},{"a5",3},{"a6",3}}},
        {"b",{{60,10},{120,10}},{{"b1",3},{"b2",3},{"b3",3},{"b4",3},{"b5",3},{"b6",3}}}};
    return d;
}
using Pairs=std::vector<std::pair<int,int>>;
Pairs pairing(const ProjectDocument& d) {
    Pairs result;
    for(const auto& p:connectorPaths(d.network,d.network.connectors.front()))
        result.push_back({p.from.laneId.back()-'1',p.to.laneId.back()-'1'});
    return result;
}
Pairs connect(DrivingSide side,int from,int to,std::optional<LaneSide> choice={}) {
    auto d=roads(side);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},from,to);
    if(choice)changeConnectorLaneSide(d,id,choice);
    return pairing(d);
}
}
TEST(lane_correspondence, equal_ends_pair_lane_for_lane) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})
        CHECK(connect(side,3,3)==Pairs({{0,0},{1,1},{2,2}}));
}
TEST(lane_correspondence, one_added_lane_goes_on_the_chosen_side_and_defaults_to_the_kerb) {
    // Lane 0 is the kerb lane on both driving sides.
    const Pairs kerb{{0,0},{0,1},{1,2}},median{{0,0},{1,1},{1,2}};
    CHECK(connect(DrivingSide::right,2,3)==kerb);
    CHECK(connect(DrivingSide::right,2,3,LaneSide::right)==kerb);
    CHECK(connect(DrivingSide::right,2,3,LaneSide::left)==median);
    CHECK(connect(DrivingSide::left,2,3)==kerb);
    CHECK(connect(DrivingSide::left,2,3,LaneSide::left)==kerb);
    CHECK(connect(DrivingSide::left,2,3,LaneSide::right)==median);
}
TEST(lane_correspondence, a_dropped_lane_follows_the_same_rule_in_reverse) {
    CHECK(connect(DrivingSide::right,3,2)==Pairs({{0,0},{1,0},{2,1}}));
    CHECK(connect(DrivingSide::right,3,2,LaneSide::left)==Pairs({{0,0},{1,1},{2,1}}));
}
TEST(lane_correspondence, two_lanes_differ_by_one_on_each_side) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        CHECK(connect(side,2,4)==Pairs({{0,0},{0,1},{1,2},{1,3}}));
        CHECK(connect(side,4,2)==Pairs({{0,0},{1,0},{2,1},{3,1}}));
        CHECK(connect(side,1,3)==Pairs({{0,0},{0,1},{0,2}}));
    }
}
TEST(lane_correspondence, a_wider_request_is_narrowed_to_what_can_pair) {
    // M3.2.9d (D75): creating across more than a two-lane difference keeps the narrower end and
    // the wider end's first lane, and drops the lanes the rule cannot pair.
    for(const auto [from,to,fitFrom,fitTo]:{std::tuple{2,5,2,4},{5,2,4,2},{1,4,1,3},{1,6,1,3}}) {
        auto d=roads(DrivingSide::right);addConnectorRange(d,{"a","a1"},{"b","b1"},from,to);
        const auto& c=d.network.connectors.front();
        CHECK(c.fromLaneCount==fitFrom);CHECK(c.toLaneCount==fitTo);
        CHECK(c.from.laneId=="a1");CHECK(c.to.laneId=="b1");
    }
    // An explicit resize is still refused: its tabs already stop at the limit.
    auto d=roads(DrivingSide::right);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,4);
    test::throws([&]{changeConnectorRange(d,id,2,5);},"EDIT_LANE_RANGE");
    // An old file may still hold one: it loads, and validation names it.
    d.network.connectors.front().toLaneCount=5;
    CHECK(!validateNetwork(d.network).empty());
}
TEST(lane_correspondence, a_range_centres_on_the_lane_it_was_dropped_on) {
    CHECK(centredLaneRange(0,4,5)==0);CHECK(centredLaneRange(4,4,5)==1);
    CHECK(centredLaneRange(2,4,5)==1);CHECK(centredLaneRange(3,3,6)==2);
    CHECK(centredLaneRange(5,3,6)==3);CHECK(centredLaneRange(0,1,1)==0);
    CHECK(centredLaneRange(2,6,4)==0); // a run longer than the Link starts at its first lane
}
TEST(lane_correspondence, moving_an_end_onto_a_narrow_link_narrows_the_other_end) {
    auto d=roads(DrivingSide::right);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},4,4);
    // The forcing: b6 is the last lane, so the moved end can only keep one.
    changeConnectorEndpoints(d,id,{"a","a1"},{"b","b6"});
    const auto& c=d.network.connectors.front();
    CHECK(c.toLaneCount==1);CHECK(c.fromLaneCount==3);CHECK(validateNetwork(d.network).empty());
}
TEST(lane_correspondence, a_side_only_means_something_for_a_one_lane_difference) {
    auto d=roads(DrivingSide::right);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,4);
    test::throws([&]{changeConnectorLaneSide(d,id,LaneSide::left);},"EDIT_LANE_RANGE");
    d.network.connectors.front().laneChangeSide=LaneSide::left;
    CHECK(!validateNetwork(d.network).empty());
    // A resize that changes the difference forgets the side it no longer describes.
    auto e=roads(DrivingSide::right);const auto other=addConnectorRange(e,{"a","a1"},{"b","b1"},2,3);
    changeConnectorLaneSide(e,other,LaneSide::left);
    changeConnectorRange(e,other,2,4);CHECK(!e.network.connectors.front().laneChangeSide);
}
TEST(lane_correspondence, the_kerb_default_is_the_proportional_pairing_for_one_lane) {
    // Schemas 1-16 paired lanes proportionally. For a one-lane difference that is exactly the
    // kerb-side rule, so no existing file changes its paths.
    for(int m=1;m<=5;++m)for(const auto [from,to]:{std::pair{m,m+1},{m+1,m}}) {
        const auto pairs=connect(DrivingSide::right,from,to);
        const int count=std::max(from,to);
        for(int i=0;i<count;++i) {
            CHECK(pairs[i].first==i*(from-1)/(count-1));
            CHECK(pairs[i].second==i*(to-1)/(count-1));
        }
    }
}
TEST(lane_correspondence, the_added_lane_is_the_one_that_tapers) {
    // The width of the added lane closes to zero at the narrower end, at that side's edge.
    auto d=roads(DrivingSide::right);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},2,3);
    auto widths=connectorLaneWidths(d.network,d.network.connectors.front());
    CHECK(widths.source==std::vector<double>({0,3,3}));
    changeConnectorLaneSide(d,id,LaneSide::left);
    widths=connectorLaneWidths(d.network,d.network.connectors.front());
    CHECK(widths.source==std::vector<double>({3,3,0}));
    addConnectorRange(d,{"a","a3"},{"b","b3"},2,4);
    widths=connectorLaneWidths(d.network,d.network.connectors.back());
    CHECK(widths.source==std::vector<double>({0,3,3,0}));
}
TEST(lane_correspondence, the_side_is_saved_undone_and_refused_before_schema_17) {
    auto d=roads(DrivingSide::left);const auto id=addConnectorRange(d,{"a","a1"},{"b","b1"},3,2);
    History h;h.reset(d);
    h.execute("side",[&](auto& m){changeConnectorLaneSide(m,id,LaneSide::right);});
    CHECK(h.document().network.connectors.front().laneChangeSide==LaneSide::right);
    auto json=documentJson(h.document());
    CHECK(json.at("schemaVersion")==17);
    CHECK(json.at("network").at("connectors").at(0).at("laneChangeSide")=="right");
    CHECK(parseDocument(Json::parse(json.dump()))==h.document());
    h.undo();CHECK(h.document()==d);
    CHECK(!documentJson(d).at("network").at("connectors").at(0).contains("laneChangeSide"));
    json.at("schemaVersion")=16;
    test::throws([&]{parseDocument(json);});
}
TEST(lane_correspondence, a_dragged_tab_puts_the_taper_on_its_own_edge) {
    // D83: widening 2 -> 2 by the far-edge tab adds the far lane, so that lane is the taper.
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        const auto far=side==DrivingSide::left?LaneSide::right:LaneSide::left;
        auto d=roads(side);const auto id=addConnectorRange(d,{"a","a2"},{"b","b2"},2,2);
        changeConnectorRange(d,id,2,3,false,true);
        CHECK(d.network.connectors.front().laneChangeSide==far);
        CHECK(connectorLaneWidths(d.network,d.network.connectors.front()).source==std::vector<double>({3,3,0}));
        CHECK(pairing(d)==Pairs({{1,1},{2,2},{2,3}}));
        // The kerb-edge tab adds the kerb lane, which is the kerb default.
        auto e=roads(side);const auto other=addConnectorRange(e,{"a","a2"},{"b","b2"},2,2);
        changeConnectorRange(e,other,2,3,true,true);
        CHECK(!e.network.connectors.front().laneChangeSide);
        CHECK(connectorLaneWidths(e.network,e.network.connectors.front()).source==std::vector<double>({0,3,3}));
        CHECK(pairing(e)==Pairs({{1,0},{1,1},{2,2}}));
        // From a two-lane difference the far tab drops the far extra lane; the kerb one is left.
        auto f=roads(side);const auto wide=addConnectorRange(f,{"a","a2"},{"b","b1"},2,4);
        const auto before=pairing(f);
        changeConnectorRange(f,wide,2,3,false,true);
        CHECK(!f.network.connectors.front().laneChangeSide);
        CHECK(pairing(f)==Pairs({before[0],before[1],before[2]}));
        // A count typed into the Inspector says nothing about an edge: the kerb default stays.
        auto g=roads(side);const auto typed=addConnectorRange(g,{"a","a2"},{"b","b2"},2,2);
        changeConnectorRange(g,typed,2,3);
        CHECK(!g.network.connectors.front().laneChangeSide);
    }
}
