# PROGRESS — TrafficSim

Append-only. Newest entry at the top. **This is what a session with no memory reads to rejoin
the work.** Never delete an entry; move old blocks whole into `docs/archive/` if this gets
long. Older entries are preserved whole there:

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

## 2026-09-22 — The editor stops redrawing what has not moved (M1.27.1)

**Request:** continue M1.27 with the editor-redraw stage. It was booked on a **call count read
from the code**, not a timing, so the first deliverable was the measurement.

**The benchmark came first, and is committed.** `tools/editor_benchmark.cpp` builds a
deterministic corridor of signalised crossings — two Links, two Connectors and one signal head
each, three lanes throughout — drives the real `EditorCanvas` offscreen and times `redraw()` and
`hitObjects()`. It prints and asserts nothing, so it is not in `check`, but it is built by
default so it cannot rot. M1.23 has wanted this harness since it was written.

**It said the lag is real.** An 80-link network redrew in 37 ms and picked in 18; a mouse move
costs one of each, so the editor was already at about 18 frames per second, and 8 at 160 links.

**Then callgrind said the booked hypothesis was wrong.** The expected culprit, `headPosition`
scanning every connector path per signal head, came in below the reporting threshold.
`drawConnectors` was **67%**, through `connectorBoundaries` (39%), `offsetGeometry` (27%) and
`connectorPaths` (21%) — the editor recomputed every Connector's ribbon on every frame. After
caching those, `connectorMarkings` rose to **52.6%**, because it recomputes the boundaries
itself rather than taking them. So the profile, not the plan, chose both changes.

**The cache key is the inputs, by value.** `connectorPaths` and `connectorBoundaries` read
exactly the Connector, the two Links it names and the driving side; nothing else in the Network
can change their answer, which is a property of the code rather than a hope. A revision counter
would have been wrong twice over: `History::undo` restores an older revision, and two files can
be opened at the same one. A Connector carrying a drag preview differs by value and simply
misses. Interleaved medians, alternating the two binaries, Debug, ms per call:

| intersections | links | redraw before → after | pick before → after |
|---|---|---|---|
| 20 | 40 | 15.22 → **3.02** | 6.29 → **1.15** |
| 40 | 80 | 37.11 → **7.19** | 18.17 → **2.45** |
| 80 | 160 | 93.35 → **22.49** | 38.98 → **7.05** |

A mouse move on the 160-link corridor went from about 132 ms to 30 — 8 frames per second to 34.

**One change was built, measured and reverted**, which is worth as much as the two kept. The
same cache for Link geometry looked like a win on single samples (−23%) and was not one when the
two binaries were run alternately: −1% redraw at 80 intersections and **+9% on picking**. A
Link's polyline is cheap enough to recompute that validating a cache entry costs what it saves,
which is exactly what is *not* true of a Connector's ribbon. An experiment with
`QGraphicsScene::NoIndex` also changed nothing and was dropped.

**What guards it.** `network_lifecycle_ui_tests` widens a lane of the Link a Connector leaves,
asserts both that the widths really changed and that the Connector compares equal to the one
before — so the test is about the cache and not about the Connector — then requires the drawn
surface to have moved; it also covers a driving-side change and a deleted Connector. Weakening
the key to the Connector alone makes it fail, which is how it was verified. Entries are erased
only in `pruneConnectorCache()` at the top of `redraw()`, because callers hold references into
the map across several cached calls and an erase mid-frame would dangle one.

**Two limits found, neither fixed.** `QGraphicsScene` item construction is now the floor, which
is M1.23's culling/LOD work. And `EditorCanvas`'s `sceneRect` is hard-coded to 20 km square
(`src/editor/canvas.cpp:26`), so the 80-intersection fixture runs off the end of it — a real
16 km corridor would too. `NoIndex` showed it does not distort these numbers, but it is a
drawable-area limit a corridor study can reach, and it belongs with M1.23.

### Next

**M1.27.1 is implemented; its gate as written said "a cache and a head index", and the head
index was not built** — the profile put it below the threshold, so building it would have been
work with no measured cause. The gate in ROADMAP now says what the evidence supports, and the
correction is the honest part of this entry, not a shortcut. Remaining in M1.27, one session
each: **M1.27.2**, the three `std::string` ids on `Vehicle` (2026-09-18's profile; gate is the
four baselines byte-identical), and **M1.27.3**, UX — the `Ctrl`+left-click collision and the
`VISSIM_PARITY.md` rows that misreport the product.

Unchanged and still ahead of both: **M1.26.1** adjustable per-lane shares (decide where the
shares live first), then **M2.1** behind **M2's pre-registered criteria, still unwritten, which
block all of M2**. M1.22 and the shared-station `runtimeSections` refusal (§3.3, M3.2) remain
open, scenario-JSON export is still neither implemented nor booked, and M1's timed owner
exercise closes none of it.

---

## 2026-09-22 — The build stops re-reading its headers (M1.27, build stage)

**Request:** the owner asked to optimize every dimension — performance, code quality, data/API,
build, and UX/UI — *"ทุกข้อ + UX UI ได้หรือไม่"*. One system per session still applies, so the
work was carved into **M1.27** with four stages and only the build stage was executed. The
other three are booked with gates in ROADMAP; none of them is started.

**Measured first, on this machine (GCC 13.3, Qt 6.4.2, 4 cores).** A single editor translation
unit cost 5.8–7.3 s to compile, of which 3.14 s was the header set nearly every one of them
shares: 2.46 s of Qt and 1.47 s of `nlohmann/json.hpp`, against 0.36 s for `json_fwd.hpp`. An
empty translation unit is 0.016 s, so almost all of it was re-reading the same headers.

**Two changes, applied and measured one at a time.**

- `src/project/json.hpp` includes `<nlohmann/json_fwd.hpp>`. It only ever *declared* with
  `Json`; `document.hpp` includes it, so all 27 editor and shell files paid for the definition
  while five used it. The thirteen files that build or read a `Json` now include it themselves —
  the compiler named every one, which is what made this safe rather than a guess.
- `trafficsim_shell` (26 Qt sources) precompiles `<QGraphicsView> <QPainterPath> <QWidget>`, and
  the eleven single-source UI test executables take that same PCH through `REUSE_FROM`.

| configuration | wall `-j4` | ninja edge-seconds | compiling |
|---|---|---|---|
| before | 86 s | — | — |
| json_fwd only | 77 s | 299 s | 274 s |
| + shell PCH | 69 s | 264 s | 234 s |
| + `REUSE_FROM` | **64 s** | **243 s** | **213 s** |

`src/editor/canvas.cpp` alone: 5.77 s → 3.29 s. **Nothing about the program changed** —
`trafficsim-cli 42` still prints `29.249359418430977`, the four seed fixtures are untouched and
`check` is 35/35 on Linux. Windows is CI's to confirm.

**One variant was measured and reverted**, which is worth as much as the ones kept: adding
`<nlohmann/json.hpp>` to the shell PCH grew the `.gch` from 4.9 s to 6.9 s, and because that
sits on the critical path of `src/shell` it cancelled what the eleven reusing objects saved
(258 edge-seconds against 243 without it). The CMake comment records it so it is not re-tried.
No project header is precompiled: touching `canvas.hpp` still rebuilds 38 objects and no
`.gch`, so incremental builds are unchanged. A dead `distanceTo` left by M1.26 was deleted.

**Decision D27 — a PCH holds third-party headers only.** A precompiled project header would
turn every edit to it into a full-target rebuild, which is the cost this change exists to
remove. `REUSE_FROM` is how a single-source target gets one without paying to build it.

### Next

**The build stage of M1.27 is done; the other three stages are not started**, in priority order
and one session each. **M1.27.1, editor redraw:** `EditorCanvas::redraw()` clears and rebuilds
the whole scene on every mouse move, and inside one frame `connectorBoundaries` recomputes
`connectorPaths` twice per connector while `headPosition` scans every connector path per signal
head. Evidence so far is a **call count read from the code, not a timing** — so the first step
is the large-network frame-time benchmark M1.23 already requires, committed, *before* any
caching. **M1.27.2, `Vehicle` string ids:** measured 2026-09-18, gate is the four baselines
byte-identical. **M1.27.3, UX:** the `Ctrl`+left-click collision (extends the selection here,
duplicates in Vissim) and the rows of `VISSIM_PARITY.md` that now misreport the product —
`Tab` cycling, `Ctrl+B`, tool shortcuts and Connector lane ranges all exist.

Unchanged and still ahead of all of it: **M1.26.1** adjustable per-lane shares (decide where the
shares live first), then **M2.1**'s positioned routing decision behind **M2's pre-registered
criteria, still unwritten, which block all of M2** and need the owner. M1.22 remains open, as
does the shared-station `runtimeSections` refusal (§3.3, M3.2). Scenario-JSON export from the
editor is still neither implemented nor booked. M1's timed owner exercise closes none of this.

---

## 2026-09-22 — A route belongs to the carriageway (M1.26)

**Request:** three findings against M1.25, in the owner's words: the drawn route ran *"สวนทิศทาง
จราจรบน Link"* (against the traffic), routing and vehicle inputs should be created *"บนทุกช่อง
จราจรบน Link ไม่ใช่แค่ช่องที่คลิกโดน"* with the Connector connecting every lane first and narrowed
later, and after a route existed the Link and Connector *"แก้ไขไม่ได้"*. Asked how to handle a
narrowed Connector, the owner settled the design: **Routing เป็นการสร้างบนทุกช่องจราจร … ไม่ได้
สร้างเป็นรายช่อง — จะเพิ่มหรือลดจำนวนช่องจราจรของ Connector อย่างไร Routing ก็ยังอยู่.**

**What changed.** An authored route names **Links and Connectors**, never a lane and never a
Connector path. `routeLaneChains` (`src/model/network/routing.cpp`) derives one lane chain per
lane the drawing actually carries, and `buildScenario` expands each authored route into one core
route per chain, each still resolved through `expandRouteSegments`. A vehicle input expands the
same way: the authored volume is the **Link total**, divided equally across those lanes. All
three complaints fall out of that one change:

- **The backwards line** was `routeGeometry` drawing whole lanes. A Connector dragged onto a Link
  body attaches at a station, so the drawing ran from the lane's start — upstream of where the
  traffic joins. `routeGeometries` draws the compiled chain, whose sections are already clipped.
- **Every lane** is covered because the route names the carriageway; the lane clicked is not
  stored anywhere. The Connector dialog now defaults to all lanes of both Links.
- **Editing is no longer refused.** `changeConnectorRange`, `changeConnectorEndpoints`,
  `changeLanes` and `reverseLink` lost their route guards — nothing a route names can be removed
  by a lane-count change. The connector and signal-head guards stay; those name positions.

**What a route can no longer do, said plainly.** A lane-specific route — Vissim's turn pocket,
where only the left lane may turn — cannot be expressed. That is the cost of the owner's ruling
and it is booked against M2.1's positioned routing decision. Adjustable per-lane shares, which
the owner also asked for, are **not** implemented: the numbers must live where both the editor
and the compiler read them, and `VehicleInput` is a core type the scenario format shares, so that
is a schema-and-signature decision of its own — booked as **M1.26.1**.

**Two things that took care to get right.** Compiling an already-compiled Scenario has to stay a
no-op, so a route already given in lane or section ids passes through untouched
(`routeAlreadyExpanded`). And two Links named one after the other **imply** the Connector between
them, each lane finding its own — without that, `splitLink` would break every route it touched,
because a split makes one bridging Connector per lane and no single one of them can stand in an
object chain.

**Schema 8.** `migrateRoutesToObjects` maps a stored lane or path id to its owner and is
idempotent, so it can run on every read. It runs in `parseDocument` **and** in `loadScenario`,
because the CLI reads M0 scenarios through the same authoring path — missing that would have
silently changed what `trafficsim-cli` compiles. `putRoute` normalises the same way and now
refuses an id that names no object at all (`EDIT_UNKNOWN_OBJECT`).

**A broken route is reported, not deleted and not refused.** `routeRuntimeIssues` emits
`UNSUPPORTED_ROUTE_TOPOLOGY` for a route whose objects do not join up for any lane: the edit goes
through, the Problems panel names the route, Run refuses it. That is D18b's boundary applied to
routes — an author retargeting a Connector must not have the document reject the change.

**Verification.** Linux GCC 13.3 / Qt 6.4.2 `check` passes **35/35**, including architecture,
file sizes, and — the acceptance condition for this whole change — the `cli` and `reference`
baselines: a single-lane expansion keeps the authored route and input ids, so the four frozen
trajectories and `29.249359418430977` are untouched. New model cases pin the continuation rule at
object level, the chain-to-destination including ambiguity, a three-lane route surviving a
narrowed Connector with the Link total redistributed, the single-lane id rule, and the drawn
route against the compiled length at a mid-body arrival — the last one fails on the old code.
Ten existing tests were rewritten to the new contract rather than deleted, each keeping what it
was really testing. Rendered offscreen and inspected: a three-lane route drawn over a mid-body
arrival, the input row reading `1800 = 3 × 600.0`, and the Thai layout. Windows evidence comes
from CI, not from these Linux results.

### Next

**M1.26 is implemented, not closed:** its gate is the keyboard-only equivalent of both pointer
gestures plus the owner's timed exercise. The ordered follow-ups: **M1.26.1** adjustable per-lane
shares (decide where the shares live before writing any UI — that is the whole task); then the
routing decision as a positioned object, which is **M2.1** and must wait for **M2's
pre-registered criteria, still unwritten, which block all of M2** and need the owner. M1.22
remains open for geometry/snapping tools, custom pivots, layer locks and bulk inspection; signal
heads are the last object still placed only through a dialog, and `objectAt`/`inputPlaced` in
`src/editor/canvas_demand.cpp` are the shape to copy. The shared-station `runtimeSections`
refusal (§3.3, M3.2) and the M3.2 conflict-policy work remain separate. Exporting scenario JSON
from the editor is still not implemented and still not booked.

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
M1.12.1, M1.21–M1.21.1 and M1.24–M1.26, with M1.12.2 closed as a measurement error rather than a
defect and M1.12.3 closed by M1.19. M1.22, M1.23 and M1.26.1 remain open. `docs/ROADMAP.md` is
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
