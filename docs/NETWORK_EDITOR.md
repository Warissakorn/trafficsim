# Native network editor

The editor supports network drawing, demand and fixed-time control editing, persistence
and recovery, and simulation on the same canvas. The M0 core remains an unvalidated
prototype: merges, internal sources and cyclic routes are rejected at Run; geometric
crossing conflicts, lane changing and priority control are not implemented.

The owner's M0 plausibility and M1 timed usability gates remain open. See
[M1_ACCEPTANCE.md](M1_ACCEPTANCE.md). Running a drawing is not scientific validation.

## Start

Build as described in [BUILDING.md](BUILDING.md), then run:

```bash
./build/desktop/bin/trafficsim-desktop
./build/desktop/bin/trafficsim-desktop --language th
./build/desktop/bin/trafficsim-desktop --scenario network.traffic.json
./build/desktop/bin/trafficsim-desktop --scenario data/scenarios/crossing.json
```

Since M1.24 the editor is what `trafficsim-desktop` opens; the separate M0 simulation window
is gone and `--editor` is accepted as a no-op.

A new editor starts empty. Multiple editors own independent documents and recovery
locks. The Network Objects sidebar stays visible; Properties is on the right and the
Objects and problems dock is below. Switching language updates controls and messages.

## Draw and navigate

In **Select (S)** or **Links (L)**, Ctrl+right-drag from empty space creates a Link.
Confirm lane count and width in Link Data. Starting the same drag on a Link and ending
on another Link creates a Connector instead. The release position is used even if the
last mouse-move event was coalesced or Ctrl was released before the mouse button. Left-click while creating
adds intermediate points. The existing click-polyline workflow also works: click each
point and press Enter or double-click to finish. Escape cancels an unfinished gesture.

Select **Select / move (S)** to pick a link body or connector path. Drag a link control
point to reshape it, or drag its body between control points to translate it. Every
completed drag is one undo entry. A connector keeps its own position: its body can be dragged
like a link's, its interior points reshape it, and its two end handles re-attach it to another
lane. Drag one off its links and it is deleted — see below.

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
| Ctrl+left-click | Add an object to the selection |
| Ctrl+left-drag on a selected object | Duplicate the selection at the drag offset |
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

Reanchoring applies the similarity transform that maps a connector's previous endpoint
chord onto its new one, so every interior point keeps its position relative to that chord
and the curve is carried rigidly, scaling uniformly if the ends move apart. The result
depends only on where the endpoints are now, never on the edits that put them there:
returning a link to an earlier position restores a hand-edited curve exactly, rather than
leaving it deformed by however many moves took it away. It is not a turning-radius or
swept-path guarantee; inspect tight turns.

## Connector lane ranges

Select **Connectors (C)**, then Ctrl+right-drag from a source link lane to a target link
lane. The dialog chooses the first lane and contiguous lane count at each end. Left-click
during creation can add intermediate points. One completed gesture creates one connector
object even when it carries several lanes.

Both ends can attach anywhere on the **body** of their Link; endpoints remain valid.
The two-click workflow (C) also picks positions on lane bodies. Picking **snaps to a point**,
which is Vissim's *Snap to Points*: the lane's two endpoints, then a station another Connector
already attaches at on that lane, then the Link's own intermediate points — the first of those
within the radius wins, and the station taken is that point's exactly. Two movements leaving one
corner are therefore authored at one cross-section rather than centimetres apart, which is under
`kMinSectionLength` and would be refused at Run, and a Connector meeting a Link at a bend lands on
the bend. The radius is a screen distance, so zooming in past roughly 20 pixels per metre narrows
it below that limit and fine placement wins, which is what an author zoomed that far in is asking
for. The endpoint snap is not cosmetic: a Connector at a Link end is stored with **no** station at
all and compiles to a departure, while one a few centimetres short is a body attachment whose stub
section is under `kMinSectionLength` and cannot Run. Hidden levels cannot be picked. Properties → Connectors exposes
actual link/lane IDs and `from.station` / `to.station` in metres from the Link's start, as
Vissim stores a position. One station names one cross-section, so every lane of a range meets
the Link square even on a curve, and the number means the same thing whichever lane is picked.
The dialog opens at **one lane per end** — a gesture that starts on a single lane authors a
single-lane Connector, and a wider range is raised deliberately, within the lanes each end has.
Changing lanes preserves the selected stations, and so does adding or removing lanes: a
station is measured on the reference polyline, which `laneOffset` keeps fixed. Releasing outside a target Link reports
why nothing was created. Esc and Cancel leave the document and history unchanged.

A connector stores a base polyline and source/target lane counts. Its lane paths are
derived in monotone order, with stable IDs: the first uses the connector ID and subsequent
paths use `id/lane-2`, `id/lane-3`, etc. Routes and signal heads can reference those paths.
Unequal counts express fan-outs or merges in the drawing; merging still fails the M0
run check. Connector ranges are limited by the existing lanes, at most 12 per end.

In Select (S), orange **lane tabs** exist even on a one-lane Connector. Each is drawn as a
rounded tab on the edge of the carriageway, joined to it by a short stem, with the resulting
lane count inside it — one shape saying what it edits and what the release will produce:

- Source handles on both sides: grow/shrink the contiguous source lane range.
- Target handles on both sides: grow/shrink the contiguous target lane range independently.
- Middle handles on both sides: set both ends to the same count, limited by available lanes.
- Each selected Link has a tab on **both sides** to add/remove lanes (up to 12). Existing
  widths and world positions are retained. Added lanes use the width of the dragged edge lane.

Drag outward to add lanes and inward to remove them; the number and geometry preview
update during the drag. One release is one undo entry. Esc cancels. Each tab changes
its own edge, leaving the opposite edge fixed. The first-side handles add/remove lanes
before the current first lane; the other handles change the last lane. Surviving lane
IDs and positions stay fixed, including on curved Links. Connector paths whose lane pair
survives a range edit retain their curve; unequal ranges can intentionally change lane mappings.
Properties count edits and downstream pocket creation expand the last-lane side.

## Lane edges, road surfaces and markings

Lane edges are mitered at a bend: a corner vertex is offset by `width/2 / cos(theta/2)`, the
distance to where the two offset legs meet, so the carriageway keeps its full width through
the corner instead of pinching to `width * cos(theta/2)` — 30% narrower at a right angle. A
straight polyline is unaffected, bit for bit. A turn sharper than about 151 degrees is cut
back to four times the offset so a hairpin cannot spike. Where a bend is tighter than the offset
itself the inner edge still crosses itself, but the surface is filled by winding rule, so the
overlap stays road instead of being punched out as a hole.

**A mitered corner reads wide along the cross-section, and that is not a fault.** The distance
between two boundaries *at* a corner vertex is `width / cos(theta/2)` — 9.94 m of a 7.00 m
carriageway at 90 degrees — because that is what the diagonal of a mitered joint is. Projected
across either leg the carriageway is exactly its full width. A width measured between boundary
vertices is only a width when it is taken square to the road; this was once recorded as a 24%
bulge and is not one (M1.12.2).

Road surfaces use the same geometry as lane positions: outer boundaries are solid and
internal lane boundaries are dashed. There is no dashed line down a lane centre. On a Connector
whose ends carry different lane counts, an interior divider is drawn only over the stretch where
the two lanes it separates are genuinely side by side — at least half their full spacing apart —
and stops where they converge, rather than continuing down the middle of the single lane they
merge into. `connectorMarkings` decides this once for the editor and the diagnostic view. The
small centre arrows show travel direction; white dots are editable geometry handles.

**A Connector may carry its own lane widths and divider markings** (Vissim's `Lanes` tab). The
Properties fields take comma-separated metres, one per lane, and comma-separated `solid`/`dashed`
names, one per interior divider; leaving either blank derives it from the Links the Connector
joins, which is what every Connector drawn before this field does. The two outer edges are always
solid. A range resize that changes the lane count clears both, because an entry the author never
typed is not a width they chose. A partial list is refused.

## Geometry and end handles

Geometry handles sit on the **centreline of the whole bundle**, as Vissim shows them, not on
the stored reference polyline — which ends up at one edge as soon as lanes are added to a
single side — and not on a Connector's first lane path. Dragging one moves the stored point
by the same offset, so what the pointer holds is what moves.

A Connector's two end handles are dark squares at the middle of its end cross-section, and
they are draggable: drop one on any lane to re-attach that end, anywhere along the lane. The
range is centred on the lane under the pointer and slides to stay inside the Link. If the new
end has fewer lanes than the Connector carries, the range is narrowed to what is there rather
than the move being rejected; a wider Link never widens the range on its own. Dropping outside
a Link leaves the attachment alone, Esc cancels, and one release is one undo entry. A
Connector used by a route or head still cannot be re-attached. Properties shows both lane
counts beside the length, so a 3 → 2 lane drop is visible without opening the inspector.
Connector path count is the larger of its source and target counts, not a third independent
lane topology. Its cross-section is built from the widths of the lanes it carries, not blended
between its two mouths: a lane that continues keeps the width its links give it from end to end,
and a lane the far end has no room for is drawn as a taper closing onto its neighbour, the way
Vissim draws a lane drop. Which lane continues follows the selected ranges, pairing them in lane
order, so a lane range anchored one lane over moves the taper to the other side. The taper runs
the whole length of the Connector; there is no separate taper length to set. Those widths are stacked
along the same mitered offset a Link's own lane edges use (`offsetGeometry`), so every lane is its
full width square to the road at every point, through a bend and past a poly point the author has
dragged. Ordinary mouths fit the Link cross-section; steep or reversed arrivals use a
full-width square fallback with an advisory (see below). Properties exposes both counts as an alternative. Retargeting
or resizing a connector used by a route or head is rejected; revise those references
first. Reshaping its curve remains allowed if the whole document validates.

## Connector shape: intermediate points and the mouth

A Connector is stored and drawn the way Vissim's is: its two attachments and a few
**intermediate points**, joined by **straight legs and mitered at each point**, exactly as a Link
is. It is not smoothed — a Connector with two intermediate points is three straight legs with a
corner at each one, which is what Vissim draws. The count is what decides how closely that polygon
follows the turn, and it is Vissim's own `Intermediate points` field, in Properties → Connectors.

Changing the count never re-derives the default curve; `Reset curve` is the button for that.
Raising it splits the longest leg each time, so every point already there survives and the drawn
line does not move at all. Lowering it spaces the points evenly along the shape that is there,
giving up only the corners the lower count cannot hold. A new Connector gets 3; 0 leaves one
straight leg between the attachments. `Reset curve` lays 3 along the arc, and laying more along
it follows the turn more closely — 2.29 m of sag from the arc at one point, 0.60 m at three,
under 0.10 m at fifteen.

**The mouth must not become a needle.** Ordinary forward arrivals use the bounded M1.19
projection/slide onto the Link's cross-section. Near-perpendicular arrivals (forward tangent
dot product below 0.25), including backwards approaches, cannot be aligned this way without
collapsing or reversing lane order. Those ends retain their full-width square cross-section
and do not slide. `WARN_CONNECTOR_ALIGNMENT` flags the fallback or a residual gap over 1 cm.
The warning is advisory: the drawing can be saved, but exact lane-edge alignment is not
claimed. Use `Reset curve` or adjust intermediate points/attachments to approach with traffic.

Dragging an endpoint grip or changing its reference in Properties rebuilds the turn at its
existing point count. Keeping stale interior points when an endpoint crossed them caused the
old wrong-way elbow. This explicit retarget is one undoable edit; Undo restores the entire old
shape. Moving a Link still follows the separate world-position/deletion contract described
above. Merely opening a file does not regenerate any authored curve.

Interior points are editable; dragging one moves that corner and nothing else. Reset curve to
lane directions lays the points along a cubic. Each control point reaches `(2/3)·chord·tan(α/2)/sin(α)`, where α is the angle
between **that end's** lane direction and the chord — the cubic that stands in for a circular
arc leaving at that angle. It is `chord/3` as α tends to zero, the constant every turn used to
get, `(2/3)·chord` for a U-turn, which used to be drawn at less than half the radius it needs
(0.18 of the chord instead of 0.45), and it is the only reading that catches a reverse curve,
whose two ends are parallel while each still leaves its chord steeply (0.18 of the chord to
0.22 on a measured S). That reach is held at its 120-degree value, `(4/3)·chord`: past there it
runs away — 11.05 times the chord at 160 degrees — and a Connector drawn where two links nearly
touch left the junction altogether, measured at 11.0 times its own chord and now 1.9. Every
ordinary turn, U-turn included, is unchanged to the last bit. A Connector that still turns tighter than its own width is reported in Objects and
issues as `TIGHT_CONNECTOR_RADIUS`; the drawing is kept and Run is not blocked.
Make straight retains only endpoints. There are no separate persisted
Bézier handles and arbitrary edits need not remain smooth, though a reshaped curve is
held to the same geometry rules as a link: no non-finite coordinates, no repeated
consecutive points and a positive total length. Coincident endpoints cannot
generate a default curve; leave a positive gap. Duplicate lane-pair connections at the same source/target stations are
rejected, including pairs already covered by another connector range. Separate stations
on the same lane pair may own separate Connectors.

## Attachment stations, Link edits and deletion

**A Connector keeps its own position.** Its geometry is what the author drew; it is not recomputed
from its Links on every edit. Moving or reshaping a Link therefore leaves the Connector standing
where it is, and what changes is which station of the Link each end now sits on:

- **The end is still on the Link's carriageway** — it snaps onto the middle of the lane under it,
  and its stored station moves to match. The lane it names may change where a lane bundle edit has
  slid the lanes sideways under a standing end; the Link never does.
- **The end is off the Link** — by more than half a lane width, which is where the carriageway
  stops — the Connector has nothing left to connect. It is **deleted**, with the routes and signal
  heads that named it, inside the same transaction as the edit that moved it, so one Undo brings
  the Link edit and the Connector back together.

That is the rule for an edit that MOVES a road: reshaping a Link, translating a selection, dragging
a Connector's body. An edit that RE-LAYS a Link's lanes without moving the road — a lane added or
removed, a width changed, the driving side flipped — is not a move: every Connector follows the lane
it names to where that lane now is, at the same station. Deleting a Connector because the author
added a lane to the Link beside it would be a surprise, not a rule.

Shortening a Link past an attachment now deletes the Connector that hung off it, rather than
clamping the attachment to the new end. Clamping moved a Connector to somewhere the author had not
put it; this leaves it where they did and says so by removing it. (A Signal head still rides its
station and is not moved: its position is validated against its lane, so an edit that would strand
one is rejected instead.) Reset curve re-derives the whole shape when that is what is wanted.

Splitting remaps both source and target attachments to the appropriate child Link by arithmetic
alone — the upstream child's polyline is a prefix, so its stations are unchanged, and downstream
stations shift by the cut. A cut through an attachment inside the 0.2 m continuity span is
rejected; move the split at least 0.1 m away.

**Runtime:** body attachments run (M1.11.1). The lane is cut into sections at every station a
Connector attaches to, so a vehicle leaving part way along travels only that far, and one arriving
part way along joins at the drawn metre rather than at the start of the lane. An arrival is a
merge, and the arriving Connector gives way to the traffic already on the lane under a priority
rule derived from the drawing, whose gap time and headway come from `data/priority-rules/`;
Run reports `EDIT_NO_PRIORITY_DEFAULTS` if those cannot be read, and never invents a zero gap
time. Run and Diagnostics still report `UNSUPPORTED_CONNECTOR_POSITION` for one case: an
attachment within 0.2 m of a lane end or of another attachment on the same lane, which leaves no
section between them. A lane with nothing attached to its body compiles exactly as it always did,
so existing endpoint-only projects keep their runtime behavior and capability guards.

Deleting a connector removes heads on its paths, affected routes and their vehicle
inputs in the same undoable transaction. Heads on unaffected links remain.

## Selection, tables and display

**Name.** Every Link, Connector and Signal head carries Vissim's `Name`. The field sits beside
the read-only id at the top of the inspector, above the property tabs, because a name belongs
to an object rather than to a kind of object: it names whatever is selected. It commits on
Return or when focus leaves the field, and only when the text changed, so clicking out of an
untouched field is not an undo entry. A name is free text of at most 200 characters and is
never a key — two objects may carry the same one, an empty one is normal, and nothing is ever
looked up by it. The three object lists show it in a Name column beside ID, and it copies with
a duplicated object.

Click replaces the selection; Ctrl-click adds an object, and Shift-click toggles it.
Dragging on empty space selects touched Links, Connectors and Signal heads in a rectangle;
Ctrl/Shift adds that rectangle to the existing selection. Picking, framing and the band
follow the visible road surface, including roads expanded away from the reference line.
The last selected object is primary; property and geometry edits act on it alone.

**Moving several objects.** Left-drag any member of a multi-selection and the whole selection
moves, with the same translucent outline the copy drag shows. Links and explicitly selected
Connectors move; a Connector also moves automatically when both its Links move. Other
Connectors keep their world positions and are reanchored or removed with their dependent
references when no longer on their Links (see the attachment section). Signal heads ride their
parent road. A selection containing only heads cannot move independently. Each move is one
undo entry; a drag below the system threshold is a click. Arrow keys nudge the selection;
see [History, keyboard editing and rotation](EDITOR_WORKFLOW.md) for increments and cancellation.
Alt-left-drag rotates the selection; Shift snaps to 15°. Ctrl+Shift+R opens the exact-angle dialog.

Hold Ctrl and left-drag an already selected object to duplicate the whole selection.
A translucent outline previews the drag offset; release commits once, including when
Ctrl is released first. A click or small jitter adds selection without copying. Esc cancels.
Internal Connectors and Signal heads copy with their Links, fresh IDs and level/style.
Heads share their existing programs. A Connector can also be copied independently: both
translated ends must drop onto existing lane ranges at their respective original levels.
A Signal head can be selected and copied onto a lane or Connector at the same level.
Invalid drops leave the entire document and selection unchanged. Routes/inputs are not
copied, because copying a drawing must not silently double arrivals. Delete selected
objects cascades dependent connectors, heads, routes and inputs; one Undo restores all.

Links, Connectors and Signal heads tables mirror network objects and support selection
and framing on the canvas. A head selects itself. Ctrl/Shift table selection also retains
selected objects of other types. These three spatial object types share canvas selection,
copy and deletion gestures; nonspatial demand/program objects use their dialogs.

Routes, Vehicle inputs and Signal programs have Add/Edit/Delete actions and dialogs;
double-click their rows to edit. Table cells are read-only views of the document.

Properties → Level and Display type apply to the primary link/connector. Level orders
drawing and picking; vehicles and heads use their carrying object's level. The sidebar
filters to all levels or one catalog level. Tab can reach a lower overlapping object.
Changing that filter removes hidden selections. Selecting a hidden object explicitly from
a table or inspector switches to all levels so the edit target is visible.

Definitions live in `data/levels/*.json` and `data/display-types/*.json`, loaded in filename
order. Add a JSON entry with English/Thai names to add a level or style; no C++ edit is
needed. Catalogs require ground level 0 and a default style. Unknown stored styles keep
their ID and render with the default, with a missing-style indication in Properties.
Levels only affect display and selection; they do not change runtime conflicts.

## Routes, inputs and fixed-time signals

1. Draw a continuous path using links and connectors.
2. With the Routes tool (R), click the start link, then click each destination: the whole chain
   leading there is appended, so a crossing takes two clicks. A route names Links and Connectors
   and covers **every lane** of them, so a Connector's lane count never invalidates it.
   Ctrl+right-click starts one the same way, Backspace removes the last object, Enter stores it,
   Esc cancels; the draft and the selected route draw with arrows, the hovered object is haloed,
   and a click no chain reaches, or two reach equally, authors nothing (Add is the same, by dialog).
3. With the Vehicle inputs tool (V), click the link traffic enters on: the dialog opens on the
   route starting there or, when none does, on **following the network from that link** with no
   route (M2.1.1). The volume is the **link total**, divided across lanes (equally or by
   weights; no lane changing). A routing decision can be **placed on a link** with destination
   links and relative flows, and draws as a diamond (SIMULATION.md). Each input draws a chevron
   and its volume. Right-clicking a route, input or decision marker edits or deletes it.
4. Signal programs edits ordered duration/color phases and cycle offset. Add a signal
   head on a lane or derived connector path, choose a program and position in metres.
   Program deletion is blocked while a head references it.
5. Run settings edits duration and fixed timeStep together. All demand/control changes
   validate and commit through the same Undo/Redo history as network edits.

Vehicle types and driver behaviours normally resolve from `data/`; Embed catalogs stores
explicit copies for portability, and an explicit empty override stays empty rather than
falling back to installed data. Intrinsic demand errors block edits; unsupported runtime
topology is a separate diagnostic.

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

Save/Open uses `*.traffic.json`. Schema 3 stores format, schemaVersion, revision, nextId,
network, optional typed definition and background/transform. It never stores a second
editable runtime network. Schema 1 migrates with one-lane connector counts, level 0 and
default display type while preserving IDs. References without a position retain
source-end/target-start semantics at every schema. Schema 3 makes the attachment contract
explicit so earlier applications reject these files instead of discarding positions.
**Schema 5 stores `station` in metres**; schemas 1–4 stored `fraction` of lane arclength and
are converted on read, at the same world position. The version decides which key is read and
what it means — a `fraction` in a schema 5 file, or a `station` in an older one, is rejected
rather than silently reinterpreted as the other unit.
Unknown future versions are rejected.

| File kind | M0 scenario | Editor project |
|---|---|---|
| Typical name | `crossing.json` | `network.traffic.json` |
| Root keys | `network`, `definition` | Versioned envelope plus network, definition and background |
| Definition | Required | Optional until demand/control is authored |
| Opened by | Editor (or `trafficsim-cli --scenario`) | Editor |
| Runs | Editor, or `trafficsim-cli` | Editor after demand/catalog/runtime checks |

The editor opens bare M0 authoring files and schema-1–6 projects and saves schema 7.
Schema 4 stores the lane bundle offset and Connector interpolation weights; older versions
default to centred lanes and arclength interpolation. Old files retain their positions.
A bare M0 scenario carries no `schemaVersion`, so the editor reads it with the pre-5
meaning; saving it writes a schema-7 project. Writing scenario JSON back out is not
implemented — `trafficsim-cli` still reads the format, and `loadScenario` still rejects a
project with `SCENARIO_IS_PROJECT` for callers that want only a runnable scenario.

Schema 7 adds shared Link boundary markings and `none`/`double` marking kinds. Unsupported
schema-7 network object fields are rejected before replacing the current document. See
[Authoring extensions](AUTHORING_EXTENSIONS.md) for the exact supported subset and the new
Link station-insert, midpoint, straighten and unreferenced-reverse inspector actions.

Save uses atomic QSaveFile replacement without direct-write fallback. A failed save keeps the
previous destination and dirty state; a failed load keeps the current model. An empty or
unrunnable drawing can be saved, and New/Open/Close prompt Save/Discard/Cancel. Undo history is
in memory, bounded to 100 operations; saved-revision tracking drives the title's asterisk, and
history resets on load.
The **History** toolbar button or **Ctrl+Shift+H** opens named states with current/saved markers.
Double-click or Enter restores a state; browsing does not edit. See [the workflow guide](EDITOR_WORKFLOW.md).

Every 15 seconds a dirty revision is atomically written to a recovery copy in the platform's
application-data recovery directory. Per-window UUIDs and process locks prevent offering copies
owned by active editors; startup offers stale ones and Recover can inspect them later. Recovery
validates before replacing the document, then opens untitled and dirty so Save asks for a real
destination, and a successful save or intentional discard removes the consumed copy. It is a
checkpoint, not a persisted Undo log: up to 15 seconds of edits may be absent after a crash.

## Verification

CTest covers core/baseline replay, the model/project/command suites and native offscreen UI
workflows: controlled splits, both driving sides, connector ranges, catalog override semantics,
atomic rollback, reference-safe deletion, persistence, migration, demand dialogs and pointer
authoring, in-editor Run and deterministic Reset, recovery, Thai UI, creation/cancellation
gestures and level-aware overlap selection. They do **not** perform the owner's timed exercise,
establish model fidelity or measure large-network performance, and Windows core checks do not
imply Windows/macOS GUI verification; installers remain M7.
