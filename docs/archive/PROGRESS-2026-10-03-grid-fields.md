# Archived PROGRESS entries — 2026-10-03 grid and field scale

Moved whole from PROGRESS.md on 2026-10-05.

## 2026-10-03 — The canvas grid is crisp at every scale

`gridIsCrisp` failed at every scale other than 100 %. The cause was measured offscreen at
1.25/1.5/2×, with the cache on and off. Blended pixels were along the whole length of every line
(7,327 / 8,815 / 11,791 interior pixels, one colour per tier), not at line ends, and identical with
`CacheNone`. So it was neither the flat caps nor D99's background cache. `hairlinePen` used a
cosmetic width of 1/dpr, on the belief that cosmetic widths are logical pixels. On Qt 6.8 they are
**device** pixels: width 1 draws exactly one device pixel at 1.25, 1.5 and 2× (run lengths
measured in the grab), with no blended pixel. Width 0 did too. `hairlinePen(colour)` is now width 1
with no dpr argument, and `drawBackground` no longer reads the dpr. The `hairlines()` assertion
"0.5 at 2×" encoded the wrong belief and now asserts width 1. The D103 mode is renamed
`--at-scale` and runs `gridIsCrisp()` too, so `design-system-ui-1.5x`/`-2x` failed before the fix
("Blended grid pixel #f7f8f9 at 3,0") and pass after. CI's Qt 6.5.3 runs the same entries, which
is the check that 6.5 agrees. Desktop 77/77 on Windows.

## 2026-10-03 — Field text at 150/200 %: a measured limit, tested (D103)

The Windows session check's `design-system-ui` failure at 150/200 % was measured (`boxModel()`,
offscreen, 1–2× in 0.25 steps). It is **not** `QLineEdit`. QLineEdit, QComboBox and QPushButton
share identical gaps at every scale (7/8, 8/10, 10/12, 12/14, 13/16 device px). They all use the
13 px body face, ascent 14 and descent 6, the descent for Thai below-vowels. Qt centres the line box,
so digits sit ≈0.5 logical px high at 100 % and ≈1.5 at 2×. The spin box's 12 px numeric face
(13/5) is within 1 device px everywhere. A QSS shift of 1 px down for body-font fields centred them
at every scale (8/7, 9/9, 11/11, 15/14). It also put their digits 1 px below the spin box's at 100 %,
which breaks D84's one row for digits. The owner kept the style (D103). The test now holds 100 % exactly
as before. Above it, a control is centred within 1.5 logical px, with rows within 1 logical px.
Two ctest entries, `design-system-ui-1.5x`/`-2x`, run `--box-model-at-scale`. They first assert that
the scale really applied, and only the box model runs, because `gridIsCrisp` fails at any scale
other than 100 % (booked in NEXT, not looked at). A 2 px upward shift fails all three scales, and a
missing scale fails the forcing check. Desktop 77/77 on Windows.

Scope-text and Windows-session entries are preserved in
[`archive/PROGRESS-2026-10-02-03-windows-scope.md`](archive/PROGRESS-2026-10-02-03-windows-scope.md).

D101/D102 session entries are preserved in
[`archive/PROGRESS-2026-10-02-d101-d102.md`](archive/PROGRESS-2026-10-02-d101-d102.md).

