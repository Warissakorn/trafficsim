# TrafficSim

Standing orders for anyone — human or model — working in this repo. Read this first, every
session.

## What this is

A **traffic microsimulator with its own simulation engine, usable in real engineering work**
(D38), built to the modelling surface PTV Vissim users already think in, producing the
movement-level delay and LOS output that traffic impact studies require. See
[`docs/PROBLEM.md`](docs/PROBLEM.md) §2 for the capabilities a study needs from the engine.

`TrafficSim` is a working name. Naming was deferred until the end of M1 (D11); M1 is now accepted
(D49), so **the name is the owner's decision** (NEXT item 4). Do not rename the project, the
repository, or any package until the owner decides.

## Where it stands

- **Gates:** M1 usability accepted by owner ruling (D49; `docs/M1_ACCEPTANCE.md` keeps what the
  attempt did not show). M2's gate passed by the owner's judgment (D51–D53, `docs/M2_GATE.md`).
  **M0 plausibility is still open.** M3.2.2a–c, M3.2.3a–c and M3.2.4a–b are done (D54–D61): authored crossings and merges run on
  the admission solver and have an editor tab and canvas tool; next is **M3.2.5**, Stop and Yield — `docs/NEXT.md`.
- **Engine:** C++ core, reduced Wiedemann-inspired car-following (unvalidated), fixed-time signals,
  derived merge priority rules (M3.1; a derived stop line sits 1 m short of the join, D50),
  authored crossing and merge areas admitted by gap time/headway with whole-area reservation
  (M3.2.3a–c, D57–D59).
  Contracts: `docs/SIMULATION.md`.
- **Editor:** Qt Widgets, Vissim's modelling surface — `docs/NETWORK_EDITOR.md`. Project files are
  schema 14 (authored right-of-way controls, M3.2.2a, D54); unsupported network-object fields
  fail on load rather than vanish on save.
  Scenario JSON and editor `*.traffic.json` are **two formats on purpose** — read
  `NETWORK_EDITOR.md` §"Two file kinds" and D19a before touching either loader.
- **Demand:** an authored route names Links and Connectors, **never a lane**; `buildScenario`
  expands it per lane (`routeLaneChains`). Intervals, compositions, routing decisions and signal
  controllers all compile into ordinary core inputs and programs, so `core/` and every frozen
  fixture stay untouched by them. A route whose objects do not join up is kept and reported as
  `UNSUPPORTED_ROUTE_TOPOLOGY` — Run refuses it, authoring does not.
- **Settled geometry — do not re-open without a new measurement:** the mitered `offsetGeometry`
  stays (D23); the M1.17 lateral mouth wedge stays reverted (M1.18's flush mouth replaced it).
- **Results:** per-movement delay and per-approach queue, one run (M2.5, D39/D40) — whole-route
  delay, including ≈3 s of entry acceleration.

The history behind each of these — and the M1.x milestones — is in `docs/PROGRESS.md` and
`docs/archive/`; read the entry for what you are changing, not all of it.

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
- **Read D28 before adding another cache** — the editor caches Connector geometry against the
  values it is derived from; the same cache for Link geometry was measured and reverted.
- **Benchmarks are not in `check`:** re-run `tools/*_benchmark.cpp` before repeating any number,
  and a harness that drives Qt pumps the event loop between iterations, or it times Qt's deferred
  work instead of the code (D31).
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
