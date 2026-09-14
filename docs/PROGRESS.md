# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks to `PROGRESS-archive.md` whole if this gets
long.

---

## Next

**Review M1.4 Connector tools, then implement M1.5 inspection and diagnostics.**

1. Build the desktop, run CTest, and launch `trafficsim-desktop --editor --language th`.
2. Follow `docs/NETWORK_EDITOR.md`: connect lanes by endpoint picking or Properties,
   reshape a curve, change lane widths/drivingSide, Undo/Redo, then save and reopen.
   Existing short/overlapping connectors can be selected by ID in Properties.
3. M1.5: add object tables and multi-selection through the existing History path. Add
   structured authoring diagnostics with object IDs and jump-to-object navigation;
   keep draft validity separate from the M0 compiler's unsupported runtime features.
4. Preserve the M1.3.1 signal-bearing-link split guard. That follow-up still needs a
   stationing/remapping policy for heads in upstream/downstream and connector spans.
5. M1.6 owns recovery/assets; M1.7 owns revision-to-run handoff and the timed four-leg,
   aerial-image, ten-minute/reopen exercise. M0 and full M1 owner gates remain open.

**Implementation:** `src/commands/connector_commands.hpp`,
`src/model/network/connector_geometry.cpp`, `src/editor/canvas_connectors.cpp`,
`src/shell/editor_connectors.cpp`. Based on `main` commit `8ff7a53` (2026-09-13).

---

## Backlog (M0, in order)

- [x] Toolchain + directory skeleton + core-import guard
- [x] `Scenario` type and a fixture: two crossing movements with explicit connectors
- [x] Fixed-timestep loop; one vehicle traverses links with continuous route distance
- [x] Reduced Wiedemann-inspired car-following; vehicles queue behind each other
- [x] Fixed-time signal; vehicles stop at red, discharge at green
- [x] Vehicle input generating arrivals from a seeded stream, retaining blocked arrivals
- [x] Native Qt harness: vehicles as dots on links (former canvas preserved in Git history)
- [x] Headless completed-trip delay diagnostic and seeded replay regression
- [ ] Owner's M0 plausibility acceptance (still open)

Later milestones are in [`ROADMAP.md`](ROADMAP.md). The owner explicitly authorized M1.1–M1.3 in D16; all other milestone gates remain in force.

---

## Open questions

Ask these before the milestone they block.

| # | Question | Blocks | Notes |
|---|---|---|---|
| ~~Q1~~ | ~~Thailand-first or international?~~ | — | **Answered 2026-09-10: international from the start.** See D7. |
| Q2 | Which lane-changing model? | M1 | MOBIL and Gipps are both defensible. Needs a short spike, not a debate. |
| ~~Q3~~ | ~~Who are the three engineers for the M2 gate?~~ | M2 gate | **Answered 2026-09-10: the project owner performs the gate alone.** This materially weakens it — see D8 and the mitigation in `ROADMAP.md` M2. |
| Q4 | Which published benchmarks define the M6 tolerance? | M6 | Decide before M5 so evaluation is built to be checkable against them. HCM is the likely baseline now that D7 makes the tool international. |
| Q5 | Final product name | Nothing before M1 | **Deferred until the end of M1** by D11 — not a blocker on any current work. Candidates and collision findings are in the D11 row; reuse them. |
| ~~Q6~~ | ~~Register `velk` on npm and PyPI~~ | — | **Withdrawn 2026-09-11 as moot** — no settled name to register. The registration question returns with the name at M1. |

---

## Decisions

Non-obvious choices **and the reasoning**. Without the reasoning a later session will
"improve" a decision away and break something invisible.

| # | Date | Decision | Why | What would make it wrong |
|---|---|---|---|---|
| D1 | 2026-09-10 | **Own simulation engine, not a front end over an existing one** | A prior six-milestone effort built a Vissim-shaped UI over SUMO and hit walls that are in the engine, not the interface: conflict areas are output-only, gap times are not expressible, signal heads must sit at stop lines, and the two things the job is actually paid for — per-movement evaluation and multi-run averaging — had to be built from scratch regardless. See `PROBLEM.md` §2. | If the M2 gate finds practising engineers would be satisfied by the wrapper. This is the single most expensive decision in the project and it has an explicit test. |
| D2 | 2026-09-10 | **Link-based network model natively; junctions are derived, not authored** | This is how the audience thinks and it is the whole point of D1. Translating to a node–edge model would reintroduce the impedance the prior effort spent six milestones papering over. | If deriving junction geometry from links proves intractable at M1. |
| D3 | 2026-09-10 | **TypeScript everywhere to start; `core/` written so it can be ported to a compiled language later without touching anything above it** | Microsimulation is CPU-bound and a compiled core is probably where this ends up. But picking a stack the user cannot run today, to solve a performance problem not yet measured, is the classic way to stall at milestone 0. Hard rule 1 (`core/` imports nothing) makes the port a contained job later, and makes it measurable first. | If M0 cannot reach real-time on a single intersection — then port immediately rather than optimising TypeScript. |
| D4 | 2026-09-10 | **Desktop is a constraint from day one, a milestone at the end** | Web-first keeps the development loop fast; a framework-free core plus isolated rendering means desktop packaging is packaging, not a rewrite. A boot smoke test in a desktop shell runs from M1 so it never becomes a surprise. | If a required capability (native file dialogs, offline licensing) turns out to need a different shell architecture. |
| D5 | 2026-09-10 | **A results screen carries a "not yet validated" marker until M6 passes** | Numbers from this tool go into documents submitted to regulators. An unvalidated engine that looks authoritative is worse than no tool. | Nothing. This one is not negotiable before M6. |
| D7 | 2026-09-10 | **International audience from the start, not Thailand-first** | Nothing in the engine is jurisdiction-specific, and the parts that are — LOS thresholds, report layouts, units — are data, not code, so building them swappable costs little now and a retrofit costs a lot. Three concrete consequences: HCM is the default LOS pack with others as swappable data; metric internally with display units switchable; **left-hand and right-hand traffic is a first-class network setting from M1** (Thailand, UK, Japan, Australia all drive left — a prior effort never implemented it at all). | If it turns out every real user is in one jurisdiction and the generality is unused weight. |
| D8 | 2026-09-10 | **The M2 gate is performed by the project owner alone, not three independent engineers** | The owner is a practising traffic engineer and no outside participants are available. Accepted with eyes open: this is **a materially weaker test than the one D1 needs**, because the person judging whether a free SUMO-based tool would have sufficed is the same person who chose to build an engine instead. Mitigation, mandatory: **the pass/fail criteria are written down and committed before M2 implementation starts**, so the judgement cannot be rationalised after the fact. Adding outside engineers later strengthens the gate and is never wasted. | Nothing makes it wrong; it is simply weak. Treat a pass as "not disproven", not as "confirmed". |
| D9 | 2026-09-10 | **The project is named Veytrix** | Chosen by the owner after working through several naming directions (domain jargon, borrowed engineering terms, abstract coinages, Thai-rooted feminine names). Verified free on npm and PyPI. **Two known flags, accepted:** `veytrix.com` is already resolving to something, and **Vectrix** is an existing electric-scooter company that is phonetically close. Neither blocks a repository or package name, but both are reasons a trademark search would be worth doing before any commercial use. | A trademark conflict surfacing later. Renaming is cheap while the repo is documentation only and gets steadily more expensive after that. **Superseded by D10 (Velk) on 2026-09-11.** |
| D10 | 2026-09-11 | **The project is named Velk**, superseding D9 | Coined, one syllable, no meaning in any major language — the owner's stated requirement. Verified free on npm and PyPI, and a brand/company search found nothing using it. `velk.dev` and `velk.app` are free; `velk.com` and `velk.io` are held, which is ordinary for a four-letter word and irrelevant to a repository or package name — accepted as a known risk. **`MicroFlow Simulator` was considered first and rejected on collision grounds** (`microflow` taken on npm and PyPI, ≥7 GitHub projects plus two orgs and a GitHub Topic, both obvious domains held) — do not re-propose it. | A trademark conflict, or the name proving so anonymous that people cannot find the project. Both are cheap to fix now and expensive once source code, packages and links exist. **Superseded by D11 on 2026-09-11.** |
| D11 | 2026-09-11 | **Keep the working name `TrafficSim`; defer naming until the end of M1** | The project was renamed three times in two days (TrafficSim → Veytrix → Velk) with several further candidate sets explored, and no code was written in that time. A name is far easier to judge against a working program than against a specification, and each further round costs a session without moving the project. Deferring also cancels work already queued: no GitHub repository rename, and no package or domain registrations to make and then undo. **Trigger to revisit: the end of M1**, when there is a working network editor to name. **Names already examined — start from these findings, do not re-derive them:** `Headway` rejected (`headwaymaps/headway`, an OSM maps stack, same field); `MicroFlow Simulator` rejected (`microflow` taken on npm and PyPI, ≥7 GitHub projects plus two orgs and a GitHub Topic, both obvious domains held); `Veytrix` set aside (`veytrix.com` held, `Vectrix` phonetically close); `Velk` set aside while clean on every channel checked (npm, PyPI, brand search; `velk.dev`/`velk.app` free) and therefore the strongest candidate to return to. | Drifting past M1 without ever deciding. The trigger exists to prevent exactly that. |
| D6 | 2026-09-10 | **Project spine written before any code** | Only what is on disk survives a session boundary. The rules in `PRINCIPLES.md` §3 were measured by a prior effort and would otherwise have to be rediscovered by paying for them again. | — |

| D12 | 2026-09-11 | **Implement the simulation core and network model together, including prerequisite tooling** | The owner explicitly requested both systems in this session. That supersedes the earlier toolchain-only Next and one-system scheduling guidance. The boundary still remains strict: model compiles a snapshot; core imports only its own modules. | If later features are pulled forward without a separate scope decision. M0 is still the boundary. |
| D13 | 2026-09-11 | **M0 uses a clearly labelled reduced Wiedemann-inspired longitudinal model; unsupported merges are rejected** | A small auditable prototype is enough to exercise the M0 architecture and queue/discharge behaviour. The published W74 safety-distance shape is an inspiration, not permission to claim a faithful W74/W99 implementation. Accepting merging paths without gap acceptance would silently invent unsafe right-of-way semantics. | If M0 plausibility fails, fix or replace the approximation before closing M0; do not remove the unvalidated marker. |
| D14 | 2026-09-11 | **Pin TypeScript 5.9.3 and the dependency lockfile** | The boundary guard uses the TypeScript compiler AST API, including type imports and dynamic imports. The initially resolved TypeScript 7 package lacks that API. Pinning the compatible compiler makes the guard executable, with deliberate negative tests. | When the guard is migrated to a supported replacement AST API and verified against the same forbidden-import fixtures. |

| D15 | 2026-09-11 | **C++20 throughout the application, Qt 6 Widgets desktop, CMake/CTest**; supersedes D3's initial stack, D4's web-first loop and D14's active TS tooling | The owner asked whether the whole program could move to C++, then authorized the proposed migration. Port the existing M0 core/model and harness together; preserve old source in Git and four frozen regression fixtures. Keep Qt/JSON outside the engine and existing modelling limitations explicit. This supersedes one-system scheduling guidance for the migration. | If behaviour diverges from the saved baseline or desktop controls cannot run, fix the port before M1. Cross-toolchain math uses an explicit tolerance; scientific fidelity still requires M6. |

| D16 | 2026-09-12 | **Implement M1.1–M1.3 together on the native C++ base** | The owner approved the editor plan and explicitly requested these three slices. This supersedes the earlier one-system scheduling and M0-only Next for this scoped work. Implement document/history, canvas/background and Link/Lane tools together with basic saving so drawings persist. Existing scientific and full M1 usability gates remain open. | If later connectors, demand or runtime behaviour are introduced without their own scope decision. |

| D17 | 2026-09-14 | **Continue M1 with the planned M1.4 Connector editor; preserve one version-1 geometry source** | The owner requested continued Network editor development. `Next` already called for M1.4, so this session implements that slice and retains M1.3.1's split guard. A lane-aligned cubic is sampled into editable polyline points, avoiding a second curve store and an unnecessary schema change. Referenced connectors may be reshaped but not retargeted; deleting one removes its affected routes and inputs atomically instead of inventing new paths. | If engineering workflows require persistent tangent handles or measured radius constraints, define their model/schema explicitly; do not claim the sampled curve provides those guarantees. |

---

## Log

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

---

### 2026-09-12 — native editor M1.1–M1.3 implemented (D16)

Added a version-1 ProjectDocument and a Qt-free command library. Every committed edit
validates a candidate before publishing it; a failed edit preserves both history and
the model. Undo/Redo keeps up to 100 document snapshots, persists the current revision
and ID counter in project files, and tracks the last saved revision. Embedded PNG data
is shared immutably between snapshots rather than copied for every gesture.

The independent Qt editor is available from the M0 window or `--editor`. It provides
metric grid/snap, pan/zoom/fit, single-link selection, point/link dragging, point insertion
and removal, lane count and individual widths, left/right driving side, opposite
carriageways, split links and an extra downstream pocket lane. Splits introduce a 0.2 m
continuity span with explicit lane connectors and remap existing routes. A link deletion
confirms its affected connectors/heads/routes/inputs and restores all of them on Undo.
Referenced lane removal is rejected. Connector endpoints reanchor on geometry edits.

Local background images are embedded, calibrated from two picked points and a known
real distance, positioned/rotated/scaled and given opacity. All background changes are
undoable. Basic Open/Save/Save As use a versioned JSON document and QSaveFile atomic
replacement; failed load/save preserves the current work. New/Open/Close ask about
unsaved changes. Native prompts and editor controls support English and Thai. The
inspector can be hidden, resized or detached. No new simulation behaviour was added.

Boundary enforcement now also rejects project-to-command/editor/Qt imports, with
negative fixtures. `docs/NETWORK_EDITOR.md` records operation, format and exact limits.
M1.3.1 explicitly tracks the unsupported signal-bearing-link split rather than moving
signal stationing silently. General connectors, tables, recovery and run handoff remain
M1.4–M1.7. Windows/macOS GUI execution and the owner's usability gate remain unverified.

**Verification:** GCC 13.3 / Qt 6.4.2 on Linux. Fresh isolated Debug and Release
builds passed all 13 CTest suites; a separate Qt-free build passed all 11 suites.
The native test executable contains 38 named cases (8 new editor model cases), and
Qt UI checks exercise actual mouse/keyboard drawing, dragging, insertion/removal,
pan/zoom, cancellation, per-lane widths, pockets, opposite carriageways, driving side,
confirmed deletion/Undo, image transforms/two-point calibration, Unicode paths,
failed save/load preservation, unsaved-work cancellation and Thai translation.
The existing four TS regression fixtures, deterministic core replay, CLI and M0
controls still pass. Architecture/negative checks, the 500-line limit and whitespace
checks pass. A Thai editor screenshot at 1000×760 was visually inspected. GUI behaviour
on Windows/macOS and GitHub-hosted runner execution are not claimed verified here.


### 2026-09-11 — native C++ migration implemented (D15)

Replaced the active TypeScript/Vite application with C++20 libraries for core, network,
scenario loading and evaluation, a native CLI, and a Qt 6 Widgets desktop harness.
CMake presets cover desktop, headless and Release. All executable developer checks
are now C++; JSON remains the catalog/locale/fixture format. The original application
is preserved at GitHub commit `70383db6ab884c718baef97a8ab81292fdc9d1b0`.

The port preserves fixed ticks, explicitly sequenced xorshift32 draws, canonical IDs,
source queues, upstream tails, red/amber stops and all unsupported-topology guards.
`SimState` is a value snapshot sharing a detached const scenario. Qt, JSON and I/O stay
outside the core. CLI diagnostics include engine/compiler versions, unfinished counts
and optional JSONL events. M0 fixture loading is read-only, not project persistence.

The Qt harness supports Run/Pause/Step/Reset, seed validation/reset, playback speed,
scenario loading and English/Thai switching. Bundled Noto Sans Thai (unmodified OFL 1.1
font with license) fixes missing Thai glyphs on minimal systems. Desktop file-dialog
paths use native wide paths on Windows. Manual screenshot inspection confirmed Thai
text and a queued crossing scene at 640 pixels wide.

**Verification:** GCC 13.3, Qt 6.4.2, nlohmann/json 3.12.0, CMake 4.4.3 on Linux.
The original 40 tests and production build passed before capture. Native tests include
30 named C++ cases, four frozen TS baseline seeds, full same-build event replay, core
and network safety/validation, strict JSON/seed handling and locale key agreement.
Debug and Release desktop builds passed all 11 CTest suites, including interactive
control actions and an entire desktop run matching CLI/baseline. Headless also built
and passed independently without Qt. Address/undefined-behaviour sanitizer tests passed;
LeakSanitizer was disabled because this container cannot inspect process tasks.
The architecture negative fixtures, 500-line check and `git diff --check` passed.
An installed CLI run from a different working directory found its adjacent data and
reproduced seed 42: 31 completed, 0 active, 0 pending, 0 safety clamps and mean delay
29.249359418430977 seconds. No performance or scientific fidelity claim is made.

Added GitHub Actions definitions for Linux desktop/headless/Release and Windows MSVC
headless builds. Windows desktop execution and macOS deployment have not been tested
in this Linux workspace. See `docs/BUILDING.md`, `docs/MIGRATION.md`, updated architecture,
simulation contracts and `tools/README.md` for setup and precise limitations.

**Status:** M0.1 technical migration checks passed on Linux. Owner M0 plausibility
acceptance remains open. No M1 editor, movement LOS, right-of-way model, calibration
or M7 installer is claimed complete. Next remains owner review, then one undoable link.

### 2026-09-11 — M0 simulation core and network model implemented

The repository previously contained documentation only. Added a strict TypeScript/Vite/
Vitest toolchain and lockfile, the directory skeleton, and an AST-based core dependency
guard that is tested against intentionally invalid imports.

**Network:** link/lane/connector authoring types; left/right driving-side lane geometry;
mid-link signal heads; geometry/reference/range validation; and a detached scenario
compiler. Junctions are not authored, and no second persisted network format was added.

**Core:** fixed timestep; explicit xorshift32 seed state; immutable snapshots and pure
steps; Poisson source arrivals with persistent external queues; reduced four-regime
following; fixed-time red/amber/green signals; route transitions with residual distance;
upstream vehicle-tail occupancy; and a streaming event interface. The final subinterval's
arrivals remain pending instead of disappearing at the run horizon.

**Integration:** a crossing scenario and vehicle/behaviour catalogs in data files; a
passive canvas harness with run/pause/step/reset, playback speed, seed reset and English/
Thai text; a headless CLI; and an explicitly unvalidated completed-trip delay diagnostic.
The engine still has no UI, model, I/O or wall-clock imports.

**Verification:** 40 automated tests cover replay (including a reference trajectory
fingerprint), pure stepping, source queues, conservation, signal timing, free acceleration,
red stops/green discharge, upstream tails, short connectors, invalid scenarios, authoring
geometry and the import boundary. Production type checking/build and the source-size
check pass. A Chromium 152 browser smoke test passed dev boot, single-step, run/pause,
seed reset, Thai translation, an entire run matching the headless output, invalid-seed
handling and a 375-pixel viewport with no horizontal overflow. No page errors occurred.
In a separate clean detached checkout, `npm ci --offline` (using the package cache),
all 40 tests, the production build, the headless example and file-size checks also passed.

**Reference run:** seed 42, 180 simulated seconds, 31 completed trips, 0 active and 0
pending at the horizon, 0 numerical safety clamps. Mean completed-trip delay is
29.249359418430977 s (includes source wait and acceleration, not HCM control delay).
This is a reproducibility fixture, not a capacity or fidelity benchmark.

**Limits remain explicit:** no lane changing, merge arbitration, geometric crossing-conflict
resolution, priority rules, full W74/W99, project persistence, movement LOS or batch
aggregation. Merges, internal inputs and repeated-route segments fail validation.
M0 remains open for the owner's plausibility acceptance. No later milestone was closed.

See D12–D14 and `docs/SIMULATION.md` for the reasoning and precise interfaces.

### 2026-09-11 — reverted to the working name TrafficSim, naming deferred (D11)

Three renames in two days with no code written. The owner called it: go back to the working
name and decide the real one once the program has shape.

Headings across `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, `ROADMAP.md` and this file are
back to `TrafficSim`, now explicitly marked as a working name so no future session reads it
as settled. D9 and D10 keep their full reasoning and are marked superseded — this file is
append-only, and the collision findings gathered over those rounds are the main thing worth
keeping from them, so they are consolidated into the D11 row. The next naming round starts
from evidence, not from zero.

**Two pieces of queued work are cancelled, not postponed:** the GitHub repository rename (the
repo is still `Warissakorn/trafficsim` and the remote already points there, so there is
nothing to do) and the npm/PyPI/domain registrations for `velk`.

**Two defects in this file were found and fixed while making this change**, both introduced by
earlier sessions of this conversation:

1. **The D10 log entry below was never actually written.** The edit that should have added it
   matched no text, and the guard around that edit only checked that *something* in the file
   had changed — which was true because other edits in the same batch succeeded. It has been
   reconstructed below from the commit message and the D10 row. Guards on edits to this file
   now assert an exact match count per edit.
2. **Entries were in oldest-first order**, contradicting this file's own header. Reordered
   newest-first. No entry text was altered.

Nothing about scope, architecture or the roadmap changed. D1–D8 stand.

### 2026-09-11 — renamed to Velk (D10)

*Reconstructed on 2026-09-11 — see defect 1 in the entry above.*

`Veytrix` replaced throughout the documentation. `velk` verified free on npm and PyPI with no
brand or company found using it; `velk.dev` and `velk.app` free, `velk.com` and `velk.io`
held — ordinary for a four-letter word and irrelevant to a repository or package name, so
accepted as a known risk.

`MicroFlow Simulator` was proposed first this session and dropped after its collision check:
`microflow` taken on npm and PyPI, at least seven GitHub projects carrying the name along
with two orgs and a GitHub Topic, and both obvious domains held. Recorded in D10 so it is not
raised again.

Also noted at the time: the MicroFlow brand write-up claimed "extends to both microscopic and
macroscopic" as a strength, which contradicts `PROBLEM.md` §5 where macroscopic assignment is
a non-goal. **§5 was left unchanged** — that is a scope decision, not a naming one.

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
