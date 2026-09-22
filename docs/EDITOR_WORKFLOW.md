# History, keyboard editing and rotation

The Network Editor offers the same workflow in English and Thai. These controls extend the
existing authoring commands; they do not change the project format or simulation model.

## History

Use the History toolbar button or **Ctrl+Shift+H**. The dock lists reachable states from oldest
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
change that centre. The canvas shows the pivot, angle and amber outlines before committing.
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
heads. The amber outline previews the rigid transform before this attachment check. **One
Undo restores the entire edit**, including any removed dependants; Redo restores its result.
Rotation clears a compiled run only on a successful change. Save/reopen retains the geometry.
Escape, focus loss, tool/selection/level changes or document replacement cancel the drag;
a later release cannot commit it. Existing copy, move and vertex gestures keep their chords.
