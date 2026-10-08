# M3.3.3a: where the extra W74 safety clamps come from — 2026-10-08

BA27 ([record](w74-discharge.md)) found W74 runs of the four-leg project clamp more than
prototype runs (9.4 per run at dt 0.1, 14.8 at dt 0.5, against ≈5.5). NEXT asked for the
clamps to be traced before any `w74` preset, in case one is an engine defect. This is that
diagnosis. **No engine change; nothing tuned; not validated.** The fixture's parameter values
are the same uncited, uncalibrated ones as BA27's.

## Inputs

- Commit `99fa529`: `tools/w74_clamp_trace.cpp` (target `trafficsim-w74-clamp-trace`, not in
  `check`). It builds its documents from `tools/w74_fixture_document.hpp`, shared with the
  BA27 sweep; after that refactor the sweep's per-cycle output is byte-identical to BA27's
  (SHA-256 `14c4146d…a2ba7e`).
- GCC 13.3.0, Linux, CMake Release `-O3 -DNDEBUG -std=c++20 -fno-fast-math -ffp-contract=off`.
  The clamp lines are identical from a Debug build.
- Command: `trafficsim-w74-clamp-trace docs/evidence/w74-discharge-behaviour.json
  data/projects/four-leg-signalised.traffic.json data <out.jsonl>`. Inputs and their
  SHA-256 are BA27's ([table](w74-discharge.md#inputs)). Same runs: prototype and `w74` ×
  dt 0.1 / 0.25 / 0.5 × seeds 42–81.
- Output: 22,933 lines (2,053 clamps, 20,880 amber onsets), SHA-256
  `4006334f872a2c4b3b613b183a430a5d35ca681d3eaf6e109ac3500b526a4388`; a rerun is
  byte-identical. Not committed (5 MB; the tool regenerates it). The clamp lines are
  flattened to [w74-clamps.csv](w74-clamps.csv).

## Method

For each `SafetyClampEvent` the tool rebuilds the tick's phase-1 snapshot (survivors plus that
tick's insertions at rest) and recomputes every obstacle that can cap a move, with core's
public functions: the vehicle leader (as `closestVehicle` picks it), non-green heads, the dead
end, a waiting courtesy hold, priority rules and zone holds (with Stop service). The clamp is
attributed to the obstacle with the smallest allowance, **only if** the proposed `follow`
move exceeds it and the published move equals it to 1e-9. Anything else would be labelled
`phase-2` or `unexplained`. For each head turning amber it also records, within 150 m
upstream: vehicles, those below 2 m/s ("queued"), and those whose one tick at their current
speed plus stopping distance at `type.maxDeceleration` passes the line ("cannot stop"; an
approximation of the clamp's precondition).

**Checks:** every run's clamp count equals BA27's `runClamps` (240/240). **No clamp is
`phase-2`, `unexplained` or on a lane-change tick.** Every clamp is a moving vehicle.

## Result

| Clamps, 40 runs each | Amber head | Vehicle leader | Red head | Dead end |
|---|---|---|---|---|
| Prototype dt 0.1 / 0.25 / 0.5 | 212 / 203 / 204 | 8 / 11 / 32 | 0 / 0 / 0 | 0 / 0 / 1 |
| W74 dt 0.1 / 0.25 / 0.5 | 357 / 390 / 490 | 18 / 27 / 98 | 0 / 0 / 2 | 0 / 0 / 0 |

### Amber heads (83–96 % of all clamps, both models)

The tick holds a vehicle at any non-green head exactly as at red. No commitment test exists
for amber, unlike priority rules (`committed`). A vehicle that cannot stop before the line
when the head turns amber therefore brakes at `maxDeceleration`, reaches the line still moving
and is clamped there. It is never more than one clamp per amber onset per head: the clamped
vehicle stops at the line and those behind it can stop.

| Amber onsets, 3,480 per arm | Clamped | "Cannot stop" | Both | Vehicles upstream (mean) | Queued (mean) | Onsets with a queue |
|---|---|---|---|---|---|---|
| Prototype dt 0.1 / 0.25 / 0.5 | 212 / 203 / 204 | 221 / 246 / 320 | 208 / 202 / 204 | 0.65 / 0.62 / 0.66 | 0.00 / 0.00 / 0.00 | 1 / 3 / 2 |
| W74 dt 0.1 / 0.25 / 0.5 | 357 / 390 / 490 | 374 / 447 / 658 | 345 / 365 / 474 | 1.84 / 1.88 / 2.14 | 0.71 / 0.74 / 0.97 | 545 / 569 / 667 |

With the fixture values, W74 discharges more slowly (BA27: mean headway ≈3.1 s against 2.0 s),
so at the end of green **16–19 % of W74 onsets still have a queue** within 150 m (prototype:
under 0.1 %), and about three times as many vehicles are upstream. More vehicles near the line
at amber means more onsets with one that cannot stop. The excess W74 amber clamps follow from
the slower discharge, through the shipped amber rule. They do not come from a W74 regime or
state path. The leader of a clamped W74 vehicle is closer (median 44 m against 70–85 m), as a
still-discharging platoon would give.

### Vehicle leader (4–17 %)

- **Prototype:** the leader is **standing** at dt 0.1 and 0.25 (9 of 32 moving at dt 0.5),
  typically the vehicle just clamped at an amber line. The follower,
  braking hard, ends inside the standstill buffer. Every one has an amber/red head on its
  route.
- **W74:** the leader is **moving** (mostly slowing) in 17/18, 25/27 and 87/98 cases. The follower
  is just past `AX` (median `g − AX` 0.34 / 0.64 / 1.88 m) in W74's emergency regime. That
  regime proposes only about −2 m/s² (median −1.9 / −2.1 / −2.3), because §4's
  emergency term aims at speed equality at `AX`
  (`0.5 · dv² / (AX − g) + emergencyLeaderWeight · aL + bMinAdd · (ABX − g) / BX`) and the
  leader is still moving. The tick's hard cap, `allowedDistance = gap − standstillGap`,
  treats every leader as standing. So the proposed move overruns the cap by centimetres and
  is clamped, although the leader also moves on during that tick. This is the conservative-cap property D105
  already recorded for the prototype's moving case (M3.2.8a.1, anticipation contract). W74
  hits it more often, because its emergency regime brakes gently near `AX`, and more still
  at dt 0.5, where one tick covers more of the deficit.

### Red heads and dead end

Three clamps in all (W74 dt 0.5: 2 red; prototype dt 0.5: 1 dead end): a vehicle that first
sees the obstacle within its one-tick reach at the coarse step.

## Verdict

**No W74 engine defect found; no failure-first row is written.** Every clamp is one of the
shipped tick's two conservative rules acting on what W74 does with these fixture values:

1. **Amber treated as red with no commitment test** (both models). W74's excess here comes
   from its slower fixture discharge.
2. **The standstill cap treating a moving leader as standing** (both models; already D105's
   recorded moving case). W74's excess here comes from its gentler emergency braking near
   `AX`.

Neither is tuned or changed here. Each would be its own contract (an amber commitment rule;
an anticipation-based cap, M3.2.8a.1) with failure-first rows, and each would change
prototype trajectories, which BA18 forbids without an owner decision. **For a later cited
`w74` preset:** rerun this trace with its values and report the clamp counts beside its
discharge figures; the counts here belong to the fixture values only.

## What this does not support

No statement about real driver behaviour at amber, W74's fidelity, or which clamp rate is
right. The "cannot stop" count approximates the clamp condition and is not exact either way:
it flags onsets where no clamp follows (the vehicle's actual braking stops it), and 4–25
clamped onsets per arm are not flagged. The causal reading of the W74 amber excess (slower
discharge → more vehicles near the line at amber) rests on these counts, not on a controlled
experiment. The not-yet-validated marker stays.
