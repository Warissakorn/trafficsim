# Queue discharge measurement (M3.3.1a)

`DischargeAccumulator` observes consecutive immutable `SimState` snapshots from
creation through the end of the run. `estimateDischarge` also accepts explicit
crossing records independently of the engine. This is an unvalidated diagnostic,
not calibrated saturation flow, PCU capacity, control delay or LOS (D121).
Existing `StopLineAccumulator` output is unchanged.

## Records and windows

Each signal head defines one lane (`segmentId`). Routes use the first occurrence
of that segment, as the existing stop-line diagnostic does. Repeated-segment routes
are outside this first slice's supported topology. Cycle Go is today's green
transition; red or amber ends it. Records contain vehicle ID, vehicle-type ID,
end-of-tick front-crossing time, and membership in the queue at Go. Classes are
not assigned yet: records retain every vehicle type; selection affects estimator
samples only, and no PCU conversion occurs.

At Go, upstream vehicles are sorted by distance to the head, then ID. The initial
queue is the contiguous prefix below `QueueDefinition.beginSpeed`, with each gap
from the head or preceding vehicle rear at most `maxGap`. This uses the project's
queue catalog. It is not an empirical saturation threshold.

Green intervals are half-open `[go,end)`. A crossing stamped at the green's end
is excluded. No interpolation is attempted: physical passage within a tick is
reported at its end, with up to one timestep of timestamp quantization; a gap
between two such timestamps can differ by up to one timestep. Simultaneous
tick crossings can yield zero gaps and are unavailable, not infinite flow.
Signal transition ordering follows the current engine's end-of-tick events.

Only complete greens wholly inside `[max(windowStart,warmup),windowEnd]` are
eligible. An initial observed green is conservatively unavailable, including
one starting at time zero; an unfinished last cycle is unavailable. Repeated
observation of the same tick is ignored; missing/out-of-order ticks or changed
Scenario identity throw. The caller must not replace a snapshot within a tick.
While ranks `1..steadyLast` are still open, a remap that changes the upstream
membership of a vehicle queued at Go makes the affected cycle unavailable (D125).
Shared-prefix routing preserves membership. Proven post-remap longitudinal motion
is reconstructed as specified below; a lateral station jump is never passage.
Source departures start at distance zero and use survivor, prior pending or exact
generation-replay type evidence. Missing/contradictory source, remap or terminal
evidence suppresses raw inference and, while ranks are open, invalidates affected
cycles. Tracked sink arrivals are counted.

## Estimator

Ranks are one-based and declared before observation. For steady ranks `a..b`,
where `a >= 2`, average `t[r]-t[r-1]` over those ranks. The rate is `3600/h`
vehicles/hour for this lane/cycle. For startup ranks `1..k`, where `k < a`,
startup lost time is `t[k]-go-k*h`, retaining its sign. Every crossing through
rank `b` must have been in the initial contiguous queue. Extra crossings remain
in the raw output and do not change the declared rank interval.

Invalid windows/rank declarations throw. Partial/outside-window cycles,
insufficient crossings, non-queued ranks, invalid identities/timestamps,
nonpositive headways and conservative remap/source exclusions return an explicit
reason with absent numeric estimates. Consumers must display unavailable as such.

## CLI, editor and remaining gate

The editor's Results tab has a *Queue discharge* page (D135) fed by the same observer with
these CLI defaults: one row per head and lane with greens, greens with an estimate, the means
over those (headway, rate, startup lost time) and the unavailable reasons with their counts.
It carries the same not-validated statement. A *Safety clamps* page lists each clamp event.

`trafficsim-cli --project FILE.traffic.json --discharge` adds `discharge` JSON.
Defaults are the full run window, zero warmup, steady ranks 3–5 and startup ranks
1–2. All defaults, timestamp convention, timestep, sample count, crossings,
cycle completeness and reasons are emitted. Unavailable numeric fields are null.
CLI controls and vehicle-type selection are defined below. Future class assignment
remains separate. Existing top-level seed/compiler/engine-version output applies.
The CLI emits captured project/catalog hashes as defined below. Retain the
original inputs and exact executable commit/toolchain alongside that JSON.

BA03 has type-selection/window evidence; future vehicle classes are not assigned.
BA05 has focused same-tick source-sink evidence (D125). Native/desktop CI and empirical/owner validation are separate gates.
See [delivery rows](../plans/DRIVING_BEHAVIOUR.md) and
[local evidence](../evidence/discharge-measurement.md).

## Declared controls and vehicle-type selection (M3.3.1b1)

The CLI accepts `--discharge-start S`, `--discharge-end S`,
`--discharge-warmup S`, `--discharge-steady-first N`,
`--discharge-steady-last N`, `--discharge-startup-last N`, and repeatable
`--discharge-type ID`. These require `--discharge --project FILE`.
Seconds must be finite and nonnegative, ranks positive integers; scientific
window/rank restrictions above still apply, and the end cannot exceed run duration.
An unknown type ID rejects before the run. Empty type selection means all types.

Type selection keeps the full raw crossing stream and original ranks. A steady
sample is included only when its follower (the vehicle at rank r) has a selected
vehicle type; its predecessor remains the actual vehicle at rank r-1, of any type.
Skipped samples never connect nonadjacent selected vehicles into a fictitious gap.
Report the sampled ranks and count. No selected samples means unavailable.
`3600/h` is inverse selected-follower mean headway, not the selected type's hourly
throughput or a new approach capacity. These IDs are current vehicle types, not
future behavior/vehicle-class assignments, and no PCU conversion occurs.

Startup uses that selected steady reference only when every vehicle in original
startup ranks 1..k is selected. Otherwise headway can remain available while
startup is null with `mixed_type_startup_prefix`. The JSON emits this separate
startup reason and the selection/rate definitions. Unfiltered defaults retain the
previous numeric results. Captured input hashes and supported shared-prefix recognition are defined below.
Proven lateral/source reconstruction is delivered in M3.3.1b2b1; source-sink type
evidence remains M3.3.1b2b2 and BA05 stays open.

## Captured input provenance (M3.3.1b2a)

With `--discharge`, `inputManifest` records SHA-256, byte count and read count for
`project`, every catalog JSON file actually read by compilation/validation, and
`evaluation/queue-counter.json`. Names are logical relative paths, not host-specific
absolute paths. Entries are sorted by logical path. Project-owned catalogs remain
inside the project hash; unused external catalogs do not appear.

The parser and digest consume the same captured binary bytes, including whitespace,
BOM and line endings. JSON is not normalized or reserialized before hashing.
Repeated reads of a logical file must have identical bytes/digest; a mismatch
rejects before simulation. Best-effort priority-catalog fallbacks are recorded,
including files read before parsing fails. The collector is explicit and optional;
ordinary loading and simulation retain their previous contracts. No filesystem
or hashing code reaches core/eval.

This is the identity of actual file reads, not a directory snapshot or an archived
copy of inputs. Hashes do not recreate missing files, establish calibration, or
capture the executable's source commit. Retain the original files and exact build
commit alongside top-level engine/compiler/seed fields for replay.

## Supported route recognition and remaining passage limits

A positioned `RoutingEvent` can preserve the queue and crossings when source and
target routes have an identical physical segment prefix through this head.
Recognizing a different suffix does not move the front off that shared lane; keep
its original queued-at-Go membership and count a subsequent passage once.
Changing a queued vehicle's upstream membership while ranks are open invalidates
its cycle estimator (rank scope below).
Proven longitudinal motion before/after a remap remains observable separately
from that queue gate. Ambiguous remaps suppress raw passage inference; an arrival
on a diverted route alone never proves passage of the old head. See reconstruction
below.

## Passage reconstruction contract (M3.3.1b2b1)

Reconstruct longitudinal motion from observer-owned prior route/front/type
positions, source departures and current survivors/sink events. Source insertion
starts at route distance zero. Identify type from a survivor, the previous
pending record, or `upcomingArrivals` (below); otherwise it is ambiguous, never
guessed from a route or mixed input. A source at the head is not a front crossing
from upstream.

Start-of-tick lane changes precede motion. Match their route pair and prior body
position to the engine's lane-change spans. Use the unique mapped target station;
no matching span or disagreeing overlapping maps is ambiguous. Do not read the
Run view's display-only LaneChangeTrace or infer passage from a lateral jump.
End-of-tick routing must agree with its decision station and terminal position.
Count movement from the post-remap upstream front through a head to a survivor
or sink once, including a vehicle inserted that tick. Preserve half-open green
boundaries and end-of-tick timestamps; source vehicles are not queued at Go.

Proven longitudinal crossings always remain in the raw stream. Unrelated or
downstream changes do not invalidate other heads. Ambiguous passage suppresses
that vehicle's raw inference. Repeated passage by one ID in a cycle is unavailable
and never duplicated in the stream. See [the local evidence](../evidence/discharge-passage.md).

## Rank-scoped invalidation and source-sink identity (M3.3.1b2b2, D125)

An estimate reads only ranks `1..steadyLast`, and every one of them must be
queued at Go. Invalidation is therefore scoped to what can change those ranks:

- Once a cycle holds `steadyLast` crossings, no later remap or ambiguity can
  change them; the cycle is not invalidated. Raw crossings after that rank can
  still be incomplete and are never an estimator input.
- Before that, a remap that adds or removes upstream membership of a vehicle
  **queued at Go** (other than shared-prefix routing) invalidates the cycle: its
  missing crossing would silently shift the ranks.
- A non-queued vehicle changing lane in or out does not invalidate the cycle.
  If it crosses within the ranks it is recorded as not queued at Go and the
  estimate is `queue_not_sustained`; otherwise it cannot affect the estimate.
- Ambiguous motion stays conservative while ranks are open: an unseen crossing
  could hide a non-queued vehicle inside the ranks.

A vehicle can be generated, inserted and reach the route sink within one tick,
leaving no survivor and no previous queue entry. Core's pure
`upcomingArrivals(const SimState&)` replays the next step's arrival generation on
a scratch copy (same RNG, id counter and inputs; the state is not changed), so the
observer records each new pending vehicle's exact type. A departure uses that type
only when its route slot, scheduled time and desired speed match the record
exactly; a mismatch is contradictory evidence and stays `untracked_source_passage`.
Type is never inferred from a route or composition.
