# Veytrix

Standing orders for anyone — human or model — working in this repo. Read this first, every
session.

## What this is

A **traffic microsimulator with its own simulation engine**, built to the modelling surface
PTV Vissim users already think in, producing the movement-level delay and LOS output that
traffic impact studies require. Deliberately **not** a front end over another engine — see
[`docs/PROBLEM.md`](docs/PROBLEM.md) §2 for why that was tried and where it hit walls.

**Current milestone:** M0 — vertical slice
**Done when:** `npm run dev` shows vehicles accelerating, queueing at red, and discharging at
green plausibly, and `npm test` proves the same seed reproduces the same run.

**Status: no code yet.** The repo currently contains only documentation. The first coding
session sets up the toolchain (see `Next` in `docs/PROGRESS.md`).

## Read these before working

| File | When |
|---|---|
| [`docs/PROBLEM.md`](docs/PROBLEM.md) | Before proposing any feature. Scope lives there. |
| [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md) | Before arguing with an existing choice. Rules 1–4 are correctness. |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Before adding a system. Update when the map changes. |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | To see which milestone this is and what closes it. |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | Every session. Read `Next` first. |

## Stack

TypeScript, `strict: true`, Vite. Simulation core is plain TypeScript with **no imports from
anywhere else in the repo** — see decision D3 for why the core is written to be portable to a
compiled language later without touching anything above it.

## Commands

```bash
npm run dev     # run it
npm test        # test it
```

If either fails on a clean checkout, fixing that comes before any feature work.

## Hard rules

The full list with reasoning is in [`docs/PRINCIPLES.md`](docs/PRINCIPLES.md). The ones that
get broken by accident:

1. **`core/` imports nothing.** No UI, no I/O, no framework. The moment it does, the engine
   stops being testable, batchable and portable, and that is the project's most valuable
   property.
2. **Reproducibility is not optional.** Same scenario + same seed = same trajectory, forever.
   No wall-clock, no unordered iteration, no thread-dependent floating point in `core/` or
   `eval/`.
3. **One source of truth.** If two places need the same value, the boundary is wrong — move
   it, do not sync it.
4. **Never claim fidelity that has not been measured.** Until M6 passes, every results screen
   carries a "not yet validated" marker.
5. **Content is data, not code.** Vehicle types, behaviour presets, LOS thresholds live in
   `data/`. If adding the 50th one needs a code edit, fix the boundary instead.
6. **Files stay near 500 lines.** Check with `python tools/check_file_sizes.py .`
7. **Never leave the build red.** If it cannot be made green, revert to the last green commit
   and write down what was attempted.
8. **Never close a milestone that has not met its gate.** Merged code is not a passed gate.
   If something must ship incomplete, carve it into a new numbered milestone in
   `docs/ROADMAP.md` in the same session.

## Working rules

- **One system per session.** If it looks like two, it is two.
- **Write the interface first** — the functions other systems will call. That contract is what
  lets a later session build against this without reading its insides.
- **Smallest version that works.** Speculative generality written blind is the most expensive
  thing in this repo.
- **Update `docs/PROGRESS.md` before committing** — what changed, what is next, and the
  reasoning behind any non-obvious decision. This is the memory the next session runs on.

## Session start

Read this file, `docs/ARCHITECTURE.md`, and the **Next** section of `docs/PROGRESS.md`. Run
the test command. Then start on `Next` — do not re-plan; the plan is already here.

## Session end

Stop at roughly three-quarters of context, or when the current system is done. In order:
build green (or reverted to green) → `Next` written specifically enough to need no questions
→ decisions logged with reasons → commit → tell the user what changed, in outcomes.

## Layout

```
docs/           PROBLEM · PRINCIPLES · ARCHITECTURE · ROADMAP · PROGRESS
src/core/       simulation engine — imports nothing
src/model/      network · demand · control data model
src/commands/   every mutation, undoable, one registry
src/project/    load, save, revisions, validation
src/render/     network view, isolated
src/editor/     tools, inspector, tables
src/shell/      layout, palette, i18n
src/eval/       event stream → measurements
src/runner/     multi-seed batches
src/report/     impact-study output
data/           vehicle types · behaviour presets · LOS thresholds
tests/
```

## Project-specific rules

<!-- Conventions discovered while building. Add here rather than re-deriving them. -->
- Documentation is written in **English**. UI strings live in translation files only.
- Screen vocabulary follows Vissim ("link", "connector", "conflict area"); **parameter names
  are never translated**, so users can find them in the project file.
