# M3.3.3a W74 pure-function evidence — 2026-10-08

Branch `claude/dazzling-gates-df64jc` on `main` `48a3276` (after PR #125). Local host:
Linux, GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, nlohmann/json 3.11.3, `--preset headless`
(`-fno-fast-math -ffp-contract=off`). No Qt in this container: no desktop or UI suite ran;
Windows evidence belongs to the PR's native CI. Contract: [W74](../reference/W74.md) §3, §4,
§6 bounds, §7 sign; decision D130.

## What exists

`src/core/w74.hpp`: `W74Parameters` (18 required keys, no defaults), `W74Traits`,
`W74Regime`, `W74State`, `w74Thresholds` and `w74Acceleration`. Nothing outside
`tests/w74_tests.cpp` calls them; no behaviour can select `w74`; `Leader::acceleration`
exists and stays 0 at every runtime site until BA28.

## Rows (`tests/w74_tests.cpp`, ctest `w74`)

Parameters are chosen so `v = 4` gives `BX = 5`, `ABX = 7`, `SDX = 12`, and at `g = 10`
`SDV = 0.25`, `CLDV = 1`, `OPDV = −0.5` — all exact in binary.

| Row | Case | Asserts |
|---|---|---|
| BA21 | §3 thresholds at `v = 4`, `g = 10`; `DMAX` from `v²/2b`; `SDV = 0` at `g ≤ AX`; `vBx` at rest | Exact equality with the hand values; `BX > 0` at `v = 0` |
| BA21 | Rows 1–7 of §4 | Regime, `FollowingMode` and hand-computed acceleration (emergency −0.2, approaching −2/3−1/2 with `aL`, following −`bNull`, free ramp 0.5, far approaching −2/9, free `bMax`) |
| BA21 | Each equality side: `g = ABX`, `g = SDX`, `dv = CLDV`, `dv = OPDV`, `dv = SDV`, `g = DMAX` | On the value the contract's side; one ULP away (`nextafter`) the other regime |
| BA21 | §7 sign: entry with `dv = 0`/`dv > 0`, after free/approaching/emergency/following ±1, standstill override, 0 outside following | Stored state as specified |
| BA22 | Emergency inside `AX`; approaching below comfortable; free `bMax` above `maxAcceleration`; `v ≥ vDes`; D105 at rest with an obstacle pulling away | Exactly `−maxDeceleration` (6, and 7.5 for a changed type), `−comfortableDeceleration`, `maxAcceleration`, 0, never positive |

Seeded mutations each fail the group (4 passed, 1 failed): `g ≤ ABX` → `<`, `dv > CLDV` →
`≥`, `dv > OPDV` → `≥`, row 6 `>`/`<` → `≥`/`≤`, and removing the standstill override.

## BA18 guard

Headless `ctest --preset headless`: 66/66 (65 before plus `w74`). `trafficsim-cli 42`
output is byte-identical (`cmp`) to the build at `48a3276`. The frozen TS baselines in
`reference` pass unchanged.

## Not shown

No run, schema, traits hash, state publish or model switch (BA23–BA29). No preset. No
calibration or Vissim/SUMO equivalence; the not-yet-validated marker stays.
