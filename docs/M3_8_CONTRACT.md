# M3.2.8 behaviour contract

M3.2.8 adds driver behaviour on top of the M3.2 right-of-way runtime
([`M3_CONTRACT.md`](M3_CONTRACT.md)). It is two systems, each with its own section here:
**M3.2.8a**, a commitment rule at waiting lines (§1), and **M3.2.8b**, mandatory lane
changing, with the one cooperation rule it could not run without (§2). Visibility,
discretionary changes (contract D95 below, not implemented) and the rest of cooperation are **M3.2.8c**. Nothing here is calibration: the
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
has to. One cooperation rule came with it, by a second owner ruling in the same session, because
the measurement without it was not usable (§2, "Cooperation"). Visibility, discretionary changes
(D95, below), `laneChangeDistance` and any further cooperation are M3.2.8c.

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
- A stub is kept only when a run of neighbouring lanes, each with a chain, leads from it to a full
  chain on the entry Link. Every chain covers that Link from its start, so each step of the run
  is a change a vehicle can make there. Any other stub is not emitted, and its lane gets no share
  of that family, which is the pre-M3.2.8b behaviour. A stub that still ends up with no span is
  refused by core validation (`LANE_CHANGE_DEAD_END_UNREACHABLE`) rather than losing its volume.
- The core gets spans and dead ends as data (`ScenarioDefinition::laneChanges`,
  `::routeDeadEnds`, filled by the compile step next to the routes they join). It never sees
  geometry, lanes or Links. Neither is in the project file.

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
3. **Forward safety.** At the mapped position, the vehicle's own `followingAcceleration` behind
   the nearest vehicle ahead on the target route is at least `−comfortableDeceleration`. Its move
   this tick at that acceleration also fits inside the gap less its `standstillDistance`.
4. **Rearward safety.** The same two tests for the nearest vehicle behind it on the target route,
   with the changer as its leader, in that vehicle's own type and behaviour.
5. **Nothing overlaps it.** No vehicle on the target route lies alongside the mapped position.

Both safety tests reuse the car-following model; nothing new is calibrated. `comfortableDeceleration`
is the vehicle type's (car 2 m/s²), the conservative choice: §1's measurement showed what
accepting `maxDeceleration` for other drivers costs. The move-fits test was added after the
first measurement: without it, a change 2 m behind a faster leader passed the acceleration test
and was clamped the next tick, because the move is capped at the gap less the standstill
distance whatever the model asks.

**Same tick.** Candidates are decided in vehicle-id order. A later candidate is also tested
against the new positions of those already accepted this tick. A vehicle that changed away
still counts at its old place, which is conservative.

**Instantaneous.** The change happens at the start of the tick, before car-following. A vehicle
occupies exactly one lane in every snapshot. There is no between-lanes state, so a change takes
no time and blocks nothing behind it on the lane it left. That is a documented limit, not a
claim. Car-following, priority rules, zones and the phase-2 checks then run on the post-change
snapshot, which every vehicle shares.

**Dead end.** A stub vehicle is held at its dead end by the stop-line mechanism, exactly as a red
head holds one, and waits there for a gap. It never arrives on a stub.

### Cooperation: holding back for a waiting vehicle

The rule above, measured alone, left M2.6 vehicles waiting at a dead end, one for 404 s,
blocking the kerb lane. Mean delay rose from 50.1 to 61.6 s and 4 vehicles were still pending
at the end: a queue or a dense stream rarely leaves a gap both safety tests accept. The owner then ruled that the
smallest cooperation rule comes with this slice.

- **Who is waiting.** A stub vehicle at walking pace (below 0.5 m/s) that car-following has
  brought as close to its dead end as it goes (`stopLineReach`, the Stop-line reach of §5). Its
  target is the one rule 1 would pick.
- **Who holds back.** Among the vehicles behind its target place on the target route, nearest
  first, the first that can stop short of it: `v² ≤ 2 · comfortableDeceleration · room` and
  `v · dt ≤ room`, where room is the gap less its `standstillDistance`. Nearer vehicles cannot
  stop comfortably, and they pass first. A vehicle alongside is not asked.
- **How.** The holding vehicle treats the target place as a standing obstacle: its move is capped
  at the gap less its standstill distance, and its acceleration is the lower of its ordinary one
  and the one behind that obstacle. This is a second obstacle, not a replacement leader. A nearer
  moving leader would otherwise hide it.
- **Stateless.** The stopping test is kinematic, not the model's commanded braking. Once a vehicle
  holds back, its stopping distance only shrinks, so the same vehicle is chosen tick after tick.
  Nothing is stored; replay is unaffected.
- The waiting vehicle still changes only when rules 1–5 hold. Cooperation opens the gap; it never
  waives a safety test.

This is not Vissim's cooperative lane change: it has no deceleration parameter and no look-ahead,
and a vehicle not yet stopped is not helped. Those are M3.2.8c, below.

### Cooperative braking: helping a vehicle before it stops (M3.2.8c)

M3.2.8c step 5 showed that the time M3.2.8b costs is the left-turn dead-end waits
(`docs/evidence/m3.2.8c-east-approach.md`, D89). The owner chose the look-ahead and the
parameter (2026-10-01).

- **The parameter.** `maxDecelerationCooperativeBraking` (m/s², positive) on the driver
  behaviour. It is named after Vissim's "Maximum deceleration for cooperative braking" and is
  data (`data/driver-behaviour/default.json`: 3). **A behaviour without it does not brake
  cooperatively**, and a scenario whose behaviours all lack it runs exactly the D71 rule above.
- **Who needs help.** A stub vehicle that is not waiting (the rule above serves that one), wholly
  inside a span with a target (rule 1's), and inside its **look-ahead**. It is inside its
  look-ahead once its dead end, taken as a standing obstacle, already governs its car-following:
  `followingAcceleration` behind `Leader{deadEnd − front, 0}` is not in free mode. So help
  starts when the dead end starts to make it brake, in its own type and behaviour. The model's
  approach test already includes `v² / (2 · comfortableDeceleration)` plus its following time
  and safety gap; at 10 m/s with the default behaviour that is about 50 m. No length parameter
  is added. The first draft used `v² / (2 · comfortableDeceleration) + stopLineReach` alone.
  The model brakes earlier and more gently than that, so the vehicle only reached that window
  as it stopped, and no moving changer was ever helped (A36 failed).
- **Who helps.** Among the vehicles behind its target place on the target route, nearest first,
  the first whose behaviour has the parameter `c` and that can fall in behind it. With
  `room = gap − standstillDistance`, that means `room ≥ 0`, and, when it is faster than the
  changer, `(v − v_changer)² ≤ 2 · c · room`. Nearer vehicles that cannot pass first. A vehicle
  alongside is not asked.
- **How.** The helper treats the changer's target place as a second leader moving at the
  changer's speed. Its acceleration behind that leader is bounded below by `−c`, and it takes the
  lower of that and its ordinary acceleration. Its move is not capped, since the obstacle is not a
  vehicle on its lane. When one vehicle is asked by several changers, the nearest place counts.
  The waiting rule above keeps its standing obstacle and its cap.
- **Stateless and ordered** as the rule above: read off the post-change snapshot, nothing
  stored, and replay is unaffected. The changer still changes only when rules 1–5 hold.
  Cooperative braking opens the gap; it never waives a safety test, and the follower's rule 4 test
  stays at `comfortableDeceleration`.
- Vissim also has a cooperative lane change, in which a vehicle moves out of the way. That is not
  modelled.

### Downstream routing decisions (M3.2.8c, D93 — implemented 2026-10-01, Windows only)

The owner's ruling (2026-10-01): a routing decision placed on a Link **D** downstream of the
entry Link works like an entry decision. This is Vissim's way, as in D71. Before D93 such a
decision was lane-fixed: a vehicle drew only among the destinations its lane reaches, so the
proportions shifted towards what the lanes allow. It is `Walk::decideDownstream` in
`routeless.cpp` now; rule 4 is `unkeptStubs`, and rule 5 is `FamilyRoute::after`. Free walk with no
decision stays lane-fixed. A vehicle with no destination has no mandatory change, and anything
else is discretionary (D95, below).

1. **Where it acts.** The decision acts when a vehicle comes onto D, as in M2.1: its station along
   D is not modelled.
2. **The draw.** A lane of D **serves** a destination when it has a full chain to it, or a kept
   stub (rule 4). A vehicle arriving on lane k draws among the destinations lane k serves, by
   their relative flows. When every lane that receives vehicles serves every destination, the
   typed proportions hold exactly, whatever the arrival lanes. Otherwise they hold exactly for
   the lanes that serve all of them and shift, as today, on the others.
3. **The family.** One destination of one downstream decision, within one input's walk, is a
   family. Its routes are each walked prefix up to D, followed by the destination's chain from
   the arrival lane on D (`routeLaneFamily`, full or stub). Every route of a family starts on the
   input's entry Link. Movement reporting, which groups by (first Link, last Link), is therefore
   unchanged: a vehicle that changes lanes still arrives under its own origin.
4. **A kept stub.** A stub on lane k of D is kept only when a run of neighbouring lanes of D, each
   with a route of the same family, leads from it to a **full** route of the family, as in §2.
   - A target route's prefix may differ from the stub's, since a change maps route distances on
     D. No route is synthesised for a lane that no walk from the entry reaches: a target needs
     the same entry Link, or the movement would be misreported.
   - When no stub is kept, lane k keeps today's lane-fixed draw for that destination, and the
     decision gets the advisory `ROUTING_DECISION_LANE_FIXED`.
5. **Where it may change.** Only at or after its arrival on D. The vehicle does not know its
   destination before the decision. `appendLaneChanges` would also find spans on Links the
   prefixes share (two prefixes on adjacent lanes of the entry Link, say), so spans that end
   before the stub route reaches D are dropped, and one that starts before D is clipped to it.
   The dead end is the last span end that remains, as in §2.
6. **Behaviour unchanged.** Rules 1–5, the same-tick order, the instantaneous change, the
   Stop/zone exclusions, waiting cooperation and cooperative braking are exactly the ones above.
   There is no new parameter. Core gets ordinary spans and dead ends and sees nothing new.
7. **Limits, recorded and not modelled.**
   - Because the decision station is not modelled, a change may begin at D's upstream end even
     when the decision is drawn further along.
   - Stubs add paths, and the 256-path limit (`ROUTELESS_TOO_MANY_PATHS`) still applies.
   - An entry decision followed by a downstream one: a vehicle that changes on an entry stub
     joins the lowest-slot full route of the entry family (§2's rule), and that route already
     carries one downstream destination. That vehicle therefore skips the downstream draw, and
     those proportions shift. This is §2's recorded limit, now reached through a decision.
   - A downstream stub vehicle that never finds a gap waits at its dead end and blocks its lane,
     as an entry stub does.

### Discretionary lane changes (M3.2.8c, D95; item 7 amended by D101 — hold time being implemented)

The owner's rulings (2026-10-02): a vehicle changes lanes **by choice** when the adjacent lane
lets it accelerate harder by at least a threshold. Lanes are chosen freely, as in Vissim's
"Free lane selection". The trailing vehicle is protected more strictly than for a mandatory
change. The default behaviour has it on, and the published reports move. Before D95, a vehicle on
a full route never changes lanes: spans are emitted from stubs only (`appendLaneChanges`), and
`decideLaneChanges` skips a route with no changes remaining.

1. **Where a vehicle may change (compiled, never authored).** A **discretionary span** joins two
   **full** routes on adjacent lanes of one Link, over the stretch both travel, by the same
   mapping as §2 (`matchedStation`).
   - Both routes belong to the same families (equal sets of family names, so any downstream
     destination is the same too) and end on the same Link.
   - Spans run in both directions. D93's rule 5 clip applies to downstream families.
   - The target is never a stub. A stub vehicle changes only by §2's rule, never by choice.
   - A discretionary change therefore never alters a destination or a movement row, and every
     compiled proportion (A40) holds.
   - Free walk with no decision has no family, so it makes no discretionary changes.
2. **Incentive.** `a_here` is the vehicle's `followingAcceleration` behind the nearest vehicle
   ahead on its own route. `a_there` is the same behind the nearest vehicle ahead of the mapped
   position on the target route. Both use the vehicle's own speed, type and behaviour.
   - It wants to change when `a_there − a_here ≥ discretionaryLaneChangeThreshold` (m/s², a
     behaviour field). The name is ours: Vissim has no such parameter.
   - Only vehicles are compared, not signals, lines or dead ends. Both routes share the
     destination, and the target is never a stub.
   - This one test covers overtaking a slower vehicle and picking the shorter queue at a red.
   - With a candidate on each side, the larger gain wins, and a tie goes to the lower route slot.
     There is no side preference and no pull back to the kerb lane.
3. **Safety.** Rules 1–5 of §2 hold unchanged, plus one stricter test. The trailing vehicle's
   acceleration behind the changer must be at least `−acceptedDecelerationTrailingVehicle`, a
   field on the **changer's** behaviour.
   - The field is named after Vissim's "Accepted deceleration" for the trailing vehicle, but its
     value is ours and unmeasured.
   - The forward test keeps `comfortableDeceleration`: the incentive already requires a gain.
4. **Order within a tick.** Mandatory candidates are decided first, in vehicle-id order. Then
   discretionary candidates, in vehicle-id order, each tested against the moves already accepted
   this tick. The change is instantaneous, as in §2.
5. **No cooperation.** Nobody holds back or brakes for a discretionary changer: D71 and D90 serve
   stubs only. Nothing waives a safety test.
6. **Enablement and data.**
   - A behaviour without `discretionaryLaneChangeThreshold` makes no discretionary changes.
   - The threshold field needs `acceptedDecelerationTrailingVehicle`, and validation refuses one
     without the other.
   - `data/driver-behaviour/default.json` gets both. The proposed values are 0.5 m/s² and 1 m/s².
     The implementation session fixes them by measurement, trying the threshold at 0.25, 0.5 and
     1.0 (A55).
7. **One record per vehicle, and a hold time (D101, owner's ruling 2026-10-02).** The stateless
   rule failed A53: the incentive compares one tick of a car-following model with hard regime
   edges, and it is blind to a leader braking into its queue (PROGRESS "Why D95's changes
   reverse"). No stateless variant reached zero.
   - Every vehicle carries `lastLaneChange`: the tick of its last change of either kind and the
     route it left. It lives in `Vehicle`, so it is copied with `SimState` and a copied state
     still replays exactly. It is written by every change and read by nothing else in the core.
   - `discretionaryLaneChangeHoldTime` (s, a behaviour field) is the hold: while fewer than that
     many seconds of ticks have passed since the vehicle's last change, it makes **no
     discretionary** change. A mandatory change is never held: a stub must still leave its dead
     end.
   - Without the field there is no hold, and the decision is exactly D95's.
   - The name is ours; Vissim has no such parameter. Its value is unmeasured and chosen by A53.
8. **Limits, recorded and not modelled.**
   - No between-lanes state.
   - No change into a stub, which Vissim allows.
   - Signals are not compared.
   - No cooperative lane change.
   - Nothing is calibrated.

### Replay

There is no RNG draw. A change rewrites the vehicle's `routeIndex` and `distance` and, since
D101, its `lastLaneChange`, all inside `SimState`, and emits a `LaneChangeEvent`. A copied state
therefore replays exactly (A25, A52). A scenario with no spans runs exactly the code it ran before.

### Demand

- **An input's volume is split over every lane of its entry Link.** It is split equally, or by
  `laneShares`, which now carry one weight per lane of that Link. A stored size that no longer
  matches falls back to the equal split, as before.
- **A routing decision on the entry Link** (D43) now spreads each destination's flow equally over
  every lane of the Link, full or stub. The typed proportions still hold exactly at entry; the
  input's lane weights stay unused there, as before.
- **Movements** are reported from full chains only. A stub is never its own row.

### Consequences to measure, not assume

- Every published multi-lane movement whose lanes do not all reach it moves: the four-leg and
  M2.6 reports are re-published (`docs/evidence/m3.2.8b-mandatory.md`).
- Left-turn delay was not expected to fall, since a through vehicle in the shared kerb lane still
  blocks a Thai left turn. **Measured, it fell on all four M2.6 approaches** (by 1.7–11.1 s), with
  the lane changes and cooperation together. That is reported, not explained: no controlled case
  isolates why. **That was seed 42 only.** Over seeds 42–46 left turns fell on East and North
  only, the right turns rose on South and North in every seed, and mean delay rose in every seed
  (`docs/evidence/m3.2.8c-right-turns.md`).
- A turning vehicle held at a dead end blocks its own lane. That is real, and it is reported.
- The single-lane frozen baselines must stay byte-identical. They do.
