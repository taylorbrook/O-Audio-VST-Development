# Research: Signal-adaptive grain length in freeze (extrema density)

**Plugin:** O-GrainScatter 3.4.0 → 3.5.0
**Milestone:** adaptive-grain-length
**Created:** 2026-10-28
**Phase:** Research
**Prototype:** `research-scripts/extrema_proto.cpp` (standalone C++17, `c++ -std=c++17 -O2`), output in `research-scripts/proto_output.txt`. Every number below comes from that program, run on an Apple M4 Max.

---

## Context Summary

- **Core requirement:** a new `grain_adapt` knob (0–100 %, default 0). While Freeze is held, each new grain's length follows the local extrema density of the snapshot (Nielsen DAFx26 span L, eq. 12). Grain Size stays the mean length. Adapt 0 must render bit-exact against v3.4.0.
- **Scope:** frozen grains only. The analysis runs once per engage. A spawn costs one O(log M) lookup. The change also adds a parameter, a UI knob with tooltips in EN/FR/zh, and a new gate.
- **Success:** sparse material gets measurably longer grains than dense material. The mean length stays within tolerance of Grain Size. No allocations, a bit-exact block-size null, flat level, and the engage cost is measured.

---

## Key numbers

| Quantity | 48 kHz | 96 kHz |
|---|---|---|
| Loop build (mid + seam crossfade), 2 s | 36–47 µs | 53–65 µs |
| Extrema pass, white noise (worst real case, M/N 0.49) | **431 µs** (4.5 ns/sample) | **688 µs** (3.6 ns/sample) |
| Extrema pass, adversarial decaying Nyquist square (M/N 0.89) | 315 µs | 566 µs |
| Extrema pass, 55 Hz sine (branch-predictable) | 91 µs | 148 µs |
| Calibration (K0 fixed-point, 4 iterations × 256 spans + nth_element) | 20–70 µs | 16–59 µs |
| Chunked (2048-sample steps) vs one-shot | identical output, same total cost | identical |
| Per-block c-solve (fast: 256 exp + prefix sums + 40 × 2 binary searches) | 2.3 µs | — |
| Per-spawn lookup (binary search over M = 47 k + span + log/exp) | 63 ns | — |

Extremum counts, 48 k, 2 s loop (N ≈ 95.5 k):

| Material | M | M/N | mean gap |
|---|---|---|---|
| sine 55 Hz | 220 | 0.0023 | 434 samples |
| sine 1 kHz | 3 980 | 0.042 | 24 |
| white noise ±0.3 | 47 030 | 0.492 | 2.0 |
| noise −50 dB (±3e-3) | 28 603 | 0.299 | 3.3 |
| noise −80 dB (±1e-4) / silence | **0** | 0 | — |
| 1 s sine 55 + 1 s noise (equal RMS) | 24 055 | 0.252 | 4.0 |
| bass 60 Hz + snare bursts | 24 821 | 0.260 | 3.8 |
| skip-tilt "music" stimulus | 4 723 | 0.049 | 20.2 |
| sine fs/4 | 47 760 | 0.500 | 2.0 |
| **adversarial**: decaying Nyquist square (even 0.9 − δn, odd −0.9 + δn) | **85 173** | **0.892** | 1.1 |

---

## Q1 — Analysis algorithm, engage cost, single block vs spread, storage bound

**Algorithm, as prototyped** (`Analyzer` in the prototype):

1. **Signal: the mid channel, (L + R) / 2, of the loop as it is heard.** One period covers `b ∈ [xfade, length)`. Inside the seam region the equal-power crossfade is applied exactly as `FreezeManager::locate` (`Source/dsp/FreezeManager.h:168`) applies it. The signal is treated as periodic, with period `loopLength`.
   - Mid is the right choice. It is the same signal the spatial path reads (`readMono`). It gives one array at half the cost of two channels, and one density per read point.
   - Per-channel analysis followed by a union merge would roughly double M on decorrelated stereo, and K0 would absorb that anyway. Taking the max of the two channels is not a linear signal, so extrema on it mean nothing.
   - Known blind spot: anti-phase side content cancels in mid and reads as sparse. This is accepted and documented.
2. **Derivative: the cubic B-spline derivative at the knot** (eq. 3 at t = 0). The four taps are b′₋₁ = −½, b′₀ = 0, b′₁ = +½, b′₂ = 0, so d[i] = (y[i+1] − y[i−1]) / 2. It has a zero at Nyquist, which is the paper's point: an alternating ±1 signal produces no extrema.
3. **Candidates:** a sign change of d against the sign of the last *nonzero* d. Runs of exact zeros, i.e. silence, never fire.
4. **Sub-sample position by reverse linear interpolation** (eq. 4–5): α = |d₋|/(|d₋|+|d|), nₘ = (i − 1) + α. The value is vₘ = B(nₘ), the uniform cubic B-spline at nₘ (eq. 1, 2 and 6).
5. **Threshold:** keep a candidate iff |vₘ − v_prev| > ε = 0.001, measured against the last *kept* extremum, which gives hysteresis (§2.2).
   - Deviation from Algorithm 1 line 6, which tests the raw sample x[n]: the prototype tests the B-spline value vₘ on both sides, so the reference and the test are the same quantity. The difference is negligible for a density measure. Either is acceptable; the plan should pick vₘ.
6. **Wrap (fix to the prototype, which runs `i = 0..n−1` and can store a wrapped nₘ < 0 out of order).**
   - Run the main pass over `i = 1 .. loopLength` with circular neighbours, so i = loopLength ≡ 0. Then nₘ = (i − 1) + α is always in [0, loopLength) and the array comes out sorted by construction.
   - Seed the state (s_prev, d_prev, v_prev) with an un-stored warm-up over the last 2048 samples of the loop, so the extremum just after the seam is judged like any other.
   - Store positions as `float`. At 192 k / 2 s the ulp is 0.03 samples.

**Cost:** 431 µs (48 k) and 688 µs (96 k) on noise, plus the loop build and calibration (≈ 0.5–0.8 ms in total). The budget at 32-sample blocks and 96 k is 333 µs, so a single block is out.

**Spread over blocks with a fixed analysis rate R = 32 analysis samples per output sample:**

- Work is metered in units: one per loop sample, plus 4096 per calibration iteration (5 iterations: 4 K0 fixed-point steps and the final sort), so W = loopLength + 5·4096.
- At the start of every block while an analysis is pending, run work until the cumulative quota `(blockEnd − engageAbs) · R` is used up.
- **Ready time is deterministic:** `readyAbs = engageAbs + ceil(W / R)`. For any block split, the block that contains readyAbs starts with a quota ≥ W, so the work is finished before that block spawns anything. Grains spawned before readyAbs use the fixed path.
  - 2 s capture: 3 640 samples = **76 ms** at 48 k, 6 640 = **69 ms** at 96 k.
- Per-block cost is proportional to block length, about 230 ns per output sample in the worst case (noise at 96 k). At 32 samples / 96 k that is 1 024 units ≈ 7.4 µs per block (2.2 % of the budget), plus one calibration iteration (≤ 14 µs) every fourth block. A 4096-sample offline block finishes everything at once (≤ 1 ms of an 85 ms block).
- **Engage itself** (the existing `captureFrom`, `PluginProcessor.cpp:646–655`) is unchanged. Only `beginAnalysis()` is added, which is O(1).

**Storage bound:**

- Each sample index yields at most one candidate, so **M ≤ loopLength** is a hard bound.
- The adversarial row shows the bound is practically reachable (0.89 N): a decaying Nyquist-rate square makes d alternate every sample with ~1.8 between values.
- Real material peaks at 0.49–0.50 (noise, fs/4). A bound of N/2 would therefore be wrong.
- **Pre-size `float positions[maxFreezeSamples]`** (`PluginProcessor.cpp:414`): 384 KB at 48 k, 1.5 MB at 192 k. Add a 2 × (chunk + 4) stereo scratch for the chunked ring reads and P = 256 doubles for calibration. A `cap` check stays in the loop as a belt-and-braces guard.

**Decision:** mid-channel B-spline-derivative analysis with ε hysteresis on vₘ and reverse-linear positions, over the periodic loop with the seam crossfade applied. Run it in chunks at R = 32 units per output sample, with the deterministic `readyAbs` rule. Pre-size the storage to `maxFreezeSamples` floats (hard bound M ≤ N).

---

## Q2 — Which signal, when, where stored, which slot

- **Data.** Snapshot level 0 at position b is ring level 0 at absolute index `origin + b`:
  - `MipSnapshot::captureFrom` (`modules/synthesis/grain-engine/cpp/BandlimitedReader.h:494`) copies from `levelPos(0, origin).index − kTapsBack − 1`, and `levelPos(0, t).index == t`. `getOrigin()` is public.
  - `MipSnapshot` has **no** public raw-sample accessor; its data are private (`:556`). `MipRing` has `copyLevel` and `levelSample` (`:377–392`).
  - The 12 s ring keeps the captured 2 s for about 10 s after engage, and the analysis is done in 76 ms. `reset()` and `prepareToPlay()` clear both the ring and freeze, and they must cancel the analysis.
  - **So read the ring:** `delayBuffer.getRing().copyLevel(0, ch, origin + b, n, scratch)` per chunk, plus one extra copy for the seam's `b − loopLength` half. This needs no module change.
  - Alternative, for the later module promotion: add `const float* levelData(level, ch)` + `levelStart(level)` to `MipSnapshot` (grain-engine 1.2.0). Not needed now.
- **Completeness at engage.** All of it is in the ring at engage, settled: the capture ends 506 samples behind the head. The seam crossfade is not stored anywhere (it is applied at read time in `locate`), so the analysis applies it itself, as in the prototype's `buildLoop`.
- **Where to store.** Use **one** analysis store, owned by `FreezeManager`, for the currently held capture. It is not per slot.
  - A grain uses the extrema only at spawn, and only grains of the current slot spawn. Grains still sounding on the other slot latched their length at spawn.
  - Re-engaging restarts the analysis and overwrites the store safely. Release stops the analysis (state idle). `clear()` resets it.
  - This halves the memory against per-slot stores.
- **Slot used by a grain:** whichever slot it latches at spawn (`gp.frozenSlot`, `PluginProcessor.cpp:844`). That is always the current slot, which is the one analysed.

**Decision:** read the ring's level 0 at `[origin, origin + length)` in chunks via `copyLevel`, apply the seam blend in the analysis, and keep a single `ExtremaMap` inside `FreezeManager`, tied to the current capture. This overturns CONTEXT's "per slot", for the reasons above.

---

## Q3 — K, Grain Size as the mean, live knob, silent fallback

CONTEXT's first guess fails: K = GS·M/len, with the span used directly. M counts extrema, so dense regions dominate it, and the time mean then overshoots on mixed material:

| Material | GS 50 ms | GS 100 | GS 200 |
|---|---|---|---|
| sine / noise / music (homogeneous) | +0.0–0.9 % | +0.0–0.9 % | +0.0–0.8 % |
| 1 s sine + 1 s noise | **+323 %** | **+136 %** | +40 % |
| bass + snare | +86 % | +33 % | +6 % |

**Recommended scheme: a fixed measurement window per snapshot, with Grain Size and Adapt as a calibrated log-domain scale.**

1. **K0 per snapshot, at analysis time.** Choose K0 (fractional) so that the time-median of L_K0 over P = 256 evenly spaced read points is Tref = 100 ms.
   - Start from the count-based guess and run 4 fixed-point steps `K ← K·Tref/median`, clamped to [1, M − 2].
   - Store `lnMed` and the sorted lr_j = ln L_K0(p_j) − lnMed.
   - Measured K0: sine 55 → 11, music → 231, sparse/dense → 1 171, noise → 2 366.
2. **Per spawn:** `lenA = clamp(GS · exp(a·(ln L_K0(p) − lnMed) + c), 10 ms, 500 ms)`, where a = Adapt / 100.
   - This is exactly CONTEXT's `fixed^(1−a) · adaptive^a`, with adaptive = GS·exp(ln L − lnMed + c/a) and the clamp applied once. Adapt acts as an exponent on the density contrast, which is the reason to keep the log morph (a pure 55 Hz-vs-noise contrast is about 200:1).
3. **Per block:** solve c (monotone, bisection) so that the clamped mean over the P points equals GS.
   - Cache the result on (GS, a, analysis generation) and recompute only when one of them changes. Cost 2.3 µs; |c_fast − c_slow| = 3e-9.
   - Without the c solve, the clamps bias the mean by up to −37 % (GS 400, a 1, sparse/dense) and the log morph by +18 % (GS 20, a 1).
   - With it, **the mean is within 0.3 % of GS for every row**: 5 stimuli × GS 20/50/100/200/400 ms × a 0.25/0.5/1, against an 8192-point truth grid. Reverse spans with forward calibration also land within 0.1 %.
4. **Grain Size knob changes need no re-analysis.** K0 is fixed, and only c moves. Lengths scale **exactly proportionally** (ratio 2.0000 for GS 100 → 200) on every point the clamps don't touch. Where they do (sparse/dense at a 1, 1 555 of 8 192 points clamped), the c solve holds the mean and the unclamped points scale 2.24×. **Mean has priority over proportionality.** The proportionality gate uses bass+snare, which is unclamped at GS 50/100, a 1 (range 19.6–212 ms).
5. **Contrast achieved** (sparse-half mean / dense-half mean, sparse/dense stimulus): 1.67 (a 0.25), 2.72 (a 0.5), 5.68 (a 1) at GS 100 ms.
   - Bass+snare at GS 100, a 1 runs 39–212 ms within each 0.5 s cycle.
   - Stationary material (noise, the "music" stimulus) stays within ±13 %. It is **useless as a gate stimulus** (vacuous).
6. **Fallback to the fixed path** (identical to Adapt 0 for that grain) when:
   - M < 8;
   - loopLength < 0.2 s;
   - the analysis is not ready (t < readyAbs).

   Silence and noise at −80 dB give M = 0, so they fall back. Partial silence gives long spans there, which are clamped, and the c solve keeps the mean.
7. **Clamp effect at the knob's ends:** at GS 400, a 1, the range compresses to 261–500 ms. Adaptation fades near 500 ms and near 10 ms, because the mean is pinned. Document this.

**Decision:** use K0 per snapshot for a time-median span of 100 ms, `lenA = clamp(GS·exp(a·(lnL − lnMed) + c(GS, a)), 10 ms, 500 ms)`, and a per-block clamped-mean solve of c. Fall back to fixed length for M < 8, a loop under 0.2 s, or before readyAbs. This overturns CONTEXT's "K derived per snapshot so mean ≈ GS (K ≈ GS·M/len)".

---

## Q4 — Lookup, wrap, reverse, pitch, pre-roll guard

- **Loop coordinate of a snapshot position:** the same mapping as `locate` (`FreezeManager.h:168–175`), i = fmod(pos − xfade, loopLength), wrapped to ≥ 0.
- **Forward grain:** reads from `frozenBase` upward (`GrainPool.h` processSample: `pos = frozenBase + readPosition`, starting at readPosition 0). Anchor p = frozenBase.
  - m = `upper_bound(pos, p) − 1`. When m = −1, use the previous period's last extremum.
  - L = (1−f)·n[m+k0] + f·n[m+k0+1] − n[m], where n[j] = pos[j mod M] + loopLength·⌊j/M⌋, k0 = ⌊K0⌋ and f = frac(K0). Measured cost 63 ns.
- **Reverse grain:** starts at frozenBase + len − 1 and reads down. That makes the start depend on len, which would be circular.
  - **Anchor it at its fixed-length start**, p_rev = frozenBase + fixedLen − 1 (the Adapt 0 start).
  - Measure the span **backwards**: m = first extremum ≥ p, L = n[m] − n[m−K0] (with fractional K0).
  - After the length is known, set `gp.frozenBase = p_rev − (len − 1)`, so the grain starts exactly at p_rev and reads the K0-span region behind it.
  - Prototype: forward and backward spans differ where it matters (at 250 ms into the sine half, forward 806 ms, back 289 ms). The backward-span mean with forward calibration is within 0.1 % of GS.
- **Pitch: lengths in output samples.** The span measured in snapshot samples is used as the output length directly; it is *not* divided by the rate. Reasons:
  - Grain Size stays the mean at every pitch. With S/rate, +36 st would cut the mean 8× (100 → 12.5 ms, clamped at 10), so Adapt 0 → 100 would jump with pitch.
  - Pitch Random would correlate length with pitch per grain.
  - It matches Adapt 0 semantics.

  The cost: at rate r a grain reads r·K0 extrema. That is already true of every fixed-length grain.
- **Pre-roll guard** (`PluginProcessor.cpp:822–839`) stays as it is, computed with the fixed length.
  - For frozen grains it has no safety role: `locate` wraps every position into the loop, so the snapshot has no write head to overrun.
  - It only places the read point, and keeping it fixed keeps every grain's start point identical across Adapt values.
  - At ±36 st, a 500 ms grain at +36 st reads 4 s of a 2 s loop, but that already happens at Adapt 0 with GS 500. Adapted lengths never exceed 500 ms before Size Random, so **no new reach**.

**Decision:** binary search plus fractional-K span with modulo-M wrap. Forward grains measure ahead of frozenBase. Reverse grains measure behind their Adapt 0 start, and their frozenBase is re-derived so that start holds. Length is in output samples. The guard is unchanged.

---

## Q5 — Spawn order, scatter, Size Random, tilt, latch, RNG, bit-exact Adapt 0

From `PluginProcessor.cpp:765–867`: Size (`:788–792`), tilt (`:793`), amp, scatter (`:802–808`), pitch, pan, reverse, pre-roll guard (`:829–839`), frozen base (`:841–846`), spatial, detune (`:858–864`), then `spawnGrain` (`:866`).

**Facts that settle the order:**
- **The scatter is scaled by `grainSizeSamples`, the knob**, not by the Size-Random length (`:806–807`). The "±1 grain length" freeze rule has always meant ±1 *Grain Size*. Scatter is not circular with adaptation as long as it stays on the knob value.
- CounterRng draws are keyed `(seed, grainCounter, slot)` (`Source/dsp/CounterRng.h:24–36`), so call order is irrelevant and re-reading `draw.nextFloat(SlotSize)` returns the same value. The adaptive path needs **no new slot and no new draw**.

**Order:**
1. Steps up to the detune run unchanged. `gp.grainLengthSamples = fixedLen` keeps today's value, with Size Random applied.
2. **Then, new:**
   ```cpp
   if (gp.readFromFrozen && grainAdapt > 0.0f && adaptReady)   // adaptReady: valid && absSample >= readyAbs
   {
       const double anchor = gp.reverse ? gp.frozenBase + (fixedLen - 1) : gp.frozenBase;
       double lenA = freezeManager.adaptiveLength (anchor, gp.reverse, adaptC, grainSizeSamples, grainAdapt); // clamped 10..500 ms
       int len = juce::jmax (1, (int) lenA);
       if (sizeRandom > 0.0f)                                   // same draw, same condition as :789
           len = juce::jmax (1, (int) (len * (1.0f + draw.nextFloat (CounterRng::SlotSize) * sizeRandom)));
       gp.grainLengthSamples = len;
       if (gp.reverse) gp.frozenBase = anchor - (len - 1);
   }
   ```
3. **Move `gp.tiltPeak = tiltPeakFor(shapeTilt, gp.grainLengthSamples, tiltMinRamp)` after this block.** It is a pure function, so at Adapt 0 the value is identical.

**Size Random:** multiplicative on the adapted length, with the same slot and condition as CONTEXT's default. The 10–500 ms clamp applies *before* Size Random, as today: GS 500 with Size Random 100 already reaches 1 s.

**Scatter:** stays on the knob, which overturns CONTEXT's "scatter by adapted length". Reasons:
- the current code never used the per-grain length;
- it avoids circularity;
- positions stay identical at every Adapt value, so A/B renders differ only in length, which gives cleaner gates;
- the mean adapted length is GS, so the anti-comb rule holds on average.

Risk: long sparse-region grains (up to 500 ms) scattered by only ±GS overlap heavily. Offsets are random per grain and frozen grains carry ±6 ct detune, so there is no fixed hop and no stable comb. This needs a listening check in verify. The lever, if needed: rescale the *same* draw, `offset · max(1, len/GS)`.

**Latch:** inherent. Length, tilt and Adapt are fixed at spawn, and Adapt is read once per block like Tilt (`:568`).

**Bit-exact Adapt 0:** `grain_adapt` is `NormalisableRange(0, 100, 0.1)` with default 0, which snaps to exactly 0.0f (Q7). With `grainAdapt > 0.0f` false the new block never runs, the c solve is skipped, and the tilt call gets the same arguments. **Never** compute `pow(fixed, 1−a)·adaptive^a` unconditionally: exp/log round-trips are not exact in float or double.

The analysis itself runs whenever Freeze engages, even at Adapt 0. It only writes FreezeManager-private state and has no audio effect. Gate it as the bit-exact rows prove. Optionally skip it at Adapt 0; but analysing always means turning Adapt up mid-hold works immediately, so **always analyse**.

**Decision:** use the order above. Scatter stays on the knob and Size Random multiplies the adapted length (same draw). Tilt moves after the length. An explicit `grainAdapt > 0.0f && frozen && ready` branch is the only code path change.

---

## Q6 — Density compensation with varying lengths, and the level gate

- **Code:** `processSample` / `processSampleSpatial` return `live = Σ stealGain` over active voices (`GrainPool.h:392`, `:460`). `nextDensityGain(live, mu2, mu1Sq, rho)` (`PluginProcessor.h:269`) forms P = N·μ₂ + ρN(N−1)μ₁² per sample and smooths it over 100 ms.
  - μ comes from `windowMoments(shape, tiltPeakFor(tilt, grainSizeSamples))` (`PluginProcessor.cpp:669`): the shape's moments over normalised phase, independent of length for the symmetric shapes.
- **Why N·μ₂ stays unbiased under any length distribution:**
  - For a stationary spawn rate λ, E[Σᵢ wᵢ(t)²] = λ·E[len]·μ₂ and E[N] = λ·E[len], so E[P_incoh] = E[N]·μ₂.
  - Varying lengths change N locally, and the per-sample live count tracks that.
  - Frozen ρ = floored² (spray ≥ 1, so sprayCoh = 0). The floor uses the fixed-length guard, which is unchanged, so ρ is unchanged.
  - Exp Decay + Tilt: μ depends on the per-grain tilt peak, which depends on length. That error class already exists today with Size Random.
- **Where the level *does* move: the never-boost rule.** P < 1 gives a gain of 1.
  - At low density, adaptation puts short grains in dense passages that no longer overlap. The cloud thins there and nothing compensates, the same as turning Grain Size down at Adapt 0.
  - Worked example: 10 g/s, GS 100, a 1 on sparse/dense. Dense N ≈ 0.15, sparse ≈ 3.2. The power estimate comes out about +1.5 dB above Adapt 0. This is a property of the existing law, not a bug; document it.
- **Level gate (non-vacuous):**
  - **Stimulus:** a 2 s periodic stimulus, 1 s 55 Hz sine and 1 s noise at **equal RMS** (0.354, mono), with Freeze engaged on a period boundary so the snapshot is exactly one period. Equal RMS makes the expected level change 0 while lengths vary 2.7–5.7×.
  - **Settings:** Density 100 (100 g/s), GS 200 ms, a = 0.5 (dense ≈ 90 ms, so N ≥ 9 everywhere), Spread 100.
  - **Measure:** RMS over 4 s from readyAbs + 0.5 s, averaged over 4 seeds (hash-mixed per job, never seed + i).
  - **Control:** an old-vs-old row (Adapt 0, seed set A vs set B).
  - **Pass:** |Adapt 50 − Adapt 0| ≤ 0.5 dB, with the control ≤ 0.2 dB.
  - **Liveness, in the same render:** the spawn-log sparse/dense mean-length ratio must be ≥ 2 (Q9).
  - Also a dense-noise-cloud row (CONTEXT's), regression only, since lengths stay within ±3 % on noise.

**Decision:** the compensation needs no change. Gate the level with the equal-RMS sparse/dense stimulus at high density, plus a liveness check on the length ratio and an old-vs-old control. Document the low-density thinning, which is the never-boost rule.

---

## Q7 — Parameter, state, presets

- `juce::ParameterID { "grain_adapt", 5 }`, "Grain Adapt", `NormalisableRange<float>(0.0f, 100.0f, 0.1f)`, default 0, label "%". Add it to `coreGroup` after `shape_tilt` (`PluginProcessor.cpp:206–211`). AU parameter version hint **5**; the current maximum is 4.
  - **Snap check:** `start + interval·round((0 − 0)/0.1) = 0` exactly. The stepped-range trap only bites a *negative* start (memory note), so 0.1 is safe and matches `grain_skip`.
- Cache `grainAdaptParam` and read it per block next to `shapeTilt` (`:568`) as `jlimit(0, 1, v/100)`.
- **Old sessions:** `migrateLegacyState` (`PluginProcessor.cpp:1222–1261`) writes the default for every parameter the tree lacks, so a v3.4.0 session loads with Adapt 0 automatically. Gate: load a v3.4.0 session into an instance whose Adapt was set to 50 first. It must render bit-exact against v3.4.0.
- **No preset manager in this plugin** (`getNumPrograms() = 1`, no preset-manager module). Nothing to migrate.

**Decision:** use `grain_adapt` v5, 0–100 step 0.1, default 0. No migration code; the existing default-fill covers old sessions.

---

## Q8 — UI placement and i18n

**Width budget, from the CSS** (`Source/ui/public/index.html:150–191, 291–296`):
- The left column is 451 px. The group content is 451 − 2·10 padding − 2·1 border = **429 px**.
- `.knob-container` is 62 px, `.dropdown-container` 80 px, and the flex gap is 6 px.
- Row 1 holds 6 knobs (Size, Density, Scan, Spread, Reverse, Feedback): 402 px. A 7th would need 470 px.
- Row 2 holds Dry, Wet, Size Rnd, Amp Rnd, Shape and Tilt: 5·62 + 80 + 5·6 = **420 px, 9 px free**.
- Adapt needs 68 px. A third Core row would add about 88 px and break the lower-panel budget.
- The right column has 15 px of slack (389 px content against Pitch & Scale's widest row of 374 px).

**Recommended layout: Adapt beside Grain Size in row 1, at Core-only `.knob-container` 57 px and gap 4 px.**
- Row 1: Size | **Adapt** | Density | Scan | Spread | Reverse | Feedback = 7·57 + 6·4 = 423 px ≤ 429.
- Row 2: 5·57 + 80 + 5·4 = 385 px.
- The 48 px dial is unchanged. Scope the override to the Core group, e.g. `.core-group .knob-container`.
- **Risk: nowrap captions at 57 px.** `label.feedback` FR was abbreviated because "Réinjection" measured 60.27 px (`i18n.js:74–80`). `label.tilt` FR "Inclinaison" has no recorded width. check-ui-labels must pass at EN/FR/zh. Fallbacks:
  - FR abbreviations ("Inclin.");
  - take up to 8 px from the right column's slack.
- The UI phase must measure; the arithmetic above is not a measurement.

**Files:**
- `index.html`: the knob-container with `data-param="grain_adapt"` and caption `data-i18n="label.adapt"`.
- `js/app.js`: `setupKnob('grain_adapt', …)` with the percent formatter, beside `:429`.
- `js/i18n.js`:
  - `label.adapt` (en "Adapt", fr e.g. "Adapt." `reviewed: false`, zh "自适应" `'mt'`);
  - `tip.adapt` in en/fr/zh, which must say **it acts only while Freeze is held** (glossary: fr *Geler/gel*, zh *冻结*), that Grain Size stays the average, and that it takes effect a moment after engaging;
  - the tooltip binding row beside `:1653`.
- `PluginEditor.h/.cpp`: `grainAdaptRelay`, `.withOptionsFrom`, and `WebSliderParameterAttachment` (pattern at `PluginEditor.cpp:53, 220, 294`).
- `tests/i18n-states.json`, if it enumerates knobs.
- **Gates:** `scripts/check-i18n.js`, `i18n-fr-lint.js` and `i18n-zh-lint.js` (glossary first), plus `check-ui-labels.js`.

**Decision:** put Adapt in row 1 beside Grain Size, with Core-only 57 px containers and a 4 px gap, keeping the 900 × 800 frame. Measure it with check-ui-labels, and fall back to FR caption abbreviations. Tooltips in 3 languages, saying Freeze-only.

---

## Q9 — Test gate design

**Harness:** `ouaricon_add_processor_console` drivers that call `processBlock` directly in custom C++. There is no pedalboard. The v3.4.0 twin pattern is the one from `CMakeLists.txt:153–178` (git archive + `list(TRANSFORM … SOURCES)`). New gate: `tests/adapt-gate`, targets `O-GrainScatter-adapt-gate` and `-adapt-gate-v340`, extracted from commit **144a73f3** (v3.4.0 follow-up, the current Source).
- Reuse `skip-tilt/main.cpp`'s `render`, `fixedSplit`, `variableSplit`, `setParams`, `makeInstance` and `kChangePoint = 98304`. That value is a block boundary in every split, so freeze engages at the same absolute sample.

**Observing lengths:** under `GS_TEST_SPAWN_LOG` (set only on the gate target, the same idea as `GS_TEST_DENSITY_COMP_OFF`), add a fixed ring of 8192 `{absSample, grainLengthSamples, loopPos, reverse, adapted}` written at spawn, plus a getter. It records only, so the audio is unchanged and the shipping binary is untouched.

**Rows:**

| # | Row | Pass |
|---|---|---|
| A1–A8 | Adapt 0 vs v3.4.0: free, sync (repeats, Euclid, stutter gate), spatial (trajectory, Doppler), freeze (engage at kChangePoint), freeze + reverse 50 + Size Rnd 50 + Tilt −80, freeze +24 st, freeze spatial | peak diff **== 0** |
| A9 | v3.4.0 session loaded into an instance with Adapt pre-set to 50 | param reads 0, render bit-exact |
| L1 | Liveness of the stimulus: sparse/dense equal-RMS snapshot, a 1, GS 100: spawn-log mean length, sine half / noise half | **≥ 3.0** (prototype 5.68); Adapt 0 control = 1.00 ± 0.02 |
| L2 | Same at a 0.5 | ≥ 1.8 (prototype 2.72) |
| M1 | Mean adapted length (pre Size Rnd) over ≥ 2000 frozen spawns, GS 50 / 100 / 200, a 1, sparse/dense + bass+snare | within **±5 %** of GS (prototype ±0.3 %; spawn sampling ~1/√2000 × spread) |
| M2 | Reverse 100 %: same | ±5 % |
| P1 | Proportionality: bass+snare, a 1, GS 50 → 100, same seed (same read points, common random numbers intended here) | per-grain ratio 2.00 ± 0.01 on unclamped grains |
| F1 | Fallbacks: silent snapshot, −80 dB noise, loop < 0.2 s, spawns before readyAbs | lengths == fixed path (bit-exact vs Adapt 0 render) |
| D1 | Block-size null at Adapt 100 in freeze (engage at kChangePoint), plus Adapt automated at kChangePoint, 32 / variable vs 512 | ≤ −120 dBFS (expected −400) |
| D2 | Ready time: first adapted spawn's absSample identical across the 3 splits; equals engageAbs + ceil(W/32) | exact |
| R1 | Alloc: operator-new counter (the O-TextureForge `scheduler-harness` pattern) around every processBlock after the first, covering engage, analysis blocks, re-engage mid-analysis, release, reset-free cycling, Adapt 0 ↔ 100 | **0**; liveness: one deliberate `new` inside the window counts 1 |
| V1 | Level: equal-RMS sparse/dense, Density 100, GS 200, a 0.5, Spread 100, 4 hash-mixed seeds, 4 s | \|Δ\| ≤ 0.5 dB; old-vs-old control ≤ 0.2 dB |
| V2 | Dense noise cloud, Adapt 100 vs 0 | ≤ 0.5 dB (regression) |
| C1 | Engage cost (report): `FreezeManager` analysis step alone, 2 s noise at 48 k / 96 k, total µs and max per-32-sample-block µs; processBlock max over the analysis window vs before | report; fail only if one 32-sample block's analysis step > 50 µs |
| N1 | Negative control: the gate's twin built with `GS_TEST_ADAPT_FIXED_K` (CONTEXT's K = GS·M/len, no c solve) | M1 must FAIL (prototype +136 % at GS 100) |

**Existing gates** rerun unchanged: `blocksize-null`, `density-gain`, `alias-gate` and `skip-tilt` (its v3.3.0 bit-exact rows still hold, since Adapt defaults to 0). Then pluginval 10 and auval.

**Decision:** one new `adapt-gate` with a v3.4.0 twin (144a73f3), the spawn-log test hook, and the rows above. The liveness rows guard against a vacuous stimulus, and the fixed-K twin is the negative control.

---

## Q10 — Module promotion and risks

**Module-promotion candidate.** `ExtremaMap` contains the analyser, the span lookup and the K0 / c calibration. It is plain C++ with no JUCE, so it can move to grain-engine as `ExtremaDensity.h` (1.2.0) once this ships, together with a `MipSnapshot::levelData` accessor so the analysis can read the snapshot instead of the ring.
- Other consumers: O-Freeze, O-simpleGrain freeze, O-TextureForge drone.
- A live-path follow-up is cheap: the streaming analysis costs ≤ 4.5 ns/sample worst case, so per-sample extrema could be kept on the write path.
- Out of scope now, per CONTEXT.

**Risks:**
1. **Absolute ε (−60 dBFS) makes quiet material read as sparse.** −80 dB noise gives M = 0 and falls back; −50 dB noise is still dense (0.30). A quiet dense passage gets long grains. This is the paper's behaviour (§2.2), kept per CONTEXT.
2. **Anti-phase stereo** cancels in mid and reads as sparse. Accepted.
3. **Sparse-before-transient spans are long** (§3.7.3): 806 ms forward span at 250 ms into the sine half. The 500 ms clamp and the a-exponent bound them, and the c solve keeps the mean.
4. **Scatter on the knob with long grains:** listening check in verify; lever in Q5.
5. **About 70–76 ms from engage until Adapt applies.** Document it in the tooltip / CHANGELOG.
6. **CPU figures are M4 Max.** Expect 2–3× on older x86. R = 32 keeps a 32-sample / 96 k block under about 25 µs (~8 %) even at 3×. Re-measure on Windows in verify.
7. **Never-boost thinning** at low density with short dense-region grains (Q6). Document it.
8. **Steals:** the local live count rises in sparse regions. Adapted lengths are ≤ 500 ms (≤ 1 s with Size Rnd), the same ceiling as fixed GS 500, so this is no new regime. 100 g/s × 500 ms = 50 < 64 voices in Free mode.

---

## Implementation Approach

**Strategy:** chunked snapshot extrema analysis in `FreezeManager`, with calibrated log-domain length per frozen spawn.

**Files to modify:**

| File | Change | Complexity |
|---|---|---|
| `Source/dsp/ExtremaMap.h` (new) | Analyser state machine (warm-up, main pass `i = 1..lp`), positions `float[maxLen]`, K0 / lnMed / sorted lr[256] calibration, `solveC(gs, a)` with prefix sums, `spanForward/Back(p)` | Medium |
| `Source/dsp/FreezeManager.h` | Owns one `ExtremaMap`. `prepare` sizes it, `clear` / `release` cancel it, `engage` calls `beginAnalysis(origin, length, xfade, loopLength, engageAbs)`. New: `analysisStep(delayBuf, blockEndAbs)` (R = 32 quota), `adaptReady(abs)`, `adaptiveLength(anchor, reverse, c, gs, a)`, loop-coordinate helper sharing `locate`'s mapping | Medium |
| `Source/PluginProcessor.h` | `grainAdaptParam`; cached `adaptC` and its (GS, a, generation) key; optional `GS_TEST_SPAWN_LOG` ring and getter | Low |
| `Source/PluginProcessor.cpp` | Parameter (v5); per-block read; `analysisStep` after the engage block (`:646–655`) and c refresh; spawn branch + tilt move (`:788–866`) | Medium |
| `Source/PluginEditor.h/.cpp` | Relay + attachment | Low |
| `Source/ui/public/index.html`, `js/app.js`, `js/i18n.js` | Knob, Core-only width override, label / tip in 3 languages, tooltip binding | Low–Medium |
| `CMakeLists.txt` | Version 3.5.0; `adapt-gate` and `-adapt-gate-v340` (144a73f3) targets | Low |
| `tests/adapt-gate/main.cpp` (new) | Rows in Q9 | Medium |
| `CHANGELOG.md` | 3.5.0 entry | Low |

**Dependencies:** grain-engine 1.1.0 as is (`MipRing::copyLevel`). No JUCE additions.

**Alternatives considered:**
- *K follows Grain Size* (CONTEXT): fails the mean on heterogeneous material (+136 % / +323 %), and changing the knob changes the contrast pattern. Rejected.
- *Linear morph* `(1−a)·fixed + a·adaptive`: preserves the mean without a c solve, but the c solve already gives the mean, and log keeps Adapt as a contrast exponent (the ratio is 5.7 at a 1 and 1.7 at a 0.25). Rejected.
- *One-shot analysis at engage:* 0.5–0.8 ms against 333 µs at 32 / 96 k. Rejected.
- *Per-slot extrema stores:* no reader after spawn needs them. Rejected (Q2).

---

## Complexity Assessment

**Overall: Medium.**
- About 10 files and one new header.
- DSP change significant but contained: a new analysis plus one spawn branch.
- UI: minor (one knob, but a tight width budget).
- One parameter added; no breaking change.

---

## Domain Detection

| Domain | Score | Keywords |
|---|---|---|
| DSP | 9 | extrema, B-spline derivative, freeze snapshot, grain length, density compensation, RT-safe, block-size null |
| GUI | 3 | knob, tooltip i18n, layout budget |
| Polish | 1 | CHANGELOG |

**Detected domain: mixed, DSP-led.** Recommended: dsp-agent for the analysis, spawn and gate; gui-agent for the knob, layout and i18n as a second wave.

---

## Version Impact

**MINOR → 3.5.0.** A new feature with backward compatibility.
- Parameter IDs: one added (`grain_adapt`, version hint 5). Existing ones are unchanged.
- Ranges, preset/state format and public API: unchanged.

---

## Overturned CONTEXT defaults

1. **K ≈ GS·M/len → K0 fixed per snapshot (median span 100 ms) plus a per-block c solve.** Grain Size is the exact clamped mean and scales proportionally where unclamped.
2. **Per-slot analysis → a single store** for the current capture.
3. **Freeze scatter by adapted length → scatter stays on the Grain Size knob**, as the code always did, with no circularity.
4. **Pitch: output samples, not snapshot-samples ÷ rate.**
5. **Engage-time analysis → chunked, R = 32, deterministic `readyAbs`** (about 70–76 ms of fixed-length grains after each engage).

## Open Questions

- Tref = 100 ms is a judgement call. Shorter gives more local contrast with more noise; longer gives smoother contrast. It is not a user control. Confirm by listening in verify; the alternative is 50 ms.
- Caption fit at 57 px (FR "Inclinaison"): the UI phase must measure.

## Next Phase

Plan: atomic tasks in two waves, DSP then GUI, with the Q9 rows as per-task verification.

*Generated by improve-milestone research phase*
