# Native network editor

The editor supports network drawing, demand and fixed-time control editing, persistence
and recovery, and simulation on the same canvas. The M0 core remains an unvalidated
prototype: merges, internal sources and cyclic routes are rejected at Run; geometric
crossing conflicts, lane changing and priority control are not implemented.

The owner's M0 plausibility and M1 timed usability gates remain open. See
[M1_ACCEPTANCE.md](M1_ACCEPTANCE.md). Running a drawing is not scientific validation.

## Start

Build as described in [BUILDING.md](BUILDING.md), then select Network Editor in the
simulation window or run:

```bash
./build/desktop/bin/trafficsim-desktop --editor
./build/desktop/bin/trafficsim-desktop --editor --language th
./build/desktop/bin/trafficsim-desktop --editor --scenario network.traffic.json
```

A new editor starts empty. Multiple editors own independent documents and recovery
locks. The Network Objects sidebar stays visible; Properties is on the right and the
Objects and problems dock is below. Switching language updates controls and messages.

## Draw and navigate

Select **Links (L)** in Network Objects, hold Ctrl and right-drag a centreline from
start to end, then confirm lane count and width in Link Data. Left-click while creating
adds intermediate points. The existing click-polyline workflow also works: click each
point and press Enter or double-click to finish. Escape cancels an unfinished gesture.

Select **Select / move (S)** to pick a link body or connector path. Drag a link control
point to reshape it, or drag its body between control points to translate it. Every
completed drag is one undo entry. A connector's endpoints stay attached to their lanes;
only its interior points can move.

Ctrl+right-click or double-click a line in Select mode inserts a geometry point.
Select an interior point and press Ctrl+Delete, or use Remove selected point, to remove
it. A link/connector must keep at least two distinct points. Delete by itself removes
selected objects after confirmation.

The world uses metres, x east/right and y north/up. Right-button or middle-button drag
pans; the wheel zooms at the pointer. F fits the network and background. Grid spacing
is in metres, and Snap affects drawing/dragging. Measuring and calibration bypass snap.

| Shortcut | Action |
|---|---|
| S / L / C | Select / Links / Connectors |
| R / V / H | Routes / Vehicle inputs / Signal heads |
| X / M / K | Split / Measure / Calibrate |
| F | Fit network |
| Shift+click | Add/remove an object in the selection |
| Ctrl+left-click | Duplicate selected links at the clicked position |
| Tab on the canvas | Cycle objects overlapping the last click position |
| Delete / Ctrl+Delete | Delete objects / remove selected geometry point |
| Ctrl+B / Ctrl+I / Ctrl+Shift+O | Toggle background / Properties / object tables |
| Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S | New / Open / Save / Save As |
| Platform Undo/Redo; Ctrl+Y | Undo/Redo; additional Redo binding |
| F5 / F6 | Run or Pause / Step |
| Space / + / − on canvas | Step / faster / slower |
| Escape | Cancel gesture and pause playback |

Tool, overlap and playback convenience keys apply to the canvas so text fields remain
editable. Normal file actions use the platform shortcut conventions.

## Lanes, opposite carriageways and turn pockets

Properties → Links edits lane count and widths together. Enter one width for all lanes
or comma-separated widths per lane, using decimal points. Retained lane IDs do not
change. Shrinking away a lane referenced by a connector range, head or route is rejected.

Create opposite carriageway makes a separate directed link with reversed geometry,
offset on the median side according to combined lane widths and the carriageway gap.
It copies level/style. It is independent after creation.

Split at distance measures from the selected centreline's start. The split tool
projects a click onto that centreline. Splitting produces upstream and downstream
links with explicit continuity connectors across a 0.2 m span. Existing routes expand
in travel order and attached external connectors are reanchored.

Split + add downstream lane also adds a median-side lane to the downstream portion.
This is a constant-width turn-pocket approach, not a variable-width taper. The new lane
has no invented demand, turn movement or lane change; author its connections explicitly.

For a signal-bearing split, each head's original lane position is projected onto the
original centreline to decide which portion owns it. Its world position is then
projected onto the new lane or connector path to obtain valid stationing. Heads strictly
inside the connector span become connector-mounted; boundary heads belong to the adjacent
link. Head/program IDs and route order survive, and one Undo restores the entire edit.

Lane ordering runs from the left shoulder towards the median for left-hand traffic,
mirrored for right-hand traffic. Changing drivingSide recomputes lane geometry and
reanchors connectors without swapping IDs. Independently drawn opposite links do not
move when this setting changes.

Reanchoring distributes endpoint displacement over intermediate connector points by
arc length. It is not a turning-radius or swept-path guarantee; inspect tight turns.

## Connector lane ranges

Select **Connectors (C)**, then Ctrl+right-drag from a source link lane to a target link
lane. The dialog chooses the first lane and contiguous lane count at each end. Left-click
during creation can add intermediate points. One completed gesture creates one connector
object even when it carries several lanes.

The two-click workflow remains: pick an orange source endpoint, then a teal target
start, to create a single-lane connector. Properties → Connectors also creates and
retargets connections using actual link/lane IDs.

A connector stores a base polyline and source/target lane counts. Its lane paths are
derived in monotone order, with stable IDs: the first uses the connector ID and subsequent
paths use `id/lane-2`, `id/lane-3`, etc. Routes and signal heads can reference those paths.
Unequal counts express fan-outs or merges in the drawing; merging still fails the M0
run check. Connector ranges are limited by the existing lanes, at most 12 per end.

Select a range connector in Select mode and drag its source/target outer corner handle
to adjust the last lane. Properties exposes both counts as an alternative. Retargeting
or resizing a connector used by a route or head is rejected; revise those references
first. Reshaping its curve remains allowed if the whole document validates.

Interior points are editable. Reset curve to lane directions creates a cubic sampled
into 12 spans; Make straight retains only endpoints. There are no separate persisted
Bézier handles and arbitrary edits need not remain smooth. Coincident endpoints cannot
generate a default curve; leave a positive gap. Duplicate lane-pair connections are
rejected, including pairs already covered by another connector range.

Deleting a connector removes heads on its paths, affected routes and their vehicle
inputs in the same undoable transaction. Heads on unaffected links remain.

## Selection, tables and display

Click replaces the selection; Shift-click toggles an object. Dragging on empty space
selects touched links/connectors in a rectangle. The last selected object is primary;
property and geometry edits act on it alone. Group dragging and rotation are not included.

Ctrl+left-click duplicates selected links so the primary link's first point lands at
the click. Internal connectors and heads are copied with new IDs, geometry offsets and
level/style values. Heads share their existing programs. Routes/inputs are not copied,
because copying a drawing must not silently double arrivals. Connector-only duplication
is rejected. Delete selected objects cascades dependent connectors, heads, routes and
inputs; one Undo restores all of them.

Links, Connectors and Signal heads tables mirror network objects and support selection
and framing on the canvas. Selecting a head selects its carrying link or connector.
Routes, Vehicle inputs and Signal programs have Add/Edit/Delete actions and dialogs;
double-click their rows to edit. Table cells are read-only views of the document.

Properties → Level and Display type apply to the primary link/connector. Level orders
drawing and picking; vehicles and heads use their carrying object's level. The sidebar
filters to all levels or one catalog level. Tab can reach a lower overlapping object.

Definitions live in `data/levels/*.json` and `data/display-types/*.json`, loaded in filename
order. Add a JSON entry with English/Thai names to add a level or style; no C++ edit is
needed. Catalogs require ground level 0 and a default style. Unknown stored styles keep
their ID and render with the default, with a missing-style indication in Properties.
Levels only affect display and selection; they do not change runtime conflicts.

## Routes, inputs and fixed-time signals

1. Draw a continuous path using links and connectors.
2. In Routes, choose Add and append lane/connector segments in travel order. The next
   segment menu lists valid continuations. Remove last backs up a choice. R followed
   by Ctrl+right-click on a lane starts the route dialog with that lane.
3. In Vehicle inputs, choose Add, select a route and vehicle type, and enter vehicles
   per hour and the start/end interval. The initial interval is the project duration.
   Source routes must begin at a supported external entry for Run.
4. Signal programs edits ordered duration/color phases and cycle offset. Add a signal
   head on a lane or derived connector path, choose a program and position in metres.
   Program deletion is blocked while a head references it.
5. Run settings edits duration and fixed timeStep together. All demand/control changes
   validate and commit through the same Undo/Redo history as network edits.

Vehicle types and driver behaviours normally resolve from `data/`. Embed catalogs
stores explicit copies in the project for portability. Explicit empty overrides remain
empty and do not silently fall back to installed data. Intrinsic demand errors block
edits; unsupported runtime topology is a separate diagnostic.

## Check and Run

Check runnability resolves catalogs and validates the current document revision.
Problems name the object and can jump to its canvas object or demand row. A document
without demand still receives topology diagnostics plus the missing-definition finding.
A committed edit clears stale findings. Rejected edits show draft issues without
changing the document or losing its previous save point or Redo history.

A clean check means the current core accepts the scenario. It does not certify traffic
engineering correctness. The permanent not-yet-validated marker remains visible.

Run (F5) compiles one revision into a detached snapshot, initializes the selected
32-bit seed and plays fixed simulation steps on the editor canvas. Pause keeps the
state; Step advances one timeStep; Reset recreates the initial state for the same seed.
Playback speed changes scheduling only. Status shows revision, seed, time and
active/pending/completed counts. Seeded arrivals are stochastic, so an initially empty
view can be normal.

Successful edits, Undo/Redo, opening/new documents and seed changes invalidate the run;
the next Run compiles the current document. No simulation runs against a stale edit.
This is the existing prototype core, with no lane changing, right-of-way or movement
LOS added by the editor.

## Background image

Import a local PNG/JPEG/BMP. The project embeds a PNG copy; input is limited to 24 MB,
32 megapixels and 32 MB encoded payload. The project input limit is 48 MB.

Calibrate image: two points measures known locations and asks for their real distance.
It preserves the first world point while changing metresPerPixel around it. Roads
already drawn are not rescaled: calibrate before tracing. Measure two points reports
world distance. Image x/y, scale, rotation and opacity commit together in Properties.
The image origin is its top-left pixel; positive rotation is counter-clockwise.

Import, transform, calibration and removal are undoable. History shares immutable
background bytes, and the canvas caches the decoded image. Ctrl+B changes visibility
without modifying the saved image.

## Save, recovery and formats

Save/Open uses `*.traffic.json`. Schema 2 stores format, schemaVersion, revision, nextId,
network, optional typed definition and background/transform. It never stores a second
editable runtime network. Schema 1 migrates with one-lane connector counts, level 0 and
default display type while preserving IDs. Unknown future versions are rejected.

| File kind | M0 scenario | Editor project |
|---|---|---|
| Typical name | `crossing.json` | `network.traffic.json` |
| Root keys | `network`, `definition` | Versioned envelope plus network, definition and background |
| Definition | Required | Optional until demand/control is authored |
| Opened by | Simulation window or editor | Editor |
| Runs | M0 harness | Editor after demand/catalog/runtime checks |

The editor opens bare M0 authoring files and schema-1/2 projects and saves schema 2.
The M0 simulation window recognizes an editor project before reading its fields and
offers to open it in the editor, even if that project already contains demand.

Save uses atomic QSaveFile replacement without direct-write fallback. A failed save
keeps the previous destination and dirty state; a failed load keeps the current model.
An empty or unrunnable drawing can be saved. New/Open/Close prompt Save/Discard/Cancel.
Undo history is in memory, bounded to 100 operations; saved-revision tracking determines
the title's asterisk and history resets on load.

Every 15 seconds, a dirty revision is atomically written to a separate recovery copy in
the platform's application-data recovery directory. Per-window UUIDs and process locks
prevent offering copies owned by active editors. Startup offers stale copies; Recover
can inspect them later. Recovery validates before replacing the document, then opens
untitled and dirty so Save asks for a real destination. A successful save or intentional
discard removes the consumed copy. Recovery is a checkpoint, not a persisted Undo log;
up to 15 seconds of recent edits may be absent after a crash.

## Verification

CTest includes core/baseline replay, model/project/command suites and native offscreen
UI workflows. These cover controlled splits, both driving sides, connector ranges,
catalog override semantics, atomic rollback, reference-safe deletion, persistence,
migration, demand dialogs, in-editor Run and deterministic Reset, recovery, Thai UI,
creation/cancellation gestures and level-aware overlap selection.

These checks do not perform the owner's timed exercise, establish model fidelity, or
measure large-network performance. Windows core checks do not imply Windows/macOS GUI
verification; installers and clean-machine packaging remain M7.
