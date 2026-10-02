# Windows session check of the owner's items — 2026-10-02

**What this is:** a session ran the owner's open Windows checks (NEXT.md) on the owner's Windows
machine, at the user's request. **It is not the owner's look.** The session could not see or drive
the live desktop (no screen-control tools), so the evidence is the Qt UI suites run on the real
`windows` platform plugin and offscreen at three scale factors, screenshots the session read back,
and CLI runs. Items whose failure condition is the owner's judgment stay open in NEXT.md.

**Machine:** Windows 11 Home 10.0.26200; MSVC 19.51.36260; Qt 6.8.3; Debug desktop preset at
`ee089c7`. Screens: 1920×1080 at 96 DPI (primary) and 1536×864.

## 1. Build and tests

`cmake --preset desktop`, build, `ctest --preset desktop -j 4`: **75/75 passed**, 66 s total.
`scenario-run-ui` alone, offscreen, Debug: **28.1 s CPU, 30.4 s wall**. Two earlier runs that overlapped a
40-seed CLI batch measured 76–82 s CPU / 89–95 s wall, so load from another process is enough to
push it past `TIMEOUT 90`. Under ctest `-j 4` it took 22.6 s.

## 2. UI suites on the real `windows` platform (`QT_QPA_PLATFORM=windows`, `QT_SCALE_FACTOR` 1 / 1.5 / 2)

At 100 %, 20 of 23 suites pass. The three that fail are artefacts of how the session ran them:
- `editor-interaction-ui` ("Select hover has no feedback") and `demand-ui` ("Hover halo missed the
  Link") pass offscreen. On a real platform the Windows cursor's own position competes with
  `QTest::mouseMove`, so hover cannot be tested this way.
- `network-lifecycle-ui` takes the screenshot path as `argv[1]`; with the right argument it passes
  at all three scales.

At 150 % and 200 % on the `windows` platform, `tables-ui`, `editor-gestures`, `priority-canvas`,
`connector-ui`, `editor-attachments` and `workspace-ui` fail with "Gesture outside viewport" or
"Default canvas crowded out". All pass **offscreen** at the same scale. The cause is the
screen: 1080 px at 150 % leaves 720 logical px, and Windows clamps the test window to it.
This is not a product defect in itself. It does say an owner on a 1080p screen at 150 % gets a
canvas smaller than `workspace-ui`'s 700×350 floor. That is the owner's call, not measured further.

**`design-system-ui` fails at 150 % and 200 %, offscreen too (a finding).** "QLineEdit text is not
centred: 10 above, 12 below" at 150 % and 13/16 at 200 %, against the test's ±1 device pixel. In an
enlarged crop of the 150 % workspace, the plain line edit's "3.5" sits ≈1 logical px high, while the
spin boxes' and the dropdown's text is centred. The test measures device pixels with a 1 px
tolerance, so at 150 % that is ⅔ of a logical pixel. D84's "text centred" holds at 100 % and is not
met at 150/200 % for `QLineEdit`. Not fixed this session.

## 3. What the screenshots show, item by item

| Item | Evidence | Session finding |
|---|---|---|
| D83/D84 look | `workspace-ui` en/th @100 %, en @150/200 % offscreen; enlarged crops | Dropdown chevrons and the two stacked spin-box chevrons are visible at 100 and 150 %. Thai and English share one face. Spin box and button text is centred; line-edit text is not centred at 150/200 % (above). |
| D84 gestures | `editor-gestures`, `editor-attachments`, `conflict-auto-ui`, `queue-counter-ui`, `priority-ui`, `signal-ui`, `authoring-ui` pass on `windows` @100 % | The suites' assertions (a left click never authors; Ctrl+right authors) hold on the real platform. Whether Ctrl+right is slow for heavy work is the owner's (D84's failure condition). |
| D86 | `conflict_follow`, `conflict-auto-ui` pass | Areas follow the overlap, and go and return with Undo, at command level. Not seen dragged in the desktop. |
| D80 mouths | `connector-surface-ui`, `connector-ui`, `editor-rotation-ui` pass @100 %; surface screenshot | The mouth meets the Link flush. Angles 45–179° were not each inspected by eye. |
| D100 wireframe | `wireframe-ui` @100/150/200 % | Centre lines read clearly on the grid. No vehicle was in shot; readability beside multi-lane lines is the owner's. |
| D102 | `lane-change-display-ui` @100/150/200 % | The changing car straddles the lane line with its nose turned toward the new lane: a slide, not a jump. |
| Run view | `scenario-run-ui` @100/150/200 % | Passes on the `windows` platform at all three scales. |

**Stale UI text (a finding).** The editor's scope banner (`editorScope`, `editorScopeCompact`)
says "no lane changing or LOS", in both locales. The engine has done mandatory lane changes since D71
and changes at downstream decisions since D93, and the Run view draws them (D102). The input split
help (`editorInputSplitHelp`) says "this engine has no lane changing". The LOS half and the
"conflicts resolved only where authored" half are still true (D68's derived areas are passive).
Not fixed this session: these are UI strings in both catalogs.

## 4. Behaviour, by CLI (Windows/MSVC, Debug)

**D90, M2.6, seeds 42–81, `--lane-changes`.** Pooled dead-end waits: East → South 43 vehicles,
2,067 s, longest 90 s. East → North 12, 139 s, 42 s. North → East 10, 432 s, 81 s. Mean movement
delay: East → West 63.8 s, East → South 57.2 s, East → North 50.0 s, West → East 48.4 s. **Every
figure is identical to the Linux "coop" column** of `m3.2.8c-cooperative-braking.md`. This is
cross-platform agreement for D90, not the owner's look at the East approach.

**D93, a scratch copy of `four-leg-signalised`:** the three routed West inputs (720 veh/h) are
replaced by one routeless input on `link-1`, with a decision on the pocket `link-4`: East exit 3,
North exit 1. Seed 42 runs (rc 0): West → East 118 vehicles, West → North 41, West → South 0. That
is 2.9 : 1, with 34 and 32 lane changes and no unplaced change. The `routeless` test's exact 3:1
draw passes on Windows. Not seen in the desktop.

D96 (route overlay) and the interaction cleanup: `demand-ui` and `editor-interaction-ui` pass offscreen
on Windows; on the `windows` platform, hover cannot be judged this way (§2).

## 5. Not covered

The live desktop by eye: curved/overlapping roads at working zoom, dragging in the Conflict
tool, the D80 angle sweep by hand, and Ctrl+right speed. M3.2.7d, M0 plausibility and the name
stay the owner's.
