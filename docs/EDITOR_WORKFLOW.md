# History and keyboard editing

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
