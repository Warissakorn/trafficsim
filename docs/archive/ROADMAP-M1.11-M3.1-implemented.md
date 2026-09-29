# ROADMAP archive — M1.11, M1.11.1, M1.12 and M3.1 bodies

Moved out of [`../ROADMAP.md`](../ROADMAP.md) on 2026-09-29; each heading and a status line stay there.

### M1.11 — Body attachments and direct lane resizing

Implemented in the Network Editor: Select/Links Ctrl+right-drag creates a Link from empty
space or a Connector between lane positions. Two-click connection also picks lane bodies.
Source/target positions persist in schema 3. Source, target and middle side handles resize
ranges from one lane; a Link side handle resizes its lane list. Undo/Redo, cancellation,
link edits, duplication and split attachment remapping share the existing command boundary.
The middle handle sets both ranges together; the Connector path count remains their maximum.
First-lane changes use the dialog or Properties. This does not claim independent arbitrary
internal Connector lane topology or close the owner's usability gate.

### M1.11.1 — Compile interior attachments into runtime lane sections

**Implemented.** `runtimeSections` cuts each lane at every station where a Connector attaches to
its body and `buildScenario` compiles the pieces, so an attachment part way along a lane runs in
both directions. **Leaving** is a diverge: the vehicle travels the drawn partial distance, and an
authored route stops at the section carrying the Connector it leaves by. **Arriving** is a merge,
arbitrated by a priority rule derived from the drawing (M3.1): the vehicle joins at the drawn
metre — the arriving path's successor is the section that *starts* at the cut, not the one that
ends there — gives way to traffic already on the lane, and is never charged for the stretch
upstream of where it came in.

Signal heads rebase onto the section they stand on, a head on a cut belonging to the upstream one
as `splitLink` already had it. Both render sites key geometry by section id from the same table
the scenario came from, and the route dialog still offers whole lanes via `authoringSegments`, so
no derived id reaches the project file.

Replay is unchanged and the four frozen baselines still pass, because `sectionId(laneId, 0)` is
`laneId`: a lane with nothing attached compiles to the `Segment` it always did. **One case still
cannot run:** a cut within `kMinSectionLength` (0.2 m) of a lane end or another
cut on the same lane, since the core accepts no zero-length segment. It reports
`UNSUPPORTED_CONNECTOR_POSITION`, reworded — the old string claimed the core runs only
end-to-start connectors, which is no longer true. A derived rule's gap time and headway come from
`data/priority-rules/`; their absence blocks **Run** with an object-linked
`EDIT_NO_PRIORITY_DEFAULTS` and does **not** block an edit, which is D18b's boundary.

**Done:** a vehicle leaves and enters at the drawn stations, travels the correct partial-lane
distances, obeys section-mounted signals, and replays deterministically on both driving sides.
The merge, internal-input and repeated-segment guards stand — `UNSUPPORTED_MERGE` was narrowed by
construction, not removed — and no persisted duplicate runtime network was introduced, because
sectioning changes no authored id and is a pure function of the drawing.

### M1.12 — Fixed lane edges, road markings and selection/copy gestures

Implemented following the owner's 1–3 lane examples: Link handles on both sides,
source/target/middle Connector handles on both sides, fixed surviving lane positions,
and shared edge/divider rendering in the editor and diagnostic view. Schema 4 stores
lane offsets and stable Connector interpolation weights. Ctrl-click adds selection;
Ctrl-drag selected Links, Connectors or Signal heads creates a single undoable copy.
Invalid attached-object drops roll back the entire edit. Tables select heads directly.
Owner Windows interaction and timed acceptance remain open.

### M3.1 — Merge priority by gap time and headway

**Implemented, and it does not close M3.** The engine had no answer at a merge at all, which is
why `validateScenario` refused one outright: a place fed by two segments had no rule for who goes.
`PriorityRule{yieldSegmentId, yieldPosition, conflictSegmentId, conflictPosition, gapTime,
headway}` supplies one — Vissim's priority rule, with the two numbers an engineer tunes.

Car-following across a merge already worked, because `OccupiedSpan::segmentIndex` is a global
index into `Scenario::segments` and spans are bucketed globally, so two vehicles see each other
the moment they share a segment. What a rule adds is seeing the major approach *before* entering
it, which is not on the minor vehicle's own route and so is invisible to `closestVehicle`. A
vehicle that must give way is held at its stop line by the **same clamp a red signal head uses**,
not a second mechanism beside it. Rule-to-route incidence resolves once per scenario in
`ScenarioIndex`, mirroring `routeHeads`.

`UNSUPPORTED_MERGE` is loosened **by construction, never by removal**: a place fed by *n*
segments is runnable only when at least *n*−1 of them give way to another of them, so exactly one
has priority and the rest have somewhere to wait. A network that has not been through the
priority model reports it exactly as before — six tests break if the guard is deleted instead.
A segment may not give way to itself.

A standing queue upstream does **not** block: a stopped major vehicle further off than the
headway is a gap, and blocking on it would deadlock the minor approach instead of releasing it.

Gap time and headway for a **derived** rule live in `data/priority-rules/`, so changing them is a
data edit (hard rule 5). They are read best-effort, because a document carrying its own catalogs
must stay portable to a machine with no data directory; a rule that has to be derived without
them is refused at the point of use rather than given a zero gap time, which would be a merge
nobody gives way at, invented in silence.

**A deterministic threshold test, not a calibrated critical-gap model:** hard rule 4's
not-yet-validated marker stays and M6 still owns fidelity.

**Explicitly NOT in M3.1, and all still M3's:** conflict areas as editable input, priority rules
as an authorable object with their own UI, stop and yield control, crossing conflicts, and signal
heads placed anywhere on a link. M3's done-condition — an unsignalized T-junction whose
minor-road delay responds correctly to changing the gap time — is **not** met by this milestone
and M3 remains open.
