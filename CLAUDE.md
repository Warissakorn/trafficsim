# TrafficSim

Standing orders for anyone — human or model — working in this repo. Read this first, every
session.

## What this is

A **traffic microsimulator with its own simulation engine, usable in real engineering work**
(D38), built to the modelling surface PTV Vissim users already think in, producing the
movement-level delay and LOS output that traffic impact studies require. See
[`docs/PROBLEM.md`](docs/PROBLEM.md) §2 for the capabilities a study needs from the engine.

`TrafficSim` is a working name. **Naming is deliberately deferred until the end of M1** —
see D11. Do not rename the project, the repository, or any package before then.

**Latest correctness work:** the 2026-09-21 Network lifecycle audit fixes wrong-side endpoint
retargets, pathological steep mouths, physical range picking and coalesced-release gestures.
Read `docs/NETWORK_LIFECYCLE_AUDIT.md` for tested states and limits. Retarget rebuilds the
directed curve; Link movement still preserves the M1.20 world-position contract. Steep or
backwards authored mouths stay full-width and carry an alignment advisory, not a fidelity claim.

**Authoring foundation:** M1.21 implements the first authoring slice of the owner's three supplied
specifications. Read `docs/SPEC_AUDIT.md`, `docs/AUTHORING_EXTENSIONS.md` and the numbered
follow-ups in ROADMAP. Schema 7 is a supported subset, not the entire proposed specification;
unsupported network-object fields fail on load rather than disappearing on save. The owner
requested audit → implementation → inclusion of the original specs; source copies are in
`docs/specs/`. All remaining runtime, editor and scientific-validation gates remain open.

**Demand authoring (M1.25, M1.26):** routes and vehicle inputs are drawn on the canvas — click
the start, click each destination, and the chain between them is appended. **An authored route
names Links and Connectors, never a lane**, and `buildScenario` expands it into one runtime
route per lane (`routeLaneChains`, `src/model/network/routing.cpp`); an input's volume is the
Link total, split equally across those lanes. So a route covers the whole carriageway, changing
a Connector's lane count cannot invalidate it, and no command refuses an edit because a route
exists. Schema 12 (M2.2 adds counted `intervals`; M2.1.1 adds `linkId` on an input and a placed routing
decision with destination Links, expanded at compile time into static routes, D42/D43; M2.1.2 adds
per-interval turning flows on a decision, D45); older
files and M0 scenarios migrate on load. **M2.7 (schema 13):** a Signal head is its stop line, placed
by click on a lane or Connector path (D47), and shows a signal group of a fixed-time Signal
Controller, compiled into ordinary core programs `<controller>#<group>` (D48); schema 12 programs
shaped like a group migrate colour for colour, others stay legacy. A route whose objects do not
join up is kept and reported as `UNSUPPORTED_ROUTE_TOPOLOGY` — Run refuses it, authoring does
not. **M1.26.1:** `VehicleInput.laneShares` is optional relative weights, one per lane the route
currently expands to, in `routeLaneChains` order (D32); empty means the M1.26 equal split, and a
stale size (the network changed lane count since) degrades to it rather than landing on the wrong
lane. `buildScenario` normalises by their sum. Persisted only when set, so an unedited input's
file and compiled volumes are unchanged. The vehicle-input dialog (`src/shell/editor_demand.cpp`)
now shows one weight field per lane the selected route currently reaches, seeded from a stored
value only when its size still matches; leaving the fields untouched leaves `laneShares`
untouched too, which is what keeps an unedited input's save and compiled volumes exactly as they
were. **M2 is under way** — its gate criteria are registered (ROADMAP §M2, D34). Counted
`intervals` (M2.2), `compositionId` from `data/compositions/` (M2.3) and static routing decisions
(M2.4) are all expanded at compile/resolve time into ordinary core inputs, so `core/` and every
frozen fixture are untouched. Connectors meeting at a lane start are ordered by M3.1's derived
rule (M2.0.1, D35 — not M3). **M2.5 is implemented** — per-movement delay and per-approach queues for one run in the
editor's Results tab and `trafficsim-cli --project` (D39, D40). It is whole-route delay, including
≈3 s of entry acceleration. **M2's gate (M2.6) passed by the owner's judgment on 2026-09-25 (D51–D53, `docs/M2_GATE.md`);
next is M3.2.2 per `docs/NEXT.md`.** A decision's station along its Link is still M2.1; per-interval flows are M2.1.2 (D45).

**Build and redraw (M1.27, M1.27.1):** `src/project/json.hpp` declares `Json` through
`<nlohmann/json_fwd.hpp>` and `trafficsim_shell` precompiles the Qt surface the UI test
executables reuse — a clean build is 86 s → 64 s, and a PCH here holds third-party headers only
(D27). `tools/editor_benchmark.cpp` then made the canvas measurable: the editor caches each
Connector's paths, boundaries and markings **against the values they are derived from** (D28),
taking a 160-link corridor from 95 ms a frame to 11. Read D28 before adding another cache — the
same one for Link geometry was measured and reverted. **M1.27.2:** `tools/engine_benchmark.cpp`
commits an engine fixture, and a runtime vehicle now names its input, route and type by **slot
in the canonical Scenario** rather than by id (D29), halving the run; names survive at the
boundary, so every frozen fixture is byte-identical. **M1.27.3 closes the program:**
`tools/gesture_walkthrough.cpp` counts the inputs an authoring task costs and replays the Vissim
reflexes — five of six transfer, and the sixth, `Ctrl`+left-click, does **nothing** rather than
the wrong thing the parity table claimed, which the owner ruled stays as it is (D30). None of the
three benchmarks is in `check`, so re-run them before repeating any of these numbers — and a
harness that drives Qt pumps the event loop between iterations or it times Qt's deferred work
instead of the code (D31, which is how the editor's first published figures came out wrong).

**Previous work:** M1 implementation covers M1.1–M1.20, including controlled splits,
demand/control editing, recovery, in-editor Run, connector ranges and levels/display types,
body attachments, fixed lane edges and Ctrl selection/copy, attachment stations in metres,
Vissim's Intermediate points, a Name on every object, group move, and a Connector mouth cut flush on
the Link's cross-section (**M1.18**, a longitudinal slide along each boundary — *not* M1.17's
lateral wedge, which folded and stays reverted).
M1.11.1 and M1.12.1 are both closed: lanes are cut into runtime sections at interior attachments,
a Connector arriving on a lane body merges under **M3.1** (a priority rule with a gap time and
headway — which does **not** close M3), and a Connector carries its own lane widths and divider
markings in schema 6. **M1.12.2** is closed: the reported miter "bulge" was
measured along the cross-section, where a mitered corner's diagonal is `width/cos(φ/2)` by
construction. Square to the road the carriageway is exact, so `offsetGeometry` was not changed —
removing the miter would reinstate the pinch it exists to fix. **M1.12.3 is closed by M1.19:**
the Link wins at the mouth and authored Connector widths take over through the body.
**M1 usability is accepted by owner ruling (2026-09-25, D49)** after one timed attempt that did not
include pockets or an aerial image — `docs/M1_ACCEPTANCE.md` keeps what it did not show. M0
plausibility is still open. **D50:** a derived priority rule's stop line is 1 m short of the join.

**Status: M0 C++ core and network model implemented; acceptance gate still open.** The Qt
Widgets network editor and the native CLI exercise both systems; **M1.24 retired the separate
M0 harness window**, so `trafficsim-desktop` opens the editor and the M0 plausibility
observation is made there (ROADMAP M0 says where). Read `docs/SIMULATION.md`
for the current simulation contracts, `docs/NETWORK_EDITOR.md` for the editor, then `docs/NEXT.md`.
Scenario JSON and editor `*.traffic.json` projects are two formats on purpose — see
`docs/NETWORK_EDITOR.md` §"Two file kinds" and D19a before touching either loader.

## Read these before working

| File | When |
|---|---|
| [`docs/PROBLEM.md`](docs/PROBLEM.md) | Before proposing any feature. Scope lives there. |
| [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md) | Before arguing with an existing choice. Rules 1–4 are correctness. |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Before adding a system. Update when the map changes. |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | To see which milestone this is and what closes it. |
| [`docs/NEXT.md`](docs/NEXT.md) | **Every session, first.** The one live to-do; write the next session's work here, never into a `PROGRESS.md` entry. |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | The history and the reasoning, including the **decision log** every `D`-number here points at. Read the entry that touches what you are about to change. |
| [`docs/VISSIM_PARITY.md`](docs/VISSIM_PARITY.md) | Before proposing editor UX work. **§1a and §2 are current; §1 and §6 are the dated 2026-09-14 assessment and under-report the product.** |
| [`docs/CONNECTOR_PARITY_AUDIT.md`](docs/CONNECTOR_PARITY_AUDIT.md) | Before touching the Connector. Holds the two benchmarks apart — the supplied target spec vs never-measured Vissim — and records the defects no test covers. |

## Stack

C++20, CMake 3.24+, Qt 6.4+ Widgets for the desktop, and nlohmann/json outside the core.
`core/` depends only on its own headers and the standard C++ library. No Qt, file I/O,
JSON, model types or wall clock may reach it. D15 supersedes the initial D3/D4 stack.
See `docs/BUILDING.md` and `docs/MIGRATION.md`.

## Commands

```bash
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
./build/desktop/bin/trafficsim-desktop
./build/desktop/bin/trafficsim-cli 42
cmake --build build/desktop --target check
```

Use `--preset headless` when Qt is unavailable. Do not claim desktop verification
from a headless-only build.

If either fails on a clean checkout, fixing that comes before any feature work.

## Hard rules

The full list with reasoning is in [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md). The ones that
get broken by accident:

1. **`core/` imports nothing.** No UI, no I/O, no framework. The moment it does, the engine
   stops being testable, batchable and portable, and that is the project's most valuable
   property.
2. **Reproducibility is not optional.** Same scenario + seed + engine/toolchain = the same trajectory.
   No wall-clock, no unordered iteration, no thread-dependent floating point in `core/` or
   `eval/`.
3. **One source of truth.** If two places need the same value, the boundary is wrong — move
   it, do not sync it.
4. **Never claim fidelity that has not been measured.** Until M6 passes, every results screen
   carries a "not yet validated" marker.
5. **Content is data, not code.** Vehicle types, behaviour presets, LOS thresholds live in
   `data/`. If adding the 50th one needs a code edit, fix the boundary instead.
6. **Files stay near 500 lines.** Check with `trafficsim-check-file-sizes .`
7. **Never leave the build red.** If it cannot be made green, revert to the last green commit
   and write down what was attempted.
8. **Never close a milestone that has not met its gate.** Merged code is not a passed gate.
   If something must ship incomplete, carve it into a new numbered milestone in
   `docs/ROADMAP.md` in the same session.

## Working rules

- **One system per session.** If it looks like two, it is two.
- **Write the interface first** — the functions other systems will call. That contract is what
  lets a later session build against this without reading its insides.
- **Smallest version that works.** Speculative generality written blind is the most expensive
  thing in this repo.
- **Update `docs/PROGRESS.md` before committing** — what changed and the reasoning behind any
  non-obvious decision. This is the memory the next session runs on. **What is next goes in
  `docs/NEXT.md` instead**, rewritten rather than appended, so there is one live to-do.

## Session start

Read this file, `docs/ARCHITECTURE.md`, and `docs/NEXT.md`. Run the test command. Then start
on what `NEXT.md` says — do not re-plan; the plan is already there. Reach into `PROGRESS.md`
for the entry behind whatever you are changing, not as a matter of course: it is long, and
reading all of it to find twenty lines is what moved `Next` out of it.

## Session end

Stop at roughly three-quarters of context, or when the current system is done. In order:
build green (or reverted to green) → `docs/NEXT.md` rewritten specifically enough to need no
questions → decisions logged with reasons in `PROGRESS.md` → commit → tell the user what
changed, in outcomes.

## Layout

```
docs/           NEXT · PROBLEM · PRINCIPLES · ARCHITECTURE · ROADMAP · PROGRESS
src/core/       simulation engine — imports nothing
src/model/      network · demand · control data model
src/commands/   every mutation, undoable, one registry
src/project/    load, save, revisions, validation
src/editor/     tools, inspector, tables
src/shell/      layout, palette, i18n
src/eval/       event stream → measurements
src/runner/     multi-seed batches
src/report/     impact-study output
data/           vehicle types · behaviour presets · LOS thresholds
tests/
```

## Project-specific rules

<!-- Conventions discovered while building. Add here rather than re-deriving them. -->
- Documentation is written in **English**. UI strings live in translation files only.
- Screen vocabulary follows Vissim ("link", "connector", "conflict area"); **parameter names
  are never translated**, so users can find them in the project file.
- Current car-following is a reduced Wiedemann-inspired prototype, not W74/W99. Never
  remove its unvalidated marker or accept merging paths before right-of-way is implemented.
- The previous TypeScript application is retained in Git history, not as a second engine.
- Native tests compare four TS baselines with a 1e-7 physical-value tolerance and exact
  same-build replay. Never regenerate baseline fixtures to make a failing port pass.
- UI text lives in `data/locales/`. Runtime catalogs are copied beside executables.
- **A test that forces a failure must first assert that the forcing worked**, then assert the
  consequence. Order it the other way and a platform where the setup silently no-ops reports a
  product bug that is not there. Never force one with a platform-specific mechanism — POSIX
  permissions, root behaviour, or `XDG_*` variables, which Windows and macOS ignore.
- Qt UI suites run on Linux **and Windows** in `native.yml`. "Verified locally" means Linux
  only; say so, and do not read a green Linux run as cross-platform evidence.
- The owner authorized the full stack migration (D15); it supersedes one-system scheduling
  guidance for that migration only. Existing M0/M1 acceptance gates still apply.

- The owner authorized M1.1–M1.3 together (D16). Project never imports command or Qt types; the shell owns atomic file replacement. Never equate authoring support with runtime support.
