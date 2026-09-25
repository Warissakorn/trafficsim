# PROGRESS archive — 2026-09-24, M1.26.1 through the M3 contract

Moved out of `PROGRESS.md` on 2026-09-25 as the oldest live entries, with the two superseded
`## Next` blocks that sat below them. Verbatim.

## 2026-09-24 — M3 contract and acceptance preparation (D41)

Owner requested the proposed sequence. `M3_PLAN.md`, `M3_CONTRACT.md` and `M3_ACCEPTANCE.md`
record the source audit, interfaces, ordered slices and 26 evidence cases. The audit identifies
missing authored priority persistence, insufficient merge-graph validation and signal-bound
queue counters. No source, schema or fixture changed; M2.6 is still unperformed.
Local file-size/architecture guards and the architecture negative self-test pass (standalone
C++20 builds); `git diff --check` passes. CTest could not start: CMake/CTest/Ninja are absent.
No local application, desktop or Windows verification is claimed. CI belongs to the draft PR.

---

## 2026-09-24 — M2.5: delay per movement and queue per approach, one run

`src/eval/movement.*` (core types only) and `src/project/evaluation.*` (movements from authored
routes, counters from signal heads, queue conditions from `data/evaluation/`), shown in the
editor's **Results** tab and printed by `trafficsim-cli --project FILE [--csv FILE]`. `core/`,
every frozen fixture and `trafficsim-cli 42` are unchanged. The four-leg fixture, seed 42, gives
12 movements with 6–134 trips, a mean delay of 35–56 s and approach queues up to 107 m. M2 runs one
seed, so "plausible" is the only claim: a 120 s cycle with 20–30 s greens gives
a uniform-delay term of 34–42 s before entry acceleration and queue spillback.
**The analytic test found a bias worth knowing:** vehicles enter from standstill, so an
unimpeded trip carries about `v/(2a)` ≈ 3 s of delay. It is pinned and documented, not hidden
(D39). A run's first cut of the tab stacked the two tables and showed one row each, found from a
screenshot; they are side by side now.

---

## 2026-09-24 — Purpose restated; gate re-registered (D38)

Owner ruling: the project is a traffic simulator usable in real engineering work, and the
comparison with another simulator is removed from every file. `PROBLEM.md` §2 now lists the
capabilities a study needs rather than what another tool lacks, §7.1 is "an engineer cannot
complete a real study with it", and the M2 gate is C1 + C2 with C4 recorded. Docs only; no code,
test or fixture mentioned it. **M2.5 is next.**

---

## 2026-09-24 — M2 registered; M2.0.1 and M2.2–M2.4 implemented

**Why everything expands before the core.** Intervals, compositions and routing decisions each
split a Poisson stream — by period, by type share, by route share — and a split Poisson stream is
exactly a set of independent Poisson streams. So each concept became an expansion into ordinary
core inputs (decision → composition in `resolveCatalogs`, then period → lane in `buildScenario`),
and one-way splits keep the plain id. The engine did not change, and neither did a fixture.
**Why intervals are the source.** With `intervals` set, the scalar start/end/volume are derived
(`deriveInputTotals`) on put and on read, so no file can hold two volumes. **Why M2.0.1 is not
M3:** it applies M3.1's existing rule where it was skipped; drawing order decides, which is
honest only because protected phasing rarely lets it bind (D35). Four tests that pinned the old
refusal now pin the arbitrated merge and that removing the rule re-fires the guard.

---

## 2026-09-24 — M2.0 closed except its decisions: same-station cut, dropped lanes, amber clamps

**Same station:** the cut is reused rather than refused, and the later Connector also gives way to
the earlier one — without that the §3.3 pair would enter together. **Dropped lanes:** still
dropped, now reported (`AMBIGUOUS_ROUTE_STEP`, advisory). **Clamps:** all at amber onset, and the
frozen baselines hold the same, so fixing them needs the owner; a test pins the diagnosis. CI on
the four-leg commit passed all five jobs, Windows included.

## 2026-09-24 — The four-leg intersection, built and run (M2.0)

`tools/four_leg_network.hpp` builds it through the editor's commands, so the committed
`data/projects/four-leg-signalised.traffic.json` is a document an author could have drawn; the
file is compared to the builder at 1e-9 (computed curves may round differently under MSVC).
It runs with no diagnostic only because turns join each exit part way along — the natural drawing
is 8 × `UNSUPPORTED_MERGE`, pinned by a test M3 or M2.0.1 flips. Findings: `M2_PLAN.md` M2.0.

## 2026-09-24 — M1 reviewed, M2 planned, criteria drafted but not registered

Owner request; everything is in [`M2_PLAN.md`](M2_PLAN.md), no code changed; `desktop` 36/36 and
`check` green on Linux. **The criteria stay a draft** because D8 makes pre-registration the
owner's own act; ROADMAP §M2 only points at it. M2 can only run protected phasing
honestly, so the gate study is a signalised intersection with protected phasing (D38 later
narrowed the criteria to C1 + C2).
**No fixture builds a four-leg intersection anywhere**, so M2.0 (the duplicate-station fix plus
a runnable four-leg fixture) goes first, allowed before the criteria as M1 defects.

---

## 2026-09-24 — M1.26.1, storage and the dialog

**M1.26.1's real question — decided (D32).** `VehicleInput` gains `laneShares`, optional relative
weights in `routeLaneChains` order; empty is the M1.26 equal split, and a stale size (the route's
lane count changed since) degrades to it rather than misapplying a weight. `buildScenario`
normalises by their sum and clears the field on each compiled per-lane input; `definitionJson`
persists it only when set, so an unedited file and its compiled volumes are unchanged. Schema
bumped to 9 for the new optional field; six tests asserting the exact prior schema number were
updated to match, and two new tests hold the gate: round-trip plus the unedited-file omission
(`project_tests.cpp`), and an uneven 1:2:3 split plus the stale-size degrade
(`editor_model_tests.cpp`). `headless` preset: 23/23, 162 test-function checks all passing.

**The editor surface, same session: M1.26.1 is closed.** The vehicle-input dialog
(`src/shell/editor_demand.cpp`) now shows one `QDoubleSpinBox` per lane the *selected* route
currently reaches, rebuilt whenever the route combo changes (the lane count is the route's, not
the input's). Seeded from a stored `laneShares` only when its size still matches; a "dirty" flag
set only by an actual `valueChanged` means leaving the fields alone leaves the input's
`laneShares` exactly as it was — usually empty, which is what keeps the bit-identical default
from the paragraph above true through the dialog too, not just through direct model edits.
`demand-ui`'s existing two-lane fixture got three new checks: default fields read 1/1, a 2:1 edit
round-trips through reopen, and cancelling a reopened dialog leaves the stored weights alone.
Needed `qt6-base-dev` installed in this container (it was not present) to build and run the
`desktop` preset at all; `desktop`: 36/36, `headless`: 23/23.

---

## Next

**M1.27 is closed.** Its four stages are done: clean build 86 s → 64 s, a 160-link corridor
95.1 → 10.6 ms a frame and 57.2 → 10.0 ms a pick (corrected 2026-09-23), the engine run
roughly halved, and the
gesture surface counted with one deliberate dead end left in it. Three benchmarks are committed
— `trafficsim-engine-benchmark`, `trafficsim-editor-benchmark`, `trafficsim-gesture-walkthrough`
— and none of them is in `check`, so re-run them by hand before claiming any of those numbers
again.

**M1.26.1, adjustable per-lane shares, is next**, and the owner asked for it two sessions ago.
Decide where the shares live before writing any UI — that is the whole task, because
`VehicleInput` is a core type the scenario format shares, and an equal split must stay the
compiled result of an unedited input, bit for bit.

Then **M2.1** behind **M2's pre-registered criteria, still unwritten, which block all of M2**
and need the owner. Still open: M1.22 (geometry/snapping tools, custom pivots, layer locks,
bulk inspection), M1.23's culling/LOD and the hard-coded 20 km `sceneRect`, the shared-station
`runtimeSections` refusal (§3.3, M3.2), and scenario-JSON export from the editor, which is
neither implemented nor booked. Inside a tick `occupiedSpans` is the largest single cost at
9.8%; no milestone is booked for it and none is needed yet. **M1's timed owner exercise
(`docs/M1_ACCEPTANCE.md`) is untouched by all of this** — a counted walkthrough is not a timed
one, and M1.27.3 never claimed to close it.

---

## Next

Moved to [`NEXT.md`](NEXT.md) on 2026-09-23. A session read this whole file to find twenty
lines of it; now it reads that one. The owner's standing items live there too.
