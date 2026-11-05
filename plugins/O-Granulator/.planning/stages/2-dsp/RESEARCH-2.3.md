# Stage 2 — DSP · Phase 2.3 (Per-grain FX + CPU budget) — Research

**Plugin:** O-Granulator
**Stage / phase:** 2-dsp / 2.3 of 6
**GSD phase:** research
**Date:** 2026-11-05
**Inputs:**
- `CONTEXT-2.3.md` (D6–D8 locked; four open questions)
- `RESEARCH-2.2.md`, `VERIFICATION-2.2.md`
- ARCHITECTURE §9, §14 and Algorithm §9
- the 2.2 engine in `Source/dsp/`: `Engine.cpp` (`process`, `renderSegment`), `Spawner.h` (`plan`, `fill`), `Grain.h`, `EngineConstants.h` (`Slot`)
- grain-engine 1.1.0 `GrainPool.h` (`activate` → `Grain {}`)
- the render harness C2 row and `tests/r12/run.sh`

**Scratch experiments:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/f15c125e-10af-4880-9024-406e92fa25ce/scratchpad/r23/`

| Script | What it does |
|---|---|
| `drive.py` | the D6 law in float32: peak, RMS and fundamental across drive; small-d deviation from x; tanh vs Padé |
| `ring.py` | float32 quadrature rotator: magnitude drift with no / per-32 / per-sample renormalisation, 5 Hz … 0.45·fs |
| `svf.py` | TPT SVF peak gains (LP / BP k·v1 / HP) across res; the per-grain audio metric and its shared-latch negative control; ring carrier recovery |
| `bench.cpp` | C++ `-O3` cost of each FX stage per grain-sample, 128 grains, 32-sample passes |

All four open questions are answered below. There are **ten findings**. Three of them (R13, R14, R15) would make a 2.3 gate fail or pass vacuously if missed.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R13** | `process()` splits pieces at **host-block boundaries** (`len = min(numSamples − done, kCell − cellPos)`). So `renderSegment` boundaries depend on the host block size. Any FX operation keyed to "the pass" (ring renormalisation, the crush phase, a hoisted state write) **breaks the N5 block-size null** | **Blocks PERF-03** | Each FX operation is per sample, or keyed to grain age / the absolute grid. Never keyed to a segment. Use the gather → FX → scatter kernel (§1) |
| **R14** | The ring drift is worst at **low** carrier frequencies, not at 0.45·fs. With renormalisation every 32 samples, the drift is **−115 dB at 5 Hz** and −118 dB at 20 Hz, so it misses the −120 dB target. A per-pass renorm would also violate R13 | Q2: target missed | One Newton step **per sample**: `m = 1.5 − 0.5·|c|²`. Measured ≤ −135.5 dB everywhere. It is block-invariant and branch-free, and costs ≈ 0.3 % of a core |
| **R15** | D6 is **peak**-unity. At full drive a −12 dBFS sine keeps its peak (+0.0000 dB), but its RMS rises **+2.24 dB** and its fundamental +1.86 dB. A ±0.1 dB gate on RMS fails from drive ≈ 0.3 | **Law gate would fail** | The D6 law gate measures the steady-state **sample peak**. RMS and fundamental are INFO rows. Compute the normaliser with the **same** function as the per-sample shaper |
| R16 | The float law is **never** bitwise x for d > 0 (only 95 % of samples are exact at d = 1e-6). For d ≤ 1e-4 the deviation is ≤ 1.6e-7 relative (≈ 1 ulp) | Q1 closed | Drive is inactive when `d < 1e-4` (drive < 0.25 %). The step at the guard is ≤ 1.4e-6 dB, so it is continuous. Use `std::tanh`; a Padé shaper is a spare lever (§2.3) |
| R17 | "Audibly per-grain" needs **two layers**. A latched-value census catches a latch bug but not a renderer that ignores the latch. Audio: Spearman(latched fc, per-grain log-centroid) = **0.998** for BP res 0.8, against **+0.17** for a shared-latch negative control. Ring carrier recovered within 2 Hz on **40 / 40** grains | Q3 closed | Census row plus audio rows (BP centroid, ring carrier), each with a `fxSharedLatch` hook as the negative control. LP/HP are covered by the census and the unit response (the HP centroid spans only 0.69 oct) |
| R18 | BP = k·v1 peaks at **exactly 0 dB** at every res, including fc = 0.45·fs, because the bilinear transform preserves the peak. LP/HP peak at +0.98 / +6.6 / +10.8 / **+20.0 dB** at res 50 / 80 / 90 / 100 % | D8 confirmed | Gate the BP law as a steady-state sine magnitude, **not** a sample peak on noise (the transient ‖h‖₁ > 1) |
| R19 | Headroom (Q4): the coherent law gives a stream wet of **+20 dB** at res 100 % on a tone at fc. With 4 coherent streams and a +6 dB level, the wet can reach **+38 dBFS** before any safety. 2.3 has no wet safety, and 2.4's `1.9953·tanh` will squash this hard | Note for 2.4 | QUAL-02 gates on **finite**, never on ≤ 1. Carry a 2.4 discuss item (§6) |
| R20 | The reserved draw slots **already exist**: `FxCutoff`, `FxCrushProb`, `FxRing` in `Slot`. Nothing needs to be appended | Supersedes a CONTEXT default | Use the reserved slots. `FxReso`, `FxDrive`, `FxCrush`, `Window` and `Skew` stay reserved and undrawn |
| R21 | CPU: all four FX on cost **+4.1 % (Padé) to +5.5 % (`std::tanh`)** of one core at 128 grains, 48 k. Projected worst case for 2.3: **≈ 13–15 %**, against the 30 % budget | PERF-02 on track | No lever is needed. C3 reports the measured value with headroom for 2.4–2.6 |
| R22 | D7 consequence: mid-tread silences any grain whose peak is below ½ LSB. That is **−12 dBFS at crush 100 %**, −20.5 dBFS at 90 %, and −33 dBFS at 75 %. A typical −18 dBFS live grain vanishes above crush ≈ 93 % | UX note, not a defect | No change to D7. Record it for the 2.4 listening checkpoint (§5) |

---

## 1. R13 — Kernel structure: gather → FX → scatter, never keyed to a segment

`Engine::process` renders each piece as `len = min(numSamples − done, kCell − cellPos)`. `renderSegment(from, to)` is then split again at every onset. A 441-sample host block and a 32-sample host block therefore produce **different segment boundaries** for the same audio. N5 (2.2) only passes because every 2.2 per-grain operation is per sample. 2.3 must keep it that way:

- **Per-sample state only.** SVF integrators, the crush hold phase, the ring rotator and the drive shaper all advance once per sample, whatever the segment length.
- **No per-segment numerics.** A ring renormalisation "per 32-sample pass" would run at different absolute samples for different host blocks, so the null would fail. R14 moves it into the sample loop.
- **The NaN kill may be per segment**, because it is a no-op whenever the state is finite (as it is in every null render). Detection: `std::isfinite (ic1[0] + ic2[0] + ic1[1] + ic2[1])` after the SVF stage. On failure, zero this segment's FX buffer before the scatter and retire the grain (`return false`). No NaN ever reaches `bus`. The build has no `-ffast-math`, so `isfinite` is not optimised away (checked).

**Recommended kernel** (inside the existing `pool.render` lambda; `n = to − from ≤ 32`):

```
1. gather   — the existing per-sample loop, unchanged: readGrain → l[i], r[i];
              w[i] = window · killGain · fade.next(); advance readAbs / age /
              killGain; stop at the retire condition → n' samples, retire flag.
2. FX       — one tight loop per ACTIVE stage over l[0..n'), r[0..n'):
              if (fx & kSvf)   svf  (g, l, r, n');   // + NaN check
              if (fx & kDrive) drive(g, l, r, n');
              if (fx & kCrush) crush(g, l, r, n');
              if (fx & kRing)  ring (g, l, r, n');
3. scatter  — bl[from + i] += w[i] · l[i] · g.gainL;  (same for R)
```

- **Neutral FX is bitwise 2.2.** With `fxActive == 0` the gather and scatter compute exactly the same float expressions in the same order (`w * l * gainL` with `w` formed identically). That is the precondition for the neutral null (R22's row in §7).
- The flag test is hoisted per stage per segment, as CONTEXT asks. The stage functions are free functions over arrays in a new `GrainFx.h`, so the law gates (D6, D7, D8) run as **unit rows on those functions** directly, with no engine render.
- Fade slots run through the same lambda, so a stolen grain keeps processing its copied FX state (`Grain` stays trivially copyable).
- The temporary arrays are `float[kCell]` locals inside the lambda. Every per-pass array stays fixed at 32 samples (CONTEXT constraint).

## 2. D6 drive (Q1) — R15, R16

### 2.1 What D6 holds and what it doesn't (`drive.py`; a 441 Hz sine with peak 0.25, float32)

| drive | d = 16·drive² | peak dB | RMS dB | fundamental dB |
|---|---|---|---|---|
| 0.10 | 0.16 | +0.0000 | +0.001 | +0.001 |
| 0.25 | 1.00 | +0.0000 | +0.044 | +0.044 |
| 0.50 | 4.00 | +0.0000 | +0.573 | +0.553 |
| 0.75 | 9.00 | +0.0000 | +1.578 | +1.414 |
| 1.00 | 16.0 | +0.0000 | **+2.237** | +1.861 |

The law pins `y(±r) = ±r`. Saturation then squares the waveform off, so the power rises at a fixed peak. **The gate is the steady-state sample peak** (and a unit row `y(r) == r` within 1 ulp). RMS and fundamental are INFO rows.

Because `y(r) = r·f(dr)/f(dr)`, the peak identity holds for **any** odd shaper f, provided the latched normaliser `r / f(d·r)` is computed with the **same** f as the per-sample shaper. If the normaliser used double `std::tanh` and the shaper used float `std::tanh`, the peak would be off by rounding only. If the shaper were Padé, it would be off by the 2 % approximation error. Compute the normaliser at spawn in float, through the same function.

Other values:
- Small-signal gain at full drive: +12.05 dB. ARCH's d = 1 floor would have jumped +0.18 dB at the first step above 0.
- Output bound `r / tanh(d·r)`: 1.02 at drive 0.25, 0.33 at 0.5, 0.25 at 1.0. This matters for headroom (R19).

### 2.2 The small-d guard (R16)

| d | drive | max relative \|y − x\| over \|x\| ≤ 4 | samples bit-exact |
|---|---|---|---|
| 1e-6 | 0.025 % | 1.05e-7 | 94.9 % |
| 1e-4 | 0.25 % | 1.59e-7 | 54.4 % |
| 1e-3 | 0.79 % | 5.4e-6 | 6.2 % |
| 1e-2 | 2.5 % | 5.3e-4 | 0.1 % |

No guard makes the law bitwise ≡ x, so "≡ x numerically" means within float rounding. **Make drive inactive when `d < 1e-4`** (drive < 0.25 %). Below that, the law differs from x by ≤ 1.6e-7 relative (≈ 1 ulp, ≤ 1.4e-6 dB), so the bypass step is continuous. Drive exactly 0 is the bitwise bypass that the neutral null needs.

### 2.3 Cost: `std::tanh` vs Padé (`bench.cpp`)

| shaper | added % of a core (128 grains, 48 k, stereo) | shape error |
|---|---|---|
| `std::tanh` (float) | +1.4 % | — |
| Padé `z(27 + z²)/(27 + 9z²)`, clamped at \|z\| = 3 (C¹ there) | +0.1 % | ≤ 2.3 % of r; RMS +2.30 dB vs +2.24 dB at full drive |

Use **`std::tanh`**: the budget allows it (R21), and the law stays exact. Padé is a spare lever ahead of ARCH's list if a later phase runs out of CPU. Switching would move only the shape, not the D6 peak gate, as long as the normaliser switches too.

## 3. Ring (Q2) — R14

`ring.py`: the float32 rotator `c ← c·rot`, with `rot = (cos ω, sin ω)` rounded to float. Each row is the worst `||c| − 1|` over 1 s at 48 k.

| renormalisation | 5 Hz | 20 Hz | 200 Hz | 5 kHz | 0.45·fs |
|---|---|---|---|---|---|
| none | — | −58.0 dB | −71.7 | −82.4 | −120.2 |
| every 32 samples | **−114.9** | **−118.2** | −127.5 | **−119.0** | −120.1 |
| every sample (Newton) | −135.5 | −138.4 | −138.9 | −137.9 | −148.7 |

- **The drift is worst at low frequency.** `|rot|` rounds to 1 ± ε, and at low ω the rounding bias of cos ω ≈ 1 accumulates coherently. CONTEXT's "0.45·fs worst case" is the wrong corner. The ring range reaches 5 Hz (20 Hz × 2⁻² at ring_rand 100 %).
- **Per-sample renormalisation:** `m = 1.5f − 0.5f·(re² + im²); re *= m; im *= m`. That is four extra flops, no branch, block-invariant (R13), and ≤ −135 dB.
- Normalising `rot` in double before rounding changes nothing measurable (identical rows). Compute it with `std::cos` / `std::sin` in double at spawn, then cast.
- Phase error after 1 s is ≤ 7e-4 rad, so the frequency is exact for all practical purposes.
- Carrier start `c = 1 + 0j` (CONTEXT default). The ring has no draw-free bypass problem: ring_mix 0 → inactive, and the frequency draw is gated on `ring_rand > 0 && ring_mix > 0`.

## 4. "Audibly per-grain" (Q3) — R17

A broken latch can fail in two ways, and each needs its own detector:

| Failure | Census of latched values (spawn log) | Audio measurement |
|---|---|---|
| The spawner latches one shared value for every grain | **fails** ✓ | fails ✓ |
| The latch is right, but the kernel reads a live / stream value instead of the grain's | passes ✗ | **fails** ✓ |

**Census row (record-based, exact).** Extend the test-hook spawn log with `fc`, `ringHz`, `crushed`, `driveD` and `fxActive`.
- Cutoff and ring: `log2(value / base)` spans ≥ 3.6 of the ±2 oct over ≥ 200 grains at rand 100 %, and it is exactly 0 at rand 0. Clamped draws sit at the clamp (CONTEXT: no rescale).
- Crush Bernoulli: exactly 0 % and 100 % at prob 0 / 1; 50 % within the binomial 99.9 % bound.

**Audio rows (render-based), each with a `fxSharedLatch` test hook as the negative control.** The hook makes the kernel use the stream's un-randomised value.
- **BP centroid:** white noise, isolated grains (no overlap: size 100 ms, density 5, jitter 0, spray 10 %), BP res 80 %, cutoff 2 kHz, rand 100 %, ≥ 40 grains. Measure the per-grain log spectral centroid against the latched fc.
  - Measured Spearman ρ = **0.998**. The negative control gives **+0.17** (its centroid spread collapses from 3.6 to 0.13 oct).
  - Gate: ρ ≥ 0.9 on the subject, |ρ| ≤ 0.5 on the negative control.
  - LP gives ρ 0.983. HP gives 0.987, but its centroid spans only 0.69 oct on white noise, so it is a weak audio probe. The audio row uses BP; LP/HP rely on the census plus the unit response.
- **Ring carrier:** a 1 kHz sine, ring mix 100 %, ring_rand 100 %. The per-grain sideband peak gives `|f̂ − 1000|`.
  - Recovered within 2 Hz on **40 / 40** grains (53–708 Hz).
  - Gate: ≥ 95 % within 2 Hz. The negative control must fall below 20 %.
- **Drive and crush** carry no randomisation beyond the crush Bernoulli. Their per-grain audio row is the crush on/off split: on crushed grains the isolated per-grain FX tap shows ≤ 2^b distinct values per hold; on uncrushed grains it shows the source.

## 5. Crush — D7 details and R22

- **Hold:** at spawn, `inc = (1000/fs)^crush` (double `pow`, then float) and `phase = 1`. Per sample: `if (phase >= 1) { phase −= 1; held = q(x); }  out = held;  phase += inc;`. The first sample is always captured (CONTEXT "fresh hold"), and the logic is per sample (R13).
- **Quantiser:** `scale = 2^(15 − 14·crush)` latched; `q(x) = std::nearbyint(x·scale) / scale`. Silence gives 0 or −0, both of which compare `== 0` (the D7 gate). Denormal inputs flush under `ScopedNoDenormals`. There is no clip: |x| > 1 passes quantised (for example after a resonant LP).
- **Draw:** `Slot::FxCrushProb`, only when `crush > 0`. Gate the probability to (0, 1) before drawing, as reverse does, so 0 % and 100 % are exact and draw nothing.
- **R22, the silence threshold** ½·LSB = `0.5 / scale`:

| crush | bits | the grain is silent when its peak is below |
|---|---|---|
| 100 % | 2 | −12.0 dBFS |
| 93 % | 3.0 | −18 dBFS |
| 90 % | 3.4 | −20.5 dBFS |
| 75 % | 5.5 | −33 dBFS |

D7 accepted "quiet tails gate to zero". The consequence is that the top ≈ 10 % of the crush knob mutes ordinary live input. That fits the classic bitcrusher, but it should be heard at the 2.4 listening checkpoint. This is a note, not a change.

## 6. SVF — R18 and headroom (Q4) — R19

**Coefficients** (Cytomic TPT, latched at spawn, float):
- `g = tan(π·fc/fs)` in double; `k = 2 − 1.9·res`; `a1 = 1/(1 + g(g + k))`; `a2 = g·a1`; `a3 = g·a2`.
- Per sample, per channel: `v3 = x − ic2`; `v1 = a1·ic1 + a2·v3`; `v2 = ic2 + a2·ic1 + a3·v3`; `ic1 = 2v1 − ic1`; `ic2 = 2v2 − ic2`.
- Outputs: LP `v2`, BP `k·v1` (D8), HP `x − k·v1 − v2`.
- The filter is stable for every k > 0 at any g, so k = 0.1 at 0.45·fs is safe.

**Peak gain** (`svf.py`, analytic through the prewarped bilinear map, confirmed in the time domain):

| res | k | LP / HP peak | BP (k·v1) peak |
|---|---|---|---|
| 0 / 20 % | 2.00 / 1.62 | 0.00 dB | 0.00000 dB |
| 50 % | 1.05 | +0.98 dB | 0.00000 dB |
| 80 % | 0.48 | +6.63 dB | 0.00000 dB |
| 90 % | 0.29 | +10.84 dB | 0.00000 dB |
| 100 % | 0.10 | **+20.01 dB** | 0.00000 dB (−0.00001 at 0.45·fs) |

- **BP gate:** measure the steady-state magnitude of a sine at fc, swept across res and fc (including the 0.45·fs clamp): ≤ +0.1 dB. Do **not** gate the sample peak of noise or a transient. A resonant BP's ‖h‖₁ exceeds 1, so a broadband input can legitimately peak above its input peak.

**Headroom (R19).**
- The output law ignores FX (D8). A tonal input at fc through LP or HP at res 100 % therefore leaves each grain +20 dB hot.
- At c = 1 the cloud sum is amplitude-coherent and the law normalises it to unity, so the **stream wet is +20 dB**.
- Four identically-set streams add coherently (+12 dB, RESEARCH-2.2 cross-stream note), and the stream level can add +6 dB. Worst case: **≈ +38 dBFS** wet from a 0 dBFS input.
- Drive after the SVF caps this: at drive ≥ 50 % the per-grain output is ≤ 0.33. Crush and ring cannot add gain.
- For 2.3: the QUAL-02 stress row asserts `isfinite` on every sample and denormal-free state. It never asserts a peak ≤ 1, because 2.3 has no wet safety.
- For 2.4 (carry into its discuss): ARCH §14's wet safety `1.9953·tanh(x/1.9953)` will turn a +38 dBFS resonant wet into a +6 dBFS near-square wave. That is safe, but it is a character decision. Two options for 2.4:
  - **(a)** accept it as the safety it is;
  - **(b)** add a per-stream LP/HP resonance make-down, `1/peak(k)`, latched at spawn. That is analytic: `peak = 1/(k·√(1 − k²/4))` for k < √2. It would amend D8.

## 7. Draw slots, neutral null, bleed, stress, block size, allocations

**R20, draw slots.** `EngineConstants.h` already reserves the following, in order: `FxCutoff`, `FxReso`, `FxDrive`, `FxCrush`, `FxCrushProb`, `FxRing`, `Window`, `Skew`.
- Use `FxCutoff` (filter_rand), `FxCrushProb` and `FxRing` (ring_rand). Append nothing.
- Each draw happens in `plan()` before `acquire()`, and only when:
  - `FxCutoff`: `filterType != Off && filterRand > 0`
  - `FxCrushProb`: `crush > 0 && 0 < crushProb < 1`
  - `FxRing`: `ringMix > 0 && ringRand > 0`
- CounterRng is keyed by slot, so no existing draw moves. **W2o is untouched:** every 2.2 row runs with neutral FX, and the neutral null below proves those renders bitwise equal. Widening seeds is therefore unnecessary unless a 2.2 row changes.

**`GrainPlan` and `fill`.** `Spawner::fill` assigns fields one at a time. Add the FX latch fields to `GrainPlan` and assign every one in `fill`: `fxActive`, `svfMode`, `svfK`, `svfA1..3`, `driveD`, `driveNorm` (new field, CONTEXT carry-in), `crushHoldInc`, `crushScale`, `ringRotRe/Im`, `ringMix`.
- Do **not** assign the state fields (`svfIc*`, `crushHoldPhase`, `crushHeld`, `ringRe/Im`). `GrainPool::activate` value-initialises the slot (`grains[i] = Grain {}`, `GrainPool.h:238`), and that is the no-bleed guarantee.
- The one exception is `crushHoldPhase = 1` (§5). Either set it in `fill` or make it the `Grain` default; the default is cleaner because value-init then provides it.
- The default for `fc` at 20 kHz is clamped to `0.45·fs`, which is 19 845 Hz at 44.1 k. That is still "active" when the type is LP. Accepted (CONTEXT).

**Neutral-FX null (first row, before any FX row counts).** Use the R12 machinery (`tests/r12/run.sh`) with `OLD_SHA = c10d0f2c`, the verified 2.2 tree. Use brace-form zsh expansion when scripting revpaths (memory).
- Cases: the R12 steal-free and capped renders with 4 streams.
- **Non-default but neutral dials:** cutoff 1 kHz, res 90 %, filter_rand 100 % with type Off; ring_freq 3 kHz, ring_rand 100 % with mix 0; crush_prob 37 % with crush 0.
- Pass: max|Δ| == 0. This proves no stage activates and no draw happens off its gate.
- Negative control: the same render with drive at 1 % must differ.

**Bleed (DSP-05).**
- Slot reuse: render a resonant grain (BP res 100 %, a sine at fc) into slot k. When it retires, the next grain into slot k on silence must output exactly 0 for its first 32 samples.
- Negative-control hook `fxNoReset`: `fill` copies the previous occupant's `svfIc*` and `crushHeld`. It must output non-zero.
- Steal: a native-stolen grain's fade-slot copy keeps `svfIc*` (compare the fade-slot state with the pre-steal state, bitwise).

**QUAL-02 stress.** Settings: res 100 % × LP/BP/HP, drive 100 %, crush 100 %, ring 100 % at 5 kHz with rand 100 %, 4 streams × 32 (cap 128), on full-scale noise and on a sine at fc.
- Every output sample `isfinite`.
- At the end, no grain state field `fpclassify == FP_SUBNORMAL` (scan the pool through a hook).
- Plus a NaN-injection row: a hook writes NaN into one live grain's `svfIc1`. That grain retires within its segment, the bus stays finite, and the other grains are unaffected.

**PERF-03 block-size null (N6).** The N5 recipe with every FX on and randomised: all four FX on all streams, LP/BP/HP across streams, filter_rand, ring_rand and crush_prob at 50 %. Host blocks 441 / 512 / 4096 / variable vs 32: max|Δ| == 0. A different-seed control must differ. This is the row that R13 / R14 protect.

**PERF-01 allocations (M3).** The M2 coverage plus sweeps of filter type (including Off ↔ LP mid-render), drive, crush and ring, plus spawn and steal bursts: 0 allocations. The FX add only latched floats and local `float[32]` arrays, so this should hold trivially.

**QUAL-01 sweep click (K2).** Values are latched, so a sweep can only step between grains. Use the K1 κ method: the subject is a cutoff sweep 200 Hz → 8 kHz (LP res 50 %) and a ring-frequency sweep; the reference is the same render with the parameter static. Gate: κ ratio of sweep vs static ≤ 2.0.
- Negative control: a hook that applies the live cutoff per segment (not latched) and steps the coefficient mid-grain. It must exceed 2.0. That is the click the latch prevents.

## 8. CPU — R21 (`bench.cpp`, Apple Silicon, `-O3`, no fast-math)

| stage | added ns per grain-sample | added % of a core (128 grains, 48 k) |
|---|---|---|
| SVF, stereo | 1.94 | 1.19 |
| drive, `std::tanh` | 2.28 | 1.40 |
| drive, Padé | 0.20 | 0.12 |
| crush (hold + quantise) | 1.30 | 0.80 |
| ring (per-sample renorm) | 2.42 | 1.49 |
| **all four** (Padé / tanh) | 6.6 / ≈ 8.9 | **4.1 / ≈ 5.4** |

- 2.2's C2 measures 8.92 %, so the 2.3 worst case is **≈ 13–15 %**. Allow ×1.5 for the bench's hot cache and the real kernel's gather/scatter: still ≤ 18 %, against the 30 % budget, which leaves about 12 % for feedback, MPM and reverb (2.4–2.6).
- **No lever needed.** The plan keeps ARCH's lever order for 2.6.
- The kernel's per-stage `if` hoisted per segment is enough. Specialised loops per active combination (16 variants) buy nothing measurable at these costs; skip them.
- **C3 row** (non-blocking INFO, like C2): 4 streams × 32 grains, all FX on (LP res 100 %, drive 100 %, crush 100 %, ring 100 %), 48 k / 128, Release. Report % of a core and the ratio to C2. Also a **typical-preset** row: 4 streams × ~16 grains, filter + drive only. Target ≤ 10 %.

## 9. Pitfalls checked

- No `juce::dsp::StateVariableTPTFilter` or `IIR::Filter`. The SVF is 8 floats of hand-rolled state inside the POD.
- `tan`, `pow`, `exp2`, `cos` and `sin` run only at spawn. Per sample: multiplies, adds, `std::tanh` and `nearbyint`.
- `fc` and `ringHz` are read as denormalised Hz from the latched `GridParams`. The skew is never inverted by hand (CONTEXT carry-in). `GridParams::Stream` gains `filterType`, `filterCutoff`, `filterRand`, `filterRes`, `drive`, `crush`, `crushProb`, `ringFreq`, `ringRand` and `ringMix`, read through the existing clamp/hold `read` / `readPct`.
- Nyquist clamp `0.45·fs` for the cutoff **and** the ring carrier (memory: SC filter Nyquist clamp).
- Every FX operation is per sample or latched (R13). Only the finite-state NaN check runs per segment, and it is a no-op on finite state.
- `Grain` stays trivially copyable (static_assert). One new field, `driveNorm`; the `crushHoldPhase` default becomes 1.

---

## Next

Ready for `/plugin-plan O-Granulator 2-dsp` (Phase 2.3). The plan must:
- carry R13–R22 as tasks or acceptance rows;
- put the **neutral-FX null vs `c10d0f2c`** first;
- implement the FX stages as unit-testable free functions (`GrainFx.h`);
- record the CONTEXT corrections:
  - the ring renormalisation is per sample, not per pass (R14);
  - the draw slots are the reserved ones, not appended (R20);
  - the D6 gate is a peak gate (R15);
- carry R19 (resonant headroom vs wet safety) and R22 (the crush silence threshold) into the 2.4 discuss.
