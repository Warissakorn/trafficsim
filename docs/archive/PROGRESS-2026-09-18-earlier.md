# PROGRESS archive — 2026-09-18, earlier entries

Moved whole out of `docs/PROGRESS.md` when it passed the 500-line guard: the Vissim snapping
audit, the attachment-station snap, the engine profile, and the M1.17 revert.

---

## 2026-09-18 — Snapping audited against Vissim: one gap closed, one instruction refused with numbers

The owner listed Vissim's four snaps and asked for all of them, opening with **"Vissim does not snap
to a Link end."** Audited against live code, the full table is in
[`VISSIM_PARITY.md`](VISSIM_PARITY.md). The short version: **one of the four was a snap gap and is
now closed; two are interactions we do not have; one is a file format we do not read.**

**Snap to Points is now complete.** `hitLanePosition` tries, in order, the lane's two endpoints, a
station another Connector already attaches at, then **the Link's own intermediate points** — the
new part. So a Connector meeting a Link at a bend lands on the bend. The vertex station is
accumulated with `polylineLength`'s own order and its own `hypot`, so the value stored is the one
measuring that prefix produces: the test asserts it with `==`, not a tolerance.

**The opening instruction was refused, and only a measurement earns that.** In our model an
attachment at a Link end is a distinct stored state — `station` absent — that `runtimeSections`
compiles to a departure rather than a cut. On a 50 m Link, drawn short of the end by:

| short by | station | result |
|---|---|---|
| 0.00 m | *(absent)* | one section, the end attachment drawn |
| 0.03 m | 49.9700 | **`unsectionable`, cannot Run** |
| 0.15 m | 49.8500 | **`unsectionable`, cannot Run** |
| 0.25 m | 49.7500 | runs, as a body attachment with a 0.25 m stub |

"Place it carefully by hand" is precisely the 9 mm / 5.8 cm failure measured on the owner's own
four-leg drawing the day before. Also: the owner's **own** *Snap to Points* item lists the End
point, so the opening line contradicts item 2 of the same list. Keeping the endpoint snap is the
Vissim-matching choice, not the deviation.

**Two of the four are not snapping at all**, and calling them snapping would have hidden their
size: heads, vehicle inputs and routes are **never placed by pointer** (`canvas_input.cpp:16-20`
takes `nearestLane()` and opens a dialog), and stop signs and PT stops are not object types —
§6 item 5, still blocked on engine behaviour. DWG/DXF snapping needs a vector import path where
`BackgroundImage` holds a base64 PNG: a milestone, deliberately **not booked**, no done-condition
written (hard rule 8 — do not start it without one).

**Verification:** 23/23 CTest on Qt 6.4.2 under `xvfb`, `trafficsim-cli 42` unchanged at
`meanDelay 29.249359418430977`. Deleting the new branch fails the suite with `A pick beside a
Link's intermediate point did not take it`. Linux only.

### Next

Unchanged and still the only thing that closes M1: the owner's timed exercise in
[`M1_ACCEPTANCE.md`](M1_ACCEPTANCE.md). The three snap-adjacent items above are scoped and
**not** booked — if the owner wants pointer placement for heads and inputs, or CAD snapping, each
needs a numbered milestone with a done-condition before any code.

---

## 2026-09-18 — A pick snaps to a station another Connector already attaches at

**The owner's own four-leg drawing would not Run, and the drawing was not wrong — the numbers
in it were.** `runDiagnostics` on it reported `UNSUPPORTED_CONNECTOR_POSITION` twice, and
`compileDocument` threw on the same two. Measuring what actually collided:

| lane | keeps its boundary | refused | apart |
|---|---|---|---|
| `lane-3` | `connector-30` (2-lane range) @ 49.376709 | `connector-32` @ 49.385893 | **0.009 m** |
| `lane-22` | `connector-46` @ 2.963768 | `connector-38` (2-lane range) @ 3.021435 | **0.058 m** |

Both pairs are two turning movements leaving or joining **one corner**, authored as separate
Connectors because they have different destinations — which is correct modelling. They differ
only by what a hand does with a mouse: 9 mm and 5.8 cm. `kMinSectionLength` is 0.2 m, so
`sections.cpp` rejects the second cut of each pair and `connectorRuntimeIssues` names its owner.
**The core is right and the guard is right**; nothing in `src/model` or `src/core` changed.

**The fix is at the point the mismatch is introduced.** `hitLanePosition` already snapped a pick
to a lane's two ends within `4/zoom` screen units; it now also takes the exact station of an
attachment already on that lane within the same radius. One branch, one file-local helper, in a
function both the two-click tool and the right-button range gesture already go through
(`canvas_input.cpp:23` and `:154`), so both ends of both workflows are covered by the one change.

**What it does not cover, stated rather than implied.** The radius is a screen distance, so past
roughly 20 pixels per metre it falls below `kMinSectionLength` itself and stops standing between
an author and a sub-0.2 m mistake. That is deliberate — an author zoomed that far in is asking
for fine placement — but it means the snap **reduces** this class of error, it does not make it
impossible. `UNSUPPORTED_CONNECTOR_POSITION` is still the backstop and still has to be.

**The test asserts its forcing first.** In `attachment_ui_tests.cpp`: the second pick really is a
distinct point, really is inside the radius, and the third really is outside it — all three
asserted before the equality, because without them the equality could hold vacuously. It then
asserts the near pick takes the first Connector's station **exactly** (`==` on the double, which
is meaningful because `attachmentStation` returns `*ref.station` verbatim), and that the far pick
**keeps its own** — a snap that always pulled to the nearest attachment would make two genuinely
distinct attachments unauthorable, and that is the failure this guards. Checked by deleting the
branch: the suite fails with `A pick inside the snap radius kept its own station`.

**Verification:** 23/23 CTest, architecture and size guards green, on a real Qt build (Qt 6.4.2
from the distribution) under `xvfb`. Linux only — `native.yml` also runs these suites on Windows
and nothing here has been near it.

### Next

The M1 position is unchanged: every sub-milestone is implemented and **only the owner's timed
exercise in `docs/M1_ACCEPTANCE.md` closes it.** Carry the file above into that exercise — it is
a real four-leg drawing that exercised a real authoring trap, and re-drawing those two corners on
a build with this snap is the cheapest check that the snap earns its place.

Two follow-ups were scoped and deliberately **not** done, so a later session does not re-derive
them:

1. **Name the conflict in the diagnostic.** Today `UNSUPPORTED_CONNECTOR_POSITION` says "move it
   0.2 m clear" without saying clear of *what* or by *how much* — I had to measure the table above
   with a throwaway tool. The enrichment belongs on `Diagnostic` (`diagnostics.hpp:8`), never on
   `core`'s `ValidationIssue` (D18a), but the data it needs is **discarded upstream**:
   `sections.cpp:66-71` keeps only the rejected Connector's own id, not the cut it lost to. So it
   is `sections.cpp` + `compile.cpp` + `diagnostics.{hpp,cpp}` — its own session, not an add-on.
2. **A station field in the inspector.** There is none (`editor_inspector.cpp`), so a station can
   only be set by dragging. Worth having for exact after-the-fact repair, but it is a new field,
   not a button.
---

## 2026-09-18 — An engine profile at scale: three measured optimizations, no behaviour change

**None of this is M1 work.** M1's engineering side is still done and the owner's timed gate in
`docs/M1_ACCEPTANCE.md` is still the only open item. This session profiled the engine and took
three wins that leave the trajectory byte-identical.

**The shipped scenario is too small to profile.** `crossing.json` is 31 trips in 180 s (57 ms), so a
12-intersection corridor at 900 s and a 40-intersection one at 120 s were generated by replicating
its validated pattern. Profiling a toy input finds toy problems.

**The profile said `std::string`, not simulation maths.** Of 1.55 G instructions, ~36% was string
handling: `memcmp` 9.6%, copy-ctor 5.8%, move-assign 5.2%, `operator==` 4.7%, plus dispose/traits.
Inclusive: `resolveRefs` 26.4%, per-tick copies 21.3%, the `Vehicle` sort 11.8%.

| # | change | 12-route | 80-route |
|---|---|---|---|
| 1 | `resolveRefs` through sorted id tables in `ScenarioIndex` | −6.2% | −33.5% |
| 2 | stop copying the vehicle list twice per tick | −5.6% | −6.1% |
| 3 | precompile `nlohmann/json` (build, not runtime) | −16% CPU | — |

Cumulative engine: **−11.5%** on the corridor, **−37.5%** on the 80-route network.

**Two things were predicted wrong and are worth remembering.** Item 1 was estimated at 25% and
delivered 6.2% on the corridor: the cost was never search asymptotics but the string comparison
itself, and with 12 routes and one vehicle type a binary search replaces ~6 compares with ~4. It
pays at 80 routes, which is the size that matters for a real study. And the first implementation
used `unordered_map`; `tools/check_architecture.cpp` rejected it immediately, correctly — core/ may
not use unordered containers at all, because their unspecified iteration order would leave
reproducibility to the standard library. Sorted vectors with `lower_bound` were the answer, with
ties ordered by position so a repeated id still resolves to its first occurrence.

**`trafficsim_shell` did not get the precompiled header.** It is the largest target at 23 sources
and would gain the most, but Qt is not installed here, so it was left alone rather than changed
without being compiled once.

**A clean checkout does not configure**: `nlohmann/json` is not found and CMake fails. `docs/BUILDING.md`
does list the dependency, so this may be the intended manual step, but the failure does not name the
package.

### Next

**Unchanged: the owner's timed four-leg / aerial-image / reopen exercise in `docs/M1_ACCEPTANCE.md`
is the only thing standing between here and M1.**

Two optimizations are diagnosed, measured and deliberately NOT done, in priority order:

1. **The precompiled header on `trafficsim_shell`** — one line, mirroring the three targets in
   `CMakeLists.txt`, but it needs a Qt-enabled machine to compile once before it is pushed.
2. **Take the three `std::string` ids off `Vehicle`.** This is the real fix for a small network, and
   it is what items 2 and 4 of the plan cannot reach: make a vehicle's identity its INDEX and derive
   `routeId`/`vehicleTypeId`/`inputId` from the scenario at the few points that need the name (event
   construction). That removes the per-tick compare, the per-tick copy and the 11.8% sort cost at
   once. It changes a core public type and every test that hand-builds a `Vehicle`
   (`tests/test.hpp:46`, `tests/core_tests.cpp:194`, `tests/attachment_tests.cpp:195`), so it is one
   system for one session, not a tweak.

The `std::sort` of vehicles by id (11.8%) was left alone on purpose: the element order it guarantees
is what makes a run reproducible, and it is not worth touching before the strings are gone.

*Verified on Linux, headless preset only (Qt not installed here); no desktop verification claimed.*

---
