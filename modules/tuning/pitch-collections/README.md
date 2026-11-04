# pitch-collections 1.0.0

Cents-based pitch collections for Ouaricon plugins: built-in tables, the text and
Scala parsers, and Scala export. Header-only (`cpp/PitchCollections.h`), `juce_core`
only, compiles at C++17.

```cmake
ouaricon_add_module(<Target> pitch-collections)
```
```cpp
#include "PitchCollections.h"
namespace PC = ouaricon::tuning::PitchCollections;
```

Origin: moved verbatim from O-GrainScatter 3.5.1 `Source/dsp/PitchEngine.h`
(`namespace PitchCollections`). GS 3.6.0 keeps its call sites with
`namespace PitchCollections = ouaricon::tuning::PitchCollections;`.

## Frozen contract

- **`enum Id`** — 32 entries, `Chromatic = 0 … MaqamRast = 30, Custom = 31`, `kCount = 32`.
  O-GrainScatter stores this index in saved sessions. Never re-order; new entries
  would go before `Custom`, which itself moves — so in practice: never add.
  `Free = 5` is a GS concept (unquantised); other consumers map around it
  (O-Granulator: choice 0 Off, 1…5 → Id 0…4, 6…31 → Id 6…31).
- **`names()`** — GS's `AudioParameterChoice` host strings. Frozen.
- **`builtIns()`** — a function-local static; call it once from the processor
  constructor so it never first-runs on the audio thread.
- Note: `JustSevenLimit` lists 12/7 after 7/4 (not ascending). Kept for GS session
  compatibility; consumers that need a sorted table sort their own copy.

## API

| Function | Notes |
|---|---|
| `Table` | `cents[64]` (degree 0 = 0), `count`, `period` (cents) |
| `ratioToCents`, `fromCents`, `equalDivision (n, period)`, `fromRatios` | table builders |
| `parsePitch (token, cents&)` | `a/b` = ratio, anything else = cents (bare integers included) |
| `tableFromPitches (pitches, out, error)` | Scala convention: last = period; fold, sort, dedupe |
| `tableFromText (text, out, error [, badToken])` | tokens split on space `,;` tab/newline; or edo shorthand |
| `tableFromScala (scl, out, asText, description, error)` | `.scl` rule: bare integer = `a/1` |
| `toScala (name, text)` | `.scl` body; `{}` if `text` does not parse |
| `scalaDescription (name)` | the description line `toScala` writes |

### Error codes

`noPitches`, `badPitch` (with `badToken`), `badPeriod` (period not in (1, 4800] ct),
`tooMany` (> 64 degrees), `badEdo` (edo N not in 1…64), `badCount` (Scala only).

### edo grammar

`^(?:edo\s*(\d+)|(\d+)\s*-?\s*edo)(?:\s+(\S+))?$`, case-insensitive, on the trimmed
text: `edo 19`, `EDO19`, `19-edo`, `19 edo`, `edo 13 3/1`. The optional third token is
the period (cents or ratio, default 1200). Checked in order: period token parses
(`badPitch`), period range (`badPeriod`), N in 1…64 (`badEdo`; more than 3 digits is
`badEdo`). Built with `equalDivision (n, period)`, so `edo 19` is bitwise
`builtIns()[Edo19]` and `edo 13 3/1` is `BohlenPierce`. Text that is not edo-shaped
(`edo`, `edo 12.5`, `edo 19, 1200`) falls to the token path → `badPitch`, token `edo`.
No input that parsed before 1.0.0 can match (the regex needs letters, which
`parsePitch` rejects). This matches the O-Granulator page's JS parser.

### toScala

Header `! <legal file name>.scl`, `!`, the description, ` <count>`, `!`, then degrees
1…count-1 and the period. A ratio token is written as typed when its value lands on a
degree unchanged; everything else is written as cents in the shortest fixed form that
reads back to the same float (always with a `.`, so the `.scl` bare-integer rule never
turns cents into a ratio). edo forms write cents and the period token (or `2/1`).

## Tests

```bash
cmake -S modules/tuning/pitch-collections/tests -B build-pitch-collections-tests -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build-pitch-collections-tests && build-pitch-collections-tests/pitch-collections-tests_artefacts/Release/pitch-collections-tests
```

U1 built-ins (digests == v3.5.1) · U2 edo equivalences · U3 edo errors · U4 offending
token · U5 v3.5.1 corpus (`tests/expected_v351.inc`, captured before the move) · U6
toScala round-trip. Exit 0 iff all 229 checks pass.
