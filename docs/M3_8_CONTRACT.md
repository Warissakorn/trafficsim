# M3.2.8 behaviour contract

M3.2.8 adds driver behaviour on top of the M3.2 right-of-way runtime
([`M3_CONTRACT.md`](M3_CONTRACT.md)). It is two systems, each with its own section here:
**M3.2.8a**, a commitment rule at waiting lines (§1), and **M3.2.8b**, mandatory lane
changing (§2). Cooperation, visibility and discretionary changes are **M3.2.8c**, not yet
written. Nothing here is calibration: the
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

    v² > 2 · maxDeceleration · gap

- `v` is its speed and `gap` the distance from its front to the line, both from the tick's
  pre-step snapshot.
- `maxDeceleration` is its vehicle type's (car 8 m/s², heavy vehicle 6 m/s² in the shipped
  catalog): the vehicle physically cannot stop before the line.
- Equality can stop, so it is not committed.
- **The owner first chose `comfortableDeceleration`.** Measured, it committed drivers up to
  about 2 s ahead of a major vehicle. It clamped the major road at the T-junction and at an M2.6
  merge, which breaks this section's gate. The owner then chose `maxDeceleration`
  (`docs/evidence/m3.2.8a-commitment.md`).

Commitment is read off positions and speeds and is **never stored**. `SimState` does not change,
so a copied state replays exactly (A25). A committed vehicle that a leader slows until it can stop
again is not committed any more, and is held as before.

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
  the owner's ruling. It re-publishes the four-leg and M2.6 numbers that D59 kept fixed (D39):
  - the four-leg report did not move;
  - the M2.6 report moved by at most 0.13 s per movement.

  Both are in `docs/evidence/m3.2.8a-commitment.md`. A derived conflict point is the join
  itself, so a major vehicle occupying it is also the committed driver's car-following leader,
  and occupancy is guarded twice there.
- It does **not** apply to signal heads: amber stays red until M4 (D36).

One predicate (`committed`, `src/core/conflicts.hpp`) serves both runtime paths.

### Consequences to measure, not assume

- A committed driver accepts a shorter effective gap than `gapTime`: up to the time it takes to
  cover its stopping distance, under a second at urban speeds. Minor delay therefore falls. This
  is reported, not tuned.
- The major road must stay at zero clamps. If it does not, the rule goes back to the owner
  rather than being adjusted.
- The existing threshold predicates (`tjunction_controlled.*`) are unchanged: a standing or
  slow minor vehicle is never committed.

## 2. Mandatory lane changing (M3.2.8b)

The owner's ruling (D71): **Vissim's way**. A movement's volume enters on every lane of its
entry Link, and a vehicle on a lane that cannot reach its destination changes lanes before it
has to. Cooperation, visibility, discretionary changes and `laneChangeDistance` are M3.2.8c.

### Why

Before M3.2.8b a movement entered only on the lanes that reach it (`routeLaneChains`), and a
vehicle kept its lane for the whole trip. Lane choice was an insertion artefact, not behaviour:
every left-turner of an M2.6 approach appeared in the kerb lane at the network edge.

### Families, stubs and spans — compiled, never authored

- **A family** is one authored route, or one destination of a routing decision on the entry
  Link. It compiles into **one chain per lane of its entry Link**.
  - A chain that reaches the end of the family is **full**, exactly as before.
  - A chain that cannot is a **stub**. It follows its own lane through the family's objects as
    far as they lead, and stops on the lane where the next object leaves from another lane.
- **A lateral span** joins a stub to another chain of its family on an **adjacent lane of the
  same Link** (by the Link's lane order), over the stretch both chains travel. It carries both
  chains' route distances at each end and maps between them linearly. Stations are matched
  across lanes by `matchedStation`, so the ends are square on a curve; between the ends a curved
  Link is approximated.
- **A dead end** is the stub's last span end: the last metre at which a change is still possible
  (the adjacent chain leaves the lane there, or the lane ends).
- A stub from which no chain of changes reaches a full chain is not emitted. Its lane then gets
  no share of that family, which is the pre-M3.2.8b behaviour.
- The core gets spans and dead ends as data (`Scenario::laneChanges`, `Scenario::routeDeadEnds`).
  It never sees geometry, lanes or Links. Neither is in the project file.

Routeless free walk and routing decisions placed downstream of the entry Link stay lane-fixed
(M3.2.8c). When a full chain branches after its destination, a stub vehicle joins the
lowest-slot chain, which shifts those downstream proportions. This is recorded, not modelled.

### The rule

A vehicle on a stub route tries, every tick and as soon as it can, to change to the adjacent
chain with the fewest changes still to make (`remaining`, derived by the index from the spans).
Ties go to the lower route slot. It changes when all of these hold in the tick's pre-step
snapshot:

1. **It is wholly inside a span.** Its rear is at or past the span start and its front is at or
   short of the span end.
2. **It is not inside a conflict area** on its current route or its target route (between a
   zone's waiting line and its exit), and it holds no Stop service.
3. **Forward safety.** At the mapped position, the nearest vehicle ahead on the target route
   leaves at least the vehicle's own `standstillDistance`. The vehicle's own
   `followingAcceleration` behind that leader is at least `−comfortableDeceleration`.
4. **Rearward safety.** The nearest vehicle behind it on the target route leaves at least that
   vehicle's `standstillDistance`. That vehicle's `followingAcceleration` behind the changer is
   at least `−comfortableDeceleration` of its type.
5. **Nothing overlaps it.** No vehicle on the target route lies alongside the mapped position.

Both safety tests reuse the car-following model; nothing new is calibrated. `comfortableDeceleration`
is the vehicle type's (car 2 m/s²), the conservative choice: §1's measurement showed what
accepting `maxDeceleration` for other drivers costs.

**Same tick.** Candidates are decided in vehicle-id order. A later candidate is also tested
against the new positions of those already accepted this tick. A vehicle that changed away
still counts at its old place, which is conservative.

**Instantaneous.** The change happens at the start of the tick, before car-following. A vehicle
occupies exactly one lane in every snapshot. There is no between-lanes state, so a change takes
no time and blocks nothing behind it on the lane it left. That is a documented limit, not a
claim. Car-following, priority rules, zones and the phase-2 checks then run on the post-change
snapshot, which every vehicle shares.

**Dead end.** A stub vehicle is held at its dead end by the stop-line mechanism, exactly as a red
head holds one, and waits there for a gap. It never arrives on a stub. With no cooperation, a
dense target lane can hold it for a long time; M3.2.8c addresses that.

### Replay

There is no RNG draw and nothing new in `SimState`. A change rewrites the vehicle's
`routeIndex` and `distance`, both already in the state, and emits a `LaneChangeEvent`. A copied
state therefore replays exactly (A25). A scenario with no spans runs exactly the code it ran
before.

### Demand

- **An input's volume is split over every lane of its entry Link.** It is split equally, or by
  `laneShares`, which now carry one weight per lane of that Link. A stored size that no longer
  matches falls back to the equal split, as before.
- **A routing decision on the entry Link** (D43) now spreads each destination's flow over every
  lane in the same way. The typed proportions still hold exactly at entry.
- **Movements** are reported from full chains only. A stub is never its own row.

### Consequences to measure, not assume

- Every published multi-lane movement whose lanes do not all reach it moves: the four-leg and
  M2.6 reports are re-published (`docs/evidence/m3.2.8b-mandatory.md`).
- Left-turn delay is not expected to fall. A through vehicle in the shared kerb lane still blocks
  a Thai left turn; this rule changes where a turning vehicle enters, not who is ahead of it.
- A turning vehicle held at a dead end blocks its own lane. That is real, and it is reported
  (lane changes and vehicles waiting at a dead end).
- The single-lane frozen baselines must stay byte-identical.
