# Direct Connector equation — D107, 2026-10-04

Owner clarification: use the equation already in the program without a PolyPoint driving
path. This supersedes D106's rail-midpoint runtime path, not its display guide or D80 mouths.

## Equation and metre stations

For each mapped lane pair, `connectorEquation` uses the same four controls previously used
by `connectorCurve`: lane-centre attachments P0/P3, directed Link tangents, and the existing
capped control reach for P1/P2. There is one cubic, with no spline fitted through drawing points:

B(t) = (1−t)³ P0 + 3(1−t)²t P1 + 3(1−t)t² P2 + t³ P3, for 0 ≤ t ≤ 1.

Length is s(t) = integral from 0 to t of |B'(u)| du. Eight-point Gauss-Legendre integration
is adaptively subdivided to depth 12, with a 1e-12 × max(1, integral) convergence threshold.
Seventeen scalar cumulative arc stations cache sixteen integration intervals. These are
integral values, not geometry points or straight segments. Bracketed Newton/bisection
inverts s to t with at most 48 iterations and 1e-11 × max(1, length) metre residual tolerance.
Positions and derivatives then evaluate the cubic directly. Closest-point placement tests
all stationary-distance polynomial roots in [0,1], plus both endpoints.

Core motion code remains unchanged: the compiler supplies lengths and metre stations; the
canvas supplies physical positions from the equation. Vehicle fronts/rears, signal markers,
head placement/dragging and derived yield positions use that equation. Route overlays still
sample points to draw, without supplying a sampled runtime path.

## Authoring compatibility and limitations

- Schema 17, IDs, lane pairing, persisted drawing geometry and station numbers remain readable.
- Drawing count (0–40), interior drags and Connector lane widths do not set the runtime curve.
  Link attachments, Link tangents and changed lane pairing do. The bilingual Inspector
  tooltip describes this distinction. A zero-point drawing can be straight while cars turn.
- Stored conflict/priority/queue controls use an explicit adapter: authored leg index plus
  fractional distance within that leg maps to uniform t, then to s(t). Inverse mapping writes
  derived controls. Keeping stored values does not promise their former physical positions.
  Editing drawing points can change this control mapping even while the equation stays fixed.
- Signal heads store metre positions on their runtime lane, so they evaluate directly at s.
- Waiting bars use the equation normal and intersect the painted rails. Conflict coverage
  still measures convex painted lane quads, then maps stations through the adapter. It is
  not an analytic curved swept-surface solver. No cross-path 2D collision or road containment
  claim follows from the engine's segment/interval safety checks.
- Manually deformed paint, tapered lanes and singular/folded extreme mouths can differ from
  the driving equation. The owner desktop exercise remains in NEXT. No Vissim calibration
  or M0/M3 acceptance gate is closed by this change.

The T-junction generated example is refreshed by its builder: only two merge extents and
matching waiting-line stations change. This is an example input, not a frozen trajectory
baseline. Both project comparison builds read the same refreshed file.

## Registered inputs and verification

Baseline is PR 99 D106 commit `2a107957d78ac57795bc9bce66ee962bc6882b41`; local equivalent
`7874cd9b9b931342e8d94067c530f392e6124e2f` has the same tree. Input metadata was recorded in
local `6318fe2` and its refreshed example hash in `66282d7`, before observing seeded outputs.
[Input metadata](connector-equation-inputs.json) records hashes, seeds and numerical settings.
GCC 13.3, C++20 Release, `-fno-fast-math -ffp-contract=off`, same machine/toolchain for both builds.

Linux headless `check` passes architecture and 500-line guards plus **54/54 CTest groups**.
Frozen reference baselines remain unchanged. Five `equation` tests independently check the
analytic parabola B(t)=(t,t²), its known arc length, inversion, endpoint/loop closest points,
exact runtime independence for 0/1/3/19/40 drawing points and control/head correspondence.
Seven updated lane-centre tests retain display checks and run 240 seeded curve/add/drop cases
with replay, accounting, segment-body separation and actual equation positions. Signal
position helpers and the Connector head-click UI regression now use equation stations.
A Qt canvas regression forces a one-chord drawing and checks vehicle front/body heading at
three interior curve stations. The old D106 desktop CI failed its leading-resize assertion
against display vertices; the updated regression checks surviving lane references, equation
length and physical positions. Qt is unavailable locally, so execution belongs to CI.

### Four supplied projects × 40 seeds

Both CLIs read identical files and seeds 42–81; JSON comparison checks field structure and
records maximum absolute numeric differences by field. Completed/active/pending and movement
counts remain unchanged. Every project's 40 reports changes; they are not numerically
interchangeable with D106. Full rows: [project evidence](connector-equation-projects.json).

| Project | Total clamps before → after | Max mean-delay difference (s) | Max mean-travel difference (s) | Max queue-length difference (m) |
|---|---:|---:|---:|---:|
| four-leg-signalised | 220 → 220 | 3.444866 | 3.460000 | 0.000020 |
| lane-change-lab | 19 → 19 | 0.005018 | 0.006349 | 0 |
| m2.6-study-template | 865 → 858 | 0.201889 | 0.218966 | 0.000245 |
| t-junction-priority | 5 → 4 | 5.032910 | 5.015789 | 27.777411 |

Values are maxima across all report rows/seeds, not aggregate improvements. Changed path
lengths/control stations alter arrivals and admissions; queue maxima can move materially.
These differences cannot be presented as calibration or capacity improvement.

### T-junction stress: 120 cases per build

Seeds 42–81 × headways 3/7/12, gap time 5. Both sweep executables exit successfully after
full replay, per-tick accounting, segment-body, swept-conflict-interval and braking checks.
All 120 trajectory digests differ. Total safety clamps **225 → 226**, minor clamps **22 → 24**.
The increase is measured and retained; no safety parameter was tuned to hide it. Full paired
rows and source-output hashes: [stress evidence](connector-equation-tjunction.json).

This headless result neither replaces Windows/Qt CI nor establishes physical collision
avoidance between different lane curves. Existing M0/M3 owner gates remain open.

## Reproduce

Build baseline and final in separate checkouts with the same Release preset/toolchain:

```sh
cmake --preset headless -DCMAKE_BUILD_TYPE=Release
cmake --build --preset headless --target check trafficsim-cli trafficsim-t-junction-clamps
```

For each supplied project and seed 42–81, run both binaries from the final checkout, so both
read the identical hashed project/catalog inputs:

```sh
BASELINE_BIN/trafficsim-cli --seed 42 --project data/projects/t-junction-priority.traffic.json --data-dir data
FINAL_BIN/trafficsim-cli --seed 42 --project data/projects/t-junction-priority.traffic.json --data-dir data
BASELINE_BIN/trafficsim-t-junction-clamps data --sweep
FINAL_BIN/trafficsim-t-junction-clamps data --sweep
```

Keep each sweep's 120 JSONL rows and pair by seed/headway. Compare complete CLI report JSON
recursively; preserve missing/null field changes as well as counts and numeric maxima.
Use a writable TMPDIR if the execution environment has no /tmp. The example builder is
`trafficsim-t-junction-fixture data/projects/t-junction-priority.traffic.json`.
