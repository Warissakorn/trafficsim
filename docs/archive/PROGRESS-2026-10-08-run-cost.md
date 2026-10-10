# Archived progress — Engine run cost: observe and publish

Moved whole from PROGRESS on 2026-10-10. Current work lives in NEXT/ROADMAP.

## 2026-10-08 — Engine run cost: observe and publish

Measured on Windows (MSVC 14.51 Release, P-cores pinned) with temporary probes on the M2.6
one-hour CLI run: compile 116 / step 332 / observe 155 ms. Inside observe the per-counter-line
walk was 115 ms: each of 12 lines built and sorted a fresh vector every tick. It now reuses one
buffer, leaves out fronts past the line (the walk skipped them) and returns 0 without sorting
when no candidate is queued. Queue hysteresis uses one forward walk over two id-ordered lists
(an unordered hand-built fleet still searches). Publish copied each `Vehicle` with its lists;
it now moves it, finding the pending decision first and locating before the push, so no
moved-from list is read. Wall median 634 → 568 ms (−10%); output byte-identical for the four
projects × seeds 42–81. Compile was split but not changed: 100 `routelessChains` walks per Run
dominate (NEXT). Windows only; Linux is CI's.
