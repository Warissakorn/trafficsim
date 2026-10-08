# Archived progress — Repository housekeeping

Moved whole from PROGRESS on 2026-10-08. Current work lives in NEXT/ROADMAP.

## 2026-10-05 — Repository housekeeping

Owner asked to clean the project files. Removed the committed local Qt installer log
from the root and ignored future installer output, temporary editor files and desktop
metadata. JSONL run output remains ignored by default, but `docs/evidence/*.jsonl`
is explicitly allowed so a new measurement record is not silently omitted from Git.
Existing evidence and frozen fixtures are retained. Decision-ID navigation is wrapped
for source readability without changing any link or decision row.
Validation: documentation guard, file-size guard, retained source-spec hashes,
Git ignore checks for generated output versus evidence and `git diff --check`.
