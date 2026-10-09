# Level of service from section delay (M5.5)

How a letter is put on a number. Decisions: [D130](../decisions/RECORD.md#d130) (O3 (b): LOS
from section-bounded delay, HCM pack first) and [D134](../decisions/RECORD.md#d134) (this
contract). The delay is [travel-time section](TRAVEL_TIME_SECTIONS.md) delay; whole-route
movement delay (D39) never gets a letter. Every letter is labelled **simulated section delay, not
HCM control delay, not validated (M6)** (rule 4).

## 1. The pack (content, rule 5)

`data/los/hcm.json`:

```json
{ "id": "hcm", "description": "...",
  "controlTypes": { "signalised":   [10, 20, 35, 55, 80],
                    "unsignalised": [10, 15, 25, 35, 50] } }
```

- Five upper bounds in seconds, for A–E; above the last is F. HCM 6th edition's control-delay
  bounds for signalised and for two-way/all-way stop-controlled intersections.
- A delay **equal** to a bound takes the better letter: 10.0 s is A, 10.01 s is B.
- Loading refuses (`EDIT_CATALOG_READ`) a missing file, a missing `id`, a missing type, a list
  that is not five finite, positive, strictly increasing numbers, or any other key. Both types
  must be present, because a section can name either.
- HCM's rule that v/c > 1 is F whatever the delay is **not** applied: the simulation reports no
  v/c. A saturated movement shows instead as unfinished trips (D132), which the output keeps.
- The pack is read only when some section has a control type, so other projects' input manifests
  and bytes do not change. Its `id` is written next to every letter.

## 2. The control type (schema 24)

`network.travelTimeSections[i].controlType`: `"signalised"` or `"unsignalised"`, written only when
the author set one. A file with any is schema 24; an older file carrying the key is refused
(`EDIT_UNSUPPORTED_FIELD`), another value is `INVALID_ENUM`. A section without one gets no
letter. W74's schema moves to 25.

## 3. Letters and rows

`losLetter(delay, controlType, pack)` is pure: the first bound the delay does not exceed, else F.

| Row | Delay | Letter |
|---|---|---|
| Section | its mean section delay | by its control type |
| Approach | sections with a control type grouped by (start Link, control type), weighted by vehicles: Σ vᵢdᵢ / Σ vᵢ | by that type |
| Intersection | every section of one control type, weighted the same way | by that type |

- Sections with no vehicle (no delay) are left out of the weights; a group with no vehicle has no
  delay and no letter.
- An approach is named after its start Link (Name, else id). The author owns what sections
  overlap; two sections that share trips both count.
- Batches letter the mean over seeds (`meanDelay.mean`) and weight by the mean vehicles. The
  95 % half-width stays beside the delay; no letter is given to an interval bound.

## 4. Output

- Section rows (JSON and both CSV blocks) gain `controlType` and `los` (empty without a type).
- With any letter, JSON gains `los {pack, approaches[], intersections[]}` (each `{name,
  controlType, vehicles, delay, los}`), and CSV gains, after the section block, the line
  `# LOS (pack <id>) from simulated section delay, not HCM control delay; not validated (M6)`
  and a block `level,name,controlType,vehicles,delay_s,los`.
- Projects without a control type write exactly what M5.4 wrote, apart from the two empty columns
  in a section block.

## 5. Editor

The *Travel-time sections* tab gains a Control column; its edit dialog sets the control type
(none, signalised, unsignalised) with the name, in one History step. Since M5.6 (D141) the Results tab's batch table shows each section's letter
(`losCell` on the mean over seeds), with the pack id in the note ([BATCH](BATCH.md) §6).

## 6. Acceptance rows

| Row | Fixture | Expected |
|---|---|---|
| L1 | Each bound of both types | Equal to the bound: the better letter; 0.01 s above: the next; above E's bound: F; 0 s: A |
| L2 | Swapped pack (test data directory) | The same delays get different letters with no code change |
| L3 | Bad packs | Missing type, four bounds, non-increasing, negative, NaN, extra key: `EDIT_CATALOG_READ` |
| L4 | Weighting by hand | Approach and intersection delays equal Σ vᵢdᵢ / Σ vᵢ; a section without vehicles or type is left out |
| L5 | Codec | Schema-24 round trip; no type keeps schema 23 bytes; schema 23 with the key and an unknown value are refused |
| L6 | Outputs | Single-run and batch letters and the marker line; no type means no `los` key, no pack read |
| L7 | Editor | Setting the type is one Undo step and survives save/reopen |
