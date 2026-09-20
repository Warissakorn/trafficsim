# Authoring extensions — M1.21

This is the implemented subset of the [supplied specifications](specs/README.md).
[SPEC_AUDIT.md](SPEC_AUDIT.md) records the baseline, conflicts and remaining requirements.
It does not establish Vissim parity or close the M0/M1 owner acceptance gates.

## Link geometry actions

Select a Link, then use its Properties tab. Each successful action is one History command;
Undo restores the full document, including references removed by an ordinary reshape.

| Action | Behavior |
|---|---|
| Insert point at station | Insert on the reference polyline at a distance in metres from its start; endpoints/out-of-range/non-finite inputs are rejected |
| Add midpoint to longest segment | Subdivide the longest reference segment; first segment wins ties |
| Straighten Link | Keep its two endpoints and remove intermediate points; existing Connector reanchor/cleanup rules apply |
| Reverse unreferenced Link | Reverse geometry and lane order and negate lane offset, preserving each named lane's physical footprint; reverse markings as well |

Inserting an existing vertex is a no-op: no dirty flag, revision change or lost redo history.
Reversal is refused if a Connector, signal head or route references the Link/its lanes.
Moving connected traffic to the opposite direction requires the later M1.22 topology contract.
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
paint; that belongs to the future lane-changing model.

Resizing a Link preserves the markings on surviving boundaries, inserts default new
boundaries, and removes the boundaries deleted with lanes. An old solid edge that becomes
interior stays solid until edited. Split/turn-pocket, opposite and duplicate operations keep
the applicable markings. Reversing reverses boundary order. Undo restores exact values.
Connector range-count changes retain their previous reset-to-derived policy; retargeting
that narrows the path count now also clears stale widths/dividers.

## Persistence and validation

Saves write schema 7; bare M0 scenarios and schema 1–6 projects still load. Existing attachment
fraction-to-metre migrations, ranges, levels, offsets and explicit widths are preserved.
Only the supported fields are written; the version number does **not** promise all the
properties shown in the supplied schema-7 snippets.

Schema-7 network, Link, lane, Connector, signal-head, lane-reference and point objects reject
unknown fields with `EDIT_UNSUPPORTED_FIELD` and a field path. Thus a proposed `behavior`,
`detectors`, `curve` or elevation `z` is not silently discarded. Opening fails before the
current document is replaced. Legacy field handling remains unchanged. The standalone
`link`/`connector` snippets in the source documents are not complete project files.

Model validation now checks Connector width/divider list lengths and values on load as well
as through commands. Widths remain finite and positive, preserving older authored files;
the target Link spec's narrower 0.5–20 m range is not imposed retroactively. Invalid C++ enum
values are rejected, too. Link lane count is limited to 12 consistently with editor commands.

## Advisories

`TIGHT_CONNECTOR_RADIUS` and new `WARN_SHORT_CONNECTOR` (reference length strictly below
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
limitations are recorded in [PROGRESS.md](PROGRESS.md).
