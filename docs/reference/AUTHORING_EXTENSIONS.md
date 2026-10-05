# Link authoring and shared markings

Current geometry-command and marking semantics. The [M1.21 snapshot](../archive/AUTHORING_EXTENSIONS-M1.21.md)
preserves the earlier implementation horizon. Project codec rules live in
[NETWORK_EDITOR](NETWORK_EDITOR.md#save-recovery-and-formats); supplied specs are target
material, not implementation or validation claims.

## Link geometry actions

Select a Link, then use its Properties tab. Each successful action is one History command;
Undo restores the full document, including references removed by an ordinary reshape.

| Action | Behavior |
|---|---|
| Insert point at station | Insert on the reference polyline at a distance in metres from its start; endpoints/out-of-range/non-finite inputs are rejected |
| Add midpoint to longest segment | Subdivide the longest reference segment; first segment wins ties |
| Straighten Link | Keep its two endpoints and remove intermediate points; existing Connector reanchor/cleanup rules apply |
| Reverse Link with no positional dependants | Reverse geometry and lane order and negate lane offset, preserving each named lane's physical footprint; reverse markings as well |

Inserting an existing vertex is a no-op: no dirty flag, revision change or lost redo history.
Reversal is refused if a Connector, signal head or right-of-way/explicit counter control
references the Link. A route alone does not block reversal: it names the Link, not its
lanes (M1.26); lane IDs and the route reference survive. This is not a connected-network
direction reversal that automatically retargets positional dependants.
A closed polyline whose endpoints coincide cannot be straightened to a zero-length road.

## Shared boundary markings

`Link::boundaryMarkings` stores one entry per boundary in **lane order**, not physical
left-to-right. For N lanes there are N+1 entries. An empty list restores solid outer edges
and dashed interior dividers. This stores a shared lane boundary once rather than allowing
contradictory `leftMarking`/`rightMarking` values on adjacent lanes.

The Properties field accepts comma-separated `solid`, `dashed`, `none`, `double`.
For a two-lane Link, `none, double, dashed` hides boundary 0, doubles the interior divider,
and dashes boundary 2. Parameter tokens remain English in the Thai UI.

Connector `laneMarkings` still describes N−1 **interior** dividers, now with the same four
kinds. Its two outer edges stay solid. Empty lists retain the old defaults.

`markingStrokes` is shared by both renderers: `none` produces no stroke, `double` produces
two solid strokes separated by 0.15 m. It never changes surfaces, hit-test boundaries,
Connector mouths, runtime paths or lane widths. Lane-changing legality is not inferred from
paint; changing the existing lane-change safety/routing rules requires its own contract.

Resizing a Link preserves the markings on surviving boundaries, inserts default new
boundaries, and removes the boundaries deleted with lanes. An old solid edge that becomes
interior stays solid until edited. Split/turn-pocket, opposite and duplicate operations keep
the applicable markings. Reversing reverses boundary order. Undo restores exact values.
Connector range-count changes retain their previous reset-to-derived policy; retargeting
that narrows the path count now also clears stale widths/dividers.

## Persistence and validation

The [project codec contract](NETWORK_EDITOR.md#save-recovery-and-formats) owns supported
versions and feature-dependent saves. Markings were introduced in schema 7; that historical
number does not describe today's save version or promise all fields in the supplied examples.
Unsupported network-object fields produce `EDIT_UNSUPPORTED_FIELD` and a field path before
replacing the document. Standalone Link/Connector spec snippets are not complete projects.

Model validation checks Connector width/divider list lengths and values on load as well
as through commands. Widths remain finite and positive, preserving older authored files;
the target Link spec's narrower 0.5–20 m range is not imposed retroactively. Invalid C++ enum
values are rejected, too. Link lane count is limited to 12 consistently with editor commands.

## Advisories

`TIGHT_CONNECTOR_RADIUS` and `WARN_SHORT_CONNECTOR` (reference length strictly below
5 m) appear as **Advisory**, separately from draft errors and runtime blockers. Recheck
diagnostics to display them; clicking the row selects the Connector. They do not block Save
or Run and are not a swept-path or calibrated traffic-engineering assessment.

## Verification

The `authoring` CTest group covers imported invalid values, schema migration/roundtrip,
unsupported field rejection, both driving sides, boundary stroke geometry, resize/split/
copy/opposite, referenced reversal rejection, command no-ops/rollback and advisory compilation.
`authoring-ui` exercises actual inspector actions, canvas pen styles, Undo/Redo, save/reopen,
Thai labels and preservation of the current document when an unsupported project is opened.
Existing replay, mouth geometry and native UI suites remain required. Results and platform
limitations are recorded in [PROGRESS.md](../PROGRESS.md).
