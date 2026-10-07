# Native network editor

The editor supports network drawing, demand and fixed-time control editing, persistence
and recovery, and simulation on the same canvas. The core remains an unvalidated prototype.
Supported merges use derived or authored right-of-way; authored crossing areas and Stop/Yield
controls also run. Unsupported merge topology, internal sources and cyclic routes still block
Run. Mandatory lane changing and cooperative braking run; discretionary changes are implemented
but disabled. See [SIMULATION](SIMULATION.md) and the relevant contracts for their limits.

Gate status is maintained in [ROADMAP](../ROADMAP.md), with the dated owner ruling in
[M1_ACCEPTANCE](../plans/M1_ACCEPTANCE.md). Running a drawing is not scientific validation.

## Start

Build as described in [BUILDING.md](../BUILDING.md), then run:

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

**Default gesture rule (D84).** Outside Routes, a plain left click only selects, and **Ctrl+right-click
or Ctrl+right-drag creates or changes** — Links, Connectors, split points, routes, vehicle inputs,
signal heads, queue counters and conflict-area priority alike, so a stray click never authors.
Dragging an already selected object (move, vertex, waiting line) stays a left drag. Routes use click-start, hover-trace, click-destination (D116); Measure/Calibrate also use left clicks
to place measuring points without changing the network.

In **Select (S)** or **Links (L)**, Ctrl+right-drag from empty space creates a Link.
Confirm lane count and width in Link Data. Starting the same drag on a Link and ending
on another Link creates a Connector instead. The release position is used even if the
last mouse-move event was coalesced or Ctrl was released before the mouse button. Left-click while creating
adds intermediate points. Escape cancels an unfinished gesture. (The earlier left-click
polyline workflow was removed by D84: a left click no longer starts a Link.)

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
| Ctrl+Tab on the canvas | Cycle objects overlapping the last click position (plain Tab moves focus) |
| Delete / Ctrl+Delete | Delete objects / remove selected geometry point |
| Ctrl+B / Ctrl+I / Ctrl+Shift+O | Toggle background / Properties / object tables |
| Ctrl+A | Wireframe display: Links and Connectors as centre lines only (a focused text field keeps select-all) |
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
change. Shrinking away a lane referenced by a Connector range, signal head or explicit
queue-counter point is rejected. Routes name Links, so a route alone does not prevent
lane removal; its runtime lane chains are rebuilt from the edited Link.

Create opposite carriageway makes a separate directed link with reversed geometry,
offset on the median side according to combined lane widths and the carriageway gap.
It copies level/style. It is independent after creation.

Split at distance measures from the selected centreline's start. The split tool
projects a Ctrl+right-click onto that centreline. Splitting produces upstream and downstream
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

Moved to [`NETWORK_EDITOR_CONNECTORS.md`](NETWORK_EDITOR_CONNECTORS.md) §"Connector lane ranges", which keeps this file under the size limit.

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
merge into. `connectorMarkings` decides this once for the editor and the diagnostic view. Travel
direction shows only on a selected object: its selection outline's two long edges carry an
arrowhead every ~72 screen px (D83); there is no arrow in the middle of the road. Small round
grips are editable geometry points, shown for a single object in Select. While a body is being
moved its geometry points and lane tabs are hidden and return on release; the pointer stays an
arrow when pointing and shows an arrow with a hand beside it only while something is carried.

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

Moved to [`NETWORK_EDITOR_CONNECTORS.md`](NETWORK_EDITOR_CONNECTORS.md) §"Connector shape: intermediate points and the mouth", which keeps this file under the size limit.

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
A Signal head is its stop line: placed by Ctrl+right-click at the pointer's station on a lane or Connector path, dragged along its lane, copied onto a lane or Connector at the same level (M2.7a, D47).
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
filters to all levels or one catalog level. Ctrl+Tab can reach a lower overlapping object.
Changing that filter removes hidden selections. Selecting a hidden object explicitly from
a table or inspector switches to all levels so the edit target is visible.

Definitions live in `data/levels/*.json` and `data/display-types/*.json`, loaded in filename
order. Add a JSON entry with English/Thai names to add a level or style; no C++ edit is
needed. Catalogs require ground level 0 and a default style. Unknown stored styles keep
their ID and render with the default, with a missing-style indication in Properties.
Levels only affect display and selection; they do not change runtime conflicts.

## Routes, inputs and fixed-time signals

1. Draw a continuous path using links and connectors.
2. With Routes (R), click the start road, move over connected roads to choose the path,
   then click the destination to save one History entry. Crossbars mark start/end and a static
   blue tint follows the actual route spans. Passing an intermediate branch disambiguates
   equal paths. Backspace goes back; Esc, focus loss or tool/level change cancels.
   Ctrl+right-click start/append and Enter remain available. Routes still name whole Links
   and Connectors, never new arbitrary endpoint stations. See [EDITOR_WORKFLOW](EDITOR_WORKFLOW.md).
3. With the Vehicle inputs tool (V), Ctrl+right-click the link traffic enters on: the dialog opens on the
   route starting there or, when none does, on **following the network from that link** with no
   route (M2.1.1). The volume is the **link total**, divided across the link's lanes (equally or
   by weights); a vehicle on a lane that cannot reach the route's end changes lanes (M3.2.8b). A routing decision can be **placed on a link** with destination
   links and relative flows, and draws as a diamond (SIMULATION.md). Each input draws entry-lane crossbars
   and its localized rate, or a period count when interval-based. Right-clicking a route, input or decision marker edits or deletes it.
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

Run (F5) compiles one revision into a detached snapshot, initializes the selected 32-bit seed and
plays fixed steps on the canvas. Pause keeps the state; Step advances one timeStep; Reset recreates
the initial state for the same seed. Playback speed changes scheduling only. Status shows revision,
seed, time and active/pending/completed counts; seeded arrivals may leave the view empty at first.

Each vehicle is drawn at its type's true length × width, front bumper at its position and turned
along the road, in its type's colour from `data/vehicle-appearance/` (a type with no entry takes
its road's display-type vehicle colour). A windshield, and on a vehicle over 7 m a cab gap, appear
once the body is about 14 px long; zoomed far out a vehicle is floored at 4 × 2.5 px so it never
vanishes (D97).

A lane change is drawn as a 3 s slide (D102): the engine moves the vehicle to its new lane in one
tick, and the Run view draws it easing across from the lane it left, nose turned toward the new
lane by its sideways over forward speed (forward floored at 5 m/s). Display only — nothing the
engine decides or the results measure changes; a second change inside the 3 s restarts the slide
from the lane it left.

Successful edits, Undo/Redo, opening/new documents and seed changes invalidate the run; the next Run compiles the current document.

**Results** tab (M2.5, D39/D40): per-movement vehicles, mean delay and mean travel time beside
per-approach mean and maximum queue, for the current run. While the run is still going it shows
the figures so far with the time reached. The note above the tables is the not-yet-validated
marker and says what the delay is not (HCM control delay, LOS). **Export results (CSV)…** (D104),
on the tab, in the Simulation menu and in the command palette, writes the same file as
`trafficsim-cli --csv`, marker line first. It is enabled only once the run has reached its end.

**Conflict areas** tab (M3.2.4a, D60): Add crossing areas on two selected roads, Take over merge on a
Connector; Enter edits priority, `gapTime`, `headway`. Run protects authored areas only, and says so.
**Conflict area tool** (`A`, M3.2.4b, D61): click an area to select it, Ctrl+right-click it or press `P` to
cycle its priority, drag a dashed waiting line along its lane. Since M3.2.4c (D68) the tool also shows what the drawing implies with no Add step: grey dashed passive crossings (not enforced at Run, as in Vissim) and each merge with its derived priority; a Ctrl+right-click or `P` on its row sets one, Delete makes a crossing passive again. Branching mouths are also shown in red; a click selects them, but priority editing is unavailable because they keep the existing vehicle-order behavior. Single-stream continuation mouths are not conflicts. Crossing and merge highlights are separate bands along each driving direction, cut at the measured overlap spans (D117/D118). Their normal rail offset is 0.5 m per side, capped at 20% of local width; the ends are not shortened. Picking uses the visible bands, while grouping retains the exact measured polygons; merge bands use physical-mouth spans rather than the one-metre admission markers. P3–P4 continues on the attached Link lane, with stations belonging to that Link. Connected Crossing/Merge/Branching of the same owner pair share one row; priority edits affect Crossing/Merge together, Branching stays read-only, and newly introduced merge-order cycles refuse the whole edit. Separate sites remain separate. Nothing automatic is saved. A set area follows its overlap when a Link or Connector is edited, its lines keeping their distance, and is removed with its rule and Stop/Yield when the two roads no longer overlap (D86, one Undo step). Add crossing areas refuses a pair whose every overlap is already set. The side that gives way is hatched. The dialog's
Control field (M3.2.5b, D63) sets Stop/Yield for the line: dashed = none, solid amber = Yield, solid red = Stop. **Queue counter tool** (`Q`, M3.2.6c, D65): Ctrl+right-click stop lines, waiting lines or places on a Link lane, Enter creates one counter (Backspace drops a line, Esc cancels); violet dotted bars. The **Queue counters** tab adds one over the selected heads, renames, deletes, and says which approach row it replaces.

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

Ctrl+A (View → Wireframe display) draws every Link and Connector as its centre line, as
Vissim's simple link display. It is view state like Ctrl+B: not saved, and off in a new window.
What is drawn is what is hit, band-selected and framed; the lane tabs are hidden because their
rails are not drawn (lane count stays in Properties). In the Run view vehicles keep their real
lane positions, so on a multi-lane road they sit either side of the line (D100).

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

The editor opens bare M0 authoring files and project schemas 1–21. Saving uses schema 17
by default, 18 for owned composition catalogs or embedded vehicle names, and 19 for
composition periods or type-conditioned routing; schema 20 when a routing decision has `position`; schema 21 when the behaviour library
or a road behaviour-type assignment is used ([DRIVING_BEHAVIOUR](DRIVING_BEHAVIOUR.md#7-implemented-library-and-codec-m332a-d126)). This is selected from the document's
features, not the version of the file opened. See [Demand catalogs](DEMAND_CATALOGS.md)
and [time/type rules](DEMAND_TIME_TYPES.md); old feature-free fixtures retain schema-17 bytes.
Schema 4 introduced lane bundle offset and Connector interpolation weights; older versions
default to centred lanes and arclength interpolation. A bare M0 scenario has no
`schemaVersion`, so its attachments use the pre-5 interpretation; Save writes a versioned
project using the same feature rule. Writing scenario JSON back out is not
implemented — `trafficsim-cli` still reads the format, and `loadScenario` still rejects a
project with `SCENARIO_IS_PROJECT` for callers that want only a runnable scenario.

Schema 7 introduced shared Link boundary markings and `none`/`double` marking kinds.
For schemas 7–19, supported network-object keys depend on the version; unknown fields are
rejected before replacing the current document, not silently dropped. See
[Link authoring and markings](AUTHORING_EXTENSIONS.md) for geometry actions, reference-safe
reversal and markings. The supplied spec's schema snippets are not a complete codec contract.

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

For first-click recognition stations, shared Route points, line dragging and runtime
semantics, see [POSITIONED_ROUTING](POSITIONED_ROUTING.md). Missing `position` retains legacy behaviour.
