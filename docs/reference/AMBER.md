# Amber stop-or-go (M4.2, D147)

Until M4.2 every signal head that was not green held a vehicle at its line (D36), so a driver who
could not stop when amber began was stopped anyway, and the engine reported a safety clamp. This
contract replaces that for projects. The model is a deterministic threshold, **not validated**
(M6): no probability model, and no calibration to local drivers.

## 1. Rule

`DriverBehaviour.amberDeceleration` is optional, in m/s², positive and finite.

| Head colour at the start of the tick | Behaviour without `amberDeceleration` | With it |
|---|---|---|
| Green | No hold | No hold |
| Amber | Held at the line, as red (D36) | **Goes** when `speed² > 2 · amberDeceleration · gap`, where `gap ≥ 0` is the distance from the front to the line, or when it cannot stop at all (below); otherwise held at the line |
| Red | Held at the line | Held at the line, unless it **cannot stop at all**: `speed² > 2 · maxDeceleration · gap` with the vehicle type's maximum, the commitment rule priority rules already use (M3.2.8a) |

- **Continuous check.** The test runs on every tick the head is amber. It needs no state on the
  vehicle, and is the default of Vissim's "Continuous check" in spirit, though not its
  implementation. A vehicle that started braking for amber and can still stop keeps stopping; one
  that can no longer stop at that deceleration goes.
- **Boundaries.** Equality stops. A standing vehicle (`speed = 0`) always stops. A front already
  past the line (`gap < 0`) is not held by that head, as before.
- **Models.** The check reads the vehicle's effective behaviour (the road/class selection of D127)
  and applies to every following model, the prototype and W74 alike. Following, conflict areas,
  priority rules and the D108 insertion guard are unchanged.
- **Red holds everyone who can stop.** A vehicle that went on amber and is still short of the line
  when red begins goes on only if it cannot stop even at its type's `maxDeceleration`. That is
  typically a fraction of a metre at speed, crossed within a tick. Before this rule the engine
  forced that impossible stop and reported a safety clamp; the owner chose commitment on
  2026-10-09, after the first measurement left 44 such clamps in 40 four-leg seeds. Every other
  vehicle is held at red, and a clamp still means the engine stopped one harder than its type
  allows. Deliberate red-light running is not modelled.
- **Intergreen.** A fixed-time Signal Controller expands each group to green, amber, red (M2.7b), so
  the all-red time is the red after amber. An amber anywhere in a program follows the rule above; a
  red → amber → green program keeps its standing queue, because a standing vehicle stops. An
  intergreen matrix is M4.1.

## 2. Where the value comes from

- **Catalog.** `data/driver-behaviour/default.json` carries `"amberDeceleration": 3`. The 3.0 m/s²
  is the 10 ft/s² deceleration of the ITE *Guidelines for Determining Traffic Signal Change and
  Clearance Intervals* (2020), used there to size the yellow interval. Every project that uses the
  catalog behaviours therefore decides at amber.
- **Owned behaviours.** A project that owns its behaviours (embedded catalogs, schema 18; the
  library, schema 21) keeps exactly what it owns. The author adds the field in the behaviour dialog.
  An owned behaviour carrying it makes the file **schema 28**; the key is refused below 28.
- **M0 legacy.** A bare M0 scenario (no `schemaVersion`; the file kind behind the frozen TS
  baselines) resolves the catalog behaviours **without** `amberDeceleration`, whether `loadScenario`
  reads it (CLI) or `parseDocument` opens it (editor). The same precedent applies as for its signal
  programs: the frozen fixtures, `trafficsim-cli 42` and the editor's run of `crossing.json` cannot
  move. An M0 scenario saved as a project becomes a project and takes the project rule.

## 3. Acceptance rows

| Row | Check | Test |
|---|---|---|
| AM1 | Stop: a vehicle with `speed² < 2·3·gap` at amber onset ends short of the line and is not clamped; the head is asserted amber first | `amber` |
| AM2 | Go: with `speed² > 2·3·gap` it crosses during amber, unclamped; the same vehicle without the field is first shown held | `amber` |
| AM3 | Equality stops; a standing vehicle at the line stays | `amber` |
| AM4 | Red: without the field a vehicle that cannot stop is held and clamped; with it, it goes, unclamped; one that can stop at its maximum deceleration is still held | `amber` |
| AM5 | A W74 behaviour with the field decides the same way | `amber` |
| AM6 | Codec: an owned behaviour round-trips at schema 28; a file without it keeps its schema and bytes; the key below 28, zero, negative and non-finite values are refused; the catalog reads 3.0 | `amber` |
| AM7 | M0 legacy: `crossing.json` resolves without the field through both readers; the frozen TS fixtures, `cli-*` and `scenario-run-ui` pass unchanged | `amber`, `reference`, `scenario-run-ui` |
| AM8 | Editor: the behaviour dialog shows the captured value and edits it in the library's one History step | `behaviour-library-ui` |

The gate's before/after over 40 seeds and its same-build replay are recorded in
[evidence](../evidence/m4.2-amber.md).
