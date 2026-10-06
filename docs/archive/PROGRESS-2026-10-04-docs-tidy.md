# PROGRESS archive — 2026-10-04 docs tidy

## 2026-10-04 — Docs tidy: a map, no broken links, NEXT regrouped

At the user's request ("tidy the docs"), the owner chose a tidy in place: no file moves, so no
path changes. Docs only.
- **`docs/README.md`** is new. It lists every file in `docs/` by purpose: start here, current
  reference, milestone plans and gates, dated audits, folders. CLAUDE.md's read table and the
  root README point at it.
- **Links:** 7 relative links in `archive/` broke when their files moved there, and now carry
  `../`. `PROGRESS-2026-09-28-d79-mouth.md` was missing from `archive/README.md`. A link and
  reference sweep of every `*.md` finds no broken link. The remaining name mismatches are history
  quoting old file names, or a proposed file (`evidence/m6-benchmark-options.md`).
- **NEXT.md** is regrouped, with no item dropped: 0 the owner's order question (O1), 1 one
  numbered checklist of owner looks on Windows, 2 owner decisions, 3 session work, 4 working
  notes, 5 do not retry. The roadmap review's corrections are carried over: WSL is not
  installed here, `Veytrix` was set aside, PRs #76–#78, M2.7 is open. D104 is recorded as part of
  the review's S3.
- **Root README:** its status still said M1 acceptance was open, the name waited on "the end of
  M1", and lane changes, conflicts and priority were future work. It now states D49/D51–D53, an
  open M0, and what the editor does, and its "Implemented" table matches `src/`.
- **NETWORK_EDITOR.md** gains the Results tab and D104's export, which no current doc described.
- **PROGRESS:** D97, D98 and the route table Length entry moved to
  `archive/PROGRESS-2026-10-02-d97-d98.md` (488 → 376 lines).
- The branch merged `main` first (the roadmap review, `cf8fb22`), so none of its 63 corrections
  is undone.

