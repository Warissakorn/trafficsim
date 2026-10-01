# PROGRESS archive — 2026-09-28: D80 central reference, editor selection cleanup

Moved out of `docs/PROGRESS.md` on 2026-10-01 as the oldest live entries.

## 2026-09-28 — Editor selection and visual cleanup (owner request)

- Unified subtle hover/selection outlines, preserving display-type road colours. Geometry
  grips are small squares only for a single object in Select. Hover clears on leave/focus loss.
- Lane tabs are 24 × 8 logical-pixel rectangles directly on both road edges, without stems or
  counts. Link tabs appear at start/middle/end; Connector end tabs follow the rendered rails.
  Picking follows the rectangular target, with nearest-centre arbitration against geometry grips.
  Connector end tabs now use the painted edge and its tangent, including angled Link joints.
  End tabs inset by half their length and shrink to fit short Connectors; P3–P4 draws as a solid boundary.
  Resizing tabs use preview geometry, keeping Link tabs on curved edges throughout the drag.
- Lane markings use a non-cosmetic 0.10 m pen; dashed markings use 3 m dashes and gaps.
- Empty clicks clear canvas and table-owned selections; mode changes cancel gestures and clear
  selection. Empty clicks preserve multi-click drafts. Escape cancels a gesture first, then
  clears selection when idle. Tab/Delete cannot accidentally edit roads in authoring tools.
- Added interaction regressions, bilingual Select guidance and CI screenshot artifacts.
- Initial Linux/Windows UI tests exposed teardown repainting after History destruction; detaching
  the canvas fixed it. Legacy inspector/Conflict tests now reselect after deselection.
- Local architecture, file-size and diff checks pass. Existing Linux/Windows screenshots were
  inspected; precision UI is added to PR #73. Local CMake/Qt unavailable, new CI pending.

---

## 2026-09-28 — D80: central reference and one P1–P4 pipeline

- Owner requested a whole-carriageway reference, fixed source/target lane pairing, removal of
  all steep-angle square caps, and investigation of the near-180-degree missing mouths.
- Reproduced the source reversal: at 120/150 degrees a 4 m source shoulder was 1.155/0.536 m,
  versus target 3.464/7.464 m. The rail-derived normal inherited the old strip's reversed order.
  A separate four-width reach guard rejected target mouths above 151.044976 degrees; it was
  not caused solely by the 75.522488-degree square fallback. Both legacy paths are removed.
- `geometry` retains schema-17 first-lane semantics for files/runtime. The derived construction
  axis is now the whole range centre, computed BEFORE edges. Widths stack symmetrically around
  it. P1/P4 and dividers pair by lane index with handedness-fixed normals at both ends.
- Mouth displacements share original-axis station weights. This removes the already-mutated
  distance bug and keeps symmetric rails centred. Grips use the axis; inverse bisection makes
  pointer position survive preview, commit, save/reopen and Undo/Redo.
- No reach cutoff or two-mouth reset on a fold. Undefined parallel intersections have no closed
  fill, but open rails remain selectable; a fold retains its computed mouths and is reported.
  Diagnostics now read the surface; the obsolete `connectorMouthFit` API is removed.
- Conflict strips now use the same final rails. Waiting bars intersect those rails at the
  authored path normal. The generated T-junction example updates measured area/line stations;
  frozen core trajectory baselines and runtime lane paths are unchanged.
- Regression tests distinguish centred body width from P1–P4 cuts, check signed lane order,
  both ends through 179.9 degrees, all retained cap points, undefined/fold cases and real
  command round trips. Old tests requiring a square cap are replaced by the owner’s contract.
- Verification: Linux/GCC 13.3 Debug headless build and CTest passed 48/48, including frozen
  core references, architecture/file-size gates and CLI checks (nlohmann/json 3.12).
  Desktop/Windows verification is pending; the local Qt SDK installation was blocked.
- Original M3.2.7 simulation evidence remains tied to its archived project snapshot
  (`docs/evidence/m3.2.7-original-project.traffic.json`), not the regenerated example geometry.
- Prior D79 implementation/history moved intact to
  [archive/PROGRESS-2026-09-28-d79-mouth.md](PROGRESS-2026-09-28-d79-mouth.md).

