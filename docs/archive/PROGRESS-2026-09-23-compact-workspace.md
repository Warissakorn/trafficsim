# Archived from PROGRESS.md — 2026-09-23, compact desktop workspace

Moved out 2026-09-24 as the oldest live entry.

---

## 2026-09-23 — Compact desktop workspace

**Owner request:** improve the UI and use screen space efficiently. The shell now has a light
slate/teal palette, local line icons, translated menus, a compact command row that splits at
narrow widths, independently scrolling property tabs, a collapsible appearance section, and
contextual table actions. The application opens maximized. All run figures and the permanent
unvalidated marker remain visible above the canvas, with wrapping instead of toolbar clipping.

Focus on network (Ctrl+Shift+F) snapshots the dock layout, hides panels and restores their
positions and visibility. Opening a panel exits focus; Reset panel layout recovers the default
workspace. Layout never mutates the document or simulation. Language also has a menu fallback.

The larger viewport exposed an outdated connector UI assertion after Link movement: M1.20 can
retain the attachment at a station along the Link. The test now checks the authored attachment,
including its station, rather than assuming it must still be the last vertex. No geometry,
engine, schema or baseline fixture is changed.

**Validation:** clean Linux desktop build and `check` pass (36/36 tests), including architecture
and file-size guards. The new workspace test exercises focus/restore, floating and hidden docks,
layout reset, narrow toolbar access and independent property scrolling in English and Thai.
GitHub Actions passes Linux headless/release/desktop and Windows core/desktop for the code commit.
Inspected screenshots are linked in `EDITOR_WORKFLOW.md`: the canvas is 807×474 in the English
1360×860 window and 471×288 in the Thai 1024×768 window on the Linux offscreen Qt platform.
No M0/M1 engineering acceptance or scientific-validation gate is closed by this UI work.

