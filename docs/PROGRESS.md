# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is the history and the reasoning** — what a session
reads to understand why the code is the way it is. What to do next is in
[`NEXT.md`](NEXT.md); the [decision log](decisions/RECORD.md) has its own indexed record. Never delete an entry;
move old blocks whole into `docs/archive/` if this gets long, and list each in
[`archive/README.md`](archive/README.md), which indexes every older entry.

---

## 2026-10-08 — Evaluation period: author-declared warm-up and window, M5.4 (D139)

The owner answered O8: a warm-up and window the user defines. Schema 23 adds
`definition.evaluationPeriod {warmup, end}` (both required, on the grid, within the run); Run
settings edits it in one History step with duration and dt. Movement figures, the run-level
mean delay and queues then cover trips arriving in (warmup, end] and states sampled there;
completed, clamps and lane changes stay whole-run and `outsideWindow` closes the count. No
period: byte-identical outputs and schema. On the four-leg project a 300 s warm-up raises mean
delay from 50.1 to 52.7 s over ten seeds ([evidence](evidence/evaluation-period.md)). On O3 the
owner asked whether HCM belongs at the analytical level; recommended: HCM as M6's computed
reference, not letters on simulated delay. New `evalperiod` group; both failure-first mutants fail it.

---

## 2026-10-08 — "Run N seeds" in the editor, M5.3 (D138)

ROADMAP S2. Simulation ▸ Run N seeds… compiles the document as Run does and starts one worker
thread on a copy of the scenario; it calls the CLI's `runSeeds`/`aggregate` seed by seed, so a
new *Seeds batch* tab in Results equals `trafficsim-cli --seeds` (tested to `batchCsv` bytes).
The tab shows progress and no rows until every seed has finished. Cancel and any document change
drop the job: it stops after its current seed (1.5 s in Debug), cannot publish, and its thread
is joined when it finishes or at close. The Results table helpers moved to `result_table.hpp`.
New `batch-run-ui`; removing both the stop flag and the publish guard fails it
([evidence](evidence/editor-batch.md)). No engine change; CLI bytes unchanged.

---

## 2026-10-08 — Multi-seed batches from one command, M5.2 (D136, D137)

The owner asked for a recommendation and chose the batch runner, answering ROADMAP O1 with (a).
`src/runner` runs a compiled scenario once per seed with the single-run loop and aggregates per
movement and approach: n, mean, SD and the 95 % Student-t half-width, sorted by seed so any
seed order gives the same bits, n counting only seeds with a value. `trafficsim-cli --project F
--seeds A-B [--csv F]` prints it with one accounting row per seed; ten four-leg seeds take 1.3 s
(Release). A new architecture rule keeps the runner to core and eval. CLI output now names the
configured commit; otherwise single-run bytes are unchanged. New `runner` group: hand-computed
statistics, t quantiles, permutation, one-seed batch = single run, accounting; zero-filling
absent delays fails it. LOS, the editor's "Run N seeds" and copy/export remain.

---

## 2026-10-08 — The UI catches up: W74 editing, queue discharge and clamps in Results (D134, D135)

The owner asked for the UI to match the engine and chose all three gaps. **D134:** the
behaviour editor (now `src/shell/behaviour_editor.cpp`) has a *Model* choice and one text
field per W74 key. A key with no value starts empty and must be entered, so no value comes
from code; Confirm runs `w74ParameterIssues` and stays open on the first issue. Going back to
the prototype takes its keys from the library's first prototype behaviour. **D135:** Results
has inner tabs. *Queue discharge* uses the CLI's default spec, with means over the greens
that gave an estimate and the reasons for the others. *Safety clamps* has one row per event
(time, vehicle, type, route, segment). New `results-tabs-ui` checks the discharge rows against
an independent observer on the four-leg project (seed 42, 900 s) and the clamp rows against
the run's count. Removing the range check or the discharge feed fails a test. No engine
change; CLI bytes unchanged.

---

## 2026-10-08 — The extra W74 clamps traced: no W74 defect

Evidence only; no engine change. `tools/w74_clamp_trace.cpp` attributes every safety clamp
of the BA27 runs (2,053, all moving) to the obstacle setting the smallest allowance, from the
tick's rebuilt snapshot, and records each amber onset. All 240 runs match BA27's counts; none
is unexplained. 83–96 % are amber heads, held as red with no commitment test: W74's slower
fixture discharge leaves a queue at 16–19 % of amber onsets (prototype under 0.1 %), so more
onsets catch a vehicle that cannot stop. The rest are the standstill cap treating a moving
leader as standing (D105's recorded case), met by W74's gentle emergency braking near `AX`.
Neither rule changed (BA18); NEXT §2 holds it as an owner decision. The sweep now shares its
document setup with the trace and stays byte-identical. See [evidence](evidence/w74-clamps.md).

---

## 2026-10-08 — W74 timestep record, BA27, and the courtesy fixture

Evidence only; no engine change. `tools/w74_discharge_sweep.cpp` runs the four-leg project,
prototype and `w74` (fixture values in `evidence/w74-discharge-behaviour.json`: three are
PTV's W74 defaults, fifteen are uncited), at dt 0.1/0.25/0.5, seeds 42–81, with the CLI's
discharge spec. Its prototype arm at dt 0.1 seed 42 equals the Debug CLI in all 90 cycles,
and a rerun is byte-identical. Mean headway rises with dt in both models, about twice as
much for W74; W74 runs clamp more (9.4 → 14.8 per run), which NEXT now asks to trace before
any preset. A `w74run` case checks that a winning courtesy hold stores its own state; a
mutation keeping the leader's state fails it. See [evidence](evidence/w74-discharge.md).

---

## 2026-10-08 — W74 composed into the tick, BA23–BA25/BA28 (D133)

Fourth M3.3.3a slice: W74 now runs. `follow` dispatches by `DriverBehaviour::w74`;
`standstillGap`/`desiredGap` replace every direct prototype-field read in the tick (source
and D108, motion, second obstacle, lane-change checks, dead end, discretionary gain,
waiting room, receiving space, `stopLineReach`). Leader acceleration rides on
`OccupiedSpan` and `CourtesyHold`; static obstacles write 0. `Vehicle::w74State` comes from
the kept result at publish. D131's Run refusal and its message are gone. A first run-level
Stop test passed without the standstill override; the §7 case (at rest 3.3 m short) was
added and now fails without it. Prototype bytes unchanged. Headless 69/69, desktop 100/100
(Linux). See [evidence](evidence/w74-composition.md). Not validated.

---

## 2026-10-08 — W74 driver traits, BA26 (D132)

Third M3.3.3a slice. `w74Traits(seed, id, driverFactor)` implements §5's splitmix64 hash and
Irwin–Hall `zOp`; `generateArrivals` stores it on `PendingVehicle` when any behaviour is
`w74`. Golden values match an independent Python implementation bit for bit, and GCC and
Clang agree on 1,000 traits. A mixed run keeps every prototype draw. Writing the
`upcomingArrivals` row exposed that its scratch state did not copy `seed`; it does now.
`W74State` and BA24/BA25 are deferred to composition, their only writer and reader (D132).
Checkpoints gain `w74Traits` only when present. Headless 68/68, desktop 99/99 (Linux),
CLI seed 42 identical. See [evidence](evidence/w74-traits.md).

---

## 2026-10-08 — W74 schema-22 codec, BA29 (D131)

The second M3.3.3a slice, codec only by the owner's choice. `DriverBehaviour::w74`
(`std::optional<W74Parameters>`) is the model tag; the W74 value types moved into
`types.hpp` so it can hold them. `parseBehaviour` dispatches on `model`, refuses each
model's keys on the other, and reads the 18 keys from `w74ParameterKeys()`, the same table
the serializer and §5 range check use. Schema 22 is written only when an owned behaviour is
`w74`; below 22 one is `UNSUPPORTED_BEHAVIOUR_MODEL`. Run refuses a `w74` behaviour a
vehicle type or road selection uses (`UNSUPPORTED_BEHAVIOUR_MODEL_RUN`), never runs it as the
prototype; an unused one changes nothing. The dialog shows a `w74` behaviour read-only
except its name. Why each choice: D131. Qt was installed in this container, so desktop
UI suites ran (Linux offscreen): headless 67/67, desktop 98/98, CLI seed 42 and shipped
projects byte-identical. See [evidence](evidence/w74-codec.md).

---

## 2026-10-08 — W74 pure function, BA21–BA22 (D130)

Review first: a clean checkout configured only after installing `nlohmann-json3-dev`
(BUILDING lists it); headless was then 65/65. The decision record's table had blank lines
between rows from D12 onwards, so GitHub rendered every later row as plain pipe text; the
blank lines are removed in a separate commit, no row changed.

Then the first M3.3.3a code, rows-first as NEXT ordered: `src/core/w74.hpp/.cpp` implement
contract §3 thresholds, the §4 seven-row classification and accelerations, the §7 sign and
the §6 bounds (type clamp, then D105) as one pure function. `Leader` gains `acceleration`,
0 everywhere until BA28 fills it; the prototype ignores it. `tests/w74_tests.cpp` (group
`w74`) checks every regime row against hand values and every equality side on the value
and one ULP away; five seeded boundary mutations each fail it. Why a pure, unwired function
first and why the bounds live inside it: D130. Headless 66/66, CLI seed 42 byte-identical;
Linux only. See [evidence](evidence/w74-pure-function.md).

---

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

---

## 2026-10-07 — W74 car-following contract (D129)

[`reference/W74.md`](reference/W74.md) writes the M3.3.3a contract NEXT asked for before any
W74 code: net-gap thresholds (AX, BX, ABX, SDX, SDV, CLDV, OPDV, DMAX), a seven-row regime
table with every equality side named, per-regime accelerations, 18 required behaviour keys,
driver traits, `W74State`, model switch, composition with the hard cap/D105/D108/Stop, and
schema 22. Rows BA21–BA29 are in the delivery plan. Docs only; no code, schema or behaviour
change. Sources are public: PTV's W74 parameter page for vocabulary and SUMO's open
`MSCFModel_Wiedemann` (citing Olstam & Tapani 2004) for the decision tree; neither is a
parity target. Why the choices (D129): PTV hides the internal constants, so each is a key
rather than a code default; traits come from a splitmix64 hash so the run stream, frozen
baselines and prototype vehicles in mixed scenarios keep their draws, and an Irwin–Hall
normal avoids `log`/`cos` cross-compiler drift; `driverFactor` already is PTV's `z`. The
following sign is hysteresis (−1 after approaching/emergency, +1 after free), the only
state W74 needs. Not resolved here: preset values, timestep sensitivity (BA27 records it).
A self-review then fixed six defects before any code: a vehicle resting in the
following band with `s = −1` behind a static obstacle never moved again and could
miss its Stop service (now `s = +1` at standstill); `Leader` has no acceleration,
so the contract now adds and fills it everywhere; the stored regime with a second
obstacle is the kept result's; `bxAdd > 0` prevents `BX = 0`; the hash's modulo
and summation order are fixed; BA23/BA26/BA28 now state the emergency start-up
delay, the real mixed-scenario guarantee and the new cases.

---

## 2026-10-07 — BA14/BA17 focused fixtures

Six `behaviourselection` cases in `tests/behaviour_selection_edge_tests.cpp` close the two
rows D127 left "by construction": selection follows the recognized route at source-zero,
mid-Link and consecutive lines and at a line just short of a join, with routing draws
unchanged; a served Stop keeps its service while the front crosses into a tighter set,
composes with the denied area and the receiving queue (read under the current set), and a
genuine clamp is still reported. Tests only; no engine change. Fixture obstacles need
desired speed 0, or a placed "standing" vehicle drives away. Four mutations caught; the
receiving-queue case survived one until its room was narrowed below the legacy set's need.
See [evidence](evidence/behaviour-selection.md). Headless 65/65 (Linux, GCC 13.3).

---

## 2026-10-07 — Behaviour library editor and road assignment UI (D128)

M3.3.2c adds the *Driving behaviours* dialog (Behaviours / Vehicle classes / Link
behaviour types; Add, Duplicate, Edit, Delete with replacement) on a staged copy that
commits once, and the inspector's *Behaviour type* for every selected Link and
Connector with each vehicle type's effective behaviour and source. The precedence is
now one shared function, `effectiveRoadBehaviours`, used by compiler and inspector.
See [contract §9](reference/DRIVING_BEHAVIOUR.md#9-implemented-editor-m332c-d128) and [evidence](evidence/behaviour-editor.md).

Headless 65/65 and desktop 96/96 CTest (offscreen) pass; the new `behaviour-library-ui`
test is stable over six runs and fails under four seeded UI mutations. CLI output is
unchanged. Windows UI jobs and the owner's visual review are separate gates.

---

## 2026-10-07 — Front-segment behaviour selection (D127)

M3.3.2b compiles Link/Connector assignments into `segmentBehaviours` (ids; class
override else default; every section and Connector path inherits its owner) and
selects every consumer's behaviour through `effectiveBehaviour` by the front's
segment, sharing `locateVehicle`'s boundary rule. Without assignments the index
table is empty and the legacy type slot is used unchanged. D126's Run refusal is gone.
See [contract §8](reference/DRIVING_BEHAVIOUR.md#8-implemented-runtime-selection-m332b-d127) and [evidence](evidence/behaviour-selection.md).

Headless 65/65 CTest including frozen TS baselines; six `behaviourselection` cases,
six of seven mutations caught (the unneeded canonical sort was removed). Seed-42 CLI
output equals a `main` Release build for the four projects; benchmark per vehicle-tick
unchanged within noise. BA14/BA17 focused fixtures remain open.

---

## 2026-10-07 — Project-owned behaviour library and road assignment storage (D126)

M3.3.2a adds schema 21: model-tagged owned behaviours with names, vehicle classes,
link behaviour types (default + per-class overrides) and a `behaviourType` on Links
and Connectors, plus `behaviour_commands` for put/duplicate/assign/users/delete with
replacement. Owner decisions: the library needs project-owned catalogs, and Run
refuses an assigned road until M3.3.2b. See [contract §7](reference/DRIVING_BEHAVIOUR.md#7-implemented-library-and-codec-m332a-d126)
and [evidence](evidence/behaviour-library.md).

Headless CTest passes with 7 `behaviourlibrary` cases (BA06–BA09, Run refusal,
EN/TH codes); seven targeted mutations each fail a case. Shipped projects re-save
unchanged and their seed-42 CLI output is byte-identical. No engine or catalog data change.

---

## 2026-10-07 — Rank-scoped remap invalidation and source-sink identity (D125)

M3.3.1b2b2 completes measurement. A remap invalidates a cycle only when it moves a
vehicle queued at Go while ranks 1..steadyLast are open; unqueued changers are
already caught by `queue_not_sustained`. Same-tick source sinks get their exact type
from core's new pure `upcomingArrivals` replay plus a route/scheduled-time/speed
fingerprint. Overlapping lateral spans stay ambiguous (engine choice depends on
safety checks). See [contract](reference/DISCHARGE.md) and [evidence](evidence/discharge-identity.md).

Headless CTest 63/63 on GCC 13.3; the BA05 forcing case fails without the replay.
Seed-42 legacy JSON and manifests match the parent; usable cycles 20→30 (four-leg)
and 91→113 (m2.6). Engine, schema and frozen fixtures unchanged. Native CI separate.

---

## 2026-10-07 — Proven source and lateral passages (D124)

M3.3.1b2b1 replays unique start-of-tick lane maps from prior front/type positions;
terminal survivors/sinks establish longitudinal passage, including insertion ticks.
Lateral jumps never count. Upstream membership changes invalidate estimates;
ambiguous maps/terminals and source sinks without type evidence remain unavailable.
No display traces, engine/schema changes or frozen fixture regeneration. See [contract](reference/DISCHARGE.md) and [evidence](evidence/discharge-passage.md).

The original observer fails 5 of 8 new forcing fixtures. The final suite passes
85 GCC/Linux tests; real source/lateral copies preserve dynamics at .1/.2 s.
Four seed-42 project legacy JSON/stop-line outputs and input manifests match the parent. Guards pass; local CMake/Ninja/Qt are absent. Parent #119 workflow 458
passed Linux/Windows; candidate CI is independent. BA05, M0/M6 and owner gates stay open.

---

## 2026-10-06 — Captured discharge inputs and physical-prefix recognition (D123)

M3.3.1b2a emits parsed-byte SHA-256/size/read counts for project, actual catalog
reads and queue definitions, with logical names and optional fallback scopes.
Changed repeated reads reject before stepping; no hashing/I/O reaches core/eval.
Positioned suffix choices on the same physical prefix retain queue membership;
diverted/lateral remaps suppress raw old-head crossing inference and stay unavailable.
Complete lateral/source reconstruction remains M3.3.1b2b; BA05 does not close.
See [contract](reference/DISCHARGE.md) and [evidence](evidence/discharge-provenance.md).

Validation: 72 focused/relevant legacy GCC/Linux tests pass, including frozen
reference fixtures; seeded CLI/hash parity and guards pass (see evidence). Parent PRs #117/#118 pass
native CI; the candidate's native/desktop CI is separate. CMake/Qt are absent locally.
No engine/schema/frozen data changed; no owner or calibration gate closes.

Follow-up: workflow 457 passed Linux but both Windows jobs failed the LF-only fixture hash.
The test now reads actual checkout bytes; controlled LF/CRLF JSON retains distinct hashes.
The old assertion fails on a CRLF copy; the corrected suite passes 73 Linux tests and
7 CRLF-copy provenance tests. See evidence; fresh native Windows CI remains required.

---

## 2026-10-06 — Declared discharge windows and type selection (D122)

M3.3.1b1 exposes CLI windows/warmup/ranks and repeatable vehicle-type selection.
Headways retain original follower/predecessor pairs and ranks; filtered samples
cannot bridge skipped types. Raw crossings stay intact. A mixed startup prefix
has a separate unavailable reason without discarding valid selected headways.
Unknown types and invalid/orphan controls reject before stepping the engine.
Input-hash output and complete remap/source passage tracking remain M3.3.1b2.
See [contract](reference/DISCHARGE.md) and [evidence](evidence/discharge-controls.md).

Validation: 15 focused GCC/Linux tests pass, including the existing trajectory
comparison. CLI integration and repository guards are recorded in the evidence.
Native/desktop Linux/Windows CI remains independent; no owner/calibration gate closes.

---

## 2026-10-06 — Queue discharge and startup measurement (D121)

M3.3.1a adds a stdlib-only observer, pure rank estimator and CLI `--discharge` JSON.
Lane/cycle records retain type identity and queued-at-Go membership; tracked sink
arrivals are counted. Windows, ranks, timestep, signed startup estimates and
unavailable reasons are explicit. StopLine output and engine code are unchanged.
BA03 class filters and BA05 complete remap/source support remain M3.3.1b; neither
measurement nor owner/calibration gates close. See [contract](reference/DISCHARGE.md)
and [local test evidence](evidence/discharge-measurement.md).

Validation: 10 GCC/Linux tests pass; direct full CLI link and seeded JSON parity;
documentation, architecture and file-size guards. CMake/Ninja/Qt are absent; native/desktop
CTest must run in CI. No frozen baseline was regenerated.

---

## 2026-10-06 — Driving behaviour contract before implementation (D120)

Owner authorized the staged plan following the supplied Driving Behavior design.
M3.3.0 records class/default/legacy precedence, compiler/runtime ownership,
front-segment tick selection, source and D119 routing integration, shared catalog
lifecycle, and the capability map separating prototype support from W74/W99/lateral
work. BA01–BA20 are pending failure-first rows, not passed runtime tests. Measurement
specifies lane/cycle/rank headways and startup estimation before changing diagnostic
code; presets and PCU target ranges remain unvalidated. No engine/schema/UI changed.

Validation: baseline and edited docs/navigation, file-size and architecture guards
compiled/run directly with GCC on Linux; all passed. `git diff --check` passed.
CMake/Ninja are absent on this host, so native/desktop CTest was not run locally;
CI is required independently. Existing prototype fixtures are unchanged. No owner,
M0 or M6 gate closes; current resolver remains type-based.

---

## 2026-10-06 — Routes recognized at clicked Link stations (D119)

Owner authorized choosing a destination when vehicles reach the first Route click.
An optional RoutingDecision.position stores a reference-polyline station; positioned
documents opt into schema 20, while absent positions retain legacy demand-time booking.
The Route gesture creates/reuses the station atomically with its traced Route. A shared
crossbar previews and selects the point; dragging commits one Undo step. The dialog can
opt existing decisions in. Link inputs use the decision; explicitly assigned Route inputs
retain their assignment. Pending demand preview destinations are labelled as deferred.

Neutral provisional route families preserve source volume and lane shares, retain all
period/type alternatives and distinguish different traces to the same end Link. At passage,
the core draws once using seeded RNG and passage-time half-open interval weights, records
the decision on the vehicle and emits a routing event. Compatible physical prefixes retain
distance and lane; generated lateral awareness starts at the station. Legacy downstream
bookings remain intact. Decisions in active conflict reservation spans are refused.

Recognition caps the crossing tick's displacement at the line and retains computed speed;
the selected suffix receives normal safety checks on the next tick. Excess proposed travel
is discarded, so recognition time is quantized by dt; whole-trip timing impact is unmeasured.
The initial slice retains one decision per Link and excludes Connector stations. The
contract and independent M2.1.3 platform/owner gates document these limits. Oldest complete
D105/D106 progress entries moved to the indexed archive to keep this file near 500 lines.

Validation: GCC 13.3/C++20 and Qt 6.4.2 desktop build on Linux. All **91/91 CTest groups**
pass after updating the existing future-schema rejection example from 20 to 999 and
rerunning that affected group. New core/compiler coverage includes nine station cases;
Qt offscreen coverage verifies gesture, overlay clipping, grouping, drag/cancel, Undo and
save/reopen. Frozen references and architecture/file-size/documentation guards pass.
Native Linux/Windows CI and owner appearance/fidelity remain separate gates.

---

## 2026-10-06 — Mixed conflict sites and P3–P4 continuation (D118)

Owner authorized grouping all three kinds for a connected owner-pair site, 0.5 m per-side
rail offsets and directional continuation through P3–P4. Groups now retain their full kind
list; Crossing/Merge share controls by owner ID, Branching stays derived/read-only. The
mixed table/dialog identifies it separately from editable parameters. Direct group commands
stage and validate the candidate and reject newly introduced merge-order cycles atomically;
History still publishes one Undo step. Merge takeover preserves complete topology controls.
Offsets are normal to rails, capped at 20% of normal local width for narrow/tapered lanes.
Shared band outlines feed paint/picking. Valid mouth caps are clipped to their attached Link
lanes; cap pieces support physical grouping and finite Link stations support directional
continuation. Caps can lie on either side of a join, so no source/target station-side clamp
is imposed. Stored entryStation–exitStation, schema, runtime paths and solver remain unchanged.
M3.2.4f is carved separately; physical-mouth admission remains M3.2.4e. Regression coverage
adds both traffic sides, oblique offsets, mixed real sites, atomic rejection, geometry Undo,
and Qt display/picking/one-Undo checks. Oldest roadmap session block moved whole to archive.
Validation: GCC 13.3/C++20 and Qt 6.4.2 on Linux; full local desktop CTest passes
**89/89 groups**, including Qt offscreen UI, frozen references and repository guards.
Native Linux/Windows CI and owner appearance remain separate; no owner/fidelity gate closes.

---

## 2026-10-06 — Conflict bands follow driving lanes with rail offsets (D117)

Owner refined D115 display to separate directional lane bands, with appropriate offset
from road edges. Exact intersection polygons and mouth classification are retained for
measurement/grouping; `conflictAreaGeometry` also exposes per-side measured station spans.
Canvas bands follow the painted rails through those spans, inset laterally by 0.30 m per
side capped at 20% of local width. Longitudinal cuts remain unchanged, keeping short
mouths visible. Picking uses the same visible band union. Merge bands use measured mouth
spans rather than the unchanged one-metre runtime admission extents. No schema or solver
rule changes. Updated oblique UI tests require two distinct directional bands and blank
offset margins to be unpickable; narrow 0.5 m lanes test adaptive offsets. Model regressions
compare merge band spans directly with measured Connector/Link mouth stations.
The oldest docs-tidy history block is moved whole into archive to retain live-file headroom.
Validation: GCC 13.3/C++20 and Qt 6.4.2; the complete local desktop CTest suite passes
**89/89 groups**, including Qt offscreen UI, frozen references and repository guards.
Native CI and owner desktop appearance remain separate; no owner/fidelity gate closes.

---

## 2026-10-06 — Road crossbars and hover-traced Routes (D116)

The owner requested a simpler common appearance: Routes, Vehicle inputs and signal
heads are lines across the road. A shared normal/rail-intersection helper places
these bars on actual road edges, including attached Link rails at Connector mouths.
Cosmetic pens keep the line readable at zoom; a wider hit area keeps it selectable.
Inputs mark contiguous served lanes and skip zero-share lanes. Edit heads are neutral
with a contrasting casing; Run heads reuse their stop geometry and show program colours.
Reset restores the Edit bars. Selected Routes tint their clipped road surfaces and
mark both ends, preserving D96's partial-Link rendering.

The Route tool now accepts click start, hover through the chosen branch, click destination.
Hover remembers a valid lane-connected chain without changing the document; revisiting
an earlier road trims it. Ambiguous/unreachable extensions are rejected. Backspace,
Escape, focus/tool/level changes and the existing Ctrl+right/Enter flow remain supported.
The destination creates one command, so one Undo removes the whole Route. English/Thai
hints and input labels describe the new interaction. Core behaviour and schemas are unchanged.

Validation: Linux Debug build with GCC 13.3 / Qt 6.4.2; offscreen regression coverage for
curves, both driving sides, Connector mouths, zoom/picking, zero-share inputs, branched
tracing, cancellation, one-command Undo and Edit/Run signal geometry. All 89 tests passed
across the full run and five affected-test reruns after fixing adjacent-lane hit spill;
architecture, file-size, documentation and whitespace checks passed. Native Windows CI
and owner appearance review remain separate; no simulation or owner gate is closed.

---

## Backlog (M0, in order)

The [historical checklist](archive/PROGRESS-M0-backlog-and-questions.md#backlog-m0-in-order)
is archived. Current work lives in [NEXT](NEXT.md); milestone gates live in [ROADMAP](ROADMAP.md).

## Open questions

The [historical register](archive/PROGRESS-M0-backlog-and-questions.md#open-questions)
is archived. Use [NEXT — Owner decisions](NEXT.md#2--owner-decisions) for current questions.

## Decisions

The complete [decision record](decisions/RECORD.md) now lives beside its
[topic index](decisions/README.md). This heading remains as a compatibility target
for existing `PROGRESS.md#decisions` links. Add new decisions to RECORD, not here.
