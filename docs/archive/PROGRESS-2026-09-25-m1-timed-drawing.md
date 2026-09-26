# PROGRESS archive — 2026-09-25, the M1 timed drawing

Moved out of `docs/PROGRESS.md` on 2026-09-25 as the oldest live entry. Unchanged.

## 2026-09-25 — M1 timed drawing: 9 min 40 s, recorded, gate still open

Owner report (Thai, 2026-09-25): a signalised intersection built in "10 minutes", then made precise
as **9 min 40 s, drawing only, no assistance**. A Windows `trafficsim-desktop.exe` was attached; its
strings show the M2.7 signal-timing view, so the build is at or after `3c11afd`. It cannot run here
and is not evidence of the network — the saved project is.

Recorded in `M1_ACCEPTANCE.md` row by row: only what the owner stated is filled, the rest says
"not stated". The < 10 min / no-assistance criterion is met by the report; the verdict stays
**Open** because the four-leg-with-turn-pockets geometry, the aerial image, documentation use,
first-attempt status and the exact save/reopen comparison are unobserved (rule 8, D8). It is not
an M2.6 observation: C1 needs the run to the Results table and C2 a time in the current tool.
No code changed.

**Then the file (`network.traffic.json`, schema 13).** Round trip exact. Two crossing dual
carriageways, 2 lanes each end to end, 8 turning Connectors, 4-group fixed-time controller
(cycle 120 s, each approach alone — protected). No turn pockets, no background image, so the
M1 geometry criterion is not met; recorded, not relaxed. It runs: 180 s, flat 1600 veh/h per
approach, routeless inputs split by D42's equal share at each lane exit (so turning volumes are
not counts), 126 completed, 138 never entered (queues fill the ≈50 m approaches), 6 clamps. Each
short-turn Connector leaves 3–8 m upstream of its lane's head and so bypasses the signal — the owner
confirmed it is the intended Thai left turn at all times. Not an M2.6 observation. The owner then
asked for the network to be fixed to pass; declined for the gate — M1 times the engineer drawing,
so a network edited by a session is not an observation (NEXT §2: it cannot be delegated).
