# TrafficSim precision editor UI redesign

The editor now follows a compact CAD/EDA workspace: command/tool navigation at the left, a central network canvas, a right-side inspector, dockable object tables, and a persistent status bar. The application remains available in English and Thai.

## 1. UI audit

| Area | Before | Change | Result |
|---|---|---|---|
| App shell | Controls and panels competed with the network view | Compact toolbars, grouped tool tree, central canvas, inspector, dockable tables, status bar | Clear work areas and predictable navigation |
| Tool selection | Flat tool list with weak grouping and inconsistent feedback | Geometry, Controls, and Tools groups; selected and hover states share the accent; one-key shortcuts | Faster scanning and direct keyboard access |
| Object selection | Selection and hover hierarchy could be hard to distinguish | Neutral hover surfaces, consistent accent selection, visible focus, synchronized table/canvas selection | Current target and keyboard focus stay legible |
| Inspector | Dense forms lacked clear range feedback | 28 px controls, aligned labels, fixed-width numeric fields, inline range hints and invalid borders | Invalid values explain the allowed range before apply |
| Tables and readouts | IDs, measures, and labels shared the same visual treatment | Compact 28 px rows, hairline dividers, alternating neutral rows, monospace IDs and measurements | Values scan and compare more reliably |
| Canvas overlays | Animated flashes and moving dashes pulled attention from geometry | Static route and draft overlays; neutral geometry handles and a single accent for active editing | Geometry stays visually calm while edit state remains apparent |
| Keyboard access | Important commands required toolbar/menu discovery | Explicit tab order, visible 2 px focus ring, one-key tools, Ctrl/⌘+K searchable command palette | Common actions work without leaving the keyboard |
| Status and validation | Status relied on color or terse errors | Text labels plus semantic status colors; inline valid ranges; accessible descriptions | Meaning remains available without color perception |
| Density and styling | Larger spacing, strong panel framing, and varied emphasis | 4 px spacing increments, 1 px dividers, no shadows/gradients/glow, five type sizes | More canvas space and a consistent engineering-tool visual language |

## 2. Design tokens

### Color

| Token | Value | Use |
|---|---|---|
| Gray 0 | `#FFFFFF` | Canvas and input surfaces |
| Gray 1 | `#F9FAFB` | Panels and toolbar surfaces |
| Gray 2 | `#F3F4F6` | Hover, disabled, and subtle selection surfaces |
| Gray 3 | `#E5E7EB` | Hairline dividers |
| Gray 4 | `#D1D5DB` | Control borders |
| Gray 5 | `#9CA3AF` | Quiet decorative marks |
| Gray 6 | `#6B7280` | Secondary outlines |
| Gray 7 | `#4B5563` | Secondary text |
| Gray 8 | `#374151` | Strong secondary text and table headings |
| Gray 9 | `#111827` | Primary text and IDs |
| Accent | `#0F766E` | Selection, focus, and active tool state |
| Danger | `#B42318` | Invalid input and error status only |
| Warning | `#8A4B08` | Warning status only |
| Success | `#176B44` | Successful status only |

The ten grays are the UI neutral scale. The teal is the only interactive accent. Red, amber, and green are reserved for semantic status. User-defined road/display colors remain model data and are not used to encode editor state.

### Type, spacing, and sizing

| Token | Value |
|---|---|
| Spacing | 4 / 8 / 12 / 16 / 20 / 24 px |
| Type sizes | 11 / 12 / 13 / 14 / 18 px |
| Uppercase labels | 11 px, tracked in English; Thai labels remain untracked |
| Numeric text | System fixed-width font, fixed pitch, tabular figures |
| Standard control height | 28 px |
| Table row height | 28 px |
| Divider | 1 px |
| Keyboard focus | 2 px accent outline |
| Motion | No continuous canvas animation; state transitions are immediate or within 180 ms |

Contrast ratios for small text against the white and subtle-gray surfaces meet WCAG AA (4.5:1 minimum):

| Text/state color | On `#FFFFFF` | On `#F3F4F6` |
|---|---:|---:|
| Gray 7 | 7.56:1 | 6.87:1 |
| Gray 8 | 10.31:1 | 9.37:1 |
| Gray 9 | 17.74:1 | 16.12:1 |
| Accent | 5.47:1 | 4.97:1 |
| Danger | 6.57:1 | 5.97:1 |
| Warning | 6.79:1 | 6.17:1 |
| Success | 6.52:1 | 5.92:1 |

## 3. Full component code

The complete implementation is in the editor source tree. Shared tokens and the component entry points are:

| Component | Source |
|---|---|
| Shared palette, typography, density, and numeric helpers | `src/editor/ui_design_tokens.hpp` |
| Application style, controls, tables, and focus states | `src/shell/editor_style.cpp` |
| Workspace shell, toolbar, table sizing, focus order, and shortcuts | `src/shell/editor_workspace.cpp` |
| Tool tree, groups, one-key tools, level filter | `src/shell/editor_palette.cpp` |
| Searchable Ctrl/⌘+K command palette | `src/shell/editor_commands.cpp` |
| Selection, translation, focus, and validation refresh | `src/shell/editor_window.cpp` |
| Inspector controls and inline numeric validation | `src/shell/editor_inspector.cpp` |
| Connector properties and endpoint ranges | `src/shell/editor_connectors.cpp` |
| Canvas style and selection feedback | `src/editor/canvas_style.hpp`, `src/editor/canvas_feedback.cpp` |
| Stable route, draft, and geometry overlays | `src/editor/canvas_demand.cpp`, `src/editor/canvas_lanes.cpp` |
| Compact object and analysis tables | `src/shell/editor_tables.cpp`, `src/shell/editor_demand.cpp`, `src/shell/editor_results.cpp`, `src/shell/editor_diagnostics.cpp`, `src/shell/editor_priority.cpp`, `src/shell/editor_counters.cpp` |
| Signal timing/status component | `src/shell/signal_timing_view.hpp`, `src/shell/signal_timing_view.cpp`, `src/shell/editor_signal.cpp` |
| English and Thai labels | `data/locales/en.json`, `data/locales/th.json` |

## 4. Before / after rationale

| Before | After | Rationale |
|---|---|---|
| Flat navigation and scattered actions | Grouped tool tree, compact toolbar, command palette | Keeps the canvas central and gives tools stable locations |
| Hover, selection, and focus had overlapping emphasis | Neutral hover, teal selected/active state, explicit focus ring | Each interaction state has a distinct, repeatable signal |
| Numeric values used the proportional UI font | Fixed-width numerals for IDs, coordinates, units, and measurements | Makes digits and decimal values easier to compare |
| Invalid input feedback depended on applying the form | Inline message states the range/cardinality and highlights the field | Prevents avoidable apply/undo cycles |
| Large visual framing and animated canvas effects | Hairline separators and static geometry overlays | Reduces distraction and preserves visual precision |
| Mode changes and object selection were mouse-led | Explicit focus sequence, documented shortcuts, searchable commands | Supports keyboard-first editing while keeping controls accessible |
| Color alone carried some state | Color paired with text, field outline, and accessible description | State remains understandable without relying on hue |
