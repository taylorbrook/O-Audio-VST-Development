# Research: Band-limited grain reader (mipmap + polyphase sinc)

**Plugin:** O-GrainScatter (first consumer) + `modules/synthesis/grain-engine`
**Milestone:** band-limited-reader
**Created:** 2026-10-27
**Phase:** Research
**Scripts:** `research-scripts/` (numpy/scipy; every number below comes from them)

---

## Context Summary

- **Core requirement:** a mip cascade (half-band ÷2, L = 3) on the ring write path, plus a 16-tap polyphase windowed-sinc bank for the residual ratio. Group delay is compensated and every phase has unity DC. GrainScatter's live ring and freeze snapshot read through it.
- **Scope:** a new header in grain-engine (1.0.0 → 1.1.0); GrainScatter 3.2.0 → 3.3.0. No parameter, state or UI change.
- **Success:** alias rejection ≥ 60 dB in the audible band (+12/+24/+36 st, with and without Doppler); levels time-aligned; existing gates hold or are re-baselined with a documented reason; CPU measured.

---

## Key numbers (the findings that shape the plan)

### 1. Baseline: v3.2.0 Lagrange-4 aliasing is severe

Worst spurious component in the audible band (20 kHz), for source content in the audible band:

| r | 1.06 | 1.5 | 2 | 4 | 8 |
|---|---|---|---|---|---|
| 44.1 k | −10 dB | −2 dB | −1 dB | 0 dB | 0 dB |
| 48 k | −12 dB | −3 dB | −1 dB | 0 dB | 0 dB |

From +12 st up, aliases come out at full level. Lagrange's −1 dB passband, averaged over fractions, ends at 12.4 kHz (44.1 k) / 13.5 kHz (48 k). Any band-limited reader is a large win. The negative control will fail the alias gate by roughly 50–60 dB.

### 2. Half-band cascade: 127-tap Kaiser, β = 9

- Spec: the junk band from decimation aliasing must land above 20 kHz when level ℓ is read at r′ ≥ 1. So the transition is 0.25 ± δ with δ = (0.5 − 20/fs)/2. 44.1 k is the strictest case: δ = 0.0232.
- Results with a Kaiser half-band (exact zeros on even offsets, `hb2.py`):

| N | nonzero | 44.1 k stopband | delay |
|---|---|---|---|
| 79 | 41 | −58 dB | 39 |
| 103 | 53 | −74 dB | 51 |
| **127** | **65** | **−90 dB** (β 9) | **63** |
| 151 | 77 | −107 dB | 75 |

- **Recommend N = 127, β = 9, one fixed design at every sample rate.** At 48 k and above the same filter is better than −90 dB.
- Cost: with symmetric folding, 32 multiplies per level output. Level ℓ runs at fs/2^ℓ, so the whole cascade costs ≈ 28 MAC per input sample per channel. That is negligible.
- Accumulated delay in level-0 samples: **D(ℓ) = 63·(2^ℓ − 1) → 63 / 189 / 441.**

### 3. Group-delay mapping, verified in simulation (`align.py`)

The cascade is causal. Level-ℓ sample j is produced on the odd parent index (`y[j] = Σ hb[k]·parent[2j+1−k]`). The level-ℓ index for absolute input time p₀ (level-0 samples, double) is then:

```
p_ℓ = (p₀ + D(ℓ) − (2^ℓ − 1)) / 2^ℓ        ℓ = 1..3
```

Level 1/2/3 reads at p_ℓ match the level-0 read at p₀ to −90 / −90 / −86 dB re rms, on content below fs/32. So there is no time jump between levels. The module test should check this same identity with a transient, at ≤ 0.05 samples.

### 4. Read guard: settled history per level

A level-ℓ read at p₀ needs level samples up to p_ℓ + 8 (half of 16 taps). Those exist only D(ℓ) + 8·2^ℓ input samples after p₀. Margin per level (delay from the write head, level-0 samples):

| ℓ | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| margin = D(ℓ) + 8·2^ℓ + 1 | **9** | **80** | **222** | **506** (≈ 11.5 ms @ 44.1 k) |

- Today `kReadMargin = 4`. It becomes `margin(ℓ)`, using the grain's level latched at spawn.
- Grains pinned to the floor move older by 5 samples at unity and by up to 11 ms at +36 st.
- `densityRho`'s `floorS` (`PluginProcessor.cpp:648`) uses `+ 4.0` and must use the same margin.

**Pre-existing bug found: Doppler escapes the spawn guard.** The pre-roll uses `playbackRate` only. The spatial path then advances by `playbackRate · dopplerFactor` (`GrainPool.h:405`), with Doppler up to 2. A forward grain near the floor therefore drives its tap below 0. That is ahead of the write head, and the read wraps to audio 12 s old. **Recommend a read-time clamp** `delay = max(delay, margin(ℓ))` inside the reader path. It is one `max` per read, it fixes the hole, and it is a no-op for every grain that respects the guard today. It changes output only for grains that currently read wrapped audio.

### 5. Sinc bank: 16 taps meets −60 dB for r′ ≤ 2, but costs top-octave bandwidth

The optimiser (`run3.py`) searches each r′ for the Kaiser (fc, β) that maximises the output −1 dB passband with every audible spurious component ≤ −60 dB. Output passband, kHz:

| r′ | 0.5 | 0.707 | 1.0 | 1.19 | 1.414 | 1.68 | 1.99 | 2.83 | 3.99 |
|---|---|---|---|---|---|---|---|---|---|
| **T16 @44.1** | 8.0 | 11.2 | 15.9 | 18.9 | 12.6 | 11.0 | **9.1** | FAIL | FAIL |
| T24 @44.1 | 9.3 | 13.1 | 18.5 | 22.2* | 16.5 | 15.2 | 13.5 | 9.4 | 4.5 |
| T32 @44.1 | 10.0 | 14.1 | 19.9 | 22.8* | 18.4 | 17.2 | 15.9 | 13.1 | 9.2 |
| **T16 @48** | 9.6 | 14.1 | 19.2 | 23.0* | 17.4 | 13.6 | **10.9** | 5.3 | FAIL |
| T24 @48 | 10.0 | 14.1 | 22.1 | 24.2* | 20.5 | 18.4 | 16.4 | 11.9 | 6.4 |

\*Above 20 kHz: for r′ ≤ 2(1 − 20/fs) no alias can land below 20 kHz, so the optimiser opens the cutoff. r′ < 1 at ℓ ≥ 1 is capped by the half-band band edge (20 kHz · r′).

What this means for the decided design (16 taps, ⌊log₂ r⌋ latch):

- **Alias target met** for every r′ ≤ 2, which covers the whole stereo path and the spatial path with Doppler ≤ ~1.4 at the top of an octave.
- **Timbre swings within each octave.** The −1 dB passband runs from ~16–19 kHz just above an octave boundary (r′ → 1) down to 9.1 kHz (44.1 k) / 10.9 kHz (48 k) just below the next one (r′ → 2). There is a step of about 7 kHz across each boundary: +11.9 st is duller than +12.1 st.
  - This is still at least the Lagrange passband, with aliases at −60 dB instead of −1 dB.
- **r′ ∈ (2.8, 4)** (only reached through Doppler > 1.4 on a grain near the top of an octave) **cannot reach −60 dB with 16 taps.** Use a best-effort set there.
  - In practice Doppler is ~1 ± 0.01. `1 + amt·dY·50`, where dY is the per-sample change of sin(az)·cos(el), so the 2× clamp is only hit on trajectory jumps.
- **Levers if the passband swing is audible** (listening check in verify):
  - T24 raises the worst case to 13.5 kHz at +50 % reader CPU; T32 raises it to 15.9 kHz at 2×.
  - A round-to-nearest latch (r′ ∈ [0.707, 1.414)) gives a worst case of ≈ 11.2 kHz and a smaller step. But it needs an alias-free half-band (N ≈ 255, stopband from 0.25, margin ≈ 2×), because at r′ < 1 the half-band junk band becomes audible. It also reverses a user decision. **Not recommended now.**

**Recommended bank (16 taps, matches the decisions):**

- **8 sets.** S0 handles r′ ≤ 1 (image-rejecting, cutoff ≈ 0.84 of Nyquist at 44.1 k). S1–S6 have upper edges 2^(k/6), k = 1..6, covering r′ ∈ (1, 2]. S7 is best-effort for r′ ∈ (2, 4).
- **Set choice.** A grain uses the set whose upper edge is ≥ r′, so the alias target always holds. Stereo path: chosen at spawn (rate is constant). Spatial path: re-chosen per sample from the live r′, a table lookup. The set boundaries are at most 2^(1/6), so the cutoff step when Doppler crosses a boundary is a small HF change with no level change, because DC is unity in both sets.
- **Fixed design.** Design at a = 20/44.1 normalised, the strictest case, so the tables don't depend on fs. Build them at static init or construction, never in `processBlock`. Hard-code the (fc, β) per set, produced offline by `run3.py`-style search: at 44.1 k the T16 optimum runs c ≈ 0.84 → 0.64 of the set's Nyquist, β 5.5–6.
- **Phases.** 256 phases with linear interpolation between rows (`phase.py`): worst kernel error −98 dB. 257 × 16 floats × 8 sets = **128 KB**. (128 phases gives −86 dB / 64 KB.)
- **Unity DC.** Normalise each row to sum = 1 in double, then store as float; the test is 1 ± 1e-6. A lerp of two unity rows is unity, so interpolated phases inherit it.
- **Kernel alignment.** For pos = i₀ + fr, read taps i₀−7 … i₀+8: symmetric, 8 newer samples (hence the margin of 9 at ℓ = 0).

### 6. Freeze snapshot: copy the live levels, end the capture at head − 506

- **Copy, don't rebuild.** The live levels already hold continuous filter history, so copying is exact at the old edge. Rebuilding from level 0 at capture gives edge transients and costs CPU on the audio thread. The memcpy is ≈ 1.875× today's (≈ 1.4 MB at 48 k, one block).
- **Newest-edge problem.** At capture, level ℓ has not yet received the newest D(ℓ) + 8·2^ℓ input samples. Frozen grains wrap around the whole loop, so they would read samples that don't exist yet.
  - **Recommend:** end the snapshot (every level, level 0 included, so all levels agree) at `head − margin(3)` = 506 samples older. Freeze then captures audio ending ≈ 10 ms before engage. That is inaudible as a timing change; document it.
- **Old edge and seam.** The loop seam crossfade reads `b` and `b − lp`. The reader is linear, so blending two level-ℓ reads equals reading the blend. Copy 8·2^ℓ + 1 extra level-0-equivalent samples of pre-roll at the old end, so kernels never clamp.
- **Per-slot, per-level fractional offset.** The mapping offset `D(ℓ) − (2^ℓ−1)` is 62 / 186 / 434, not divisible by 2^ℓ for ℓ ≥ 2. Store the snapshot's level-ℓ origin as a double per slot instead of forcing alignment.

### 7. CPU and memory estimates (to be measured in verify)

- **Reader, worst case** (72 slots active, stereo): per grain-sample, 16 phase lerps + 32 MAC ≈ 50 flops, so ≈ 3.6 kflop/sample ≈ 170 MFLOP/s at 48 k. Expect a few % of a core with NEON/SSE, against Lagrange's 8 MAC plus 8 `%` today.
  - **Ring mirroring** removes the per-tap modulo: each level ring gets a 16-sample duplicated guard, written twice, so a 16-tap window is always contiguous. Today's code does 4 `%` per channel per read.
  - **Spatial path:** sum the L and R taps before convolving (linear, same result), so 16 MAC instead of 32.
- **Memory:** levels add ½ + ¼ + ⅛ = 0.875× the ring.
  - 12 s stereo float at 192 k: 18.4 MB → 34.6 MB.
  - Freeze: 2 slots × 2 s × 1.875.
  - Allocate all of it in `prepare`; `clear()` zeroes it without `setSize` (CR-02 rule).

---

## Implementation Approach

### Recommended approach

**Strategy:** mirrored mip ring with a causal half-band cascade, plus a fixed polyphase sinc bank, as one header in grain-engine.

- **New header** `modules/synthesis/grain-engine/cpp/BandlimitedReader.h`, plain C++20 with no JUCE, namespace `ouaricon::grain`:
  - `HalfbandKernel`: 127-tap constants, symmetric folded.
  - `MipRing<L = 3>`:
    - Stereo, mirrored guard, absolute `int64` write counter.
    - `push (l, r)` runs the cascade per sample, so it is block-size invariant by construction.
    - `levelIndex (ℓ, p₀)` implements the mapping in §3.
    - `margin (ℓ)`.
  - `SincBank`: 8 sets × 257 phases × 16 taps, unity DC, built in the constructor.
  - `read (ring, ℓ, ch, p₀, set)` / `readMono (ring, ℓ, p₀, set)`. These return the tap; the caller's tap delay is clamped to `margin(ℓ)`.
  - `MipSnapshot`: freeze slot storage with every level, `captureFrom (ring, endDelay, length)`, and per-level origin offsets.
  - Helpers: `latchLevel (rBase)` = clamp(⌊log₂ r⌋, 0, 3), and `setFor (r′)`.
- **GrainScatter:**
  - `DelayBuffer` keeps its interface but holds a `MipRing`, so `pushSample` feeds the cascade.
  - `GrainVoice` gains `level` (latched at spawn from `playbackRate`, which already includes the freeze detune) and `sincSet`.
  - Both render paths read through the bank. The stereo path uses the latched set; the spatial path uses the set from the live r′.
  - `FreezeManager` slots become `MipSnapshot`s, with the seam and wrap logic unchanged and run on level coordinates.
  - The spawn guard and `floorS` use `margin(ℓ)`, and the read-time clamp covers Doppler.
  - `CMakeLists.txt` gains `ouaricon_add_module(O-GrainScatter grain-engine)`. GrainScatter does not link the module today.

**Pros:**
- Meets the alias target across the whole non-Doppler range.
- Time-aligned levels.
- Deterministic and block-size invariant.
- The tables don't depend on fs, so there is no work in `prepare` beyond allocation.

**Cons:**
- At 16 taps the passband swings across each octave, as described in §5.
- About 11 ms more pre-roll for +36 st grains.
- Freeze captures 10 ms earlier.
- Unity timbre changes: flatter top than Lagrange, though not bit-identical to it.

### Alternatives considered

- **Polyphase IIR half-band** (allpass pairs): cheaper, but its group delay is not constant. That breaks the alignment requirement, so it is rejected (as CONTEXT expected).
- **Rebuild freeze levels at capture:** edge transients plus audio-thread CPU. Rejected (§6).
- **Round-to-nearest latch:** a better worst case, but it needs a 2× half-band and reverses a decision. Kept as a fallback (§5).
- **Direct high-tap sinc without mipmaps:** 2·N_z·r taps, 256 taps at r = 8. Not viable (research §3.3).

---

## Affected Components

| File | Change | Complexity |
|---|---|---|
| `modules/synthesis/grain-engine/cpp/BandlimitedReader.h` | **new**: MipRing, SincBank, MipSnapshot, helpers | High |
| `modules/synthesis/grain-engine/tests/grain_engine_tests.cpp` | DC per phase; level alignment ≤ 0.05 smp; half-band ≤ −90 dB; set stopbands; block-split invariance of `push`; alloc-free | Medium |
| `modules/synthesis/grain-engine/module.yaml`, `README.md` | 1.1.0, provides, `used_by` O-GrainScatter | Low |
| `plugins/O-GrainScatter/Source/dsp/DelayBuffer.h` | wraps MipRing; `readSample` replaced by a level/set read; `copyRegion` → snapshot capture | Medium |
| `plugins/O-GrainScatter/Source/dsp/FreezeManager.h` | slots → MipSnapshot; capture end at head − margin(3); level-aware `readSample` | Medium |
| `plugins/O-GrainScatter/Source/dsp/GrainPool.h` | `level`/`sincSet` on voice; both read sites; read-time margin clamp; mono L+R tap sum | Medium |
| `plugins/O-GrainScatter/Source/PluginProcessor.cpp` | spawn guard → margin(ℓ) (`:778-785`); `floorS` (`:648`) | Low |
| `plugins/O-GrainScatter/CMakeLists.txt` | `ouaricon_add_module(... grain-engine)` | Low |
| `plugins/O-GrainScatter/tests/` | new `alias-gate` harness (v3.2.0 backup as negative control); updates in §Risks | Medium |
| `LagrangeInterpolation.h` | becomes unused by the grain path. Keep it unless nothing references it | Low |

No JUCE DSP modules are needed and there are no external dependencies.

---

## Risks to existing gates (flag before plan)

1. **`blocksize-null` row "ring read precision, 192 kHz, head > 2^21" (≤ −100 dB against the analytic sine).**
   - The row tests position precision through Lagrange. The sinc's near-DC deviation will not reach −100 dB against the analytic value.
   - **Recommend:** compare the reader at head > 2^21 against the same reader at a small head (same fractional delay). That isolates double-position precision, which is the row's purpose.
2. **`density-gain` "sparse" row (null against v3.1.0 to ≤ −70 dB, `pitch_random` 50).** The interpolator changed, so this row will fail. Its purpose is "gain is exactly 1 below one grain".
   - **Recommend:** keep the gain claim and change the reference. Either use the v3.3.0 build with density compensation forced off, or a baseline built from the 3.2.0 backup with the reader patched in. Plan decides; it must not be weakened silently.
3. **`density-gain` incoh/coh/freeze rows:** unity pitch, so they are expected to hold. With pitch on, noise loses its folded alias power, so levels can drop by several dB. That is correct, not a regression; re-baseline only with a measured, documented reason.
4. **pluginval 10 / auval:** no state change. The memory growth at 192 k is noted (§7).

---

## Gates to build (verify)

- **`alias-gate`** (new, GrainScatter, offline harness like `blocksize-null`):
  - Stimulus: 20 kHz-band-limited sine sweep and fixed tones, at +12/+24/+36 st, plus +36 st spatial with forced Doppler 2.
  - Verdict: worst audible spurious ≤ −60 dB. The v3.2.0 backup must FAIL as the negative control (expect ~0 dB).
  - Also include a frozen +36 st row, and r = 1.99 vs 2.01 alignment (cross-correlation peak lag ≤ 0.05 smp on a transient).
- **Passband table:** measured output −1 dB per r′, compared with §5, plus listening across +11 → +13 st. This is the user's check on the 16-tap choice.
- **CPU:** 64 grains at +24 st, stereo and spatial, ns/sample against v3.2.0.
- **Existing:** blocksize-null (all rows, with the reworked precision row), density-gain, pluginval 10, auval.

---

## Open decisions for plan (with recommendations)

| # | Decision | Recommendation |
|---|---|---|
| 1 | 16 taps, given the measured 9.1 kHz worst-case passband at r′ → 2 (44.1 k) | **Keep 16** (decided). Listening check in verify; T24 is a one-constant switch if needed |
| 2 | r′ ∈ (2.8, 4) can't reach −60 dB at 16 taps | Best-effort S7 set; document it. Realistic Doppler stays near 1 |
| 3 | Freeze captures 10 ms earlier (capture end at head − 506) | Accept |
| 4 | Fix the Doppler guard hole with a read-time clamp | Include (one `max`; only changes grains that read wrapped audio today) |
| 5 | Rework the two existing gate rows (Risks 1–2) | As recommended above |
| 6 | Version | GrainScatter **3.3.0** (MINOR: audible change, no state change); grain-engine **1.1.0** |
