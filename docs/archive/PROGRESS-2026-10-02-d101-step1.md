# Archived PROGRESS entry

Moved whole from PROGRESS.md on 2026-10-05.

## 2026-10-02 — Why D95's changes reverse (D101 step 1)

Reproduced with `trafficsim-lane-change-sweep` (Release, seeds 42–51, threshold 0.5): lab 4,817
changes, 112 back-and-forth; four-leg-signalised 2,850 changes, 289, all back. A temporary probe in
`decideLaneChanges` (removed, never committed) logged each accepted change's leaders and modes.
**Two causes, both in the incentive, which compares one tick of `followingAcceleration`:**
1. **The car-following regimes have hard edges** (`following.cpp:25–30`). Past `speedThreshold`
   (0.2) and inside the approach horizon (`room < closing²/(2b) + v·T`) the result is
   approaching (≈0 or braking); just outside either it is free (up to `maxAcceleration`).
   One lane flips by >1 m/s² for a 0.05 m/s or 1.5 m difference: lab vehicle 184 changed every
   tick; four-leg vehicle 67 saw a leader 95.3 m ahead as free, 93.8 m ahead as approaching
   (−1.43). A regime edge is crossed in 110/112 lab reversals and 127/138 four-leg ones within 1 s.
2. **Myopia at queues:** a moving leader looks better than a standing one while it is still
   braking into its own queue; 1–3 s later the lane just left looks better. Most four-leg
   reversals of 1.1–3 s are this (only 52/160 cross a regime edge).

**Stateless fixes tried (probes):** E1, the worst of the first three vehicles ahead; E3, one
continuous approach expression for the incentive only. Back-and-forth, lab / four-leg: current
112/289; E1 110/212; E3 47/94; E1+E3 28/54. But the **current incentive at threshold 1.0 gives
28/43 and keeps more of the delay gain** (lab 6.72 s vs 8.17, no-D95 8.83; four-leg 46.81 vs
47.21, no-D95 50.11). No stateless variant reached zero. Windows only; development evidence.
Side finding, not acted on: the same edges shape ordinary car-following (unvalidated prototype).
