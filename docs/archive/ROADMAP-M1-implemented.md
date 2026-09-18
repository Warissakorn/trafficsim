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
