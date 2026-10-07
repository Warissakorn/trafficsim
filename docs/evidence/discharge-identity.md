# M3.3.1b2b2 rank scope and source-sink identity evidence — 2026-10-07

Candidate branch: `codex/discharge-passage` (PR #120), on top of
`d82bfc69995dab85fdf01115541e2b30f32a04dd`. Local host: Linux, GCC 13.3.0,
CMake 3.28.3, Ninja 1.11.1, nlohmann/json 3.11.3; headless preset only (no Qt).
Candidate native Linux/Windows CI is separate. Contract:
[DISCHARGE](../reference/DISCHARGE.md#rank-scoped-invalidation-and-source-sink-identity-m331b2b2-d125); decision D125.

## Failure-first fixtures

`tests/discharge_identity_tests.cpp` (group `discharge`) adds five cases:

- `same_tick_source_sink_types_come_from_exact_replay`: two inputs on one 5 mm
  route (head at 4 mm), car and bus with disjoint desired speeds, 3600 veh/h each,
  real engine steps at .1 and .2 s. It first asserts the forcing worked — at least
  five vehicles departed and arrived in one tick with no survivor and no previous
  queue entry (27–28 observed) — then that every crossing's type matches the type
  implied by its event's desired speed, both types occur, complete cycles are
  available, and crossings equal arrivals inside green. Every dynamic field, RNG and
  accounting value matches an unobserved copy. With the replay line removed the
  case fails (`r.unavailable.empty()`): those cycles were `untracked_source_passage`.
- `upcoming_arrivals_replay_next_generation_without_mutation`: on the same run,
  `upcomingArrivals` leaves inputs/RNG/id counter unchanged, its count equals the
  next step's id advance, and types agree with the next queue/fleet; empty when
  nothing is due.
- `contradictory_pending_fingerprint_stays_untracked`: a matching pending record
  yields one crossing (control); a scheduled-time mismatch yields none and
  `untracked_source_passage`.
- `unqueued_lane_change_keeps_both_cycles_available` and
  `queued_lane_change_invalidates_only_while_ranks_are_open`: a moving changer never
  queued at Go leaves both heads available; a queued vehicle leaving after two
  crossings invalidates with `steadyLast=3` but not with `steadyLast=2`.

`actual_lateral_motion_counts_target_crossing_not_departed_lane` now expects both
cycles available (its changer moves at 10 m/s, never queued) and the raw crossing
retained as not queued at Go.

## Commands and results

`cmake --preset headless && cmake --build --preset headless && ctest --preset headless`:
**63/63 pass**, including architecture, file-size and documentation guards and the
frozen TS reference scenarios; `trafficsim-tests discharge` runs 35 cases.

Seed-42 CLI `--stop-lines --discharge`, parent build (d82bfc6) versus candidate:
all non-`discharge` JSON and every `inputManifest` are equal for four-leg-signalised,
t-junction-priority, lane-change-lab and m2.6-study-template. Raw crossings are
unchanged (470 and 1648). Usable cycles:

| Project | Cycles | Usable before | Usable after | `route_or_lane_change` before → after |
|---|---|---|---|---|
| four-leg-signalised | 90 | 20 | 30 | 26 → 1 |
| m2.6-study-template | 363 | 91 | 113 | 64 → 6 |

The rest move mostly to `insufficient_crossings` or `queue_not_sustained`, which
were previously masked by the earlier reason. These are observed diagnostics, not
calibrated capacity.

## Retained source hashes

| File | SHA-256 |
|---|---|
| `tests/discharge_identity_tests.cpp` | `d46a5965092ddb86f7aa600267784807668f42ead90c9c284e37f029733ed58e` |
| `src/eval/discharge_passage.cpp` | `6004c13b4b62a12e2fc7927e6ff642d9f14eeb4fb94df85b894f54a3f4691345` |
| `src/core/demand.cpp` | `5741f84f6622e32ebe06a12add9204d7b8457dd8e86146702c5122999f6b2793` |

## Limits

Overlapping lateral spans with different mapped stations stay ambiguous: the engine
chooses among them by conflict-area and safety checks the observer cannot repeat.
No M0/M6, empirical or owner gate closes.
