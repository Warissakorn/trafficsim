# Archived progress — continuous vehicle headings

Moved whole from PROGRESS on 2026-10-07. Current work lives in NEXT/ROADMAP.

## 2026-10-05 — Continuous vehicle headings across route segments

Owner authorized phase 1 of the vehicle-position audit: repair heading discontinuities
before introducing axle kinematics. The old Run view used a front tangent when local
station was below vehicle length, then switched to a chord inside the segment. On the
90-degree test curve that switched by 4.28 degrees for a 4.5 m car and 11.52 degrees for
a 12 m vehicle; exiting onto the straight target also switched to its tangent.

`vehicle_pose.*` now samples the complete ordered route at front distance and one type
length upstream, using the existing Link geometry and direct Connector equation. The
initial tangent extends behind route entry; coincident samples use a finite front
tangent fallback. Missing geometry has no drawable pose. Canvas shares per-frame route
parts with its lane-change slide; it keeps the true type length for heading at low zoom.
The new Qt-free target has headless geometry regressions, and the Qt suite checks actual
scene-item poses at both joins and the former length thresholds for both shipped types.
Both the extracted legacy formula and the original Canvas fail the new join regression.

This remains a display chord approximation. The upstream arc sample is not the actual
rear bumper or axle, and no wheelbase, overhang, steering or articulated trailer model
is introduced. Core state/events, runtime equations, schema and frozen fixtures are
unchanged. Linux GCC 13.3 / Qt 6.4.2 Debug: headless 56/56 and desktop 81/81 pass,
including architecture and file-size guards. Against main `3403b72`, crossing event logs
and reports match byte-for-byte for seeds 0, 42, 43 and 4294967295; seed-42 reports/CSVs
also match for four-leg-signalised, t-junction-priority, lane-change-lab and m2.6-study-template.
Windows CI is pending; owner appearance and axle-model work remain open in NEXT.
