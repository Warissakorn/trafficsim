# Documentation map

Every file in `docs/`, grouped by what it is for. Engineering documentation is in English;
the supplied Thai specifications in `specs/` retain their documented owner edit. **Status lines live in
[`NEXT.md`](NEXT.md) and [`ROADMAP.md`](ROADMAP.md) only.** Other files describe a design, a
contract or a dated record. When one of them states a status, its own date applies.

## Read by task

Start at [AGENTS.md](../AGENTS.md), then NEXT and the relevant module README. The table
selects additional context; read related decisions and evidence only when needed.

| Task | Read before editing | Code entry points |
|---|---|---|
| Engine, following, lane changes or safety | [SIMULATION](reference/SIMULATION.md); [M3_CONTRACT](reference/M3_CONTRACT.md); [M3_8_CONTRACT](reference/M3_8_CONTRACT.md); [M3_ACCEPTANCE](plans/M3_ACCEPTANCE.md) | `src/core/`, `src/model/network/`, `tests/` |
| Link/Connector geometry | [CONNECTOR_FOUR_POINT_MOUTH](reference/CONNECTOR_FOUR_POINT_MOUTH.md); [NETWORK_EDITOR_CONNECTORS](reference/NETWORK_EDITOR_CONNECTORS.md); [CONNECTOR_PARITY_AUDIT](audits/CONNECTOR_PARITY_AUDIT.md) | `src/model/network/`, `src/editor/`, `src/commands/` |
| Demand, catalogs or routing | [DEMAND_IMPROVEMENT](plans/DEMAND_IMPROVEMENT.md); [DEMAND_CATALOGS](reference/DEMAND_CATALOGS.md); [DEMAND_TIME_TYPES](reference/DEMAND_TIME_TYPES.md); [SIMULATION](reference/SIMULATION.md) | `src/project/`, `src/model/demand/`, `src/model/network/routing.cpp` |
| Editor UI, gestures or Undo/Redo | [NETWORK_EDITOR](reference/NETWORK_EDITOR.md); [EDITOR_WORKFLOW](reference/EDITOR_WORKFLOW.md); [VISSIM_PARITY](audits/VISSIM_PARITY.md); [UI_REDESIGN_AUDIT](audits/UI_REDESIGN_AUDIT.md) | `src/editor/`, `src/shell/`, `src/commands/` |
| Vehicle display and turning | [VEHICLE_POSE](reference/VEHICLE_POSE.md); [SIMULATION](reference/SIMULATION.md) | `src/editor/vehicle_pose.*`, `rear_axle_pose.*`, `lane_change_pose.*` |
| Load/save or schema changes | [NETWORK_EDITOR](reference/NETWORK_EDITOR.md) save/recovery; relevant Demand/geometry contract; [MIGRATION](reference/MIGRATION.md) | `src/project/`, `tests/project_tests.cpp` |
| Evaluation or reporting | [PROBLEM](PROBLEM.md); [SIMULATION](reference/SIMULATION.md); [ROADMAP](ROADMAP.md) M5/M6; [M2_PLAN](plans/M2_PLAN.md) | `src/eval/`, `src/project/evaluation.*`, `src/runner/`, `src/report/` |
| Build or portability | [BUILDING](BUILDING.md); [MIGRATION](reference/MIGRATION.md); tools README | `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/` |

Resolve the document names through the indexes below. `src/runner/` and `src/report/`
are planned module boundaries, not implemented batch/report products.

## Sources and status

- NEXT owns the live queue and pending owner checks; ROADMAP owns milestone status/gates.
- Contracts define behaviour; ARCHITECTURE defines boundaries; PRINCIPLES defines rules.
- PROGRESS records sessions. The [decision index](decisions/README.md) locates their reasoning.
- Dated reviews are evidence/proposals, not permission to change the owner's priorities.
- Engineering docs are English. Supplied Thai specs retain their documented provenance.

## Start here, every session

| File | What it is |
|---|---|
| [`NEXT.md`](NEXT.md) | The one live to-do. Read first. |
| [`PROBLEM.md`](PROBLEM.md) | Audience and scope. Every feature traces back to a line here. |
| [`PRINCIPLES.md`](PRINCIPLES.md) | Rules that do not get relitigated. Rules 1–4 are correctness. |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Modules, dependencies and contracts between them. |
| [`BUILDING.md`](BUILDING.md) | Linux/Windows toolchains and dependency setup. |
| [`CMakeUserPresets.windows.example.json`](CMakeUserPresets.windows.example.json) | Local Windows preset example referenced by BUILDING. |
| [`ROADMAP.md`](ROADMAP.md) | M0–M7, milestone gates, the dated accuracy review, owner decisions O1–O10 and proposed sessions S0–S5. |
| [`PROGRESS.md`](PROGRESS.md) | Session history, newest first; links to the indexed decision record. |

## Folders

| Folder | What it holds |
|---|---|
| [`reference/`](reference/README.md) | Current behaviour and contracts. |
| [`plans/`](plans/README.md) | Milestone plans and acceptance/gate records. |
| [`audits/`](audits/README.md) | Dated audits and reviews. |
| [`specs/`](specs/README.md) | Supplied Thai target specifications and their original/retained byte manifests. |
| [`evidence/`](evidence/) | Measurement records behind decisions: sweeps, CSVs, metadata, session checks. Each is cited by a `PROGRESS.md` entry or a decision row. |
| [`images/`](images/) | Screenshots used by the docs. |
| [`archive/`](archive/README.md) | Old PROGRESS, ROADMAP and VISSIM_PARITY blocks, moved out whole. Indexed newest first; nothing there is current. |
| [`decisions/`](decisions/README.md) | Complete decision record with stable D-number anchors and a topic index. |

## Keeping this tidy

- A new maintained doc gets a row in its folder index in the same commit.
- The tables above index docs at the root and link to the purpose folders.
- Run `trafficsim-check-docs .` (also in `check`/CTest) for local inline Markdown links,
  anchors and maintained-folder index coverage. Source-spec example parts are excluded.
- `PROGRESS.md` stays under 500 lines (`trafficsim-check-file-sizes`). Move the oldest live
  entries whole into `archive/`, and list the new file in `archive/README.md`.
- Moving a file into `archive/` means its relative links need `../`.
- Keep status in NEXT/ROADMAP; link to it from summaries instead of copying detailed queues.
- A contract distinguishes implemented behaviour, planned behaviour and acceptance evidence.
- Record the verification platform and date; automated CI does not replace owner observation.
- Start archiving/splitting maintained docs near 450 lines, before the 500-line check fails.
- Preserve decision IDs, milestone IDs, gates and source-spec bytes during reorganization.
