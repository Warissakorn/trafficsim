# PROGRESS archive — measured optimization pass (D70)

Moved out of `docs/PROGRESS.md` on 2026-09-29 as the oldest live entry.

## 2026-09-26 — Measured optimization pass: tests, evaluator, routing, leader search (D70)

Behaviour-preserving; every change was checked byte for byte (M2.6 report, crossing event
streams for seeds 7/42/43, corridor trip counts) and the full suite (63 tests) passed after each.
Linux only, this container, Release unless noted.

| Item | Before | After |
|---|---|---|
| `ctest --preset desktop -j4` (Debug) | 64.1 s | 21.3 s |
| M2.6 template, one hour, `trafficsim-cli 42 --project …` | 668 ms (661–707), 5.74G instr | median 454 ms (436–661, one outlier), 4.35G instr |
| `stepSimulation` instr, corridor 96 × 300 s / 12 × 300 s / M2.6 | 4.26G / 761M / 2.99G | 3.48G / 655M / 2.88G |

1. **One ctest test per model-test group (D70).** `all-model-tests` ran the registry serially
   and repeated every named group; it is now `--check-groups`, which fails on an unlisted group.
2. **`MovementAccumulator::observe`** kept queue state in a per-tick `std::map` (31% of the M2.6
   run, 4.7M allocations an hour); it is a vector sorted by id.
3. **`routeShortestChains`** (moved from `demand_paths.cpp` into `routing.cpp`) builds the route
   object graph once per search; `routeContinuations` rebuilt it 3,300 times per Run.
4. **`closestVehicle` stops** once a part starts beyond the nearest gap: parts ascend and span
   rears are ≥ 0, so the strict tie-break keeps the same leader.

Not done, measured: editor frames are 1.3–5.5 ms at 40 intersections (no work needed);
`allocateId`/`putRoute` are O(n²) over a scripted build-up but under 1 ms per click.
