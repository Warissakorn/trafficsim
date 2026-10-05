# Project Demand catalogs — slice 5

## Contract

A vehicle type defines one rigid vehicle's dimensions, speed range, acceleration,
deceleration, behavior reference and optional axle dimensions. A composition defines
relative positive weights over vehicle types; it never defines physical dimensions.
Names are authoring metadata. IDs are stable references, not editable labels.

Project schema 18 adds an optional `definition.compositions` array. Absence means
external composition files, preserving schema 1–17 behavior. Presence, including an
empty array, means a project-owned catalog. Existing embedded vehicle/behavior fields
keep their presence semantics. Vehicle names are stored on embedded type entries;
composition names are stored with their weights. The core receives neither metadata
nor composition authoring objects.

The catalog dialog resolves the current catalog once into local staging values.
Confirming captures all three catalogs (types, behaviors, compositions) together in
one History edit; Cancel does not change ownership, revisions or allocator state.
This makes saved edited projects independent of the installed Demand catalogs.
Legacy files without owned compositions or vehicle names retain schema 17 on save.
Owned compositions or embedded vehicle names require schema 18. Opening an old
project does not silently embed or upgrade its catalog contents.
Unedited projects continue to use installed catalogs.

All owned catalog IDs must be nonempty and unique within their catalog. Type numeric
rules and behavior references use the existing core validator. Composition entries
must refer to existing owned types, contain no duplicate type IDs, have finite positive
weights and a positive finite total. Input composition references are checked at
edit/save/load when the composition catalog is owned. Unused owned entries are checked
too. Referenced types/compositions cannot be deleted: inputs and composition members
must be updated explicitly. Changes are atomic and undoable.

The UI offers type/composition lists, add/edit/delete, immutable IDs, editable names,
engine parameters and optional axle fields. Shares are weights, not percentages;
zero in a dialog excludes a type, and at least one type must remain. Desired speeds
are displayed in m/s, dimensions in m and accelerations in m/s². A behavior selector
uses the captured behavior catalog. Behavior preset editing and new motorcycle or
articulated dynamics are outside this slice.

## Gates

- Old external projects retain absent composition fields and the same compiled values.
- Owned catalogs round-trip and compile with no external Demand directory.
- Invalid/duplicate weights and unknown references fail without changing History.
- Changing a type or composition changes Run and Preview from one source of truth.
- Cancel, no-op edits, Undo/Redo and save/reopen preserve catalog values and ownership.
- Type dimensions and axle sum, speed min/max and deceleration ordering use existing
  validators rather than a shell-only rule.
- Linux automated UI and native CI are distinct from owner appearance acceptance.

Time-varying compositions and type-specific routing remain slice 6. Reporting stays M5.
