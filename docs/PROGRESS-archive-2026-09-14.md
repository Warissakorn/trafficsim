# PROGRESS archive — 2026-09-14

Entries moved whole from [PROGRESS.md](PROGRESS.md) to keep the active log below 500 lines.

### 2026-09-14 — hot path 1/3: route geometry resolved once per run

`routeParts` was recomputed for every vehicle on every tick — 1,867,548 calls in an
8-corridor profile — even though it is a pure function of an immutable `Scenario`. A
`ScenarioIndex` now resolves it once in `createSimulation` and is carried through `SimState`
as a `shared_ptr`, so per-tick state copies share it rather than duplicating it. Routes live
in a contiguous vector, so `partsFor` recovers a route's index from its own address in O(1)
with no extra lookup. The index is built from the **canonical** scenario, so part order
matches the sorted routes.

The uncached `routeParts`, `locateVehicle` and `occupiedSpans` overloads are retained for
`src/render/` and the existing tests; cached and uncached paths share one implementation each
so they cannot drift. `stepSimulation` tolerates a hand-built state without an index by
building one, rather than requiring every caller to change.

**Measured** (Release, GCC 13.3, median of 3, identical commands as the baseline entry above):
600 s of simulation on 1/2/4/8/16/32 corridors went 0.067/0.142/0.488/1.739/4.876/17.724 s to
0.036/0.079/0.285/1.157/3.257/11.327 s, i.e. **-33% to -46%, -36% at 466 vehicles**. The
O(V^1.85) growth is unchanged and is deliberately left to the next slice; this change removes
constant work per call, not the quadratic term.

No behaviour change was intended and none was observed: 12/12 headless CTest including the new
trajectory digest and the exact `29.24935` CLI pin, plus 12 multi-seed multi-size CLI runs
(4 network sizes x 3 seeds) byte-identical to the pre-change binary.

**Verification:** headless preset only. Qt is absent in this container, so the three desktop
suites were not built or run; `src/render/` compiles against the unchanged overloads but no
desktop verification is claimed.

### 2026-09-14 — core hot-path optimization: measurement baseline and trajectory guard

Profiling (Release, GCC 13.3, callgrind) of a synthetic multi-corridor scenario shows engine
cost growing at **O(V^1.85)** in active vehicle count: 466 vehicles take 17.7 s of wall time
for 600 s of simulation. Attribution: `__memcmp_avx2_movbe` 30.9% of all instructions (linear
`detail::byId` searches over `std::string` IDs), `closestVehicle` 39.3% inclusive (nested
`parts x spans` scan, the quadratic term), `routeParts` ~35% inclusive over **1,867,548 calls**
recomputing a value that is constant for an entire run.

Before changing any engine code, per-tick trajectory is now pinned. The four frozen TypeScript
baselines deliberately exclude `MovedEvent`, so positions between the every-100-tick checkpoints
were unguarded. `tests/reference/trajectory-digest.json` records weighted means over the full
`MovedEvent` stream for the same four seeds; means (not sums) keep magnitudes physical so the
existing 1e-7 tolerance applies unchanged, and order/segment/vehicle weights make a reordering
visible that plain sums would hide. The four TypeScript baselines were **not** touched.

The guard was verified non-vacuous: perturbing only the reported position in `locateVehicle`
by 1e-6 relative — which changes `MovedEvent` but not checkpointed `distance` — fails
`meanOrderWeightedPosition` on all four seeds. A 1e-9 relative perturbation of acceleration is
caught by the pre-existing checkpoint comparison. Both perturbations were reverted.

These digests characterize what the engine currently does. They are not a fidelity claim and
do not affect the not-yet-validated marker or any milestone gate.

**Verification:** headless preset, 12/12 CTest. Qt is not installed in this container, so the
three desktop suites were not built or run and no desktop verification is claimed.


### 2026-09-14 — CI packaging workflow for testable binaries

Added `.github/workflows/package.yml`, a manually dispatched (`workflow_dispatch`) and
`v*`-tag workflow that builds, tests and uploads runnable binaries so the owner can try a
build without a local toolchain. Linux uses the `release` preset with apt Qt 6 and
`ctest --preset release` under the offscreen platform; Windows uses MSVC 2022, vcpkg
nlohmann/json and an aqt-installed Qt 6.5.3, then `windeployqt` so the archive runs on a
clean machine. Both stage the existing `install()` rules into `dist/` (desktop, CLI, data
catalogs) and add a `RUN.txt` that repeats the not-yet-validated marker.

Existing `native.yml` push/PR verification is unchanged; packaging is deliberately a
separate workflow so a slow Qt install never sits in the pull-request path. These are
unsigned test builds — installer work still belongs to M7, and no milestone gate is
affected. No engine, model or UI code changed.

**Verification:** workflow YAML parsed locally; the build itself is proven by the CI run,
not by this container, which has neither Qt nor nlohmann/json installed.

### 2026-09-14 — MIT License added

Added a top-level `LICENSE` (MIT, copyright 2026 Warissakorn) and a README License section.
The bundled Noto Sans Thai font keeps its SIL OFL 1.1 terms and Qt keeps its own; the MIT
grant covers this repository's own source and documentation only. No code change.

### 2026-09-14 — M1.4 Connector editor implemented (D17)

Added general lane-to-lane creation using two canvas endpoint clicks or Properties.
Source/target markers, hover previews and cancellation are transient; committing creates
one History entry. Connectors can be selected on the canvas or by ID (including short
split connectors), reshaped by dragging/inserting/removing interior points, reset to a
lane-aligned sampled curve, or made straight. Endpoints remain attached to their lanes.
Properties now has Links, Connectors and Image tabs, with English/Thai controls.

Connector commands share endpoint maintenance with Link/Lane/driving-side edits and
route/input cleanup with link deletion. Retargeting preserves an unreferenced curve's
interior points by weighted displacement; referenced retargeting is rejected. Duplicate,
invalid and failed edits preserve revision, ID allocation, saved state and Redo. Confirmed
connector deletion restores related routes/inputs together on Undo. The existing format
persists exactly the edited polyline and IDs; no new schema, Qt dependency in the model,
simulation physics, demand or right-of-way behaviour was introduced.

**Verification:** GCC 13.3, Qt 6.4.2, nlohmann/json 3.11.3, CMake 3.28.3 on Linux.
The unchanged base first passed all 13 desktop CTest suites. The extended Debug and
Release desktop builds pass 15/15, and the independent Qt-free build passes 12/12.
There are 46 named native cases, including eight new Connector cases. UI workflows
exercise real endpoint picking, curve drags, insert/delete, cancellation and locked
endpoints, plus ID selection, Properties actions, reference-safe deletion/Undo, Unicode
save/reopen and Thai errors. Four TS baselines, seeded replay and M0 controls still pass.
Architecture/negative fixtures, the 500-line budget and whitespace checks pass. The Thai
Connector tab and curve were visually inspected at 1000×760.

M1.3.1 remains open, along with M1.5–M1.7 and the owner's M0/M1 acceptance gates.
These are local Linux results; Windows/macOS GUI execution and hosted CI are not
established by them. Curves are editable sampled polylines, not swept-path validation.


### 2026-09-13 — native migration and editor branches integrated into `main`

Merged `codex/cpp-desktop-migration` (D15) and `codex/network-editor-m1-1-3` (D16) into
`main` as two explicit merge commits. The migration commit is an ancestor of the editor
commit, so both branches shared one merge base at the last TypeScript commit `70383db`
and neither merge produced a conflict. No source or documentation was edited to make the
integration succeed; `main` now carries the C++20/CMake/Qt tree exactly as reviewed on
the branches.

Verified on the `headless` configuration only: full build clean, CTest **11/11 passing**,
including the architecture boundary, its negative fixtures, file sizes and the CLI checks.
**The Qt desktop harness and `editor_ui_tests` were not built or run** — no Qt in the
integration environment — so no desktop verification is claimed, per `docs/BUILDING.md`.
Building also required `nlohmann-json3-dev`, which a clean checkout must install first.

Neither the M0 acceptance gate nor the M1 gate is closed by this merge; merged code is
not a passed gate. `Next` is unchanged apart from its base note.

### 2026-09-10 — named Veytrix (D9)

Working name `TrafficSim` replaced throughout the documentation. `veytrix` is free on npm
and PyPI; `veytrix.com` is taken and `Vectrix` (electric scooters) is phonetically close —
both recorded in D9 as accepted, known risks rather than discovered later.

**Still to do by hand:** the GitHub repository is still called `trafficsim`. Renaming it needs
repository-admin access, which this session's GitHub app does not have — the owner renames it
in the repository settings, after which the git remote here needs updating.


### 2026-09-10 — Q1 and Q3 answered (D7, D8)

- **Q1 → international from the start** (D7). Consequences recorded: HCM as the default LOS
  pack with jurisdictions as swappable data, metric internally with switchable display units,
  and **left-hand/right-hand traffic as a first-class setting from M1** — added to the M1
  scope in `ROADMAP.md` because retrofitting it touches every geometry routine.
- **Q3 → the project owner performs the M2 gate alone** (D8). Recorded honestly as a
  weakening of the gate, with a mandatory mitigation: the M2 pass/fail criteria must be
  written into `ROADMAP.md` and committed **before** M2 implementation starts. `ROADMAP.md`
  now carries an unfilled placeholder for those criteria; starting M2 without filling it
  voids the gate.
- **Q5 opened:** final product name. `Veytrix` is a placeholder. `Headway` was considered
  and rejected — `headwaymaps/headway` is an existing open-source maps stack, too close a
  neighbour in the same field.

### 2026-09-10 — repository initialized, documentation spine written

Created a fresh repo for a new project, separate from the prior SUMO-wrapper effort.

**Written:** `PROBLEM.md` (who this is for, the engine-level walls that motivate D1, non-goals,
and what would make the project wrong), `PRINCIPLES.md` (hard rules, deliberate non-goals, and
measured discipline inherited from the prior effort), `ARCHITECTURE.md` (the five-layer map,
marked planned throughout), `ROADMAP.md` (M0–M7 with done-conditions and two hard gates),
`CLAUDE.md` (standing orders), this file.

**Decisions:** D1–D6 above. D1 is the one everything else rests on, and it has an explicit
falsification test at the M2 gate.

**No code was written.** The Systems table in `ARCHITECTURE.md` describes intent, not reality;
every row is marked `planned`.

**Next:** toolchain setup — see the `Next` section above.
