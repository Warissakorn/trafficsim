# Supplied specifications: implementation audit

Reviewed against `9066b08b4b0281aa7afe46dd676ff8b25056724f` on 2026-09-20.
The three supplied Thai documents describe a target, not measured Vissim parity.
Section numbers below refer to those documents (see [specs/README.md](specs/README.md)).
The copies are verbatim except that, on the owner's instruction of 2026-09-24 (D38), one
import-format entry was removed from §18.1 and from the part-04 comparison table.
`Partial` means some contracts exist; it never means the entire section is implemented.

## Link

| Sections | Baseline status and evidence | Remaining work |
|---|---|---|
| 1–2: directed authoring object | Partial: `network.hpp` has IDs, names, geometry, lanes, level, display type | Road classes, attachments, behavior, elevation; retain one authored source |
| 3: geometry/stations | Partial: `geometry.cpp`, `split_link.cpp`; reference stations in metres | Named straighten/insert/midpoint/reverse operations; spline/arc/elevation/extend/merge |
| 4: lanes | Partial: lane IDs/widths and contiguous connector ranges; fixed-edge resize | Lane types, restrictions, authored taper, markings; lane IDs are globally unique |
| 5–6: behavior/lane changing | Absent at Link level; global driving side exists | Distributions, inheritance, per-Link overrides and actual lane-changing engine |
| 7: hierarchy | Absent | Data-driven classes and routing/priority interpretation |
| 8: detectors/DCP | Absent | Authoring, events, aggregation and outputs |
| 9: signals | Partial: heads on lanes/paths, fixed-time programs, rebased section positions | Groups, controllers, actuated/adaptive control |
| 10: routing/parking/transit | Partial: explicit static routes and typed inputs in `definition.hpp` | Decision stations, proportions, compositions, parking and transit |
| 11: nodes/stop lines/crosswalks | Absent as authored objects | Evaluation-node contract and pedestrian interactions |
| 12: cross-section | Partial: lane widths/offsets and derived boundaries | Authored markings, shoulders, medians, sidewalks |
| 13–14: editor/commands | Partial: creation, split/pocket/opposite, reference cleanup, History | Remaining geometry/attachment commands; node commands require node model |
| 15: schema | Schema 6 in `document.cpp`, migration in `parse.cpp` | Incremental versioned extensions, not wholesale ingestion of the example |
| 16: runtime | Partial: `sections.cpp`, `compile.cpp`, signal/route rebasing | Detector semantics, behavior inheritance and multimodal runtime |
| 17: validation | Partial: finite geometry, unique IDs, references | Imported connector width/marking lists bypass command checks; stricter contracts |
| 18: visualization | Partial: QGraphicsScene/QPainter, named display styles and levels | Authored markings, density/speed overlays, measured LOD |
| 19–21: roadmap/examples | Not implementation evidence | Freeway/roundabout examples require more M3 right-of-way capability |

## Connector

| Sections | Baseline status and evidence | Remaining work |
|---|---|---|
| 1–4: object/curve/mapping | Substantially implemented: `connector_geometry.cpp`, `connector_paths.cpp` | Authored spline parameters/elevation; arbitrary topology remains excluded |
| 5–6: behavior/lane-change/emergency | Absent per Connector | Implement with engine contracts; do not silently delete blocked vehicles |
| 7: priority/conflicts | Partial: deterministic interior-arrival merge rules in `sections.cpp` | Crossing conflicts, explicit priority controls and calibration |
| 8: signal integration | Partial: path-mounted heads with fixed-time program references | Group/controller model and actuated/adaptive control |
| 9: routing | Partial: routes name stable path IDs; reference-safe edits | Probabilistic routing decisions, partial/dynamic routes |
| 10: widths/markings/mouth | Partial: authored widths and solid/dashed dividers; M1.19 Link-width mouths | Missing load-time list/value checks, additional marking kinds, authored mouth parameters |
| 11–12: editor/commands | Implemented for existing properties: inspector, handles, attach/detach, cleanup | New commands depend on new model/runtime fields |
| 13: schema | Schema 6, optional widths/markings | Versioned extension; unsupported fields must not silently disappear |
| 14: runtime | Implemented for existing routes, residual travel, sections and interior merges | Per-Connector behavior and conflict runtime |
| 15: validation | Partial: existing codes and curvature advisory | Import validation, distinct warning severity, short-Connector advisory |
| 16: visualization | Partial: render, select, level/display, lane markings | Full per-object visual overrides, LOD and markers |
| 17–18: roadmap/source map | Future source filenames are proposals | Do not describe nonexistent modules as delivered |

## Network Editor

| Sections | Baseline status and evidence | Remaining work |
|---|---|---|
| 1–3: architecture | Implemented native model/commands/project/canvas/shell boundaries | Spec file/class names are illustrative, not required renames |
| 4: rendering | Partial: `canvas.cpp`, `network_view.cpp`, levels/styles | No measured GPU renderer, large-network culling or LOD |
| 5–6: coordinates/camera | Partial: metric grid, pointer zoom, pan/fit, calibrated background | CRS/georeferencing and animated navigation |
| 7: tools | Partial: select/draw/split/measure/calibrate/connect/route/input/head | Detector, node and other new-object tools need their models |
| 8: selection | Partial: Ctrl selection, band select, overlap cycling, multi-delete/copy/move | Explicit filters and configurable cascade selection |
| 9: snapping | Partial: grid and lane attachment picking | Unified priority snap engine, angle/alignment/guideline constraints |
| 10–11: drag/handles | Partial: transient previews, cancel, group translation, range/point handles | Rotation, keyboard nudging, additional constraints |
| 12: history | Partial: atomic before/after snapshots, 100 entries, recovery | History panel and navigation; avoid rewriting working transaction model |
| 13: inspector | Partial: names, widths, levels, attachment stations and ranges | New properties, bulk property editing and geometry actions |
| 14: layers | Partial: level filtering and background visibility | Lock/reorder/opacity per object layer |
| 15: picking | Partial: hit shapes/tolerance and Tab overlap cycling | Explicit spatial index and hover feedback |
| 16: diagnostics | Partial: structured object-linked findings, on-demand recheck | Distinct advisories; incremental real-time validation needs profiling |
| 17: menus/shortcuts | Partial: native actions and fixed keyboard shortcuts | Context menus and customization without gesture collisions |
| 18: interchange | Native projects, scenarios and background images only | GeoJSON/OSM/Shapefile, coordinate conversion and exports |
| 19: projects | Partial: per-window document, save/reopen, recovery locks | Tabs/recent files/project settings; native JSON remains the source |
| 20: performance | Unproven targets | Real-network benchmark before claiming 10k/100k-object performance |
| 21: accessibility/i18n | Partial: English/Thai catalog, buddies and native widgets | Keyboard-only/accessibility audit, locale-number handling |
| 22: testing | Native model, command and Qt UI tests, Linux/Windows CI | New-feature tests, visual regression/fuzzing and manual acceptance |
| 23–24: source map/use cases | Illustrative | OSM workflow and full freeway merge cannot be claimed today |

## Conflicts and implementation decisions

1. Keep current project and scenario formats separate. Standalone `link`/`connector` JSON
   snippets are not complete TrafficSim projects. A version number alone is not capability.
2. Preserve schema 1–6 data. The Connector document's compatibility summary incorrectly
   resets all schema 1–5 ranges/levels; existing migrations must retain authored values.
3. Keep lane ordering from `laneGeometry` and existing tests. The Link document alternates
   between left-to-right and driving-side ordering and incorrectly lists Thailand under
   right-hand traffic. No automatic lane reordering or driving-side migration is justified.
4. Keep Link widths at Connector mouths (M1.19); authored Connector widths govern the body.
   The simple authored-width-first table does not supersede the measured mouth contract.
5. The first Connector path is the Connector ID, not `id/lane-1`; later paths use `/lane-2`,
   etc. Routes name lane/path IDs, not Link IDs. Do not invent aliases that break references.
6. A shared lane boundary needs one marking value, not conflicting left/right copies on two
   lanes. Use `Link::boundaryMarkings` in lane order (N+1 values, or empty for defaults).
   The UI labels this order explicitly. Physical left/right depends on travel/driving side.
7. Per-Link desired speed contradicts `PROBLEM.md`'s vehicle-owned desired speed. Resolve
   limits/factors versus distribution ownership before implementing runtime overrides.
8. Automatic crossing conflicts are both promised (Connector §7) and excluded (§17).
   Keep current merge guards; book crossing conflicts explicitly in M3.
9. A signal must not erase physical merge/collision safety. Signal-overrides-priority wording
   needs an explicit conflict policy and tests, not unconditional bypass of gap checks.
10. Removing blocked vehicles after 30 s would bias delay/throughput results. Any future
    removal policy needs explicit event accounting and opt-in semantics.
11. Maintain current shortcuts and QPainter/Qt architecture. GPU, 3D, collaborative editing,
    nodes and competitor imports are not prerequisites for correcting authoring defects.
12. Reversing a referenced road needs a topology/routing redesign. The initial reverse action
    applies only to unreferenced Links and rejects the rest atomically with a clear message.

## Delivery sequence and acceptance

`CLAUDE.md` requires one system per session. This change handles the authoring system first;
it does not label all three target specifications implemented. Numbered follow-ups in
[ROADMAP.md](ROADMAP.md) retain every remaining feature group.

- **M1.21:** close authoring validation holes; Link geometry actions and shared road markings;
  persistence, command rollback/Undo/Redo, both renderers and bilingual inspector; advisories.
- **M2.1:** agree behavior ownership; distributions, road/lane classes, inheritance and demand.
- **M3.2:** lane changing, crossing conflicts, explicit right-of-way and blocked-vehicle policy.
- **M4.1:** detectors/DCP, groups/controllers, actuated/adaptive signals and external interfaces.
- **M1.22:** remaining geometry, snapping, transforms, layers/history, inspector and accessibility.
- **M1.23:** interchange, CRS, multi-document workflow and measured large-network rendering.
- **M5.1:** nodes, evaluation/overlays, parking/transit/multimodal behavior and reporting contracts.

Each needs model → commands/codec → UI → runtime (where applicable) → regression tests.
M0/M1 owner acceptance and M6 scientific validation remain open regardless of CI results.

## Result of this change

M1.21 is implemented in [AUTHORING_EXTENSIONS.md](AUTHORING_EXTENSIONS.md), with ten new
model/command cases and an inspector/render/persistence UI suite. Local Linux verification:
18/18 headless and 26/26 desktop CTest pass; the frozen replay fixtures and seed-42 diagnostic
remain unchanged. The tables above intentionally describe the audited baseline, so the new
features can be compared with it. The remaining numbered milestones are still open.
