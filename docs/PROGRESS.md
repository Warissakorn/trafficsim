# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is the history and the reasoning** — what a session
reads to understand why the code is the way it is. What to do next is in
[`NEXT.md`](NEXT.md); the decision log is at the bottom of this file. Never delete an entry;
move old blocks whole into `docs/archive/` if this gets long. Older entries are preserved there:

- [`archive/PROGRESS-2026-09-25-m3.2.6a-b.md`](archive/PROGRESS-2026-09-25-m3.2.6a-b.md) — 2026-09-25 — M3.2.6a/b: signal positions and queue counters (D64); moved out 2026-09-26 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.5b.md`](archive/PROGRESS-2026-09-25-m3.2.5b.md) — 2026-09-25 — M3.2.5b: Stop/Yield in the editor; one waiting line per lane (D63); moved out 2026-09-26 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.5a.md`](archive/PROGRESS-2026-09-25-m3.2.5a.md) — 2026-09-25 — M3.2.5a: Stop and Yield at the waiting line (D62); moved out 2026-09-26 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.4b.md`](archive/PROGRESS-2026-09-25-m3.2.4b.md) — 2026-09-25 — M3.2.4b: conflict areas on the canvas (D61); moved out 2026-09-26 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.4a.md`](archive/PROGRESS-2026-09-25-m3.2.4a.md) — 2026-09-25 — M3.2.4a: the Conflict areas tab (D60); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.3c.md`](archive/PROGRESS-2026-09-25-m3.2.3c.md) — 2026-09-25, M3.2.3c, shared receiving space, merges on the solver, hold cycles (D59); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.3b.md`](archive/PROGRESS-2026-09-25-m3.2.3b.md) — 2026-09-25, M3.2.3b, spans, atomic chains, authored merges run (D58); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.3a.md`](archive/PROGRESS-2026-09-25-m3.2.3a.md) — 2026-09-25, M3.2.3a, one authored crossing runs (D57); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.2b-c.md`](archive/PROGRESS-2026-09-25-m3.2.2b-c.md) — 2026-09-25, M3.2.2b (controls follow their owners, D55) and M3.2.2c (waiting lines upstream, crossing coverage, D56); moved out 2026-09-25 as the oldest live entries
- [`archive/PROGRESS-2026-09-25-m3.2.2a-scrutinised.md`](archive/PROGRESS-2026-09-25-m3.2.2a-scrutinised.md) — 2026-09-25, M3.2.2a scrutinised, four defects fixed (D55); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m3.2.2a.md`](archive/PROGRESS-2026-09-25-m3.2.2a.md) — 2026-09-25, M3.2.2a, authored right-of-way controls at the file/model seam (D54); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-docs-pass.md`](archive/PROGRESS-2026-09-25-docs-pass.md) — 2026-09-25, docs pass: stale instructions out, session-start reading cut; moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m2-gate-passed.md`](archive/PROGRESS-2026-09-25-m2-gate-passed.md) — 2026-09-25 — M2 gate passed by the owner (D53); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-c2-and-c4-withdrawn-the-owner-judges-the.md`](archive/PROGRESS-2026-09-25-c2-and-c4-withdrawn-the-owner-judges-the.md) — 2026-09-25 — C2 and C4 withdrawn; the owner judges the gate (D51, D52); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m1-accepted.md`](archive/PROGRESS-2026-09-25-m1-accepted.md) — 2026-09-25, M1 accepted by owner ruling, the M2.6 template, the D50 merge deadlock; moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m1-timed-drawing.md`](archive/PROGRESS-2026-09-25-m1-timed-drawing.md) — 2026-09-25, the M1 timed drawing (9 min 40 s); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m2.7b-controllers.md`](archive/PROGRESS-2026-09-25-m2.7b-controllers.md) — 2026-09-25, M2.7b, fixed-time Signal Controllers (D48); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-25-m2.7a-heads.md`](archive/PROGRESS-2026-09-25-m2.7a-heads.md) — 2026-09-25, M2.7a, a Signal head placed where clicked (D47); moved out 2026-09-25 as the oldest live entry
- [`archive/PROGRESS-2026-09-24-m2.1-demand.md`](archive/PROGRESS-2026-09-24-m2.1-demand.md) — 2026-09-24, M2.1.1 and M2.1.2 (routeless inputs, counted turning proportions); moved out 2026-09-25 as the oldest live entries
- [`archive/PROGRESS-2026-09-24-m2-slices.md`](archive/PROGRESS-2026-09-24-m2-slices.md) — 2026-09-24, M1.26.1 through the M3 contract, and the superseded `## Next` blocks; moved out 2026-09-25 as the oldest live entries
- [`archive/PROGRESS-2026-09-23-editor-benchmark.md`](archive/PROGRESS-2026-09-23-editor-benchmark.md) — 2026-09-23, the editor benchmark was measuring itself (D31); moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27.3-reflexes.md`](archive/PROGRESS-2026-09-22-m1.27.3-reflexes.md) — 2026-09-22, M1.27.3, the reflexes counted; moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27.2-slots.md`](archive/PROGRESS-2026-09-22-m1.27.2-slots.md) — 2026-09-22, M1.27.2, a vehicle carries scenario slots; moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-23-compact-workspace.md`](archive/PROGRESS-2026-09-23-compact-workspace.md) — 2026-09-23, the compact desktop workspace; moved out 2026-09-24 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27.1-redraw.md`](archive/PROGRESS-2026-09-22-m1.27.1-redraw.md) — 2026-09-22, M1.27.1, the connector cache; its numbers corrected 2026-09-23; moved out 2026-09-23 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.27-build-stage.md`](archive/PROGRESS-2026-09-22-m1.27-build-stage.md) — 2026-09-22, M1.27 build stage, the PCH and json_fwd work; moved out 2026-09-23 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.26-carriageway-routes.md`](archive/PROGRESS-2026-09-22-m1.26-carriageway-routes.md) — 2026-09-22, M1.26, a route belongs to the carriageway; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.24-one-window.md`](archive/PROGRESS-2026-09-22-m1.24-one-window.md) — 2026-09-22, M1.24, the one-window editor; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.25-pointer-demand.md`](archive/PROGRESS-2026-09-22-m1.25-pointer-demand.md) — 2026-09-22, M1.25, demand drawn by pointer; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-22-m1.22.2-rotation.md`](archive/PROGRESS-2026-09-22-m1.22.2-rotation.md) — 2026-09-22, M1.22.2, selection rotation; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-21-m1.22.1-keyboard.md`](archive/PROGRESS-2026-09-21-m1.22.1-keyboard.md) — 2026-09-21, M1.22.1, editor history and the keyboard workflow; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-21-toolchain-and-3.3.md`](archive/PROGRESS-2026-09-21-toolchain-and-3.3.md) — 2026-09-21, the toolchain install and the §3.3 measurement; moved out 2026-09-22
- [`archive/PROGRESS-2026-09-21-connector-parity-audit.md`](archive/PROGRESS-2026-09-21-connector-parity-audit.md) — 2026-09-21, the Connector parity audit; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-21-m1.21.1-lifecycle.md`](archive/PROGRESS-2026-09-21-m1.21.1-lifecycle.md) — 2026-09-21, M1.21.1, the Network lifecycle correctness audit; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-20-m1.21-authoring.md`](archive/PROGRESS-2026-09-20-m1.21-authoring.md) — 2026-09-20, M1.21, the supplied-spec audit and authoring foundation; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-connector-position.md`](archive/PROGRESS-2026-09-18-connector-position.md) — 2026-09-18, M1.20; moved out 2026-09-22 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-m1.19-lane-middles.md`](archive/PROGRESS-2026-09-18-m1.19-lane-middles.md) — 2026-09-18, M1.19, the Connector lane middles land on the Link lane middles; moved out 2026-09-21 as the oldest live entry
- [`archive/PROGRESS-2026-09-18-square-mouth.md`](archive/PROGRESS-2026-09-18-square-mouth.md) — 2026-09-18, the square-mouth revert (M1.17 reverted); moved out 2026-09-21 when it was the oldest live entry and the parity audit superseded it
- [`archive/PROGRESS-2026-09-18-earlier.md`](archive/PROGRESS-2026-09-18-earlier.md) — 2026-09-18, the snapping audit, the engine profile and the M1.17 revert
- [`archive/PROGRESS-2026-09-18-flush-mouth.md`](archive/PROGRESS-2026-09-18-flush-mouth.md) — 2026-09-18, M1.18, the flush mouth
- [`archive/PROGRESS-2026-09-17-mouth.md`](archive/PROGRESS-2026-09-17-mouth.md) — 2026-09-17, the wedge mouth and the miter "bulge"
- [`archive/PROGRESS-2026-09-17.md`](archive/PROGRESS-2026-09-17.md) — 2026-09-17, later entries
- [`archive/PROGRESS-2026-09-17-early.md`](archive/PROGRESS-2026-09-17-early.md) — 2026-09-17, earlier entries
- [`archive/PROGRESS-2026-09-16.md`](archive/PROGRESS-2026-09-16.md) — 2026-09-16
- [`archive/PROGRESS-2026-09-14.md`](archive/PROGRESS-2026-09-14.md) — 2026-09-14
- [`archive/PROGRESS-2026-09-10--2026-09-15.md`](archive/PROGRESS-2026-09-10--2026-09-15.md) — 2026-09-10 to 2026-09-15

---

## 2026-09-26 — M3.2.8a: commitment at a waiting line (D69)

M3.2.8 was split:
- **a** — commitment;
- **b** (ROADMAP) — lane changing, cooperation and visibility, whose contract
  (`M3_8_CONTRACT.md` §2) is not yet written.

The contract (`docs/M3_8_CONTRACT.md` §1) was committed before the code (`a31c029`).

**The rule:** a vehicle short of a line it gives way at, with `v² > 2 · maxDeceleration · gap`,
cannot stop there. It ignores the headway and gap-time parts of the test at that line.
- It never ignores occupancy, an unserved Stop, receiving space or the swept check.
- One predicate, `committed` (`src/core/conflicts.hpp`), serves both paths: `zoneHold`, through
  the new `ZoneState::majorInside`, and the derived-rule loop in `stepSimulation`.
- It is read off the snapshot and never stored, so a copied state replays exactly.

**The owner's rulings, and one reversed by measurement:**
- The rule covers derived M3.1 merges too, re-publishing the four-leg and M2.6 numbers D59 had
  kept fixed.
- The owner first chose `comfortableDeceleration`. Measured on seed 42, it clamped the major road:
  - at the T-junction's congested variant, an eastbound vehicle was held 0.4 m short of the
    crossing at 7.5 m/s behind a minor driver who had committed about 2 s ahead of it;
  - at an M2.6 merge, a turn committed in front of a through vehicle within headway, and through
    vehicles were clamped standing at the join (36 clamps, against 28).

  Both broke NEXT's gate of zero major-road clamps, so it went back to the owner, who chose
  `maxDeceleration`.
- **Found while re-checking:** a derived rule's span front is clipped at its segment's end, so a
  major vehicle across the join reads `reach == 0`. The committed path therefore skips it.
  - That major vehicle is then on the minor route's next segment, and so is its car-following
    leader, which holds it anyway.
  - The mutation that drops derived-rule occupancy survives for the same reason; the contract
    says so.
  - The comfortable figures above were re-measured after checking this and did not change.

**Results** (`docs/evidence/m3.2.8a-commitment.md`; rows `m3.2.8a-sweep.csv`,
`m3.2.8a-headway.csv`; the M3.2.7 metadata still describes the inputs):
- **T-junction clamps:**
  - gap arm 76 → 3;
  - headway runs 52 → 0;
  - congested arm 83 → 20. Of those, 15 are eastbound vehicles at the fixed-time head (D36
    amber) and 5 are minor vehicles standing or at walking pace within 1 m of the merge line,
    **not diagnosed**.
- **No clamp is a major vehicle at a conflict area. All runs drained.**
- Minor delay fell by 0–11 s, and the gapTime and headway responses still hold per seed.
- **Four-leg:** byte-identical.
- **M2.6:**
  - clamps 28 → 23; the 5 at merge lines are gone;
  - movements move by at most 0.13 s;
  - `m26study.a_minor_vehicle_held_at_the_join_itself_*` now asserts more clamps with lines on the
    join than with the D50 setback (37 against 22). Pending demand now appears on only 2 of 7
    seeds, since a driver who cannot stop no longer waits on the join.
- **Cost:** +0.25% instructions (callgrind, M2.6 hour); wall time within the clock's spread.

**Tests:**
- `commitment.*` (6 tests), each with its forcing asserted first:
  - a committed driver goes through a gap closed by anticipation, with no clamp;
  - a driver able to stop still waits;
  - occupancy still holds it, through `zoneHold` itself and not only the swept check;
  - equality can stop;
  - a derived rule, go and wait;
  - replay.
- `tjunction.commitment_removes_the_minor_clamps_*` reads the archived CSV.
- Two tests whose forcing was a clamp now observe standing vehicles instead:
  - `conflict_zone.congested_*` counts minor vehicles standing at the line;
  - in `conflict_zone.a10_*` the minor vehicle can now stop.
- Mutations caught:
  - zone occupancy ignored;
  - `>=` at the boundary;
  - the derived loop not using the predicate.

## 2026-09-26 — M3.2.4c: automatic conflict areas (D68)

**The owner's ruling**, given while looking at the T-junction in the editor:
- conflict areas should appear on their own wherever roads overlap, as in Vissim;
- the author only changes priority, with the Conflict area tool;
- an unset crossing is **passive**, as in Vissim;
- merges are shown too.

This supersedes D61's "an unauthored crossing is not an area".

- **Model** (`src/model/network/automatic_conflicts.cpp`, `automaticConflicts`), derived from the
  drawing and never stored:
  - **Crossings:** every pair of lane paths from two different roads on one level whose surfaces
    overlap (`surfaceOverlap`) and that no authored area covers. Excluded: a Connector against
    its own Links, and two Connectors leaving or entering the same lane. Crossings are passive,
    so they compile to nothing; runtime is exactly what it was.
  - **Merges:** each pair of every automatic merge group, from the same `mergeSide` that
    `takeOverMerge` now uses (moved out of the commands), so what is shown is what a take-over
    stores.
  - Output is in key order, whatever the input order.
- **Command** `authorAutomaticConflict`:
  - a passive crossing becomes one area for that lane pair, a turning Connector giving way to a
    Link, sharing (and moving upstream) the lane's line (D63);
  - a merge is taken over whole;
  - Delete (`removeConflictArea`) makes a crossing passive again by derivation.
  - `addCrossingAreas` was **not** refactored onto it: its id allocation order is what the
    committed T-junction file and its evidence hashes record.
- **Editor:**
  - Under the Conflict area tool only, passive crossings are drawn grey and dashed, and merges
    dashed in their derived colours.
  - A click where no authored area is authors the automatic one.
  - The tab lists automatic rows after authored ones, with no id. Enter or `P` authors a row;
    Delete and Restore stay disabled on them.
  - `priority-ui` now counts authored and automatic rows separately; before, it counted every
    row.
- **Cost:** 3.4 ms per revision on the four-leg and M2.6 templates in a Release build (29 ms in
  Debug). It is computed once per revision, and only while the tool or the tab is in use.
- **Test runs:** Linux headless 28/28, desktop 46/46 offscreen. Seed 42 and the four-leg and M2.6
  reports are unchanged. Three mutations were caught: the diverge exclusion removed, the merge
  sides computed apart from the take-over, and the click not authoring.

## 2026-09-26 — M3.2.7c: signal composition and a congested major road on the T-junction (D67)

Two new builder options (`tools/t_junction_network.hpp`), both off by default, so the committed file
and the M3.2.7b metadata are unchanged; `tjunction_signal.the_base_fixture_is_untouched_by_the_new_options`
checks the defaults.
- `minorSignal`: a fixed-time head on the minor Link 1 m before its end. That puts it upstream of
  both minor waiting lines, which lie on the Connectors.
- `congestedMajor`: a fixed-time head on eastbound 16 m past the crossing (30 s green, 3 s amber,
  27 s red).

**Signal composition** (`tjunction_signal.*`, A20 on the fixture):
- **Always green with Stop:** every minor vehicle that finishes was served at a Stop line, and the
  minor movement rows equal the no-signal Stop run exactly. Green is the head's permission only.
- **Always red:**
  - The forcing: the minor queue counter's maximum is above zero.
  - No minor trip completes and no minor front passes the head.
  - The major counts equal the open run's.
- **Fixed cycle, both driving sides:**
  - No minor front passes on red; trips complete; the run drains.
  - The streams never share the crossing, and the major road has no clamp.
  - Both minor delays are above the gap-test-alone run's.
- **Mutations:** heads forced always green fails the red and cycle tests; Stop service removed
  fails the green test.

**Congested major road, the headway exercise:** the M3.2.7b headway arm said nothing, because at
free flow the gap-time window always covered the headway.
- The first placement, 50 m past the near-turn merge, congested only the merges: its queue never
  reached the crossing, and the crossing-only forcing test failed. The head was moved to 16 m past
  the crossing's exit.
- With headway 12, some ticks on seed 42 then have a major vehicle within 12 m of the crossing
  entry but outside the gap window. This is asserted before any headway row exists
  (`a_congested_major_road_lets_headway_decide`); the variant drains, the streams never share the
  crossing, and the major road has no clamp.

**The headway arm's metadata is committed first**, as in M3.2.7b:
`docs/evidence/m3.2.7c-headway-metadata.json`, written by `trafficsim-t-junction-sweep
--metadata-congested`.
- The variant is not a file of its own, so the metadata records the added head's link, station and
  phases exactly, as the builder reads them back.
- It does not hash the variant's JSON text: that text includes computed Connector geometry, which
  MSVC may round differently in the last bit.
- A test checks the metadata still describes the variant.

**The headway arm** (`docs/evidence/m3.2.7c-headway.*`, run after `e5e1e5f`): all 12 runs drained.
Headway now changes the outcome:
- The minor queue mean never fell from 3 to 7 to 12 m in any seed.
- The near turn's delay rose in every seed. It merges 4 m past the head's line, so standing major
  vehicles are within 7 and 12 m of its entry, but not 3.
- The crossing turn responded in two of four seeds.
- Eastbound delay rose to 13.6–17.1 s. There are 5–9 clamps per run.

**Owner exercise → M3.2.7d.** The owner exercise (§3) needs the owner and Windows, so it is carved as
M3.2.7d and stays pending. `docs/M3_ACCEPTANCE.md` §3 gains a recording sheet.

## 2026-09-25 — M3.2.7a/b: the T-junction evidence (D66)

M3.2.7 was split:
- **a** — the fixture and the controlled cases;
- **b** — the diagnostic sweep;
- **c** (ROADMAP) — the signal-composition variant and the owner exercise. The exercise needs the
  owner and a Windows build, so it is pending and never inferred.

**Fixture** (`tools/t_junction_network.hpp`), built only through the editor's commands, as the four-leg
fixture is. The committed file is `data/projects/t-junction-priority.traffic.json`, and
`tjunction.committed_file_is_the_builder_output` keeps the two identical.
- **Layout:** a two-way major road with a 1 m median, and one minor approach. The minor road's far
  turn crosses the near major stream (`addCrossingAreas`), then merges into the far one
  (`takeOverMergesOf`, with the minor side yielding). Its near turn only merges.
  - Yield or Stop is set at the lines where the minor road first gives way.
  - The queue counter measures at both of those lines.
- **Variants:** right- and left-hand (a mirror in y, in which the crossing movement is the minor
  RIGHT turn, as the header says), Yield and Stop, and a blocked exit (a permanently red head on the
  far lane past the merge).
- **Demand:** round numbers, with inputs stopping at 900 s in a 1500 s run so demand drains.
- **Geometry fixes found while building it:** the first drawing had no median. The turn's lane then
  touched the near stream right up to its end, so the crossing area ran into the merge. A 1 m median
  and a 20 m join fixed it.

**What the fixture shows** (`tjunction.*`):
- One crossing zone and two merge zones in every variant. On the crossing turn the merge begins past
  the crossing's exit, against a different major stream.
- The sink clearance fits the heavy vehicle (12 m).
- Every variant runs and drains. The two streams are never inside the crossing at once, and the
  major road has **no** safety clamp.
- The mirror gives identical movement and queue rows on the same seed.
- Stop delays both minor movements more than Yield, and leaves the major count unchanged.
- **Blocked exit:** the crossing turn never enters, and eastbound completes exactly as many trips as
  with the exit open. Two guards hold the turn, each enough alone:
  - the crossing and the merge are one chain (the 6.8 m between them is less than the heavy
    vehicle's waiting room);
  - the receiving-space rule.

  Disabling either alone passes the test; disabling both fails it.

**Controlled cases** (`tjunction_controlled.*`, test-owned states on the compiled fixture, no demand):
- **gapTime 4/5/6 s**, with the major 5 s from entry (§2's example): admit, admit, deny, on both
  driving sides.
- **Headway 7 m ± 0.01 m** with time blocking inactive: block, block, pass. **Equality blocks,**
  as in A09.
- **Movement level:** the clearance margin is asserted first (3.8 s from standstill against 5 s).
  The bounds were stated before running:
  - permitted admission within 1.6 s — observed 0.7 s;
  - denied admission no earlier than the major's rear leaving the area, which the run's own trace
    shows at 6.1 s, and within 10 s of the predicted 6.10 s — observed 6.8 s.

  The denied trip arrives later.
- **Mutations**, each failing a named test: the gap `<` → `<=`, the headway `<=` → `<`, and Stop
  service removed.

**Found, not fixed — minor-road safety clamps.** A waiting line closes the tick a major vehicle
enters the gap-time window. A minor driver already too close to stop is then clamped: it arrives at
about 10 m/s, 0.2 m short of the line. There is no commitment or amber rule for a waiting line.
- Seen as 4 clamps in the Yield base run (seed 42), all on the minor approach. The tests assert
  that the major road has none; they do not freeze the minor count.
- This is M3.2.3's admission model, not the fixture, so it is carved into the M3.2.8 ROADMAP row
  rather than tuned away here.

**Sweep metadata first.** `tools/t_junction_sweep.*` records:
- duration, time step, inputs, seeds and rules;
- FNV-1a hashes (carriage returns dropped) of the project file and every catalog the compile
  reads;
- the build.

It was committed as `docs/evidence/m3.2.7-sweep-metadata.json` **before** the sweep ran.
`tjunction.the_archived_sweep_metadata_still_describes_the_fixture` fails if the fixture or a
catalog changes under it.

**Sweep** (b, `docs/evidence/m3.2.7-sweep.*`, 20 runs, run after the metadata commit `b66313b`):
- Every run drained.
- Minor delay and queue rose with gapTime 3 → 5 → 7 s for every seed; the spread between seeds is
  wide.
- **headway 3/7/12 m gave identical rows.** Investigated: no major vehicle within 12 m of an entry
  was ever slower than 10 m/s, so the gap-time window always covered the headway and headway never
  decided a block. The arm exercises nothing at this demand, and a congested variant is left to
  M3.2.7c.
- All 60 clamps in the gap arm were minor vehicles at a waiting line: the carved mechanism.

## 2026-09-25 — M3.2.6c: queue counters in the editor (D65)

M3.2.6c closes M3.2.6. Counters are now authored, listed, drawn and deleted in the editor.
- **Queue counter tool** (`Q`, `Tool::counter`). Each click adds one line to a draft:
  - a click on a stop line adds a reference to that head;
  - a click on a waiting line adds a reference to that line;
  - anywhere else on a **Link** lane, it adds an explicit point, through the new
    `laneControlPoint`. That function maps the lane-polyline station `nearestHeadSlot` picks onto
    the reference station a `ControlPoint` stores.

  Keys:
  - Enter commits the draft as one counter, one Undo step.
  - Backspace drops the last line; Esc drops the draft.
  - A head clicked twice is one line.
  - A Connector path, or empty ground, is refused.
- **Queue counters tab** (`src/shell/editor_counters.cpp`, index 10), with columns Id, Name, Lines,
  Replaces approach. Its actions:
  - *Add queue counter* over the heads in the selection, which is the keyboard route: select heads
    in the Signal heads table, then add;
  - *Edit* (Name) from Enter or a double-click;
  - *Delete*, which brings the derived row back.
  All of them go through `execute`.
- **Marks:** counter lines are violet dotted bars above stop and waiting lines, taken from the same
  helpers that draw or resolve their objects (`headBar`, `waitingLineBar`).
- **One source for the tab and the report:** `queueRowName` and `replacedApproaches`
  (`project/evaluation.hpp`). `evaluationSpec` now suppresses a derived row through
  `replacedApproaches`.
  - The four-leg and M2.6 reports are **byte-identical** to the previous commit's CLI, and so is a
    project that carries an authored counter.
  - Disconnecting the suppression fails the UI suite.

Tests:
- Headless: the lane click lands on the clicked place on a curve (both sides; lane and reference
  stations differ by more than 0.1 m there), and the tab's helpers agree with the report.
- `queue-counter-ui` (offscreen) on the four-leg template:
  - Esc and a refused click, then three stop lines and a lane place committed with Enter; Undo and
    Redo.
  - Rename through the dialog; the Results tab shows "Counted" in place of the pocket's row, with
    the same row count.
  - Keyboard add, Delete, then Undo of each.
  - Deleting the counter restores the derived rows.
  - Thai headers; Save and reopen.
- Three mutations are caught by this suite: Enter not committing, stop lines not recognised, and
  suppression disconnected.

Other changes:
- The suite's `require` prints before it throws. A failing assertion otherwise aborted in
  `~EditorWindow` (`map::at` while a window with unsaved edits closes during unwind), which hid the
  message. That behaviour is older than this change and is only seen on a failure path.
- Oldest entries (M3.2.2b, M3.2.2c) moved to `docs/archive/` to keep this file under 500 lines.

Test runs: Linux headless 28/28, desktop 45/45 offscreen. Seed 42 is unchanged. Not run on Windows.

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
| Q5 | Final product name | Nothing | Deferred until the end of M1 by D11; **M1 is accepted (D49), so it is now the owner's call** (NEXT item 2). Candidates and collision findings are in the D11 row; reuse them. |
| ~~Q6~~ | ~~Register `velk` on npm and PyPI~~ | — | **Withdrawn 2026-09-11 as moot** — no settled name to register. The registration question returns with the name at M1. |

---

## Decisions

Non-obvious choices **and the reasoning**. Without the reasoning a later session will
"improve" a decision away and break something invisible.

| # | Date | Decision | Why | What would make it wrong |
|---|---|---|---|---|
| D1 | 2026-09-10 | **Own simulation engine, not a front end over an existing one** | Engineering use needs conflict areas and priority rules as authored inputs, signal heads anywhere on a link, and vehicle-owned driver behaviour, and the two things the job is actually paid for — per-movement evaluation and multi-run averaging — have to be built regardless. Owning the engine makes all of them first-class. See `PROBLEM.md` §2. | If the M2 gate (D38) shows an engineer cannot complete a real study with it. This is the single most expensive decision in the project and it has an explicit test. |
| D2 | 2026-09-10 | **Link-based network model natively; junctions are derived, not authored** | This is how the audience thinks and it is the whole point of D1. Translating to a node–edge model would reintroduce the impedance the prior effort spent six milestones papering over. | If deriving junction geometry from links proves intractable at M1. |
| D3 | 2026-09-10 | **TypeScript everywhere to start; `core/` written so it can be ported to a compiled language later without touching anything above it** | Microsimulation is CPU-bound and a compiled core is probably where this ends up. But picking a stack the user cannot run today, to solve a performance problem not yet measured, is the classic way to stall at milestone 0. Hard rule 1 (`core/` imports nothing) makes the port a contained job later, and makes it measurable first. | If M0 cannot reach real-time on a single intersection — then port immediately rather than optimising TypeScript. |
| D4 | 2026-09-10 | **Desktop is a constraint from day one, a milestone at the end** | Web-first keeps the development loop fast; a framework-free core plus isolated rendering means desktop packaging is packaging, not a rewrite. A boot smoke test in a desktop shell runs from M1 so it never becomes a surprise. | If a required capability (native file dialogs, offline licensing) turns out to need a different shell architecture. |
| D5 | 2026-09-10 | **A results screen carries a "not yet validated" marker until M6 passes** | Numbers from this tool go into documents submitted to regulators. An unvalidated engine that looks authoritative is worse than no tool. | Nothing. This one is not negotiable before M6. |
| D7 | 2026-09-10 | **International audience from the start, not Thailand-first** | Nothing in the engine is jurisdiction-specific, and the parts that are — LOS thresholds, report layouts, units — are data, not code, so building them swappable costs little now and a retrofit costs a lot. Three concrete consequences: HCM is the default LOS pack with others as swappable data; metric internally with display units switchable; **left-hand and right-hand traffic is a first-class network setting from M1** (Thailand, UK, Japan, Australia all drive left — a prior effort never implemented it at all). | If it turns out every real user is in one jurisdiction and the generality is unused weight. |
| D8 | 2026-09-10 | **The M2 gate is performed by the project owner alone, not three independent engineers** | The owner is a practising traffic engineer and no outside participants are available. Accepted with eyes open: this is **a materially weaker test than the one D1 needs**, because the person judging whether the tool is usable for a real study is the same person who chose to build it. Mitigation, mandatory: **the pass/fail criteria are written down and committed before M2 implementation starts**, so the judgement cannot be rationalised after the fact. Adding outside engineers later strengthens the gate and is never wasted. | Nothing makes it wrong; it is simply weak. Treat a pass as "not disproven", not as "confirmed". |
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

| D18a | 2026-09-14 | **Diagnostics resolve object IDs in the model layer; `ValidationIssue` stays frozen** | Adding an `objectId` field to `ValidationIssue` in `src/core/types.hpp` would have compiled — no test compares a whole issue — but it is the wrong boundary. `core/` validates a `Scenario` whose only objects are segments, so the field would be empty for nearly every core code, and it would duplicate what the index path already locates (hard rule 3). Deriving the ID on read from the index path keeps `src/core/` untouched by M1.5 entirely, which is also what puts the four frozen TypeScript baselines and the trajectory digest structurally out of reach. | If an index path ever needs to survive an edit and be re-resolved later, a stored ID becomes the cheaper representation. It is not needed for a panel that is rebuilt per revision. |
| D18b | 2026-09-14 | **Draft validity blocks an edit; runtime supportability only informs, and only on demand** | These answer different questions and must not be merged. `validateNetwork` asks "is this a coherent drawing" and `History::execute` rightly refuses anything else. `validateScenario` asks "can the M0 core run this", and the authoring model deliberately expresses things the core cannot yet run — a merge is the standing example. Making the second blocking would forbid legal authoring; making the first advisory would let broken documents be saved. **A structural consequence the UI must own: because `validateDocument` throws on every non-`EMPTY_NETWORK` issue, a committed document can never carry a draft-invalid object, so the draft list is fed only by a blank document and by the issues of a *rejected* edit** — which is exactly where the object IDs earn their keep. | Nothing, unless the runtime gains merge arbitration, at which point `UNSUPPORTED_MERGE` stops being a finding. Do not "fix" the usually-empty draft list by loosening commit validation. |
| D18c | 2026-09-14 | **M1.5 tables cover network objects only; demand tables carve out to M1.5.1** | Routes and vehicle inputs have no model struct — they are untyped JSON under `ProjectDocument::definition` — so tabling them means designing an authoring demand model, which is M2's subject. Diagnostics still name routes and inputs by their own IDs, which is what the milestone actually required. Carved into a numbered milestone in the same session rather than left as a note, per hard rule 8. | If M2 demand authoring lands first, M1.5.1 is absorbed into it rather than done separately. |
| D18d | 2026-09-14 | **Vehicle-type and behaviour findings are withheld, not reported, when no catalog is loaded** | Those catalogs live in `data/`, not in the project file, so a document alone genuinely cannot resolve them. Reporting `UNKNOWN_VEHICLE_TYPE` for every input of an otherwise valid M0 fixture would blame the drawing for an absence that is by design, and would train users to ignore the panel. One `EDIT_NO_CATALOG` row says what was not checked instead. | When M1.7 resolves catalogs at run handoff, the check becomes real and the withholding should be removed rather than left as a permanent blind spot. |
| D19a | 2026-09-14 | **Keep M0 scenarios and editor projects as two formats; classify the file instead of merging them** | The reported 304 was a category error, not a corruption: a drawn network legitimately has no `definition`, and the simulation window had no way to tell a project from a scenario because both matched `*.json`. Merging the two schemas would have removed the failure by making every drawing claim it is runnable, which is precisely the fidelity claim hard rule 4 exists to prevent — a drawing has no demand, so it cannot run, and the format should keep saying so. Classifying the file before any field is read, and routing a project to the window that can open it, fixes the user's actual problem without that claim. | If M1.8 gives projects a real Run **and** demand authoring (M1.5.1) makes `definition` non-optional in practice, the distinction stops earning its keep and one format becomes honest. Converge then, not before. |
| D19b | 2026-09-14 | **Load errors carry a code on a typed exception, not a formatted message** | The shell already translates `EDIT_*` codes by locale key (`EditorWindow::showError`); the simulation window instead concatenated `e.what()`, which is how nlohmann's text reached a user running `--language th`. `ScenarioLoadError` carries file, code and detail separately so the shell can translate, show the path, and offer an action, while an unknown parser detail still falls back to raw text rather than a blank dialog. One error channel, one lookup, two windows. | If load errors ever need structured per-object issues the way `ValidationError` does, promote the code to an issue list rather than growing the string. |
| D19d | 2026-09-14 | **Error codes are resolved in exactly one place per window; no caller formats `what()` itself** | The first cut of D19b gave `ScenarioLoadError` a code but left three callers printing `what()`, which for a classification failure *is* the bare code — so `--scenario` on an editor project went from an unreadable nlohmann string to an unreadable identifier. Worse, not better: the user lost the one sentence the old message did contain. `MainWindow::explain` and `EditorWindow::openFileOrReport` are now the only places a code becomes text, and startup no longer dies on a file it could have explained. | If a third window appears, the two `text()` lookups become genuine duplication and should be hoisted to a shared locale helper rather than copied a third time. |
| D19c | 2026-09-14 | **The parity review books three milestones and deliberately leaves seven gaps unbooked** | `VISSIM_PARITY.md` §6 ranks ten gaps; only in-editor Run, the sidebar/gesture set and levels/display types are carved into `ROADMAP.md`. The rest — editable object tables, group drag, and the absent Vissim object types (priority rules, stop signs, reduced speed areas, conflict areas) — need engine behaviour that does not exist yet. Booking authoring for an object the core cannot honour would invite a user to believe it is modelled, and would put a date on work whose prerequisites are unscheduled. Recording them without a milestone keeps the roadmap true (`ROADMAP.md` rule 2) while keeping the finding. | When M3 lands right-of-way, conflict areas and priority rules stop being unhonourable and should be booked immediately — the review section is the list to work from. |
| D20 | 2026-09-17 | **Implement M3.1 — merge arbitration by gap time and headway — rather than carve M1.11.1's second half out** | A Connector arriving inside a lane body is structurally a merge: the section downstream of the arrival has two predecessors. D13 forbids accepting that without gap acceptance, so M1.11.1's done-condition ("leaves **and** enters") was unreachable. The owner, shown the choice, chose to build the necessary part of M3 instead of deferring. Scoped to the one M3 bullet it needs — a priority rule with real seconds and metres — reusing the red-signal clamp rather than a second braking path, and relaxing `UNSUPPORTED_MERGE` **by construction** (n−1 of n predecessors must yield to another) rather than by removal. Filed at its number; **M3 is not closed** and its done-condition is not met. | Nothing about the mechanism. But if a later session reads M3.1 as "M3 is underway", the gate discipline is lost: M3 still owns conflict areas, authorable priority rules, stop/yield control, crossing conflicts and heads anywhere on a link. |
| D21 | 2026-09-17 | **Lane sections are derived every compile, never persisted** | `ROADMAP.md` forbids persisted duplicate runtime networks, and sectioning changes no authored id, so it is a pure function of the drawing — unlike `splitLink`, which must rewrite routes because the ids an author stored really do change. `sectionId(laneId, 0)` returns `laneId`, so an uncut lane compiles to exactly the `Segment` it always did, which is what keeps the four frozen baselines valid; breaking that one identity fails 54 tests. Route expansion lives **inside** `buildScenario` because `validateAuthoredDemand` compiles on every save, and anywhere else would break saving. | If sectioning ever becomes expensive enough to matter, cache it beside the document — but never store it in the project file, and never let an authored route name a section id. |
| D22 | 2026-09-17 | **A Connector's `laneMarkings` is indexed per interior divider, not per lane** | Vissim's `Lanes` tab field is per lane, but a lane has two edges and there are `paths + 1` boundary lines, so per-lane does not map onto them unambiguously — any choice is a choice. Per divider is complete and unambiguous, and the two outer edges stay solid because they are the edge of the carriageway, not a lane divider. **Not verified against Vissim**, and recorded as a chosen representation rather than a parity claim (rule 4). | A look at real Vissim showing the field means something else. The change is small — the vector's length and one index — so it was not worth blocking M1 to confirm. |
| D23 | 2026-09-17 | **The reported miter "bulge" is closed as a measurement error; `offsetGeometry` is unchanged** | Measured on 90.47° of deflection: 9.9403 m along the cross-section at the mitered vertex, 7.0425 m perpendicular point-to-polyline, and **exactly 7.000000 m projected across the leg**. The first is `width/cos(φ/2)`, which is what the intersection of two offset legs is — the diagonal of a correct mitered joint, not a bulge. The 8.698 m on record is the same identity at a gentler bend. Removing the miter would reinstate the pinch it exists to fix (30% at a right angle), and `network_tests` pins it to 1e-9. What was actually missing was an **upper** bound on width; it is now asserted exactly on every interior leg, and catches a 0.1% error. | Nothing, unless Vissim is shown to cut corners rather than miter them. The standing lesson: a distance between two boundaries is a width only when taken square to the road — `perpendicular()` says so in its comment, `apart()` does not, and the 24% figure was taken with `apart()`. |
| D24 | 2026-09-22 | **Retire the M0 harness window; the editor is the only window** | The owner asked whether it could go, and the measured answer is yes: `trafficsim-cli` already prints a superset of the figures it showed (`meanDelay`, `safetyClamps`, plus `--events`), and `parseDocument` needs only `network`, so the editor opens bare M0 scenarios already. Keeping a second window meant a second renderer (`src/render/`), a second run loop and a second place for the delay figure to drift from the CLI. What makes the removal safe is not the deletion but the replacement: `scenario-run-ui` pins the editor's run of `crossing.json` to the CLI baseline exactly (31 trips, mean delay 29.249359418430977), so the M0 plausibility observation changed surface without changing meaning. The gate itself was not touched — only the sentence naming where the observation is made. | If a results screen ever needs to run a scenario without the authoring surface (a batch review window, M5), build it on `runSimulation` and the event stream, not by restoring `MainWindow`. If the editor ever stops opening bare M0 scenarios, this decision is void and the CLI becomes the only M0 surface. |
| D25 | 2026-09-22 | **An authored route names Links and Connectors; the per-lane routes are compiled** | The owner's ruling: *Routing เป็นการสร้างบนทุกช่องจราจร … ไม่ได้สร้างเป็นรายช่อง*. A lane-level route tied demand to a Connector's lane count, so narrowing one either invalidated the route or had to be refused — and it was refused, which left an author unable to correct their own drawing. Naming the objects makes the lane chains derived data, which is where they belong: `routeLaneChains` recomputes them on every compile, the reference guards disappear because nothing a route names can vanish, and a vehicle input becomes what Vissim's is, a Link total split across the lanes that carry it. The single-lane expansion deliberately keeps the authored id, which is what lets the four frozen baselines replay unchanged. | What it costs is the lane-specific route: a turn pocket where only the left lane may turn cannot be expressed. If that has to come back before M2.1's positioned routing decision, the honest fix is an explicit lane restriction on the route, not a return to lane ids — the ids were the thing that made every Connector edit fragile. |
| D26 | 2026-09-22 | **A Connector gesture connects every lane of both Links by default** | This reverses the M1.12 default of one lane per drag, which existed for a good reason at the time: pre-filling the maximum authored a wide Connector from a single-lane gesture, and the author could not narrow it afterwards once a route used it. D25 removes that trap, and the owner asked for the wide default explicitly — *ให้เชื่อมจำนวนช่องตามจำนวนช่องใน Link ทั้งหมดก่อน ค่อยปรับลงภายหลัง*. | If authors start drawing single-lane turns more often than full carriageways, make the default follow the drag width instead of flipping it back: the dialog already has both counts, and the gesture knows which lane it started on. |
| D27 | 2026-09-22 | **A precompiled header holds third-party headers only** | `trafficsim_shell` compiles 26 Qt translation units and re-read the same 2.46 s of `<QGraphicsView>` in every one of them; precompiling it and sharing that PCH with the eleven single-source UI test executables through `REUSE_FROM` took a clean build from 86 s to 64 s. What is deliberately *not* in it is any project header: a PCH over a header that changes turns every edit to it into a full-target rebuild, which is the cost this exists to remove — measured, touching `canvas.hpp` still rebuilds 38 objects and no `.gch`. Adding `<nlohmann/json.hpp>` was tried and reverted: it grew the `.gch` by 2 s on `src/shell`'s critical path and cancelled the saving (258 edge-seconds against 243 without). | If the shell is ever split into two targets, give each its own PCH rather than one shared across different flag sets — GCC rejects a `.gch` whose macro state differs, silently, and the only symptom is the speed-up quietly disappearing. |
| D28 | 2026-09-22 | **Derived geometry is cached against the values it is derived from, never against a revision** | The canvas recomputed every Connector's ribbon on every frame, which callgrind put at 67% of a run. What makes the cache safe is not the speed-up but the key: `connectorPaths` and `connectorBoundaries` read the Connector, the two Links it names and the driving side and nothing else — checked in the source, not assumed — so comparing those four values is exactly as strong as recomputing. `ProjectDocument::revision` was rejected as a key for two independent reasons: `History::undo` restores an older revision, so the number is not monotonic, and `History::reset` continues from the file's own revision, so two documents can share one. A preview object differs by value and misses, which is the behaviour a drag needs anyway. | If a future `connectorBoundaries` starts reading a third object — a neighbouring Connector at a shared station, say, which §3.3 may force — this key silently goes stale. The defence is the test that widens a Link the Connector does not name: extend it the same way for whatever the new input is, and make it fail first. And do not generalise the pattern by reflex: the same cache for Link geometry was measured and reverted, because recomputing a polyline is cheaper than proving the cache is still valid. |
| D29 | 2026-09-22 | **A runtime vehicle names its scenario objects by SLOT; only the boundary uses names** | A `Scenario` is immutable and canonically sorted from `createSimulation` onwards, so an index identifies exactly what an id identified — and a tick copies, sorts and compares the whole vehicle list, which three `std::string`s per vehicle made the engine's largest cost (17.4% string copying, 11.4% the sort, 10.3% `resolveRefs`). Slots halved the run. What keeps it honest is that ids become slots only through `detail::byId`, whose first-occurrence rule is exactly what the lookups it replaced returned, and that **names survive at the boundary**: events carry `routeId`, `pendingJson` takes the `Scenario` and emits the same three strings, so the frozen fixtures are byte-identical and a human still reads names. The reverse direction — a slot escaping into a file or an event — is the thing to refuse: a slot means nothing outside the Scenario it indexes. | If a `Scenario` ever becomes mutable after `createSimulation`, or if anything re-sorts one mid-run, every slot in flight is wrong at once and silently. That is the invariant to defend, not the indices. `stepSimulation` already asserts `state.inputs` is parallel to `scenario.inputs`; add the same kind of assertion for anything else that starts indexing the scenario. |
| D30 | 2026-09-22 | **`Ctrl`+left-click stays a dead end; it does not duplicate** | The owner's ruling, asked in user-facing words during M1.27.3 and answered *ไม่มีอะไรเกิดขึ้น*. `VISSIM_PARITY.md` had ranked this High since 2026-09-14 as a *collision* — the chord that duplicates in Vissim extending the selection here — but `trafficsim-gesture-walkthrough` measured it doing **nothing at all** on an already-selected object, which is the only case a Vissim user's hand reaches for. So the choice offered was not "take a verb away" but "fill an empty slot", and the owner declined to fill it. Nothing is lost: `Shift`+click extends a selection and `Ctrl`+drag duplicates. | Do not re-open it from the §1/§2 text alone — those rows describe the 2026-09-14 editor. Re-run the walkthrough first. If a future session gives `Ctrl`+left-click any verb, it must not be one that edits the drawing without a visible result, which is what made this chord dangerous on paper.
| D31 | 2026-09-23 | **A Qt benchmark pumps the event loop between iterations, or it is measuring Qt's deferred work instead of the code** | `QGraphicsScene::clear()` defers reclaiming its index entries to the event loop. A tight timing loop that clears and refills the scene therefore measures an index growing without bound: six identical batches climbed 28 → 159 ms, and the same loop with `processEvents()` stayed flat at 11.5. The harness's answer depended on how many repetitions it was asked for, which is the signature of this class of bug. | Any future harness that drives a Qt object must pump the loop, and any number produced by one that did not must be re-measured before it is quoted. The tell is a result that changes with the repetition count — check that before trusting a Qt timing, the way an interleaved A/B is checked against the noise floor. |
| D32 | 2026-09-24 | **Per-lane demand shares live on `VehicleInput` itself, as optional weights in `routeLaneChains` order** | `VehicleInput` is not wrapped by a separate editor type — `AuthoringDefinition` reuses `ScenarioDefinition`'s `VehicleInput` list directly (`src/model/demand/definition.hpp`) — so it is the only place both the editor and `buildScenario` already read, and the "schema-and-signature decision" the milestone named. Rejected: keying by lane id (the chain's starting lane), which is more robust to the network's lane count changing under an authored route (M1.26's whole point) but adds indirection nothing here asked for yet. Chose the plain positional vector instead, with a deliberately cheap safety net — a size that no longer matches the route's current lane count, or any non-positive weight, degrades to the M1.26 equal split rather than misapplying a weight to the wrong lane. Persisted only when non-empty (`definitionJson`), so an unedited input's saved file and compiled volumes are byte- and bit-identical to before this landed; schema bumped to 9 since the field is new, even though nothing here is version-gated the way network parsing is. `buildScenario` normalises by the weights' sum, so they need not sum to 1. The editor dialog landed later the same session (D33). | If a route's lane count turns out to change often enough in practice that the size-mismatch fallback fires constantly and authors find their shares silently reset, key by the chain's starting lane id instead — the rejected alternative above, not a redesign. The two tests holding the gate (`project.m1_26_1_lane_shares_round_trip_and_stay_out_of_an_unedited_file`, `editor.m1_26_1_lane_shares_weight_the_split_and_degrade_when_stale`) are the ones to extend, not replace, if that happens. |
| D33 | 2026-09-24 | **The vehicle-input dialog writes `laneShares` only when a field was actually touched, never merely displayed** | The dialog has to show *something* in each lane's weight box, and the natural seed for an unset input is the equal split (1 each) — but if accepting the dialog always wrote whatever was showing, opening an unedited input and clicking OK would write an explicit `{1,1,...}` where D32 depends on empty meaning "equal split" for the bit-identical guarantee, and would defeat the size-mismatch fallback the first time anyone merely looked at the dialog after a network edit. A `sharesDirty` flag, set only by `QDoubleSpinBox::valueChanged` and reset whenever the fields are rebuilt (seeded via `QSignalBlocker` so the seeding itself never sets it), makes "touched" the actual question asked, not "was the dialog opened". Rebuilding the fields on every route-combo change was the other half: the lane count is the selected route's, not the input's, so switching routes mid-dialog must not silently resize the wrong array. | If a future field in this dialog needs the same "only write if changed" contract, copy the flag-plus-`QSignalBlocker` pattern rather than inferring dirtiness by comparing values — a user typing the same number back is not a meaningful distinction to make, but it also is not wrong to treat as dirty, and the flag is simpler than either comparison. |
| D34 | 2026-09-24 | **M2's gate criteria are registered: C0–C4 as drafted in `M2_PLAN.md` §3** | Ratified by the owner in session, answering "accept as drafted", before any M2.2+ code. Amendment chosen by the owner the same day: C0, the portfolio audit, is answered before M2.5 produces its first delay figure rather than before any M2 code — its purpose is to be answered blind to TrafficSim's numbers, and until M2.5 there are none. The record lives in `M2_GATE.md`. | If a delay figure is produced before C0 is filled in, C3 stops being blind and the gate is weakened again; the stop point in NEXT exists for that. **C0 and C3, and the C0-before-M2.5 amendment, superseded by D38.** |
| D35 | 2026-09-24 | **M2.0.1 extends M3.1's derived rule to Connectors meeting at a lane's start; it does not start M3** | Asked "can the necessary part of M3 be done first", the owner chose to run the natural four-leg drawing. What that needs is the already-implemented M3.1 rule applied where it was skipped (a joined section starting at 0), ordered by drawing order, as same-station body arrivals already are. Conflict areas, crossing conflicts and authorable priority rules stay M3, behind the M2 gate. | If a study needs a different priority than drawing order, that is authorable priority rules — M3, not a patch here. |
| D36 | 2026-09-24 | **Amber stays red until M4** | Every safety clamp in the four-leg run and six in the frozen TS baselines are vehicles caught at the line by amber; a stop-or-go decision would change frozen fixtures. The owner chose to keep it; M2.5 shows the clamp count beside its figures. | When M4 builds signal control, or a study's delays are visibly driven by it. |
| D37 | 2026-09-24 | **Compositions and the static routing decision move from M2.1 to M2.3/M2.4** | M2's done-condition (counted volumes on the M1 intersection) needs both; M2.1's gate (validated distributions, behaviour parameters) does not. Both expand at compile time into the core's existing inputs, so `core/` and every frozen fixture are untouched. | If a positioned or per-interval decision is needed for the gate study — that remains M2.1. |
| D38 | 2026-09-24 | **The project is a simulator usable in real engineering work; the M2 gate is re-registered as C1 + C2, with C4 recorded** | Owner ruling in session: every reference to comparing against another simulator is removed from the repository, every file and line (git history keeps it), and the purpose is stated positively — a traffic simulation program engineers can use for real work. C0 (portfolio audit) and C3 (the comparison question) existed only to answer that comparison, so both are withdrawn; C1, C2 and C4 keep their numbers. Changed **before any gate observation** — C0 was never answered and M2.5 had produced no figure — so this is a re-registration, not a criterion moved after the fact (D8). M2.5 is unblocked. The owner's supplied spec copies in `docs/specs/` were edited too, on the same instruction. | An observation made before this date that the change could have been tailored to — none exists. |
| D39 | 2026-09-24 | **Movement delay is the run summary's whole-route term, grouped by (entry Link, exit Link)** | Smallest version that adds up: the movements' trips plus `notInMovement` equal the run's completed trips, and each movement's route starts on its approach and ends on its exit anyway. It includes source wait and the entry acceleration from standstill (≈3 s for a car), which is stated beside the table and pinned by a test. | When an engineer needs delay between two cross-sections (a travel-time section), or the entry bias matters to a figure. That is M5's measurement, and it removes the bias. |
| D40 | 2026-09-24 | **A queue counter at every signal head; an approach reports the maximum over its lanes** | Vissim's queue counter at the stop line with its default conditions (5 km/h, 10 km/h, 20 m) as data. It is measured along each route that crosses the line, so a queue spilling back past the pocket into the upstream Link is counted. Mean over every step, and the maximum. | An unsignalised approach (M3) needs a counter with no head, so counters become authorable objects then. |
| D41 | 2026-09-24 | **Prepare the ordered M3 contract without inventing an M2 gate result** | The owner asked to carry out the plan, which explicitly retains the M2 prerequisite. The contract replaces overridden merge groups atomically, detects cyclic/incomplete priority, reserves finite crossing extents through rear clearance and gives unsignalised queues real measurement lines. M3.2.1-M3.2.8 retain the full existing scope. | M2.6 remains the prerequisite for implementation. No owner observation, gate waiver or calibration result is inferred from this instruction. |
| D42 | 2026-09-24 | **A routeless input and a placed decision are expanded at compile time into static routes, one per complete path** | Every choice is random, independent and fixed, so a path's probability is the product along it, and splitting a Poisson stream by fixed probabilities is exact (as for M2.3/M2.4). The engine keeps one static route per vehicle. The rules: an equal share at each way out of a lane (the owner's choice); a decision acts where its Link begins (its station is not modelled); a lane that can reach no destination carries on routeless with an advisory; after its destination a vehicle is routeless again. A revisited lane, or more than 256 paths, blocks Run. | Lane changing (M3.2.8) makes a vehicle's lane a choice rather than a given, so decision legs must then be chosen per vehicle at run time, and this expansion is replaced. |
| D43 | 2026-09-24 | **A decision on the entry Link places each destination's flow in the lanes that reach it** | Measured: on the four-leg drawing, decisions after the entry left the counted proportions unreachable (136:16:39 against 500:120:100), because without lane changing the equal lane split decides the turns. Drivers sort themselves by lane before the junction; putting the flow in those lanes is what that achieves. It ignores the input's lane weights, which the table stops showing. Decisions further downstream still split per lane and are documented as such. | When lane changing exists, and the lane choice becomes the drivers' own. |
| D44 | 2026-09-24 | **A lane remainder shorter than 4.5 m after the last way out is not a network exit** | The owner's network had Connectors clicked 0.35–2.58 m short of Link ends, and the equal-split walk sent a third of the traffic out through the stub. 4.5 m is the shortest shipped vehicle (car), so no vehicle can be meant to drive into less. The owner chose this compile-time rule over snapping in the editor, which would restrict where a Connector can be drawn. A constant in `routeless.hpp`, not data: it is not a vehicle parameter a study tunes. | When a station on a decision is modelled, or a real exit this short turns up. |
| D45 | 2026-09-24 | **A per-interval turning proportion is chosen by the time a vehicle enters the network** | The engine carries one static route per vehicle, so the choice must be made at compile time, where only the entry time is known. The error is the travel time from entry to decision -- seconds against 15-minute counts. Outside every counted interval the whole-period flow applies, so a count table shorter than the run is not refused. A choice at the decision itself needs route choice in `core/`. | When `core/` gets runtime route choice, or a study's decision sits minutes downstream of its entry. |
| D46 | 2026-09-24 | **A decision's counts are proportions of the input's volume, and an uncounted interval uses the whole-period ones** | Owner request: turning counts and approach counts come from different sheets and rarely agree. The input's volume is the authority for how many vehicles enter; the turning counts say only where they go. Refusing an interval with no turn counted would block Run on ordinary data, and sending its vehicles nowhere would lose them. | If a study needs the input derived from the turning counts instead. |
| D47 | 2026-09-25 | **A Signal head is placed at the clicked station, one per lane, and is drawn as its stop line** | Owner report and choice (Vissim: one head per lane). The runtime already stops traffic at the head's station, so the stop line is the head, not a second object; the editor had simply discarded the click. One pick (`nearestHeadSlot`) serves click, hover and Ctrl-drag copy. | If a separate stop-line object is needed (e.g. a stop line away from the head, or unsignalised stop control in M3). |
| D48 | 2026-09-25 | **Signal control is authored as fixed-time controllers with signal groups and compiled into ordinary core programs; schema 12 programs shaped like a group migrate** | Owner report and choice (Vissim's model). A group is what a timing sheet lists; compiling it keeps `core/` and every fixture unchanged. Migration groups programs by cycle length into one controller with offset 0, because that is the only grouping the old file implies; colours are proved identical at every tick. Programs of any other shape stay legacy rather than being refused. | When M4 adds actuated control, intergreens or conflict checks, or if a file's programs of one cycle belong to different junctions and must be split. |
| D49 | 2026-09-25 | **M1 usability accepted by owner ruling, not by the written exercise** | After one timed attempt (9 min 40 s, no assistance; save/reopen exact; no turn pockets, no aerial image), the owner ruled: an engineer who has used another traffic simulator can model an intersection of ordinary complexity in this program in well under 10 minutes. The owner holds the gate, so M1 usability is accepted on that ruling; the record keeps what the attempt did not show (pockets, aerial image, first-attempt and documentation status) so no one reads it as the written exercise passed. M0 plausibility is a separate observation and stays open. | A later attempt by someone new to the program, or one that fails the pocket/aerial task, reopens the question. |
| D50 | 2026-09-25 | **A derived priority rule's stop line is 1 m short of the join, not on it** | Held exactly at the join, the waiting vehicle's front is on the shared lane; the major vehicle behind it stops within the headway and each waits for the other for ever. A Thai left turn at all times arrives at speed during the cross street's green and deadlocked the M2.6 template in its first minute (445 vehicles never entered). 1 m is geometry, not behaviour, so it is a constant (`kYieldClearance`, `sections.cpp`), not a `data/` value. The four-leg fixture's run and `trafficsim-cli 42` are byte-identical before and after. | An authored conflict area (M3.2) places its own stop line and supersedes this for authored rules. |
| D51 | 2026-09-25 | **No test times TrafficSim against Vissim; the owner judges pass or fail** | Owner ruling: withdraw the timing comparison with Vissim from every test, and let the owner assess the result. Made before any M2 gate observation, so it is a re-registration (as D38 was), not a criterion changed after the fact (D8). C2 is withdrawn and keeps its number; C1 still defines the study and C4 is still recorded, since it compares delays, not time. The verdict line in `M2_GATE.md` is the owner's. M1's 10-minute limit was absolute, never against Vissim, and M1 is already accepted (D49). | A later owner ruling. A judgment by the builder alone is the weakness ROADMAP §M2 names; outside engineers still strengthen it. |
| D52 | 2026-09-25 | **C4 withdrawn: no test compares delays with Vissim** | Owner ruling, following D51. Still before any M2 gate observation, so a re-registration, not a changed criterion (D8). C4 was never scored, only recorded; with it goes the "two LOS letters apart → investigation" trigger for the gate. What remains is C1 — the study completed end to end — and the owner's verdict. The Results table is still produced and recorded; it is simply not set beside another tool's. Plausibility against real-world data stays M6's (validation), untouched. | A later owner ruling. |
| D53 | 2026-09-25 | **The M2 gate is passed on the owner's word** | Under D51 the verdict is the owner's, and the owner reported "M2.6 passed". Recorded as *not disproven* (D8). The study's site, counts, file and Results table were not supplied; the record says so rather than filling them in. M3 may start. | Evidence that the study did not meet C1, or a later owner ruling. |
| D54 | 2026-09-25 | **M3.2.2 is split: M3.2.2a ships the authored model, schema 14, commands and the resolver; the reference lifecycle is M3.2.2b** | One system per session. The file/model seam — types, strict codec, History commands and ONE effective-priority resolver used by both compile and diagnostics — is testable on its own (A01–A04, A06–A08). Lifecycle (split/copy/retarget/resize/delete remapping, A05) touches every geometry command and is its own slice; until it lands, deleting a Link or Connector a control names is refused whole (safe but blunt), and a lane change leaves a stale, Run-blocked draft; `rightofway.until_m3_2_2b_*` pins both. Every authored area is Run-blocked (`UNSUPPORTED_CONFLICT_RUNTIME`) until M3.2.3, even an explicit merge the M3.1 mechanism could already run, because the plan says new controls stay blocked until their runtime is implemented. A taken-over merge compiles to exactly the fallback's rule (same 1 m waiting line, D50), so reversing it is the only change an author makes. Stop controls and queue counters are left to M3.2.5/M3.2.6, where their runtime lands. | M3.2.2b, or the M3.2.3 admission solver changing what a compiled area needs. |
| D55 | 2026-09-25 | **Authored controls follow their owners the way signal heads do; a split through one is refused; lane edits never retarget** | Deleting a Link or Connector (including a drag that detaches a Connector) cascades the areas on it, their rules, the lines on it and lines that served only those areas, in the same command, so Undo restores the whole relationship. A split moves stations by the same arithmetic as Connector ends and maps lane ids through the split's replacements; a Connector lane pair whose end moved downstream takes the new lane id. A control in or across the 0.2 m span is refused (`EDIT_SPLIT_CONTROL`) rather than guessed (contract §1, first slice). Copy takes a control only when every owner was copied — a Connector lane pair needs both end Links, because only then do its lane ids map. Lane-count and retarget edits are allowed and keep ids: a pair that no longer matches is a Run-blocked `CONFLICT_UNRESOLVED_PATH`, never an ordinal pick (§6). Reverse is refused like a head. The M3.2.2a scrutiny fixes ride under this number too. A waiting line on a preceding Link and crossing coverage are resolver work, carved to M3.2.2c. Recorded, not fixed: control ids are unique against network ids only. | An editor surface (M3.2.4) that lets an author select a control, which would need copy/delete of the control itself. |
| D64 | 2026-09-25 | **A queue counter is a set of places on runtime segments; derived and authored counters share that one mechanism; an authored counter over a Link's heads replaces that Link's derived row; copying a Link does not copy counters** | Contract §1 asks for counters independent of heads with one source of truth for their location. Measuring by place rather than by head removes the only thing that tied evaluation to signals, and the head-derived counters kept byte-identical reports because a head's compiled place is exactly the line it was measured from. Suppressing the derived row is the simplest rule that meets "no duplicate approach rows". A copied counter would duplicate a row name, which is why demand is not copied either |
| D65 | 2026-09-25 | **Queue counters are authored with their own tool: a stop-line click references the head, a waiting-line click references the line, a lane click is an explicit point on Link lanes only; the keyboard route is Add queue counter over the selected heads; the tab's Replaces column and the report's suppression read the one `replacedApproaches`** | A reference keeps the counter's line on the object when that object is moved, which a copied point would not. An approach queue is counted on the Link that carries it, so a point on a Connector path is refused rather than given a second station convention. Heads are already keyboard-selectable through the Signal heads table, so no second keyboard picker was needed. Sharing the rule is what keeps the tab from promising a replacement the report does not make |
| D66 | 2026-09-25 | **M3.2.7 is split a (fixture + controlled cases), b (diagnostic sweep), c (signal-composition variant + owner exercise); the fixture is single-lane Links with a 1 m median; minor-road clamps at a closing waiting line are reported, not asserted away, and carved to M3.2.8** | One lane per Link keeps every lane on its reference line, so stations read directly; the median is what separates the crossing from the merge. The spec asks for clamps to be reported; the missing commitment rule is admission-model behaviour, and fixing it inside the evidence milestone would tune the model while measuring it |
| D67 | 2026-09-26 | **The T-junction's signal variant puts the minor head upstream of both waiting lines; the headway exercise is a congested major road (a fixed-time head 16 m past the crossing), with the metadata recording the added head exactly instead of a geometry hash; the owner exercise is carved to M3.2.7d** | Upstream of the lines is where a signalised minor arm stands, and it composes the head with Stop/Yield rather than replacing them. Headway only decides for slow major vehicles near the entry, which free flow never produced (M3.2.7b). Hashing computed geometry would fail on another compiler for a last-bit difference the fixture tests already tolerate. A session cannot supply the owner |
| D68 | 2026-09-26 | **Conflict areas are automatic (the owner's ruling, superseding D61's no-passive rule): every at-grade overlap of two roads is a passive area and every merge shows its derived priority; they are derived from the drawing, never stored, and a click authors one** | Vissim's modelling surface generates them and the owner asked for it. Deriving instead of storing keeps one source of truth and needs no lifecycle: an edit to the drawing simply derives again. Passive keeps today's runtime exactly, so no study result moves until the author sets a priority |
| D69 | 2026-09-26 | **A driver who cannot stop at its line at `maxDeceleration` is committed: it ignores headway and gap time there, never occupancy, an unserved Stop, receiving space or the swept check; it applies to authored zones and derived merges; read off the snapshot, never stored** | The M3.2.7 sweeps' clamps were all drivers too close to stop when the gap closed; going is what a driver does, and the clamp was the model failing to. The owner ruled on derived merges (re-publishing the four-leg and M2.6 numbers; the four-leg did not move) and first on comfortable deceleration, which measured major-road clamps at the T-junction and an M2.6 merge and was replaced by the owner with maximum deceleration. Stateless keeps replay exact | The 5 residual minor clamps at walking pace near a merge line (not diagnosed); a stop-or-go decision at amber (D36, M4) |
| D63 | 2026-09-25 | **A crossing gesture makes one waiting line per lane, before the first area the lane meets; a Stop/Yield control covers every area giving way at its line** | Owner choice. A line per area left the far lane's line inside the near lane's area, where a Stop would halt a vehicle in the crossing. The areas behind one line were already admitted together (A15), so sharing the line changes where vehicles wait, not what they are admitted to. Existing documents keep their lines: only new gestures change |
| D62 | 2026-09-25 | **A Stop is served by coming to the line below walking pace and then resting at zero for one whole tick, which the Stop itself enforces; Yield is the existing gap test; the mode belongs to the waiting line** | Contract §5 asks for zero speed at the line, but the reduced car-following model only approaches zero behind an obstacle (0.04 m/s after 29 s), so a literal test never fires. Accepting 0.1 m/s within the gap the model keeps at that pace, then holding the vehicle at zero, keeps the one-tick minimum with no dwell parameter; the rest is ordinary braking, not an emergency clamp, so clamp counts stay honest. One control per line because a physical line cannot be Stop for one area and Yield for another; changing who gives way clears the area's control rather than leaving it on the wrong line |
| D61 | 2026-09-25 | **Conflict areas are picked by their own tool; a click on the selected one cycles priority without a passive state; a dragged line is kept and reported, not clamped; the yielding side is hatched** | Hit-testing areas under Select would steal the Link at every junction, which is the object an author clicks most there; Vissim avoids the same collision with its object-type sidebar. There is no passive state because an unauthored crossing is not an area (M3_PLAN §2); deleting the area is how an author gets one back. Clamping a waiting line at its entry would hide a draft that the resolver already names (`CONFLICT_WAITING_LINE_AFTER_ENTRY`), and authoring does not refuse what Run refuses. A crossing's two sides cover the same square, so one of them must let the other show through |
| D60 | 2026-09-25 | **The conflict-area editor ships table-and-dialog first; canvas gestures follow as M3.2.4b** | Every authored control is reachable by keyboard: the table, Enter, and a dialog whose fields keep the file's parameter names. That route reaches the same commands the pointer does, so A24's "pointer and keyboard submit the same commands" holds for what exists. Add-crossing expands to every overlapping lane pair and says how many before committing (contract §1). Its waiting lines stand 1 m short of entry, the D50 setback, so the result runs without further editing. The canvas draws areas and lines from the resolver's own geometry, so a picture cannot disagree with what runs. Clicking areas, cycling priority and dragging lines need hit-testing that competes with Link selection at every crossing: a separate interaction design, carved to M3.2.4b. | Canvas gestures once the Vissim conflict-area interaction is written up (VISSIM_PARITY §2); drawing two overlapping sides so both stay readable. |
| D59 | 2026-09-25 | **Authored merges run on the zone solver while derived merges stay on M3.1 rules; only hold cycles are refused; requests behind one standing leader share its room** | Contract §4 asks the major side to wait for an admitted minor, which the M3.1 rule cannot express. The solver does, with the same line and thresholds, so an authored merge moves to it. The D58 replay-equality test therefore no longer holds, and the take-over keeps the fallback's numbers rather than its trajectory. Derived merges stay on the rule, because moving them would change the published four-leg and M2.6 numbers (D39); that needs its own decision. A route minor at one zone and major at another deadlocks only if the holds close a loop, and a total priority order cannot close one. So the static waits-for graph replaces the blanket refusal, and a taken-over three-way merge runs. When requests share one standing leader, serving them in vehicle-id order is the contract's stable last tie-break; each takes its length plus standstill. | Queue gridlock, where an admitted vehicle stops inside an area behind a leader that stopped later: admission would have to reserve the downstream space, not only check it. Derived merges on the solver, with the owner's agreement to re-publish the four-leg numbers. |
| D58 | 2026-09-25 | **Zones are resolved per route in route distances; a chain of zones shares one line; authored merges run on their rules; mixed roles are refused** | Route distances make a section cut invisible to admission, as contract §4 asks. A route that turns off frees the area where it leaves, and a major route joining part way is seen from the join. A vehicle has nowhere to wait between two zones when there is less than the longest vehicle plus its standstill between them. Giving both the first line and the last exit makes admission atomic with no new state: it waits until every zone admits, then holds them all. This is conservative, because the later zone's majors wait from the first line. Authored merges already compiled to `PriorityRule`s (D54), the mechanism derived merges run on, so blocking them only withheld a runtime that exists. The evidence is an exactly equal event stream after take-over. A route minor at one zone and major at another can deadlock by mutual hold, and a minor route joining its chain part way never passes the line. Both are refused rather than half-run. | Arbitration that breaks mutual holds (M3.2.3c); merges moved to the zone solver so the major waits for an admitted minor. |
| D57 | 2026-09-25 | **M3.2.3's first slice runs one isolated crossing; a grant is read off positions, not stored; the whole area is reserved** | A minor vehicle past its waiting line with its rear short of the exit holds the crossing. Routes are fixed and positions monotone, so this is the contract's "retain until the rear clears", never revoked, with nothing added to `SimState`: a copied state replays exactly (A25), and stop service arrives with M3.2.5. A major vehicle waits at entry while another vehicle holds. A minor vehicle waits at its line on the M3.1 threshold (inside, within headway, or sooner than gap time; equality passes). It also waits while a standing leader leaves no room past the exit, because an admitted vehicle stopping inside the area would block the major road. The swept check runs over all candidate moves before publishing, so the result does not depend on vehicle order. Reserving the whole area loses capacity and is disclosed. Merges, connected groups (A15), areas over a section cut and receiving space shared between zones are carved to M3.2.3b and refused by name rather than half-run. | Measured capacity loss on the T-junction (M3.2.7); a moving leader that stops after admission leaves the vehicle inside the area — admission reserving the receiving space, not only checking it. |
| D56 | 2026-09-25 | **A waiting line is compiled as metres along the yielding segment, negative on the approach; it resolves only along single predecessors; crossing extents must contain the measured overlap** | "Upstream on every applicable route" (contract §1) is read off the runtime graph instead of the routes. Walking back from the side's entry, a segment with a second predecessor means some vehicle can reach the conflict without crossing the line. That is a named blocker, not a guess. A diverge is harmless: the core applies a rule only to routes through the yielding segment. Keeping the rule on the yielding segment, with a negative position, is what makes it apply to exactly those routes. Putting it on the line's segment would hold vehicles that turn away. The core's bound follows the same chain, so the two cannot disagree. Coverage uses the lane strips vertex for vertex with the authored polyline, so an overlap station is the cross-section `matchedStation` names. A larger extent is the author's choice; a smaller one, no overlap, two overlaps or a folded strip block Run. | Route-aware incidence (M3.2.3) that proves a bypassing route never reaches the area; a folded strip handled by `trimSelfIntersections`-style repair instead of refusal. |
