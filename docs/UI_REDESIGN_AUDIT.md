# TrafficSim precision-tool UI restyle (D81)

Scope: presentation only — canvas, view, inspector, shell, palette and QSS. No model, command,
codec or runtime change, no shortcut change, no new dependency, no GPU/3D renderer.

**Verification status.** The coding workspace that wrote this had no Qt/CMake toolchain, so none
of it was compiled or run there; `design-system-ui` and the existing UI suites are what verify it,
on the Native C++ workflow (Linux + Windows). No performance claim is made: nothing here was
benchmarked, and the grid now draws fewer lines than before only by arithmetic (minor spacing
≥ 8 px), not by measurement.

## 1. Audit (against the brief)

| # | Requirement | Found before this pass | Now |
|---|---|---|---|
| 1 | All colour through QPalette roles | QSS coloured everything through `@gray*` hex substitution; canvas used a dozen literals (`#de8618`, `#167b98`, `#b33f8d`, `#7c3aed`, `#dc2626`, …) | QSS is `palette(role)` only (test: no hex in QSS); canvas reads `editorPalette()` roles through `canvasStyle::*()`; the only hex literals live in `ui_design_tokens.hpp` |
| 2 | One accent `#2F6FED`; semantic reserved | Accent `#0F766E`; orange/teal/magenta/violet used for tools | Accent `#2F6FED` = `Highlight`; every tool overlay uses it; error/warning/advisory/ok are the only other hues |
| 3 | 4 px scale, 28 px rows/controls, 32 px toolbar, 16 px 1.5 px icons | Controls 30 px (content 20 + padding + border), focus border changed height, icons 20 px / 1.6 | Box model 1 + 4 + 18 + 4 + 1 = 28; 2 px focus/invalid border takes 1 px of padding back; toolbar 24 + 2×4 = 32; icons 16 px, 1.5 px stroke |
| 4 | Five type sizes; 11 px tracked uppercase labels | Sizes right, but `letter-spacing` in QSS is not a Qt property — tracking never applied | Tracking set on `QFont` (`styleGroupLabel`, English only; Thai untracked); QSS banned from using it |
| 5 | Tabular, right-aligned, fixed decimals, unit, QLocale | Monospace font yes; `QString::number(…,'f')` (C locale) for lengths/coordinates; trimmed decimals in range hints | `formatValue(value, decimals, unit, locale)`; numeric spin boxes/edits right-aligned; range hints use the field's own locale and decimals |
| 6 | Hairlines: cosmetic, 0.5 px offset, devicePixelRatio | Grid: cosmetic pen of width 1 (2 device px at 2x), unsnapped | `hairlinePen` = 1/dpr wide cosmetic; `snapHairline` centres each line on a device pixel (+0.5) from `painter->deviceTransform()` |
| 7 | Two-tier grid, LOD from `levelOfDetailFromTransform` | Single tier, step ×10 until 20 px | Minor ≥ 8 px, major = 10 × minor, both from `levelOfDetail(worldTransform)` |
| 8 | Full states + inline validation stating range | Present (hover/focus/disabled/invalid, range hints) | Kept; hover border and disabled/read-only tokens moved to roles; invalid no longer changes control height |
| 9 | Buddies, tab order, native focus | Present | Untouched (tab-order code not edited); test asserts buddies still take focus |
| 10 | No gradient/glow/shadow/radius>3/animation>150 ms | Radius 2, no gradients | Asserted by test on the QSS text and on `QAbstractAnimation` children |

## 2. Tokens

### QPalette roles (`editorDesign::editorPalette()`)

| Role | Value | Use |
|---|---|---|
| `Base` / `Light` | `#FFFFFF` | Canvas, inputs, table body |
| `Window` / `Button` / `AlternateBase` | `#F9FAFB` | Panels, headers, zebra rows |
| `Midlight` | `#F3F4F6` | Hover, disabled, minor grid |
| `Mid` | `#E5E7EB` | Hairlines, selection fill, major grid |
| `Dark` | `#6B7280` | Control outline (4.8:1 on white, ≥ 3:1 required) |
| `WindowText` | `#4B5563` | Secondary text, icons (7.6:1) |
| `Text` / `ButtonText` | `#111827` | Primary text |
| `Highlight` (+`Accent` on Qt ≥ 6.6) | `#2F6FED` | Selection, focus, active tool. Fill/border only: 4.55:1 on white, 4.1:1 on `Midlight` |
| `HighlightedText` | `#FFFFFF` | On the accent (4.55:1) |
| `BrightText` | `#B42318` | **error** |
| `LinkVisited` | `#8A4B08` | **warning** |
| `Link` | `#0B6E8A` | **advisory** |
| `Shadow` | `#176B44` | **ok** (the editor draws no shadows, so the role is free) |

Roles the editor does not name keep no hue (test: only the accent and the four semantic roles are chromatic).

### Metrics

| Token | Value |
|---|---|
| Spacing | 4 / 8 / 12 / 16 / 20 / 24 px |
| Type | 11 / 12 / 13 / 14 / 18 px |
| Control / row | 28 px (18 content + 2×4 padding + 2×1 border) |
| Toolbar | 32 px (24 button + 2×4 padding) |
| Icon | 16 px logical, 1.5 px stroke, rastered 1×/2×/3× |
| Radius | 2 px (cap 3) |
| Motion | none in QSS; cap 150 ms |
| Grid | minor ≥ 8 px, major = 10 × minor |

## 3. Code map

| Concern | File |
|---|---|
| Tokens, palette, semantic map, `formatValue`, hairline/grid helpers | `src/editor/ui_design_tokens.hpp` |
| QSS (`editorStyleSheet`), icons, `applyEditorStyle` | `src/shell/editor_style.cpp` |
| Canvas colour functions | `src/editor/canvas_style.hpp` |
| Two-tier snapped grid | `src/editor/canvas.cpp` (`drawBackground`) |
| Overlays moved to roles | `canvas_conflicts/connectors/counters/demand/feedback/heads/lanes/rotation/run/spatial.cpp` |
| Group-label fonts | `src/shell/editor_window.cpp`, `src/shell/editor_palette.cpp` |
| Locale formatting | `src/shell/editor_window.cpp`, `editor_connectors.cpp`, `editor_files.cpp`, `editor_inspector.cpp` |
| Signal view semantic colour | `src/shell/signal_timing_view.cpp` |

## 4. Before / after

| Before | After | Why |
|---|---|---|
| Hex in QSS and canvas | Palette roles everywhere | One source of truth (rule 3); a theme change is one function |
| Teal accent; orange/violet/magenta tool colours | One blue accent for every active/preview state | Colour then means one thing; hue is left for error/warning/advisory/ok |
| Signal/conflict red-amber-green literals | `semantic()` roles | Same reservation, one definition |
| 30 px controls that grew on focus | 28 px, constant through focus/invalid | Rows align; no layout jitter while typing |
| 20 px icons, 1.6 stroke | 16 px, 1.5 stroke | Brief; denser toolbar (32 px) |
| Inert `letter-spacing` QSS | `QFont` tracking + capitalisation | The property Qt actually honours |
| `QString::number` lengths | `QLocale` + fixed decimals + unit | Separator follows the user; digits align |
| 2-device-px, unsnapped grid line | 1-device-px, pixel-centre-snapped lines | Crisp at any devicePixelRatio |
| 20 px single-tier grid | 8 px minor / ×10 major from LOD | Reads scale at every zoom |

## 5. Regression tests

`tests/design_system_ui_tests.cpp` → CTest `design-system-ui` (offscreen):

1. Accent is `#2F6FED`; only accent + semantic roles carry hue; text roles ≥ 4.5:1 on `Base` and `Midlight`; text on accent ≥ 4.5:1; outline ≥ 3:1.
2. QSS: no hex literal, no gradient/shadow/glow/transition/animation/letter-spacing/rgb(); every `font-size` ∈ {11,12,13,14,18}; every radius ≤ 3; spacing tokens multiples of 4.
3. Box model: `QLineEdit`, `QDoubleSpinBox`, `QComboBox`, `QPushButton` `sizeHint().height()==28`, and still 28 when `validationState=invalid`; toolbar 32; icon raster ≥ 16.
4. Typography: numeric face fixed-pitch 12 px; English group label 11 px, uppercase, absolute tracking; Thai label untracked.
5. `formatValue` in German/US locales, fixed decimals, no-break unit.
6. Hairlines: cosmetic, 1/dpr wide; snapped coordinate lands on `k + 0.5` device px for positive/negative scale and any offset.
7. Grid tiers: LOD of a flipped 4× view is 4; minor jumps to 10 m at 4 px/m; ≥ 8 px at any zoom; bad input yields no grid.
8. Empty canvas render: every pixel is exactly `Base`, `Midlight` or `Mid` (a blended pixel means a line straddled two device pixels).
9. `EditorWindow`: labels keep buddies that can take focus; no `QAbstractAnimation` exists; the window carries the editor palette.

Existing suites keep covering tab order, Ctrl+K, range hints ("1–12", "> 0 m"), toolbar single-row and shortcuts.

## 6. Known gaps (not done)

- **Dock titles are uppercase 11 px but untracked**: `QDockWidget::title` has no font-spacing hook, and a custom title-bar widget would lose native drag/float.
- **Tabular figures rely on the monospace face**, not an OpenType `tnum` switch (`QFont::setFeature` is Qt 6.7; the floor is 6.4).
- **Not every table value is `formatValue`d**: per-cell `QString::number(…,'f',n)` remains in results, counters and the signal table (fixed decimals, right-aligned, but C-locale separators, and no unit in the cell — the column header carries it).
- Windows/high-DPI visual review is still the owner's; Linux offscreen cannot show fractional scaling.
