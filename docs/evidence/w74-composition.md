# M3.3.3a W74 composition evidence — 2026-10-08

Branch `claude/dazzling-gates-df64jc` on `e495e60` (after D132). Local host: Linux, GCC 13.3.0,
CMake 3.28.3, Ninja 1.11.1, Qt 6.4.2 (offscreen). `--preset headless` 69/69,
`--preset desktop` 100/100. Windows evidence belongs to the PR's native CI. Contract:
[W74](../reference/W74.md) §2, §6, §7; decision D133. **Development evidence only: no
calibration, no Vissim/SUMO equivalence; the not-yet-validated marker stays.**

## Rows (`tests/w74_run_tests.cpp`, ctest `w74run`)

The parameter set zeroes every trait-scaled coefficient, so hand values hold for any id
(at v = 4: ABX = 7, SDX = 12; bNull = 0.25). Cars: 4 m, max 3 / comfortable 2 / max 6 m/s².

| Row | Case | Asserts |
|---|---|---|
| BA28 | One tick, follower at 4 m/s, g = 10 behind a leader at 2 m/s with aL ∈ {0, 1, −1.5} | Approaching; acceleration = −2/3 + 0.5·aL (so `closestVehicle` carries aL); state {approaching, 0} |
| BA28 | Same follower 11.5 m from a red head | −16/9: a static obstacle contributes aL = 0 |
| BA24 | Route a(proto) b(w74) c(w74b) d(proto); a leader 10 m ahead at equal speed; front exactly at each join | b\|c keeps {following, −1} (−0.25); entry with no state initialises {following, +1} at a\|b and b\|c; exit at c\|d and anywhere on a clears it |
| BA25 | Prototype and w74 followers behind a standing or moving leader whose type is prototype or w74 | Equal acceleration, state and distance: the leader's model never matters |
| BA28 | 15 m/s, red head 1 m ahead | Hard cap at the head, `safety-clamp` counted |
| BA28 | At rest, gap 2 and 1 (≤ ax) to a leader pulling away at 2 m/s² | No positive acceleration (D105) |
| BA28 | w74 input behind a vehicle within ax of the entry, 10 ticks | No departure (source rule with `standstillGap`) |
| BA28 | Stop line: arriving at 10 m/s; and at rest 3.3 m short (following band at v = 0, beyond `stopLineReach` 2.89) after braking | Both served (≥ 2 ticks at the line) and cross; the second only because of the standstill `s = +1` override |
| BA23 | 60 s red then green, 900 veh/h, 400 m; and an open 2 km platoon, 600 veh/h, 600 s | Unbounded following accelerations are exactly ±bNull; a vehicle at rest within ABX(0.1) of the one ahead stays at rest; the open platoon never clamps |
| Replay | State copied at tick 400 and both stepped to the end; two full runs | Equal vehicles (state, traits) and events |

`w74codec`'s run case now asserts a `w74` behaviour used by a vehicle type or a road runs
and carries `w74State`; an unused one still leaves vehicles equal but for traits.

Seeded mutations each fail a case: `closestVehicle` or `OccupiedSpan` dropping the leader's
acceleration, publish not storing the state, motion ignoring the previous state,
`standstillGap` or `desiredGap` reading prototype fields for w74, and removing the
standstill override (the run-level Stop case; first written without the 3.3 m case, which
the mutation showed was missing).

## Byte guards

`trafficsim-cli 42` byte-identical to `48a3276`; frozen TS baselines, every prototype suite
and the shipped projects unchanged.

## Not shown

The courtesy/cooperative second obstacle's tie with a w74 follower has no fixture; the code
keeps the existing strict `<`, so the vehicle ahead and its state are kept. BA27 (dt
0.1/0.25/0.5 discharge sensitivity, seeds 42–81) is the next record. No preset; no
parameter editing UI; queue discharge behaviour is the model's (§4 `dv²` term), not tuned.
