# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks whole into `docs/archive/` if this gets
long. Older entries are preserved whole there:

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

## 2026-09-23 — The editor benchmark was measuring itself

**Request:** another optimization pass. The booked candidate was `occupiedSpans`, the largest
cost left inside a tick. The pass never got there, because the first measurement was wrong.

**What started it.** The editor harness said a frame with something selected cost 98 ms against
42 for an empty one — a 2.3× penalty nobody had looked at. Three experiments killed three
guesses: it was not `ItemIgnoresTransformations` on the lane-grip labels, not `drawLaneHandles`,
not `drawCopyPreview`. Then the fourth question — *is it the selection at all?* — measured six
identical batches in a row:

    28.0 → 54.3 → 85.0 → 108.6 → 136.3 → 158.8 ms

**Every redraw was making the next one slower**, and the "selection penalty" was only the third
batch of a climbing series. `QGraphicsScene::clear()` hands its items to an index that reclaims
them on a **deferred** update; the timing loop never pumped the event loop, so the index grew
without bound. With `QApplication::processEvents()` between frames — which is what the real
editor does, since Qt delivers mouse moves through that loop — it is flat at 11.5 ms.

**So the product was never the problem, the instrument was**, and the instrument's answer
depended on its repetition count. That makes every number M1.27.1 published wrong, including the
one in `CLAUDE.md`. Hard rule 4 is about fidelity claims; a frame-rate claim is one.

**Re-run against the fixed harness**, same interleaved medians of five, alternating a binary
built at `b2e1244` (the commit with the harness and no cache) with today's:

| intersections | links | redraw | pick |
|---|---|---|---|
| 20 | 40 | 15.73 → **2.30** (−85%) | 8.53 → **1.28** (−85%) |
| 40 | 80 | 36.78 → **4.91** (−87%) | 20.15 → **3.31** (−84%) |
| 80 | 160 | 95.07 → **10.64** (−89%) | 57.18 → **9.98** (−82%) |

**D28 pays more than it claimed, not less.** A mouse move on the 160-link corridor is 152 ms →
21, or 6.6 frames per second → 48; the entry had said 132 → 30 and 8 → 34. The `after` column is
where the artifact did its damage: with the cache a frame is cheap, so the growing index was
most of what the old harness timed. **M1.27.1's gate is met more strongly than recorded**, so
nothing reopens under rule 8 — which is the answer this had to establish before anything else.

**The correction is in the entries themselves**, not only here: the M1.27.1 table, the two
frame-rate sentences, the M1.27.3 summary line, ROADMAP M1.27 and `CLAUDE.md` all carry the
measured numbers now, each marked as corrected on this date.

### Then `occupiedSpans` stopped reading the whole route

The pass's next item. `appendSpans` (`src/core/routes.cpp`) walked **every** `RoutePart` of a
vehicle's route for every vehicle on every tick. Parts are contiguous and ordered by station, so
the ones a vehicle covers are a single run: `lower_bound` to the first part whose end is past the
rear bumper, then forward while the front has reached the next start. Both comparisons are
written exactly as the scan wrote them — same subtraction, same operand order — so the same parts
match in the same order. `occupiedSpans` also now reserves, since most vehicles sit in one
segment and the vector is rebuilt from empty every tick.

**The wall clock could not resolve it.** The box swung ±7% this session, the same size as the
effect: one round said −6.3%, the next spanned 975–1119 ms. So the judgement was made on
callgrind instruction counts, which have no noise at all.

| | 6 intersections | 24 intersections |
|---|---|---|
| `occupiedSpans` itself | 34.98M → **30.96M** (−11.5%) | 180.96M → **130.44M** (−27.9%) |
| whole program | 328.0M → **324.0M** (−1.2%) | 1930.3M → **1879.9M** (−2.6%) |

Isolated at 24 intersections: the binary search is −1.94% and the `reserve` a further −0.67%, so
the one line pays for itself.

**The estimate was 10–13% and the answer is 2.6%, which is the part worth remembering.** The
scan was not 90% waste, it was about 28% at this size: the per-element test is two comparisons,
so scanning fifty of them is not much dearer than six `lower_bound` steps — and **80% of the
fixture's vehicles are on the short entering routes** (12 × 600 veh/h) rather than the long
through route (1800 veh/h). The route length was the wrong thing to reason from; the demand mix
decides how many vehicles ever pay the long scan. The fixture was not changed to flatter the
change. The owner kept it at −2.6% because the saving is a complexity one and grows with the
network (−11.5% → −27.9% of `occupiedSpans` between the two sizes), where M2 and M5 will be.

Replay is byte-identical: the four seed fixtures and `trajectory-digest.json` pass untouched,
`trafficsim-cli 42` prints `29.249359418430977`, and the benchmark completes the same 475 trips
with the same peak of 235 vehicles.

### And a click stopped rebuilding the scene twice

`cancel()` forgets the in-flight gesture **and** repaints. Four callers then repainted again for
their own reasons, so every click on the canvas rebuilt the whole scene twice. `cancel()` is now
`resetGesture(); redraw();`, and the callers that already draw take `resetGesture()`.

The harness only timed `redraw()`, so the click had to be made measurable first — a new row that
alternates between two links, committed with its baseline before the fix. At 160 links, medians
of five: **20.51 → 9.70 ms a click, −53%**, against 10.9 ms for a single redraw. A click now
costs one rebuild, which is the floor.

**One of the four was not a double at all, and `editor-rotation-ui` said so.** `setVisibleLevel`
ends in `visibleLevelChanged` and `selectionChanged`, which tell the shell but do not draw, so
`cancel()`'s repaint was its only one; dropping it left a cancelled rotation preview on the
canvas. That site keeps `cancel()`, with a comment saying why. The test that caught it counts
preview items in the scene after each of six cancellation routes — it was written for M1.22.2 and
it earned its keep here.

### Next

**The optimization pass resumes at item 5, the documents.** `CLAUDE.md` is 3,151 tokens and
`PROGRESS.md` 14,818, and the standing orders send every session into both although `Next` is
twenty lines — splitting `Next` into its own file is about 12k tokens off every session start.
`src/render/` was deleted long ago and `ARCHITECTURE.md`, a read-before-working file, still
points at it. The audit is `.optimize/baseline/docs_audit.md` (69 files, 173,401 tokens); most of
its 79 "broken refs" are false positives — `nlohmann/json.hpp`, `id/lane-2`, the TS paths in
`MIGRATION.md`, and the files `docs/specs/` proposes but nothing has built.

Below the bar and deliberately unbooked: the per-tick vehicle `std::sort` (2.8%, and the vector
is nearly sorted), and `compileDocument` (15.2%, paid once per Run press, not per tick). Inside
`occupiedSpans` what is left is the `push_back` work itself, not the search.

**Measure the engine with callgrind, not the clock, until this box settles.** ±7% on wall time
cannot resolve a 3% change. The editor's numbers are steadier (±2%) and the clock is fine there.

Unchanged and ahead of all of it in the owner's order: **M1.26.1**, adjustable per-lane shares.

---

## 2026-09-22 — The reflexes, counted (M1.27.3)

**Request:** finish M1.27 with the UX stage. Its gate is a counted walkthrough, so the
instrument came first, exactly as the two stages before it.

**`tools/gesture_walkthrough.cpp` drives the real `EditorWindow` offscreen**, counts the
primitive inputs each authoring task costs, then replays the Vissim reflexes from
`VISSIM_PARITY.md` §1 against the drawing it just made. A reflex is judged by what it did to
the **document** — it transfers, it is a dead end, or it silently does something else, which is
the case worth finding. Not a test and not in `check`: it reports, the UI suites assert.

| Task, native path | inputs |
|---|---|
| Draw a 3-lane Link | 4 |
| Connect two 3-lane Links, all 3 lanes | 3 |
| Add a curve point to a Link | 3 |
| Duplicate a Link | 2 |
| Rotate a Link | 2 |

**Five of the six replayed reflexes transfer.** `Ctrl`+right-click inserts a curve point,
`Alt`+left-drag rotates, dragging moves a whole selection, right-drag pans, `Tab` reaches the
object behind. `Ctrl`+right-drag drawing a Link and creating a Connector is measured by the two
creation tasks above. The one row nothing here replays is the Connector's corner handles, which
the `editor-gestures` suite covers.

**Each row is a predicate, not a reading of the code**: what that reflex means in Vissim,
translated into this document. Two rows only started reporting the truth after the predicate
got stricter — `Alt`+left-drag passed a naive check while doing nothing, because the harness
grabbed the Link at its own centre, where the editor refuses to rotate on purpose.

**The ninth contradicted the documentation, in the useful direction.** §1 and §2 called
`Ctrl`+left-click a *collision* — the chord that duplicates in Vissim extending the selection
here. Measured, it extends the selection only on an object that is **not** already selected,
which is not the case a Vissim user's hand is in: on a selected object it does **nothing**. So
the danger the table has ranked High since 2026-09-14 is not a wrong verb, it is a dead end.

**Offered the Vissim verb for that dead end, the owner ruled it stays as it is** (D30). The
counted delta is therefore zero by decision, not by omission, and the harness is committed so
the next person can measure a real one.

**A second owner correction:** the table listed `Ctrl+N` as Vissim's simple-network-display
toggle and called it a Medium collision with New. **It is `Ctrl+A`.** `Ctrl+N` collides with
nothing, and `Ctrl+A` is unbound here — and stays unbound, because there is no simplified
display to toggle until M1.23 builds one.

**What the documentation cost.** `VISSIM_PARITY.md` was wrong in about a dozen rows, all in the
same direction: it under-reported the product. §1 and §6 are kept as the dated 2026-09-14
assessment and a new **§1a** carries the measured state; §2's three keyboard tables were
rewritten, since a table headed *what the editor binds today* cannot be excused as history.
Three of the six collision rows are gone because the editor was changed to match Vissim. The
2026-09-17 wedge-mouth follow-up moved to `docs/archive/` to pay for the space.

### Next

**M1.27 is closed.** Its four stages are done: clean build 86 s → 64 s, a 160-link corridor
95.1 → 10.6 ms a frame and 57.2 → 10.0 ms a pick (corrected 2026-09-23), the engine run
roughly halved, and the
gesture surface counted with one deliberate dead end left in it. Three benchmarks are committed
— `trafficsim-engine-benchmark`, `trafficsim-editor-benchmark`, `trafficsim-gesture-walkthrough`
— and none of them is in `check`, so re-run them by hand before claiming any of those numbers
again.

**M1.26.1, adjustable per-lane shares, is next**, and the owner asked for it two sessions ago.
Decide where the shares live before writing any UI — that is the whole task, because
`VehicleInput` is a core type the scenario format shares, and an equal split must stay the
compiled result of an unedited input, bit for bit.

Then **M2.1** behind **M2's pre-registered criteria, still unwritten, which block all of M2**
and need the owner. Still open: M1.22 (geometry/snapping tools, custom pivots, layer locks,
bulk inspection), M1.23's culling/LOD and the hard-coded 20 km `sceneRect`, the shared-station
`runtimeSections` refusal (§3.3, M3.2), and scenario-JSON export from the editor, which is
neither implemented nor booked. Inside a tick `occupiedSpans` is the largest single cost at
9.8%; no milestone is booked for it and none is needed yet. **M1's timed owner exercise
(`docs/M1_ACCEPTANCE.md`) is untouched by all of this** — a counted walkthrough is not a timed
one, and M1.27.3 never claimed to close it.

---

## 2026-09-22 — A vehicle stops carrying its names (M1.27.2)

**Request:** continue M1.27 with the engine stage. This is the one item another session
measured first: 2026-09-18 profiled the engine, found ~36% of instructions in `std::string` and
11.8% in the per-tick vehicle sort, wrote the fix into its `Next` and left it, because it
changes a core public type and every test that builds a `Vehicle`.

**Its corridors were never committed, so its numbers could not be reproduced.** That came
first: `tools/benchmark_network.hpp` now holds the generator both benchmarks use, and
`tools/engine_benchmark.cpp` compiles a corridor through `compileDocument` and runs it — no Qt,
so the engine stays measurable under the headless preset. Its one parameter is where the
northbound approach joins: the editor keeps the end attachment M1.27.1's published numbers were
measured on, and the engine passes a station, because a **mid-body arrival is what derives the
M3.1 priority rule a merge needs to run at all** — without it the corridor will not compile.

**The profile on that fixture reproduced 2026-09-18 exactly**, which is the useful part: string
copy ctor 17.4%, `Vehicle` copy ctor 12.1%, the vehicle sort 11.4% (they measured 11.8%),
`resolveRefs` 10.3% of which `lookup` 9.1%, `~string` 8.1%.

**What changed.** A `Scenario` is immutable and canonically sorted from `createSimulation`
onwards, so an index names exactly the object an id named — and unlike an id it costs nothing to
copy, compare or sort. `PendingVehicle` holds `inputIndex`/`routeIndex`/`typeIndex`, resolved
once per scenario in `ScenarioIndex`'s new `routeOfInput`/`typeOfInput`. `routeOfId`,
`typeOfId`, `IdSlot` and the `lookup` they served are gone: that undoes part of 2026-09-18's own
optimisation, which this subsumes, because there is no id left to look up. `InputState` loses
its `id`, which duplicated `scenario.inputs[i].id` once the vector became index-addressed.

**Resolving in `generateArrivals` was not enough, and the profile said so.** The first version
looked the input's route and type up there instead — the same lookup in a new place, 11% of the
run, once per input per tick whether or not a vehicle arrived. Hoisting it into the index is
what took the second half of the win.

Interleaved medians of five, alternating the two binaries, Debug, 300 simulated seconds:

| intersections | segments | before | after | |
|---|---|---|---|---|
| 12 | 171 | 1560.8 ms | **792.6 ms** | −49.2% |
| 24 | 351 | 2566.2 ms | **1331.0 ms** | −48.1% |

**The trajectory is identical, not merely the summary.** The four seed fixtures and
`trajectory-digest.json` pass unchanged — the digest pins per-tick position, speed, acceleration
**and vehicle ordering**, so a reordering could not hide — `trafficsim-cli 42` still prints
`29.249359418430977`, and the benchmark completes the same 475 trips with the same peak of 235
vehicles. No fixture was regenerated. What protects this is that ids only ever became slots
through `detail::byId`, whose first-occurrence rule is what every lookup they replaced returned.

**Names did not disappear, they moved to the boundary.** `DepartedEvent` and `ArrivedEvent`
still carry `routeId`, read from the scenario where the event is built, and `pendingJson` takes
the `Scenario` and emits the same three strings — a slot means nothing outside one Scenario, and
the fixtures are read by people. `checkpointJson` now throws rather than dereferencing a null
scenario, because without one there is nothing to resolve against.

**Tests.** `test::Placement` describes a vehicle by route and type id and `withVehicles`
resolves it against the canonical scenario, so the two representations meet in one place instead
of in three test files. `stepSimulation` asserts `state.inputs` is parallel to
`scenario.inputs`, so a hand-built state mis-indexes loudly instead of reading past the end.

### Next

**M1.27.3, UX, is the last stage of M1.27 and is not started.** It carries two things already
documented as High severity in `VISSIM_PARITY.md`: `Ctrl`+left-click extends the selection here
and **duplicates** in Vissim, and link creation is a polyline of clicks where the Vissim reflex
is `Ctrl`+right-drag. It also has to correct that table's stale rows — `Tab` cycling
(`EditorCanvas::cycleOverlap`), `Ctrl+B`, the tool shortcuts and Connector lane ranges all exist
now and the table says they do not. **Gate:** a counted walkthrough delta (clicks per task, dead
ends before and after), which does **not** close M1's timed owner exercise. UX changes are
behaviour changes, so each needs the owner's approval in user-facing words before it is made.

Unchanged: **M1.26.1** adjustable per-lane shares (decide where the shares live first), then
**M2.1** behind **M2's pre-registered criteria, still unwritten, which block all of M2**. M1.22,
M1.23's culling/LOD and the hard-coded 20 km `sceneRect`, and the shared-station
`runtimeSections` refusal (§3.3, M3.2) remain open. Inside a tick `occupiedSpans` is now the
largest single cost at 9.8%; there is no booked milestone for it and none is needed yet.

---

## Next

**Two engineering items are open** — items 0 and 0b below. Everything else here is the owner's.

**0b. The demand authoring gate, and what is still missing.** Routes and vehicle inputs are
authored by clicking and a route now names Links and Connectors (M1.25, M1.26 — see the top
entry), but the gate is the keyboard-only equivalent of both gestures plus the owner's timed
exercise below, and neither is done. **M1.26.1** (adjustable per-lane shares) is open and its
real question is where the shares live, since `VehicleInput` is a core type the scenario format
shares. Signal heads are the last object still placed only through a dialog;
`objectAt`/`inputPlaced` in `src/editor/canvas_demand.cpp` are the shape to copy, inside M1.22.
A routing decision as an object at a station along the link — what Vissim actually places, and
what would bring back the lane-specific route M1.26 gave up — is **M2.1**, and M2 may not start
until its pre-registered criteria are written into `ROADMAP.md`. Writing them is the owner's,
and it blocks all of M2.

**0. Fix the duplicate-station refusal in `runtimeSections`.** A second Connector arriving at a
station the lane is already cut at is rejected as unsectionable, because `sections.cpp:68` measures
its cut against `boundaries.back() + kMinSectionLength` and the boundary the FIRST arrival just made
is at that very station — so it is measured against itself, and Run is blocked for a pair that is
physically fine. Reuse the existing cut instead of rejecting it. Section table only, one system, and
the test already in `tests/connector_tests.cpp` flips to its commented expectation when it lands.
See the top entry of this file, and `CONNECTOR_PARITY_AUDIT.md` §3.3.

**1. Drive M1.19 and M1.20 in the desktop editor.** Both are measured at the model and command
layer only. Nobody has yet dragged a Connector off a Link with a mouse, or looked at a mouth on
screen. Watch for a Connector deleted by a Link drag the author did not expect to touch it (the
Undo is there; the surprise is the thing to judge), and for whether half a lane width is the right
distance for "off the Link" — one constant, in `laneContains`.

> **Carry into the acceptance exercise:** the mouth is the shape an author sees at every merge, and
> both times it has been wrong it was found from a render, not from a test. When a Connector is
> drawn onto a Link's body at a sharp angle, check the joint by eye: every lane of the Connector
> should meet the lane of the Link it feeds, middle on middle, and the mouth should span the Link's
> own carriageway rather than spreading past it. At arrivals steeper than about 60 degrees it
> cannot: `kMouthSpanFloor` in `road_boundaries.cpp` is the dial, and `connectorMouthFit` reports
> what a mouth could not reach, in metres.

Every M1 sub-milestone and carve-out is implemented: M1.1–M1.20, plus M1.3.1, M1.5.1, M1.11.1,
M1.12.1, M1.21–M1.21.1 and M1.24–M1.27, with M1.12.2 closed as a measurement error rather than a
defect and M1.12.3 closed by M1.19. M1.22, M1.23 and M1.26.1 remain open. **M1.27.3 counted the
authoring gestures; a counted walkthrough is not the timed exercise in item 2 and closes nothing
of it.** `docs/ROADMAP.md` is
the authority on each; the bodies of the long-implemented ones live in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md).

**2. Run the timed acceptance exercise** in [`M1_ACCEPTANCE.md`](M1_ACCEPTANCE.md). It is the only
thing that closes M1 and it cannot be delegated: an engineer draws a four-leg intersection with
turn pockets over an aerial image, from a blank editor, **under 10 minutes, without documentation
or assistance**, then the file is reopened and compared field for field. Not one row of that record
has been filled in. Merged code does not close it (rule 1), and a rehearsed retry is not the first
observation.

**Do this on Windows.** Everything in these sessions was verified on Linux only. `native.yml` runs
the Qt suites on Windows too, and CI run 105 previously failed in `windows-core` on a vcpkg
`z-applocal` race before any test ran — so a green Linux run is not evidence about the platform the
owner actually draws on. The M1.12 review checklist from the 2026-09-16 sessions is the list to
work through first.

**3. Record the M0 plausibility observation** separately — acceleration, queue at red, discharge at
green. Owner observation, not calibration, and not M6 validation. The not-yet-validated marker
stays either way.

**4. Decide the name (Q5).** D11 deferred it "until the end of M1", and that trigger is now live.
`Velk` is the strongest recorded candidate — clean on npm, PyPI and a brand search, with
`velk.com`/`velk.io` held, which is ordinary for a four-letter word. Do **not** re-derive the
candidates that were already rejected: `Headway`, `MicroFlow Simulator` and `Veytrix` all have
findings in the D11 row. **This is the owner's decision, not a session's.**

**Not started, and deliberately:** M2 implementation. Its gate is pre-registered and
`ROADMAP.md`'s "Pre-registered criteria: TO BE WRITTEN before M2 implementation starts" is still
unfilled. Starting M2 before writing them **voids the gate** (D8), and that gate is the honesty
check on the whole project's premise. Write the criteria first.

**M3 is open.** M3.1 supplied merge arbitration only — a deterministic gap-time/headway threshold,
not a calibrated critical-gap model. Conflict areas as editable input, priority rules as an
authorable object with their own UI, stop and yield control, crossing conflicts and signal heads
anywhere on a link are all still M3's, and its done-condition about minor-road delay responding to
gap time is not met.

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
