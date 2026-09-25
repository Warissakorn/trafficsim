# M3 delivery plan

Prepared 2026-09-24 against `aebfee54678481f289125c97bb32af46413045aa`, following the
owner's request to carry out the agreed sequence. **Preparation only; no M3 runtime work
has started.** `M2_GATE.md` still records the M2.6 study as not performed. The instruction
to follow the sequence does not record a gate result or waive the prerequisite in ROADMAP.

The live next action remains in [NEXT.md](NEXT.md). [ROADMAP.md](ROADMAP.md) owns milestone
status; this document specifies dependencies, contracts and deliverables. No M0/M1/M2
acceptance or M6 validation is closed by this plan.

## 1. Baseline audit

These are source findings, not measurements from a new simulation run.

| Surface | Existing implementation | Work M3 actually needs |
|---|---|---|
| Merge arbitration | `core/types.hpp`, `core/simulation.cpp`: stop-line clamp using `gapTime` (s) and `headway` (m) | Preserve M3.1; make precedence authorable and handle crossing occupancy |
| Derived rules | `model/network/sections.cpp`: ordered body/start arrivals; `compile.cpp` appends rules | Explicit rules must replace the applicable fallback rather than add an opposite rule |
| Merge validation | `core/validate.cpp` counts yielding predecessors | A count does not prove an acyclic order or one winner; validate the actual graph before authoring arbitrary rules |
| Priority persistence | `ScenarioDefinition` has runtime `priorityRules`; `project/parse.cpp` and `definition.cpp` do not read/write them | Introduce a real authored contract and codec; a C++ member is not a saved project feature |
| Crossings | Distinct segment paths do not interact at geometric intersections | Finite conflict intervals, full vehicle clearance and simultaneous-arrival arbitration |
| Signal positions | `NetworkSignalHead`, `rebaseHead`, existing commands and dialog already support interior positions | Exercise placement, section boundaries and geometry lifecycle; do not rebuild the runtime feature |
| Queues | `eval/movement.hpp` counters name signal heads; `project/evaluation.cpp` derives counters from them | Real counters at unsignalised control lines, with no dummy signal heads |
| Reporting | M2.5 completed-trip movement delay, one run; active/pending/clamps reported | Reuse it honestly; HCM control delay, LOS and report-grade multi-run aggregation stay M5 |

The merge validator accepts the *count* in both `A -> B, B -> A` and an order containing
unresolved ties. These are counterexamples to its sufficiency, not claims that generated
M3.1 rules contain cycles. Generated drawing-order rules are already ordered. The new
validator must keep their accepted cases while refusing contradictory authored control.

## 2. Preconditions and boundaries

Before implementation, the owner completes the unchanged M2.6 study and records evidence
in [M2_GATE.md](M2_GATE.md): the owner judges C1's study a pass, with C4 recorded (D51). Fix and repeat a failed gate
under ROADMAP's rules; never infer success from an automated test, this plan or a merge.

After that, run the existing desktop build/tests and `check` on the implementation checkout.
Use headless only where Qt is unavailable, and state the coverage limit. Preserve all four
frozen references and seed-42 output when the new controls are absent. Do not regenerate
references to hide a regression.

The first supported geometry is an at-grade, fixed-lane crossing or merge with explicit
control. Lane changes cannot occur inside it until M3.2.8 supplies that interaction contract.
Do not infer elevation safety from `level`, which is currently presentation data. Automatic
discovery/coverage of every geometric conflict is a separate capability decision: the first
slice protects explicitly authored areas and must say that, including in the Run UI.

No model parameter is advertised as a calibrated Vissim parameter. Amber remains red under
D36; actuated controllers and dilemma-zone decisions belong to M4. No blocked vehicle is
removed on a timer. Retain the permanent not-yet-validated marker.

## 3. Ordered slices

These are sub-slices of the existing M3.2 scope, not a declaration that its remaining
lane-changing, cooperation, visibility or calibration work has disappeared.

| Slice | Prerequisite | Deliverable | Completion evidence |
|---|---|---|---|
| M3.2.1 Contract and acceptance design | Owner's planning instruction | `M3_CONTRACT.md`, this sequence and `M3_ACCEPTANCE.md` | Source audit and reviewable contracts; prepared, implementation still gated |
| M3.2.2 Authored model and compiler | M2 gate passes; M3.2.1 | References, validation, codec/migration, command transactions, explicit/fallback resolution | Roundtrips, reference lifecycle, cycles/ties rejected; unsupported runtime capability blocks Run |
| M3.2.3 Crossing runtime | M3.2.2 | Conflict incidence, swept occupancy, deterministic grants, rear clearance, exit-space checks | Controlled crossing/merge tests and unchanged no-new-control fixtures |
| M3.2.4 Conflict/priority editor | M3.2.3 | Canvas selection, tables/inspector, gap/headway editing, Problems links, English/Thai text | Mouse/keyboard workflows, cancellation, Undo/Redo, save/reopen, observable runtime effect |
| M3.2.5 Stop and Yield | M3.2.4 | Persistent controls, full-stop service state, gap and signal composition | Each queued vehicle stops once at the line; yield can pass without a mandatory stop |
| M3.2.6 Signal workflow and unsignalised queues | M3.2.5 | Signal-position regression/placement workflow, authored counters independent of heads | Interior/cut positions, standalone queue fixtures, identical CLI/editor reports |
| M3.2.7 T-junction evidence | M3.2.6 | Command-built fixture, gap sweep and owner exercise | `M3_ACCEPTANCE.md` evidence; only the right-of-way done-condition is assessed |
| M3.2.8 Lane changes, cooperation and visibility | M3.2.7; separate interface/behavior design | Remaining M3.2 behavior and conflict integration; calibration evidence with M6 | Controlled changes, safe lead/follower gaps, conflict interaction, congestion accounting and replay |

M3.2.7 can satisfy the T-junction exercise without completing M3.2.8. M3/M3.2 must remain
open while their booked scope or gates remain open. Scientific validity remains M6's gate,
including the original M3.2 requirement for calibrated gap acceptance.

### M3.2.2 — implement the file/model seam first

1. Define the authored types and pure resolution functions in `src/model/network/` using
   [M3_CONTRACT.md](M3_CONTRACT.md). Add structural and runtime diagnostics separately.
2. Add the next project schema (11 if 10 is still current). Preserve schema 1-10 migration
   and the distinct M0 scenario format. Unknown future fields must fail rather than vanish.
3. Add put/delete commands through `History`, then integrate split, copy, retarget, lane
   resize and delete cleanup. A stale geometric reference is a visible Run blocker, not an
   instruction to select a different lane silently.
4. Resolve effective priority once for both compiler and diagnostics. New conflict controls
   remain Run-blocked until M3.2.3; do not expose an editor tool that pretends they run.
5. Add model/codec/command tests at this seam. No simulator changes merely to accept an
   unsupported new object.

### M3.2.3 — implement one admission mechanism

Use the existing pre-step snapshot and route index. Compute candidate movement, then
admit or cap it using explicit conflict reservations before publishing any next state.
Do not sequentially mutate one vehicle and let the next observe a different world.

Index the full major approach route up to its conflict entry, not just the segment
containing the marker. A section cut must not hide a major vehicle. Build occupied intervals
from front/rear positions and test swept travel over a tick; two vehicles can cross between
snapshots even when neither ends the tick inside the area.

Keep longitudinal leader and signal constraints in force. Resolve connected conflict groups
atomically where a vehicle cannot wait safely between them; unsupported groups fail Run.
Benchmark only if profiling identifies a cost. No performance target is invented here.

### M3.2.4-6 — make authored behavior usable and measurable

- Follow the existing Network Objects palette, inspector, Problems panel and History paths.
  Runtime support lands before a new runnable tool. Pointer and keyboard actions submit
  the same commands; gestures follow `VISSIM_PARITY.md`'s current sections and M1.22.
- Show conflict extents, direction of priority, waiting lines and the rule's units together.
  Changing a rule invalidates the run snapshot, as every successful edit already does.
- Offer Stop/Yield as controls over explicit conflict references. A green head is permission
  from that head only; it is never permission to overlap another vehicle.
- Keep existing signal position semantics. Test a head before, exactly on and after a section
  cut and after Link edits, including all-lane selection and both driving sides.
- A queue counter can reference a control line or an explicitly positioned measurement line.
  Its location has one source of truth. Existing signal-derived counters keep their results.
  Queue thresholds stay in the evaluation catalog. Completed-trip delay keeps its M2.5 name
  and entry-acceleration bias until M5 supplies travel-time sections.

## 4. Change map

New names below are intended seams, not files already implemented.

| Layer | Existing integration points | New responsibility |
|---|---|---|
| Model | `network.hpp`, `sections.cpp`, `compile.cpp`, `validate.cpp`, `diagnostics.cpp` | Authored controls, geometric references, effective priority and runtime spans |
| Core | `types.hpp`, `routes.hpp/.cpp`, `simulation.cpp`, `validate.cpp` | Numeric runtime contracts, incidence, grants, control state and validation |
| Project | `document.cpp`, `definition.cpp`, `parse.cpp`, `run.cpp` | Schema, lossless persistence, capability checks and portable explicit parameters |
| Commands | `network_commands`, `connector_commands`, `split_link`, `delete_objects`, `history` | Atomic control edits and reference lifecycle |
| UI | `editor_control.cpp`, `editor_palette.cpp`, `editor_tables.cpp`, `canvas*`, `data/locales/` | Authoring, feedback and visible capability limits |
| Evaluation | `movement.hpp/.cpp`, `project/evaluation.cpp`, `editor_results.cpp` | Independent counter locations; shared CLI/editor measurement |
| Evidence | `tests/`, `tools/four_leg_network.hpp` pattern, `data/projects/` | New T-junction builder and assertions; no changes to frozen reference files |

Split along these seams before files exceed 500 lines. `core/` remains standard-library-only;
`eval/` reads core states/events; project never imports command implementations or Qt.

## 5. Verification and reporting discipline

Each implementation slice must state what changed, which acceptance rows it covers and what
still cannot run. Run the existing architecture/file-size guards and relevant tests before
committing. The final integration uses desktop/headless `check`, native Linux/Windows CI,
seed-42 regression, full same-build replay and the owner exercise.

Current preparation did not run a simulator or produce new performance/delay figures. The
local environment has a C++ compiler but no CMake/CTest/Ninja; the attempted CTest command
could not start. Standalone repository guards can still run. CI status belongs to the
actual draft PR checks and must not be inferred from previous `main` runs.
