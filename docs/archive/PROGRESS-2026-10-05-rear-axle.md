# Archived progress — rear-axle display reference

Moved whole from PROGRESS on 2026-10-07. Current work lives in NEXT/ROADMAP.

## 2026-10-05 — Rear-axle display reference (phase 2)

Owner authorized the next turning slice after phase 1 merged (#103). The route still
prescribes the traffic front bumper; its scalar station/length contract is unchanged.
`RearAxlePath` solves the rear no-slip equation using the bumper-to-rear lever, with
rigid axle/body offsets. Canvas items now originate at the rear axle, with the nose
kept at its station, including low zoom. This is a bumper-guided approximation, not
front-wheel tracking, swept-body collision clearance or measured Vissim fidelity.

Optional complete type `axles` data is parsed, validated and retained on save. Missing
old-file data stays omitted and resolves 60/20/20 percent proportions. The shipped car
and rigid heavy dimensions are explicit modelling assumptions. Catalog/inline type
extension is additive; network schema remains 17. No engine motion equations changed.

Heading is solved on fixed spatial steps, split at joins/vertices and read without
vehicle history. One derived track per route/type is retained against immutable Scenario
ownership and Run-network replacement, never a document revision (D28). Memory is
bounded; disconnected/missing paths draw none. Existing lane-change slide/yaw remains
an overlay outside the no-slip equation. The full contract is in VEHICLE_POSE.md.

Original type catalog bytes are preserved beside the old sweep evidence. Its guard
permits only added display axles, rejecting changed traffic fields; no fixture/result
is regenerated. Linux GCC 13.3 / Qt 6.4.2 Debug: all 82 desktop suites pass across the
full run and four repair reruns; architecture/size guards pass. A chord substitution
fails the analytic-turn regression. Crossing logs/reports for four seeds and seed-42
reports/CSVs for four projects are byte-identical to main `5d823be`. The Run-view
benchmark completes 3000 M2.6 Steps in both versions; concurrent build/test load means
no timing claim. Headless and Windows CI results belong to the PR.
Owner appearance and subsequent engine slices remain in NEXT.
