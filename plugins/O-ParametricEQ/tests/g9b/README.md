# G9 (b): the page's response maths against C++

PLAN-3.2 P4, RESEARCH-3.2 Q2. UI-02's JS half: the curve the page draws equals the plugin's own closed form within **0.01 dB**.

## Run

```bash
plugins/O-ParametricEQ/tests/g9b/run.sh        # BUILD_DIR defaults to <repo>/build
```

Needs a build configured with `-DOUARICON_BUILD_TESTS=ON` and node. About a minute: the harness renders the 260 XR-g9 draws, then node scores them.

Step by step:

1. `O-ParametricEQ-render-harness --dump-g9 <dir>` runs only the XR-g9 jobs (50 random 1–24-band draws per rate at 44.1 / 48 / 88.2 / 96 / 192 kHz, plus 10 mono) and writes one JSON per draw: the processor's `DisplayModel` packets and header, a grid (192 log points + each active band's frequency), and `composeFromPackets` on the unit circle (LL / RR / MM / SS). Without the flag the harness is unchanged.
2. `node g9b.mjs <index.html> <dir>` lifts `evalBand`, `magDb` and `composeSum` **by name** from the shipping `Source/ui/public/index.html` and scores them with XR-g9's floors (within 80 dB of the largest maximum among the views, ≥ −120 dB).

## What fails it

- worst |page − C++| > 0.01 dB;
- a vacuous run: a lifted function found 0 or 2 times (a rename), fewer than 260 draws or 100 000 scored points, a rate or the mono set missing, any NaN below Nyquist;
- a negative control that does **not** miss by more than 0.05 dB:
  1. `k × 1.01` in every SVF section;
  2. `onePoles` dropped;
  3. the first Left and Mid bands swapped in order;
  4. Side composed with the Mid matrix (one-token edit of the lifted `composeSum`);
  5. relay placement on mono: (a) the shipping `composeSum` gives the identical result with relay placements the engine ignores (odd Left, even Right) and with the packets' placement; (b) the 0.7.0 `composeSum` (`composeSum-0.7.0.js`, read the relay) fed the same placements misses by > 0.05 dB (59.5 dB at 3.2);
  6. the `active` filter removed.

Control 5 is the reason this gate exists in this shape: research found the mono bug (RESEARCH-3.2 finding 3) that a stereo-only scoring would have printed green over.

## Files

| File | |
|---|---|
| `run.sh` | build check, dump to a temp dir, node run |
| `g9b.mjs` | the scorer, guards and controls |
| `composeSum-0.7.0.js` | the pre-fix `composeSum`, verbatim; control 5b only. Never edited |
