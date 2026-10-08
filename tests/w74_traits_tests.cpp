#include "test.hpp"
#include "../src/core/w74.hpp"
#include <algorithm>
using namespace trafficsim;
// M3.3.3a (D132): row BA26 of docs/plans/DRIVING_BEHAVIOUR.md -- the hashed driver traits of
// docs/reference/W74.md §5. Nothing reads them yet; the composition slice (BA28) will.
namespace {
const W74Parameters kP{2, 2, 1, 2, .5, 16, 4, .5, .5, 20, 1, 1, 3, .25, .5, -1, .5, .25};
// test::straight with an unused w74 behaviour: the run is the prototype's, vehicles carry traits.
Scenario mixed() {
    auto s = test::straight();
    DriverBehaviour w74{"w74"}; w74.w74 = kP; s.behaviours.push_back(w74);
    return s;
}
// Everything but the traits, so a mixed run can be compared with the prototype-only one.
template<class V> std::vector<V> stripped(std::vector<V> list) {
    for (auto& v : list) v.w74Traits.reset();
    return list;
}
}

TEST(w74traits, the_hash_matches_an_independent_implementation_bit_for_bit) { // BA26
    // Golden values from a separate Python implementation of §5 (docs/evidence/w74-traits.md).
    struct Case { std::uint32_t seed; std::uint64_t id; W74Traits expected; };
    const Case cases[] = {
        {42, 1, {0x1.0000000000000p-1, 0x1.713d41f672900p-7, 0x1.0732c71a88a40p-1, 0x1.6a008383d7572p-2, 0x1.6dfa0f552f5c4p-3}},
        {0, 7, {0x1.3333333333333p-2, 0x1.6414d5f0fa298p-3, 0x1.e29e59f004107p-1, 0x1.3ac1386f8586dp-1, 0x1.d5213525c2277p-1}},
        {0xffffffffU, 1ULL << 40, {0x1.0000000000000p+0, 0x1.720a8f641f508p-2, 0x1.9a15efd9987f0p-2, 0x1.24199bbfca707p-1, 0x1.54b63d1dc2237p-1}}};
    for (const auto& c : cases) CHECK(w74Traits(c.seed, c.id, c.expected.zBx) == c.expected);
    // Seed 0 is hashed as given, not remapped like the run stream's state.
    CHECK(w74Traits(0, 7, 0.3) != w74Traits(0x6d2b79f5U, 7, 0.3));
    for (std::uint64_t id = 1; id <= 2000; ++id) {
        const auto t = w74Traits(42, id, 0.5);
        for (const double u : {t.zEx, t.zCx, t.zOsc}) CHECK(u >= 0 && u < 1);
        CHECK(t.zOp >= 0 && t.zOp <= 1); CHECK(t.zBx == 0.5);
        CHECK(t == w74Traits(42, id, 0.5));
    }
    CHECK(w74Traits(42, 1, 0.5) != w74Traits(42, 2, 0.5)); CHECK(w74Traits(42, 1, 0.5) != w74Traits(43, 1, 0.5));
}

TEST(w74traits, a_mixed_scenario_keeps_the_prototype_draws_and_every_vehicle_its_traits) { // BA26
    auto plain = createSimulation(test::straight(), 42), w74 = createSimulation(mixed(), 42);
    std::size_t seen = 0;
    while (plain.tick < totalTicks(*plain.scenario)) {
        // The traits the next step will create are the ones upcomingArrivals predicts.
        const auto upcoming = upcomingArrivals(w74);
        plain = stepSimulation(plain); w74 = stepSimulation(w74);
        CHECK(plain.randomState == w74.randomState); CHECK(plain.nextVehicleId == w74.nextVehicleId);
        CHECK(plain.events == w74.events);
        CHECK(stripped(plain.vehicles) == stripped(w74.vehicles));
        for (std::size_t i = 0; i < plain.inputs.size(); ++i) CHECK(stripped(plain.inputs[i].queue) == stripped(w74.inputs[i].queue));
        for (const auto& v : plain.vehicles) CHECK(!v.w74Traits);
        for (const auto& v : w74.vehicles) {
            CHECK(v.w74Traits == w74Traits(42, v.id, v.driverFactor)); ++seen;
        }
        for (const auto& p : upcoming) {
            CHECK(p.w74Traits == w74Traits(42, p.id, p.driverFactor));
            const auto made = [&](const auto& list) {
                return std::any_of(list.begin(), list.end(), [&](const auto& v) { return v.id == p.id && v.w74Traits == p.w74Traits; });
            };
            bool found = made(w74.vehicles);
            for (const auto& input : w74.inputs) found = found || made(input.queue);
            CHECK(found);
        }
    }
    CHECK(seen > 0);
}

TEST(w74traits, traits_survive_a_copied_state_and_replay_exactly) { // BA26
    auto state = createSimulation(mixed(), 7);
    for (int i = 0; i < 300; ++i) state = stepSimulation(state);
    const auto copy = state;
    auto a = state, b = copy;
    for (int i = 0; i < 300; ++i) { a = stepSimulation(a); b = stepSimulation(b); }
    CHECK(a.vehicles == b.vehicles);
    CHECK(test::finish(createSimulation(mixed(), 7)).vehicles == test::finish(createSimulation(mixed(), 7)).vehicles);
}

TEST(w74traits, checkpoints_carry_traits_only_when_present) { // §9
    auto plain = createSimulation(test::straight(), 42), w74 = createSimulation(mixed(), 42);
    while (plain.vehicles.empty() && plain.tick < totalTicks(*plain.scenario)) {
        plain = stepSimulation(plain); w74 = stepSimulation(w74);
    }
    CHECK(!plain.vehicles.empty()); CHECK(!w74.vehicles.empty());
    const auto p = checkpointJson(plain), w = checkpointJson(w74);
    CHECK(p.dump().find("w74Traits") == std::string::npos);
    CHECK(w.dump().find("w74Traits") != std::string::npos);
    const auto t = w74.vehicles.front().w74Traits;
    CHECK(w["vehicles"][0]["w74Traits"]["zOp"] == t->zOp);
}
