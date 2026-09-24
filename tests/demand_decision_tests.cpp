#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <cmath>
using namespace trafficsim;
// M2.4: static turning proportions. A decision's relative flows split an input's volume across
// routes leaving one Link; resolving it gives ordinary routed inputs, so core/ is untouched.
namespace {
// The West approach of the four-leg fixture: through, left and right leave the same Link.
struct West { ProjectDocument document; std::vector<std::string> routes; };
West west() {
    auto built = fixture::fourLegIntersection();
    West w{std::move(built.document), {built.routes[0], built.routes[1], built.routes[2]}};
    // Drop the fixture's own demand, so what runs is what each test authors.
    w.document.definition->inputs.clear();
    return w;
}
}
TEST(demand, a_decision_compiles_to_the_hand_split_inputs) {
    auto a = west(), b = west();
    const auto decision = putRoutingDecision(a.document, {"", "West turns",
        {{a.routes[0], 500}, {a.routes[1], 120}, {a.routes[2], 100}}});
    VehicleInput input{"in", "", "car", 720, 0, 900, {}, {}};
    input.routingDecisionId = decision;
    putInput(a.document, input);
    validateDocument(a.document);
    // The same thing by hand: one input per route at its share of 720.
    for (std::size_t k = 0; k < 3; ++k)
        putInput(b.document, {"in/route-" + b.routes[k], b.routes[k], "car",
                              std::vector<double>{500, 120, 100}[k], 0, 900, {}, {}});
    const auto x = compileDocument(a.document, test::root() / "data").scenario;
    const auto y = compileDocument(b.document, test::root() / "data").scenario;
    CHECK(x.inputs.size() == y.inputs.size());
    for (std::size_t i = 0; i < x.inputs.size(); ++i) {
        CHECK(x.inputs[i].id == y.inputs[i].id); CHECK(x.inputs[i].routeId == y.inputs[i].routeId);
        test::near(x.inputs[i].vehiclesPerHour, y.inputs[i].vehiclesPerHour, 1e-9);
    }
}
TEST(demand, a_decision_with_counted_intervals_scales_each) {
    auto w = west();
    const auto decision = putRoutingDecision(w.document, {"", "", {{w.routes[0], 3}, {w.routes[1], 1}}});
    VehicleInput input{"in", "", "car", 0, 0, 0, {}, {}};
    input.routingDecisionId = decision; input.intervals = {{0, 450, 400}, {450, 900, 800}};
    putInput(w.document, input);
    // The second period's share for route 1, summed over however many lanes it expands to:
    // what is under test is the scaling, not the lane expansion, which has its own tests.
    const auto prefix = "in/route-" + w.routes[1] + "/int-2";
    double second = 0; std::string seen;
    for (const auto& i : compileDocument(w.document, test::root() / "data").scenario.inputs) {
        seen += i.id + " ";
        if (i.id.rfind(prefix, 0) == 0) second += i.vehiclesPerHour;
    }
    if (std::abs(second - 200) > 1e-9) {
        // Everything each stage saw, so a platform-only failure explains itself from the CI log.
        std::string stages = " | authored: " + documentJson(w.document)["definition"].dump();
        const auto split = withRoutingDecisions(*w.document.definition);
        stages += " | decided:";
        for (const auto& i : split.inputs) stages += " " + i.id + "->" + i.routeId + "@" + std::to_string(i.intervals.size());
        const auto resolved = resolveCatalogs(*w.document.definition, test::root() / "data");
        stages += " | resolved:";
        for (const auto& i : resolved.inputs) stages += " " + i.id + "->" + i.routeId;
        throw std::runtime_error("expected 200 under " + prefix + ", got " + std::to_string(second) +
                                 " in: " + seen + stages);
    }
}
TEST(demand, a_decision_mixing_origins_is_refused) {
    auto built = fixture::fourLegIntersection();
    History h; h.reset(built.document);
    const auto before = documentJson(h.document());
    // The forcing: route 0 leaves the West approach and route 3 the East one.
    const auto& routes = h.document().definition->routes;
    CHECK(routes[0].segmentIds.front() != routes[3].segmentIds.front());
    test::throws([&] { h.execute("mixed", [&](auto& d) {
        putRoutingDecision(d, {"", "", {{built.routes[0], 1}, {built.routes[3], 1}}}); }); },
        "ROUTING_DECISION_MIXED_ORIGIN");
    test::throws([&] { h.execute("zero", [&](auto& d) {
        putRoutingDecision(d, {"", "", {{built.routes[0], 0}}}); }); }, "INVALID_SHARE");
    test::throws([&] { h.execute("ghost", [&](auto& d) {
        VehicleInput input{"", "", "car", 100, 0, 900, {}, {}}; input.routingDecisionId = "nowhere";
        putInput(d, input); }); }, "UNKNOWN_ROUTING_DECISION");
    CHECK(documentJson(h.document()) == before);
}
TEST(demand, deleting_routes_prunes_decisions_and_then_their_inputs) {
    auto w = west();
    History h; h.reset(w.document);
    std::string decision;
    h.execute("decide", [&](auto& d) {
        decision = putRoutingDecision(d, {"", "", {{w.routes[0], 2}, {w.routes[1], 1}}});
        VehicleInput input{"", "", "car", 600, 0, 900, {}, {}}; input.routingDecisionId = decision;
        putInput(d, input);
    });
    h.execute("drop left", [&](auto& d) { deleteRoute(d, w.routes[1]); });
    CHECK(h.document().definition->routingDecisions.front().routes.size() == 1);
    CHECK(h.document().definition->inputs.size() == 1); // Still splits -- now all to route 0.
    h.execute("drop through", [&](auto& d) { deleteRoute(d, w.routes[0]); });
    CHECK(h.document().definition->routingDecisions.empty());
    CHECK(h.document().definition->inputs.empty());
    h.undo(); h.undo();
    CHECK(h.document().definition->routingDecisions.front().routes.size() == 2);
}
TEST(demand, decisions_round_trip_and_stay_absent_when_unset) {
    auto w = west();
    CHECK(!documentJson(w.document)["definition"].contains("routingDecisions"));
    const auto decision = putRoutingDecision(w.document, {"", "West turns", {{w.routes[0], 5}, {w.routes[2], 1}}});
    VehicleInput input{"", "", "car", 600, 0, 900, {}, {}}; input.routingDecisionId = decision;
    putInput(w.document, input);
    const auto saved = documentJson(w.document);
    CHECK(saved["definition"]["routingDecisions"][0]["name"] == "West turns");
    const auto reopened = parseDocument(Json::parse(saved.dump()));
    CHECK(reopened.definition->routingDecisions == w.document.definition->routingDecisions);
    CHECK(reopened.definition->inputs == w.document.definition->inputs);
    // A document naming a decision is diagnosed clean, not as an input with no route.
    CHECK(runDiagnostics(reopened, test::root() / "data").empty());
}
