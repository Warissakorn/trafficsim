# Documentation map

Every file in `docs/`, grouped by what it is for. Engineering documentation is in English;
the supplied Thai specifications in `specs/` are kept verbatim. **Status lines live in
[`NEXT.md`](NEXT.md) and [`ROADMAP.md`](ROADMAP.md) only.** Other files describe a design, a
contract or a dated record. When one of them states a status, its own date applies.

## Start here, every session

| File | What it is |
|---|---|
| [`NEXT.md`](NEXT.md) | The one live to-do. Read first. |
| [`PROBLEM.md`](PROBLEM.md) | Audience and scope. Every feature traces back to a line here. |
| [`PRINCIPLES.md`](PRINCIPLES.md) | Rules that do not get relitigated. Rules 1–4 are correctness. |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Modules, dependencies and contracts between them. |
| [`ROADMAP.md`](ROADMAP.md) | M0–M7 as a sequence, with each milestone's gate. |
| [`PROGRESS.md`](PROGRESS.md) | Session history, newest first, and the decision log (D-numbers). |

## How the product works (current reference)

| File | What it is |
|---|---|
| [`SIMULATION.md`](SIMULATION.md) | Engine contracts: stepping, car-following, signals, right-of-way, lane changes, results. |
| [`NETWORK_EDITOR.md`](NETWORK_EDITOR.md) | The desktop editor: tools, demand, control, Run, Results, save/recovery and file formats. |
| [`NETWORK_EDITOR_CONNECTORS.md`](NETWORK_EDITOR_CONNECTORS.md) | The editor's Connector sections, split out to stay under 500 lines. |
| [`CONNECTOR_FOUR_POINT_MOUTH.md`](CONNECTOR_FOUR_POINT_MOUTH.md) | The Connector mouth geometry contract (D80). |
| [`EDITOR_WORKFLOW.md`](EDITOR_WORKFLOW.md) | History, keyboard editing and rotation. |
| [`AUTHORING_EXTENSIONS.md`](AUTHORING_EXTENSIONS.md) | M1.21: the implemented subset of the supplied specifications. |
| [`BUILDING.md`](BUILDING.md) | Toolchains for Linux and Windows, presets, Qt licensing. |
| [`CMakeUserPresets.windows.example.json`](CMakeUserPresets.windows.example.json) | Example user presets for Windows, referenced by `BUILDING.md`. |
| [`MIGRATION.md`](MIGRATION.md) | The D15 move from TypeScript to C++, and the baseline fixtures it keeps. |

## Milestone plans, contracts and gate records

Dated design and acceptance documents. For what is done, see `ROADMAP.md`.

| File | What it is |
|---|---|
| [`M1_ACCEPTANCE.md`](M1_ACCEPTANCE.md) | M1 owner acceptance record (accepted by ruling, D49). |
| [`M2_PLAN.md`](M2_PLAN.md) | The M1 review and the M2 plan (2026-09-24). |
| [`M2_GATE.md`](M2_GATE.md) | M2 gate record (passed by the owner's judgment, D51–D53). |
| [`M3_PLAN.md`](M3_PLAN.md) | M3 delivery plan: the slices (2026-09-24). |
| [`M3_CONTRACT.md`](M3_CONTRACT.md) | M3 right-of-way contract: conflict areas, priority, waiting lines. |
| [`M3_8_CONTRACT.md`](M3_8_CONTRACT.md) | M3.2.8 behaviour contract: commitment, lane changes, cooperation. |
| [`M3_ACCEPTANCE.md`](M3_ACCEPTANCE.md) | M3 test matrix, evidence rows and the owner exercise sheet (§3). |

## Audits and reviews (dated)

| File | What it is |
|---|---|
| [`ROADMAP_REVIEW-2026-10-03.md`](ROADMAP_REVIEW-2026-10-03.md) | Roadmap accuracy audit and the owner's ten order questions (O1–O10). |
| [`VISSIM_PARITY.md`](VISSIM_PARITY.md) | Feature-by-feature gap to Vissim's surface. §1a and §2 are current. |
| [`CONNECTOR_PARITY_AUDIT.md`](CONNECTOR_PARITY_AUDIT.md) | Connector against the supplied spec and against Vissim, kept apart. |
| [`SPEC_AUDIT.md`](SPEC_AUDIT.md) | The supplied specifications against the code (2026-09-20). |
| [`NETWORK_LIFECYCLE_AUDIT.md`](NETWORK_LIFECYCLE_AUDIT.md) | Network authoring regression audit (2026-09-21). |
| [`UI_REDESIGN_AUDIT.md`](UI_REDESIGN_AUDIT.md) | The D81 precision-tool restyle; §6 lists the open gaps. |

## Folders

| Folder | What it holds |
|---|---|
| [`specs/`](specs/README.md) | The three supplied Thai target specifications (Link, Connector, Network Editor), verbatim. |
| [`evidence/`](evidence/) | Measurement records behind decisions: sweeps, CSVs, metadata, session checks. Each is cited by a `PROGRESS.md` entry or a decision row. |
| [`images/`](images/) | Screenshots used by the docs. |
| [`archive/`](archive/README.md) | Old PROGRESS, ROADMAP and VISSIM_PARITY blocks, moved out whole. Indexed newest first; nothing there is current. |

## Keeping this tidy

- A new top-level doc gets a row here in the same commit.
- `PROGRESS.md` stays under 500 lines (`trafficsim-check-file-sizes`). Move the oldest live
  entries whole into `archive/`, and list the new file in `archive/README.md`.
- Moving a file into `archive/` means its relative links need `../`.
