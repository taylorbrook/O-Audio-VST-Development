# Stage 2 — DSP · Phase 2.2 (Four streams, randomisation, pitch/scale, window morph, normalisation) — Research

**Plugin:** O-Granulator
**Stage / phase:** 2-dsp / 2.2 of 6
**GSD phase:** research
**Date:** 2026-11-04
**Inputs:**
- `CONTEXT-2.2.md` (D4, D5 locked; four open questions)
- `RESEARCH-2.1.md`, `SUMMARY-2.1.md`, `VERIFICATION-2.1.md`
- ARCHITECTURE §4–§11 and Algorithm §5
- the 2.1 engine in `Source/dsp/` (Spawner, OutputLaw, Engine, StreamScheduler, Grain, EngineConstants, ControlGrid)
- grain-engine 1.1.0 `WindowLut.h` and `CounterRng.h`
- pitch-collections 1.0.0 `PitchCollections.h`
- O-GrainScatter `PitchEngine.h`
- the processor's custom-scale and mask state

**Scratch experiments:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/66075dcd-7701-4879-96c5-389ec24e5ec7/scratchpad/r22/`

| Script | What it does |
|---|---|
| `k.py`, `k1.py`, `k2.py` | grain-cloud coherence probes |
| `k3.py` | the proposed law vs measurement across density, through the cap |
| `wf.py`, `wf2.py` | stream-fair stealing vs the water-filled law |
| `wm.py` | morph-bank moments, skew-warped cumulatives, worst-case capped overlap |

All four open questions are answered below. There are **eight findings the plan must carry**. Two of them (R5 and R7) would make 2.2's own level gates fail or pass vacuously if they were missed.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R5** | The ARCHITECTURE §10 κ formula keys on the wrong variables. **Jitter does not decorrelate** forward unison grains: they read `t − D` whatever the onset time. **Base pitch ≠ 0 and reverse do** decorrelate, and κ omits both. Measured: the 2.1 law is **+19.7 dB hot** at spray 0, 200 g/s × 1 s. The "+7 dB" figure was overlap 8 only | **Blocks the ±1.5 dB density gate** | Replace the dB κ-blend with the derived partial-coherence law (§1). Measured: within ±0.25 dB from density 1 to 200 through the cap, on noise and sine |
| **R6** | The capped law needs **cumulative ∫w and ∫w²** per morph table, not just half-window μ₁/μ₂. Skew is exact through a two-piece formula. The worst capped overlap with the bank is **P/μ₂ = 333** (Trapezoid, skew −1), not 43.02 | Gate E0 values, law accuracy | Bank stores `C1[65][2049]` and `C2[65][2049]`. Use the §2 formula. Update the E0 worst-case row |
| **R7** | The unity floor `max(1, P/μ₂H)` puts a **domain** on the level gates. A window is normalised only for N ≥ μ₂H/μ₂(m, skew). That is 1.0 for Hann, 3.6 for Expo and **33 for Expo at skew −1**. At the 100 ms default size, density 1–10 is sub-floor | **Vacuous / failing gates** | DSP-03 runs at N = 40. The density gate runs at size **1000 ms** (N = 1…200, crossing the cap). Gate below the floor separately: no boost, so peak ≤ the unity window |
| **R8** | With a real-valued quota, `live > q` **flags a sparse stream** (q < 1) that has one grain alive, so it gets stolen first (−0.85 dB) | Fairness defect | Flag the stream only when `live_s > ceil(q_s)`. Measured: −0.18 dB |
| R9 | Mask bit d is the **source-table** index, and the 7-Limit Just table is unsorted. Sorting the collection first misaligns the mask | Correctness trap | Build with the original d, then sort the *active* array. Restrict to the pitch window at pick time by binary search, so a pitch move never triggers a rebuild |
| R10 | Spread × stream pan: a per-grain balance about centre × the ramped bus balance keeps mean power **exactly** at any stream pan. It does not pile up at the edges, and spread 0 is a bitwise no-op | Q3 closed | Spread goes into `gainL/R` at spawn. Stream pan stays on the bus (2.1). Coherent S1 gets the factor `E[g] = (4/πs)·sin(πs/4)` |
| R11 | Sine probes: incoherent sums of a sine have high variance (+2 dB spread over 2 seeds). Jitter 0 with pitch ≠ 0 and regular onsets **comb-cancels** (−230 dB at +12 st, N = 8) | Gate-design trap | Average ≥ 8 seeds for sine rows. Never run a sine row at jitter 0 with pitch ≠ 0 |
| R12 | A **2.1-equivalence out-of-tree null** is possible if (step 0, skew 0) keeps the closed-form Hann in *both* the renderer and the law, and the law reduces bitwise at c = 0 | Regression net (recommended) | Seeded `git archive 6508c635` pair at 2.1-equivalent settings (§7) |
| Q1 | Water-filling is continuous in the demands, and a disabled stream's demand is 0. Quota changes cannot step g_out, and g_out is ramped on top | Closed | Closed-form water-fill over 4 at the grid (§3) |
| Q2 | κ break-points depend on the **source**. Decorrelation completes at about 1 ms of spray / 0.1 st for noise, 2 ms / 0.5 st for a 440 Hz sine, and ~8 ms / > 1 st for a 110 Hz sine. No source-blind break-point holds ±1.5 dB *inside* the transition band | Closed (gate scoped) | W₀ = 5 ms, p₀ = r₀ = 0.5 st. Gate outside the band. Document the band (§1.4) |
| Q3 | Spread composition | Closed | R10 |
| Q4 | `quota_s` feeds `M_m`: per-stream power matches `q_s·M(q_s/N_s)` within **0.65 dB** across six 4-stream cases (grain_limit 32/64/96/128). Native steal-oldest starves small streams by **−18 to −25 dB** | Closed | Use q_s. Native steal-oldest is the FUNC-03 negative control |

---

## 1. R5 — Output law: replace the κ dB-blend with derived partial coherence

### 1.1 What decorrelates a grain cloud (measured, `k1.py` / `k2.py`)

A forward grain at rate r latches `readAbs = s − D` and reads `s − D + r·(t − s) = r·t − D + (1−r)·s` at output time t. For the cloud to be coherent, that read must not depend on the onset time s, and D must not vary between grains.

| Variable | Effect on coherence | ARCHITECTURE κ | Measured, N = 8, len 100 ms |
|---|---|---|---|
| jitter | **none.** It moves s, and s cancels at r = 1 | includes `(1 − jitter)` ✗ | sine: jitter 0 / 20 / 100 % → +7.27 / +7.27 / +7.28 dB over incoherent |
| spray | decorrelates once the spray width W exceeds the source's correlation length | `spray·span / (0.25·len)` (break at 25 ms) | complete by about 1 ms (noise), 2 ms (440 Hz), 8 ms (110 Hz) |
| pitch_rand | decorrelates (rates differ) | `pitchRand / 0.5 st` | about 0.1 st (noise), 0.5 st (440 Hz), > 1 st (110 Hz) |
| **base pitch ≠ 0** | **decorrelates**: (1−r)·s ≠ 0 | **omitted** ✗ | noise: 0.1 st partial, 1 st complete. At +12 st, ARCH κ = 1 → **−7 dB too quiet** |
| **reverse** | **decorrelates**: the read is 2s − D − t | **omitted** ✗ | rev 50 %: +2.9 dB, as the mixture predicts (§1.2) |

### 1.2 The law (derived, not fitted; memory: "do not fit a second factor")

Let c_s be the probability that a grain of stream s is in the **mutually coherent subset**: forward, unison rate, and the same D. Its coherent sum adds in amplitude. The rest adds in power, and cross terms vanish.

```
S1 = mean Σw  =  N·μ₁                (N <= q)     |  q·C1(x)/x   (N > q, x = q/N)
S2 = mean Σw² =  N·μ₂                              |  q·C2(x)/x
S1 *= E[g_spread]                    (§5: (4/(π·s))·sin(π·s/4); exactly 1 at spread 0)
P  = max (c·S2, (c·S1)²) + (1 − c)·S2
g_out = 1 / sqrt (max (1, P / μ₂_Hann))
```

- **c = 0 reduces bitwise to the 2.1 law.** `max(0, 0) + 1·S2` gives `S2`. This is what makes R12 possible.
- **c = 1** gives `max(S2, S1²)`. That is the exact regular-onset coherent power at N ≤ 1 (no overlap) and for N ≥ 2. Measured against `k1.py` it matches to 0.01 dB at N = 1, 2, 4, 8 and 32.
- It is continuous in c, N and q. It is continuous at N = q, because C(1) = μ.
- Jitter on a coherent set adds ripple variance: at most +0.6 dB measured at jitter 100 %, inside the gate.

```
c = (1 − reverse_prob) · c_spray · c_pitch
c_spray = (1 − clamp (W / W₀, 0, 1))²            W = spray·span (samples), W₀ = 5 ms·fs
c_pitch (scale Off) = (1 − clamp (|pitch| / p₀))² · (1 − clamp (pitch_rand / r₀))²     p₀ = r₀ = 0.5 st
c_pitch (scale on, pitch_rand > 0) = (1 − clamp (|pitch| / p₀))² · (unisonActive ? 1 / nInWindow : 0)
c_pitch (scale on, empty set / no degree in span) = (1 − clamp (|pitch| / p₀))²     (every grain sits at the base pitch, D4)
```

With the scale on, the unison factor is **exact**: picks are uniform over the window's active set (D4), so `P(offset = 0) = 1/nInWindow` when degree 0 is active.

### 1.3 Measured against the gate (`k3.py`)

Setup: size 1000 ms, cap 128, Hann, jitter 20 %, 2 seeds. Each cell is the output level relative to target, in dB.

| Source / config | d = 1 | 5 | 20 | 100 | 128 | 200 |
|---|---|---|---|---|---|---|
| noise, spray 0, **new** law | −0.15 | −0.22 | −0.24 | −0.24 | −0.24 | −0.19 |
| noise, spray 0, **2.1 law** | −0.15 | +5.01 | +11.0 | +18.0 | +19.1 | **+19.5** |
| sine 440, spray 0, new | +0.02 | 0.00 | 0.00 | 0.00 | 0.00 | +0.04 |
| noise, spray 10 %, new (c = 0) | −0.10 | −0.15 | −0.17 | −0.13 | −0.11 | −0.11 |
| noise, reverse 50 %, new (c = 0.5) | −0.06 | +1.29 | +0.22 | +0.17 | −0.02 | −0.28 |
| sine 440, reverse 50 %, new | +0.09 | +1.03 | +0.11 | +0.69 | +0.45 | −0.37 |

The worst cell is +1.29 dB, at the `max()` kink near N ≈ 4. It is inside ±1.5 dB, and the law is analytic, so this is not a fitting residual.

### 1.4 The transition band (Q2): document it, don't gate inside it

Physical decorrelation depends on the source spectrum. For a sine, the pair coherence under uniform spray is sinc²(f·W), so no source-blind law can track it. Inside 0 < W < W₀, or 0 < |pitch|, pitch_rand < 0.5 st, level can be off by up to the coherent excess 10·log(N·μ₁²/μ₂). That is about 7 dB at N = 8 for a mismatched source.

The band is small in knob terms. W₀ = 5 ms is spray 0.06 % at the default 8 s span and 0.5 % at the 1 s minimum. The ±1.5 dB gate runs at the band edges:
- spray 0, pitch 0, pitch_rand 0 (c = 1)
- spray ≥ 1 %, or |pitch| ≥ 0.5 st (c = 0)
- reverse 50 % (c = 0.5)

The plan must not retune W₀/p₀ against a single source. That would fit the band to one spectrum.

**Cross-stream note:** two streams with identical coherent settings are mutually coherent, so they sum +6 dB, not +3. The law is per stream, so this is accepted and documented (it is the same as duplicating a track).

## 2. R6 — WindowMorphBank: what it must store

`wm.py` builds the bank with the ARCHITECTURE §8 segments:
- A: Tukey α 1 → 0.25
- B: lerp Tukey 0.25 → Trapezoid (12.5 % ramps)
- C: lerp Trapezoid → WindowLut Expo-Decay (2 % raised-cosine attack, e^(−5u))

| step | m | μ₁ | μ₂ | μ₂ vs Hann | N floor = μ₂H/μ₂ |
|---|---|---|---|---|---|
| 0 | 0 | 0.4998 | 0.3748 | 0.00 dB | 1.00 |
| 16 | 0.25 | 0.781 | 0.726 | +2.87 | 0.52 |
| 22–42 | ⅓…⅔ | 0.875 | 0.843 | +3.5 | 0.44 |
| 56 | 0.875 | 0.453 | 0.230 | −2.13 | 1.63 |
| 64 | 1 | 0.199 | 0.104 | −5.56 | **3.60** |

- **Store cumulative tables:** `C1[k][i] = ∫₀^{i/2048} w` and `C2[k][i] = ∫₀^{i/2048} w²`. Use 2049 entries, trapezoid-integrated in double from the float table the renderer reads (the WindowLut principle: the constants describe what is heard), and store them as float. That is 65 × 2049 × 2 × 4 B ≈ 1.07 MB, plus the 0.53 MB window tables, all built in the constructor. The law evaluates them only at the grid, by lerp. Minimum x = q/N = 32/400 = 0.08, where 2049 points is ample.
- **Skew** is WindowLut's warp with peak `t = 0.5 + 0.45·skew` (`makeTilt` clamps to [0.05, 0.95]). The warped cumulative is exact (the max error vs direct integration is 9e-6):
  ```
  q(x) = warp(x, t);  Ch = C(0.5)
  ∫₀ˣ w_t^k = x <= t ? 2t·C(q)  :  2t·Ch + 2(1−t)·(C(q) − Ch)
  ```
  The uncapped moment is `μ_t = t·mean_lo + (1−t)·mean_hi`, which is the WindowLut form. ARCHITECTURE §8's `2·p·μ_lo` is the same thing **only if** μ_lo is the half-*integral*, not the half-*mean*. Use the WindowLut form.
- **Worst capped overlap** across 65 steps × 9 skews:
  - q = 128, N = 400: **P/μ₂H = 333** (step 22, skew −1)
  - q = 32, N = 400: 77
  - Hann at q = 128, N = 400: 43.02

  The trapezoid at skew −1 reaches full level at 2.5 % of its length, so truncation barely removes any power. The E0 unit row's "worst case" must move to 333. That is −25 dB of g_out, still far from denormal or precision trouble.
- **Step 0, skew 0: keep the closed-form Hann** in the renderer (`hannWindow`) and in the law (`hannCumW2`), and branch on the latched `windowStep == 0 && skew == 0`. The branch is perfectly predicted, because it is latched per grain. This keeps the Hann ratio exactly 1, keeps E0 continuity exact, and enables R12. Every other step reads table + lerp + warp, which should cost less than the float `cos` it replaces.
- Latch at spawn: `windowStep = round(m·64)`, plus the `Tilt` {t, a, b}. The Grain POD already has `windowStep` and `windowSkew`; store the tilt's a and b, or recompute them from skew per grain at spawn into the existing float field (one division pair per spawn).

## 3. Q1 / R8 — Quotas: water-filling, the over-quota flag, the steal policy

**Water-fill (grid, 4 values, closed form):** sort the demands `N_s = density_s·size_s` of the enabled streams (disabled = 0). If `ΣN ≤ Q`, then `q_s = N_s`. Otherwise find λ with `Σ min(N_s, λ) = Q` and set `q_s = min(N_s, λ)`. λ is continuous and piecewise linear in the demands, so quotas, and therefore g_out, cannot step. Muted and non-soloed streams keep their demand (CONTEXT).

**Steal policy** (`wf.py`; it slots into 2.1's `Spawner::acquire` without restructuring):
```
live_s     = live, non-killing grains of stream s     (counted in the existing 128-slot scan)
overQuota  = live_s > ceil (q_s)                      // R8: NOT live_s > q_s
progress(g) = 2·overQuota[g.stream] + (g.stream == spawning) + age·invLen
progress'(g) = (killing ? 3 : 0) + progress(g)        // 2.1 R3, unchanged
```
- Counting `live_s` needs one extra pass over 128 slots per spawn (≤ 800 spawns/s), or a first pass that fills `live[4]` before the argmax pass. It is deterministic, because it reads pool state. It decides a victim and never enters a gain law (2.1 R3 rule).
- **One stream reduces to the 2.1 victim.** The +2 and +1 terms are equal for every candidate, so the argmax is unchanged. R12 holds for steal renders too.
- A just-disabled stream has q = 0, so its ringing grains are flagged and stolen first. That is the right priority.

**Measured, per stream, measured power vs `q_s·M(q_s/N_s)` (dB):**

| Case (N_s; Q) | fair (ceil) | native steal-oldest |
|---|---|---|
| 200, 1, 1, 1; 128 | −0.09 … 0.00 | −0.48 on small |
| 200, 100, 1, 1; 128 | ≤ 0.04 | **+4.75 / −4.13** |
| 50 × 4; 128 | ≤ 0.03 | ≤ 0.03 |
| 400, 50, 10, 1; 64 | **+0.65** / ≤ 0.13 | **+15.8 / −25.2** |
| 200, 1, 1, 1; 32 | +0.25 / 0.00 | +1.9 / **−18.3** |
| 60, 60, 0.1, 0.1; 96 | −0.14 / −0.18 (−0.87 without ceil) | −0.14 / −0.20 |

FUNC-03 starvation gate: run "400, 50, 10, 1 @ 64" with per-stream RMS against each stream rendered alone at its quota-law prediction. Use the native-progress hook as the **negative control**: it must fail by > 10 dB.

## 4. R9 — Scale quantiser (D4) over pitch-collections

- **Index mapping:** `sN_scale` 0 = Off; 1…5 map to gs Id 0…4; 6…31 map to gs Id 6…31; 31 = Custom reads the audio-side custom copy. The mask is keyed by **spec index** and **1-based stream**: `maskIndex(stream 1…4, collection 1…31)` (verified in `PluginProcessor.h:132`). The table lookup goes through `GranulatorParamIds::scaleChoiceToId`. Two traps:
  - The engine's streams are 0-based, so pass `s + 1`.
  - `PluginProcessor.cpp:242` passes `PitchCollections::Custom` as a *collection*. That is correct only because both indices are 31.

  Add a unit row: the mask for spec 1 (Chromatic) ≠ the mask for gs-Id 1 (Major) when they are set differently.
- **Rebuild key** (GS `rebuildIfNeeded` parity): (scale, mask, span = pitch_rand·100 ¢, customVersion). Build the active cents in **[−span, +span]**, and look up the mask bit with the **original** degree index `d`, then **sort the active array**. An insertion sort over ≤ 512 entries runs only at rebuild. The sort fixes 7-Limit Just (12/7 = 933 ¢ after 7/4 = 969 ¢, memory). Sorting the source table first would misalign the mask bits.
- **Don't key on base pitch.** `sN_pitch` is a mod destination in 2.5, so keying on it would rebuild every grid tick. At pick time, restrict to the window `[max(−span, −2400 − 100p), min(span, 2400 − 100p)]` with two `lower_bound`s, then `pick = active[lo + draw.nextInt(Slot::Pitch, hi − lo)]`. Every pick is a degree (the gate requirement), with no pile-up at ±24 st. The final ±24 clamp (D4) stays as a safety no-op. If `hi == lo`, the offset is 0.
- `CounterRng::Draw::nextInt(slot, n)` exists (`uniformInt`, top 32 bits × n >> 32). Use the same `Slot::Pitch` as the Off path. The two are mutually exclusive, and the draw happens only when pitch_rand > 0 (draw before obtain).
- **Precision:** float cents (e.g. 386.3137 ¢) → `rate = exp2((p + c/100)/12)` in double. The error is ~3e-5 ¢, against a 1 ¢ gate.
- Gate rows:
  - **Record-based**, for all 31 collections, including Bohlen-Pierce and Custom: `1200·log2(rate) − 100·pitch` is within 1 ¢ of an active member inside the span. The empty mask gives `rate == exp2(p/12)` exactly.
  - **Audio**: a 1 kHz sine through 5-Limit Just with only 5/4 active; the FFT peak lands at 1250 Hz ± 1 ¢.
  - **Monotone**: the 7-Limit Just active array is sorted.
- **Custom table to the audio thread.** The processor already keeps `customTable` under a `juce::SpinLock` with an atomic `customVersion`. At the grid, when the version changes, the audio thread takes a **`ScopedTryLock`** and copies the table (≈ 264 B). If the try fails, it retries next grid. A try-lock never waits, it reuses the existing state, and it needs no new double buffer. This is a simpler equivalent to CONTEXT's "double buffer + version"; the plan should pick one, and I recommend the try-lock. The masks are already `atomic<uint64_t>` (relaxed loads at the grid). The custom-mask reset already happens on the message thread inside `setCustomScale`. ✓

## 5. R10 — Stereo spread and gain rand

- **Spread:** `x = spread·(2u − 1)` (Slot::Pan, drawn only when spread > 0), then `(gL, gR) = panGains(x)`, which is 2.1's √2 balance and snaps to exactly (1, 1) at 0. Latch it into `Grain::gainL/R`. Stream pan stays the 10 ms-ramped **bus** balance.
  - For a symmetric x, E[gL²] = E[gR²] = 1 exactly, so the mean power equals the bus balance at **any** stream pan. There is no edge pile-up (a clamp of `pan_s + x` piles half the grains at the edge when hard-panned).
  - At a hard pan, spread becomes per-grain level variation. That is inherent with one active side.
  - Coherent-subset factor: `E[gL] = E[gR] = (4/(π·s))·sin(π·s/4)`: −0.16 dB at 30 %, −0.9 dB at 100 %. It multiplies S1 in §1.2, and equals 1 at s = 0.
- **Gain rand (D5):** `g = exp10(−gain_rand·u·24/20)` in double, then float (Slot::Gain, drawn only when gain_rand > 0). Multiply it into gainL/R. The output law ignores it (D5).
  - Gate: at 0, the render is bitwise equal to the no-draw path.
  - Gate: at 100 %, every latched gain is in [10^(−24/20), 1].
- **Draw order inside `plan()`** is fixed and append-only: the existing Size, Pitch, Reverse, PosSpray, then Pan, then Gain. Every draw happens before `acquire()`.

## 6. Mixer, ramps, streams

- **Mute/solo:** `audible_s = !mute_s && (!anySolo || solo_s)`, evaluated at the grid. A per-stream 10 ms `ExactRamp` toward 0 or 1, multiplied into `streamGainL/R` with level and pan.
  - 10 ms = 15 cells at 48 k. Add `rampCellsFor(ms)`; 2.1 hard-codes 20 ms in `kBaseRampMs`.
  - If level and pan also move to 10 ms (CONTEXT), level-automation renders differ from 2.1. Static renders, and R12, are unaffected.
- Remove the P5 `s == 0` mask in `gridTick()`. Everything else is already `[4]`. The onset merge breaks ties on (offset, stream), and the epoch survives re-enable (R4).
- Disabled-stream gate (FUNC-03): zero scheduler calls and zero draws. Count `Draw` constructions per stream behind a test hook. Zero onsets alone does not prove zero draws.

## 7. R12 and the gate recipes

**First row (before any spawner or pool change), the steal-free bit-identity row:**
- Settings: `grain_limit` 128, one stream, N = 100 (density 100 × 1 s, jitter 20 %).
- Assert `steals == 0 && kills == 0`, otherwise the row is vacuous.
- Compare the engine path against a `TestHooks::nativeAcquire` path: `pool.tryObtain() ?: pool.obtain(age·invLen)`, bypassing `Spawner::acquire`. Require max|Δ| == 0.
- **Negative control:** the same pair at N = 300 must **differ**, which proves the hook bypasses something.
- Land it on the 2.1 tree first, then keep it green through every 2.2 change.

**R12 2.1-equivalence null (recommended):**
- Build a seeded out-of-tree pair from `git archive 6508c635` (memory: null test needs a seeded out-of-tree pair).
- Settings: s1 only, spray 10 % (so c = 0), window 0, skew 0, spread 0, gain_rand 0, scale Off.
- Cover steal-free and capped (`grain_limit` 32) renders.
- Pass condition: max|Δ| == 0. It proves the 4-stream refactor, the fair progress and the law rewrite changed nothing they shouldn't.

**Level gates (R7 domain):**

| Gate | Recipe |
|---|---|
| DSP-03 morph × skew ±1 dB | N = 40 (density 200 × 200 ms), uncapped, spray 10 %, c = 0, noise, averaged 4 seeds; 65 steps × skew {−1, −0.5, 0, 0.5, 1}. Plus one capped row (N = 400, q = 128) at the worst corner (step 22, skew −1) |
| Density flatness ±1.5 dB | size **1000 ms**, density {1, 2, 5, 10, 20, 50, 100, 128, 160, 200}; rows c = 1 (spray 0), c = 0 (spray 10 %), c = 0.5 (reverse 50 %); band-limited noise **and** sine 440 at jitter 20 %; sine rows average ≥ 8 seeds (R11); plus an old-vs-old control row |
| The 2.1 hot spot is gone | The c = 1 row at density 200: 2.1 reads +19.5 dB, 2.2 must read within ±1.5 dB. That is the negative control |
| Sub-floor (N < floor) | peak ≤ the window peak × level (no boost) |

Other rows carry over from CONTEXT unchanged (reverse binomial, steal click with a tonal 0.001 source and checked overflow, mute/solo vs the same-motion reference, block-size null with 4 streams and all randomisation plus a different-seed control, alloc with scale/mask changes and a custom swap). Use the 2.1 recipes in RESEARCH-2.1 §9.

## 8. Pitfalls checked

- Laws and quotas come from latched parameters at the grid. `live_s` appears only in the victim choice (a decision, not a gain).
- Draw before obtain, and each randomisation at 0 draws nothing (spread, gain, pitch). The Pan and Gain slots are already reserved in `Slot`, so nothing is renumbered.
- Bitwise unity paths: `panGains(0)`, gain_rand 0 and closed-form Hann at step 0 / skew 0 all keep the steal-free and R12 nulls exact.
- No APVTS writes from mute/solo (additive solo, no exclusivity in listeners).
- `ExactRamp`, not `SmoothedValue`. Mixer ramps never derive unity from a skewed `get()`.
- The custom table is copied on the audio thread only through a non-blocking try-lock. Masks are reset on the message thread.

---

## Next

Ready for `/plugin-plan O-Granulator 2-dsp` (Phase 2.2). The plan must:
- carry R5–R12 as tasks or acceptance rows;
- put the steal-free row first, on the 2.1 tree;
- record two errata: ARCHITECTURE §10's κ variables are superseded by §1.2, and the E0 worst case moves from 43.02 to 333.
