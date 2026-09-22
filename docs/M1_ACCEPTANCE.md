# M1 owner acceptance record

**Status: not performed.** M1 implementation and automated checks do not close the
owner's M0 plausibility gate or M1 usability gate. Record a real observation before
changing ROADMAP to accepted.

## Timed M1 exercise

This section is for the person administering the exercise. Give the engineer the task,
not a walkthrough of the controls. They must not consult documentation during the timer.

Prepare a desktop build from the PR commit, a local aerial image the engineer is
authorized to use, a visible distance reference and an empty save directory. Start with
a blank editor and record platform, screen size, language, driving side and prior
familiarity. Do not use a prebuilt network.

Ask the engineer to draw a four-leg intersection with turn pockets over the image and
save the project. Start timing when they begin. Stop when their drawing and first save
are complete. Observe which controls they search for, errors, cancellations and whether
they need assistance.

Pass the drawing portion only if it takes **less than 10 minutes**, uses no documentation
or assistance, and contains the requested geometry and connections. After stopping the
timer, reopen the saved file and compare the network, lane widths/counts, connector
ranges/curves, image calibration/transform, driving side, IDs and any demand/control or
appearance metadata. Save an unchanged reopened copy and compare parsed JSON with the
first save; whitespace is irrelevant, authored values must be identical.

If the engineer misses the time or needs help, record the failure and the specific
friction. Do not retroactively change the criterion or count a rehearsed retry as the
first observation. Preserve each attempt.

## Follow-on checks

These are separate from the ten-minute drawing task.

- Author a supported route and vehicle input; Run in the editor, Pause, Step and Reset
  with the same seed. Observe that the revision is displayed and editing clears the run.
  Do not expect unsupported merges or crossing-conflict behaviour from the M0 core.
- **The M0 plausibility observation, since M1.24.** Open `data/scenarios/crossing.json` in the
  editor and Run it. Judge whether vehicles accelerate, queue at red and discharge at green
  the way a traffic engineer expects. The run status shows the completed-trip mean delay and
  the safety-clamp count; `trafficsim-cli 42` prints the same run's figures for comparison.
  This is the observation ROADMAP M0 asks for; it used to be made in a separate window.
- Save a copy, make an unsaved edit, allow a recovery checkpoint, then terminate only
  that test instance. Reopen and recover it. Confirm it is untitled/dirty, Save asks for
  a destination, and the original saved project is intact.
- Create overlapping ground/bridge links, apply a named style, select at different
  zooms, cycle with Tab and filter levels. Reopen and check the values.
- Switch English/Thai and check dialogs, sidebar, tables and error messages.
- Record M0 signal plausibility separately: acceleration, queue at red and discharge
  at green. This is owner observation, not calibration or M6 validation.

## Result to fill in

| Field | Observation |
|---|---|
| Commit / build | Pending |
| Engineer / observer / date | Pending |
| Platform / screen / language / driving side | Pending |
| Prior familiarity / earlier attempts | Pending |
| Aerial image / distance reference | Pending |
| Elapsed time | Pending |
| Documentation or assistance used | Pending |
| Four-leg geometry and turn pockets complete | Pending |
| Exact parsed-JSON save/reopen comparison | Pending |
| Run / recovery / level checks | Pending |
| M0 plausibility observation | Pending |
| Friction, defects and evidence paths | Pending |
| M1 gate verdict | **Open** |

Commit the completed record and any authorized evidence references. A failing exercise
creates concrete follow-up work; keep the gate open until its original criteria pass.
