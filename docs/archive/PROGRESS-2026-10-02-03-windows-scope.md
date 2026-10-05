# Archived PROGRESS — Windows session and scope text

Moved whole from PROGRESS on 2026-10-05.

## 2026-10-03 — Scope text says what lane changing there is

The owner approved new wording for `editorScope`, `editorScopeCompact` and `editorInputSplitHelp`
(en/th). Each "no lane changing" becomes "lane changes only where a route requires one". That is
what the engine does: mandatory changes (D71, D93), with discretionary ones off (D95). "Not yet validated",
"no LOS" and "conflicts resolved only where authored" stay, since they are still true. The split
help also mentions lane shares, which the same dialog sets (M1.26.1). At a 1360 px window the
English banner now wraps to two lines, about 20 px of canvas; `workspace-ui`'s size floors still
pass. Desktop 75/75 on Windows.

## 2026-10-02 — The owner's Windows items, session-checked (no code change)

At the user's request, a session ran NEXT's owner checks on the owner's Windows machine. It used
the UI suites on the real `windows` platform and offscreen at 100/150/200 %, the screenshots, and the
CLI. It could not drive the live desktop. Full results:
`docs/evidence/windows-session-check-2026-10-02.md`. Desktop 75/75 (MSVC 19.51, Qt 6.8.3, Debug).
- **Two defects:** the scope banner and the input split help still say "no lane changing", which
  has been untrue since D71/D93. `QLineEdit` text sits ≈1 logical px high at 150/200 %:
  `design-system-ui` fails at those scales, offscreen too, so D84's centring holds at 100 % only.
  Both are booked in NEXT, not fixed here.
- **Matches:** D90's 40-seed M2.6 dead-end waits and delays are identical on MSVC and Linux. A
  D93 routeless pocket decision of 3:1 ran 118 : 41 : 0. D102's slide is visible in screenshots.
- **Method note:** on a real platform the Windows cursor competes with `QTest::mouseMove`
  (hover assertions fail). At 150/200 % on a 1080p screen, Windows clamps the test window, so the
  gesture suites are only meaningful offscreen at those scales.
- **Why nothing is closed:** a suite passing is not the owner's look. D84's and D100's failure
  conditions are the owner's judgment, and hard rule 8 forbids closing on it.
