# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks to `PROGRESS-archive.md` whole if this gets
long. Older entries have been moved whole to [`PROGRESS-archive.md`](PROGRESS-archive.md).

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

### 2026-09-14 — hot path 4: pending-vehicle insertion stops rebuilding every span

The pending-vehicle loop called `occupiedSpans` over the whole vehicle list **once per
candidate**, which was 42% of all span construction (122,061 of 288,400 `appendSpans` calls in
the profile). Spans and their buckets are now built once per tick and extended in place:
`appendVehicleSpans` adds exactly the spans a full rebuild would have appended for the newly
inserted vehicle, and the bucket fill is stable, so the grown structures are identical to what
a rebuild produced. Equivalence again rests on order, not on arithmetic.

**A first attempt built the structures unconditionally before the loop and was measurably
worse on small networks** — +21% at one corridor, +16% at four — because most ticks have no
arrival at all and previously did no span work whatsoever. Building lazily, only once a
candidate has survived the source filter, removes that cost: small networks return to parity
(+0.3% to +1.3%, inside run-to-run noise) and large ones keep the gain. The regression and its
cause are recorded here because the obvious eager version looks correct and is not.

**Measured** (Release, GCC 13.3, median of 5, identical commands):
0.028/0.049/0.117/0.292/0.670/**1.499 s** for 1/2/4/8/16/32 corridors — **-29.6% against the
previous slice at 466 vehicles**. Growth is now about **O(V^1.2)**.

Behaviour unchanged: 12/12 headless CTest including the trajectory digest and the exact CLI
value pin, plus 12 multi-seed multi-size CLI runs byte-identical to the pre-session binary.

**Verification:** headless preset only; Qt absent, so no desktop verification is claimed.

### 2026-09-14 — PROGRESS.md split, oldest entries archived

`PROGRESS.md` reached 513 lines and failed the 500-line budget (hard rule 6). Following this
file's own instruction, the naming-era entries of 2026-09-10/11 were moved **whole** into a new
`docs/PROGRESS-archive.md`; nothing was edited or summarised. `Next`, the backlog, the open
questions and the decision table all stay here, so a session with no memory still reads one
file to rejoin the work. The naming history those entries carry is already summarised in the
D9-D11 rows, which were not moved.

### 2026-09-14 — hot path 3/3: scenario lookups resolved once per tick

After the first two slices, string handling was still about half of all instructions, almost
all of it `detail::byId` doing a linear scan with an `std::string` compare per element. The
fix is to call it far less often rather than to make it cleverer:

- `resolveRefs` resolves each vehicle's route, type and behaviour **once per tick** into
  indices, replacing roughly six lookups per vehicle across the step loop, `occupiedSpans`
  and `locateVehicle`.
- `ScenarioIndex::routeHeads` precomputes, per route, the signal heads actually on it with the
  station of the first matching part — replacing an `std::find_if` over route parts with a
  string compare, run per head per vehicle per tick.
- `ScenarioIndex::programOfHead` plus a per-tick `headColors` vector evaluates each head's
  colour once per tick instead of once per head per vehicle; colour depends only on the tick's
  time, so every vehicle was recomputing the same answer.
- `locateOnParts` lets the step loop reuse the parts it already holds instead of looking the
  route up again.

Equivalence rests on order again: `routeHeads` is built in `signalHeads` order and records only
the first matching part, so each vehicle sees an identical sequence of heads and stations, and
the `allowedDistance`/leader updates fold in the same order as before. `byId` itself is
unchanged and still linear; it is simply no longer on the per-vehicle path.

**Measured** (Release, GCC 13.3, median of 3, identical commands):
0.028/0.048/0.117/0.298/0.740/**2.130 s** for 1/2/4/8/16/32 corridors — **-43% against the
previous slice at 466 vehicles**. Growth is now about **O(V^1.3-1.5)**.

**Cumulative for the three slices: 17.724 s -> 2.130 s at 466 vehicles, -88%**, and
0.067 s -> 0.028 s on the single-corridor case. Total instruction count on the profiling
scenario fell from 3.31 G to well under 1 G.

Behaviour unchanged throughout: 12/12 headless CTest including the trajectory digest and the
exact `29.24935` CLI pin, plus 12 multi-seed multi-size CLI runs byte-identical to the
pre-session binary at every slice.

**Verification:** headless preset only; Qt absent, so no desktop verification is claimed.

### 2026-09-14 — hot path 2/3: leader search grouped by segment

`closestVehicle` scanned every occupied span for every route part of every vehicle on every
tick, rejecting non-matching ones with an `std::string` segment comparison. That nested scan
was 39% of total instructions and carried the quadratic growth term.

Spans are now grouped by segment in a flat CSR layout (`start` offsets plus an `items` index
array), so a vehicle only ever looks at spans on the segments its own route actually uses.
`RoutePart` and `OccupiedSpan` carry a resolved `segmentIndex`, recovered during index
construction from the segment's address in the contiguous `segments` vector, so grouping needs
no string hashing. CSR rather than a vector-per-segment keeps this to three allocations
instead of one per segment, which matters because the pending-vehicle loop regroups per
candidate.

**Order is the correctness argument.** The bucket fill is stable, so each segment's spans keep
their original relative order, and the outer loop still walks route parts in order. The set and
sequence of spans that survive to the `gap < nearest->gap` test is therefore exactly what the
full scan produced, and that strict comparison keeps first-encountered-wins tie-breaking
unchanged. The dropped `span.segmentId != part.segmentId` test is now implicit in the bucket.

**Measured** (Release, GCC 13.3, median of 3, identical commands):
0.031/0.058/0.152/0.433/1.196/**3.731 s** for 1/2/4/8/16/32 corridors — **-67% against the
previous slice at 466 vehicles, -78.9% against the session baseline of 17.724 s**. Growth fell
from O(V^1.85) to about **O(V^1.6)**. The residual superlinear term is the per-candidate
`occupiedSpans` rebuild and the per-vehicle signal-head scan, both untouched here.

Behaviour unchanged: 12/12 headless CTest including the trajectory digest and the exact CLI
value pin, plus the same 12 multi-seed multi-size CLI runs byte-identical to the pre-session
binary.

**Verification:** headless preset only; Qt absent, so no desktop verification is claimed.

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
