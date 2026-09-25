# M3 acceptance design and evidence record

**Status: M3.2.2a–c automated rows recorded (2026-09-25); owner exercise not performed.** This is a prepared engineering test matrix and owner exercise,
not a passed gate or a scientific validation result. M2.6 remains the prerequisite for
implementation. [M3_CONTRACT.md](M3_CONTRACT.md) defines the proposed behavior;
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

Extend the matrix for M3.2.8 before implementing lane changing. It must cover forward and
rearward safety, required lane-change distance, cooperation, visibility, conflict reservations,
emergency stopping, congestion accounting and replay. Passing A01-A26 alone does not close it.

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

## 4. Evidence record

| Field | Observation |
|---|---|
| M2 gate evidence and pass prerequisite | Passed by the owner's judgment, 2026-09-25 (D53, `M2_GATE.md`) |
| Implementation commit / build / platform | Pending |
| Fixture metadata and data hashes | Pending |
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

**A26 partial** (M3.2.7a/b, D66): `tools/t_junction_network.hpp` → `data/projects/t-junction-priority.traffic.json` (`tjunction.*`) — the minor far turn crosses the near stream then merges separately into the far one; right-hand and its left-hand mirror (the crossing movement is then the minor right turn; identical rows on the same seed), Yield, Stop (more minor delay, same major count), blocked receiving lane (the turn never enters; chain admission and receiving space each suffice, both removed fails); streams never share the crossing; no major-road clamp; sink clearance fits the 12 m heavy vehicle. Controlled cases and the sweep are in the rows below. Uncovered: the signal-composition variant, a congested case that exercises headway end to end, and the owner exercise (§3) — M3.2.7c.

**A24 partial** (M3.2.4a, D60): `tests/right_of_way_editor_tests.cpp` (`rightofway_editor.*`) — lane-pair expansion, one-step edits, take over/restore, delete keeping shared lines, drawn geometry equals the resolver's, every right-of-way code in en/th; `tests/priority_ui_tests.cpp` (`priority-ui`) — add via dialog, Enter-to-edit, Undo, take over/restore, Problems → area, Run note, Thai. M3.2.4b (D61): `tests/priority_canvas_tests.cpp` (`priority-canvas`) — the Conflict area tool picks and cycles by pointer and `P`, each one Undo step, Select still picks the Link; a waiting-line drag along its lane is one step; the two sides are told apart; Save and reopen keep every edit and still run. M3.2.5b (D63): the same suite sets Stop from the dialog, one Undo step, listed and drawn, disabled for an undetermined area, and kept through Save and reopen. M3.2.6c (D65): `tests/queue_counter_ui_tests.cpp` (`queue-counter-ui`) — the Queue counter tool builds a counter from stop lines and a lane place and commits it on Enter as one Undo step (Esc, Backspace, a refused click), the tab adds one over heads selected by keyboard, renames and deletes it (each one step), the Results tab lists the authored row in place of the derived one with the same row count and restores it on delete, Thai headers, and Save and reopen keep, draw and still run it. A24 stays partial until the Windows run and the owner's attempt below. A18–A23, A25–A26 need M3.2.5+ |
| Linux headless/desktop and Windows native CI | Pending |
| Frozen fixtures and seed-42 regression | M3.2.2a: four TS baselines pass; `trafficsim-cli 42` output byte-identical before/after; the two project fixtures changed only `schemaVersion` 13 → 14 (regenerated by their tools) |
| Same-build replay | Pending |
| Controlled gap/headway/clearance outcomes | M3.2.7a (D66), `tjunction_controlled.*` on the fixture's compiled scenario: gapTime 4/5/6 s at 5 s to entry admit/admit/deny on both driving sides; headway 7 m − 0.01/7/+0.01 block/block/pass (equality blocks, as A09); movement case with the clearance margin asserted first (3.8 s against 5 s) and bounds stated before the run — permitted admission within 1.6 s (0.7 s), denied not before the major's rear leaves the area (6.1 s from the trace, predicted 6.10 s; admitted 6.8 s), later arrival. Linux only |
| Seeded sweep reports and unserved counts | M3.2.7b (D66): `docs/evidence/m3.2.7-sweep.{csv,md}`, metadata committed first (`b66313b`). 20 runs, all drained (active = pending = 0). Minor delay and queue rose with gapTime 3 → 5 → 7 s for every seed; **headway 3/7/12 m gave identical rows**, investigated: no major vehicle within 12 m of an entry was slower than 10 m/s, so headway never decided a block at this demand. Clamps 3–10 per run, all minor vehicles at a waiting line (carved to M3.2.8). The headway arm needs a congested variant (M3.2.7c) |
| Owner, date, Windows version and attempt number | Pending |
| Observed priority, crossing, Stop/Yield and queue behavior | Pending |
| Save/reopen/repeat-seed outcome | Pending |
| Plausibility verdict and unresolved defects | Pending |
| **M3 right-of-way done-condition** | **Open** |
| **M3.2.8 remaining behavior** | **Open** |
| **M6 scientific validation** | **Open; not assessed here** |
