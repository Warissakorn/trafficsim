# TrafficSim

Standing orders for anyone — human or model — working in this repo. Read this first, every
session.

## Start here

These instructions apply to the whole repository, regardless of the AI tool used.
The user's current instruction takes priority over the standing session queue.

1. Read [NEXT](docs/NEXT.md) for the live queue and pending owner reviews.
2. Read [ARCHITECTURE](docs/ARCHITECTURE.md) and the relevant module README.
3. Use [the task reading map](docs/README.md#read-by-task) to select the contract,
   acceptance rows and decision history for this change. Do not load the entire archive.
4. Run the appropriate checks below. Report unavailable tools or failed checks honestly.

TrafficSim owns a traffic microsimulation engine for engineering studies (D38).
Scope is in [PROBLEM](docs/PROBLEM.md). `TrafficSim` is a working name (D11);
do not rename the repository, project or packages until the owner decides.

## Documentation authority

| Information | Maintained source |
|---|---|
| Next work and pending owner checks | [NEXT](docs/NEXT.md), rewritten rather than appended |
| Milestone scope, status and closing gates | [ROADMAP](docs/ROADMAP.md) |
| Project principles and their reasoning | [PRINCIPLES](docs/PRINCIPLES.md) |
| Module boundaries and interfaces | [ARCHITECTURE](docs/ARCHITECTURE.md) and module READMEs |
| Behaviour and file semantics | The relevant contract in [the documentation map](docs/README.md) |
| Session history and decisions | [PROGRESS](docs/PROGRESS.md) and [decision record/index](docs/decisions/README.md) |
| Toolchain setup | [BUILDING](docs/BUILDING.md) |

README summaries are navigation, not another status ledger. Dated audits, plans and
archive entries describe their own date. A supplied target specification is not proof
of implementation or validation. If sources disagree, inspect the applicable decision,
code and tests; record the discrepancy rather than silently changing an owner gate.

## Read these before working

| File | When |
|---|---|
| [`docs/PROBLEM.md`](docs/PROBLEM.md) | Before proposing any feature. Scope lives there. |
| [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md) | Before arguing with an existing choice. Rules 1–4 are correctness. |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Before adding a system. Update when the map changes. |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | To see which milestone this is and what closes it. |
| [`docs/NEXT.md`](docs/NEXT.md) | **Every session, first.** The one live to-do; write the next session's work here, never into a `PROGRESS.md` entry. |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | The session history and reasoning. Use the [decision index](docs/decisions/README.md) to find the relevant D-number; read only related entries. |
| [`docs/audits/VISSIM_PARITY.md`](docs/audits/VISSIM_PARITY.md) | Before proposing editor UX work. **§1a and §2 are current; §1, §3 and §6 are the dated 2026-09-14 assessment and under-report the product.** |
| [`docs/audits/CONNECTOR_PARITY_AUDIT.md`](docs/audits/CONNECTOR_PARITY_AUDIT.md) | Before touching the Connector. Holds the two benchmarks apart — the supplied target spec vs never-measured Vissim — and records the defects no test covers. |
| [`docs/README.md`](docs/README.md) | To find any other document. The task reading map and folder indexes; index new maintained docs in the same commit. |

## Stack

C++20, CMake 3.24+, Qt 6.4+ Widgets for the desktop, and nlohmann/json outside the core.
`core/` depends only on its own headers and the standard C++ library. No Qt, file I/O,
JSON, model types or wall clock may reach it. D15 supersedes the initial D3/D4 stack.
See `docs/BUILDING.md` and `docs/reference/MIGRATION.md`.

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

The full list with reasoning is in [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md), except rule 5
below, whose reasoning is D7. The ones that get broken by accident:

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
  non-obvious decision. Record new decision IDs and reasoning once in `docs/decisions/RECORD.md`.
  This is the memory the next session runs on. **What is next goes in
  `docs/NEXT.md` instead**, rewritten rather than appended, so there is one live to-do.

## Session start

Read this file, `docs/ARCHITECTURE.md`, and `docs/NEXT.md`. Run the test command. Then follow the user's current task, or
`NEXT.md` when no task was supplied; do not silently reorder the owner's queue. Reach into `PROGRESS.md`
for the entry behind whatever you are changing, not as a matter of course: it is long, and
reading all of it to find twenty lines is what moved `Next` out of it.

## Session end

Stop at roughly three-quarters of context, or when the current system is done. In order:
build green (or reverted to green) → `docs/NEXT.md` rewritten specifically enough to need no
questions → session logged in `docs/PROGRESS.md`, decisions with reasons in `docs/decisions/RECORD.md` → commit → tell the user what
changed, in outcomes.

## Layout

```
docs/           NEXT · PROBLEM · PRINCIPLES · ARCHITECTURE · ROADMAP · PROGRESS
src/core/       simulation engine — imports nothing
src/model/      network · demand · control data model
src/commands/   every mutation, undoable, one registry
src/project/    load, save, revisions, validation
src/editor/     canvas: tools, gestures, run view
src/shell/      window, inspector, tables, results, layout, palette, i18n
src/eval/       event stream → measurements
src/runner/     multi-seed batches (planned; README only, M5)
src/report/     impact-study output (planned; README only, M5)
data/           vehicle types · behaviour presets · compositions · priority rules · levels ·
                display types · locales · example projects (LOS thresholds planned, M5)
tools/          CLI, sweeps, benchmarks, checks
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

## Model invariants to check before editing

- Scenario JSON and editor `*.traffic.json` are deliberately different formats (D19a).
  Read NETWORK_EDITOR's save/recovery contract before changing either loader. Unsupported
  network-object fields must fail on load rather than vanish on save.
- Authored routes name Links and Connectors, never lanes. `buildScenario` expands
  `routeLaneChains`; authoring-only Demand fields compile into ordinary core inputs.
  Keep disconnected routes and report `UNSUPPORTED_ROUTE_TOPOLOGY`; Run refuses them,
  authoring does not. Read the Demand contracts before changing that expansion.
- Do not reopen settled geometry without new measurements: mitered `offsetGeometry`
  stays (D23), and the M1.17 lateral mouth wedge stays reverted in favour of M1.18.
- Consult SIMULATION for results semantics. Completed-trip/whole-route delay includes
  source waiting and acceleration; it is not HCM control delay or validated LOS.
