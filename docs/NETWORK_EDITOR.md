# Network editor — M1.1–M1.3

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
different zoom levels. Only one link is selected in this slice; multi-selection and
object tables belong to M1.5. Crossing lines are not automatically connected.

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
movement. Variable-width tapers and general connector editing are not in this slice.

For left-hand traffic lane order runs from the left shoulder towards the median; the
geometric ordering mirrors for right-hand traffic. Changing drivingSide recomputes
lane offsets and reanchors existing connectors without swapping their referenced IDs.
It does not relocate independently drawn opposite carriageways.

Edits reanchor existing connector endpoints. Intermediate connector points receive an
arc-length-weighted endpoint displacement, then the complete result is validated.
This is not a curvature/turning-radius guarantee. Tight hairpins and complex junction
geometry need engineering inspection; no swept-path validation is claimed.

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

M1.4 owns general connector creation/editing. M1.5 owns tables, multi-selection and
structured problem navigation. M1.6 owns the full persistence/recovery workflow.
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

These tests do not close the owner's M0 plausibility gate or M1's ten-minute usability
criterion. Windows/macOS GUI execution, installers and large-network performance remain
unverified until measured on those platforms/workloads.
