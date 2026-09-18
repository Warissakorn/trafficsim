# ROADMAP — TrafficSim

Not a schedule. A **sequence**, so that any session can see where it sits and what closes
the milestone it is in.

**Two rules, inherited from a prior effort that broke both and paid for it:**

1. A milestone closes **only** when its done-condition is met and its gate has passed.
   Merged code is not a passed gate.
2. If a milestone must ship incomplete, the missing part is **carved out into a new numbered
   milestone in this file, immediately**, not left as a note. "We'll come back to it" is how
   a roadmap stops being true.

---

## M0 — Vertical slice

The smallest thing that is genuinely a traffic simulator, end to end. Every layer exists in
miniature, so wrong assumptions surface while they are still cheap.

**Scope:** two crossing links · one connector each way · one fixed-time signal · one vehicle
input · Wiedemann-style car-following · a native 2D view showing vehicles as dots · a headless run
that prints average delay.

**Done when:** `trafficsim-desktop` shows vehicles accelerating, queueing at red, and
discharging at green in a way a traffic engineer recognises as plausible — and CTest
proves the same seed produces the same run on a fixed engine/toolchain.

**Explicitly not in M0:** lane changing, editing anything, saving anything, LOS, multiple
seeds, priority control. The slice is about the *shape*, not the feature set.

---

## M0.1 — Native C++ migration (D15)

**Status:** implemented; Linux native/desktop checks passed. Owner M0 acceptance remains open.

**Technical scope:** C++20 core/model/evaluation and CLI, CMake/CTest, Qt Widgets harness,
strict JSON loading, four TypeScript baseline fixtures, and native developer checks.
The former TS application remains in Git history. All new application development is C++.

**Done when:** core/network behaviour passes the saved baseline comparisons and native
regressions; the desktop compiles and passes Run/Pause/Step/Reset/seed/language smoke tests;
and the native build/run instructions are usable. This does not close the owner's M0
plausibility gate, the M1 editor or M7 installer.

---

## M1 — Network editor

The Vissim modelling surface, natively: links are first class, connectors are real objects,
junctions are not something the user places.

**Status:** M1.1–M1.17 are implemented, including the carve-outs M1.3.1, M1.5.1, **M1.11.1** and
**M1.12.1**; **M1.12.2 is closed** — the reported miter bulge was measured along the cross-section
and is not a defect. **Two items are open again**, both from the owner's requirement that a
Connector's lanes meet the Link lanes they are assigned to: **M1.18** (a flush mouth), **M1.19**
(each lane on the Link lane it feeds) and **M1.12.3** (closed by M1.19). M1.7's owner acceptance remains
open, and M1 is not closed until its timed gate passes — no amount of merged code closes it.

M1.1–M1.6 and M1.8–M1.10 are implemented and their full bodies are in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md); each keeps its heading and
a status line here so the sequence stays whole.

**Sub-milestones below are in numeric order, and a carve-out made under rule 2 is filed at its
number rather than at the end** — M1.11.1 inside M1.11, M1.12.1 and M1.12.3 inside M1.12 — because
that is where a session looking for unfinished work will look for them.

**Done when:** an engineer draws a four-leg intersection with turn pockets from scratch, over
an aerial image, in under 10 minutes, without reading documentation — and reopening the file
gives back exactly what they drew.

**Includes, because D7 makes it non-optional:** left-hand / right-hand traffic as a project
setting that actually drives connector and conflict geometry. Retrofitting this later touches
every geometry routine.

---

### M1.1 — Document and commands

Implemented: a Qt-free versioned `ProjectDocument`, named atomic edits, 100-entry Undo/Redo, save-point tracking and failed-edit rollback.

### M1.2 — Canvas and background

Implemented: pan/zoom/fit, metric grid/snap, selection, embedded background images with two-point calibration and distance measurement.

### M1.3 — Link and lane tools

Implemented: drawing, point/link dragging, per-lane widths, reference-safe deletion, splitting with continuity connectors, opposite carriageways, turn pockets.

### M1.3.1 — Split links carrying signal heads

Implemented: heads are classified by projection onto the original centreline and reprojected onto the owning lane or split connector, in one undoable transaction.

### M1.4 — Connector editor

Implemented: lane-to-lane creation, editable interior curve points, straight/curve reset, reference-safe deletion and retargeting, shared-endpoint maintenance. Curves are sampled polylines, not swept-path validation.

### M1.5 — Inspection and diagnostics

Implemented: Links/Connectors/Signal-heads tables with two-way selection, multi-selection, delete-many as one transaction, and structured diagnostics that name and jump to an object. Draft validity blocks an edit; runnability only informs (D18b).

### M1.5.1 — Demand object tables and editing

Implemented: typed authoring demand replaces the untyped JSON field. Routes, inputs, programs and heads have validated atomic commands, dialogs and tables. Compositions and turning proportions remain M2.

### M1.6 — Complete persistence workflow

Implemented: atomic bounded save/open, asset validation, 15-second dirty-revision recovery copies, per-window locks, explicit catalog embedding. Schema 1 and bare M0 files load without changing IDs; unknown future versions are rejected.

### M1.7 — Run handoff and owner acceptance

Run handoff implemented: compile one document revision with resolved catalogs into a
detached network/scenario snapshot. Unsupported features are exposed before Run;
successful edits invalidate a run. The engine's existing capability guards remain.

**Owner acceptance remains open.** Perform the four-leg/aerial-image/ten-minute/reopen
exercise in [M1_ACCEPTANCE.md](M1_ACCEPTANCE.md). The M0 plausibility gate also remains
open. No automated test or implementation status closes either gate.

---

### M1.8 — Run inside the network editor

Implemented: Run/Pause (F5), Step (F6 or Space), Reset, seed and playback speed inside the editor; vehicles and fixed-time heads render over the drawn network and the status names the revision and seed. Every step uses the unchanged core and the not-yet-validated marker stays visible. **Explicitly not in M1.8:** movement results, control delay, LOS, or any relaxation of D5.

### M1.9 — Network Objects sidebar, Vissim gestures and shortcuts

Implemented: a permanent Network Objects sidebar; Ctrl+right-drag opens link/connector dialogs; Ctrl+right-click creates demand/control or inserts a geometry point; Ctrl-click extends selection and Ctrl+left-drag duplicates with fresh IDs; Delete removes objects, Ctrl+Delete a vertex; tool shortcuts, Tab overlap cycling, Ctrl+B background, Ctrl+Shift+O tables. **Done when** a daily Vissim user draws the four-leg intersection without searching for controls — an owner test that automated gesture checks do not replace. **Explicitly not in M1.9:** new parity object types, editable table cells.

### M1.10 — Levels and display types

Implemented: links and connectors persist a level and named display type; rendering, overlays, hit testing and visible-level filtering use level order, and Tab can select an overlapping lower object. Levels and styles live in `data/levels/` and `data/display-types/`, so adding a style needs no C++ change; unknown style IDs keep their value and use the default appearance. **Explicitly not in M1.10:** 3D, or any simulation effect from elevation — a drawn flyover adds no right-of-way, merging or crossing-conflict logic.

### M1.11 — Body attachments and direct lane resizing

Implemented in the Network Editor: Select/Links Ctrl+right-drag creates a Link from empty
space or a Connector between lane positions. Two-click connection also picks lane bodies.
Source/target positions persist in schema 3. Source, target and middle side handles resize
ranges from one lane; a Link side handle resizes its lane list. Undo/Redo, cancellation,
link edits, duplication and split attachment remapping share the existing command boundary.
The middle handle sets both ranges together; the Connector path count remains their maximum.
First-lane changes use the dialog or Properties. This does not claim independent arbitrary
internal Connector lane topology or close the owner's usability gate.

### M1.11.1 — Compile interior attachments into runtime lane sections

**Implemented.** `runtimeSections` cuts each lane at every station where a Connector attaches to
its body and `buildScenario` compiles the pieces, so an attachment part way along a lane runs in
both directions. **Leaving** is a diverge: the vehicle travels the drawn partial distance, and an
authored route stops at the section carrying the Connector it leaves by. **Arriving** is a merge,
arbitrated by a priority rule derived from the drawing (M3.1): the vehicle joins at the drawn
metre — the arriving path's successor is the section that *starts* at the cut, not the one that
ends there — gives way to traffic already on the lane, and is never charged for the stretch
upstream of where it came in.

Signal heads rebase onto the section they stand on, a head on a cut belonging to the upstream one
as `splitLink` already had it. Both render sites key geometry by section id from the same table
the scenario came from, and the route dialog still offers whole lanes via `authoringSegments`, so
no derived id reaches the project file.

Replay is unchanged and the four frozen baselines still pass, because `sectionId(laneId, 0)` is
`laneId`: a lane with nothing attached compiles to the `Segment` it always did. **One case still
cannot run:** a cut within `kMinSectionLength` (0.2 m) of a lane end or another
cut on the same lane, since the core accepts no zero-length segment. It reports
`UNSUPPORTED_CONNECTOR_POSITION`, reworded — the old string claimed the core runs only
end-to-start connectors, which is no longer true. A derived rule's gap time and headway come from
`data/priority-rules/`; their absence blocks **Run** with an object-linked
`EDIT_NO_PRIORITY_DEFAULTS` and does **not** block an edit, which is D18b's boundary.

**Done:** a vehicle leaves and enters at the drawn stations, travels the correct partial-lane
distances, obeys section-mounted signals, and replays deterministically on both driving sides.
The merge, internal-input and repeated-segment guards stand — `UNSUPPORTED_MERGE` was narrowed by
construction, not removed — and no persisted duplicate runtime network was introduced, because
sectioning changes no authored id and is a pure function of the drawing.

### M1.12 — Fixed lane edges, road markings and selection/copy gestures

Implemented following the owner's 1–3 lane examples: Link handles on both sides,
source/target/middle Connector handles on both sides, fixed surviving lane positions,
and shared edge/divider rendering in the editor and diagnostic view. Schema 4 stores
lane offsets and stable Connector interpolation weights. Ctrl-click adds selection;
Ctrl-drag selected Links, Connectors or Signal heads creates a single undoable copy.
Invalid attached-object drops roll back the entire edit. Tables select heads directly.
Owner Windows interaction and timed acceptance remain open.

---

### M1.12.1 — A Connector's own lane widths and markings

**Implemented.** Both fields of Vissim's Connector `Lanes` tab that the model could not express:

- **`laneWidths`** — one metre value per lane path. Previously every width was read from the Link
  each end joins, so a widening taper had to be authored on the Links instead.
- **`laneMarkings`** — the `MarkingType` painted on each **interior divider**, replacing a
  hard-coded dashed line. The two outer edges stay solid: they are the edge of the carriageway,
  not a lane divider. *Indexing note:* Vissim's field is per lane; ours is per divider
  (`paths − 1`), because per-lane does not map unambiguously onto `paths + 1` boundary lines.
  **This mapping was not checked against Vissim** — it is a chosen representation, not a measured
  parity claim (rule 4).

Both are **empty by default**, meaning "derive it from the Links", which is what every Connector
drawn before schema 6 does and what one whose lanes were never given a width must keep doing.
`connectorLaneWidths` is the single place a width is decided, so `connectorBoundaries` (drawing)
and `connectorShapeIssues` (`TIGHT_CONNECTOR_RADIUS`) cannot disagree once one is authored —
before this they computed it independently (rule 3). Schema 6 is additive-optional: absent keys
give an empty vector and nothing is converted on read, because nothing changed meaning. Marking
names are stored as `"solid"`/`"dashed"` so a human reading the file sees words, and adding a kind
cannot renumber what older files meant.

A resize that changes the path count **drops** the authored arrays rather than padding them: an
entry the author never typed is not a width they chose, and the derived value is the honest
fallback — the same reasoning that clears `laneBlend` when geometry changes. A partial list is
rejected (`EDIT_LANES`), since no field would say which lanes were authored and which derived.

**Done:** a width and a divider style are authorable, round-trip through the project file, undo as
one entry, and feed the drawing; a Connector never given either is unchanged to 1e-12, verified by
loading a schema-5 file written before the field existed. `BlockedVeh`, `NoLnCh` and
`Has overtaking lane` are **not** in this milestone; they wait on the lane-changing model (Q2).

### M1.12.2 — The miter "bulge": investigated, measured, and **not a defect**

**Closed.** The reported 24% over-width was measured ALONG the cross-section, where a mitered
corner's diagonal is `width / cos(φ/2)` by construction; projected across the leg the vertex lies
on, the carriageway is 7.000000 m exactly. `offsetGeometry` was not changed — removing the miter
would reinstate the pinch it exists to fix (18% at 63°, 30% at a right angle). The full
measurements, and the lesson that a distance between two boundaries is only a width if it is
measured square to the road, are in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md).

### M1.12.3 — The Link wins at the mouth (widths) — **closed by M1.19**

Carved out of M1.12.1. An authored width replaces the Link's width at **both** ends
(`road_boundaries.cpp:69-70`), so a Connector whose author typed a width no longer matches the
lanes it attaches to. The owner's rule: the Link wins at the mouth, the authored width takes over
through the body, the difference shows as a taper. Deliberately not done alongside M1.18 — that
one moves where a mouth sits, this one how wide it is, and together a failing width test and a
failing mouth test are indistinguishable.

**Closed by M1.19**, which had to decide the same question to put the lane middles on the Link's:
the mouth is built from the Link's widths, the authored width takes over through the body over a
transition zone of one carriageway width. `an_authored_width_is_exact_where_the_connector_is_
straight` pins both halves to 1e-9. `TIGHT_CONNECTOR_RADIUS` (`compile.cpp:73`) was **not**
re-derived: it reads `connectorShapeIssues`, which measures from `connectorLaneWidths`, and that
function is unchanged.

### M1.13 — Attachment stations in metres

**Implemented.** A Connector end is attached by `LaneReference::station`: metres along the
link's reference polyline, as Vissim stores a position, replacing the fraction of lane
arclength that slid every interior attachment whenever a Link was stretched. One station names
one cross-section, so every lane of a range meets the Link square on a curve; `matchedStation`
maps that station onto any lane or boundary derived from the same reference, and is the single
place the mapping lives. `laneOffset` keeps the reference polyline fixed under lane edits, so
adding or removing lanes cannot move an attachment either.

Shortening a Link past an attachment clamps it to the new end in `reanchorConnector`, which
every edit that can change a reference length already routes through; the Link edit is never
rejected. Signal heads keep their existing contract, where validation rejects such an edit.
`splitLink` carries stations across a cut by arithmetic alone. Schema 5 stores `station`;
schemas 1–4 and pre-schema M0 scenarios are converted on read at the same world position, with
the version — not the key that happens to be present — deciding the unit.

**Done:** stretching a Link's far end leaves an interior Connector at the same metre and the
same world point; schema 4 files load unchanged. M1.11.1 splits a lane at a station that, because
of this, does not move underneath it.

### M1.14 — Intermediate points, as Vissim counts them

**Implemented.** A Connector stores its two attachments and a settable number of intermediate
points, and is drawn straight between them and mitered at each one — the same rule a Link is
drawn by, confirmed against a Vissim connector with the count set to 2. Properties carries
Vissim's `Intermediate points`, which re-lays the shape the Connector already has and never
re-derives the default curve: raising it splits the longest leg so no placed point is lost,
lowering it spaces the points evenly. A new Connector gets 3. The arc reach that lays those
points is held at its 120-degree value, which stops a Connector drawn between two nearly
touching links from running to 11 times its own chord. Existing save files were deliberately not
migrated: the owner confirmed the project is still a test bed.

**Done:** a default Connector shows five grips; the count changes without losing the author's
shape; a count of 2 draws the three straight legs Vissim draws.

### M1.15 — A Name on every object

**Implemented.** `Link`, `Connector` and signal heads each carry Vissim's `Name`: free text, at
most 200 characters, never a key — two objects may hold the same one and an empty one is the
normal state. One field in the inspector's common section names whichever object is selected,
the way Vissim puts Name beside No. on every dialog, and the three object lists show it in a
Name column next to ID. It round-trips through the project file, copies with a duplicated
object, and undoes as one entry.

**Done:** an interchange is authored in the author's own words rather than in `link-17`.

### M1.16 — Moving several objects at once

**Implemented.** Left-dragging any member of a multi-selection moves the whole selection, which
Vissim has always done and this editor refused to do. Links carry the geometry; a Connector
rides the junction rigidly when both of its Links are moving and stays attached when they are
not; signal heads ride a station and need no moving. A selection holding no Link reports
`EDIT_MOVE_TARGET` rather than doing nothing quietly. One drag is one undo entry, and a drag
under the system drag threshold stays a click — without that, a two-pixel tremor either side of
a grid line moved a whole junction by a metre.

The reason this was expensive is gone: reanchoring a Connector now moves the one poly point
attached to the Link that moved (M1.14), so the group move had only to decide which Connectors
travel whole. `Alt`-drag rotation is still not implemented and is not booked.

**Done:** two Links and the Connector between them move as one shape, and one Undo puts them back.

### M1.17 — The mouth is a wedge cut on the Link — **reverted 2026-09-18**

**Reverted on the owner's instruction**, then superseded by M1.18. The wedge cut the end vertices
laterally onto the Link's cross-section and folded at oblique arrivals; it and its bounded re-miter
are in Git history at `55294fa`, removed by `9d8cb04`. The square end it reverted to stood
4.7-17.2 cm clear of the Link on a gentle join, up to 0.88 m on a hard reverse curve. What M1.17
fixed alongside the cut is **kept, and still is**: the cross-section is not interpolated through
the body, so a 3.5 m lane is 3.5 m at every interior point (interpolation drew 1.06 m on a reverse
curve, 0.46 m at a 90-degree arrival).

---

### M1.19 — Every Connector lane on the Link lane it feeds

The owner's requirement after seeing M1.18's mouth: a Connector's lanes must **line up** with the
Link's, not merely meet its cross-section. M1.18's slide left every boundary at its full offset
square to the Connector, so on an oblique cut the lanes spread by `1/cos(arrival)` — the right line,
the wrong width, every lane middle beside the Link's. The offsets each mouth is left at are now read
off **the Link's own lane boundaries**, projected onto the Connector's cross-section and re-solved
against the end leg each boundary produces (`kMouthPasses`, a fixed 8 iterations). Bounded three
ways, each for a measured failure: `kMouthSpanFloor` against compressing the mouth to a point,
`kMouthShiftLimit` against an unbounded solve (3.6e7 m measured), and a fallback to the Connector's
own cross-section where the two lane orders run opposite, which otherwise folded 120 of 288 cases.

**Done when:** a two-lane mouth spans the Link's own 7.000 m at every arrival up to 60 degrees; each
lane middle is on its Link lane's to 1 mm wherever the mouth's outer edges land on the Link's; the
sweep still has 0 folds; and `trafficsim-cli 42` still prints `meanDelay 29.249359418430977`. **All
met.** The owner's timed editor exercise in `docs/M1_ACCEPTANCE.md` is still the gate.

---

### M1.18 — A flush mouth by longitudinal shear

The mouth cut on the Link's own cross-section by sliding each boundary **longitudinally** along its
own offset curve — not M1.17's lateral wedge, whose revert stands. Superseded in part by M1.19,
which keeps the slide and changes the offsets it starts from. The full body, its measurements and
the trade the owner took at the time are in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md).

---

## M2 — Demand, run, first numbers · **GATE**

Vehicle inputs per interval, compositions, turning proportions. Press Run, get average delay
and queue per movement.

**Done when:** the M1 intersection, loaded with counted volumes, runs and produces a delay
table.

**GATE — the honesty check.** Before M3 starts, a practising traffic engineer completes a
small **real** study in this tool and in their current tool, and answers directly whether the
free SUMO-based alternative would have been good enough for this job.

> **This gate is currently performed by the project owner alone (D8), which makes it weak** —
> the person judging is the person who chose to build an engine. It is therefore run as a
> **pre-registered** test: **the pass/fail criteria are written into this file and committed
> before any M2 implementation begins.** Criteria decided afterwards are not a test. A pass
> under these conditions means "not disproven", never "confirmed". Recruiting outside
> engineers later strengthens the gate and is never wasted effort.

**Pre-registered criteria: TO BE WRITTEN before M2 implementation starts.** Leaving this
line unfilled and starting M2 anyway voids the gate.

- If the answer is broadly *yes*, this project is the wrong answer to the problem —
  see `PROBLEM.md` §7.1 — and the honest move is to stop and reconsider, not to continue
  because effort has been spent.
- This gate exists because the whole justification for owning an engine (`PROBLEM.md` §2)
  is an argument, not yet an observation.

---

## M3 — Right-of-way: conflict areas and priority rules

**The differentiating milestone.** Everything a SUMO wrapper structurally cannot do.

- Conflict areas as **editable input**: at each conflict point, choose which movement yields,
  or make it undetermined.
- Priority rules with real **gap time and headway in seconds and metres**.
- Stop and yield control.
- Signal heads placed **anywhere on a link**, not only at a stop line.

**Done when:** an unsignalized T-junction with a minor-road left turn produces plausible,
tunable minor-road delay that responds correctly to changing the gap time — and the same
network is demonstrably not expressible in a SUMO wrapper.

---

### M3.1 — Merge priority by gap time and headway

**Implemented, and it does not close M3.** The engine had no answer at a merge at all, which is
why `validateScenario` refused one outright: a place fed by two segments had no rule for who goes.
`PriorityRule{yieldSegmentId, yieldPosition, conflictSegmentId, conflictPosition, gapTime,
headway}` supplies one — Vissim's priority rule, with the two numbers an engineer tunes.

Car-following across a merge already worked, because `OccupiedSpan::segmentIndex` is a global
index into `Scenario::segments` and spans are bucketed globally, so two vehicles see each other
the moment they share a segment. What a rule adds is seeing the major approach *before* entering
it, which is not on the minor vehicle's own route and so is invisible to `closestVehicle`. A
vehicle that must give way is held at its stop line by the **same clamp a red signal head uses**,
not a second mechanism beside it. Rule-to-route incidence resolves once per scenario in
`ScenarioIndex`, mirroring `routeHeads`.

`UNSUPPORTED_MERGE` is loosened **by construction, never by removal**: a place fed by *n*
segments is runnable only when at least *n*−1 of them give way to another of them, so exactly one
has priority and the rest have somewhere to wait. A network that has not been through the
priority model reports it exactly as before — six tests break if the guard is deleted instead.
A segment may not give way to itself.

A standing queue upstream does **not** block: a stopped major vehicle further off than the
headway is a gap, and blocking on it would deadlock the minor approach instead of releasing it.

Gap time and headway for a **derived** rule live in `data/priority-rules/`, so changing them is a
data edit (hard rule 5). They are read best-effort, because a document carrying its own catalogs
must stay portable to a machine with no data directory; a rule that has to be derived without
them is refused at the point of use rather than given a zero gap time, which would be a merge
nobody gives way at, invented in silence.

**A deterministic threshold test, not a calibrated critical-gap model:** hard rule 4's
not-yet-validated marker stays and M6 still owns fidelity.

**Explicitly NOT in M3.1, and all still M3's:** conflict areas as editable input, priority rules
as an authorable object with their own UI, stop and yield control, crossing conflicts, and signal
heads placed anywhere on a link. M3's done-condition — an unsignalized T-junction whose
minor-road delay responds correctly to changing the gap time — is **not** met by this milestone
and M3 remains open.

---

## M4 — Signal control

Controllers, signal groups, programs, fixed-time and actuated, detectors, ring-barrier.

**Done when:** an eight-phase two-ring, two-barrier plan is built from a real timing sheet in
under 8 minutes with zero validation errors, and runs.

---

## M5 — Evaluation and reporting

The reason the whole project exists (`PROBLEM.md` §4).

- Movement-level delay, LOS, queue length, travel time.
- Multi-seed batch runs with means and confidence intervals.
- Report tables that go into an impact study without passing through a spreadsheet.
- LOS thresholds as swappable per-jurisdiction data, never compiled in.

**Done when:** one intersection, ten seeds, one command → a movement-level LOS table with
confidence intervals, ready to paste into a report.

---

## M6 — Calibration and validation · **GATE**

**Done when:** the engine reproduces published benchmark results — capacity and delay for a
signalized approach, gap-acceptance capacity for an unsignalized minor movement — within a
stated tolerance, and the tolerance is published in the docs and shown in the app.

**This is a hard gate.** Numbers from an unvalidated engine must never reach a regulator.
Until M6 passes, every results screen carries a permanent "not yet validated" marker.

---

## M7 — Desktop packaging

Offline install, native file dialogs, no server required.

**Done when:** a non-technical user installs from a single file on Windows and macOS, opens a
project by double-clicking it, and works with no network connection.

> The Qt desktop now exists from M0.1 (D15), with an automated controls smoke test.
> M7 still owns installers, clean-machine deployment, platform integration and file
> associations on Windows/macOS. A developer executable does not close M7.

---

## Later, with their own milestones — not to be started inline

Recorded so nobody starts them opportunistically:

- Pedestrian and public-transport modelling
- Regional/macroscopic assignment
- Scenario management and comparison
- 3D presentation
- Collaboration
