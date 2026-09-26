# M3.2.8 behaviour contract

M3.2.8 adds driver behaviour on top of the M3.2 right-of-way runtime
([`M3_CONTRACT.md`](M3_CONTRACT.md)). It is two systems, each with its own section here:
**M3.2.8a**, a commitment rule at waiting lines (§1), and **M3.2.8b**, lane changing,
cooperation and visibility (§2, not yet written). Nothing here is calibration: the
not-yet-validated marker stays, and gap acceptance remains a deterministic threshold until M6
evidence exists.

## 1. Commitment at a waiting line (M3.2.8a)

### Why

The T-junction sweeps (M3.2.7b/c, `docs/evidence/m3.2.7-sweep.md`) clamp 3–10 minor vehicles
per run. Every one is a minor vehicle within 1.5 m of its line when the gap closed: a line closes
the tick a major vehicle enters the gap-time window, and a driver already too close to stop is
halted by the emergency clamp (D66). A real driver in that position does not stop; they go.

### The rule

A vehicle short of a line it must give way at is **committed** to that line when

    v² > 2 · comfortableDeceleration · gap

- `v` is its speed and `gap` the distance from its front to the line, both from the tick's
  pre-step snapshot.
- `comfortableDeceleration` is its vehicle type's (car 2 m/s², heavy vehicle 1.5 m/s² in the
  shipped catalog). The owner chose comfortable over maximum deceleration: a driver who would
  have to brake harder than comfortably proceeds.
- Equality can stop, so it is not committed.

Commitment is read off positions and speeds and is **never stored**. `SimState` does not change,
so a copied state replays exactly (A25). A committed vehicle that a leader slows until it can stop
comfortably again is not committed any more, and is held as before.

### What it overrides, and what it never does

A committed vehicle ignores the **anticipation** part of the gap test at that line:
- a major vehicle within `headway` of the entry (or conflict point);
- a major vehicle arriving sooner than `gapTime`.

It never ignores:
- **Occupancy.** For a zone, a major vehicle whose front is past the entry and whose rear is
  not clear of the exit. For a derived rule, a major vehicle whose front is past the conflict
  point and whose rear is not.
- **An unserved Stop** (contract §5). A Stop line is approached braking anyway, because it holds
  until served.
- **Receiving space**: a standing leader that leaves no room past the chain's exit, and the
  shared room of same-tick requests (phase 2).
- **The phase-2 swept check**: a major vehicle's front reaching the area within the tick.

If one of these halts a committed vehicle it cannot stop for, the safety clamp fires and is
counted, as today. Commitment converts the too-late anticipation stop into going; it does not
hide a physical conflict.

### Where it applies

- **Authored crossing and merge zones** (the M3.2.3 solver, the minor side's waiting line).
- **Derived M3.1 merge rules** (`PriorityRule`, the stop line 1 m short of the join). This is
  the owner's ruling. It re-publishes the four-leg and M2.6 numbers that D59 kept fixed (D39); the
  before and after figures are in `docs/evidence/m3.2.8a-commitment.md`.
- It does **not** apply to signal heads: amber stays red until M4 (D36).

One predicate (`committed`, `src/core/conflicts.hpp`) serves both runtime paths.

### Consequences to measure, not assume

- A committed driver accepts a shorter effective gap than `gapTime`: up to the time it takes to
  cover its comfortable stopping distance. Minor delay therefore falls and the major road may
  wait more for a minor vehicle already across its line. Both are reported, not tuned.
- The major road must stay at zero clamps. If it does not, the rule goes back to the owner
  rather than being adjusted.
- The existing threshold predicates (`tjunction_controlled.*`) are unchanged: a standing or
  slow minor vehicle is never committed.

## 2. Lane changing, cooperation and visibility (M3.2.8b)

Not yet written. It is written before M3.2.8b's code, as §1 was for M3.2.8a.
