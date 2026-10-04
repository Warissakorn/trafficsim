# M3 acceptance design and evidence record

**Status: automated rows recorded through A46 (M3.2.8c, §4); A47–A58 implemented and off (D102); owner exercise M3.2.7d not performed.** This is a prepared engineering test matrix and owner exercise,
not a passed gate or a scientific validation result. The M2 gate passed 2026-09-25 (D53). [M3_CONTRACT.md](M3_CONTRACT.md) defines the proposed behavior;
[M3_PLAN.md](M3_PLAN.md) names the slices. New tests and runnable fixtures are not added by
this documentation change. Record actual evidence below as each slice is implemented.

## 1. Automated matrix

Every row starts **Pending**. A test for a rejected case must first prove that its setup
actually contains that case. Compare physical quantities using a declared tolerance; keep
same-build event replay exact. Do not weaken the frozen reference comparisons.

| ID | Slice | Setup | Required result |
|---|---|---|---|
| A01 | .2 | Old schemas and M0 scenario; no new controls | Existing compiled scenario and four frozen runs unchanged; seed 42 regression retained |
| A02 | .2 | Explicit crossing, merge group, rule, stop and counter | Save/Open preserves IDs, units, values and references; derived state absent from file |
| A03 | .2 | Unknown field/version, NaN, duplicate ID, bad enum, dangling owner | Specific error; edit/load leaves the published document unchanged |
| A04 | .2 | `undetermined`, stale lane pair, unsupported geometry | Draft can be inspected/saved; Problems identifies it and Run refuses |
| A05 | .2 | Split, copy, lane resize, retarget and delete on curved roads, both driving sides | Correct remapping or explicit blocker; no ordinal reassignment; Undo/Redo restores exact authored state |
| A06 | .2 | Two-way cycle, three-way cycle, tied major paths and missing pair | Effective graph rejected; a valid total order and legacy derived order accepted |
| A07 | .2 | Reverse a merge's priority explicitly; then remove one rule | Fallback removed only for that whole group; no reciprocal hidden rule; incomplete group stays blocked |
| A08 | .2 | Missing defaults with fully explicit rules, then with an automatic merge | Explicit saved group is portable; unresolved fallback reports missing defaults |
| A09 | .3 | `d/headway` and `d/v/gapTime` just below, at, above thresholds | Exact inequality semantics in the contract; independent safety constraints remain active |
| A10 | .3 | Moving/stopped major vehicle on the segment upstream of a section cut | It participates when its route reaches the conflict; no disappearance at the segment boundary |
| A11 | .3 | Two vehicles arrive in the same tick; shuffle unordered input collections | Same permitted admission and event stream; preserve authored drawing order where it is intentionally semantic |
| A12 | .3 | Short/long vehicles, front beyond exit but rear inside | Opposing admission denied until rear clearance; insufficient sink distance rejected |
| A13 | .3 | Fast vehicles traverse a whole conflict within one tick | Swept trajectories cannot overlap even when end snapshots show an empty area |
| A14 | .3 | Exit queue, simultaneous requests sharing exit space, then queue discharge | No double reservation; vehicles wait upstream and resume when receiving space clears |
| A15 | .3 | Closely spaced conflict areas and an unsupported group topology | Atomic group admission where supported; otherwise explicit Run blocker, never partial unsafe entry |
| A16 | .3 | Minor admitted, then priority vehicle arrives | Existing grant retained; major waits safely; finite demand clears without deleting vehicles |
| A17 | .3 | Same-side following through an area | Ordinary longitudinal spacing remains; no unintended one-vehicle-per-area restriction on compatible traffic |
| A18 | .5 | Empty major road, same arriving vehicle with Stop then Yield | Stop reaches and serves the line; Yield has no mandatory zero-speed dwell |
| A19 | .5 | Several queued vehicles behind Stop; pause/resume and reset | Every vehicle serves at the line, not at queue tail; no repeated stop after service; reset clears service |
| A20 | .5/.6 | Red/amber/green co-located with control; occupied conflict on green | Restrictions compose as documented; green does not erase physical safety |
| A21 | .6 | Head before/on/after section cut, then stretch/split/copy Link | Current placement semantics retained; command/UI/run agree; both driving sides covered |
| A22 | .6 | Known stationary queue at independent line; no signal heads | Correct metres and hysteresis; no fake signal objects; changing counter leaves trajectory unchanged |
| A23 | .6 | Signal-derived counters and the same explicit measurement lines | Matching queues; shared CLI/editor delay and queue reports; no duplicate approach rows |
| A24 | .4-.6 | English/Thai, pointer/keyboard create/edit/delete, cancel, Undo/Redo, reopen | Units/parameters correct; selection and Problems point to the authored object; successful edits invalidate run |
| A25 | .3-.7 | Branch a copied state, same seed/toolchain; congested finite demand | Independent snapshots, exact replay, no lost vehicles; completed/active/pending/clamps reported |
| A26 | .7 | T-junction gap/headway sweep described below | Controlled boundaries and admission times match; stochastic differences reported without invented monotonic guarantees |

M3.2.8b rows (mandatory lane changing, [`M3_8_CONTRACT.md`](M3_8_CONTRACT.md) §2), written
before its code:

| ID | Slice | Case | Must show |
|---|---|---|---|
| A27 | .8b | Two-lane Link, a movement leaving from one lane, inserted on the other, empty road | The stub vehicle changes once, completes on the full chain; no clamp; a stub never arrives |
| A28 | .8b | A leader alongside or just ahead in the target lane | Refused while the forward gap or own braking fails; changes once it clears |
| A29 | .8b | A fast follower close behind in the target lane | Refused (the forcing is asserted first: the follower would brake harder than `comfortableDeceleration`); changes once safe |
| A30 | .8b | Target lane blocked for a long time | Held at its dead end by the stop-line mechanism, zero clamps, then changes and completes |
| A31 | .8b | Two stub vehicles aiming at one gap in the same tick | Only the lower id changes that tick; no overlap |
| A32 | .8b | A span over a conflict area | No change between a waiting line and the area's exit, on either route |
| A33 | .8b | Branch a copied state during changes; congested finite demand | Exact replay; completed + active + pending conserved; changes and dead-end waits reported |
| A34 | .8b | The four-leg drawing compiled | Each turn has one chain per entry lane; the stubs, spans and dead ends are as drawn; single-lane baselines unchanged |
| A35 | .8b | A stub vehicle waiting at its dead end beside a steady stream with no acceptable gap | The nearest target-lane vehicle that can stop comfortably holds back; the waiting vehicle changes mid-stream, with no clamp (added with the owner's cooperation ruling, contract §2 "Cooperation") |
| A36 | .8c | A stub vehicle approaching its dead end beside a steady stream, behaviours with `maxDecelerationCooperativeBraking` | It changes **before it stops** (never below walking pace on the stub), with no clamp. The forcing is asserted first: with the field cleared, the same vehicle waits at its dead end |
| A37 | .8c | A36's run, every tick | No vehicle brakes harder than `maxDecelerationCooperativeBraking` (acceleration below its negative), and nobody is clamped |
| A38 | .8c | A stub vehicle far from its dead end (outside its look-ahead) beside a stream | No cooperative hold is issued for it |
| A39 | .8c | A35's road with the field cleared | Identical event stream to the D71 rule; a copied state replays exactly with the field set |
| A40 | .8c | Downstream decision (D93): an entry Link feeds a two-lane Link D by Connector; a decision on D sends to a destination only lane 1 reaches and one only lane 2 reaches, and both lanes receive vehicles | The compiled volume of each destination equals its typed share of the vehicles reaching D exactly (no shift towards what the lanes allow) |
| A41 | .8c | A40's network, a vehicle drawn to the destination its arrival lane cannot reach | It changes lanes on D before its dead end, with no clamp, and arrives on the full route, reported under (entry Link, destination Link). The forcing is asserted first: it is on a stub |
| A42 | .8c | A40's network, a stub vehicle still on the entry Link beside an open gap in the adjacent entry lane | No change before D: no span lies upstream of its arrival on D |
| A43 | .8c | A Link D whose adjacent lane no walk from the entry reaches | No stub is kept; that lane keeps the lane-fixed draw and the decision reports `ROUTING_DECISION_LANE_FIXED`. The forcing is asserted first: the adjacent lane has no arriving path |
| A44 | .8c | Every shipped project and frozen fixture (none has a downstream decision) | Byte-identical CLI output, four TS baselines and published reports; free walk with no decision stays lane-fixed |
| A45 | .8c | A40's network, a copied state branched during changes | Exact replay; no new `SimState` field |
| A46 | .8c | A30's and A36's cases built on D | Waiting cooperation and cooperative braking act on a downstream stub as on an entry stub |
| A47 | .8c | Discretionary (D95): a two-lane Link, one authored route full on both lanes; a slow vehicle ahead on lane 1, a faster one behind it, lane 2 empty | The follower changes to lane 2 with no clamp, emits a `LaneChangeEvent` and arrives under the same movement. The forcing is asserted first: it was in following mode behind the slow vehicle |
| A48 | .8c | A47 with a vehicle behind on lane 2 that would have to brake harder than `acceptedDecelerationTrailingVehicle` but no harder than `comfortableDeceleration` | No change. The forcing is asserted first: §2's mandatory rules 3–4 accept the same placement |
| A49 | .8c | A47 with lane 2 better by less than `discretionaryLaneChangeThreshold` | No change. The forcing is asserted first: `a_there > a_here` |
| A50 | .8c | The four-leg and M2.6 drawings compiled | No span leads from a full route to a stub; every discretionary span joins full routes with equal family sets ending on the same Link; movement rows and compiled volumes are unchanged, and A40 still holds exactly |
| A51 | .8c | Every behaviour without `discretionaryLaneChangeThreshold` | No discretionary change: with the field removed, the three projects' CLI output equals D94's byte for byte. With it present, the single-lane T-junction output and the four TS baselines are unchanged |
| A52 | .8c | A47's road, a copied state branched during discretionary changes, with and without a hold | Exact replay; `lastLaneChange` is the only new `SimState` data (D101) |
| A53 | .8c | A47's road under a steady two-lane stream; then four-leg and M2.6, seeds 42–81 (rewritten by D101) | A discretionary change straight back to the route the vehicle's previous change left, within 10 s of it, is a return. Returns are reported per project and threshold; D95 passes when they are at most 1% of discretionary changes (cap proposed 2026-10-02, the owner may move it). The hold must be shorter than the 10 s window, or the count tests nothing |
| A54 | .8c | A47's incentive inside a conflict area, or while holding a Stop service | No discretionary change (§2 rule 2) |
| A55 | .8c | Four-leg and M2.6, seeds 42–81, threshold 0.25 / 0.5 / 1.0 | Per-movement Δdelay ± SE against D94, discretionary change counts and clamps, in `docs/evidence/m3.2.8c-discretionary.md`; the published reports are re-published. Clamps rising goes back to the owner |
| A56 | .8c | A47 with `discretionaryLaneChangeHoldTime` H: the follower has just changed (discretionary), and lane 1 becomes better by more than the threshold | No change back before H has passed since its change, and the change is allowed once it has. The forcing is asserted first: the same state with no hold changes back at once |
| A57 | .8c | A stub vehicle two lanes from its full route, with a hold longer than the time between its two mandatory changes | Both mandatory changes happen as without a hold: the hold delays only discretionary changes. The forcing is asserted first: the two changes are closer together than the hold |
| A58 | .8c | Behaviours with the D95 fields and without `discretionaryLaneChangeHoldTime` | The run equals D95's without the record, change for change (the record is written but never read) |

Cooperative braking is A36–A39 (M3.2.8c). Downstream routing decisions are A40–A46 (D93),
implemented 2026-10-01. Discretionary lane changes are A47–A55 (D95) and the hold after a change
A56–A58 (D101), implemented and off by the owner's choice (D102); A53 fails
(`docs/evidence/m3.2.8c-discretionary.md`). Visibility, `laneChangeDistance` and any further
cooperation are **M3.2.8c** and get their rows before their code. Passing A01–A58 alone does not
close M3.2.

## 2. T-junction fixture specification

Implement `tools/t_junction_network.hpp` through existing commands, following the four-leg
fixture pattern, then generate `data/projects/t-junction-priority.traffic.json`. These are
planned paths, not present artifacts. Tests compare the builder with the saved project and
compile/run it through the same entry points the editor uses.

- A two-way major road and one minor approach, at grade, with explicit through and turning
  routes. Use a right-hand layout where the minor-road left turn crosses the opposing stream
  and then merges, plus its left-hand mirror. Document the mirrored turning movement rather
  than silently claiming a left-hand near-side turn exercises the same crossing.
- The minor movement has at least one **crossing area and a separate downstream merge**.
  This prevents an existing merge-only fixture from pretending to exercise crossing control.
- Use dedicated lanes/routes, no runtime lane changes, no signals in the base case. Include
  a Yield version, a Stop version and a blocked downstream receiving lane. The signal
  composition case is a separate variant.
- Put the waiting line before the first area; size the sink clearance for the longest tested
  vehicle. Add a real queue measurement line on the minor approach. Keep vehicle and behavior
  catalogs, duration, timestep and geometry identical across paired parameter runs.

### Controlled cases before stochastic sweeps

Use test-owned deterministic initial states/arrival schedules to isolate the threshold. This
does not require a new public demand model. Hold the major vehicle at a prescribed approach
speed and place the minor at its admission line. Ensure its receiving lane is free.

Example: major front 50 m before entry at 10 m/s, `headway = 7 m`, so its instantaneous
time to entry is 5 s. Check `gapTime = 4, 5, 6 s`: the threshold allows the first two and
denies the third. Check headway separately at 7 m minus tolerance, exactly 7 m, and 7 m plus
tolerance with time-gap blocking inactive. These are **predicate** expectations; the end-to-end
solver must additionally satisfy clearance and swept safety before allowing movement.

For the movement-level test, choose geometry/vehicle parameters so a permitted minor vehicle
can clear before the major arrives. Assert the setup's clearance margin first. Changing only
gap from the permitted to denied case must postpone this vehicle's admission and increase
its eventual completed-trip delay with the same route/free-flow reference. Specify expected
tick bounds from that setup before running it; do not freeze output from the new engine as
its own oracle. Both finite-demand cases must drain, or compare unserved counts explicitly.

### Diagnostic seeded sweep

Before observing outputs, commit the final fixture metadata: duration, timestep, volumes,
catalog IDs and hashes, geometry revision and build/toolchain. Start with seeds
`0, 42, 43, 4294967295` and paired `gapTime = 3, 5, 7 s` at `headway = 7 m`, then paired
`headway = 3, 7, 12 m` at `gapTime = 5 s`. These are test inputs, not calibrated values.
Archive each run's completed movement counts, whole-route delay, approach mean/max queue,
active/pending and clamps. Retain event traces for investigated cases.

Expect greater restriction in controlled admissibility tests. Do **not** require delay to be
monotone for every stochastic seed: interaction and incomplete-trip selection can change the
reported mean. A smaller completed-trip mean with more unserved vehicles is not an improvement.
Record differences and investigate unexplained changes; do not tune criteria after seeing them.
This development sweep does not implement M5 batch reporting or establish M6 validity.

## 3. Owner exercise

After automated evidence is green, the owner uses a Windows build to create/open the fixture,
inspect the two areas, set which movement yields, change gap/headway, and watch the crossing,
merge, Stop and Yield variants. Then save, close, reopen and repeat the same seed. Confirm that
the control lines and rule values survive and that the reported run repeats.

Record whether the minor-road behavior and response are plausible, with actual timings,
observations and defects. This operationalizes ROADMAP's existing M3 done-condition; it adds
no fabricated delay tolerance or Vissim-equivalence test. A green CI run is not this exercise.
Keep any failure in the record; fixes receive separate attempts rather than overwriting it.

**Recording sheet (M3.2.7d).** Pending until the owner fills it in; no session may fill it.

- **File:** `data/projects/t-junction-priority.traffic.json`, opened in the Windows build.
- **Steps:**
  1. Inspect the three areas: *Minor crosses eastbound*, *Minor joins westbound* and *Minor joins
     eastbound*.
  2. Flip one priority, then set it back.
  3. Change `gapTime` and `headway`.
  4. Set Stop, then Yield.
  5. Run seed 42, and note the Results rows.
  6. Save, close, reopen.
  7. Run seed 42 again, and compare the rows.
- **Record:** owner, date, Windows version, attempt number, and for each step what was seen, with
  timings. Then the plausibility verdict, and any defect with its steps to reproduce.

## 4. Evidence record

| Field | Observation |
|---|---|
| M2 gate evidence and pass prerequisite | Passed by the owner's judgment, 2026-09-25 (D53, `M2_GATE.md`) |
| Implementation commit / build / platform | D105 session: baseline `f538a7e`, local input registration `048b541`; Linux x86_64, GCC 13.3.0, Release, JSON 3.12.0. Fix source and all results: [clamp evidence](evidence/m3.2.8a-clamps.md). This is Linux headless evidence, not Windows or an owner verdict. |
| Fixture metadata and data hashes | D105: [current-fixture metadata and SHA-256](evidence/m3.2.8a-clamp-metadata.json), registered before outputs in local commit `048b541`. The archived pre-D80 fixture and its metadata remain unchanged; it is not accepted by current coverage validation. |
| A01-A26: test names, results, artifacts, uncovered rows | M3.2.2a, `tests/right_of_way_tests.cpp` (`rightofway.*`), all passing on Linux headless and desktop: A01, A03, A04, A06, A07, A08. **A02 partial** — merge areas, waiting lines and rules round-trip; Stop controls and counters do not exist until M3.2.5/M3.2.6. **A05** (M3.2.2b): `tests/right_of_way_lifecycle_tests.cpp` (`rightofway_lifecycle.*`) — delete/drag cascade with Undo/Redo, split remap to the same world point (1e-9) and straddle refused, copy only with every owner, lane/retarget edits keep ids and report unresolved, reverse refused; curved Link, both driving sides; six of eight fail on the pre-M3.2.2b commands. **A04 extended** (M3.2.2c): `tests/right_of_way_resolution_tests.cpp` (`rightofway_resolution.*`) — a waiting line on the preceding Link compiles before the Connector; a bypassable or non-upstream line and an uncovered, non-overlapping or doubly-crossing extent are named Run blockers; five of six fail on the pre-M3.2.2c code. **M3.2.3a** (D57): `tests/conflict_zone_tests.cpp` (`conflict_zone.*`) — A09 exact thresholds, A10 major seen before a section cut, A11 identical events for shuffled inputs, A12 sink clearance refused, A13 whole-area jump in one tick and a same-tick request capped by the swept check, A14 standing queue past the exit, A16 grant kept while the major waits, A17 same-side following, A25 copied state replays; a congested sweep with no swept overlap. Six fail with admission disabled; the rest check thresholds, replay, validation and non-over-restriction. `tests/right_of_way_runtime_tests.cpp`: an authored crossing compiles to one zone and runs; minor-road travel time rises with gap time; span, group and undetermined areas refused by name. **M3.2.3b** (D58): `tests/conflict_chain_tests.cpp` (`conflict_chain.*`):
- A15: chained zones admit together, and nothing overlaps under demand.
- A side over a section cut, with a route turning off inside it.
- `CONFLICT_ROUTE_JOINS_INSIDE` and broken chains refused.
- Disabling chaining or chain-following each fails its own test.

`rightofway_runtime.*`:
- Span and two-area cases compile and run.
- A taken-over merge replays the fallback's seed-42 events exactly, and its reversal changes them.

**M3.2.3c** (D59), in `conflict_chain.*`:
- A14: requests behind one standing leader share its room.
- At a merge zone, the major side waits for an admitted minor, where the M3.1 rule let it onto the
  join.
- A hold cycle is refused and an acyclic mix is valid.
- Disabling the room reservation or the cycle check each fails its own test.

In `rightofway_runtime.*`:
- A taken-over merge compiles to a zone carrying the fallback's line and numbers.
- A taken-over three-way merge runs with every vehicle served, as the fallback does.

**A18–A20** (M3.2.5a, D62), `tests/stop_control_tests.cpp` (`stop_control.*`):
- **A18:** the same vehicle on an empty major road. It crosses under Yield without ever standing
  still, and under Stop only after standing at the line at the start of two ticks.
- **A19:**
  - Three queued vehicles each serve at the line; the tail holds no service.
  - Service survives a later block by a major vehicle, and is empty once all are through.
  - A copied state replays identically, and a new run starts unserved.
- **A20:** a green head at the line neither serves the Stop nor lets a Yield vehicle into an
  occupied area; a red head holds a served vehicle.

At the model seam (`rightofway_runtime.*`, `stop_control_model.*`):
- A Yield control's event stream equals no control's.
- Under Stop, every minor vehicle that crossed stood at the line first (more than 20 of them).

Four mutations, each caught by its own test:
- no Stop hold;
- no rest;
- service not carried;
- a reach wide enough to serve a queue.

**A21** (M3.2.6a, D64), `tests/signal_position_tests.cpp` (`signal_position.*`), both driving sides:
- heads before, on and after a cut;
- one head per lane;
- stretch, split (three places) and copy;
- a red head past a cut.

Command station, canvas slot and runtime point agree within 1e-6.

**A22–A23** (M3.2.6b, D64), `tests/queue_counter_tests.cpp` (`queue_counter.*`):
- **A22:**
  - an exact standing queue at a line with no signal;
  - a Stop line's queue measured by reference and by the same point, agreeing;
  - hysteresis stays pinned by `movement.queue_state_has_hysteresis`.
- **A23:**
  - an authored counter over an approach's heads replaces its row with identical numbers;
  - no duplicate row;
  - unchanged movements and compiled scenario;
  - the name appears in `movementJson`, which CLI and editor share.
- The four-leg and M2.6 CLI reports are byte-identical to before the refactor.

**A26 partial** (M3.2.7a/b, D66): `tools/t_junction_network.hpp` → `data/projects/t-junction-priority.traffic.json` (`tjunction.*`) — the minor far turn crosses the near stream then merges separately into the far one; right-hand and its left-hand mirror (the crossing movement is then the minor right turn; identical rows on the same seed), Yield, Stop (more minor delay, same major count), blocked receiving lane (the turn never enters; chain admission and receiving space each suffice, both removed fails); streams never share the crossing; no major-road clamp; sink clearance fits the 12 m heavy vehicle. Controlled cases and the sweep are in the rows below. M3.2.7c (D67), `tjunction_signal.*`: a minor-road head upstream of both waiting lines — always green still has every finished minor vehicle served at the Stop (A20 on the fixture) and changes no minor row; always red holds the whole minor road and no major count changes; a fixed cycle never passes on red and adds minor delay on both driving sides. Uncovered: the owner exercise (§3) — M3.2.7d.

**A24, automatic areas** (M3.2.4c, D68): `tests/automatic_conflict_tests.cpp` (`automatic_conflict.*`) — a T-junction with nothing authored derives exactly one passive crossing (not the diverge, not a Connector against its own Links) and two merges equal to what a take-over stores; a click authors what *Add crossing areas* would; a lane's second area shares and moves its line (D63); levels apart do not conflict; input order does not matter; passive areas compile to no zone. `tests/conflict_auto_ui_tests.cpp` (`conflict-auto-ui`) — drawn only under the tool, a click authors (one Undo step) and a second cycles, Delete makes it passive, a merge click takes over with its priority, `P` on a passive row authors it, Thai statuses, only authored areas saved. Linux only.

**A27–A35** (M3.2.8b, D71): `tests/lane_change_tests.cpp` (`lanechange.*`) — A27 empty road (one change, completes on the full chain, no clamp, no stub arrival); A28 a vehicle alongside refuses, the change follows once it clears; A29 a fast close follower refuses (its braking beyond `comfortableDeceleration` asserted first); A30 a blocked target holds the vehicle at its dead end with zero clamps, then it changes and completes; A31 two changers at one gap, only the lower id goes; A32 no change inside a zone; A33 a copied state replays exactly, completed + active + pending conserved; A34 the four-leg family (left stub two spans, dead end where the left turn leaves; right stub one span); A35 a waiting vehicle beside a steady 10 m/s stream is let in mid-stream with no clamp; validation of unknown routes, bad ranges and an unreachable dead end. Four-leg and M2.6 re-published in `docs/evidence/m3.2.8b-mandatory.md`. **Verified on Windows headless only** — no Qt build locally, and the two shell edits were not compiled here; Linux and Windows CI pending.

**A36–A39** (M3.2.8c, D90): `tests/lane_change_tests.cpp` (`lanechange.*`). A36: a changer at 10 m/s, 80 m short of its dead end beside a 10 m/s stream, goes in before slowing to walking pace, with no clamp. The forcing is the same run without the parameter, where it comes to a stand. A37: in that run some vehicle holds cooperatively (asserted first), and no stream vehicle brakes below −3 m/s². A38: 160 m short of its dead end, refused and outside its look-ahead, no cooperative hold. A39: no moving hold without the parameter, and a copied state replays exactly with it. The T-junction archived-metadata tests restore the pre-D90 behaviour hash only when the new field is the sole change and the fixture has no spans (`sweep::restoreArchivedBehaviour`). **Linux/GCC headless only.**

**A40–A46** (M3.2.8c, D93): `tests/downstream_decision_tests.cpp` (`routeless.*`), on a drawing
where a two-lane entry Link A feeds a two-lane Link D whose lanes leave for L and R, with a
decision on D of 60:40.
- **A40:** the compiled volumes are 360 to L and 240 to R exactly. The forcing: some volume
  rides a stub.
- **A41/A42:** every span starts at or past D on both routes. The forcing: the stub and its
  target start side by side on A, so unclipped spans would reach back. Disabling the clip fails
  this test. On a run: no arrival on a stub, no clamp, both movements carried, and every change
  past A's 100 m.
- **A43:** with a one-lane entry into D lane 1, no chain touches D lane 2, `ROUTING_DECISION_LANE_FIXED`
  is reported, and all 600 go to L.
- **A44:** the three projects, seeds 42–44 with every diagnostic, plus `--events 42`, are
  byte-identical before and after. All 71 CTest entries pass.
- **A45:** a copied state replays identically.
- **A46:** A36 on D. Without the parameter the changer stands and is then let in with no clamp;
  with it, it goes in before it stops.
- **Changed with D93, not regenerated:** `routeless.a_placed_decision_splits_by_destination`.
  The four-leg pocket decision now holds 3:1 exactly, with no South leak and no advisory; it was
  East 0.625, North 0.125, South 0.25.
- **Windows/MSVC only.**

**A24 partial** (M3.2.4a, D60): `tests/right_of_way_editor_tests.cpp` (`rightofway_editor.*`) — lane-pair expansion, one-step edits, take over/restore, delete keeping shared lines, drawn geometry equals the resolver's, every right-of-way code in en/th; `tests/priority_ui_tests.cpp` (`priority-ui`) — add via dialog, Enter-to-edit, Undo, take over/restore, Problems → area, Run note, Thai. M3.2.4b (D61): `tests/priority_canvas_tests.cpp` (`priority-canvas`) — the Conflict area tool picks and cycles by pointer and `P`, each one Undo step, Select still picks the Link; a waiting-line drag along its lane is one step; the two sides are told apart; Save and reopen keep every edit and still run. M3.2.5b (D63): the same suite sets Stop from the dialog, one Undo step, listed and drawn, disabled for an undetermined area, and kept through Save and reopen. M3.2.6c (D65): `tests/queue_counter_ui_tests.cpp` (`queue-counter-ui`) — the Queue counter tool builds a counter from stop lines and a lane place and commits it on Enter as one Undo step (Esc, Backspace, a refused click), the tab adds one over heads selected by keyboard, renames and deletes it (each one step), the Results tab lists the authored row in place of the derived one with the same row count and restores it on delete, Thai headers, and Save and reopen keep, draw and still run it. A24 stays partial until the Windows run and the owner's attempt below. A18–A23, A25–A26 need M3.2.5+ |
| Linux headless/desktop and Windows native CI | D105: Linux headless 52/52 at baseline and after the fix; includes architecture, negative architecture, file-size and frozen-reference checks. Desktop and Windows verification are pending for this change; historical runs above retain their dates. |
| Frozen fixtures and seed-42 regression | M3.2.2a: four TS baselines pass; `trafficsim-cli 42` output byte-identical before/after; the two project fixtures changed only `schemaVersion` 13 → 14 (regenerated by their tools) |
| Same-build replay | D105: all events and mutable snapshot fields replay exactly at every tick of 120 congested T-junction runs (seeds 42–81, headways 3/7/12). Before/after numerical trajectory digests, movement counts/delays and queues are identical; 17 stationary false starts removed, moving clamps preserved. [Raw runs and remaining causes](evidence/m3.2.8a-clamps.md). |
| Controlled gap/headway/clearance outcomes | M3.2.7a (D66), `tjunction_controlled.*` on the fixture's compiled scenario: gapTime 4/5/6 s at 5 s to entry admit/admit/deny on both driving sides; headway 7 m − 0.01/7/+0.01 block/block/pass (equality blocks, as A09); movement case with the clearance margin asserted first (3.8 s against 5 s) and bounds stated before the run — permitted admission within 1.6 s (0.7 s), denied not before the major's rear leaves the area (6.1 s from the trace, predicted 6.10 s; admitted 6.8 s), later arrival. Linux only |
| Seeded sweep reports and unserved counts | M3.2.7b (D66): `docs/evidence/m3.2.7-sweep.{csv,md}`, metadata committed first (`b66313b`). 20 runs, all drained (active = pending = 0). Minor delay and queue rose with gapTime 3 → 5 → 7 s for every seed; **headway 3/7/12 m gave identical rows**, investigated: no major vehicle within 12 m of an entry was slower than 10 m/s, so headway never decided a block at this demand. Clamps 3–10 per run, all minor vehicles at a waiting line (carved to M3.2.8). M3.2.7c (D67): repeated on a congested major road where headway demonstrably decides (asserted before the run, metadata first, `docs/evidence/m3.2.7c-headway.*`): minor queue mean never fell with headway 3 → 7 → 12 m, the near turn's delay rose in every seed, the crossing turn's in two of four; all drained. M3.2.8a (D69): re-run with the commitment rule (`docs/evidence/m3.2.8a-*`, same metadata): clamps 76 → 3 in the gap arm and 83 → 20 in the congested arm (15 amber at the head, 5 minor near-standing, undiagnosed), none a major vehicle at an area; the gapTime and headway responses still hold; all drained |
| Owner, date, Windows version and attempt number | Pending |
| Observed priority, crossing, Stop/Yield and queue behavior | Pending |
| Save/reopen/repeat-seed outcome | Pending |
| Plausibility verdict and unresolved defects | Pending |
| **M3 right-of-way done-condition** | **Open** |
| **M3.2.8 remaining behavior** | **Open** |
| **M6 scientific validation** | **Open; not assessed here** |

### 2026-10-04 — D105 session-fillable evidence (review S5)

The current fixture's five near-standing minor-road events are traced individually in
[the clamp evidence](evidence/m3.2.8a-clamps.md). Four were stationary false starts;
one was a moving vehicle with zero hard-buffer clearance and remains counted. The
failure-first regression and exact boundary check are in `core`; no frozen baseline
was changed. All 120 congested runs preserve accounting and drain, with no body or
swept crossing overlap and no excessive braking hidden by removing a clamp event.
Three committed projects × 40 seeds keep their CLI reports identical except clamp
counts. Their active/pending traffic is recorded, not silently excluded.
M3.2.8a.1 carries the remaining moving and source-insertion cases in ROADMAP/NEXT.
M0 plausibility, M3.2.7d and M6 remain open; owner rows above are not filled by tests.

### 2026-10-04 — D108 source first-step rows (M3.2.8a.1a)

A59–A61 in `tests/source_insertion_tests.cpp` (`core`):
- **A59:** positive clearance smaller than first motion keeps the identical pending vehicle,
  emits no clamp, preserves RNG/copy replay and accounting, then departs once room opens.
  The scheduled time remains original; actual entry is later. Fails before D108.
- **A60:** exact first-step equality admits without a new margin; just below defers.
  The below-bound assertion fails before D108.
- **A61:** exact standstill equality admits at rest without a false clamp; below remains pending.

Linux Release 54/54, frozen references unchanged. Input metadata committed before output.
120 stress cases pass replay/accounting/body/swept/braking checks; all drain, source clamps
3 → 0, moving minor clamps 21 → 21. Four projects × 40 seeds preserve counts and delays;
travel time/queue changes are measured in [the source evidence](evidence/source-first-step.md).
No moving-merge, cross-path 2D, owner plausibility or M6 gate is closed.
