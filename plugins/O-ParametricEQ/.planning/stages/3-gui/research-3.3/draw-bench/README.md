# research-3.3 draw-bench: draw() time with the analyzer on, and a per-band response cache

Measurement only. Nothing under `Source/`, `tests/` or `execute-3.2/` was changed. Nothing was installed or committed.

## Setup

- **Machine:** Apple M4 Max, 64 GB, macOS 26.6.2, built-in Liquid Retina XDR display. **dpr 2.**
- **Editor:** 1600 x 1000. The `#curve` canvas is 1458 x 838 CSS px (2916 x 1676 backing px), the same geometry as 3.2's E-draw.
- **Harness:** a copy of `execute-3.2/editor-bench`, which runs the real editor and page with the plugin TUs compiled in.
  - New JS: `src/BenchScript33.h`. New rows: `src/Rows33.inc`.
  - New flags: `--drawbench --cachebuild --drawn N`.
- **How draws are timed:** the 3.2 hooks are unchanged. The clock starts at the `clearRect` on `#curve` and stops at the end of that rAF callback.
- **Driving:**
  - One draw every 2 frames (the 3.2 cadence), unless the case says "1f".
  - 15 untimed warm-up draws, then N = 150 timed draws per case per run.
- **Analyzer data:** sent through the real event path, `backend.emitByBackend('analyzerPost' | 'analyzerPre', JSON-string)`. That is the string form JUCE's C++ emit uses, so the page's own listener parses it.
  - Every draw gets a fresh 256-point spectrum.
  - Shape: pink-ish, −30 dB at 10 Hz, −3 dB/oct, rolled off above 16 kHz, ±4 dB seeded noise, clamped to −90…−6.
  - Pre is the same curve +4 dB.
- **Configurations:**
  - **Heavy:** the E-draw config. 24 bands, shelf/cut types, slope index 7, mixed placements, 192 sections.
  - **Light:** 4 Bells. **These are 1 section each (4 sections total), not 2.** A Bell is a single SVF section.
- **Runs:**
  - `rel-1` and `rel-2` use the real `Source/`, unmodified.
  - `cache-1`, `cache-2` and `cache-3` use `peq-src/`, the scratch copy with the prototype. Their cases without the cache flag run with the cache off.
  - `cache-3` adds a finer split of the sum phase.
- **Screen lock:** lockstate read 0 before and after every run. **No run was void.** 0 timeouts, 0 failed checks.

## Results (ms; pooled over runs, so n = 150 x runs)

| case | build | n | median | p99 | max |
|---|---|---|---|---|---|
| a. heavy, analyzer Off | real | 300 | 10 | 11 | 11 |
| a. heavy, Post but no data (3.2's E-draw state) | real | 300 | 10 | 11 | 11 |
| b. heavy, Post with data | real | 300 | 10 | 11 | 11 |
| c. heavy, Pre+Post with data | real | 300 | 10 | 11 | 12 |
| b'. heavy, Post, a draw every frame (1f) | real | 300 | 8 | 9 | 9 |
| d. heavy, Post + listen on band 12 (halo + wash) | real | 900 | 7 | 8 | 9 |
| f. light (4 Bells), Off | real | 300 | 4 | 5 | 5 |
| f. light, Post | real | 300 | 4 | 5 | 6 |
| f. light, Pre+Post | real | 300 | 4 | 5 | 5 |
| heavy, Off, repeated at the end (drift check) | real | 300 | 10 | 11 | 11 |
| a. heavy, Off, cache off | scratch | 450 | 10 | 13 | 29 |
| e. heavy, Off + cache | scratch | 450 | 8 | 9 | 10 |
| b. heavy, Post, cache off | scratch | 450 | 10 | 11 | 12 |
| e. heavy, Post + cache | scratch | 450 | 9 | 10 | 10 |
| c. heavy, Pre+Post, cache off | scratch | 450 | 10 | 12 | 12 |
| e. heavy, Pre+Post + cache | scratch | 450 | 9 | 10 | 10 |
| heavy, Post 1f, cache off | scratch | 450 | 8 | 9 | 9 |
| e. heavy, Post 1f + cache | scratch | 450 | 7 | 8 | 8 |
| e. heavy, Post + cache + gradient-object cache | scratch | 450 | 9 | 10 | 12 |
| heavy, Post + listen, cache off | scratch | 1350 | 7 | 8 | 9 |
| e. heavy, Post + listen + cache | scratch | 1350 | 6 | 7 | 7 |
| e. light, Post + cache | scratch | 450 | 4 | 5 | 5 |
| 3. one band's packet changes every frame (1f, Post), cache off | scratch | 450 | 8 | 9 | 9 |
| 3. same, with the cache | scratch | 450 | 7 | 8 | 8 |
| 3. real pointer drag, band 12, frequency, cache off | real | 302 | 8 | 11 | 18 |
| 3. real pointer drag, band 12, frequency + cache | scratch | 453 | 8 | 10 | 21 |
| 3. real pointer drag, band 12, gain, cache off | real | 302 | 8 | 12 | 19 |
| 3. real pointer drag, band 12, gain + cache | scratch | 453 | 6 | 10 | 16 |

- The scratch build agrees with the real one within 1 ms on every shared case.
- Per-run figures are in `results/summary-*.txt`. Every run's median is within 1 ms of the pooled median.
- The single 29 ms max (scratch, heavy Off) is one draw in cache-1. No other case went above 21 ms.
- The real drags fed relay writes → C++ → eqSections, which arrived 144–145 times in 150 steps.
- **Cache hit / miss:**
  - Frequency drag: 71 / 3553, with 147 grid resets. The cache does almost nothing.
  - Gain drag: 3460 / 164, with 1 grid reset.
  - Packet-only change: 3450 / 150 per run, exactly 1 miss per draw.

## Where the time goes (heavy, scratch build, mean ms per draw, cache-3)

| phase | Off | Post | Pre+Post | Post + cache |
|---|---|---|---|---|
| grid | 0.02 | 0.04 | 0.01 | 0.01 |
| analyzer (`drawAnalyzer`) | 0.00 | 0.09 | 0.01 | 0.05 |
| frequency grid + `evalBand` x 24 | 2.75 | 2.83 | 2.79 | 0.19 |
| per-band magDb → y | 0.51 | 0.65 | 0.62 | 0.53 |
| per-band wash fill + line (24 gradients) | 1.54 | 1.40 | 1.50 | 1.67 |
| `composeSum` + y | **5.17** | **5.23** | **5.14** | **5.91** |
| sum bleed stroke (5 px, round) | 0.05 | 0.03 | 0.06 | 0.03 |
| sum ink stroke | 0.01 | 0.05 | 0.02 | 0.03 |
| nodes + readout | 0.17 | 0.14 | 0.17 | 0.21 |

Light config, Post: composeSum 1.31, eval 0.54, wash + line paint 0.72, the rest under 0.4 each.

`performance.now()` is quantized to 1 ms in this WKWebView. That is why every median above is a whole number. The phase figures are means of 1 ms-quantized deltas over 150 draws.

## What the cache prototype changed (`peq-src/ui/public/index.html`; full diff in `cache-prototype.diff`)

- **Module scope:** `const respCache = { key: '', m: new Map() }`.
- **Inside `draw()`:**
  - A grid signature: `plotW | displaySampleRate | freqs.length | the active bands' relay freq values`. When it changes, the map is cleared.
  - In the `responses` loop: if `window.__peqCache === true` and the cached entry has `e.pk === bandPackets[b]` (the same object identity), the cached `{re, im}` is reused. Otherwise the band runs `evalBand` as before and the result is stored.
  - Washes, lines, composeSum, analyzer and nodes are still drawn every frame.
- **Measurement-only additions, also in that diff:**
  - Phase marks (`window.__peqPhases`).
  - Hit/miss counters (`window.__peqCacheStats`).
  - A gradient-object cache A/B (`window.__peqGradCache`).
  - All of these are inert unless the bench sets the window flag.

## Observations (measured, nothing beyond)

1. **Analyzer cost is negligible in JS time.**
   - Post adds at most about 0.1 ms per draw. Pre+Post is the same.
   - Medians with and without analyzer data are identical: heavy 10 / 10 / 10, light 4 / 4 / 4.
   - The two gradient-filled traces (one `createLinearGradient` each per draw) do not show up.
2. **The heavy draw is already under 16.7 ms without any cache:** p99 11–13 ms, max 29 ms once.
   - With draws every frame (1f cases), the frame interval held at median 17 / p99 18 ms, so no frames dropped.
   - In the every-other-frame cases the rAF intervals alternate around 8–9 and 25–28 ms (pairs ≈ 33 ms).
3. **The cache saves ~1.5–2 ms per draw in the heavy case, not more.**
   - It removes `evalBand` almost entirely (2.8 → 0.2 ms).
   - But 5.2 ms of the 10 ms is `composeSum`, the 24-band 2x2 complex compose over ~755 points. The cache does not touch it, and it read about 0.7 ms higher with the cache on.
   - The light case gains nothing (eval is 0.5 ms there).
4. **A frequency drag defeats the cache as built.** The grid includes each band's frequency, so every frequency change clears all 24 entries (71 hits in 3624). A gain drag keeps the grid: 6 ms median against 8 without the cache.
5. **The gradient fills cost little in JS time.**
   - Per-band `createLinearGradient` + fill + stroke over 24 bands: 1.4–1.7 ms.
   - Reusing the gradient objects changed nothing (9 / 10 against 9 / 10).
   - The 5 px round-join bleed stroke and the ink stroke read ~0.05 ms, so canvas rasterization is not captured by this JS-side timer. It is deferred to the compositor, and that part was not measured here.
6. **Per-draw time drops when draws come every frame:** 10 → 8 ms heavy, the same code. Listen cases (the pulse draws every frame) also read lower: 7 ms. The cause was not measured.

## Files

- `run.sh <name> rel|cache [--drawn N]`: waits for an unlocked screen and validators, runs under caffeinate, and marks the run VOID if the screen locked.
- `summarize.py results/*.json`: writes the pooled table plus `summary.json`.
- `results/rel-{1,2}.json` and `results/cache-{1,2,3}.json` hold the raw per-draw arrays (`rows.draw33.<case>.raw`). Also: `results/summary-{rel,cache}.{txt,json}` and `results/runs.txt` (lockstate log).
- `peq-src/`: scratch copy of `Source/`. Only `ui/public/index.html` differs.
- `cache-prototype.diff`: diff of that page against the real one.
- Build dirs: `build-rel/` (real Source) and `build-cache/` (`-DPEQ_SRC=peq-src`).
