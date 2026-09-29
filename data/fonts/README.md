# Bundled UI font

Noto Sans Thai, under the SIL Open Font License 1.1 (see OFL.txt). One face for Thai, Latin and
digits; its digits are tabular (all 572 units), so numbers align in columns without a second,
monospace face.

Two static weights, cut from the upstream variable font with fontTools (no other change to the
outlines), because weight axes are not honoured on every Qt version and platform this builds on:

| File | Weight |
|---|---|
| `NotoSansThai-Regular.ttf` | 400 |
| `NotoSansThai-SemiBold.ttf` | 600 |

Source: https://github.com/google/fonts/tree/main/ofl/notosansthai
Original filename: `NotoSansThai[wdth,wght].ttf` (upstream Git blob
34b48ab6f74867dbfce19410a2f452abef34e3ff)

Regenerate from that file:

```python
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer
for w, name in ((400, "Regular"), (600, "SemiBold")):
    f = instancer.instantiateVariableFont(TTFont("NotoSansThai[wdth,wght].ttf"),
                                          {"wght": w, "wdth": 100}, updateFontNames=True)
    # Both cuts keep one family name, "Noto Sans Thai" (nameID 1), told apart by weight class;
    # nameID 16/17 are removed and nameID 2 is the weight name.
    f.save(f"NotoSansThai-{name}.ttf")
```

The desktop loads these local files at startup so Thai text works offline even on systems
without a Thai font. This is data, not a runtime network dependency.
