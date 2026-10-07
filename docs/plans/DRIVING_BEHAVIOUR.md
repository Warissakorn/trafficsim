# Driving behaviour delivery and acceptance

Owner authorized the staged improvement plan on 2026-10-06 (D120). P0
wrote the interface/capability/acceptance design first; subsequent measurement
slices add diagnostics and CLI settings without changing engine behavior or schema. Read [the contract](../reference/DRIVING_BEHAVIOUR.md),
[ROADMAP](../ROADMAP.md#m33--driving-behaviour-library-and-models) and
[NEXT](../NEXT.md) for authoritative scope, gates and live work.

## 1. Delivery slices

| Slice | System and result | Prerequisite / closing evidence |
|---|---|---|
| M3.3.0 (P0) | Assignment contract, capability map and acceptance design | Docs/navigation guards; baseline provenance; no runtime completion inference |
| M3.3.1a (P1) | [Discharge observer/CLI](../reference/DISCHARGE.md) delivered | BA01/BA02/BA04 focused evidence; existing StopLine output unchanged |
| M3.3.1b1 (P1) | Vehicle-type follower selection and declared CLI windows/ranks delivered | BA03 type/window evidence; no behavior classes or PCU inference |
| M3.3.1b2a (P1) | Captured input hashes and shared-prefix recognition delivered | [Provenance/passage evidence](../evidence/discharge-provenance.md); raw diverted sink inference suppressed |
| M3.3.1b2b1 (P1) | Proven lateral/insertion-tick passages reconstructed | [Passage evidence](../evidence/discharge-passage.md) |
| M3.3.1b2b2 (P1) | Rank-scoped remap invalidation and same-tick source-sink type identity delivered | [Identity evidence](../evidence/discharge-identity.md); BA05 focused evidence; native CI per PR |
| M3.3.2a (P2a) | Project library/class/behaviour-type contracts and codec | BA06–BA09; portable schema/legacy behavior before UI |
| M3.3.2b (P2b) | Compile assignment and select prototype per road/type | BA10–BA18; one effective resolver for all consumers |
| M3.3.2c (P2c) | Library/assignment editor | BA19–BA20; EN/TH, Linux/Windows automated checks and independent owner review |
| M3.3.3a/b (P3) | W74 then W99 | Equations/state/traits/model-switch contracts before code; analytic transition/replay and timestep cases; M0 observation remains independent |

Later plan portions belong to their owning systems: signal behavior P4 to M4;
physical-mouth/anticipation P5 to M3.2.4e/M3.2.8a.1; extended lane changes P6 to
M3.2.8c; lateral infrastructure and motorcycle filtering P7 need numbered slices
and body-occupancy contracts before code. Presets P8 ship only after their used
capabilities pass. Batches/export P9 belong to M5; independent calibration P10
belongs to M6. Do not combine these into a single implementation session.

## 2. Measurement definition before implementation

`src/eval/stop_lines.*` currently measures crossing ticks, mean gaps within one
green, waiting and residual queues. This is not yet a steady-discharge estimator
or startup-lost-time result; never relabel the existing meanHeadway as saturation
headway. It sees tick snapshots, not exact interpolated crossing times.

The M3.3.1 contract must define head/lane identity, vehicle class, green cycle,
crossing timestamp policy and measurement/warm-up window. For the first fixture,
accept a declared crossing-record stream independently of simulation. Declare a
steady headway interval by queued vehicle rank before observing results. When
that interval lacks enough crossings or its queue is not sustained, return an
unavailable estimate with a reason; do not return zero or silently include free
arrivals. Several physical lanes must not interleave into one fictitious headway.

For declared steady mean headway h, the reported discharge-rate estimate is
3600/h vehicles/h for that lane/window, not a universal approach capacity.
Define startup lost time as the declared initial queue headways' sum minus their
count times h, with the first headway measured from effective Go to first crossing.
Report sign, rank range, sample count, time quantization and excluded cycles.
Negative startup estimates remain visible for diagnosis, not silently clamped.
Effective Go is green under today's D36; future red-amber behavior needs its own
definition. Confirm cycle-start queued membership with snapshots/explicit flags.

PCU conversion, empirical target ranges, warm-up rules for real studies and
confidence-interval precision need chosen sources/data. No universal 1,700–1,900
or 2,100–2,300 PCU/h gate follows from the owner's suggested parameter sheet.

## 3. Failure-first acceptance rows

BA01/BA02/BA04 have focused Linux evidence in [M3.3.1a](../evidence/discharge-measurement.md).
BA03 has [type/window evidence](../evidence/discharge-controls.md), with future class assignment deferred.
BA05 has [proven-motion](../evidence/discharge-passage.md) and [source-sink identity](../evidence/discharge-identity.md)
evidence; all later rows are pending. P0 writing a row does not pass it. Record exact fixture/input hashes, commit, compiler/flags, commands,
assertions and platform in evidence when the corresponding slice lands.

| Row | Fixture / forcing condition | Required outcome |
|---|---|---|
| BA01 | Queued lane: crossing times 3, 5.5, 7.5, 9.5, 11.5 s after Go; steady ranks 3–5 | h=2 s, discharge estimate=1800 veh/h; startup ranks 1–2 give 1.5 s; metadata identifies windows |
| BA02 | Empty/one-crossing window, interrupted queue, partial first/last green | Estimates unavailable with reasons; no division by zero or biased silent inclusion |
| BA03 | Two lanes, two types; boundary timestamps and warm-up exclusions | Independent lane headways; declared class filters; half-open windows with no duplicate records |
| BA04 | Same seeded run with and without measurement | Identical full trajectory/RNG/accounting; observer does not mutate state |
| BA05 | Copied snapshots, route changes, sinks, timestep variants | No duplicate/lost crossings; documented tick error; crossings at sink handled explicitly |
| BA06 | Duplicate/edit set with references from two roads | New independent ID; editing original affects only its declared users; one Undo restores everything |
| BA07 | Duplicate class membership, bad units/model/IDs, unused invalid set | Reject candidate/load atomically; preserve published document and revision |
| BA08 | Delete referenced set/class/type and explicit reassignment | Simple deletion rejected; validated reassignment is one transaction; Undo restores references |
| BA09 | Save/reopen old and new owned catalogs; unsupported future fields | Portable ownership; old schema/bytes preserved without new features; unsupported fields rejected |
| BA10 | Unassigned road, assigned default, assigned class override | Exact precedence; unclassified vehicle uses assigned default; missing IDs do not silently fallback |
| BA11 | Multi-lane/section-cut Link and derived Connector paths | Road assignments survive all compiler expansion; no route-specific disagreement on a shared segment |
| BA12 | Front just before, exactly at and just after a join; front/rear span two roads | Boundary convention holds; trailing length does not change selected front profile |
| BA13 | One tick crosses multiple/short roads and same-profile boundaries | Document selection lag; no new movement cap; identical-set fixture trajectory unchanged |
| BA14 | D119 source-zero, mid-Link and consecutive decision lines; alternate suffix | Select after actual prefix/route updates; no early behavior from unknown suffix; routing draws unchanged |
| BA15 | Mandatory/discretionary changer and distinct trailing profiles | Follower uses own physical location/set; recompute after accepted remap; preserve hard safety |
| BA16 | Source record blocked by first-step clearance; new profile at entry | Same sampled pending vehicle waits/releases; no resample/false departure; equality case covered |
| BA17 | Served Stop while profile changes; denied conflict and receiving queue | No reset at same served line; physical constraints compose; no suppressed genuine clamp |
| BA18 | Legacy prototype fixtures, immutable copy and same-build replay | Existing exact/tolerance contracts pass; baselines never regenerated to hide failures |
| BA19 | Dialog cancel/no-op/edit, bulk road assignment, rename/delete | Atomic History and revision invalidation; effective/inherited values distinguishable; local changes not leaked |
| BA20 | EN/TH, save/reopen, Linux/Windows UI and owner appearance | Parameter names preserved; automated platform evidence separate from owner's visual verdict |

Future W74/W99 rows must force each transition/limit, mixed-model following,
profile/model boundary state transfer, distribution sampling and physical safety.
Future lane/lateral/conflict rows must check occupancy during the maneuver and
rear clearance, not just final positions or rendered appearance.

## 4. Baseline and integration recipe

Record the clean base commit, source/catalog hashes, compiler/version/flags,
timestep and seed list before experiment code. Compare whole outputs for the
four supplied projects and T-junction stress fixture when changing engine motion;
use at least seeds 42–81 for movement comparisons (D88), retain gridlocked and
unfinished seeds, and record generated/completed/active/pending/clamps.
This seed count is a development rule, not statistical sufficiency for all studies.

Prototype baseline fixtures stay frozen. Validate no-feature default outputs
before enabling assignment. Add analytic estimators/forced runtime cases before
implementation, then perform relevant native/Qt checks. Benchmark any new
neighbor search/table/state cost on the same toolchain; no unmeasured performance
claim. Use documented [BUILDING](../BUILDING.md) commands and report missing tools
honestly. Headless checks do not establish desktop appearance or Windows behavior.

## 5. Scope and validation limits

The target does not authorize vehicle deletion at a diffusion timeout, geometry
distortion to hit throughput, 3D or articulated dynamics. Scalar clearance is
not swept-body evidence. New presets remain experimental until their capability
and validation gates pass. Keep the not-yet-validated marker and disclose that
Vissim-calibrated parameter values need independent recalibration here.

P0 is documentation delivery only; P1–P10 are subsequent systems. This does not
close M0, M2/M3 owner exercises, M6 or the existing owner review queue.
