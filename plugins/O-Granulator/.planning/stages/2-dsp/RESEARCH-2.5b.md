# Stage 2 — DSP · Phase 2.5b (Onset detector + burst, MPM pitch follower, absolute pitch follow) — Research

**Plugin:** O-Granulator
**Stage / phase:** 2-dsp / 2.5b of 7
**GSD phase:** research
**Date:** 2026-11-05
**Tree:** `main` @ `d1b70f15` (engine unchanged since `2cf30120`)
**Inputs:**
- `CONTEXT-2.5b.md` (D17–D20 locked; seven open questions)
- ARCHITECTURE §4, §5, §15, §17, Algorithm §5, §7, §8
- the 2.5a engine: `Engine.cpp` (`process` pass order, `scheduleStreams`, `spawn`), `StreamScheduler.h` (Free `restart`/epoch, Tempo clock), `Spawner.h` (`plan` draw order, live/frozen delay, `fill` ring-domain anchor), `ScaleQuantiser.h` (`ActiveSet`), `EngineConstants.h` (RNG streams / slots, ring capacity), `ModSources.h:64`
- grain-engine 1.1.0 `GrainScheduler.h` (`reset`, `process`, `getOnsetCount`); pitch-collections 1.0.0 `PitchCollections.h` (`Table::period`, Bohlen-Pierce = 13 ÷ 3/1)
- O-GrainScatter 3.6.0 `dsp/PitchEngine.h` (root precedent)
- JUCE 8.0.15 `juce_dsp/frequency/juce_FFT.cpp` (AppleFFT / FFTFallback allocation paths), `juce_dsp.cpp:48` (`JUCE_USE_VDSP_FRAMEWORK` default 1)

**Scratch experiments:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/55810a67-2076-49d4-9542-2756105d1924/scratchpad/r25b/`

| Script | What it does |
|---|---|
| `mpm.py` | float32 model of ARCH §15 / Alg §7: 64-tap Blackman-windowed sinc at 0.4·fs/D, decimate on `abs % D == 0`, W 1024, FFT 2048 autocorrelation, running m(τ), first key max ≥ 0.9·max, parabolic refinement, clarity ≥ 0.8, −50 dBFS gate. Sine accuracy 40–1500 Hz at 44.1 / 48 / 96 k |
| `oct.py` | octave/fifth-error stimuli: H2/H1 and H3/H1 ratios, breathy (sine + white noise, SNR 20…0 dB), H2 + noise, AM at f0/2, missing fundamental; silence and noise clarity |
| `flip.py`, `guard.py`, `guard2.py` | near-threshold intermittent flips; D20 as written (median-of-3 + 2-hop hysteresis) vs an asymmetric lookahead confirm; synthetic injected flip lengths L = 1…5 |
| `onset.py`, `drums.py` | ARCH §8 detector (TPT 1-pole HP 150 Hz, fast 0.5/10 ms, slow 80/80 ms, +6 dB, 50 ms refractory): sustained-tone re-fires, single-sample clicks vs 1 ms bursts at 44.1–192 k; accent/ghost drum loop at 250…55 ms IOI, level- vs edge-triggered |
| `bench.cpp` | one MPM hop on vDSP (FFT 2048 fwd + inv, m(τ), NSDF, peak scan): mean and worst µs |

All seven open questions are answered (§9). There are **twenty findings** (R59–R78). Five would make a 2.5b gate fail, or pass vacuously, if missed: **R59, R60, R62, R66, R67**.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R59** | The pass runs the input side (write, env) over the **whole piece** before `scheduleStreams`. If the MPM hop and the f0 ring write run there too, a spawn at offset 3 sees a hop computed at offset 20 of the same piece. Pieces are host-block dependent, so the lookup result is too | **PERF-03 fails** | Hop results go to a 1-entry pending slot and are **committed at the next `gridTick`** (hop period ≥ 128 samples > kCell, so ≤ 1 per cell). Spawns and the Pitch source read committed state only. Onset events are safe as they are: they only act at their own sample |
| **R60** | ARCH §8 is **level-triggered**: after the 50 ms refractory the condition is re-tested, and the slow envelope (80 ms attack) is still rising. Every sustained onset fires at **0, 50 and 100 ms** (440 Hz and 200 Hz tones, every rate) | **FUNC-08 "no double fires" fails** | **Edge-trigger with re-arm** (ARCH §8 erratum, sign-off). Fire when the condition holds AND the detector is armed AND the refractory has expired. Re-arm when fast < +3 dB·slow. Measured: 1 fire per tone; the drum loop keeps every hit (§2) |
| R61 | The 0.5 ms fast attack makes the response to a **single-sample click** rate dependent: a 0 dBFS impulse reaches −27.7 dB at 48 k, so it is detected at 44.1 / 48 k and **missed at 96 / 192 k** (−30 dB threshold). Any burst ≥ 1 ms is detected at every rate. Detection lag after the true onset is 0.4–5 ms (HP + attack) | Gate design | The FUNC-08 click train uses **≥ 1 ms tone bursts**, never impulses. "Within 0 samples" is measured against the **detector's reported offset** (as CONTEXT says), not the stimulus onset. Add a 96 k row |
| **R62** | The newest f0 entry's window centre lags the head by **3613.5 samples (75 ms) at 48 k**: W/2·D + FIR 31.5 + 3 hops (R66). At `sN_position` 0 (the default), a grain reads content **newer than any entry**. Strict D17 ("missing → interval mode") makes follow a **no-op at default settings** | **D17 / FUNC follow vacuous** | Lookup rule: if the read point is newer than the newest committed entry, use the **newest** entry (the ARCH live-f0 behaviour, which is exact there). Only never-written, overwritten or unconfident entries fall back to interval mode. Add a gate row at position 0 |
| R63 | The f0 ring must be indexed in the **ring domain** (R23: W lags absCount after a freeze). A hop knows only the absolute sample of its window centre. Freeze is whole-piece (`ring.getWriteCount() == wPiece + (frozen ? 0 : len)`) | Design | Keep a **per-cell log** `{W at cell start, live}` of `ceil (lag/32) + 2` cells (≤ 452 at 192 k), sized in `prepareToPlay`. Centre in a live cell → `W = W_cell + (c − cellStart)`; centre in a frozen cell → no entry. Entry = `{int64 tag, float note (NaN = unconfident)}` at `floor (W/Hs) mod N`, N = `ceil (kRingSamples/Hs) + 2` (≈ 3.9 k, 62 KB). The tag rejects never-written and wrapped slots |
| R64 | Circular dependency: the read point depends on the delay; the delay depends on the rate (reach floor `margin + (r − 1)·len + 1`, frozen `floorOff = r·len + 1`); the rate depends on the follow interval | Design | **Two-pass plan for follow streams only.** (1) Run the existing plan in interval mode as a provisional plan. (2) Look up f0 at its content span. (3) Re-run the pitch → rate → delay steps with the follow interval. Draws are slot-keyed (`uni (Slot)` is pure), so no draw moves and nothing is drawn twice in a way that matters. Follow-off streams never take the path (N3) |
| R65 | If the 250 ms **hold** is written into the ring, any unconfident gap shorter than 250 ms reads confident, so the D17 "unconfident region → interval mode" row is vacuous for short gaps | Gate / design | The ring stores the **post-guard per-hop confidence with no hold**. The hold applies only to the live Pitch source (ARCH) |
| **R66** | D20 as written (median-of-3 + 2-hop symmetric hysteresis) suppresses **only 1-hop flips**, and median-of-3 alone already does that. The hysteresis delays the flip *and* the return by the same amount, so an L = 2 flip comes out as L = 2 (synthetic, §4). It also misses the **+19 st** (3rd-harmonic) lock, which happens **before** the octave lock: analytically H3/H1 ≥ 3.74 vs H2/H1 ≥ 4.36 | **D20 gate fails** | **Asymmetric lookahead confirm** (D20 amendment, sign-off). Median-of-3. Then a **harmonic-class** jump from the accepted note (±7.02, ±12, ±19.02, ±24 st, each ±50 ct) is accepted only if the next N = 3 medians agree within ±0.5 st. A return to the accepted note, or a non-harmonic move, is immediate. Measured: L ≤ 2 removed, L ≥ 3 passes, legit leaps are exact in the ring |
| **R67** | The CONTEXT D20 stimuli are **vacuous or impossible**. A breathy sine + white noise **never flips** (0 wrong hops even at SNR 0 dB: decimation removes ~80 % of the noise band). A strong 2nd harmonic **above** the 4.36 boundary flips **persistently**, which no causal guard should "fix", so a "≤ 1 hop" gate fails on it | Gate design | D20 rows: (a) **unit rows on injected hop sequences** (L = 1…5, legit 4th and octave leaps); (b) audio near the boundary, H2/H1 = 4.36 + noise SNR 20 dB (raw: 29 runs shorter than 3 hops → guard: 0), **with a guard-off negative control that must show short runs**; (c) persistent H2/H1 = 8 is documented as "tracks the octave", not gated |
| R68 | MPM model accuracy: worst **2.78 ¢** (1500 Hz at 44.1 k), ≤ 1.32 ¢ elsewhere, 40–1500 Hz at 44.1 / 48 / 96 k. Silence and noise: 0 confident hops (max clarity 0.18) | DSP-08 margin | DSP-08's 10 ¢ has a 3.6× margin. Keep ARCH k = 0.9 / clarity 0.8. Compute m(τ) only up to τ_max = ⌈fs_d/40⌉ + 1 (it is a running difference and loses precision toward W) |
| R69 | Exact lags (§3): window-centre lag `511.5·D + 31.5` input samples ≈ 41–47 ms; newest committed entry `+ 3·Hs` (median 1 hop + lookahead 2 hops) ≈ 71–82 ms; live Pitch source ≈ 75 ms + 20 ms slew | Constants | The constants are computed in `prepareToPlay` (`D`, `Hs = 128·D`). The timestamp of a median entry is the **centre of hop i − 1** (centred median). Document the source latency in the UI copy (Stage 3) |
| R70 | `quantise_abs` for non-1200 periods (Q4): GrainScatter has **no absolute precedent**. Its `rootNote` is cents above the unshifted input (`PitchEngine.h:148`). Bohlen-Pierce needs a register | Design / Q4 closed | Absolute set `A = {6000 + 100·root + c_d + k·P}` (MIDI cents; **1/1 at C4 = MIDI 60 + root**, the Scala default base). It is identical for every P = 1200 scale. Query reduced modulo P into one prebuilt `ActiveSet` (≤ 384 members for 64 degrees, < kMaxActive 512): O(log n) per grain, no per-grain rebuild |
| R71 | D4 × follow composition: `quantise_abs (f0 + pitch + pick)` would quantise twice (the pick is already a degree relative to the input) and bias the distribution | Design | Follow on, confident: centre `T = f0 + pitch`. pitch_rand > 0 → **uniform pick** among A ∩ [T ± span]. pitch_rand 0, or no member in span → **nearest** member (ties up). Interval mode returns "offset 0" in that case, but here that would make follow a no-op for a small spread. Then interval = t − f0, restricted to ±24 st by the window, as `ActiveSet::window` does now |
| R72 | D19 implicit chromatic, and what is measurable: output pitch = true f0 + (t − f0_est), so the sounding pitch lands within the **MPM error** (≤ 2.8 ¢), not within 1 ¢ | Gate design | Scale Off + follow → a static `equalDivision (12)` table, all degrees, the R70/R71 path (pick via `nextInt (Slot::Pitch)`). Gate D19 and absolute-quantisation rows on the **engine-recorded target t and interval** (spawn record), with sounding pitch as an INFO row |
| R73 | Onset index spaces (Q3). **Free re-phase needs no new space**: split the stream piece at i (`process (i)`, `restart()`, `process (n − i)`). `restart()` folds the onset count into the epoch, and `GrainScheduler::reset()` fires at the next processed sample = i, as index 0 of the new epoch (u₀ = ½, no jitter draw) | Design / Q3 closed | Free: reuse `restart()` (the R4 mechanism). Tempo extra grain: counter `2^63 | n_s` on `kGrainStream + s`, where n_s is a per-stream burst count zeroed only in `prepare()`. Disjoint from every grid counter (< 2^63) with **no Spawner signature change**. frac 0, no jitter. Per-stream onset list capacity `kCell + 1`; tie order: grid grain first |
| R74 | Feedback alignment (Q1): tapping the ring write (input + feedback) would make the MPM see its own grains, a positive pitch loop through the harmoniser. It would also make the analysis depend on the audio path, which removes N3's "the analysis is independent" proof | Q1 closed | Keep the **conditioned input** (2.5a default). Follow harmonises against the *source note*. Content that is feedback-heavy gets the input's f0 for that time. Document it, plus a DAW item (Stage 3) |
| R75 | Onset detector numerics (Q6): the TPT 1-pole HP `g = tan (π·150/fs)` is well conditioned up to 192 k. Envelope coefficients `1 − exp (−1/(τ·fs))` in double are fine in float (slow attack at 192 k = 6.5e-5). A per-sample `20·log10` is not needed | Q6 closed | Compare in **linear**: `fast > thrLin && fast > 1.9953·slow`, with `thrLin = 10^(thr/20)` at ticks. Denormals: `ScopedNoDenormals` is already in `processBlock` (`PluginProcessor.cpp:247`). Onset source analytic at the tick: `exp (−(tick − lastEventAbs)/τ)` (int64 event clock, no per-sample state) |
| R76 | FFT allocation (PERF-01): AppleFFT (vDSP, `JUCE_USE_VDSP_FRAMEWORK` defaults to 1) is in place with no allocation. FFTFallback (Windows) uses `alloca` for scratch below 256 KB: order 11 = 16.4 KB → **stack, no heap**. Results differ by ulps across the two backends | PERF-01 / INFO | The M-gate covers hops on macOS. Windows CI pluginval covers the fallback path. Follow-dependent goldens are platform-local (the harness already runs on one platform); never assert cross-platform bitwise f0 |
| R77 | CPU (Q7): one hop is **5.9–6.8 µs** mean on vDSP (worst 24–44 µs, which includes OS jitter) → ~0.06 % of a core at 94 hops/s. The detector (~10 flops/sample), the polyphase decimator (64 MAC per decimated sample) and 4 follow lookups ≈ 0.2 % | Q7 closed, INFO | **Don't spread the hop** across passes. The spike is < 1 % of a 32-sample block (667 µs at 48 k). **C6 ≈ C5 + 0.3 % ≈ 17 %**. Report the worst single 32-sample block as its own INFO row |
| R78 | Burst level. Tempo-mode extra grains (D18) are outside the R5 output law, which uses the grid density: at 1/16 @ 120 bpm (8 g/s) and a 50 ms refractory, the most that can be added is +20 g/s → up to **+5.4 dB** (incoherent). A Free re-phase is level-neutral on average | Accept, INFO | Accept as transient emphasis: the D9 knee bounds the wet signal. Quotas and R8 stealing apply unchanged. QUAL-01 compares against a **same-grain-count** reference. Add a DAW item |

---

## 1. Pass ordering and block-size invariance (R59)

`Engine::process` per piece: `gridTick` (cell start) → `CaptureWriter::write` (whole piece) → `env.process` (whole piece) → `scheduleStreams` → render + spawn at onset splits → mix → feedback chain. 2.5b adds the analysis next to `env.process`, so it also runs over the whole piece before any spawn in that piece.

- **Onset events** are consumed at their own sample i: a burst spawns at i, and a re-phase only changes onsets ≥ i. Nothing that happens *earlier* in the piece depends on an event found later in it. **This is safe.**
- **MPM hops** change shared state (the f0 ring, the newest-entry pointer, the live note). A spawn at offset o < hop offset h in the same piece would see the hop only when the piece reaches h, and that depends on the host-block split. **This is not safe.** Fix: `pendingHop` (≤ 1 per cell, because `Hs = 128·D ≥ 128 > 32`) is committed at the next `gridTick`, before `tickSources`. The Pitch source, the ring and the lookups all read committed state. The cost is ≤ 32 samples of extra lag, which is already inside R69's constants if the lag is defined from the commit tick.
- The **Onset source** is evaluated at the tick from `lastEventAbs`, so it is block invariant by construction (R75).
- Decimator phase `abs % D == 0` and the hop counter on the absolute decimated index (CONTEXT default) are both block-invariant. The FIR history persists across pieces.

PERF-03 row: the existing N8-style null at 32/441/512/4096/variable with triggers on (Free + Tempo), follow on ×4, Pitch/Onset routed. Add a **negative control** (hook `commitHopImmediately`) which, on a tonal stimulus with follow on and position 0, must DIFFER across block sizes. That proves the row can see the R59 class.

## 2. Onset detector (R60, R61, R75)

`onset.py`, ARCH parameters, −12 dBFS tone with a 5 ms attack (and a hard-gated 200 Hz tone):

| fs | 440 Hz, level-trigger (ARCH) | edge + re-arm +3 dB | 200 Hz level | 200 Hz edge | 1-sample 0 dBFS click | 1 ms burst −6 dBFS |
|---|---|---|---|---|---|---|
| 44.1 k | 1.8, **51.8, 101.8** ms | 1.8 | 0.4, **50.4, 100.6** | 0.4 | 1 | 1 |
| 48 k | 1.8, **51.8, 101.8** | 1.8 | 0.4, **50.4, 100.6** | 0.4 | 1 | 1 |
| 96 k | same | 1.8 | same | 0.4 | **0** | 1 |
| 192 k | same | 1.8 | same | 0.4 | **0** | 1 |

Why it re-fires: the slow envelope reaches `0.637·A·(1 − e^{−t/80 ms})`. The +6 dB condition stays true until about t ≈ 123 ms, which is longer than the 50 ms refractory.

Drum loop, `drums.py`: accent −6 dBFS and ghost −18 dBFS alternating, 30 ms decay, noise + 180 Hz:

| IOI | Level-trigger: hits / extra | Edge: hits / extra | Max detection lag (samples @ 48 k) |
|---|---|---|---|
| 250 ms | 12/12 / **1–3** | 12/12 / 0 | 24 |
| 125 ms | 23/23 / 0–1 | 23/23 / 0 | 123 |
| 70 ms | 20/40 / 1 | 20/40 / 0–1 | 19 |
| 55 ms | 26–27/51 / 0–1 | 26/51 / 0 | 29–45 |

At IOI ≤ 70 ms the ghosts sit under the accent's slow envelope and are not detected in either mode. That is ARCH behaviour (relative +6 dB), not a regression. FUNC-08's "hit rate" row should use **equal-level** bursts at IOI ≥ 60 ms.

**Recommended detector (ARCH §8 erratum, sign-off):**

```
per sample (conditioned mono, post input_gain):
  hp   = TPT 1-pole HP 150 Hz;  a = |hp|
  fast += (a > fast ? cFa : cFr)·(a − fast);   slow += (a > slow ? cSa : cSr)·(a − slow)
  if (! armed && fast < kRearm·slow) armed = true          // kRearm = 10^(3/20)
  if (refr > 0) --refr
  if (armed && refr == 0 && fast > thrLin && fast > kRatio·slow)   // kRatio = 10^(6/20)
      emit event at abs; lastEventAbs = abs; refr = round (0.05·fs); armed = false
  NaN-sticky: any non-finite state → zero state, armed = true (scrub counter)
```

`thrLin` is latched at ticks. Coefficients are computed in double in `prepareToPlay`.

## 3. MPM, decimator and lags (R68, R69)

**Accuracy** (`mpm.py`, sine −12 dBFS, worst |¢| over confident hops; every hop is confident):

| fs (D) | 40 | 55 | 110 | 220 | 440 | 700 | 1000 | 1500 Hz |
|---|---|---|---|---|---|---|---|---|
| 44.1 k (4) | 1.20 | 0.88 | 0.16 | 0.85 | 0.93 | 0.36 | 0.22 | 2.78 |
| 48 k (4) | 1.32 | 0.97 | 0.26 | 0.74 | 1.02 | 0.15 | 0.24 | 0.18 |
| 96 k (8) | 0.83 | 0.68 | 0.43 | 0.10 | 0.39 | 0.50 | 0.41 | 0.18 |

**Lags** (input samples; `Hs = 128·D`; the centre of the window of decimated samples `[h − 1024, h)` sits `511.5·D + 31.5` behind the hop sample):

| fs | D | Hs | Window-centre lag | + median (1 hop) + lookahead (2 hops) = newest entry | ms |
|---|---|---|---|---|---|
| 44.1 k | 4 | 512 | 2077.5 (47.1 ms) | 3613.5 | 81.9 |
| 48 k | 4 | 512 | 2077.5 (43.3 ms) | 3613.5 | 75.3 |
| 88.2 k | 7 | 896 | 3612 (41.0 ms) | 6300 | 71.4 |
| 96 k | 8 | 1024 | 4123.5 (43.0 ms) | 7195.5 | 75.0 |
| 192 k | 16 | 2048 | 8215.5 (42.8 ms) | 14 359.5 | 74.8 |

(R59's tick commit adds ≤ 31 samples on top of these.)

**Clarity under noise and silence:**

| Stimulus | Confident hops |
|---|---|
| Silence | 0/133 |
| White −20 dBFS | 0/133 (max clarity 0.163) |
| White −60 dBFS | 0/133 (max 0.162) |
| Brown −20 | 0/133 (max 0.181) |

The DSP-08 clarity row holds with a wide margin. Add a row for the 250 ms hold → unconfident transition on the live source.

## 4. Octave guard (R66, R67)

**Analytic boundaries** (stationary two-partial tone, NSDF at the sub-period lag vs k = 0.9):
- At τ = T/2: `(a₂² − a₁²)/(a₁² + a₂²) ≥ 0.9`, so an octave-up lock occurs at **H2/H1 ≥ √19 = 4.36**.
- At τ = T/3: `(a₃² − ½a₁²)/(a₁² + a₃²) ≥ 0.9`, so a twelfth-up lock (+19.02 st) occurs at **H3/H1 ≥ √14 = 3.74**. That comes before the octave lock.

Measured: H2/H1 = 8 gives +12 on 133/133 hops. H3/H1 = 4 gives +19 on 133/133. H3/H1 3.5 and H2/H1 4.2 give 0. AM at f0/2 (depth 0.5) gives a persistent −12 (a real period doubling). Missing fundamental (H2–H4) is tracked correctly.

**Stationary stimuli flip all-or-nothing.** Breathy sine + white noise: 0 wrong hops at every SNR down to 0 dB. Intermittent flips only appear **near a boundary**:

| Stimulus (220 Hz, 6 s, 555 hops) | Raw: wrong / runs shorter than 3 hops | median-of-3 | D20 as written | Asymmetric N = 3 |
|---|---|---|---|---|
| H2/H1 4.36, SNR 20 dB | 88 / 39 | 69 / — (max run 11) | identical to median | 67 / **0 short runs** |
| H2/H1 4.36, SNR 12 dB | 216 / 27 | 220 | identical | 219 / **0** |
| H2/H1 4.5, SNR 12 dB | 491 / 2 | 511 | identical | 500 / **0** |

**Synthetic injected flips** (`guard.py` / `guard2.py`): wrong-run length in the output for a raw flip of length L:

| L | median-of-3 | median + symmetric 2-hop confirm (D20 as written) | median + asymmetric N = 3 |
|---|---|---|---|
| 1 | 0 | 0 | 0 |
| 2 | 2 | 2 | **0** |
| 3 | 3 | 3 | 3 |
| ≥ 4 | L | L | L |

**Legit leaps** (57 → 69 and 57 → 62) with the asymmetric form: the accepted value switches at the first new median (1 hop, from the median) and is **exact in the ring**, because the ring entry for hop j is written once hop j + 2 is known. The live source carries the 2-hop decision lag uniformly (R69).

The guard does not fix the long ambiguous stretches near a boundary: total wrong hops barely change. That is physical ambiguity, and it is documented. The gate is "no short flips survive", not "no wrong hops".

**Recommended D20 amendment (sign-off):**
- Confident-only median-of-3. Fewer than 2 confident hops out of 3 → unconfident.
- Harmonic-class set {7.02, 12, 19.02, 24} st, each ±0.5 st, both signs.
- N = 3, lookahead form.
- Ring entries are written at decision time and tagged with the centre of hop j (R63).

## 5. f0 history ring (R62–R65)

**Write side.** At each committed hop (R59) with a decided value for hop j (R66), compute the centre `c_j` (absolute samples). Map it through the cell log:
- live cell → `W = W_cell + (c_j − cellStart)` → `slot = floor (W/Hs)`, write `{tag = slot, note}`
- frozen cell → skip

`note = NaN` when unconfident. **There is no hold in the ring (R65).**

**Read side** (spawn, follow streams only):
1. Compute the provisional plan in interval mode (R64), with `readStart = W_on − D` and `rl = r·len`.
2. Lookup point: **start** (D17 as locked) or **content midpoint** `readStart + dir·rl/2` (recommended, sign-off). For long grains, start-only is wrong for most of the grain. Size can be 2 s at rate 4, which is 8 s of content. Midpoint ≈ majority for a grain that straddles 2 notes. Same cost.
3. `k = floor (Wq/Hs)`. If `k > kNewest` → use `kNewest` (R62). Else, if `tag[k mod N] != k` → miss. Then NaN → miss. **Miss → interval mode.**
4. Re-plan pitch → rate → delay (R64). The draws are unchanged.

**Freeze:** nothing is written while frozen. The held span keeps its entries. A pos-0 frozen grain reads near `W0`, which is newer than `kNewest`, so it uses the last pre-freeze entry. That is the right note.

**Sizing:** `N = ceil (kRingSamples (fs)/Hs) + 2`. That is 3532 entries at 44.1 k, 3844 at 48 / 96 / 192 k and 4036 at 88.2 k, at 16 B each. The cell log needs `ceil (14 360/32) + 2 = 451` cells at 192 k.

**Gate rows:**
- D17 rows as in CONTEXT.
- A **position-0 row**: follow on with a sine at 220 Hz → target from 220 Hz. With strict D17 this row reads interval mode, so it is the R62 negative control.
- A **wrap row**: a slot overwritten after 41 s must miss.
- **Freeze-edge rows:** the centre in the last live cell is written; the first frozen cell is not.

## 6. Absolute quantisation (R70–R72)

- Root reference: `R = 6000 + 100·root` ¢ (C4 = MIDI 60 + root). For every 1200-period scale, any register gives the same set. Bohlen-Pierce (P = 1901.955) and Custom periods ≠ 1200 are anchored at C4 + root. This follows Scala's default 1/1 = 261.63 Hz.
- Prebuilt set per stream, **rebuilt at ticks** on (choice, mask, span, customVersion, abs flag): members `c_d + k·P` over `[−span − P, span + P]`, where span = pitch_rand·100.
- Per grain:
  - `o = T − R`, `k0 = floor (o/P)`, `q = o − k0·P` ∈ [0, P)
  - window `[q − span, q + span]`, intersected with the ±24 st interval clamp `[f0 − 2400 − R − k0·P, f0 + 2400 − R − k0·P]`
  - pick, or take the nearest; then `t = R + k0·P + member`
  - All in double.
- Size bound: 64 degrees × (2·(2400 + P)/P + 2) ≤ 384 for P = 1200. Smaller periods use the existing outward truncation (`wasTruncated` telemetry).
- D19: a static `equalDivision (12)` table, full mask (degree masks are ignored), the same path.
- Gate rows read the spawn record. Add `rec.f0`, `rec.target`, `rec.followHit` under `OGRAN_TEST_HOOKS`. The ±24 clamp row uses f0 at MIDI 28 and 100 with pitch ±24.

## 7. Onset → scheduler (R73, R78)

- **Free (D14):** in `scheduleStreams`, for each enabled trigger-on stream with an event at offset i in the piece:
  1. `process (i)`
  2. `restart()` (the epoch fold)
  3. `process (len − i)`, with offsets shifted by i
  
  The re-phase grain is index 0 of the new epoch, at frac 0. `epoch` grows by ≥ 1 per event, so no draw replays. At most one event lands per piece (the refractory is ≥ 2205 samples), so one split per piece is enough.
- **Tempo (D18):** keep `processTempo` unchanged. Insert `{offset i, frac 0, globalIndex = 2^63 | n_s++}` into the stream list in offset order, after any grid onset at i. Size `streamOnsets[s]` to `kCell + 1` and the merged list to `kMaxStreams·(kCell + 1)`. The slot, `tIndex` and `lastEmit` are untouched (TS1 and the item 6 watch).
- **Disabled stream:** no split, no draw. **Frozen:** fires as normal (frozen geometry).
- **Level (R78):** grid-density-based quotas and the R5 law are unchanged. The QUAL-01 reference renders the same onset instants as plain grid grains (an equal-count control), so a click verdict isn't confounded by a level change.

## 8. Sources, telemetry, cost (R75–R77)

- **Pitch source:** committed live note (with hold) → `clamp ((note − 24)/84, 0, 1)`, then a one-pole at tick rate `a = 1 − exp (−32/(0.02·fs))`, held while unconfident.
  - Initial value: 0 until the first confident hop. This is a recommendation; confirm at plan.
  - The 40–1500 Hz range uses only 0.04–0.79 of the source. That is INFO for the Stage 3 copy.
- **Onset source:** `exp (−(tick − lastEventAbs)/(0.15·fs))`. Treat "150 ms" as the time constant τ (ARCH wording). It is 0 before the first event.
- **Telemetry:** the existing source atomics at the cell. Gate: they equal the engine values.
- **CPU:** see R77. The hop benchmark is in `bench.cpp`.

## 9. Open questions → answers

| CONTEXT question | Answer |
|---|---|
| f0 ring under feedback | R74: keep the conditioned input. Follow tracks the source note. Documented, plus a DAW item |
| MPM group delay; straddling grains | R69 constants (`511.5·D + 31.5`, plus a centred median at hop i − 1). R64/§5: lookup at the content midpoint (sign-off) |
| Onset index space in Free | R73: none needed. `restart()` epoch fold. Tempo: `2^63 | n_s` |
| `quantise_abs` non-1200 periods | R70: 1/1 at C4 + root. There is no GrainScatter absolute precedent |
| D20 threshold, sub-octave, fifth | R66: harmonic class ±7/12/19/24 both signs. The symmetric hysteresis is replaced by an asymmetric N = 3 lookahead (sign-off) |
| Detector at 96/192 k | R75: TPT stable, linear compare. R61: impulses are rate dependent, so stimuli are ≥ 1 ms bursts |
| Hop CPU spike | R77: ~6 µs; do not spread |

## 10. Sign-offs needed at plan (Taylor)

1. **R60:** edge-triggered detector with +3 dB re-arm (ARCH §8 erratum).
2. **R66:** D20 amendment, asymmetric harmonic-class lookahead confirm N = 3 (replaces the symmetric 2-hop octave hysteresis).
3. **R64:** f0 lookup at the content **midpoint** of the provisional plan (narrows D17's "read start").
4. **R71:** absolute mode with pitch_rand 0 / no member in span snaps to the **nearest** member (interval mode keeps "offset 0").

## 11. Carry-ins for plan

- **N3 first:** an out-of-tree null vs `2cf30120` in `tests/n3/` (the n2 pattern). The analysis runs in the neutral case. The four negative controls and the `onset_threshold` positive control are as in CONTEXT.
- Add R59's `commitHopImmediately` negative control to the PERF-03 row.
- W2o (0.017 dB): if it flips, widen the seeds and never touch the threshold.
- Update the stage comments at `Engine.h:58` ("analysis — 2.5b") and `ModSources.h:64`.
- `DAW-2.5b.md` at execute:
  - onset bursts on drums (Free + Tempo)
  - follow on a monophonic line at position 0 and at 2 s
  - frozen-chord harmonisation
  - a feedback-heavy follow (R74)
  - the Tempo burst level (R78)
