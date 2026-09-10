#!/usr/bin/env python3
"""Report source files that have grown past the line budget.

Usage:
    python check_file_sizes.py <project-dir> [--limit 500] [--top 20]

Files drift over the limit one append at a time, and no single append feels
wrong — which is why this is worth checking mechanically rather than by feel.
Exits 1 if anything is over the limit, so it can be wired into CI.
"""

import argparse
import pathlib
import sys

SOURCE_EXT = {
    ".py", ".gd", ".cs", ".ts", ".tsx", ".js", ".jsx", ".c", ".h", ".cpp",
    ".hpp", ".rs", ".go", ".java", ".kt", ".rb", ".lua", ".swift", ".m", ".mm",
}

SKIP_DIRS = {
    ".git", "node_modules", "__pycache__", ".venv", "venv", "dist", "build",
    "Library", "Temp", "Obj", ".godot", ".pytest_cache", "vendor", "third_party",
    "target", ".next", ".vite",
}


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("target", nargs="?", default=".")
    p.add_argument("--limit", type=int, default=500)
    p.add_argument("--top", type=int, default=20)
    args = p.parse_args()

    root = pathlib.Path(args.target).expanduser().resolve()
    rows = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in SOURCE_EXT:
            continue
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        try:
            n = sum(1 for _ in path.open("r", encoding="utf-8", errors="ignore"))
        except OSError:
            continue
        rows.append((n, path.relative_to(root)))

    if not rows:
        print(f"no source files found under {root}")
        return 0

    rows.sort(reverse=True)
    over = [r for r in rows if r[0] > args.limit]
    total = sum(n for n, _ in rows)

    print(f"{len(rows)} source files, {total:,} lines total, limit {args.limit}\n")
    for n, rel in rows[: args.top]:
        flag = "  OVER" if n > args.limit else ""
        print(f"{n:>6}  {rel}{flag}")

    if over:
        print(f"\n{len(over)} file(s) over the limit. Split along a seam that already "
              f"exists inside them (logic vs. rendering, one concept per file) rather "
              f"than at an arbitrary midpoint — and update docs/ARCHITECTURE.md if the "
              f"split changes the map.")
        return 1

    print("\nAll files within budget.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
