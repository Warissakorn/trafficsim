# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is the history and the reasoning** — what a session
reads to understand why the code is the way it is. What to do next is in
[`NEXT.md`](NEXT.md); the [decision log](decisions/RECORD.md) has its own indexed record. Never delete an entry;
move old blocks whole into `docs/archive/` if this gets long, and list each in
[`archive/README.md`](archive/README.md), which indexes every older entry.

---

## 2026-10-10 — M5.8a scenario comparison, CLI (D148)

`trafficsim-cli --project BASE --compare ALT --seeds LIST [--csv FILE]` answers PROBLEM §1 step 6.
It runs both projects as batches over the same seeds and reports the alternative minus the base
per movement, section, approach and the network. Each row has a Welch 95 % interval: the
Welch–Satterthwaite df, then `tQuantile975(⌊ν⌋)`, the batch's own conservative table rule.

Why Welch, not paired: one random stream per run diverges as soon as the projects differ, so
equal seeds are not common random numbers, and the output says so.

What the command refuses or reports:
- **Matching.** Rows are matched by name; base-only, alternative-only and duplicated names are
  listed in JSON and on a `# Unmatched` CSV line, never dropped.
- **Refusals.** Evaluation periods that differ in warm-up, end or cool-down are refused before
  any run; different seed lists before any output.
- **No LOS.** A difference never gets an LOS letter.

Contract and rows CMP1–CMP7 are in [BATCH](reference/BATCH.md) §7, with
`tests/compare_tests.cpp` (group `compare`) and `cli-compare`/`cli-compare-conflict` in ctest. The
CLI's batch path was factored into `prepare`/`runAll` for both commands. `--seeds` output is
byte-identical before and after (`cmp`, seeds 42–44 on four-leg and 42–43 on M2.6 and the
T-junction; only `buildCommit` differs). The editor is M5.8b (NEXT).

An independent review of the M6.0 sheet also ran this session, and its 25 findings were fixed in
a separate commit. The run-cost entry of 2026-10-08 moved whole to `archive/`.

---

## 2026-10-10 — M6.0 benchmark option sheet (Q4)

Docs only, the D143 queue's item after M4.2. The [option sheet](evidence/m6-benchmark-options.md)
prepares Q4, open since 2026-09-10, for the owner to answer by letter
([OWNER_SITTING](plans/OWNER_SITTING.md) B9). It covers three kinds: S1 saturation flow and
capacity, S2 delay, and U minor-movement capacity with the gap-acceptance calibration R7 moved
to M6. Each kind has five published options, each giving its quantity, conditions and limits,
and the command that would produce TrafficSim's comparable figure or what is missing for it. No
TrafficSim figure was produced for it, and no tolerance is proposed, so the choice stays blind.

What the sheet found for the next M6 session:
- **Unsignalised capacity has no command yet.** The engine's rule is a gap time plus a distance
  headway, occupancy and area reservation, so a published critical headway must be translated,
  not copied into `gapTime`. A comparison also needs a saturated-minor project per major-flow
  level, and a follow-up headway measured at a priority line, which `--discharge` cannot give.
- **Effective green is not measured.** Amber ends the discharge window, so capacity needs a
  declared green or a saturated count.
- **Poisson generation** makes a random-arrival delay formula (Webster) the closer match, not an
  exact one: insertion and car-following reshape the stream before the stop line. HCM's uniform
  term alone describes arrivals the engine cannot generate.
- **No verifiable Thai study.** No published Thai study of these quantities could be verified,
  so a local benchmark would be the owner's own measurement.

**How the citations were checked.** Agents researched them with web search, an adversarial pass
checked them, and a third pass re-checked only the printed details. The HCM and HBS are paid and
were not read: their values come from agency and software documentation and are marked to be
read in the manual. Unconfirmed exhibit numbers are left out.

PR #135 (M5.9, M4.2) merged, with native CI green on Linux and Windows. NEXT now points at M5.8.
The D129 entry moved whole to `archive/`.

---

## 2026-10-09 — M4.2 amber stop-or-go (D147)

An optional behaviour `amberDeceleration` (catalog 3.0 m/s², ITE) turns amber into a continuous
check: go when `speed² > 2·a·gap`, else stop. With it, a vehicle that cannot stop at its type's
maximum deceleration goes on amber or red, as at a priority rule. That second rule was the owner's
choice after the first measurement left 44 physically impossible stops at red onset. One comparison
in the core head loop, no vehicle state. M0 scenarios (no `schemaVersion`) resolve the catalog
without the field through a never-written, never-compared provenance flag. So the frozen TS
fixtures, `trafficsim-cli 42` and the editor's `crossing.json` run did not move; a save and reopen
round-trips exactly. An owned behaviour with the field is schema 28. Capturing the catalog now owns
it, which exposed a latent defect: owned catalogs without a library were written without `model`
at schema 22+ and could not be reopened. The writer now tags every owned behaviour from schema 21.

Contract and rows AM1–AM8 in [AMBER](reference/AMBER.md); `tests/amber_tests.cpp` (group `amber`)
plus a regression for the untagged catalogs. Four-leg over seeds 42–81: clamps 220 → 1, every
movement's delay down 1.4–7.5 s; M2.6: clamps 844 → 35 (not classified), delay −1.1 to −12.3 s
([evidence](evidence/m4.2-amber.md)). Changed tests, each for a stated reason:
- The four-leg clamp diagnosis is pinned on the run with the field removed (D36), and a new row
  pins zero clamps with it.
- The archived M3.2.7 sweep inputs drop the field, noted in that evidence as D36-era.
- `results-tabs-ui` uses a copy owning the D36 behaviour so its clamp page still has clamps.
- Fixtures that test schemas 18–25 strip the field.
- Future-schema probes move to 29.

Linux only: desktop preset (Qt 6.4.2 offscreen, GCC 13.3), ctest 113/113 and `check` green.
Windows is the PR's native CI. The BA14/BA17 and D128 entries moved whole to `archive/`.

---

## 2026-10-09 — M5.9 evaluation cool-down (D146)

`definition.evaluation.cooldown` (schema 27) runs the simulation that many seconds past `duration`
with no new demand (inputs still end by it; `compileDocument` extends the compiled duration after
every expansion, so core is untouched). With a cool-down the window selects trips by release
(`scheduledTime` in `[warmup, end]`, `end` defaulting to `duration`) whenever they finish, in
movement and section rows, and only window vehicles left at the end are unfinished; without one
every D132 rule stands. New projects get 900 s; the run-settings dialog edits it in the same
History step as the warm-up; JSON, CSV and both Results notes name it. Contract and rows CD1–CD8 in
[BATCH](reference/BATCH.md) §5, written first; `tests/cooldown_tests.cpp` (group `cooldown`).
On `four-leg-signalised` seeds 42–51 the warning goes from 11 of 12 movements to none at 120 s and
every window trip finishes at 300 s ([evidence](evidence/m5.9-cooldown.md)); the example itself
stays schema 17 (owner's choice, sitting B8). Nine CLI outputs of the committed examples are
byte-identical before and after. Future-schema tests now probe 28. Linux only: desktop preset
(Qt 6.4 offscreen, GCC 13.3), ctest and `check` green; Windows is the PR's native CI. The D126/D127
entries moved whole to `archive/`.

---

## 2026-10-09 — Three modelling levels designed and booked (D145)

The owner asked for more capability chosen by what studies need — parking lots, dynamic
assignment, then Micro, Meso and Macro levels, Macro as a full four-step model, after the D143
queue. Docs only, no code: [design](plans/MULTI_LEVEL_MODELLING.md) (study → output matrix, one
authoring network compiled to three views, shared Zone/OD demand, parking lots as zones with
capacity, between-run dynamic assignment, pure `src/meso/` and `src/macro/` engines, subarea
cut), ROADMAP M8–M12, PROBLEM §1–§6 amended (the assignment non-goal replaced), ARCHITECTURE
planned rows, OWNER_SITTING B7 for the design's open parameters. Parking and regional modelling
left *Later*. Reasoning in [D145](decisions/RECORD.md#d145). The D124/D125 entries moved whole
to `archive/`. Checks: `trafficsim-check-docs` and `trafficsim-check-file-sizes` built directly
with GCC 13.3 (the headless preset lacks nlohmann/json on this machine); no engine build claimed.

---

## 2026-10-09 — Branch audit; Results discharge and clamp pages recovered (D144)

Every origin branch ahead of `main` was checked. `main` was re-rooted on 2026-09-28 (four root
snapshots), so the 38 older branches share no history with it and git's "ahead" counts mean
nothing; by content, every decision they name is in `main`, and the only files they hold that
`main` lacks are the retired M0 window (D24) and an unreferenced `data/fonts/NotoSansThai.ttf`.
PR #128 (`claude/dazzling-gates-df64jc`, closed unmerged) held the one piece of lost work:
Results inner pages for queue discharge and safety clamps (`9b81b17`, `cbfff89`). Its W74 and
M5 commits were already replayed or superseded. The two commits were cherry-picked onto the D142
branch, merged by hand with M5.6 (the batch view sits in the Movements page; `startBatch` selects
it), renumbered D135 → D144 (and a stray D134 → D139 in DRIVING_BEHAVIOUR), and `section-ui`
now finds the Results page as the other suites do. `check`: 111/111 (Linux, Qt 6.4.2 offscreen).

---

## 2026-10-09 — Owner's answers to the roadmap review (D143)

The owner accepted every recommendation, R1–R8. Documents only, no code: D143; ROADMAP closes M2
(gate D53, sub-milestones carried), relabels M1.26, narrows M1.22 to what `src/` lacks (checked:
no spline/arc, extend/merge, layer locks or per-Link driving side; reversal refuses referenced
Links), defines M1.23's CSV, moves M5.1's transit/crosswalks and scenario management beyond M5.8
to *Later*, and adds M3.4 (the M3.2.8c remainder), M4.2 (amber) and M6.0 (benchmark sheet, which
now owns calibrated gap acceptance). M5_PLAN gains M5.8 and M5.9 and the order after M5.7.
PROBLEM gains §8 (motorcycles). `plans/OWNER_SITTING.md` is now the single list of owner items
(gates, chat decisions, 22 desktop looks, each dated with a yes/no failure question); NEXT keeps
only the session queue and points there. `check`: docs and file sizes clean.

---

## 2026-10-09 — Volume from turning counts (D142, schema 26)

The owner answered ROADMAP O10's "count sheet typed once vs D46": an input may take its volume
from its entry decision's turning counts. `VehicleInput.volumeFromCounts` is opt-in, so D46 and
every existing file are untouched. `src/project/counted_volumes.cpp` holds the one rule:
`countedDecision` (the decision named, or the placed one on the entry Link), `countedVolumes`
(summed base-route counts × 3600 / length per decision interval) and `syncCountedVolumes`, called
on read, in `putInput`/`putRoutingDecision`, in `History::execute` and at the top of
`withRoutingDecisions`, so no path sees a stale volume. The input's intervals are not written for
such an input. Issues `INPUT_COUNTS_NO_DECISION`, `INPUT_COUNTS_EMPTY` and
`INPUT_COUNTS_POSITIONED` (a D119 decision is a downstream split, not a source). The input dialog
gains the checkbox, which locks the typed volume, counts and Periods and shows the counted total;
the input and decision rows say which feeds which. `field<bool>` now refuses a non-boolean.

Evidence (Linux, GCC, Qt 6.4.2 offscreen, Debug): `demandcounts` 8/8 (VC1–VC7, each movement's
vehicles equal its counts for unplaced and placed decisions), `counted-volume-ui` (VC8), and the
D46 test unchanged. CLI `42 --project` output for the four shipped projects equals `origin/main`'s
(`c7628e8`, built in a scratch worktree) byte for byte. Native Windows CI and the owner look are owed.

---

## 2026-10-09 — M5.6: the editor's Run seeds, Copy and Export (D141)

The editor now runs the CLI's batch. **Run seeds** parses the *Seeds* field with `parseSeedList`
(default `42-51`, O9), compiles the document on the UI thread and hands a `Scenario` and an
`EvaluationSpec` by value to one `std::thread` that calls `runSeeds` and `aggregate`
(`src/shell/editor_batch.cpp`). Progress and the result come back through queued
`QMetaObject::invokeMethod` calls tagged with a generation number; `cancelBatch()` (called from
`clearRun()`, so every edit, Undo, Open, Reset and a single Run reach it) bumps the number and sets
the worker's cancel flag, so whatever a stale worker posts is dropped. The destructor cancels and
joins. No mutable statics were found in `src/core`, `src/eval`, `src/model` or `src/runner`.

The batch table replaces the single-run tables: movements and sections with n, mean delay, ±95 %,
vehicles, travel time, unfinished, control type and LOS; approaches with n, mean ±95 % and max; a
note with the marker, n, seeds, evaluation period, overloaded seeds, movements with unfinished trips
and the LOS pack. Export writes `batchCsv`; Copy writes `csvToTsv(batchCsv(...))` (new in
`csv_format.hpp`: comment lines kept, unquoted commas become tabs, quotes dropped), and a finished
single run copies `movementCsv` the same way. Contract and rows EB1–EB6 in BATCH §6.

Evidence (Linux, GCC, Qt 6.4.2 offscreen, Debug): `batch-run-ui` compares the editor's
`BatchReport` with `aggregate(runSeeds(...))` on four-leg plus two signalised sections, the export
bytes with `batchCsv` and the clipboard with its TSV; cancel, an edit, a bad list, a batch after a
cancel and closing mid-batch are exercised; 13–14 s, three repeats stable. Its first version left an
autosave draft (an unsaved rename outlived the 15 s timer) that blocked the next run on the recovery
dialog; the test now saves after the edit. `check`: 108/108 tests, architecture, file sizes and
docs clean. Native Windows CI and the owner's look are separate and still owed.

---

## 2026-10-09 — Optimization pass: lld, D140, validate benchmark, NEXT

Measured first (Linux, 4 cores, Qt offscreen): the engine (0.15–0.30 µs per vehicle-tick, linear
to 96 crossings), the run view (~3.3 ms per Step) and redraw (3.6 ms at 40 crossings) are not
bottlenecks and were left alone. **Link with lld when the compiler accepts it**
(`TRAFFICSIM_LLD`, default ON, MSVC untouched): touching one source relinks some 40
executables, and GNU ld was most of an incremental Debug build. `trafficsim-tests` links in
4.5 s with ld and 0.57 s with lld; touching `src/shell/editor_demand.cpp` rebuilt in 31.3 s
before and 13.0–13.5 s after, `src/model/network/routing.cpp` 28.7 s → 10.7–11.1 s. CI's Linux
apt line installs `lld`; without it the default linker is kept silently.

**D140: Connector paths once per expansion.** `History` validates every command, and validating
the M2.6 study template took 21.6 ms against 1.0 ms for four-leg on the same network: callgrind
put 85 % of it in `connectorPaths`, recomputed by every `routeShortestChains`, `routeLaneFamily`
and `routelessChains` call inside one `expandRouteless`. `ConnectorPathTable` (lazy, local, never
stored) is now built once by `expandRouteless` and `routelessIssues` and passed down; the old
signatures build their own, so editor and compile callers are unchanged. Validation 21.6 → 5.0 ms
(median of 15; four-leg 1.03 → 0.91 ms). `--seeds 1-3` output of all four shipped projects is
identical apart from the build stamp; the ten-seed study batch and the engine benchmark are
unchanged within noise (an interleaved ld/lld engine run ruled out the linker).
`tools/validate_benchmark.cpp` (`trafficsim-validate-benchmark [project] [repetitions]`) keeps
that number measurable; like the other benchmarks it prints and is not in `check`.

**Route continuations find their tail once.** After D140, 43 % of study-template validation was
`continuations` rescanning every route object to find the tail, once per candidate. It now finds
it once (the last match, as before). Validation 4.97 → 2.75 ms (four-leg 0.85 → 0.72 ms); the
editor's route hover and gesture use the same function. CLI output identical.

**Destination chains once per expansion (D140's walk scope).** `placedDecisions` runs once per
time slice × vehicle type and resolved the same `routeShortestChains(decision Link, destination)`
each time, about 86 calls per study-template validation. A `DestinationChainMemo` beside the
table resolves each pair once. Same-session A/B: 3.32 → 2.46 ms median (five runs of 31 each);
four-leg unchanged (no destination decisions). CLI output identical.

**`appendStationRouting` takes the expansion's `runtimeSections`.** It rebuilt the table on every
call (≈5 per study-template validation, 23 % of the remaining instructions); `expandRouteless`
already holds that table and passes it to `appendLaneChanges` beside it. A/B: 2.60 → 1.72 ms
median (five runs of 31); four-leg unchanged (no positioned decisions). CLI output identical.
Validation overall this session: 21.6 → 1.7 ms. Left: `routelessChains` still walks once per
input × slice; at under 2 ms per edit it was not started.

**NEXT.md slimmed** (31.4 → 26.8 KB, −15 %): the five 2026-10-05/06 owner-instruction sections narrated
finished slices already recorded in ROADMAP (M2.8, M3.2.4, M3.3 status lines), PROGRESS and
RECORD (D115–D129, D135–D139, each checked before removal). One section keeps every open item
verbatim — the D128/D119/D116/D115–D118 Windows reviews, the Demand PR review order, W74's next
steps, BA18, the M3.2.4e contract rule and pending native CI. §1–§5 are live owner gates and
working notes and were not cut, so the planned −40 % was not reached.

---

## 2026-10-09 — W74 replayed on top of M5: D135–D139, schema 25

W74 (M3.3.3a) and M5 were built in parallel from the same `main` and both used D130–D134 and
schema 22. By owner instruction the M5 branch is the base and the W74 slices are replayed on it:
D130–D134 of the W74 branch are now D135–D139 everywhere (record, index, code comments,
evidence), and `w74` is written in schema 25 (22–24 are M5's evaluation period, sections and
control type); a file below 25 carrying `"w74"` is refused, and every "future schema" probe is 26.
The entries below keep their original dates and test counts. D139 (the dialog editing `w74`
keys, with no code defaults) had no entry of its own; its reasoning is in the record. Not carried
over from the W74 branch: its own `--seeds`, editor Run N seeds and evaluation period (M5.2–M5.3
there, superseded by D131–D132 here; the editor batch is a reference for M5.6) and its Results
tabs (discharge and clamps). The decision record's blank lines between rows are removed again.

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

## 2026-10-08 — W74 schema-25 codec, BA29 (D136)

The second M3.3.3a slice, codec only by the owner's choice. `DriverBehaviour::w74`
(`std::optional<W74Parameters>`) is the model tag; the W74 value types moved into
`types.hpp` so it can hold them. `parseBehaviour` dispatches on `model`, refuses each
model's keys on the other, and reads the 18 keys from `w74ParameterKeys()`, the same table
the serializer and §5 range check use. Schema 22 is written only when an owned behaviour is
`w74`; below 25 one is `UNSUPPORTED_BEHAVIOUR_MODEL`. Run refuses a `w74` behaviour a
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

## Backlog (M0, in order)

The [historical checklist](archive/PROGRESS-M0-backlog-and-questions.md#backlog-m0-in-order)
is archived. Current work lives in [NEXT](NEXT.md); milestone gates live in [ROADMAP](ROADMAP.md).

## Open questions

The [historical register](archive/PROGRESS-M0-backlog-and-questions.md#open-questions)
is archived. Use [the owner sitting, §B](plans/OWNER_SITTING.md#b--decisions-in-chat-one-line-each) for current questions.

## Decisions

The complete [decision record](decisions/RECORD.md) now lives beside its
[topic index](decisions/README.md). This heading remains as a compatibility target
for existing `PROGRESS.md#decisions` links. Add new decisions to RECORD, not here.
