# O-ParametricEQ Stage 3 (GUI), Phase 3.3: Research (analyzer, meter, fills, listen)

**Researched:** 2026-11-16
**Scope:** ROADMAP Phase 3.3 and Stage 3 CONTEXT U2, U7, U8 (3.3 half) and open questions 2 and 3. It covers:
- the analyzer wiring (editor-owned `SpectrumAnalyzer`, `analyzerPre` / `analyzerPost`, Off gating) (UI-03);
- the output meter and the `vu-meter` 1.0.0 fit (UI-06, U7, open question 2);
- per-band fills (UI-07);
- the listen state machine: drag, latch, return, release on disable / delete / preset load (FUNC-08, U2, open question 3);
- the listen NaN fix (VERIFICATION-2.6 Issue 1, probe N1e) (U8);
- the 60 fps row and audio-thread time;
- the rows carried from VERIFICATION-3.2 (Issues 1, 2, 4).

**Method:** two scratch benches, kept in `research-3.3/` (see its README).

| Bench | What it is |
|---|---|
| `probe/probe33.cpp` | A probe built from the render harness's compile and link commands (the 2.6 verify `build.sh` pattern), against the repo's `Source/` and against a scratch copy with the listen fix. Rows: N1e gated, a cross-build render hash, AO1 (Off gating), AC1 (analyzer tick + payload cost), AT1 (processBlock time per analyzer mode), M1 (meter reads under host block sizes) |
| `draw-bench/` | A copy of the 3.2 editor bench (real editor and page in WKWebView) timing `draw()` with analyzer data injected through the real event path, Pre / Post / Pre+Post, listen, a per-band response-cache prototype, and real pointer drags |

- **[BENCH]:** measured on this machine (Apple M4 Max, macOS 26.6.2, dpr 2, JUCE 8.0.15, Release, `55da7efc`). Probe timings quoted are the first quiet run; a second quiet run is saved as `probe/probe-output-quiet-a.txt` and agrees within run-to-run noise (e.g. 24 × 96 dB/oct Pre+Post 260 vs 259 µs no consumer; 44.1 kHz Post 89 / 109 / 112 µs without the first run's outlier burst). The first run overlapped the draw-bench build and is kept as `probe/probe-output-contended.txt`, not used.
- **[ARGUED]:** from reading `Source/` at `55da7efc`, the page, the `vu-meter` module, or JUCE 8.0.15.

**Contracts read:** CONTEXT (Stage 3), ROADMAP § 3.3, REQUIREMENTS UI-03 / UI-06 / UI-07 / FUNC-08, VERIFICATION-2.6 Issue 1, VERIFICATION-3.2 Issues 1 / 2 / 4, SUMMARY-3.2 "For 3.3", `mockups/v1-integration-checklist.md` (meter, analyzer events), `mockups/v1-ui-test.html` `drawMeter`, `modules/metering/vu-meter/` (README, `VUMeterBridge.h`, `vu-meter.js`), `SpectrumAnalyzer.h/.cpp`, `Listen.h`, `PluginProcessor.h/.cpp`, `PluginEditor.h/.cpp`, `index.html`.

## Summary

1. **Most of 3.3's page already exists.** The page has the analyzer traces (`drawAnalyzer`, listeners for `analyzerPre` / `analyzerPost`), the per-band watercolour fills (UI-07), the listen halo and wash, the strip's latch button and `setListen`, and the editor registers `setListenBand` and resets listen in its destructor. What 3.3 adds: the editor's analyzer and meter emits, the meter itself, listen-on-drag with latch return, release on disable, the NaN fix, and the gates [ARGUED].
2. **The listen NaN fix works and changes nothing else.** `Listen::renderVoice` flags a non-finite last sample; step 8 takes the flag every chunk and treats it as a non-finite lane. N1e (a NaN inside an engage's input ramp, no band running): **12 / 16 → 16 / 16** (mono / stereo, blocks 64 / 512, band on / off, engage / change). A 3.4 MB render set with listen on and off, auto gain on, random blocks, 44.1 / 96 kHz, mono / stereo hashes **identical** across the two builds [BENCH]. Trap: the flag must be taken unconditionally, not inside `! finite || …` (P1).
3. **`vu-meter` 1.0.0 cannot draw the approved meter, and its bridge would make it worse.** The JS has one DOM bar per instance, a gradient that stretches with the level (zone colours move), no ruler, per-instance rAF loops and a per-frame peak decay. The C++ bridge stores one block's peak per call (overwrites), where the processor's `readAndResetOutputPeak` already max-holds. **Call C1 for Taylor:** take U7's fallback (a page-drawn canvas meter ported from the mockup's `drawMeter`), recommended [ARGUED].
4. **The meter must hold across ticks with no block.** At 60 Hz, a host block longer than a tick leaves ticks where `readAndResetOutputPeak` returns 0: 47 / 165 ticks at 1024 / 44.1 kHz, 106 / 165 at 2048. A max over the last ⌈block / tick⌉ + 1 reads holds −6.00 dB on every tick up to 50 ms blocks [BENCH]. The editor knows the prepared block size, so no processor change is needed (P4).
5. **The analyzer is cheap enough for 60 Hz; send tenths of a dB.** `SpectrumAnalyzer::process` per 60 Hz tick: Post 89 µs at 48 kHz, 320 µs at 192 kHz; Pre+Post 164 / 627 µs (p99 ≤ 745 µs). The payload as full doubles costs 82 µs and 5.2 KB per tap; as integer tenths of a dB, 9 µs and 1.5 KB [BENCH] (P2).
6. **The 60 fps row passes without a response cache.** In WKWebView at 1600 × 1000, dpr 2, on the 192-section E-draw: `draw()` median 10 / p99 11 – 12 ms with Off, Post or Pre+Post; analyzer traces add ≤ 0.1 ms; drawing every frame held 17 / 18 ms frame intervals (no drops); real pointer drags 8 ms median, p99 11 – 12 ms. The cache prototype saves 1.5 – 2 ms and a frequency drag defeats it. Half the draw is `composeSum` (5.2 ms), which no cache touches. **Recommendation: no cache in 3.3** [BENCH].
7. **The analyzer adds no measurable audio-thread time.** processBlock median at 48 kHz, 24 × 96 dB/oct, block 512: no consumer 264 µs, Off 261, Post 266, Pre+Post 274 (≈ 4 %, inside run-to-run noise); 4 bells 12 – 13 µs every mode. Off drains 0 / 0 samples (AO1 PASS) [BENCH]. The redraw has no path to the audio thread [ARGUED].
8. **One page bug: analyzer points are drawn up to 2.8 px off frequency.** The page places point `i` at `i / 255` of the axis; point `p` spans `[f_p, f_{p+1})` with `f_p = 10 × 3000^(p / 256)`. Use the span centre `(i + 0.5) / 256` (P3) [ARGUED].

One call for Taylor (C1). Everything else is the plan's.

---

## Q1. Analyzer wiring (UI-03)

### What exists [ARGUED]

- `SpectrumAnalyzer` (Phase 2.6): message thread, registers itself as the processor's consumer in its constructor and unregisters in its destructor; `process (dt)` drains both taps, transforms, applies a 20 dB/s release using the passed `dt`, and reduces to 256 points over 10 Hz – 30 kHz. It re-prepares itself on a rate or generation change. Points above `fs / 2` read the floor (−200 dB).
- The processor writes a tap only while a consumer is registered and `analyzerMode` has its bit (`tapMaskForBlock`, read once per block). `analyzerMode` 0 → no writes.
- The editor builds no analyzer yet; `timerCallback` (`PluginEditor.cpp:646`) ends with a 3.3 placeholder.
- The page draws `analyzerPre` / `analyzerPost` as arrays of dB on a 0 to −96 dBFS scale (`ANALYZER_FLOOR_DB`), and requests a render on each event.

### Wiring (P2)

- The editor owns `std::unique_ptr<SpectrumAnalyzer>`, built in the constructor after the `DisplayModel`, destroyed first in the destructor (unregisters the consumer before anything else goes).
- **One reader per ring.** The taps are single-reader. JUCE gives a processor one active editor, so one analyzer exists at a time. Keep it that way: no second consumer (a harness or a bench must not run its own analyzer while an editor is open).
- `timerCallback`: `analyzer->process (measuredDt)` **every tick, whatever the mode.** With mode 0 it drains nothing and computes no frame, but the release keeps running, so the held values fall to the floor. If `process` were skipped while Off, switching back on would show a stale spectrum from minutes ago, falling at 20 dB/s. `measuredDt` = `Time::getMillisecondCounterHiRes()` delta, clamped to [0, 0.25] s.
- Emit per tap only when the mode includes it **and** its points differ from the last emitted array (`memcmp`). On a mode change that adds a tap, emit it once. Once the display has fallen to the floor with no audio, nothing is emitted, so 3.1's idle-traffic row (0 messages) still holds with the analyzer on.
- **Payload:** an array of 256 integers, tenths of a dB, clamped at −1200 (−120 dB, below the page's −96 floor). Spans that start at or above `fs / 2` go out as `null`. The page maps `x / 10` and `null` → `NaN` explicitly: `ay(null)` would read 0 dBFS (`null / −96 = −0`, clamped to the top), which draws a wall at the right edge at 44.1 / 48 kHz. `tracePath` already skips `NaN`.
- Rate: 60 Hz is affordable (Summary 5). At 192 kHz Pre+Post the tick is 0.63 ms median, 0.75 ms p99, plus 2 × 9 µs payload.

### Cost [BENCH]

`probe33 a`, AC1: noise in, 6 s, a tick every 1 / 60 s of audio, ticks after the first second:

| fs | N | Post med / p99 / max µs | Pre+Post med / p99 / max µs |
|---|---|---|---|
| 44.1 k | 4096 | 91 / 611 / 915 (one outlier burst) | 165 / 193 / 194 |
| 48 k | 4096 | 89 / 103 / 104 | 164 / 184 / 186 |
| 96 k | 8192 | 162 / 184 / 194 | 302 / 333 / 343 |
| 192 k | 16384 | 320 / 362 / 388 | 627 / 745 / 857 |

| Payload per tap | med / p99 µs | bytes |
|---|---|---|
| 256 doubles → `var` → `JSON::toString` | 82 / 125 | 5241 |
| 256 ints, tenths of a dB, floor −120 | 9 / 17 | 1543 |

### Off stops the audio-thread writes (ROADMAP criterion 1) [BENCH]

AO1: consumer registered, mode 0 for 1 s of noise: 0 / 0 samples drained; mode 2 for the next second: pre 0, post 48 000. PASS. The gate in 3.3 is this row in the render harness plus the editor-side check that `setUiState ("analyzerMode", 0)` reaches `analyzerMode` (already a 3.1 row).

### Display mapping (P3, finding 8) [ARGUED]

`drawAnalyzerTrace` places point `i` at `PLOT_MARGIN.l + i / (n − 1) × plotW`. Point `p`'s span is `[10 × 3000^(p/256), 10 × 3000^((p+1)/256))`; its centre is at `(p + 0.5) / 256` of the axis. The error runs from −0.5 / 256 to +0.5 / 256 of the plot: ±2.8 px at the 1458 px plot. Fix: `xs[i] = PLOT_MARGIN.l + (i + 0.5) / n × plotW`. A gate: a 1 kHz sine's analyzer peak is drawn at `fx(1000)` within 1 px (pixel probe, editor bench).

---

## Q2. Output meter: `vu-meter` 1.0.0 fit (UI-06, U7, open question 2)

### What the module offers [ARGUED]

`modules/metering/vu-meter/` (1.0.0, no consumer in the registry):

- **C++ `VUMeterBridge`.** `storeLevel (gain)` stores `gainToDecibels` of one value per call, overwriting. A block-rate store polled at 60 Hz keeps only the last block's peak before each tick. The processor's own `readAndResetOutputPeak (ch)` max-holds every block on every path since the last read, which is what a peak meter needs. `generateMeterScript()` builds an `evaluateJavascript` string calling a global `updateMeters`; the page uses events. Nothing in the bridge is needed.
- **JS.** `LEDMeter` (DOM segments with glow), `SmoothMeter` (one DOM bar), `NeedleMeter`. `BaseMeter` (not exported) has the ballistics.
  - One bar per instance, each with its own rAF loop.
  - `SmoothMeter`'s fill is a `linear-gradient` on an element whose height is the level. The colour stops scale with the level, so the sap / ochre / madder zones (below −6, −6 to 0, above 0) move as the level moves. The approved look has fixed zones.
  - No ruler. The checklist requires the ruler inside the same bordered box.
  - Ballistics are exponential in dB (`1 − e^{−speed × dt × 60}`), but the peak decays `0.05` of the gap **per frame**: twice as fast on a 120 Hz ProMotion display. `maxDB` defaults to 3 (the approved scale is +6).
  - `LEDMeter` could take three colours by segment fraction, but its look (segments, glow) is not the approved continuous bar.

### Assessment

The module would pass UI-06 (it converges to a constant target within 0.01 dB) but cannot draw the approved meter (`drawMeter` in `v1-ui-test.html`: one canvas, two bars, fixed zones, peak line in madder above 0 dB, ruler at 6 / 0 / −6 / −12 / −24 / −36 / −48 / −60 inside the box). Using it means subclassing a class whose constructor builds DOM, overriding `buildDOM` and `render`, and coordinating two rAF loops on one canvas. Nothing from the module would remain but the ballistics, which are frame-rate dependent.

### C1 — for Taylor: meter implementation

- **Recommended: U7's fallback.** A page-drawn canvas meter, ported from the mockup's `drawMeter`, fed by an `outputLevel` event from the editor's timer (`readAndResetOutputPeak`, P4). Ballistics in dB per second from the measured frame `dt` (attack instant, release ≈ 20 dB/s like the analyzer, peak hold 1 s then 20 dB/s), so a 120 Hz display behaves like a 60 Hz one. Drop `vu-meter` from the plugin: the compile-proof include in `PluginProcessor.h`, `ouaricon_add_module(… vu-meter)`, the embedded `vu-meter.js` and its resource-map entry (`/module-remove`), and amend ROADMAP's risk row and CONTEXT U7.
- **Alternative: upgrade the module.** Add a canvas two-bar meter with fixed zones, an inside ruler and time-based ballistics to `vu-meter` as 1.1.0, and consume it here. That makes this plugin the module's first consumer, as U7 hoped, but it is shared-module work (`/module-upgrade`) inside a GUI phase, and no other plugin needs it yet. The page-drawn meter can be extracted into the module later.

### Feeding the meter (P4) [BENCH]

M1 (model: blocks finish in real time; the editor reads both channels at 60 Hz; −6 dBFS 1 kHz sine; 3 s):

| fs / block | block ms | ticks reading 0 (no block since the last read) | raw read range | max over the last 3 reads, worst tick |
|---|---|---|---|---|
| 44.1 k / 1024 | 23.2 | 47 / 165 | −200 … −6.00 | −6.00 |
| 44.1 k / 2048 | 46.4 | 106 / 165 | −200 … −6.00 | −6.00 |
| 44.1 k / 4096 | 92.9 | 135 / 165 | −200 … −6.00 | −200 |
| 48 k / 1024 | 21.3 | 36 / 165 | −200 … −6.00 | −6.00 |
| 48 k / 4096 | 85.3 | 133 / 165 | −200 … −6.00 | −200 |
| 192 k / 2048 | 10.7 | 0 / 165 | −6.00 | −6.00 |

All blocks ≤ 512 at every rate: 0 empty ticks.

- Fed raw, the page's release pulls the bar towards the floor on every empty tick: a sawtooth that fails UI-06 at 1024 samples and above.
- **Fix:** the editor keeps the last `H = ⌈preparedBlock / fs × 60⌉ + 1` reads per channel and emits their max. `getBlockSize()` is the prepared maximum, so H covers the host's largest block (H = 4 at 2048 / 44.1 kHz, 7 at 4096 / 44.1 kHz). This adds at most H ticks of hold to a real drop, which the page's 20 dB/s release dominates anyway. No processor change.
- Emit `outputLevel` as `[dbL, dbR]` (tenths of a dB as for the analyzer, floor −600 → the meter's −60). Mono: one value, drawn as two equal bars. Emit only on change, so silence costs nothing (idle-traffic row).
- **Accuracy:** a −6 dBFS 1 kHz sine reads −6.00 dB at every rate (sample peak of 16+ cycles per tick). UI-06 gate: render-harness row M1 (the hold rule against the block sizes above, ±0.5 dB) and an editor-bench pixel row (the bar's top at `my(−6)` ± 1 px after 0.5 s of a −6 dBFS sine pushed through the processor).

---

## Q3. Per-band fills (UI-07)

### What exists [ARGUED]

`draw()` already paints each enabled, not-bypassed band's response as a wash between its curve and 0 dB in the band's pigment (`rgba(c, 0.27 × k)` at the edges, 0.07 in the middle; `k` 1.5 when selected), and its line in the pigment. Bypassed bands draw a dashed line and no wash. The selected band is painted last.

### What 3.3 owes

Only a gate. Editor-bench pixel probe: with bands 1 – 3 enabled at distinct frequencies and ±9 dB, the pixel at each band's peak, halfway between its curve and 0 dB, has a hue within ±15° of `pigment(b)`; with band 2 bypassed, that pixel is the background (no wash). V7 / V8 from VERIFICATION-3.2 Issue 4 belong in the same row group (Q7).

---

## Q4. Listen state machine (FUNC-08, U2, open question 3)

### What exists [ARGUED]

- Page: `ui.listen` (0 or 1 – 24), `setListen (b)` (writes `setListenBand`), the strip button toggles the selected band, `deleteBand` ends listen on that band. No listen on drag.
- Editor: `setListenBand (1 … 24, 0 off)` → `listenBand` (0 … 23, −1). Destructor stores −1.
- Engine: one target. A request during a move waits for the move's end, latest wins; engage 30 ms ramp + 20 ms fade, change the same, release 20 ms. **Listen does not look at the band's `enabled` or `bypass`** (it solos `F(dry)` by the band's placement), so the engine keeps listening to a disabled band. Ending listen on disable is the UI's job.

### Ownership (open question 3)

**The page is the only writer of `listenBand`** (plus the editor destructor's reset). The editor never decides listen on its own, so there is no second state to reconcile.

Page state: `ui.latch` (0 or band) and `ui.dragListen` (0 or band); the effective value is `dragListen || latch`. One function, `syncListen()`, computes it and calls `setListenBand` only when it changes.

| Event | Effect |
|---|---|
| Node drag passes the threshold (`d.moved` becomes true, the P5 block) | `dragListen = d.b`. Not at pointerdown: a click to select must not solo audio for 50 ms |
| Drag ends (`onUp`: pointerup / cancel / lost capture; idempotent) | `dragListen = 0` → back to the latch, or off |
| Strip Listen on the selected band | `latch = (latch === sel) ? 0 : sel` |
| `bNN_enabled` → false from any source (delete, chip, host automation, preset load) | if `latch === b`, `latch = 0`; if `dragListen === b`, `dragListen = 0` |
| Editor closed | the destructor's −1 (exists) |

- Hook the disable rule on the enabled relay's `valueChangedEvent` (it fires for host and preset changes, not only the page's own), not on `deleteBand`. `deleteBand` keeps working through it.
- A preset load that keeps the latched band enabled keeps listening (the band's new frequency / Q reach the voice on its tick; Stage 2 rows).
- Bypass does not end listen (CONTEXT says disabling or deleting). The halo and the solo stay on a bypassed band. Stated so the plan does not invent a rule.
- A drag on another band while latched: `change` to the dragged band, and on release `change` back. Fast repeated drags queue in the engine (latest wins), so no state is lost.
- Typed entry, wheel and keyboard edits do not listen (U2 says node drag).
- The halo (`listenPulse`) follows the effective band; `ui.listen` becomes that derived value.

### Gates

Editor-bench rows (the bench reads `listenBand` through its hook): drag past the threshold → band; release → previous latch or −1; a click without movement → never set; latch → drag another → release → latch; latch → host automation of `bNN_enabled` → −1; latch → preset load disabling it → −1; latch → bypass → unchanged. Plus mutants: listen at pointerdown, no return to latch, disable hook only in `deleteBand`.

---

## Q5. The listen NaN fix (VERIFICATION-2.6 Issue 1, U8)

### Fix (P1) [BENCH]

In `Source/dsp/Listen.h`:
- a member `bool nonFiniteVoice = false;`;
- in `renderVoice`, after `soloRender`: if `outL[count − 1]` (or `outR[count − 1]` on stereo) is not finite, set it;
- `takeNonFiniteVoice()` returns and clears it;
- `goIdle()` clears it.

In `PluginProcessor.cpp` step 8, before the `! finite` test:

```cpp
if (listen.takeNonFiniteVoice())
    finite = false;
```

- **Take it every chunk, unconditionally.** Folding it into `if (! finite || listen.takeNonFiniteVoice())` would short-circuit when a lane is already non-finite, leave the flag set, and clear every band's state again at the next (finite) chunk: an audible glitch on clean audio.
- The flag only ever sets when a voice is rendered, i.e. listen engaged. Listen-off output is untouched.
- Cost: one `isfinite` per rendered voice per 256-sample piece.

### Results [BENCH]

`probe33 n`, NaN at `t1 + 700` (inside the 30 ms ramp of an engage at sample 1001), 48 kHz:

| Build | N1e | Late non-finite runs |
|---|---|---|
| `55da7efc` | 12 / 16 FAIL | engage, band off: [2448 .. 2472] at block 64, [2448 .. 2536] at block 512, mono and stereo |
| scratch fix | **16 / 16 PASS** | none |

- `change` (5 → 3) never failed, even unfixed: the outgoing voice is heard, so the NaN reaches the lane at its own chunk.
- Cross-build hash of renders with no NaN (listen engaged / off / switched, auto gain on, random blocks ≤ 1500, 44.1 / 96 kHz, mono / stereo, 3.4 MB): `6274ff9b5b0e488a` both builds.
- In 3.3: N1e becomes a render-harness row (the probe row as written, 16 cases), XR-lid stays SHA-equal, and a mutant (flag never set; flag taken inside the `||`) must fail it. The second mutant needs a row of its own: NaN in a listen voice's ramp → every chunk after the next one is bit-equal to a render with the NaN sample zeroed, from two chunks after.

---

## Q6. 60 fps redraw and audio-thread time (ROADMAP criterion 5)

### Page time [BENCH]

`draw-bench` (full table in `research-3.3/draw-bench/README.md`). 1600 × 1000 editor, plot 1458 × 838 CSS px, dpr 2, n ≥ 300 per case, no void runs:

| Case | median / p99 / max ms |
|---|---|
| E-draw (192 sections), Off | 10 / 11 / 11 |
| same, Post with data | 10 / 11 / 11 |
| same, Pre+Post | 10 / 11 / 12 |
| same, Post, a draw every frame | 8 / 9 / 9 (frame interval 17 / 18) |
| same, Post + listen (halo every frame) | 7 / 8 / 9 |
| 4 bells, Post | 4 / 5 / 6 |
| real pointer drag, frequency, Post | 8 / 11 / 18 |
| real pointer drag, gain, Post | 8 / 12 / 19 |
| per-band cache, Post | 9 / 10 / 10 |
| per-band cache, frequency drag | 8 / 10 / 21 (cache defeated: 71 hits / 3553 misses) |

Where the time goes (heavy, per draw): `composeSum` 5.2 ms, `evalBand` × 24 2.8 ms, washes 1.5 ms, `drawAnalyzer` ≤ 0.1 ms.

- **No cache in 3.3.** The heavy draw fits 16.7 ms at p99 with the analyzer on, and the cache misses on the common drag. SUMMARY-3.2's concern is retired.
- If a later phase needs headroom, the target is `composeSum`: with every running band Stereo (the common case) the 2 × 2 composition is a scalar product per point. Not needed now.
- `performance.now()` is quantised to 1 ms in this WKWebView. A gate on `draw()` time should use medians / p99 over ≥ 120 draws and bounds in whole ms.
- Canvas rasterisation happens in the compositor and is not in these timings. The frame-interval figures (17 / 18 ms with a draw every frame) are the end-to-end check, so the 60 fps row should measure **rAF intervals during a drag** (median ≤ 17.5 ms, p99 ≤ 25 ms), not only `draw()`.

### Audio thread [BENCH] [ARGUED]

AT1, processBlock time, 48 kHz, block 512, noise, the analyzer drained at 60 Hz between blocks:

| Config | no consumer | mode 0 | Post | Pre+Post (µs median) |
|---|---|---|---|---|
| 4 bells | 11.7 | 13.0 | 13.0 | 13.0 |
| 24 × 96 dB/oct | 264 | 261 | 266 | 274 |

At 192 kHz, all four within 3 µs of each other. The tap push is a copy into a ring; the redraw runs on the message thread and reaches the audio thread only through `analyzerMode` and `listenBand` atomics.

Gate (P8): in the editor bench, a thread runs `processBlock` (24 × 96 dB/oct, 48 kHz, 512) paced in real time for 10 s three times: editor closed, editor open with the analyzer Off and no drag, editor open with Pre+Post and a scripted 60 fps node drag. Median block time of the third within 10 % of the first, p99 within 25 %. Run on a quiet machine (the contended AT1 run had p99s of 50 ms from a concurrent build).

---

## Q7. Rows carried from VERIFICATION-3.2

| Issue | Row for the 3.3 bench |
|---|---|
| 1. 60 Hz rate ungated (V1) | During the scripted drag, the median interval between `eqSections` emits ≤ 20 ms (a 30 Hz timer reads 33) |
| 2. E-create blind to placement / bypass (V15, V20) | Seed band 1 as High Shelf / Side / bypassed and disabled before the plot click; require Bell / Stereo / not bypassed after |
| 4. Global-bypass flat line (V7) | Bypass on → the ink pixels along the plot lie on `gy(0)` ± 1 px |
| 4. View follows the selected band's placement (V8) | Mid and Side bands present, a Side band selected → the ink matches the SS composition at three frequencies (pixel probe) |
| 3. Point-tip housekeeping | Stays Stage 4 (i18n review) as VERIFICATION-3.2 says |

---

## Recommendations for the plan

| ID | Item |
|---|---|
| P1 | Listen NaN fix as in Q5; N1e render row (16 cases) + the "taken unconditionally" row; XR-lid SHA-equal; two mutants |
| P2 | Editor-owned `SpectrumAnalyzer`; `process (dt)` every tick whatever the mode; emit per tap on change, tenths of a dB, `null` above Nyquist |
| P3 | Page: analyzer events as tenths with `null` → `NaN`; points at span centres `(i + 0.5) / n` |
| P4 | Editor meter feed: `readAndResetOutputPeak` both channels, max over the last `H` reads, `outputLevel` on change |
| P5 | Meter per C1: page-drawn canvas (recommended) or `vu-meter` 1.1.0; dB-per-second ballistics from measured `dt`; ruler inside the box; mono draws two equal bars |
| P6 | Listen state machine of Q4: `latch`, `dragListen`, `syncListen()`, disable hook on the enabled relays |
| P7 | No per-band response cache |
| P8 | Editor-bench rows: analyzer Off / Pre / Post / Pre+Post emits and pixel peak at 1 kHz; meter −6 dBFS; fills hue; the listen rows of Q4; rAF intervals during a drag with Pre+Post; audio-thread time with the editor open; the four rows of Q7 |
| P9 | Render-harness rows: N1e, AO1, M1 (hold rule against block sizes); version 0.9.0 |

## Sources

- `research-3.3/probe/` — `probe33.cpp`, `build.sh`, outputs (`probe-output-base-n.txt`, `probe-output-fix-n.txt`, `probe-output-contended.txt`); the quiet AC1 / AT1 run is quoted above.
- `research-3.3/draw-bench/` — README (full tables and phase split), `cache-prototype.diff`, `results/`.
- VERIFICATION-2.6 Issue 1 and `stages/2-dsp/verify-2.6/probe.cpp` (N1e's origin).
- `modules/metering/vu-meter/` 1.0.0.
