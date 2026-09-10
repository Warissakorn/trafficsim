# TrafficSim

Standing orders for anyone (human or model) working in this repo. Read this first, every session.

## What this is

TODO: one line description

**Current milestone:** TODO: current milestone
**Done when:** TODO: done condition

## Stack

TypeScript (strict), Vite. Canvas rendering isolated in src/render.

## Commands

```bash
npm run dev     # run it
npm test    # test it
```

If either command fails on a clean checkout, fixing that comes before any feature work.

## Working rules

- **One system per session.** Finish it, run it, log it, commit it. Don't leave two half-built things.
- **Files stay near 500 lines.** Over that, split along the seam that's already there. Check with `python tools/check_file_sizes.py .` when unsure.
- **Content is data, not code.** New enemies/items/levels/rules go in `data/`. If adding one requires editing code, fix the boundary instead.
- **The build is never left broken.** Ending a session with a red build costs the next session an hour.
- **Update `docs/PROGRESS.md` before committing** — what changed, what's next, and why any non-obvious decision was made. This is the memory the next session runs on.
- **Check `docs/ARCHITECTURE.md` before adding a system**, and update it when the map changes. A stale map is worse than none.

## Session start

Read this file, `docs/ARCHITECTURE.md`, and the **Next** section of `docs/PROGRESS.md`. Run the test command. Then start on Next — don't re-plan; the plan is already here.

## Session end

Stop at roughly three-quarters of context, or when the current system is done. In order: build green (or reverted to green) → `Next` in `docs/PROGRESS.md` written so specifically it needs no questions → decisions logged with reasons → commit → tell the user what changed, in outcomes.

## Layout

```
docs
public
src/data
src/engine
src/entities
src/render
src/systems
tests
```

## Project-specific rules

<!-- Conventions discovered while building. Add to this rather than re-deriving them. -->
-
