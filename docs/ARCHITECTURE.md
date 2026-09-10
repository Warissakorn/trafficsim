# Architecture — TrafficSim

The map of this codebase. Read before adding a system; update when the map changes.

## Shape

One paragraph: what the major pieces are and how data flows between them, in plain language. Someone who reads only this should be able to guess where a new feature goes.

TODO: shape

## Systems

| System | Owns | Location | Talks to |
|---|---|---|---|
| | | | |

Fill a row when a system is built, not when it's planned. This table describes what exists.

## Interfaces that matter

The contracts other systems code against. When one changes, everything calling it needs checking — so record them here rather than making a future session read the implementation.

```
# e.g.
# combat.apply_damage(world, target_id, amount) -> DamageResult
# save.write(world, slot: int) -> bool
```

## Data layer

Where content lives and what its shape is. A future session adding the 50th enemy should be able to do it from this section alone.

- `data/` — typed content modules or JSON validated against a schema

## Deliberate non-goals

Things intentionally not built, so a later session doesn't add them by reflex.

-
