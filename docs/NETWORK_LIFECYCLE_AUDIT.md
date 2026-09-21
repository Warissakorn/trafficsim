# Network authoring regression audit — 2026-09-21

Scope: the currently implemented native Network editor, following the owner's reports of
wrong-side endpoint moves and spiked perpendicular mouths. This is not completion of the
unimplemented requirements in `SPEC_AUDIT.md`, nor proof that every possible authored shape
is drivable. The original screenshots contain no project coordinates; fixtures reproduce
the reported failure mechanisms, not an asserted exact reconstruction of the source files.

## Confirmed defects and corrections

| Failure | Correction | Regression evidence |
|---|---|---|
| Moving an attachment past its previous interior point retained a backwards final leg | Explicit retarget rebuilds the directed cubic at the existing intermediate-point count; Undo restores all authored points | `lifecycle.retarget_across_the_road…` failed before the fix, passes after |
| Near-perpendicular/reversed arrivals could compress or stretch a mouth into a needle | Below a forward tangent dot product of 0.25, keep the full-width square end and do not apply the ill-conditioned slide | 66 anchored sharp-angle fixtures, both driving sides, 1–3 lanes; failed before the fix |
| Clamped/misaligned mouths were not clearly diagnosed | `WARN_CONNECTOR_ALIGNMENT` is a selectable, bilingual, non-blocking advisory; fit reports `squareFallback` as well as residual metres | Each sharp fixture asserts the advisory, severity and selected object |
| Endpoint preview duplicated outdated retarget logic | Command and preview share `retargetConnector`, including range narrowing and stale width/marking cleanup | Native mouse test compares the preview surface to the committed surface |
| A multi-lane grip could select the wrong range, especially with even counts/unequal widths | Compare physical boundary midpoints, not rounded lane indices | Two driving sides, four successive range placements on 3/7/2/5 m lanes |
| Coalesced final mouse motion lost group moves or rectangle selections | Group threshold and rectangle use release coordinates | Release-only Qt events plus one-step Undo |
| Outward dragging a capacity-limited merge's body tab could shrink both ends | Preserve the range on an impossible outward expansion; do not clamp growth into contraction | Three-to-one merge on a one-lane destination |
| Body resize tabs ignored authored/tapered widths | Anchor tabs on the actual derived boundary; clamp floating-point counts before integer conversion | Existing range/attachment suites plus merge gesture regression |
| Averaging direction across a 180-degree vertex produced a zero vector | Shared directed segment sampling: incoming side for source attachments, outgoing side for targets | Exact cusp tangent and valid Connector construction |
| A nonzero reference curve could offset into a zero-length lane | Reject collapsed/non-finite derived lanes before painting, picking or compiling | Forced offset collapse, import rejection, unchanged history/savepoint |
| Dragging through a collapsed offset could throw before command validation | Draw the invalid transient reference as a dashed outline, then reject release atomically | Exact 3:4:5-miter Qt drag; no exception from painting, document unchanged |
| Double-click could leave another drag state armed | Cancel the complete transient gesture before insertion | Existing vertex workflows and cancellation tests |
| Four intermediate-point tests were never registered with CTest | Register `points` and an unfiltered registry run, preventing future orphaned groups | `points` plus `all-model-tests` |
| The historical angle-sweep fixture was disconnected at its source | Position its reference around the lane attachment and assert model validity first | 288 heading/arrival combinations in `mouths` |

## State coverage

Existing suites were rerun, not replaced with weaker checks. Names below are CTest groups;
the new fixtures are in `network_lifecycle_tests.cpp` and `network_lifecycle_ui_tests.cpp`.

| State / transition | Coverage |
|---|---|
| Empty project → Link; click-draw and Ctrl-right drag; modal accept/cancel | `editor-ui`, `editor-gestures`, `editor-attachments` |
| Connector source pending → hover → target; end/body stations; invalid/coincident/duplicate target | `connector-ui`, `editor-attachments`, `connectors`, `attachments` |
| Endpoint retarget, both driving sides; stale curve; range-centre picking; release without final move | `lifecycle`, `network-lifecycle-ui`, `editor-attachments` |
| Source/target/whole range, leading/trailing edges, 1→3 / 3→1 tapers | `ranges`, `attachments`, `editor-attachments`; new 36-case range-transition matrix |
| Vertex drag, insertion/deletion, collapse, intermediate count 0–40, reset/straighten | `points`, `connectors`, `authoring`, `authoring-ui`, `network-lifecycle-ui` |
| Link geometry/width/count/driving-side change with attached Connectors | `connectors`, `attachments`, `editor-attachments` |
| Link split/pocket, opposite creation, reversal restrictions | `editor`, `attachments`, `authoring`, `m1-workflow` |
| Single/group move and duplicate; overlapping objects; hidden levels | `editor-gestures`, `editor-attachments`, `authoring`, `network-lifecycle-ui` |
| Esc, tool switch, selection change, invalid drop, invalid command | `network-lifecycle-ui`, `editor-attachments`, `connectors`, `editor-ui` |
| Referenced retarget/range restrictions; deletion cleanup of heads/routes/inputs | `connectors`, `ranges`, `demand`, `tables-ui` |
| Successful edit → Undo → Redo; invalid edit preserves document/IDs/savepoint | `editor`, `connectors`, `lifecycle`, UI suites |
| Schema migration, save/reopen, invalid/unsupported fields, recovery | `project`, `authoring`, `m1-workflow`, `editor-attachments` |
| Rendered surface contains sampled driving path; model/render diagnostics agree | `network-lifecycle-ui`, `mouths`, `lifecycle`, `diagnostics` |
| Simulation behavior, fixed-seed replay, reference baselines | `core`, `reference`, `cli`; no baseline regenerated |

## Deliberate behavior and limits

- Retargeting is different from moving a Link. Explicitly assigning another attachment now
  rebuilds the curve; Link movement still follows M1.20's world-position contract and can
  delete a detached Connector with associated references in the same undoable transaction.
- Stored geometry remains a polyline with the requested intermediate-point count, not a
  secretly smoothed spline. A zero-point Connector remains straight. Opening a project does
  not rewrite its authored points or lane topology.
- Ordinary forward mouths still use the existing bounded fitting solver. An impossible
  perpendicular/reversed authored mouth stays square and may overlap/stand off the Link;
  the advisory says to reset the curve or adjust the points/attachments. This is not a claim
  of exact lane-edge alignment for impossible geometry or of traffic-engineering validity.
- Automated coverage is finite. The owner's original project, dense real networks, visual
  inspection on a physical Windows display and the M0/M1 acceptance exercises remain useful
  checks; no automated run closes those acceptance gates.

For repeatable visual QA, run `trafficsim-lifecycle-ui-tests <output.png>` under the desktop
build. It writes a twelve-panel view of generated and steep authored joins on both driving sides using the normal
catalog and canvas. The image is a test artifact, not another persisted network format.
