# History, keyboard editing and rotation

The Network Editor offers the same workflow in English and Thai. These controls extend the
existing authoring commands; they do not change the project format or simulation model.

## Selection and editing feedback

Hover, selection and active tool previews use the shared palette accent (D81/D84), with
different stroke/tint treatment; road display-type colour and markings remain visible. Geometry points are small square
grips, shown only for a single object in Select. Rectangular lane tabs sit directly against
both edges at the start, middle and end; drag a tab outward/inward to add/remove lanes. Tabs
have no count labels. Lane markings are 0.10 m wide, with 3 m dashes and gaps that scale with
zoom. Each stroke of a double marking has the same width.

A plain click on empty space clears selection, Properties and table-owned highlights. Shift
or Ctrl with an empty click preserves the selection; dragging a box adds its objects. Change
of tool clears the previous selection and cancels unfinished gestures. Table selection in an
authoring tool can still show properties, but exposes no geometry/lane grips. Ctrl+Tab cycling and
canvas Delete operate in Select (conflict areas also support Ctrl+Tab in their own tool).

Escape first cancels an unfinished gesture/draft; when idle, it clears selection. Empty-space
clicks in any authoring tool clear selection while keeping an unfinished
multi-click draft, without showing an error. Enter commits and Escape cancels those drafts.
Plain right/middle drag remains pan; Ctrl+right-click or Ctrl+right-drag creates or changes
in every tool, and a plain left click never authors outside Routes (D116) and Measure/Calibrate. Shift-click toggles, Ctrl-click adds an unselected object, and
Ctrl-drag of a selected object copies it, retaining the D30 convention.

## History

Use **View → History** or **Ctrl+Shift+H**. The dock lists reachable states from oldest
to newest, including undone states. The current state is bold and labelled; the saved state
has its own marker. Undo/Redo buttons name the operation they will perform.

Select a row and press **Enter**, or double-click it, to restore that state. Browsing with arrow
keys does not change the document. Restoration clears an existing simulation snapshot and
refreshes the canvas, properties, tables and diagnostics. Returning to the saved state clears
the unsaved asterisk. Undo/Redo history retains up to 100 edits; its oldest state may therefore
be later than the original document. Expired states cannot be restored.

Editing after undo creates a new branch and discards the previous redo future. Revision IDs
are not reused. Failed/no-op edits preserve history. Save marks the current state without
clearing history; New/Open/recovery begin a fresh history. The history is not saved in the file.

## Keyboard moves and cancellation

In **Select (S)** with the canvas focused, arrow keys move selected Links/Connectors by the grid
interval with Snap on, or by one metre with Snap off. **Shift** multiplies the step by ten.
Up means increasing world Y; Right means increasing X. Each key step is one undoable edit.
Text/spin fields keep their normal arrow-key behavior. Ctrl/Alt/Meta arrows do not move objects.

The existing group-move rules apply: internal Connectors move with both parent Links; other
Connectors retain their own positions unless selected explicitly. Moving a road away from an
attachment can remove that Connector and its dependent objects in the same undoable edit.
Heads ride their parent roads; head-only selections cannot be nudged independently.

Arrows cannot move objects during a mouse gesture. Escape, tool/selection/level changes and
loss of canvas focus cancel active drags, copies and Ctrl-right creation; a late mouse release
cannot commit the cancelled operation. Multi-click drawing/connection drafts remain available
when simply moving focus between controls between clicks.

Level filtering drops hidden selections and disables their editing controls. Explicit table,
inspector or diagnostic selection reveals a hidden object by switching the filter to all
levels. Invalid or deleted object IDs are not accepted as selection targets.

## Rotate a selection

In **Select (S)**, hold **Alt** and left-drag a selected road to rotate the whole selection.
Alt-dragging an unselected road first selects it. The pivot is the centre of the world-axis
bounds of the affected road surfaces, including internal Connectors. Signal heads do not
change that centre. The canvas shows the pivot, angle and accent outlines before committing.
Keep the pointer away from the pivot: a press or release within eight screen pixels of it
has no stable angle and does not rotate anything. A click or small pointer jitter also does
not rotate. Drag angles ignore the metre grid; hold **Shift** for **15-degree** increments.

Use **Rotate selection** on the toolbar or **Ctrl+Shift+R** to enter an exact angle from
−360 to +360 degrees, to two decimal places. Positive angles turn counter-clockwise in world
coordinates; negative angles turn clockwise. The dialog uses the same selection and pivot as
the gesture. Zero and complete turns leave the document, saved state and redo history intact.
This control also works when a window manager reserves the Alt-drag chord.

Selected Links and explicitly selected Connectors rotate once. A Connector also rotates when
both parent Links are selected; its authored stations, curve points, lane blend, lane widths
and markings are retained. Heads ride their parent roads at their existing stations. A
head-only selection cannot rotate independently. Unrelated selected heads stay on their roads.

Other Connectors keep their world positions under M1.20. After rotation, affected attachments
are re-read; an endpoint still on its parent carriageway snaps to its lane middle. A Connector
that leaves either parent road is removed together with its dependent routes, inputs and
heads. The accent outline previews the rigid transform before this attachment check. **One
Undo restores the entire edit**, including any removed dependants; Redo restores its result.
Rotation clears a compiled run only on a successful change. Save/reopen retains the geometry.
Escape, focus loss, tool/selection/level changes or document replacement cancel the drag;
a later release cannot commit it. Existing copy, move and vertex gestures keep their chords.

## Draw a route and a vehicle input

Routes use an explicit trace gesture (D116, owner request):

| Gesture | Effect |
|---|---|
| `R`, then click a road | Starts an unsaved route at the whole Link or Connector |
| Move over connected roads | Previews the actual path; passing a branch remembers that choice |
| Move back onto an earlier route object | Trims the preview back to that object |
| Click the destination road | Saves the complete preview as one undoable edit |
| `Backspace` | Removes the last object from the trace |
| `Esc`, tool/level change or focus loss | Cancels without storing anything |
| `Ctrl`+right-click | Retains the earlier start/append gesture; `Enter` saves it |
| `V`, then `Ctrl`+right-click a Link | Opens a vehicle input on that Link, with or without a route |
| Right-click a stored route or input | Edit, delete, or show in its table |

An unreachable or equally ambiguous path is rejected. Pass over the desired intermediate
branch to disambiguate it; the gesture does not guess a turn. Hover never mutates the document.
Double-click adds no second commit. Other authoring tools retain D84's Ctrl+right gesture.

Route start/end, vehicle inputs and signal heads share three-pixel cosmetic crossbars with
wider hit regions. Bars intersect the road normal with the actual lane rails. A route's
static blue surface tint follows its compiled spans, including mid-Link arrivals; it has no
straight rubber band, moving dashes or repeated arrows. The start/end are the existing route
object boundaries, not new arbitrary stations. Only the selected stored route is tinted.
Inputs mark the served entry lanes (separate bars across non-contiguous positive lane shares),
with a localized rate unit; interval inputs show their number of periods instead of a misleading
instantaneous scalar. Inputs on a hidden level are hidden. Signal bars cover only their controlled
lane. In Edit they are neutral, with a blue selection halo; Run colours the same snapshot bars
from the real signal program. Reset restores the authored bars. None of this paint state is saved.

## Workspace and screen space

The application opens maximized. The command bar uses one row on wide windows and two rows
below 1180 logical pixels, keeping playback and language controls reachable. File, Edit, View,
Simulation, Help and Language menus retain full command names and keyboard shortcuts.

- **Ctrl+Shift+F** (View → Focus on network) hides docks and restores the previous layout on
  the next press, including hidden and floating panels. Opening a panel also leaves focus mode.
- **View → Reset panel layout** restores the palette, Properties and Objects docks.
- **Ctrl+I**, **Ctrl+Shift+O**, **Ctrl+Shift+H** and **Ctrl+Shift+T** toggle Properties, Objects,
  History and the Network Objects palette respectively.
- Properties keeps ID, Name and its tabs visible while each tab scrolls independently.
  Expand **Network and appearance** for drivingSide, level and display type.
- Signal-head table actions appear on the Signal heads tab. Run settings and runnability
  checks are always available in the Simulation menu. **F1** opens gesture/keyboard help.

The unvalidated marker and complete simulation figures remain visible in focus mode.

Layout examples from the Linux offscreen Qt platform:
[English, 1360×860](../images/workspace-en.png),
[Thai, 1024×768](../images/workspace-th.png), and
[Thai focus mode](../images/workspace-focus.png).
