#include "test.hpp"
#include "../src/core/w74.hpp"
#include <cmath>
#include <limits>
using namespace trafficsim;
// M3.3.3a (D129/D135): rows BA21-BA22 of docs/plans/DRIVING_BEHAVIOUR.md against the pure
// w74Acceleration of docs/reference/W74.md. Expected values are computed by hand in the comments;
// the parameters are chosen so most of them are exact in binary.
namespace {
// zBx = 0.5 -> BX = 2.5 sqrt(v). At v = 4: BX = 5, ABX = 7, EX = 2, SDX = 12, CX = 16,
// bNull = 0.25 + 0.5 * 0.5 = 0.5, bMax = 1 + 1 * (3 - 2) = 2.
const W74Parameters kP{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16,
    .cxMult = 0, .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0.5, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
const W74Traits kZ{.zBx = 0.5, .zEx = 0.5, .zCx = 0.5, .zOp = 0.5, .zOsc = 0.5};
VehicleType car() {
    VehicleType t; t.id = "car"; t.length = 4.5;
    t.maxAcceleration = 3; t.comfortableDeceleration = 2; t.maxDeceleration = 6;
    return t;
}
W74Result at(double gap, double leaderSpeed, std::optional<W74State> previous = {},
             double speed = 4, double leaderAcceleration = 0, const VehicleType& type = car(),
             const W74Parameters& p = kP) {
    return w74Acceleration(speed, 15, type, p, kZ, previous, Leader{gap, leaderSpeed, leaderAcceleration});
}
double up(double x) { return std::nextafter(x, std::numeric_limits<double>::infinity()); }
double down(double x) { return std::nextafter(x, -std::numeric_limits<double>::infinity()); }
}

TEST(w74, thresholds_match_the_hand_computed_values) { // BA21, contract §3
    const auto t = w74Thresholds(4, 10, car(), kP, kZ);
    CHECK(t.bx == 5); CHECK(t.abx == 7); CHECK(t.ex == 2); CHECK(t.sdx == 12); CHECK(t.cx == 16);
    CHECK(t.sdv == 0.25); CHECK(t.cldv == 1); CHECK(t.opdv == -0.5);   // ((10-2)/16)^2, x EX^2, x -0.5
    CHECK(t.dmax == 20); CHECK(t.bNull == 0.5); CHECK(t.bMax == 2);
    CHECK(w74Thresholds(10, 30, car(), kP, kZ).dmax == 25);            // v^2 / 2b beats dMax
    CHECK(w74Thresholds(4, 2, car(), kP, kZ).sdv == 0);                // g <= AX
    const auto rest = w74Thresholds(0, 3, car(), kP, kZ);
    CHECK(rest.bx > 0); test::near(rest.bx, 2.5 * std::sqrt(0.1), 1e-15); // vBx = max(v, 0.1)
}

TEST(w74, every_regime_row_gives_its_hand_computed_acceleration) { // BA21, contract §4
    // Row 1, no obstacle: free at bMax.
    auto r = w74Acceleration(4, 15, car(), kP, kZ, {});
    CHECK(r.state.regime == W74Regime::free); CHECK(r.acceleration == 2); CHECK(r.mode == FollowingMode::free);
    // Row 2, AX < g <= ABX: 0.5*1/(2-4.5) + 0.25*2 + (-1)(7-4.5)/5 = -0.2 + 0.5 - 0.5.
    r = at(4.5, 3, {}, 4, 2);
    CHECK(r.state.regime == W74Regime::emergency); CHECK(r.mode == FollowingMode::braking);
    test::near(r.acceleration, -0.2 + 0.5 - 0.5, 1e-15);
    // Row 3, g < SDX and dv = 2 > CLDV = 1: 0.5*4/(7-10) + 0.5*(-1) = -2/3 - 1/2.
    r = at(10, 2, {}, 4, -1);
    CHECK(r.state.regime == W74Regime::approaching); CHECK(r.mode == FollowingMode::approaching);
    test::near(r.acceleration, -2.0 / 3 - 0.5, 1e-15);
    // Row 4, OPDV < dv = 0.5 <= CLDV: following; entering with dv > 0 gives s = -1.
    r = at(10, 3.5);
    CHECK(r.state.regime == W74Regime::following); CHECK(r.mode == FollowingMode::following);
    CHECK(r.acceleration == -0.5); CHECK(r.state.sign == -1);
    // Row 5, g < SDX and dv = -1 < OPDV: free, inside 2 ABX: min(bNull, 2 * 3/7) = 0.5.
    r = at(10, 5);
    CHECK(r.state.regime == W74Regime::free); CHECK(r.acceleration == 0.5); CHECK(r.state.sign == 0);
    // Row 6, g = 16 >= SDX, dv = 2 > SDV = 0.765625, g < DMAX: 0.5*4/(7-16) = -2/9.
    r = at(16, 2);
    CHECK(r.state.regime == W74Regime::approaching); test::near(r.acceleration, -2.0 / 9, 1e-15);
    // Row 7, g = 30 beyond DMAX: free outside 2 ABX, bMax.
    r = at(30, 2);
    CHECK(r.state.regime == W74Regime::free); CHECK(r.acceleration == 2);
}

TEST(w74, every_equality_side_is_the_contracts) { // BA21, contract §4 "Equality sides"
    CHECK(at(7, 4).state.regime == W74Regime::emergency);                // g = ABX
    CHECK(at(up(7), 4).state.regime == W74Regime::following);
    CHECK(at(12, 3.5).state.regime == W74Regime::approaching);           // g = SDX: out of the band
    CHECK(at(down(12), 3.5).state.regime == W74Regime::following);
    CHECK(at(10, 3).state.regime == W74Regime::following);               // dv = CLDV = 1
    CHECK(at(10, down(3)).state.regime == W74Regime::approaching);
    CHECK(at(10, 4.5).state.regime == W74Regime::free);                  // dv = OPDV = -0.5
    CHECK(at(10, down(4.5)).state.regime == W74Regime::following);
    CHECK(w74Thresholds(4, 16, car(), kP, kZ).sdv == 0.765625);
    CHECK(at(16, 4 - 0.765625).state.regime == W74Regime::free);         // dv = SDV
    CHECK(at(16, down(4 - 0.765625)).state.regime == W74Regime::approaching);
    CHECK(at(20, 2).state.regime == W74Regime::free);                    // g = DMAX
    CHECK(at(down(20), 2).state.regime == W74Regime::approaching);
}

TEST(w74, following_sign_follows_the_previous_tick_and_standstill) { // BA21, contract §7
    const auto following = [](std::optional<W74State> previous, double leaderSpeed = 3.5) {
        return at(10, leaderSpeed, previous);
    };
    CHECK(following({}, 4).state.sign == 1);                              // entering, dv = 0
    CHECK(following({}, 3.5).state.sign == -1);                           // entering, dv > 0
    CHECK(following(W74State{W74Regime::free, 0}).acceleration == 0.5);
    CHECK(following(W74State{W74Regime::approaching, 0}).state.sign == -1);
    CHECK(following(W74State{W74Regime::emergency, 0}).state.sign == -1);
    CHECK(following(W74State{W74Regime::following, 1}).state.sign == 1);
    CHECK(following(W74State{W74Regime::following, -1}).state.sign == -1);
    // Standstill overrides: at rest behind a static obstacle in the band, s = +1 so it creeps.
    const auto rest = at(3, 0, W74State{W74Regime::approaching, 0}, 0);
    CHECK(rest.state.regime == W74Regime::following);
    CHECK(rest.state.sign == 1); CHECK(rest.acceleration == 0.5);
    // Outside following the stored sign is 0.
    CHECK(at(10, 2, W74State{W74Regime::following, -1}).state.sign == 0);
    CHECK(at(4.5, 3, W74State{W74Regime::following, 1}).state.sign == 0);
}

TEST(w74, type_limits_bound_the_result_exactly) { // BA22, contract §4/§6
    CHECK(at(1.5, 4).acceleration == -6);                                 // emergency inside AX
    auto heavy = car(); heavy.maxDeceleration = 7.5;
    CHECK(at(1.5, 4, {}, 4, 0, heavy).acceleration == -7.5);              // the type's, not a key
    CHECK(at(2.5, 0).acceleration == -6);                                 // 0.5*16/(2-2.5) - ... clamped
    const auto approaching = at(7.5, 0);                                  // 0.5*16/(7-7.5) = -16
    CHECK(approaching.state.regime == W74Regime::approaching);
    CHECK(approaching.acceleration == -2);                                // floor -comfortableDeceleration
    auto strong = kP; strong.bMaxAdd = 10;                                // bMax = 11 > maxAcceleration
    CHECK(w74Acceleration(4, 15, car(), strong, kZ, {}).acceleration == 3);
    CHECK(w74Acceleration(4, 4, car(), kP, kZ, {}).acceleration == 0);    // v >= vDes
    CHECK(w74Acceleration(5, 4, car(), kP, kZ, {}).acceleration == 0);
    // D105: a stopped vehicle within AX of an obstacle pulling away gets no positive acceleration.
    for (const double gap : {2.0, 1.0, 0.0}) CHECK(at(gap, 10, {}, 0, 3).acceleration <= 0);
}
