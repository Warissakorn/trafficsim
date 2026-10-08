# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is the history and the reasoning** — what a session
reads to understand why the code is the way it is. What to do next is in
[`NEXT.md`](NEXT.md); the [decision log](decisions/RECORD.md) has its own indexed record. Never delete an entry;
move old blocks whole into `docs/archive/` if this gets long, and list each in
[`archive/README.md`](archive/README.md), which indexes every older entry.

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

## 2026-10-08 — W74 composed into the tick, BA23–BA25/BA28 (D138)

Fourth M3.3.3a slice: W74 now runs. `follow` dispatches by `DriverBehaviour::w74`;
`standstillGap`/`desiredGap` replace every direct prototype-field read in the tick (source
and D108, motion, second obstacle, lane-change checks, dead end, discretionary gain,
waiting room, receiving space, `stopLineReach`). Leader acceleration rides on
`OccupiedSpan` and `CourtesyHold`; static obstacles write 0. `Vehicle::w74State` comes from
the kept result at publish. D136's Run refusal and its message are gone. A first run-level
Stop test passed without the standstill override; the §7 case (at rest 3.3 m short) was
added and now fails without it. Prototype bytes unchanged. Headless 69/69, desktop 100/100
(Linux). See [evidence](evidence/w74-composition.md). Not validated.

---

## 2026-10-08 — W74 driver traits, BA26 (D137)

Third M3.3.3a slice. `w74Traits(seed, id, driverFactor)` implements §5's splitmix64 hash and
Irwin–Hall `zOp`; `generateArrivals` stores it on `PendingVehicle` when any behaviour is
`w74`. Golden values match an independent Python implementation bit for bit, and GCC and
Clang agree on 1,000 traits. A mixed run keeps every prototype draw. Writing the
`upcomingArrivals` row exposed that its scratch state did not copy `seed`; it does now.
`W74State` and BA24/BA25 are deferred to composition, their only writer and reader (D137).
Checkpoints gain `w74Traits` only when present. Headless 68/68, desktop 99/99 (Linux),
CLI seed 42 identical. See [evidence](evidence/w74-traits.md).

---

## 2026-10-08 — W74 schema-22 codec, BA29 (D136)

The second M3.3.3a slice, codec only by the owner's choice. `DriverBehaviour::w74`
(`std::optional<W74Parameters>`) is the model tag; the W74 value types moved into
`types.hpp` so it can hold them. `parseBehaviour` dispatches on `model`, refuses each
model's keys on the other, and reads the 18 keys from `w74ParameterKeys()`, the same table
the serializer and §5 range check use. Schema 22 is written only when an owned behaviour is
`w74`; below 22 one is `UNSUPPORTED_BEHAVIOUR_MODEL`. Run refuses a `w74` behaviour a
vehicle type or road selection uses (`UNSUPPORTED_BEHAVIOUR_MODEL_RUN`), never runs it as the
prototype; an unused one changes nothing. The dialog shows a `w74` behaviour read-only
except its name. Why each choice: D136. Qt was installed in this container, so desktop
UI suites ran (Linux offscreen): headless 67/67, desktop 98/98, CLI seed 42 and shipped
projects byte-identical. See [evidence](evidence/w74-codec.md).

---

## 2026-10-08 — W74 pure function, BA21–BA22 (D135)

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
first and why the bounds live inside it: D135. Headless 66/66, CLI seed 42 byte-identical;
Linux only. See [evidence](evidence/w74-pure-function.md).

---

## 2026-10-09 — M5.5 LOS letters from section delay (D134)

Section delay now gets a letter, and only section delay. The owner chose an author-set control
type (`signalised`/`unsignalised`, schema 24) over a derived guess, and approaches grouped by
start Link. Bounds are content: `data/los/hcm.json` (HCM 6th edition, A–E upper bounds; equal
takes the better letter), read only when some section has a type so other projects' manifests
stay unchanged. `src/eval/los.*` is pure (`losLetter`, vehicle-weighted `losGroups`);
`src/project/los_output.*` loads the pack strictly and writes the shared JSON `los` object and
the CSV block under the line `# LOS (pack hcm) from simulated section delay, not HCM control
delay; not validated (M6)`. Section blocks gain `controlType,los`. Batches letter the mean over
seeds. HCM's v/c > 1 rule is not applied (no v/c is simulated); unfinished trips stay visible.
The editor's section dialog sets the type in the same History step as the name. W74 → schema 25.
Contract and rows L1–L7 ([LOS](reference/LOS.md)) came first. Evidence: group `los` (6 cases —
every bound on both sides for both types; a stricter pack turns D into F on the same measured
delay; eight malformed packs refused; weighting by hand) and `section-ui`. Linux GCC 13.3:
headless 71/71, desktop 103/103 offscreen. A 10-seed four-leg batch with three signalised
sections: west through 48.3 s D, north left 57.6 s E, east through 43.0 s D; the intersection
row 46.4 s D equals the hand-weighted mean. Windows evidence is CI's.

---

## 2026-10-09 — M5.4 travel-time sections (D133)

The quantity M5.5's LOS will read now exists. A section is `network.travelTimeSections[]`
(schema 23 only when present): a start and an end line, each a cross-section of one Link at a
station, spanning every lane so a lane change inside is timed once. The contract
([TRAVEL_TIME_SECTIONS](reference/TRAVEL_TIME_SECTIONS.md)) and its rows TT1–TT11 were written
before the code. `SectionAccumulator` (`src/eval/sections.*`, owned by `MovementAccumulator`, so
batches get it free) interpolates each crossing between the two observed states, treats a new
vehicle as entering at distance 0 at `enteredTime`, and times an end line crossed in the arrival
step from the last speed. Delay is `max(0, travel − length/desiredSpeed)`, the whole-trip term
restricted to the section; trips count by their end crossing (D132's rule). Outputs gain a
`section` block only with a section, so every other project keeps its bytes (TT10 checks the
four-leg report equals the section-less one). Link delete removes, split moves, reverse refuses,
like other controls. Editor: Section tool (`T`), two Ctrl+right-clicks make one Undo step, and a
*Travel-time sections* tab renames and deletes; en/th strings. Results-tab rows for sections wait
for M5.6. W74 moves to schema 24.
Evidence: group `traveltime` (11 cases — a lone vehicle at desired speed gives exactly 100/15 s
and zero delay; a red held to 30 s gives 20.01 s against an analytic floor of 16.67 s; the
period's equality sides; a hand-built lane change) and `section-ui`. Linux GCC 13.3: headless
70/70, desktop 102/102 offscreen. A 10-seed batch of the four-leg project with a west-through
section gives 48.3 ± 4.3 s against 52.8 ± 4.3 s whole-route movement delay, with the same 121
mean vehicles: the gap is the entry acceleration the section excludes. Windows evidence is CI's.

---

## 2026-10-09 — M5.3 evaluation period and unfinished trips (D132)

Projects can now say which part of the run the results describe: `definition.evaluation
{warmup, end?}`, schema 22 only when set. Movement rows count trips that end in the period and
queues average its ticks; run totals stay whole-run so every vehicle is still accounted for.
Each movement reports `unfinished` (active or pending on its routes), and batches warn when a
movement's unfinished share passes 5 %. Owner's choices: no key means warm-up 0 (old results
unchanged), and new editor projects start with 900 s warm-up over 4500 s. Run settings gains a
Warm-up field, applied with duration and time step in one History step. Making the new window's
document carry a definition exposed that a demand-less definition took the runtime Problems
path and lost the drawing's topology rows (`tables-ui`); `runDiagnostics`/`documentDiagnostics`
now treat a definition holding only run settings (`onlyRunSettings`) as a drawing, with `EDIT_NO_INPUTS`.
`connector-ui` now checks its intent (lane widths keep the file's schema) instead of the literal
17, and the behaviour-library "future schema" pin moved from 22 to 23. W74's planned schema is 23.
Tests first: group `evaluationperiod` (9 cases; both equality sides of the window, a red wholly
inside the warm-up, unfinished = active + pending, schema-22 refusal below 22) and a Run settings
UI check in `signal-ui`. Linux GCC 13.3: headless 69/69 and, with Qt 6.4.2 installed locally,
desktop 100/100 offscreen. Single-run JSON for the four projects keeps every number; the CSV
gains only the `unfinished` column and the period line. Windows evidence is CI's. The oldest live PROGRESS entry moved whole to the archive.

---

## 2026-10-09 — M5.2 multi-seed batches and `--seeds` (D131)

`src/runner/` now holds the first M5 product code: `runSeed`/`runSeeds` run one compiled project
over independent seeds with the CLI's own loop, and `aggregate` reports n, mean, sample SD and the
95 % half-width (tabulated Student t) per movement and approach, summing in seed order. The CLI
takes `--project F --seeds 42-51 [--csv F]`; the JSON carries per-seed accounting and a
build-time `buildCommit`. Seeds with over 5 % of generated vehicles still pending are flagged and
kept in the means, by the owner's choice: dropping them would hide the worst seeds. Why the
formatter sits in `src/project/` and why the t table only widens: D131. Tests were written first
(group `batch`, 9 cases: hand statistics, permutation, one seed equal to the single run,
generated = completed + active + pending, a saturated input flagged, seed-list parsing, marker).
Linux GCC 13.3 headless Debug: 68/68 ctest, including `cli-batch`; single-run `--project` JSON
for the four projects, the M0 run and the four-leg CSV are byte-identical to before. Ten seeds
of the four-leg take ~10 s in Debug. No desktop or Windows claim; native CI is separate.

---

## 2026-10-08 — M5 first: owner answers and the M5 slice plan (D130)

The owner asked for a plan that makes the program usable for real work as fast as possible.
The 2026-10-03 review had found the back half of PROBLEM §4 (ten seeds, LOS, a table to
paste) without product code. The owner answered O1 (a), O3 (b), O8 (warm-up default 900 s,
unfinished trips counted and warned) and O9 (c); D130 records them and waives Q4's "decide
before M5" for M5 only. [M5_PLAN](plans/M5_PLAN.md) numbers M5.2–M5.7; NEXT puts M5.2 first
and parks W74. Why section delay before letters: whole-route delay includes source waiting
and entry acceleration (D39), so a letter on it would read worse than the junction is.
Docs only; no code, schema, gate or baseline changed. The D113 entry moved to the archive.

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

## 2026-10-06 — Exact conflict polygons and classified mouths (D115)

Owner authorized the geometry/type/display slice after inspecting why measured
Connector/Link mouth overlaps were excluded from crossings. Quad clipping now retains
convex polygon pieces beside both station intervals. Classification distinguishes crossing,
merge, branching and one-stream continuation without discarding measured mouth geometry.
Automatic crossing keys and topology-derived merge priorities/extents remain stable;
branching is derived/read-only and continuation adds no control. Two arriving Connectors
with only an edge contact can display their measured common receiving-lane mouths.

Canvas fill, hit testing and connected-place grouping use the retained polygon union;
the former full side strips and 0.3 m display/pick inset are superseded. Each lane pair
still has its own reservation and paired priority layers. Branching rows are translated
in English/Thai and reject priority authoring in both UI and commands, including a
disguised/stale suggestion. Polygons are recomputed from network geometry, never added
to the file schema. Geometry edits and Undo restore their derived shapes.
New/Open/Recover also invalidate the conflict cache across document revision resets.

Validation: GCC 13.3/C++20 local desktop build with Qt 6.4.2 and strict floating point;
all model tests pass **486 cases, 0 failures**. New analytical oblique-area, source/target
mouth, continuation, branching refusal, codec and edit/Undo regressions accompany Qt
offscreen canvas/picking/grouping tests. Qt/model edge comparisons allow only numerical
area tolerance; explicit outside-overlap probes must remain unpickable.
The complete local desktop CTest suite passes **88/88 groups**, including documentation,
architecture, file-size and frozen-reference checks.
Native Linux/Windows CI and owner appearance review remain separate gates. This is
M3.2.4d; full physical-mouth merge admission/clearance is M3.2.4e, not implemented here.
No cross-path swept-body, fidelity or owner/milestone closure claim is made.

---

## 2026-10-06 — Edited Connector lanes drive motion and controls (D114)

Owner reported that reshaping a Connector leaves cars on the endpoint-only cubic,
authorized the geometry-based correction, then explicitly requested the lane centres
and other systems use shared data. `connectorPaths` now supplies final adjacent-rail
midpoints with named lane-centre terminals and no endpoint-only runtime equation.
Lengths, front positions, rolling rear-axle guides, heads, route overlays and right-of-way
station mappings use those paths. The cubic remains the default drawing generator.
Zero points/Reset straight now drive straight; widths, ranges and interior edits can
change travel lengths and results. IDs, file schemas and core motion code remain unchanged.

`conflictSurface` is the shared source for overlap calculation, outlines and waiting bars,
using those same final painted rails and authored cross-sections. Physical setbacks map
to the runtime lane distance. Common mouth pieces remain topology joins even when P1–P4
cuts lie longitudinally before/after the Link attachment; a finite Link end can truncate
the painted terminal leg. A separate interior crossing of the attached lane remains a
crossing. Existing History following and Run invalidation handle edits/Undo/Redo.
The T-junction example is regenerated by its builder: ten control/extent numbers change;
network geometry and frozen reference fixtures are unchanged. Old authored conflict areas
are validated against current coverage on load, not silently repaired.

New fixed-end edit regressions cover both driving sides, both taper directions,
independently interpolated rail centres, compiled lengths, heads, waiting bars/setbacks,
file round trips and Undo/Redo. Updated canvas tests check edited lane fronts/body heading
and replacement of the same canvas's Run network. D107 motion-independence and D109
coverage-independence expectations are superseded, with their historical evidence retained.
Validation: GCC 13.3/C++20 Release, strict floating point, Linux headless `check` passes
**60/60 CTest groups**, architecture/file-size/documentation guards and unchanged frozen
references. Existing lane-centre sweeps cover 240 seeded curve/taper runs with replay,
accounting and segment-body separation. T-junction seeds 42–81 × headways 3/7/12 pass
**120/120** replay/accounting/body/swept-conflict-interval/braking checks;
[full rows](evidence/connector-edited-centres.jsonl) retain clamp counts and trajectory digests.
Reproduce with `trafficsim-t-junction-clamps --sweep out.jsonl REPO_ROOT`.
Local builds disable precompiled headers and use a writable TMPDIR; CMake/Ninja and
nlohmann/json are local build dependencies, not committed repository changes. Qt is
unavailable locally; native desktop CI and owner appearance remain separate. These checks
are interval/segment checks, not cross-path 2D collision proof. No owner/M0/M3/M6 gate or
fidelity claim is closed.

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
