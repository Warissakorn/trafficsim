# Archived progress — continuous lane-change guidance

Moved whole from PROGRESS on 2026-10-07. Current work lives in NEXT/ROADMAP.

## 2026-10-05 — Continuous lane-change guidance (phase 3)

Owner asked to continue after phase 2 (#104). Replaced the independent D102 lateral
slide/yaw with a composed front guide and the same rear rolling solution. Accepted
changes capture their source/target stations and pre-change speed in snapshot-owned,
runtime-only `laneChangeTrace`; following, lane admission and evaluation never read it.
Station remaps reconstruct longitudinal travel. Quintic blends compose with the ongoing
guide, so overlapping changes start from its position/tangent. Stopped vehicles hold
pose; body heading continues settling after the front reaches the lane. Blend length
is `3 * max(startSpeed, 5)` m, an uncalibrated display assumption replacing a time window.

Guide polyline sampling splits at route vertices, sections and blend endpoints, then
uses arc length in the existing RK4 rear model with the source's initial rolling heading.
Polyline tangents are walked once rather than repeatedly searched for every sample.
Actual Canvas cache keys include complete trace/type and immutable Scenario/Run geometry;
seeking rebuilds older journeys and arrivals discard paths. No revision cache. Numerical
sample bounds and missing/disconnected/inconsistent-input behaviour are documented in
VEHICLE_POSE.md, including acceptance rows P3.1–P3.6. Engine occupancy still changes in
one tick; this does not add swept-body conflict clearance or a between-lanes state.

Independent analytic-guide RK4/no-slip, two rigid vehicle sizes, differing station remaps,
overlapping changes, curved join, convergence, invalid input and query-order tests pass.
Crossing reports/events (seeds 0/42/43/4294967295) and four example-project reports,
lane-change diagnostics and CSVs (seed 42) match phase-2 `de9aee3` byte-for-byte.
Linux GCC 13.3 / Qt 6.4.2 Debug: 58/58 headless and 83/83 desktop suites pass;
final guide/Canvas reruns follow the tangent-walk repair. Architecture and size guards
pass. Windows CI results belong to the PR. Owner appearance review stays open. Two old display-scale entries moved whole to the archive.
