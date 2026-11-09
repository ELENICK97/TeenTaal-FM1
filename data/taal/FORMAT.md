# Taal file format (`data/taal/*.json`)

One file per taal; the files are taken in name order (the number in front sets the order on the device).
`tools/taal_data/build_taal.py` validates them and generates `taal/taal_tables.h`: there is no JSON parser on the
device. Adding a taal = writing a file here and rebuilding.

```json
{
  "name": "Teentaal",
  "matras": 16,
  "vibhag": [4, 4, 4, 4],
  "marks": ["X", "2", "0", "3"],
  "theka": ["Dha","Dhin","Dhin","Dha", "Dha","Dhin","Dhin","Dha",
            "Dha","Tin","Tin","Ta", "Ta","Dhin","Dhin","Dha"],
  "tempo": 160,
  "source": "where the theka comes from"
}
```

| Field | Required | Meaning |
|---|---|---|
| `name` | yes | shown on the display; at most 15 characters, unique |
| `matras` | yes | matras in the cycle, 3..32 |
| `vibhag` | yes | the vibhags' lengths; they add up to `matras` |
| `marks` | yes | one per vibhag, one character: `X` sam (only the first), `0` khali, `1`..`9` tali. Sam is always the first matra, even when its vibhag is marked `0` (Rupak) |
| `theka` | yes | one entry per matra |
| `tempo` | no | a suggested tempo, matras a minute (20..600) |
| `source` | no (wanted) | the theka's source, and its known variants |

A theka entry is a bol, or several bols splitting the matra evenly, joined by `-`: `"Ti-Ra-Ki-Ta"` (four quarters),
`"Dha-Ge"` (two halves). `S` (avagraha) is a rest: `"S"` a silent matra, `"S-Dha"` a rest then Dha on the second half.
At most 8 parts in a matra. Bols are those of `data/tabla/bols.json` (case does not matter); any other is an error
naming the file, the matra and the bols available.

Played: the sam a little stronger, then the first matra of each vibhag; the parts of a divided matra a little
lighter after the first. Timing and dynamics get the small irregularity set by the taal's "human" parameter.
