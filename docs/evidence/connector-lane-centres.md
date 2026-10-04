# Connector runtime lane-centre verification — 2026-10-04 (D106)

Baseline: `319b54d9cc29bb3e3ff330b88ed38388f2e1c34c` (PR #98 merged).
Inputs were registered in local commit `c74b3fb` before the seeded comparisons;
its published metadata counterpart is `addeb738`. See
[inputs](connector-lane-centres-inputs.json), [project comparisons](connector-lane-centres-projects.json)
and [T-junction sweep](connector-lane-centres-tjunction.json).
Linux, GCC 13.3.0, C++20 Release, `-fno-fast-math -ffp-contract=off`.

## What changed

Stored schema-17 geometry and authored control stations keep their meanings. Runtime path
interior vertices are the midpoints of the final adjacent painted rails. Terminal legs
join those vertices to the named Link lane centres, including a taper's receiving lane.
The compiler, route overlays and run canvas use these same paths and lengths. Waiting bars
now intersect the rails on the normal at the mapped runtime position, rather than the
first-lane authoring polyline's position. Widths share topology-only lane pairing to avoid
surface/path recursion. No core motion code or frozen baseline was changed.

The initial three lane-centre regressions all failed against the baseline and passed after
the fix. The forcing test's right-hand 90-degree, 3 → 3, 3.5 m fixture has a first-lane
interior-vertex discrepancy greater than 1 m on the old path. The corrected interior
vertices agree with the final-rail midpoints within 1e-9 m. This is an **interior** statement:
terminal attachment legs deliberately differ from the rail-midpoint line. In particular,
a zero-width taper ends on an edge while a vehicle must arrive at the receiving lane centre.

## Verification

- Headless `check` passed: architecture/file-size guards and **53/53 CTest groups**.
- `lane_centres` has seven tests: 52 count/width/handedness/side combinations, rotations,
  curved body attachments, frozen blend weights, station/bar correspondence, stable IDs,
  file round trips, Undo/Redo, exact symmetric single-lane paths and singular drafts.
- Its runtime test runs **240 scenarios**: seeds 42–81 × both driving sides × 3 → 3,
  3 → 2 and 2 → 3. Every tick checks same-build vehicle/event/input replay, accounting and
  body separation on each runtime segment. More than 1,000 actual vehicle observations
  inside the curved lane interiors check front position against the interpolated rail midpoint.
- The congested T-junction runs **120 cases per build**: 40 seeds × headways 3/7/12,
  gap time 5 s. Full trajectory digests and all report fields are identical. Existing replay,
  accounting, segment-body, swept-crossing and braking checks pass. The sweep still has
  22 minor clamps, including two stopped cases; this geometry change does not resolve those.
- Four supplied projects run for 40 seeds each (**160 before/after comparisons**).

| Project | Changed reports / 40 | Max delay difference (s) | Max queue difference (m) | Clamp totals before → after |
|---|---:|---:|---:|---:|
| Four-leg signalised | 40 | 0.000000211 | 0 | 220 → 220 |
| M2.6 study template | 40 | 0.000206837 | 0.000001477 | 865 → 865 |
| Lane-change lab | 0 | 0 | 0 | 19 → 19 |
| T-junction priority | 0 | 0 | 0 | 6 → 6 |

The study-template's maximum travel-time difference is 0.000207040 s. These are measured
geometry/station effects, not bit-identical report claims. All counts, clamps and remaining
report fields are unchanged; the JSON lists the maximum difference for each changed field.
The supplied projects do not replace the purpose-built curved multilane runtime test.

## Reproduce

Build the baseline revision and the PR with the same compiler and Release options. On each:

```sh
cmake --preset headless -DCMAKE_BUILD_TYPE=Release
cmake --build --preset headless -j2
ctest --preset headless -j2
build/headless/bin/trafficsim-t-junction-clamps --sweep sweep.jsonl .
```

Compare the two sweep files byte-for-byte. For each supplied project, run seeds 42–81 with
`trafficsim-cli --seed SEED --project FILE --data-dir data` on both builds; compare all JSON
fields and aggregate absolute differences by field. The two retained JSON evidence files
record sweep rows and per-project/per-seed counts plus the measured metric maxima.
Run `trafficsim-tests lane_centres` on the PR to execute the curved multilane vehicle checks.
This session linked the old two model objects into a baseline archive with the same core,
project and evaluation libraries; those libraries have no changes in this PR.

## Limits

This is Linux headless evidence. Qt desktop/Windows execution is delegated to repository
CI; the owner's visual check remains in NEXT. The segment-body check does not establish
2D vehicle collision avoidance across distinct paths. D80's singular, unbounded and folded
mouth authoring rules remain: following a retained drawn midpoint is not evidence that an
extreme turn is drivable. No calibration or Vissim fidelity claim is made.
