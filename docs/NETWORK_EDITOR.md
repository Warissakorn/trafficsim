# Network editor — Link, Lane and Connector tools

The native editor creates an authoring network. It does not simulate edited projects yet.
The existing M0 simulation window remains available and retains its unvalidated marker.

## Start

Build the desktop as described in [BUILDING.md](BUILDING.md), then either select
**TrafficSim — Network Editor** in the simulation window, or run:

```bash
./build/desktop/bin/trafficsim-desktop --editor
./build/desktop/bin/trafficsim-desktop --editor --language th
./build/desktop/bin/trafficsim-desktop --editor --scenario network.traffic.json
```

A new editor opens an empty document. Opening a second editor creates an independent
project; it is not a live view of the simulation window. Existing M0 authoring JSON can
be opened through the editor's Open command. Saving it writes the versioned editor
format, so use Save As if you want to retain the original fixture format.

## Draw, select and edit

1. Select **Draw link**, set the lane count and new-link lane width, and click the
   centreline points. Press Enter or double-click to finish; Escape cancels the draft.
2. Select **Select / move**. Click near a link centreline to select it. Drag a white
   control point to reshape it, or drag the centreline between points to move the link.
   One drag creates one undo entry, regardless of the number of mouse movements.
3. Double-click a centreline to insert a point. Click a point then press Delete or
   **Remove selected point** to remove it. A link must retain at least two distinct points.
4. Change **Number of lanes** and **width per lane**, then select **Apply lane count /
   widths**. Enter one width for all lanes, or comma-separated values for every lane.
   Values are in metres; use a decimal point. IDs of retained lanes do not change.
5. Right-button or middle-button drag pans. The wheel zooms around the pointer. **Fit
   network** (F) frames the drawing and background. Grid spacing is in metres; **Snap**
   snaps drawing/dragging to the grid. Measurement/calibration bypass grid snapping.

The world uses x east/right, y north/up, and metres. The image origin is its top-left
pixel. Positive image rotation is counter-clockwise in world coordinates.
The Properties toolbar action (Ctrl+I) hides or restores the inspector; it can also be
resized or detached. Selection tolerances and control handles use screen pixels, so they remain usable at
different zoom levels. Several objects can be selected at once (see M1.5 below), but
property edits act on the last one selected. Properties has separate Links, Connectors and
Image tabs; selecting an object opens its corresponding tab. Crossing lines are not
automatically connected.

## Opposite carriageway and turn pockets

**Create opposite carriageway** makes a separate directed link with reversed travel
geometry and a centreline offset based on the combined lane widths plus the specified
carriageway gap. It is placed on the median side for the current drivingSide. It is a
copy, not a live mirrored pair: subsequent edits are independent.

**Split at distance** measures from the beginning of the selected centreline. The
click-to-split tool projects the clicked point onto that centreline. A split creates
an upstream link, a downstream link and explicit one-to-one lane connectors. There is
a 0.2 m longitudinal connector span centred on the split location; this avoids zero-length
runtime segments. Existing routes referencing a split lane are expanded in order.

**Split + add downstream lane** also adds one lane on the median side of the downstream
link. This creates the authoring geometry for a turn pocket with constant lane widths
per link. The added lane has no automatically invented input, lane change or turn
movement. Use the Connector tools below to author the turn movement explicitly.
Variable-width tapers remain unsupported.

For left-hand traffic lane order runs from the left shoulder towards the median; the
geometric ordering mirrors for right-hand traffic. Changing drivingSide recomputes
lane offsets and reanchors existing connectors without swapping their referenced IDs.
It does not relocate independently drawn opposite carriageways.

Edits reanchor existing connector endpoints. Intermediate connector points receive an
arc-length-weighted endpoint displacement, then the complete result is validated.
This is not a curvature/turning-radius guarantee. Tight hairpins and complex junction
geometry need engineering inspection; no swept-path validation is claimed.

## Connect lanes and edit curves (M1.4)

1. Choose **Connect lanes** in the drawing tools. Orange circles mark the end of every
   source lane. Click one, then click a teal circle at the start of the target lane.
   Hovering a target previews the curve. Two clicks commit one undoable command.
   Endpoint picking ignores grid snap so the connection lands on the exact lane end.
   Escape, changing tools, opening a project or Undo cancels an unfinished connection.
2. Alternatively, open **Properties → Connectors**, choose **from** and **to** lane
   references, then **Create connector**. The controls show actual link/lane IDs.
3. Switch to **Select / move** and click a connector line, or choose its ID in
   **Properties → Connectors → Connector** (useful for short or overlapping connections).
   Drag a white interior point
   to reshape it. Double-click the line to insert a point; select an interior point and
   press Delete (or **Remove selected point**) to remove it. One drag is one command;
   Escape cancels an in-progress drag. Grid snap applies to interior-point dragging.
4. Square endpoint handles are locked to their lanes. The connector body selects the
   object; it does not translate the attached endpoints. To attach an unreferenced
   connector to different lanes, choose its **from / to** values and **Apply from / to
   lanes**. This displaces the existing curve; it does not replace it with a new curve.
5. **Reset curve to lane directions** replaces the current shape with a sampled cubic
   aligned to the travel directions at the two lane endpoints. **Make straight** reduces
   it to its two endpoints. Both actions are undoable, including on imported connectors.
6. **Delete connector** confirms removal of the connector, routes using it and those
   routes' vehicle inputs. Undo restores all of them together. Links and signal heads
   remain in place.

The initial curve is a cubic sampled into 12 straight spans (13 points), using tangent
handles one-third of the endpoint separation from the ends. The resulting polyline
is the sole authoring geometry, stored in the existing version-1 `geometry` field.
There are no separately persisted Bézier handles. Moving/inserting/removing interior
points edits that polyline directly; arbitrary point edits need not remain smooth.
This is not a minimum-radius, conflict, clearance or swept-path check. Inspect tight
turns and U-turns. Coincident lane endpoints cannot generate a default curve; leave a
positive gap between the links. Duplicate connections between the same two lanes are
rejected without consuming IDs or losing the saved state or Redo history.

A connector used by an existing route may be reshaped, but its lane references cannot
be changed until that route is revised or removed. M1.4 does not invent a replacement
route or vehicle demand. Authoring can express merges; the existing M0 compiler still
rejects unsupported merging paths. Saving a drawing does not certify it for simulation.

## Object tables, multi-selection and problems (M1.5)

The **Objects and problems** dock (Ctrl+B) sits below the canvas with four tabs.

1. **Links**, **Connectors** and **Signal heads** list every object of that kind with its ID,
   and with lane counts, endpoints, lengths, positions and programs as applicable. Selecting
   rows selects those objects on the canvas and moves the view onto the last one. Selecting
   on the canvas highlights the matching rows. The tables are read-only; they are rebuilt
   from the document, never edited in place. A signal head is not a canvas object, so
   selecting its row selects the link that carries it.
2. **Ctrl-click** or **Shift-click** on the canvas adds or removes one object. **Dragging on
   empty space** draws a selection box; every link and connector the box touches is selected,
   in network order. The last object added is the **primary**: it keeps the white control
   handles, and lane counts, widths, splits, opposite carriageways, connector endpoints and
   geometry all act on it alone. The inspector says how many objects are selected so this is
   never ambiguous. **There is no group drag** — moving many objects at once would have to
   reanchor every attached connector, and that is not in this slice.
3. **Delete selected objects** removes every selected link and connector in one transaction,
   together with attached connectors, signal heads, affected routes and their vehicle inputs.
   One Undo restores all of it. An object that a link's own cascade already removed is
   skipped rather than reported as an error, so the result does not depend on click order.

### Draft problems and runnability

These are different questions and the dock keeps them apart.

- **Draft** problems are what makes a drawing incoherent — an unknown lane, a non-positive
  width, a signal head past the end of its lane. These **block** an edit, and always have:
  a rejected edit leaves the document untouched. What M1.5 adds is that the rejection is no
  longer one line of red text. Each issue now names the object it is about and selects and
  frames it when you pick the row. Because commits are validated, a *saved* drawing can never
  carry a draft problem, so this list is normally empty; that is the design, not a defect.
- **Runnability** problems are what the M0 simulation core cannot run. Press **Check
  runnability** to compile the current document and list them. They **never** block an edit
  or a save. The standing example is a merge: two connectors feeding one lane is legitimate
  authoring that the core has no gap acceptance for, so it is reported, not refused.
  The check is on demand, and its result is discarded as soon as the document changes, so a
  verdict is never shown for a revision it was not computed on.

A clean runnability check means the M0 core accepts this topology. It is **not** a statement
that the network is correct, buildable, or validated — M6 owns validation, and the
not-yet-validated marker stands regardless. A project with no simulation definition is
checked for topology only; vehicle type and driver behaviour references are not judged at
all, because those catalogs live in `data/` rather than in the project file, and resolving
them before a run belongs to M1.7. The dock says so rather than reporting them as unknown.

Routes and vehicle inputs are named by their own IDs in problems, but have no table and no
editing: they have no authoring model yet. That is M1.5.1.

## Background image

Import a local PNG/JPEG/BMP. The image is converted to PNG and embedded in the project,
so moving the project file does not lose the background. Limits: 24 MB input file,
32 megapixels, and 32 MB encoded image payload. The project input limit is 48 MB.

Use **Calibrate image: two points** to select two locations with a known real distance,
then enter that distance in metres. Calibration keeps the first selected world point
fixed and changes image scale around it. It does not rescale roads already drawn.
Calibrate before tracing roads. **Measure two points** reports the current world distance.

The inspector also exposes x, y, metresPerPixel, rotation and opacity. Press **Apply
image transform** to commit them together. Import, calibration, transform and removal
are all undoable. Background bytes are shared immutably between history snapshots;
they are not copied per edit. The canvas caches the decoded image.

## Save and history

- Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S: new / open / save / save as.
- Undo/Redo use the native platform shortcuts and toolbar actions.
- The title's asterisk marks a document different from its last saved revision.
  Undoing back to that revision clears it; branching after Undo discards Redo.
- New, Open and Close prompt to Save, Discard or Cancel when the document is dirty.
- A failed edit or load leaves the previous model intact. Failed save leaves the
  previous destination intact and keeps the current dirty state.
- A completely empty network can be saved. A project need not be runnable to be saved.

The version-1 JSON contains format, schemaVersion, revision, nextId, the authoring
network, an optional M0 definition and the embedded background/transform. It does not
persist a second compiled runtime network. IDs are retained on edits/save/reopen;
the allocator skips imported IDs. Undo restores the allocator with the document.
Revision numbers distinguish local committed edits, not globally unique studies or
simulation provenance. Undo history is in memory, limited to 100 operations, and resets
on load. It is not a crash-recovery journal; autosave/recovery remains M1.6.

Project JSON conversion and validation are Qt-free. The shell uses QSaveFile with no
direct-write fallback for atomic file replacement. Saving marks a revision clean only
after commit succeeds. Future schema versions are rejected rather than guessed.

## Referential safety and limits

Deleting a link asks for confirmation that attached connectors, heads, affected routes
and their inputs will be removed as one transaction. Undo restores them together.
Removing an individual lane referenced by a connector, signal or route is rejected.
Splitting a link carrying a signal head is currently rejected: preserving control
stationing through a split needs a dedicated policy, tracked as M1.3.1 in ROADMAP.
Moving/shrinking a link that would place a head beyond its lane is also rejected.

M1.4 implements general connector creation/editing as described above. M1.6 owns the full persistence/recovery workflow.
M1.7 owns edited-project simulation handoff and the timed four-leg acceptance exercise.
The M0 core still rejects merges and does not resolve geometric crossing conflicts or
lane changing; an editable intersection is not a validated runnable intersection.

## Verification

`ctest --test-dir build/desktop --output-on-failure` includes the `editor` model suite
and `editor-ui` native offscreen workflow. Model checks cover atomic failure, save-point
Undo/Redo/branching, JSON versions, reference-safe deletion, route remapping through
splits, both driving sides, lane widths, ID collisions and immutable image sharing.
The UI check exercises real mouse/keyboard drawing and dragging, pan/zoom, cancellation,
per-lane editing, pockets, opposite links, image calibration, Unicode file paths,
failed saves/loads, deletion/Undo, unsaved-work cancellation and Thai translation.
The `connectors` model suite checks both driving sides, turns/U-turns, invalid and
duplicate connections, exact persistence, endpoint locks and reanchoring, retargeting,
route/input cleanup and the runtime merge guard. The `connector-ui` workflow uses
actual mouse/keyboard endpoint picking, preview/cancellation, curve-point dragging,
insertion/removal, endpoint locks, Properties actions, Unicode save/reopen and Thai
feedback. It also verifies that deleting an imported connector restores its routes
and inputs on Undo.

The `diagnostics` model suite covers index-path-to-object-ID resolution including malformed
and out-of-range paths, draft issues naming the right object, an empty network staying a
legal draft, an authored merge being reported as unrunnable while remaining committable, the
runtime pass being skipped rather than throwing on an invalid drawing, missing and malformed
definitions being reported rather than thrown, catalog-dependent findings being withheld, and
every code the validators can emit having a non-empty English and Thai string. The
`tables-ui` workflow drives real gestures: table rows following draws and Undo, row selection
selecting and framing, Ctrl-click, rubber banding, cancelled and confirmed multi-delete
restored by a single Undo, a rejected edit populating Problems, jump-to-object from both a
draft and a runtime row, and Thai tabs, column headers and messages.

These tests do not close the owner's M0 plausibility gate or M1's ten-minute usability
criterion. Windows/macOS GUI execution, installers and large-network performance remain
unverified until measured on those platforms/workloads.
