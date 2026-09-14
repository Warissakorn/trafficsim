# PROGRESS archive — TrafficSim

Entries moved whole out of [`PROGRESS.md`](PROGRESS.md) to keep it inside the 500-line budget
(hard rule 6). Nothing here is edited or summarised — only relocated. Newest first.

The `Next` section, the backlog, the open questions and the decision table all stay in
`PROGRESS.md`; this file is log entries only.

---

### 2026-09-11 — reverted to the working name TrafficSim, naming deferred (D11)

Three renames in two days with no code written. The owner called it: go back to the working
name and decide the real one once the program has shape.

Headings across `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, `ROADMAP.md` and this file are
back to `TrafficSim`, now explicitly marked as a working name so no future session reads it
as settled. D9 and D10 keep their full reasoning and are marked superseded — this file is
append-only, and the collision findings gathered over those rounds are the main thing worth
keeping from them, so they are consolidated into the D11 row. The next naming round starts
from evidence, not from zero.

**Two pieces of queued work are cancelled, not postponed:** the GitHub repository rename (the
repo is still `Warissakorn/trafficsim` and the remote already points there, so there is
nothing to do) and the npm/PyPI/domain registrations for `velk`.

**Two defects in this file were found and fixed while making this change**, both introduced by
earlier sessions of this conversation:

1. **The D10 log entry below was never actually written.** The edit that should have added it
   matched no text, and the guard around that edit only checked that *something* in the file
   had changed — which was true because other edits in the same batch succeeded. It has been
   reconstructed below from the commit message and the D10 row. Guards on edits to this file
   now assert an exact match count per edit.
2. **Entries were in oldest-first order**, contradicting this file's own header. Reordered
   newest-first. No entry text was altered.

Nothing about scope, architecture or the roadmap changed. D1–D8 stand.

### 2026-09-11 — renamed to Velk (D10)

*Reconstructed on 2026-09-11 — see defect 1 in the entry above.*

`Veytrix` replaced throughout the documentation. `velk` verified free on npm and PyPI with no
brand or company found using it; `velk.dev` and `velk.app` free, `velk.com` and `velk.io`
held — ordinary for a four-letter word and irrelevant to a repository or package name, so
accepted as a known risk.

`MicroFlow Simulator` was proposed first this session and dropped after its collision check:
`microflow` taken on npm and PyPI, at least seven GitHub projects carrying the name along
with two orgs and a GitHub Topic, and both obvious domains held. Recorded in D10 so it is not
raised again.

Also noted at the time: the MicroFlow brand write-up claimed "extends to both microscopic and
macroscopic" as a strength, which contradicts `PROBLEM.md` §5 where macroscopic assignment is
a non-goal. **§5 was left unchanged** — that is a scope decision, not a naming one.

### 2026-09-10 — named Veytrix (D9)

Working name `TrafficSim` replaced throughout the documentation. `veytrix` is free on npm
and PyPI; `veytrix.com` is taken and `Vectrix` (electric scooters) is phonetically close —
both recorded in D9 as accepted, known risks rather than discovered later.

**Still to do by hand:** the GitHub repository is still called `trafficsim`. Renaming it needs
repository-admin access, which this session's GitHub app does not have — the owner renames it
in the repository settings, after which the git remote here needs updating.


### 2026-09-10 — Q1 and Q3 answered (D7, D8)

- **Q1 → international from the start** (D7). Consequences recorded: HCM as the default LOS
  pack with jurisdictions as swappable data, metric internally with switchable display units,
  and **left-hand/right-hand traffic as a first-class setting from M1** — added to the M1
  scope in `ROADMAP.md` because retrofitting it touches every geometry routine.
- **Q3 → the project owner performs the M2 gate alone** (D8). Recorded honestly as a
  weakening of the gate, with a mandatory mitigation: the M2 pass/fail criteria must be
  written into `ROADMAP.md` and committed **before** M2 implementation starts. `ROADMAP.md`
  now carries an unfilled placeholder for those criteria; starting M2 without filling it
  voids the gate.
- **Q5 opened:** final product name. `Veytrix` is a placeholder. `Headway` was considered
  and rejected — `headwaymaps/headway` is an existing open-source maps stack, too close a
  neighbour in the same field.

### 2026-09-10 — repository initialized, documentation spine written

Created a fresh repo for a new project, separate from the prior SUMO-wrapper effort.

**Written:** `PROBLEM.md` (who this is for, the engine-level walls that motivate D1, non-goals,
and what would make the project wrong), `PRINCIPLES.md` (hard rules, deliberate non-goals, and
measured discipline inherited from the prior effort), `ARCHITECTURE.md` (the five-layer map,
marked planned throughout), `ROADMAP.md` (M0–M7 with done-conditions and two hard gates),
`CLAUDE.md` (standing orders), this file.

**Decisions:** D1–D6 above. D1 is the one everything else rests on, and it has an explicit
falsification test at the M2 gate.

**No code was written.** The Systems table in `ARCHITECTURE.md` describes intent, not reality;
every row is marked `planned`.

**Next:** toolchain setup — see the `Next` section above.
