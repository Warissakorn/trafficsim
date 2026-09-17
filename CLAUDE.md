# TrafficSim

Standing orders for anyone — human or model — working in this repo. Read this first, every
session.

## What this is

A **traffic microsimulator with its own simulation engine**, built to the modelling surface
PTV Vissim users already think in, producing the movement-level delay and LOS output that
traffic impact studies require. Deliberately **not** a front end over another engine — see
[`docs/PROBLEM.md`](docs/PROBLEM.md) §2 for why that was tried and where it hit walls.

`TrafficSim` is a working name. **Naming is deliberately deferred until the end of M1** —
see D11. Do not rename the project, the repository, or any package before then.

**Current work:** M1 implementation covers M1.1–M1.17, including controlled splits,
demand/control editing, recovery, in-editor Run, connector ranges and levels/display types,
body attachments, fixed lane edges and Ctrl selection/copy, attachment stations in metres,
Vissim's Intermediate points, a Name on every object, group move, and the wedge mouth.
M1.11.1 is closed: lanes are cut into runtime sections at interior attachments, and a Connector
arriving on a lane body merges under **M3.1**, a priority rule with a gap time and headway — which
does **not** close M3. **One carve-out remains open:** M1.12.1 (a Connector's own lane widths and
markings), plus one booked defect, the miter bulge.
**Remaining gate:** the owner performs the timed four-leg/aerial-image/reopen exercise in
`docs/M1_ACCEPTANCE.md`. M0 plausibility and M1 usability are not closed by automated tests.

**Status: M0 C++ core and network model implemented; acceptance gate still open.** A Qt
Widgets desktop harness and native CLI exercise both systems. Read `docs/SIMULATION.md`
for the current simulation contracts, `docs/NETWORK_EDITOR.md` for the editor, then `Next` in `docs/PROGRESS.md`.
Scenario JSON and editor `*.traffic.json` projects are two formats on purpose — see
`docs/NETWORK_EDITOR.md` §"Two file kinds" and D19a before touching either loader.

## Read these before working

| File | When |
|---|---|
| [`docs/PROBLEM.md`](docs/PROBLEM.md) | Before proposing any feature. Scope lives there. |
| [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md) | Before arguing with an existing choice. Rules 1–4 are correctness. |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Before adding a system. Update when the map changes. |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | To see which milestone this is and what closes it. |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | Every session. Read `Next` first. |
| [`docs/VISSIM_PARITY.md`](docs/VISSIM_PARITY.md) | Before proposing editor UX work. Says which gaps are booked and which are deliberately not. |

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
- **Update `docs/PROGRESS.md` before committing** — what changed, what is next, and the
  reasoning behind any non-obvious decision. This is the memory the next session runs on.

## Session start

Read this file, `docs/ARCHITECTURE.md`, and the **Next** section of `docs/PROGRESS.md`. Run
the test command. Then start on `Next` — do not re-plan; the plan is already here.

## Session end

Stop at roughly three-quarters of context, or when the current system is done. In order:
build green (or reverted to green) → `Next` written specifically enough to need no questions
→ decisions logged with reasons → commit → tell the user what changed, in outcomes.

## Layout

```
docs/           PROBLEM · PRINCIPLES · ARCHITECTURE · ROADMAP · PROGRESS
src/core/       simulation engine — imports nothing
src/model/      network · demand · control data model
src/commands/   every mutation, undoable, one registry
src/project/    load, save, revisions, validation
src/render/     network view, isolated
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
