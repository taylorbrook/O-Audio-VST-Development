# Stage 3 — GUI · Phase 3.2 (Buffer display + live grains) — Research

**Plugin:** O-Granulator
**Stage / phase:** 3-gui / 3.2 of 3
**GSD phase:** research
**Date:** 2026-11-06
**Tree:** `main` @ `21e7587d` (3.2 discuss). Nothing was edited, built or installed.
**Inputs:**
- `CONTEXT-3.2.md` (D30–D33 locked; seven open questions)
- Source: `PluginProcessor.h/.cpp` (`Telemetry`, `readTelemetry`), `PluginEditor.cpp` (`buildTelemetry`, `getTelemetry`), `ScaleStateJson.h` (`telemetryToVar`), `dsp/Engine.h/.cpp` (`process`, `matrixTick`, `handleFreezeEdges`, `spawn`, `renderSegment`, `readGrain`), `dsp/CaptureWriter.h` (`FreezeState`), `dsp/Spawner.h` (`plan`, `frozenDelay`, `acquire` census), `dsp/Grain.h`, `dsp/ControlGrid.h` (`panGains`, `GridParams::Stream`), `dsp/ModMatrix.h` (`writeTelemetry`)
- Module: grain-engine `BandlimitedReader.h` (`MipRing::copyLevel`, `levelSample`, `margin`), `GrainPool.h` (`isActive`, `getGrains`)
- Page: `Source/ui/public/index.html` — `pollTelemetry` / `ingestTelemetry` (2810–2845), `drawBuffer` (3023–3100), `updateReadouts` (3198–3230), font-load boot (3706)
- Gates: `tests/ui-stub/generic-overrides.json` (C++-generated, P106), `tests/render-harness/engine_gates_31.h` (TL31)
- Memory: canvas text needs font-load repaint, webview completion gated on isVisible, UI fixture state producible by C++, distinct buffer addresses are not an allocation bound, pointer moved ≠ audio changed

There are **ten findings** (R117–R126). All seven open questions are answered (§10). Two would ship a visible defect if they were missed:
- **R117:** the torn-read path blanks the strip. The page replaces `telemetry` wholesale, so it does not keep its last frame.
- **R121:** a wave computed at "head now" drifts up to a whole host block away from the dots.

One finding removes a requirement instead of adding one: **R118** recommends a triple buffer, which has no torn read at all.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R117** | **"The page keeps its last frame" is false.** `pollTelemetry` does `telemetry = JSON.parse(raw)` (wholesale). A torn answer carries `scaleRev` alone, so for that frame `wave` / `grains` / `sources` / `counts` are all absent: the strip blanks, the readout shows 0 / limit, and the chips go dark | **Defect if the seqlock design ships** | Use a design with no torn read (R118). Any surviving `false` path must re-send the editor's last good frame, never a bare `scaleRev` |
| **R118** | **Triple buffer, not seqlock.** A seqlock over a non-atomic payload is a formal data race (UB, and TSan-visible). It needs a retry policy and still has the R117 failure. A triple buffer (3 × the snapshot, one `atomic<uint8_t>` index + dirty bit) is wait-free on both sides and never tears. With no new frame, the reader re-reads the front buffer, which keeps the last frame for free | Design | `TripleBuffer<UiSnapshot>` in a new `dsp/UiTelemetry.h`. Audio side: one `exchange` per publish. Reader side: a `SpinLock` for multiple editors / harness. `readTelemetry` returns false only when the engine is not prepared |
| R119 | **Audio thread copies only.** Every dot field can be derived from fields already in `Grain`: `readAbs`, `age`, `frac`, `invLen`, `len`, `rate`, `gainL/R`, `killGain`, `windowStep`, `tilt*`, `stream`. Do the math on the message thread (the bank is immutable, as in R107). The audio thread copies a 56-byte POD per active slot (≤ 144) | Design | §3 field table. Publish at most every 1/60 s (sample-counted), at the end of `process()` |
| R120 | **Peak ring:** one peak per 32-sample ring cell (`max(|L|, |R|)` of what was **pushed into the ring**, read back with `copyLevel`), so it is read-only and bitwise-neutral. Fixed capacity 2^18 cells, allocated in the **constructor** and never reallocated, so a `prepareToPlay` call off the message thread cannot free it under the reader. 61 500 cells @ 48 k, 246 000 @ 192 k; above 204 kHz, 64 samples per entry | Design | §4. 1 MB of `atomic<float>`. Cleared in `prepare` |
| **R121** | **The wave must be binned at the snapshot's head, not at the live head.** If the reader bins at `ring.getWriteCount()` and the dots carry the head of their publish, the two differ by up to a block: 512 @ 48 k = 10.7 ms = **10 px at a 1 s span**. The snapshot carries `W` (ring domain, the stopped head when frozen). The reader bins `[W − span, W)` | **Visible misalignment if missed** | `UiSnapshot::head`. Every dot delay and wave bin is measured from it |
| R122 | **Bin stability.** Fractional bins re-anchored to a moving head make each bin's max window slide every frame, so transient material shimmers. Instead, anchor bins to absolute cell space: `B = spanCells / 464`, `k0 = floor((Wc − spanCells) / B)`, bin `k` = cells `[⌊(k0+k)·B⌋, ⌊(k0+k+1)·B⌋)`. Content per bin is then stable and the wave scrolls in whole-bin (2 px) steps. Alignment error < 1 bin. While frozen, the wave is bitwise identical frame to frame | Design | §4. Gate WV4 (frozen frames identical) |
| R123 | **D30 needs no new modulation math.** After `matrixTick()`, `gp.s[s].position` / `.spray` already hold the tick-effective (modulated) values as 0…1 fractions (`pctFromRaw`), and `spanSamples` already holds the modulated `buffer_length`. Copy them into the snapshot. Per-spawn latching (R48) lerps inside the cell, which is sub-pixel at 30 Hz. The band should show the tick value | Q4 closed | `bands: [[pos, spray] × 4]` (fractions). Page: `drawBuffer` reads `t.bands[n−1]` with a fallback to `scaled(…) / 100` |
| R124 | **Frozen geometry.** `spanSec` while frozen = `min(spanSamples, heldCaptured) / fs` (`spanFrz`, the R24/R27 value `spawn` uses), with `heldCaptured = W0 − margin(2) − 1`. Within the first span seconds after the plugin starts, that is shorter than `buffer_length`, which is correct: only that much was captured. The frozen band is an approximation. The engine measures position from the floor `rl + 1` over `A = span − rl − 1` and adds a minimum spray `rl`. So the band can sit up to `rate·len` left of the real spawn region (a 2 s grain at a 30 s span is ≈ 7 %) | INFO | Accept. Hands-on watch item. Dots are exact regardless |
| R125 | **The stream index is 1-based on the wire.** `Grain::stream` is 0…3, but the page skips `n < 1` and the C++ overrides already emit 1…4. Copying `g.stream` verbatim hides **every stream-1 dot** and shifts the colours | Trap | Snapshot stores `stream + 1`. Gate GD1 checks that the stream-1 dot is present |
| R126 | **TL31 asserts that the 3.2 fields are zero.** It fails the moment 3.2 lands. This is a planned row change, not audio movement | Gate bookkeeping | Rewrite as TL32 (3.2 fields live, 3.3 fields still zero / NaN). Record it in the carried-diff attribution, as was done for A‴ |

---

## 1. Torn-read semantics (R117)

`index.html:2818`:
```js
const raw = await telemetryFn();
telemetry = typeof raw === 'string' ? JSON.parse(raw) : raw;
```
`telemetryToVar (t, torn = true)` returns `{ scaleRev }` only (`ScaleStateJson.h:118`). On the next rAF:
- `drawBuffer` finds no `wave` or `grains`, so the waveform and the dots vanish, and the span falls back to `buffer_length`. If frozen, the "held for" caption jumps.
- `updateReadouts` finds no `counts`, so it shows "0 / 128", and every stream head reads "0 sounding".
- `updateLiveModulation` finds no `sources`, so it returns early: the chips freeze, which is the harmless case.

That is a one-frame flash at a rate set by audio/UI collisions. The CONTEXT text ("the page keeps its last frame") describes intent, not the code. Fixing the page instead (a merge) would widen the D28 exception beyond `drawBuffer`. Avoid it: pick a reader that never tears (R118).

## 2. Snapshot transport (R118)

**Recommendation: a triple buffer.**

```
struct TripleBuffer<T> {            // T trivially copyable
    T buf[3];
    std::atomic<uint8_t> mid { 1 };      // bits 0-1: index, bit 2: fresh
    uint8_t back = 0, front = 2;          // owned by writer / reader respectively
    T& writeBuffer()          { return buf[back]; }
    void publish()            { back = mid.exchange (back | 4, acq_rel) & 3; }
    const T& read()           { if (mid.load (relaxed) & 4) front = mid.exchange (front, acq_rel) & 3; return buf[front]; }
};
```
- **Audio side:** fill `writeBuffer()`, then `publish()`: one atomic exchange, wait-free, no retry, no spin.
- **Reader side:** `read()` is wait-free and always returns a complete frame. When no new frame arrived, it returns the previous front buffer, which gives "keep last frame" at the source (R117).
- **Single reader** is the precondition. Guard `read()` with a `juce::SpinLock` that only readers take: a second editor window, or the harness calling `readTelemetry` while an editor exists. The audio thread never touches that lock.
- **Size:** `UiSnapshot` ≈ 144 × 56 B + header ≈ 8.2 KB; × 3 ≈ 25 KB, held as `Engine` members (no heap).

**Why not a seqlock?** The payload would have to be all-atomic, or the code would rely on `memcpy` with a fence, which is formally a race. The reader also needs a retry/false policy, and a false result reintroduces R117. A seqlock's only advantage is a smaller buffer, and memory is not the constraint here.

`readTelemetry` keeps `bool`. It returns false only when `! engine.isPrepared()`, and then `hasBuffer = false` (the page falls back to the base knobs). The `torn` argument of `telemetryToVar` becomes dead. Keep the parameter, but stop the editor from passing true, or delete it. The planner decides; deleting it is cleaner.

## 3. Grain snapshot fields (R119, R125)

**Copied per active slot on the audio thread.** The copy reads the pool only and never writes it.

| Snapshot field | Source | Note |
|---|---|---|
| `stream1` (uint8) | `g.stream + 1` | R125 |
| `readAbs` (double) | `g.readAbs` | |
| `phase` (double) | `(g.age + g.frac) · g.invLen` | the exact expression from `renderSegment` |
| `rate` (double) | `g.rate` | |
| `lenSamples` (float) | `g.len` | |
| `gainL`, `gainR` (float) | `g.gainL`, `g.gainR` | spread × gain_rand, latched |
| `killGain` (float) | `g.killGain` | |
| `windowStep` (int16), `tiltT/A/B` (float) | latched | |
| `killing` (bool) | `g.killStep > 0 ‖ spawner.isPendingKill(i) ‖ i >= kPoolLive` | excludes the slot from `counts` |

Header: `head` (int64, ring domain: `ring.getWriteCount()`, which is W0 when frozen because the head stops), `spanSamples` (double, R124), `frozen` (bool, the engine's real state), `bands[4][2]`, `limit` (`gp.grainLimit`), `streamPan[4]` (see `pan` below), `fs`, `count`. Live slots (0…127) are copied first, then the 16 fade slots while there is room, so the total stays ≤ `kMaxGrains = 128`.

**Derived on the message thread** (`readTelemetry`):

| Wire field | Formula | Check |
|---|---|---|
| `delaySec` | `max(0, (head − readAbs) / fs)` | Same domain as the wave (R121). The R23 ring-domain anchor means `readAbs` is a ring index. While frozen, forward dots drift right toward the dashed head, which is the true geometry |
| `pitchSt` | `12 · log2(rate)` | Includes the scale pick and absolute follow (both live in `rate`). Already clamped to ±24 by `plan` |
| `pan` | `clamp((4/π)·atan2(gainR, gainL) − 1 + streamPan, −1, 1)` | Inverts `panGains`: θ = (p+1)·π/4. Exact at spread 0 (1, 1) → 0. Adding the stream pan (on the bus) is display-only, so a hard-left stream does not draw centred |
| `gain` | `sqrt((gainL² + gainR²) / 2)` | `panGains` is √2·(cos, sin), so the norm returns the gain_rand factor exactly (1 at gain_rand 0) |
| `env` | `window(phase) · killGain` | Step 0 with tilt 0.5 uses `hannWindow(phase)`; otherwise `WindowMorphBank::read(bank.table(step), tilt, phase)`. Both are already const and message-thread safe (R107). The pool fade gain (`fade.next()`) is not in `Grain`; fade slots draw at full env. Acceptable: they last 5 ms |
| `lengthSec` | `len / fs` | output samples |
| `counts[s]` | Count of `! killing` slots in stream s | The same census as `Spawner::acquire` pass 1, so Σ ≤ `limit` holds by construction (gate CN1) |
| `limit` | `gp.grainLimit` | D32 |

**Publish cadence.** At the end of `process()`, when `absCount − lastPublish ≥ round(fs/60)` (or on the first block). Inside one host block the cells run faster than real time, so publishing mid-block would only produce frames the reader never sees. For hosts with blocks of 32 samples, this throttle avoids 1 500 copies a second. Cost estimate: ≤ 144 × 56 B ≈ 8 KB memcpy-class work at 60 Hz, below 0.05 % of a core. The C-row INFO re-run confirms it.

## 4. Peak ring + wave (R120–R122, D31)

**Writer (audio thread, per piece, after step 3 `CaptureWriter::write`).**
- When the ring was written (not frozen), copy the piece's ring samples back with `ring.copyLevel(0, ch, wPiece, len, tmp)` into a `float[2][kCell]` stack buffer and take `pk = max |x|`.
- Cell index `c = (wPiece / 32) mod N`. Ring-domain write counts stay cell-aligned across freezes (R23: the ring lags `absCount` by whole cells), so `wPiece % 32 == cellPos`.
- If `cellPos == 0`, store `pk`; otherwise store `max(old, pk)`. Relaxed `atomic<float>`. The audio thread is the only writer.
- While frozen there is no push and no peak write, so the held cells keep their peaks.
- This reads what the grains read (input × input_gain + feedback return, release crossfade included) and never touches `CaptureWriter`'s statements. Changing those risks FP contraction and bitwise drift (see its header comment).

**Capacity.** `N = 2^18` entries allocated in the **constructor**, never resized. `prepare` zero-fills them with relaxed stores. Samples per entry `P = 32 · max(1, ceil(ringSamples / 32 / N))`, so P = 32 up to 204 kHz. A `prepareToPlay` on a non-message thread (some hosts do that) can then never free memory under the reader. Zeros read mid-prepare are benign.

**Reader (message thread).** Given the snapshot's `head` (W) and `spanSamples`:
```
Wc = W / P;  S = spanSamples / P;  B = S / 464
k0 = floor((Wc − S) / B)
bin k = max over cells [floor((k0+k)·B), floor((k0+k+1)·B))   (cells ≥ Wc or < Wc − ringCells → 0)
v = pk > 0 ? clamp((20·log10(pk) + 48) / 48, 0, 1) : 0           (D31)
```
- **Cost:** a 30 s span @ 192 k is ≈ 180 000 relaxed loads per poll, ≈ 5.4 M/s at 30 Hz. That is message-thread milliseconds per second, with nothing on the audio thread. @ 48 k it is 45 000 per poll.
- **Alignment:** absolute-cell anchoring (R122) keeps each bin's content fixed while it scrolls, and the residual offset is < 1 bin ≈ 2 px. Fractional per-frame bins (anchored at W) shimmer instead.
- **Partial head cell:** the newest cell may hold a partial max. That is correct: it is what has been written.

## 5. D30 bands (R123)

`matrixTick()` (`Engine.cpp:850-862`) writes `ModMap::streamFieldFromRaw (gp.s[s], f, matrix.apply (…))` for every routed stream field at the tick, and recomputes `spanSamples` when `buffer_length` is routed. `gp.s[s].position` / `.spray` are fractions (`ControlGrid.h:284-285`). So, at publish:
```
bands[s] = { gp.s[s].position, gp.s[s].spray }   // already modulated, 0..1
```
`destTelemetry` is not used. It is v01 of the parameter range, which happens to match a fraction for 0–100 linear, but `gp` is the value the spawner actually uses, and it is consistent with `spanSamples` in the same snapshot.

**Page edit (the only D28 exception), in `drawBuffer`:**
```js
const bd = t && Array.isArray(t.bands) && Array.isArray(t.bands[n - 1]) ? t.bands[n - 1] : null;
const pos = bd ? Number(bd[0]) : scaled(`s${n}_position`) / 100, spray = bd ? Number(bd[1]) : scaled(`s${n}_spray`) / 100;
```
Nothing else in the page changes. `frozen` is still drawn from the `freeze` toggle, and `spanSec` already comes from C++.

**Overrides:** add a widest-value `bands` (for example, every stream at pos 0.5 / spray 1.0, the widest band) to the C++ `--dump-ui-overrides` frame, so the stub exercises the new branch. Regenerate it, and confirm it is byte-identical across two runs.

## 6. Frozen semantics (R124)

| Quantity | Live | Frozen |
|---|---|---|
| `head` | `ring.getWriteCount()` | `fz.W0` (the same call: the head is stopped) |
| `spanSamples` | `spanSamples` (modulated) | `min(spanSamples, fz.heldCaptured)` |
| wave bins | `[W − span, W)` | `[W0 − spanFrz, W0)`, bitwise stable |
| dot delay | `head − readAbs` | the same (forward dots drift right, reverse dots left) |

The deferred stop (`freezePending`, 9 cells ≈ 6 ms @ 48 k) is not shown as frozen. The page styles freeze from the toggle, so it switches ≈ 6 ms early. That is invisible.

## 7. Threading + allocation constraints (verified against CONTEXT)

- **Audio thread:** copy into the back buffer, one `exchange`, and per-piece peak stores. No lock, no allocation. Every new member is a fixed array (the peak ring is constructor-allocated), so the M7-class alloc gate covers block 0.
- **Message thread:** `read()` under a reader-only SpinLock, the derivations, wave binning, and `juce::var` building (it may allocate).
- **Audio must not move:** every new audio-thread statement only reads DSP state (`pool`, `gp`, `ring`, `fz`) and writes only telemetry members. Gate: the default harness carried diff against B is empty, plus N3/N2/N1/N0 `--strict` + R12 rc 0 (as in 3.1).
- **Payload:** 464 + 128 × 7 numbers. JUCE serialises `var` doubles at full precision, so a frame is ≈ 25 KB of JSON, about 0.75 MB/s at 30 Hz. Plan an INFO row that measures the serialised size of the widest frame. If it exceeds ≈ 32 KB, quantise on the message thread (wave to 1/1000, delays to 0.1 ms) before building the var. Check at execute that the JUCE formatter then emits the short form.

## 8. Harness rows (proposal)

| Row | Checks | Negative control |
|---|---|---|
| TB1 | Threaded triple-buffer stress (writer at 10 kHz, reader spinning, 1 s): every frame read is internally consistent (seq in header == seq in tail) | A single-buffer variant tears (mismatches > 0) |
| WV1 | Impulse train at known ring positions: peak bins at the expected delays ± 1 bin, live, at 44.1 / 48 / 96 / 192 k | A reader binning at the live head instead of snapshot `head`, with a 512 block, fails ± 1 bin at a 1 s span |
| WV2 | D31 map: silence → 0; −30 dBFS sine → 0.375 ± 0.01; 0 dBFS → 1; −60 → 0 | — |
| WV3 | Frozen: `spanSec == min(span, heldCaptured) / fs`; early freeze (< span captured) gives the short span | — |
| WV4 | Frozen: the wave array is bitwise identical over 50 consecutive publishes | Fractional head-anchored bins (R122 control) differ |
| GD1 | Dots vs the spawn log: delay = `(head − readAbs)/fs` within 1e-6 s; stream-1 dots present and 1-based (R125) | Copying 0-based (`g.stream`) drops stream 1 |
| GD2 | Field recovery: spread 0 / gain_rand 0 → pan 0, gain 1 exactly; spread 1 → pan within ±1; gain_rand → the expected cut; pitch +7 st → 7; env == `windowCurve` at the phase | — |
| CN1 | `Σ counts == Spawner census alive`, each ≤ limit, `limit == grain_limit`, across the 4 caps | Counting killing / fade slots exceeds the limit under a cap-lowered burst (GL3 stimulus) |
| BD1 | LFO → `s1_position`: snapshot band ≠ base and == `gp.s[0].position` at the tick; unrouted streams == base bitwise | — |
| M8 | 0 allocations from block 0 with telemetry publishing (existing counter); 8192-after-512 block sequence | — |
| TL32 | Replaces TL31: 3.1 fields unchanged; 3.2 fields live; 3.3 fields zero / NaN | — |
| PL1 (INFO) | Serialised bytes of the widest frame; publish cost µs @ 48 k / 192 k | — |

Then: carried diff vs B empty (TL31 → TL32 attributed), N-suites `--strict`, codec bytes 9/9, the five UI gates + probe 220 × 2, overrides regenerated × 2 byte-identical, install + auval + pluginval 10.

## 9. HANDS-ON-3.2 items (D33, Standalone)

1. A live waveform scrolls right-to-left. Quiet material (−30 dBFS) is visible (D31).
2. Write head solid. With freeze on, the head is dashed, the wave turns amber and stops scrolling, the "held for N s" caption shows the real held span, and the span is short just after launch.
3. Dots line up with transients in the wave at a 1 s span (R121) and at a 30 s span.
4. Pitch ±12 moves the dots up and down; spread / stream pan offset them; window choice changes their fade.
5. LFO → position: the band moves; with the LFO off it sits at the knob value (D30).
6. Grain limit 32: the readout never exceeds 32 / 32, and the per-stream bars add up.
7. EB Garamond in the canvas time labels after reopen (font-load repaint).
8. Watch: the frozen band offset with long grains (R124 INFO).

## 10. Open questions → answers

| CONTEXT question | Answer |
|---|---|
| Peak ring granularity / 464-bin derivation / 192 k cost | Per 32-sample ring cell, read back from the ring; binned on the message thread at the snapshot head with absolute-cell anchoring; 1 MB fixed; ≈ 180 k loads per poll worst case (§4) |
| Seqlock vs triple buffer; cadence; retry | Triple buffer, no torn reads, no retry; publish ≤ 60 Hz at block end (§2, §3) |
| Fields for delay / env / gain / pitch / pan | All are derivable from existing `Grain` fields. Copy on audio, derive on message (§3) |
| Effective position / spray | `gp.s[s]` after `matrixTick` (tick value) (§5) |
| Frozen held span + wave mapping | `min(span, heldCaptured)`, bins over `[W0 − spanFrz, W0)` (§6) |
| Harness rows | §8 |
| Overrides stub | Yes: add `bands`, regenerate from C++ (§5) |

## Sign-offs needed at plan

1. **R118 triple buffer** replaces the CONTEXT's "seqlock + false-on-torn". The `torn` serialisation path is retired.
2. **Dot `pan` includes the stream pan** (display-only sum, R119), rather than only the per-grain spread.
3. **R124 frozen-band approximation** is accepted (no per-grain-length correction).
