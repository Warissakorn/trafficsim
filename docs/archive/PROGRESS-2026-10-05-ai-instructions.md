# Archived progress — shared AI instructions and documentation authority

Moved whole from PROGRESS on 2026-10-09. Current work lives in NEXT/ROADMAP.

## 2026-10-05 — Shared AI instructions and documentation authority

Owner authorized the documentation organization plan. AGENTS.md is the shared entry
point; CLAUDE.md delegates to it. Standing rules and model invariants are retained,
while detailed status is read from NEXT/ROADMAP. The documentation map now selects
context by task and the decision index locates existing D-numbers without copying
their reasoning. README's codec range is corrected to schemas 1–19, matching
`documentFromJson`; feature-dependent legacy save versions remain unchanged.
The owner's current task explicitly takes priority over the standing session queue.
No product scope, gate, owner review or engine behaviour is changed.
Three pre-existing archive links are repaired. The source-spec parts stay byte-identical:
Network Editor's existing D38 edit is now explicit in README/manifest, retaining the
original hash plus a retained-copy hash and its authorizing commit.
Validation: local Markdown links/anchors, source-spec hashes, file-size guard and
`git diff --check`. No local CTest claim: CMake/CTest are unavailable in this workspace.
