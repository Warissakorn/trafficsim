# ROADMAP archive — implemented M1 sub-milestones

Sub-milestone bodies (M1.1–M1.6, M1.8–M1.10) moved whole out of [`ROADMAP.md`](../ROADMAP.md) to keep the active
sequence inside the 500-line budget (hard rule 6). Nothing here is edited or summarised — only
relocated. These are all **implemented**; `ROADMAP.md` keeps each heading with a one-line status,
and the current behaviour they describe lives in [`NETWORK_EDITOR.md`](../NETWORK_EDITOR.md).
Open milestones, gates and every M1.7-and-later entry stay in `ROADMAP.md`.

### M1.1 — Document and commands

Implemented: a Qt-free versioned ProjectDocument, persistent revision/ID counter,
named atomic edits, 100-entry Undo/Redo, save-point tracking and failed-edit rollback.

### M1.2 — Canvas and background

Implemented: pan/zoom/fit, metric grid/snap, link selection, embedded local background
images, two-point image calibration, transform/opacity editing and distance measurement.

### M1.3 — Link and lane tools

Implemented: drawing, point/link dragging, insert/remove points, per-lane widths,
reference-safe deletion, splitting with continuity connectors, opposite carriageways,
and a downstream extra lane for a turn-pocket approach. Basic atomic save/open landed
with these tools so drawings are not disposable. See NETWORK_EDITOR.md for the exact limits.

### M1.3.1 — Split links carrying signal heads

Implemented. Heads are located on the original lane geometry, classified by their
projection onto the original centreline, and projected onto the owning upstream lane,
downstream lane or split connector path. Heads inside the 0.2 m connector span become
connector-mounted controls. Route order, head IDs and program IDs are preserved in
one undoable transaction, including both driving sides and turn-pocket splits.

### M1.4 — Connector editor

Implemented: lane-to-lane creation by endpoint picking or Properties, editable interior
curve points, straight/curve reset, selection, reference-safe deletion and retargeting,
and shared endpoint maintenance after Link/Lane/driving-side edits. All changes use
History and persisted polyline geometry (schema 1/2 is migrated to schema 4). Curves are sampled polylines, not
swept-path or turning-radius validation. See NETWORK_EDITOR.md.

### M1.5 — Inspection and diagnostics

Implemented: Links, Connectors and Signal heads tables with two-way selection, canvas
multi-selection by Shift-click and rubber band, delete-many as one undoable transaction, and
structured diagnostics whose rows name an object and jump to it. Draft validity still blocks
an edit; runnability against the M0 compiler is reported on demand and never blocks. Exact
limits: property and geometry edits act on one object, there is no group drag, and
vehicle-type/behaviour references are not judged without a catalog. See NETWORK_EDITOR.md.

### M1.5.1 — Demand object tables and editing

Implemented. Optional typed authoring demand replaces the untyped JSON field.
Routes, vehicle inputs, fixed-time programs and heads have validated atomic commands
and native dialogs; routes, inputs and programs have their own tables. Route deletion
cascades inputs; referenced program deletion and topology-changing retargets are rejected.
Vehicle compositions, turning proportions and movement evaluation remain M2.

### M1.6 — Complete persistence workflow

Implemented. Atomic, bounded save/open and asset validation are shared with 15-second
dirty-revision recovery copies. Per-window locks exclude active editors; restored
documents open untitled and dirty. Vehicle/behaviour catalogs can be embedded explicitly.
Schema 1 and bare M0 authoring files load without changing IDs; saves now write schema 4,
with default ranges/levels/styles for older files. Unknown future versions are rejected.

### M1.8 — Run inside the network editor

Implemented. Run/Pause (F5), Step (F6 or Space on the canvas), Reset, seed and playback
speed control operate in the editor. Vehicles and fixed-time heads render over the
drawn network; status identifies the document revision and seed. Every fixed step
uses the unchanged core and the not-yet-validated marker stays visible.

**Done when:** an engineer draws a network, authors demand and watches it run without
leaving the editor. The automated drawing/demand/run/replay workflow covers the software
path; the owner's hands-on exercise remains in M1.7.

**Explicitly not in M1.8:** movement results, control delay, LOS or relaxation of D5.

### M1.9 — Network Objects sidebar, Vissim gestures and shortcuts

Implemented. A permanent Network Objects sidebar selects the creation type.
Ctrl+right-drag opens link and connector data dialogs; one connector owns contiguous
lane ranges. Ctrl+right-click opens demand/control creation or inserts a geometry point
in Select mode. Connector corner drags resize unreferenced ranges. Left-click during
creation adds intermediate polyline points.

Ctrl-click extends selection; Ctrl+left-drag duplicates selected spatial objects and their
internal connectors/heads with fresh IDs, preserving programs without doubling demand.
Delete removes selected objects; Ctrl+Delete removes a vertex. Tool shortcuts and Tab
overlap cycling are available. Ctrl+B toggles the background; Ctrl+Shift+O toggles tables.

The model derives stable per-lane paths for compilation, reanchoring and reference
cleanup. Unequal ranges may express merges in authoring; the core still rejects them.

**Done when:** a daily Vissim user draws the four-leg intersection without searching for
controls. Implementation and automated gesture checks do not replace this owner test.

**Explicitly not in M1.9:** new network types from the parity review, editable table cells,
group drag and rotation.

Connector reanchoring is no longer part of what makes those two expensive. VISSIM_PARITY
§1 and §6 item 10 cite it as a blocker, and that text stays as the 2026-09-14 assessment,
but reanchoring is now a similarity transform of the connector's endpoint chord, so
applying one transform as a sequence of single-object edits composes exactly. Measured on
a hand-edited curve between two links, a shared translation applied as two separate link
moves fell from 8.17 m of distortion to 2.6e-14 m, and a shared rotation from 5.53 m to
7.1e-15 m. What these gestures still need is selection-wide transform plumbing, and for
paste the ID allocation the parity review also names — not connector geometry.

### M1.10 — Levels and display types

Implemented. Links and connectors persist a level and named display type. Rendering,
vehicle/head overlays, hit testing and visible-level filtering use level order; Tab
can select an overlapping lower object. Levels and styles live in `data/levels/` and
`data/display-types/`; adding a style needs no C++ change. Unknown style IDs retain
their value and use the default appearance.

**Done when:** a grade-separated junction draws and selects correctly at different
zooms and a new display type is a data file. Automated gesture/persistence coverage is
included; the owner should inspect a representative junction as part of acceptance.

**Explicitly not in M1.10:** 3D or simulation effects from elevation. A drawn flyover
does not add right-of-way, merging or crossing-conflict logic.

---

---

---

### M1.12.2 — The miter "bulge": investigated, measured, and **not a defect**

**Closed without a code change, because there was nothing wrong.** The record said a 2→2 Connector
through a sharp bend "bulges to 8.698 m of a 7.000 m width, 24% over". Measured three ways on a
hard bend (90.47° of deflection):

| how the width is measured | reading |
|---|---|
| along the cross-section, at the mitered vertex | **9.9403 m** (+42%) |
| perpendicular, point to the far polyline | 7.0425 m (+0.6%) |
| **projected across the leg the vertex lies on** | **7.000000 m** (exact) |

The first is `width / cos(φ/2)`: the corner-to-corner diagonal of a correctly mitered joint, what a
road painted round a kink measures across its corner. Square to the road the carriageway is
untouched; the original 8.698 m is the same identity at a gentler bend (`7.000 / cos(36.4°)`).

**`offsetGeometry` must not be "fixed".** `bends_keep_their_full_carriageway_width` pins the miter
to 1e-9 and asserts both halves deliberately — 10.5 m projected across each leg *and*
`3.5*sqrt(2)` between adjacent boundaries at a right-angle corner, which is `3.5/cos(45°)`.
Removing the miter would reinstate the pinch it was added to fix: 18% at 63°, 30% at a right angle.

**What was missing, and is now there.** Width had only ever been bounded from **below**
(`least > .9*3.5`), which is how a claim of 24% over stood for a session unchallenged.
`a_bent_connector_holds_its_width_square_to_the_road_from_both_sides` asserts it **exactly**, to
1e-9, on every interior leg of a hard bend, and M1.12.1's curved-width bound was tightened to the
same equality. Both catch a 0.1% width error.

**The lesson, since it cost a session:** a distance between two boundaries is only a width if
measured square to the road. The 24% figure was taken with `apart()`, which is not.

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

---

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

A lateral wedge onto the Link's lane edges. It re-aimed each boundary's last leg, let neighbouring
boundaries cross and folded the mouth to a point; reverted on the owner's instruction and
superseded by M1.18, then M1.19. The body is in
[`archive/ROADMAP-M1-implemented.md`](archive/ROADMAP-M1-implemented.md).

---

### M1.20 — A Connector keeps its own position

The owner's rule: a Connector stores its geometry rather than having both ends recomputed from its
Links. An end still on its Link is snapped onto the lane under it with its station moved to match;
an end off its Link means the Connector has nothing to connect, so it is deleted with its routes
and heads in the same transaction. An edit that RE-LAYS a Link's lanes without moving the road is
not a move: every Connector follows the lane it names, at the same station.

**Done when:** a Link edit leaves every authored point untouched; an end standing on a lane that has
slid under it re-reads its lane and station; a Link moved out from under an end deletes the
Connector and one Undo restores it; a Connector's body can be dragged, including off its Links.
**All met at the model and command layer**; driving it by hand in the editor is still owed.

---

### M1.19 — Every Connector lane on the Link lane it feeds

The owner's requirement after seeing M1.18's mouth: a Connector's lanes must **line up** with the
Link's, not merely meet its cross-section. M1.18's slide left every boundary at its full offset
square to the Connector, so on an oblique cut the lanes spread by `1/cos(arrival)`. The offsets each
mouth is left at are now read off **the Link's own lane boundaries**, projected onto the Connector's
cross-section and re-solved against the end leg each boundary produces (`kMouthPasses`, 8 fixed
iterations). Bounded three ways, each for a measured failure: `kMouthSpanFloor` against compressing
the mouth to a point, `kMouthShiftLimit` against an unbounded solve (3.6e7 m measured), and a
fallback to the Connector's own cross-section where the two lane orders run opposite, which
otherwise folded 120 of 288 cases.

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


---

## Moved out of ROADMAP on 2026-09-22 (M1.25 session), to keep that file near 500 lines

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



### M1.21 — Authoring extensions from the supplied specifications

**Implemented; automated verification recorded in PROGRESS.** Shared Link boundary markings,
none/double marking strokes in both renderers, schema-7 persistence, Link insert/midpoint/
straighten/unreferenced-reverse actions, import cross-section validation and warning severity.
Unknown schema-7 network-object fields are rejected, so unsupported behavior is never silently
lost. [AUTHORING_EXTENSIONS.md](AUTHORING_EXTENSIONS.md) defines the actual supported subset.
Owner M1 acceptance remains open. This does not close the supplied target specifications.

### M1.21.1 — Network lifecycle correctness audit

Wrong-side endpoint retarget, steep-mouth fallback/advisory, physical range picking,
release-only group/rectangle gestures, capacity-limited resize and degenerate lane/tangent
handling are implemented. `NETWORK_LIFECYCLE_AUDIT.md` records finite model/UI coverage,
the restored CTest point group and preserved M1.20 semantics. Owner acceptance stays open.

