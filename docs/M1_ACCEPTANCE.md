# M1 owner acceptance record

**Status: drawing timed once (2026-09-25), gate still open.** The owner reported a
signalised intersection drawn in 9 min 40 s without assistance; the rows below say which
criteria that observation covers and which are still unobserved. M1 implementation and
automated checks do not close the owner's M0 plausibility gate or M1 usability gate.

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
| Commit / build | Windows `trafficsim-desktop.exe` supplied by the owner (2026-09-25); it contains the M2.7 signal-timing UI (`SignalTimingView`), so it is at or after `3c11afd`. Exact commit not stated |
| Engineer / observer / date | Owner, self-timed, reported 2026-09-25. Observer not stated |
| Platform / screen / language / driving side | Windows (from the build supplied). Screen, language, driving side not stated |
| Prior familiarity / earlier attempts | Not stated — whether this was the first attempt is unknown |
| Aerial image / distance reference | Not stated |
| Elapsed time | **9 min 40 s, drawing only** (owner report). Signal control, demand and Run were not in the timed portion |
| Documentation or assistance used | No assistance (owner report). Documentation use not stated |
| Four-leg geometry and turn pockets complete | A signalised intersection was drawn; four legs and turn pockets not stated, saved file not supplied |
| Exact parsed-JSON save/reopen comparison | Pending — needs the saved `*.traffic.json` |
| Run / recovery / level checks | Pending |
| M0 plausibility observation | Pending |
| Friction, defects and evidence paths | Pending |
| M1 gate verdict | **Open** — time criterion (< 10 min, no assistance) met by the report; geometry, documentation, first-attempt and save/reopen rows still unobserved |

Commit the completed record and any authorized evidence references. A failing exercise
creates concrete follow-up work; keep the gate open until its original criteria pass.
