# M1 owner acceptance record

**Status: M1 usability accepted by owner ruling (2026-09-25, D49); M0 plausibility open.**
The written exercise below was not passed as written — the ruling accepts M1 on the owner's
judgment of the attempt. The owner drew a signalised
intersection in 9 min 40 s without assistance and supplied the saved file. The time and the
save/reopen comparison pass; the requested geometry does not — no turn pockets, no aerial image. M1 implementation and
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
| Aerial image / distance reference | **None** — the saved file's `background.pngBase64` is empty |
| Elapsed time | **9 min 40 s, drawing only** (owner report). Signal control, demand and Run were not in the timed portion |
| Documentation or assistance used | No assistance (owner report). Documentation use not stated |
| Four-leg geometry and turn pockets complete | Four legs: yes — two crossing dual carriageways (`link-2`/`link-11`, `link-5`/`link-8`), each Link 2 × 3.5 m lanes end to end, 8 turning Connectors (a short and a long turn per approach), through movements on the Links. **Turn pockets: none** — no Link gains a lane before the stop line. Stop lines (8 heads, 4 groups) all sit upstream of the first crossing carriageway |
| Exact parsed-JSON save/reopen comparison | **Pass** — `parseDocument` → `documentJson` on the supplied file (schema 13, revision 45) gives JSON equal to the file and an equal document, checked 2026-09-25 against `c1e3ce9` |
| Run / recovery / level checks | Run: the file compiles and runs (`trafficsim-cli --project`, seed 42, 180 s, 6 safety clamps). Recovery and levels not observed |
| M0 plausibility observation | Pending |
| Friction, defects and evidence paths | The short turn (`connector-14/16/18/20`) leaves each approach's first lane 3–8 m **upstream** of that lane's head, so it bypasses the signal — **the owner confirmed this is intended** (Thai left turn at all times, 2026-09-25), so it is not a defect. Evidence: owner's `network.traffic.json`, not committed |
| M1 gate verdict | **Accepted by owner ruling (D49, 2026-09-25)**: an engineer who has used another traffic simulator models an intersection of ordinary complexity in well under 10 minutes. As written, this attempt meets the time (< 10 min, no assistance) and the save/reopen comparison, but **not the requested geometry** (turn pockets, drawn over an aerial image). Documentation use and first-attempt status not stated |

Commit the completed record and any authorized evidence references. A failing exercise
creates concrete follow-up work; keep the gate open until its original criteria pass.
