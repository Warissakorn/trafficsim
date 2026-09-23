# PROGRESS entry moved out of docs/PROGRESS.md on 2026-09-22

## 2026-09-21 — Connector parity audit (documentation only; no code changed)

**Request:** the owner asked whether the Connector matches Vissim in every respect, then asked
for everything that could actually be done about it in the session.

**The audit itself** is `CONNECTOR_PARITY_AUDIT.md`. It is the first place in this repository that
states the **two benchmarks** side by side: the owner's supplied Thai specification (a *target*,
per `specs/README.md` and `SPEC_AUDIT.md`) and Vissim itself (**never measured here** — only the
owner's screenshots are on file). §1 of the audit lists what matches the specification, §2 what is
absent, §3 the defects, §4 what was deliberately not done. Do not quote §1's "matches" as a
Vissim claim.

**What was changed in this session** is documentation and comments only:

- `CONNECTOR_PARITY_AUDIT.md` — new.
- `VISSIM_PARITY.md` — a 2026-09-21 entry, plus a one-line inline "superseded" note on the
  2026-09-18 wedge entry, whose sentence *"a plain square end. That is now what is drawn"* has
  been false since M1.18.
- `connector_commands.hpp` — two header comments corrected against the code they declare:
  `changeConnectorGeometry` *does* move the endpoints (only the lane reference is fixed there),
  and `resampleConnectorPoints` splits the longest leg when **raising** the count rather than
  re-spacing evenly, because even re-spacing was measured to cut a hand-placed corner by 1.00 m.
- `CLAUDE.md` — a row in the "Read these before working" table.

**The stale `PROGRESS.md` entry was not rewritten — it was moved whole to
[`PROGRESS-2026-09-18-square-mouth.md`](PROGRESS-2026-09-18-square-mouth.md).** The
2026-09-18 entry *"The Connector mouth is a plain square end again"* is append-only history, so its
text is preserved exactly as written; only its **location** changed, and the reason is hard rule 6
— this entry pushed the live file to 504 lines, past the 500-line guard `tools/check_file_sizes.cpp`
fails on. It was the oldest live entry, so it is the one the file's own header says to move. This
entry is the correction of record: what is drawn today is the **M1.18 longitudinal slide onto the
Link's cross-section**, with a full-width square end kept only as the fallback for an arrival more
than roughly 75° off its spine (`road_boundaries.cpp:324-325`), reported as
`WARN_CONNECTOR_ALIGNMENT`.

**Three runtime findings are recorded, not fixed** — all need a build, and this session had no
C++ toolchain (`cmake`, `g++`, `cl`, `clang++`, `ninja` all absent), so under hard rule 7 nothing
was touched that could not be verified. Booked:

1. **Two Connectors arriving at the same station on one lane do not yield to each other.** The
   derived rule names only the upstream lane section, never another Connector's path
   (`sections.cpp:195`). Whether that permits a one-step overlap at the drawn station is
   **untested either way** — a test comes before any fix, and the fix belongs with conflict areas
   in M3.2.
2. **A merge at a lane's *start* is uncontrolled and unreported** (`sections.cpp:203`). Consistent
   with `UNSUPPORTED_MERGE`, but nothing tells the author.
3. **`buildScenario` can carry a zero-gap priority rule** when the catalog is unset; only the
   `compileScenario` path refuses it (`compile.cpp:95`, `core/validate.cpp:73`). Latent — no such
   caller exists today.

**No milestone is closed by this entry.** No gate is met by documentation. M0/M1 owner acceptance
and M6 validation remain open, and no test was added or run.

**Superseded the same day, in the entry above:** finding 1 was answered with a toolchain — the pair
is *refused*, not overlapped, so the audit's §3.3 has been rewritten and a test added. Findings 2
and 3 stand as recorded.

---
