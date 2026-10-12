# Stage 2 — DSP · Phase 2.1 (Source → Encoder → Stereo decode) — PLAN

**Plugin:** O-simpleAmbisonics · **Stage:** 2 DSP · **Phase:** 2.1 of 3 · **Date:** 2026-10-12
**Inputs:** `CONTEXT-2.1.md` (D1–D9), `RESEARCH-2.1.md` (R1–R7, F1–F9, open items 1–10), `research/ARCHITECTURE.md` (immutable), `ROADMAP.md` §Phase 2.1, Stage-1 source + harness
**Executor:** dsp-agent
**Branch:** `main` @ `60ecb8ce` (RESEARCH-2.1 committed)

## Goal

Replace the Stage-1 pass-through with the **audible ambisonic chain minus rotation and HRTF**: conditioned mono source → distance model (soft-knee gain + air LP with D7 wet mix) → SN3D/ACN encoder with linearly-ramped order gating → identity rotation slot → 16-channel bus with meter tap → ±60° virtual-cardioid stereo decode → 64-sample delay → decoder mixer (binaural leg = placeholder) → seeded output gain. Every control quantity moves on a **fixed 64-sample control grid** (the D5 amendment) so the render is bit-identical across any host slicing even with parameters moving and orbit On. Latency stays exactly `kPartition = 64`; `setLatencySamples` is not touched.

Covers (partial or full): FUNC-01, FUNC-02, FUNC-03 (encoder side), FUNC-06, DSP-01, DSP-04, DSP-05, DSP-06, PERF-01, QUAL-01 (partial). Gated by the harness probes in T11.

## Decisions adopted from RESEARCH-2.1 (planner picks — each is binding on the executor)

| # | Decision | Source |
|---|---|---|
| **A1 — D5 amendment** | Control rate = **one step per 64 samples on a grid that persists across `process` calls** (`gridPos ∈ [0, 64)`), not per slice. Constant `kOnePole = 1 − exp(−64/(τ·fs))` (0.0645 at 48 kHz) and `kRampStep = 64/(τ·fs)` (1/15 at 48 kHz), both computed once in `prepare`. A host block shorter than 64 may contain no control step; the lerp continues toward the endpoints of the last step | R3, F8 |
| **A2 — ramps vs one-poles** | `orderGain[1..3]` and `decoderMix` are **linear ramps** stepped at the control grid (`kRampStep`; final step assigns the target exactly when within one step). Angles, elevation, `g`, `G`, `m` are one-poles that **snap** when `|target − cur| ≤ 1e-6·max(1, |target|)` | F1 (open item 2: ramp step lives at the control grid) |
| **A3 — mixer / lerp form** | Every blend is `b + t·(a − b)`: decoder mixer, air-LP wet mix, per-sample coefficient/gain lerp. Never `t·a + (1−t)·b` | F2 |
| **A4 — decoder literals** | `L = k·(0.5f·W + 0.25f·X + 0.4330127f·Y)`, `R = k·(0.5f·W + 0.25f·X − 0.4330127f·Y)`, `k = kStereoLevelMatch = 1.0f` (placeholder constexpr in `StereoDecoder.h`). Literals, never `0.5f·std::cos(…)` | F3 |
| **A5 — per-sample lerp fraction** | Within a grid, sample `j ∈ [0, 64)` uses `t = (float)(j + 1) · (1.0f/64)` (exact power-of-two scaling); the last sample of a grid lands on the current endpoint. The reference model uses the same `t` | R3 bit-identity |
| **A6 — `wrapTo180`** | `x − 360·floor((x + 180)/360)` → **[−180, 180)**. Documented in `Smoothers.h`; Stage 3 may display −180 as 180 | F4 |
| **A7 — prepare order** | Processor fills `Params` from the atomics **before** `engine.prepare(fs, maxBlock)`; `prepare` snaps every smoother/ramp/orbit phase to `Params` so block 0 is at steady state | F5 |
| **A8 — meters** | Engine owns peak accumulation across `process` calls (`resetPeaks()` / `frame()`); processor does the `hasNewData()`-gated reset at the top of each host block and one `publish()` at the end. `outPeakL/R` are post-gain, accumulated by the processor under the same rule. `TripleBuffer<MeterFrame>` is a **public** processor member (house pattern, like `apvts`) so the harness and the Stage-3 Timer read it | R7 |
| **A9 — DSP-05 wording** | Level strictly decreasing on [0.2, 20] m; HF/LF ratio **exactly 0 dB (bit-level: wet mix 0) for d ≤ 1 m** and strictly decreasing on (1, 20]; anchors 0 / −10.79 dB ratio and +8.87 / 0 / −24.19 dB level | R5 |
| **A10 — click metric** | `worstCurvature` second difference, ratio vs a matched control ≤ 2.0 (`kClickRatioCeiling`), 100 Hz tone amplitude 0.5 (480 samples/cycle), literals printed, negative control through the reference model, liveness half-circle ≥ 6 dB | R1 |
| **A11 — alloc counter** | O-Octagon `tests/render-harness/main.cpp:111–241` verbatim with provenance; no ASan ifdef; pre-sized buffers + unarmed warm-up + liveness `new` | R2 |
| **A12 — TripleBuffer** | Verbatim copy of O-GrainScatter's with the O-simpleGrain-style provenance comment, `static_assert(std::is_trivially_copyable_v<T>)`, and a "read() from exactly one thread" comment; module extraction deferred to SUMMARY's list | R6 |
| **A13 — JUCE-free `Source/dsp/`** | All 2.1 headers: `<cmath>`, `<array>`, `<algorithm>`, `<cstdint>`, `<cstddef>`, `<atomic>` (TripleBuffer), `<type_traits>` only | D3 |
| **A14 — D6 placeholder** | `DecoderMixer` binaural input = the delayed stereo leg (same two `FixedDelay<64>` outputs); with A3 the output equals the stereo leg bit-exactly in **both** decoder modes. Named in SUMMARY as a 2.3 obligation | D6, F2 |
| **A15 — D7 number** | Any comment/lesson text says the TPT pole at the 0.45·fs clamp is **−0.06 dB at 10 kHz**; the wet mix exists for bit-exactness at ≤ 1 m, not for tone | R5 correction |

---

## The control step (shared spec for T7 and T9 — write it once, the model must match it op-for-op)

```
process(mono, outL, outR, n):
  i = 0
  while i < n:
    if gridPos == 0: controlStep()               // endpoints for the next 64 samples
    run = min(n − i, 64 − gridPos)
    for j in [0, run): per-sample(i + j, t = (gridPos + j + 1) · (1/64))
    i += run; gridPos = (gridPos + run) mod 64

controlStep():
  read Params (copied by setParams once per host block; sampled here)
  orbit: if on and was off → phase = 0; if on → phase += orbitRateHz·64/fs, wrap 1
         azTarget = wrapTo180(azimuthDeg + 360·phase)   (on)   |  azimuthDeg (off)
  az   = WrapSmoother.step(azTarget)            // one-pole on the wrapped delta, snap rule A2
  el   = OnePole.step(elevationDeg)
  g    = OnePole.step(1.25/(d + 0.25))
  fc   = min(20000·min(1, d^−0.5), 0.45·fs);  G = OnePole.step(tan(π·fc/fs)/(1 + tan(π·fc/fs)))
  m    = OnePole.step(clamp(d − 1, 0, 1))
  orderGain[b] = LinearRamp.step(b ≤ order ? 1 : 0), b = 1..3 ; orderGain[0] ≡ 1
  mix  = LinearRamp.step(decoder == 0 ? 1 : 0)
  prev ← cur for {c[16], g, G, m, mix}; cur.c[acn] = Y_acn(az, el) · orderGain[band(acn)]  (encodeSH16 in radians)
  frame: shGain = cur.c, azEff/elEff = az/el, azRot/elRot = azEff/elEff (identity rotation this phase),
         orderActive = (order+1)², decoderMix = mix, latencySamples = 64

per-sample(x = mono[k], t):
  gS = prev.g + t·(cur.g − prev.g); GS, mS, c[acn] likewise          // A3 form
  u  = x·gS
  v  = (u − z)·GS; lp = v + z; z = lp + v                                // TPT one-pole (D4)
  y  = u + mS·(lp − u)                                                   // D7 wet mix
  bus[acn] = y·c[acn], acn = 0..15;  shPeak[acn] = max(shPeak[acn], |bus[acn]|)
  (identity rotation slot — ShRotator drops in here at 2.2)
  stL = 0.5f·bus[0] + 0.25f·bus[3] + 0.4330127f·bus[1];  stR = … − 0.4330127f·bus[1]   // A4, k = 1
  dL = delayL.process(stL); dR = delayR.process(stR)                    // FixedDelay<64> ×2
  binL = dL; binR = dR                                                  // A14 placeholder
  outL = dL + mix·(binL − dL); outR = dR + mix·(binR − dR)              // A3 form → exactly dL/dR this phase
```

Band map: acn 0 → 0; 1–3 → 1; 4–8 → 2; 9–15 → 3. `shGain` in the frame is `cur.c` (signed). The engine does **not** apply `outputLevel` (processor, per sample, `getNextValue()`).

---

## Tasks

### T1 — `Source/dsp/Smoothers.h`
**Files:** create. **Depends on:** none.
- `inline float wrapTo180(float x)` per A6, with the [−180, 180) note.
- `struct OnePoleSmoother { float cur, target; void snapTo(float); float step(float target, float k); }` — `cur += k·(target − cur)`, then the A2 snap compare; `step` returns `cur`.
- `struct WrapSmoother` — same, delta through `wrapTo180`, result re-wrapped.
- `struct LinearRamp { float cur; void snapTo(float); float step(float target, float stepSize); }` — `d = target − cur; cur = |d| ≤ stepSize ? target : cur + copysign(stepSize, d)`. Exact 0/1 arrival is the point (F1).
- Comment the τ = 20 ms contract and that `k`/`stepSize` are computed by the engine from the **64-sample grid**, never from the slice length.

### T2 — `Source/dsp/ShEncoder.h`
**Files:** create. **Depends on:** none.
- Lift `kSqrt3_2`, `kSqrt15_2`, `kSqrt5_8`, `kSqrt3_8`, `AmbiTrigCache`, `encodeSH16(float azRad, float elRad, float* out16)` **verbatim** from `plugins/O-GrainScatter/Source/dsp/SpatialEncoder.h` (lines 33–103). Provenance comment: origin file, "verified against the generic SN3D triple product to 4.7e-16 double / 2.8e-08 float, RESEARCH-2.1 R4", AmbiX: ACN, SN3D, no Condon–Shortley, `sin(|m|a)` for m < 0. **Do not** lift `SmoothedSHCoeffs`.
- `inline constexpr int kNumSh = 16;` `inline constexpr int shBand(int acn)` (band map above); `inline void applyOrderGains(const float* y16, const float orderGain[4], float* out16)`.
- Takes **radians**; the engine converts once per control step (`deg · π/180`).

### T3 — `Source/dsp/DistanceModel.h`
**Files:** create. **Depends on:** T1.
- `static float gainFor(float d)` = `1.25f/(d + 0.25f)`; `static float cutoffFor(float d, double fs)` = `min(20000·min(1, 1/√d), 0.45·fs)`; `static float tptCoefficient(float fc, double fs)` = `t/(1+t)`, `t = tan(π·fc/fs)`; `static float mixFor(float d)` = `clamp(d − 1, 0, 1)`.
- Control-step state: `OnePoleSmoother g, G, m`; `prevG/curG`, `prevCoef/curCoef`, `prevMix/curMix`; `float z` (TPT state); `double fs`.
- `prepare(fs)` + `snapTo(d)` (all three smoothers snapped; `z = 0`); `controlStep(d, k)`; `float processSample(float x, float t)` per the spec; `reset()` clears `z` only (smoother values are re-snapped by the engine).
- Accessor `float clampedCutoffHz() const` (the clamp **target**, for the DSP-05 clamp gate — `pattern_downstream_guard_masks_defect_gate_the_clamp_target`).
- Comment A15 (−0.06 dB at 10 kHz at the clamp) and the Nyquist-clamp memory.

### T4 — `Source/dsp/OrbitLfo.h`
**Files:** create. **Depends on:** T1.
- `double phase` (double so a 2 Hz orbit over an hour has no drift artefact), `bool wasOn`.
- `float targetAzimuthDeg(bool on, float rateHz, float azimuthDeg, double fs)` called **once per control step**: Off→On edge → `phase = 0`; On → `phase += rateHz·64/fs`, wrap to [0,1); returns `wrapTo180(azimuthDeg + 360·phase)` when On, `azimuthDeg` when Off. Never touches host parameters.
- `reset()` → `phase = 0, wasOn = false`.

### T5 — `Source/dsp/StereoDecoder.h` + `Source/dsp/DecoderMixer.h`
**Files:** create both. **Depends on:** `FixedDelay.h` (Stage 1).
- `StereoDecoder.h`: `inline constexpr float kStereoLevelMatch = 1.0f;` (comment: 2.3's generator emits the real value into `BinauralFilterData.h` and this header reads it from there), `inline void decode(const float* bus16, float& L, float& R)` with the A4 literals. Comment the ±60° cardioid pair, W/Y/X only, in-phase for every direction (F7: `V ≥ 0.5·(1 − cos e) ≥ 0`), 22.9 dB at +90°, L = R = 0.5 at e = ±90°.
- `DecoderMixer.h`: `DecoderDelay delayL, delayR;` `LinearRamp mix;` `void prepare()/reset()` (delays zeroed); `void controlStep(int decoder, float rampStep)`; `void processSample(float stL, float stR, float binL, float binR, float t, float& outL, float& outR)` — delays the stereo leg, lerps `mix` per sample (A3), blends per A3. 2.1 caller passes `binL = delayedL, binR = delayedR` (A14) — make the placeholder **explicit at the engine call site**, not hidden in the mixer, so 2.3's replacement is a one-line diff.

### T6 — `Source/dsp/MeterFrame.h` + `Source/dsp/TripleBuffer.h`
**Files:** create both. **Depends on:** none.
- `MeterFrame`: `float shGain[16]; float shPeak[16]; float azEffDeg, elEffDeg, azRotDeg, elRotDeg; int orderActive; float decoderMix; float outPeakL, outPeakR; int latencySamples;` — all plain, `static_assert(std::is_trivially_copyable_v<MeterFrame>)`. Doc block = the Stage-3 reader contract (R7): `shPeak`/`outPeak` are **peak-since-the-UI-last-read** hold values (JS applies decay only); `shGain` signed; `latencySamples` constant 64; `orderActive` 4/9/16 is the FUNC-03 meter-count criterion; `azRot/elRot == azEff/elEff` until 2.2.
- `TripleBuffer.h`: per A12 — copy `plugins/O-GrainScatter/Source/dsp/TripleBuffer.h` verbatim (API: `getWriteBuffer()`, `publish()`, `read()`, `hasNewData()`), add the provenance comment in the O-simpleGrain form, the `static_assert`, the one-reader comment.

### T7 — `Source/dsp/SpatialEngine.h` (composition)
**Files:** modify (replace the Stage-1 stub body; keep `kPartition`, `DecoderDelay`, `Params`, the public signature, `latencySamples()`). **Depends on:** T1–T6.
- Members: `WrapSmoother az; OnePoleSmoother el; DistanceModel dist; OrbitLfo orbit; LinearRamp orderGain[3]; DecoderMixer mixer; float prevC[16], curC[16]; float busTap[16]; int gridPos; float kOnePole, kRampStep; double fs; int maxBlock; MeterFrame frame; Params params;`. **No 16 × maxBlock bus buffer:** the chain is strictly per-sample feed-forward, so the bus for one sample lives in `busTap[16]` (also the order-fade probe's tap, T11) and the engine allocates nothing at all. `maxBlock` is kept for 2.3's convolver FIFO sizing.
- `prepare(fs, maxBlock)`: compute `kOnePole`, `kRampStep`; `dist.prepare(fs)`; `mixer.prepare()`; then **snap everything from `params`** (A7): `az.snapTo(params.azimuthDeg)`, `el.snapTo`, `dist.snapTo(d)`, `orderGain[b].snapTo(b ≤ order ? 1 : 0)`, `mixer.mix.snapTo(decoder == 0 ? 1 : 0)`, `orbit.reset()`, evaluate `curC = prevC = c_eff(az, el)`, `gridPos = 0`, `resetPeaks()`, fill the frame's state fields. Every value finite by construction (`critical_smoothedvalue_reset_does_not_clear_a_nan` analogue).
- `reset()`: delays + TPT state zeroed, `gridPos = 0`, re-snap (same as the tail of `prepare`, no allocation).
- `setParams(const Params&)`: copy (called once per host block by the processor; sampled at the next control step).
- `process(mono, outL, outR, n)`: exactly the control-step spec above. No allocation, no transcendental per sample (two trig pairs + one `tan` + one `sqrt` per control step only), no branches on `n`.
- `void resetPeaks()`; `const MeterFrame& frame() const`.
- Comment block: the D5 amendment and why (R3 numbers: −5.56 dB band-3 chord dip at 4096-sample slice-rate control vs ≤ 0.002 dB on the 64 grid; bit-identity across slicing), the identity-rotation slot for 2.2, the A14 placeholder for 2.3.

### T8 — Processor wiring + CMake
**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `CMakeLists.txt` (modify). **Depends on:** T6, T7.
- Cache the remaining 10 raw atomics in the constructor (`azimuth, elevation, distance, order, yaw, pitch, roll, decoder, orbit, orbitRate`), `jassert` each.
- `osa_dsp::Params fillParams() const noexcept` — `order = (int) load + 1` (choice index 0..2 → 1..3), `decoder = (int) load`, `orbit = load > 0.5f`, the rest direct.
- Members: `osa_dsp::SpatialEngine engine; std::vector<float> monoScratch; std::vector<float> rightScratch; float outPeakAccL = 0, outPeakAccR = 0;` and **public** `OSimpleAmbisonics::dsp::TripleBuffer<OSimpleAmbisonics::dsp::MeterFrame> meterFrames;`. Remove `delay` (the engine's `DecoderMixer` owns the delays now).
- `prepareToPlay`: `setLatencySamples(kPartition)` (unchanged, still the only call site) → `maxBlock` → `numInputLegs` → `monoScratch.assign(maxBlock, 0)`, `rightScratch.assign(maxBlock, 0)` → `engine.setParams(fillParams())` → `engine.prepare(fs, maxBlock)` → seeded `outputGain` (unchanged) → `outPeakAcc* = 0`.
- `processBlock`: `ScopedNoDenormals`; `if (! meterFrames.hasNewData()) { engine.resetPeaks(); outPeakAccL = outPeakAccR = 0; }`; `engine.setParams(fillParams())`; `outputGain.setTargetValue(…)`; slice loop: conditioner into `monoScratch[0..len)` (reads in0/in1 for the slice **before** any write), `engine.process(monoScratch.data(), out0 + start, out1 ? out1 + start : rightScratch.data(), len)`, then per sample `× outputGain.getNextValue()` on both outputs and the peak accumulation; finite scrub unchanged; extra channels cleared unchanged; then `auto& w = meterFrames.getWriteBuffer(); w = engine.frame(); w.outPeakL/R = acc; meterFrames.publish();` (once per host block, after the last slice).
- `CMakeLists.txt` `target_sources`: add `Source/dsp/{Smoothers,ShEncoder,DistanceModel,OrbitLfo,StereoDecoder,DecoderMixer,MeterFrame,TripleBuffer}.h`. No other CMake change; harness CMake unchanged (single TU + `PluginProcessor.cpp`).
- Header comment: update the Stage-1 chain description to the 2.1 chain; keep the `#if JUCE_WEB_BROWSER` note at `createEditor`.

### T9 — `tests/render-harness/ReferenceModel.h` (F9)
**Files:** create. **Depends on:** none in `Source/` (it must share **nothing** with `ShEncoder.h` but `<cmath>`).
- Double-precision transcription of the control-step spec: same 64 grid, same `kOnePole`/`kRampStep` (computed in double), same snap rule, same ramps, same A3 forms, same `t`, TPT one-pole in double, D7 mix, ±60° matrix with the same literals (as doubles `0.5, 0.25, 0.4330127`), 64-sample delay, orbit.
- **Generic SN3D table** (R4): `factorial()`, `N(n,|m|) = sqrt((2 − δ_m0)·(n−|m|)!/(n+|m|)!)`, explicit `P_n^|m|(x)` polynomials of the encoding deep-dive §2.5 (no Condon–Shortley), `T_m(a)`; `acn = n(n+1) + m`. Exposed as `refSh16(azDeg, elDeg, double out[16])` for the DSP-01 gate.
- Two switches for the negative controls: `snapCoefficientsPerGrid` (no per-sample lerp of `c`) and `snapOrderGains` (step instead of ramp). Default off.
- API: `prepare(fs)`, `setParams(const RefParams&)` (same fields as `dsp::Params`), `process(const float* mono, double* L, double* R, int n)` (any slicing — it has the same grid), `const double* currentC() const`, `double currentAzEff() const`.

### T10 — Harness infrastructure (`tests/render-harness/main.cpp`, sections before `main`)
**Files:** modify. **Depends on:** T8, T9.
- **`rtcheck`** — O-Octagon `main.cpp:111–241` verbatim (8 `operator new/delete` variants incl. both `std::align_val_t` overloads, `arm()`/`disarm()`, thread filter, `foreignNote()`), provenance comment, section header "PERF-01 allocation counter".
- **Click helpers** — `worstCurvature(const std::vector<float>& interleaved, int ch, int from, int to)` (second difference; adapt from O-Octagon `:973` to the harness's interleaved vectors), `struct ClickReading { event, control; ratio() }`, `constexpr float kClickRatioCeiling = 2.0f;` with the O-Octagon doc comment. One helper `clickVerdict(name, ClickReading, extraDetail)` that prints **both literals** and the ratio.
- **`checkFailsAsExpected(bool innerPassed, const char* line)`** — O-Strata `:168` idiom (a negative control that passes is a FAIL).
- **Drivers:** keep `renderWith` (processor, any slicing); add `renderHooked(proc, total, gen, hook)` in fixed 64-sample blocks calling `hook(n)` before the block at absolute sample `n` (O-Octagon `:990` shape, stereo input filled on both legs from `gen`); add `renderEngine(engine, params-hook, total, blockSizes)` that drives `SpatialEngine` directly under `juce::ScopedNoDenormals` (F6) and returns interleaved L/R; `renderRef(model, …)` likewise for the reference model.
- **Signals:** `Sine{f, amp}` generator (`std::sin(2π f n / fs)`), `tone100 = Sine{100, 0.5}`, two-tone `Sine{200, 0.25} + Sine{12000, 0.25}`, exact-bin DFT helper `binMagnitude(x, from, len, cycles)`.
- `kFs = 48000` stays; probes that need another rate construct their own processor/engine with that rate.

### T11 — Harness probes (`main()`)
**Files:** `tests/render-harness/main.cpp` (modify). **Depends on:** T10. Every probe uses a **fresh instance**; verdicts are ratios or exact anchors; goldens are analytic or the reference model; literals printed in the detail string.

| Probe | Gate (PASS condition) |
|---|---|
| **P0** | unchanged (12-param contract, `getLatencySamples() == 64`) |
| **P1 null** | processor at az 37°, el −12°, d 3.3 m, order 3, Stereo, orbit Off, Mono sum, 0 dB vs `ReferenceModel` on the same noise: `max |err| < 1e-5` on both channels over 8192 samples (skip nothing — the model is seeded the same way) |
| **P1 anchor** | front / 1 m / order 1 — **Binaural (default)** and **Stereo**: both channels `memcmp`-equal to `0.75f · s[n−64]` with `s = 0.5f·(L+R)`, `out[0..63] == 0`, `out[64]` exact. Also engine-direct (same anchor) |
| **P1b** | Left only → `0.75f·L[n−64]`; mono-in bus → `0.75f·in0[n−64]` (both channels) |
| **P2 / P3** | unchanged (defaults, orbit Off): ragged ≡ flat; prep-64 + 4096 feed ≡ prep-4096 ≡ flat, bit-equal |
| **P2b / P3b** (F8) | orbit On at 2 Hz, order 3, Stereo, az 45°, d 2.5 m: same ragged / oversized slicings, **bit-equal** to the flat 512 render. (Pins the D5 amendment — fails on slice-rate control) |
| **P4** | impulse at 100 → first nonzero at `100 + getLatencySamples()`; amplitude `memcmp`-equal to `0.75f` |
| **P5** | −60 dB → exact silence (unchanged) |
| **DSP-01** | 13 × 7 (az × el) grid, az ∈ {−180, −150, …, 180}, el ∈ {−90, −60, …, 90}: `encodeSH16` (float, via `ShEncoder.h`) vs `refSh16` (double): `max |err| < 1e-5` (expect ≈ 2.8e-08 — print it); `W == 1.0f` at every point; ACN 1 (Y) > 0 at az +90° (sign anchor); Appendix-B spot vectors front ACN 13/15 = −0.6124 / +0.7906, left ACN 9/11 = −0.7906 / −0.6124 within 1e-4 |
| **DSP-04** | `StereoDecoder::decode` on `encodeSH16` vectors: front `L == R` (bit); +90° `20·log10(L/R) ≥ 20` and `L > R` (expect 22.9 — print); e = ±90° `L == R == 0.5f`; 72 directions (az step 10°, el ∈ {−60, 0, 60}): `L ≥ 0 && R ≥ 0`. Processor in Stereo mode, 100 Hz tone, az +90°: `rms(L)/rms(R) ≥ 20 dB`; correlation of L and R with the delayed source ≥ 0 over the direction set |
| **DSP-05** | engine-direct, two-tone, 1 s static window after a 0.1 s settle, 25 log-spaced d ∈ [0.2, 20] + d = 1.0: level (200 Hz bin, dB) strictly decreasing; HF/LF (12 kHz bin − 200 Hz bin) **exactly 0 dB for every d ≤ 1** (bit: `m == 0` → `y == x`, assert the 12 kHz bin equals the d = 1 render's bin bit-wise) and strictly decreasing on (1, 20]; anchors within 0.1 dB: ratio −10.788 dB at 20 m; level +8.87 / 0.00 / −24.19 dB at 0.2 / 1 / 20 m. Liveness: 20 m ratio < −5 dB (a dead distance path reads a flat 0) |
| **DSP-05 clamp** | `DistanceModel::clampedCutoffHz()` == `0.45·44100` exactly at d = 1.0 / fs 44100, == `20000` at fs 48000; engine at **fs 22050**, d = 1.5 (clamp engaged in the audible path, m = 0.5): all finite, HF/LF (200 Hz + 4 kHz tones) < 0 dB. Gates the clamp target, not a downstream guard |
| **DSP-06a trajectory** | engine-direct, settle at +170°, set −170°, log `frame().azEffDeg` per control step for 0.25 s: every logged value satisfies `|wrapTo180(az − 180)| ≤ 10 + 1e-3`, none satisfies `|az| < 90`; converged within 1e-3 of −170 by 0.2 s (snap). Also: Off→On orbit resets to the puck (first frame after the edge `azEff` within one step of `azimuth`); On→Off from az_eff = +170 with `azimuth` = −170 glides through 180 (same assertion) |
| **DSP-06b sweep click** | 100 Hz / 0.5 tone, Stereo, order 3: (i) **hook-driven** at 64-sample blocks: azimuth parameter stepped +0.48°/block through ±180 over 1 s (`renderHooked`); (ii) **orbit-driven** 1 Hz at 64 and at 4096 host blocks — the two orbit renders must be `memcmp`-equal. Event window: skip 64 + 0.02·fs, then the full circle. Control: same tone through the processor parked at +60° (for L) / −60° (for R), the channel with the larger legitimate curvature. `ratio ≤ 2.0` for (i) and (ii), both channels; literals printed. **Negative control:** `ReferenceModel` with `snapCoefficientsPerGrid = true` over the same orbit → `checkFailsAsExpected(ratio ≤ 2.0)` (expect ≥ 24). **Liveness:** L energy over the second half-circle vs the first differs by ≥ 6 dB. Informational: energy outside 100 Hz ± 2 bins ≤ −60 dB; hook-driven at 1024-sample blocks printed, no verdict (a 7.7° parameter step per host block is automation granularity under the τ = 20 ms contract, not a zipper) |
| **Order fade** | engine-direct, 100 Hz tone, order 1 → 3 at 0.5 s → 1 at 1.0 s. **The stereo output cannot see bands 2–3, so a click gate on L/R would be vacuous** — the gate reads the 16-channel bus through a **per-sample bus tap**: `SpatialEngine` keeps `float busTap[16]` (the bus of the last processed sample — it is computed anyway for the peak scan; zero extra cost), and `renderEngine` records `busTap[4]` and `busTap[9]` per sample when asked. The `ReferenceModel` exposes the same tap. (a) exact arrival: `frame().shGain[4]/Y_4` and `shGain[9]/Y_9` reach **exactly** 1.0f within 16 control steps of the first switch and exactly 0.0f within 16 steps of the second, monotonic in between; (b) click: event = engine tap, control = reference-model tap (same ramps, `snapOrderGains = false`), `ratio ≤ 2.0` on ACN 4 and ACN 9 across both switches; negative control `snapOrderGains = true` → `checkFailsAsExpected`; (c) after the fade back to order 1, a fresh peak window has `shPeak[4..15] == 0` exactly |
| **PERF-01** | processor prepared at 64; `AudioBuffer(2, 4096)` + `midi.ensureSize(256)` built **before** `arm()`; one unarmed warm-up block; `arm()` → 200 blocks at 64 covering: order 1 → 3 → 2 (fades armed), decoder Binaural → Stereo → Binaural (fade armed), orbit On at 2 Hz, d 0.2 → 20, one 4096-sample block (slice loop) → `disarm()`; `allocations == 0`, print `foreignNote()`. **Liveness:** separate armed window with one `new int`/`delete` → counter == 1 exactly |
| **Meter frame** | processor: order 1/2/3 → `read().orderActive` == 4/9/16 and `shGain[(order+1)²..15] == 0` after 20 ms; orbit On 2 Hz → `azEffDeg` across 10 reads over 0.5 s advances monotonically modulo 360 and spans ≥ 300°; peaks: 8 blocks without a `read()` then one `read()` → `shPeak[0]` == max over the 8 blocks (accumulation), then after the read the next published frame's `shPeak[0]` == that block's peak only (reset); `latencySamples == 64`; `azRotDeg == azEffDeg` |
| **Extremes (QUAL-01 partial)** | processor at fs 48000 and 96000: 16 corner settings (az ±180, el ±90, d 0.2 / 20, order 3, orbit On 2 Hz, decoder both, outputLevel −60 / 0, inputMode both) × 4096 samples of noise → every output sample finite; no `clear()` triggered (assert output not all-zero when outputLevel = 0) |
| **Digest** | FNV-1a 64 of the P1-null render (new 2.1 baseline, informational) |

Probe order in `main()`: P0, P1, P1b, P2, P2b, P3, P3b, P4, P5, DSP-01, DSP-04, DSP-05 (+ clamp), DSP-06a, DSP-06b, order fade, meter frame, extremes, PERF-01, digest. The header comment of `main.cpp` is rewritten to list the 2.1 probe set.

### T12 — Build, harness, install, validate
**Depends on:** T1–T11. Long commands in the background (600 s watchdog).
1. `cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON` only if the harness target is not already configured (it is — `build/CMakeCache.txt` has `OUARICON_BUILD_TESTS=ON`); otherwise plain `ninja`.
2. `ninja -C build O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test` — **0 warnings** under the recommended warning flags (watch `-Wshadow` on `fs`/`g`, `-Wconversion` on the `(float)(j+1)` forms, unused `maxBlock` if no bus scratch).
3. Run the harness; **ALL PASS**; record every printed literal (DSP-01 max err, DSP-04 dB, DSP-05 anchors, click ratios + the negative-control ratios, alloc count + foreignNote, digest).
4. `./scripts/build-and-install.sh O-simpleAmbisonics` (AU cache clear + dual-variant sweep).
5. `auval -v aufx OSAm OuDv` — never `auval -a`.
6. pluginval strictness 10, `--timeout-ms 600000`, **twice each** on the installed `O-simpleAmbisonics-dev.vst3` and `.component`.
7. Standalone quick open is optional; the DAW hands-on is the verify phase's human item.

### T13 — SUMMARY-2.1, STATUS, commit
**Files:** `.planning/stages/2-dsp/SUMMARY-2.1.md` (create), `.planning/STATUS.md` (modify). **Depends on:** T12. `PLUGINS.md` untouched this phase.
- SUMMARY-2.1: files created/modified; the control-step spec as implemented (name the D5 amendment); probe table with every literal; digest; pluginval/auval results; **allocation-gate coverage statement** (R2 wording: covered = every new-expression on the audio thread; not covered = HeapBlock/malloc paths and `AsyncUpdater` post — grep + inspection; nothing in 2.1 scope lives in the uncovered set); **deferred list:** D6 placeholder → 2.3 obligation (verify 2.3 asserts it is gone), `kStereoLevelMatch` real value → 2.3, TripleBuffer → `core/triple-buffer` module candidate (3 consumers), `ShRotator` slot → 2.2, −180/180 display choice → Stage 3; the A15 number for lesson text; any deviation from this plan with its reason.
- STATUS.md: Phase 2.1 table `execute ✓`, `current_phase: execute`, `next_phase: verify`, `next_action: plugin-verify`, `status: stage_2_phase_2_1_execute_complete`, progress bar, Completed-So-Far entry, Files Created additions.
- Commit **temp-index, path-scoped** (`index_git_shared_checkout`): `git branch --show-current` + `git status --short` immediately before; `GIT_INDEX_FILE=$(mktemp -d)/index; git read-tree HEAD; git update-index --add -- <each file explicitly>; git write-tree; git commit-tree; git update-ref HEAD`; re-check `git log -1` after. Message: `phase: O-simpleAmbisonics 2-dsp/2.1 execute complete (…)`.

## Dependency graph

```
T1 Smoothers ─┬─→ T3 DistanceModel ─┐
              └─→ T4 OrbitLfo ──────┤
T2 ShEncoder ───────────────────────┼─→ T7 SpatialEngine ─→ T8 Processor+CMake ─┐
T5 StereoDecoder+DecoderMixer ──────┤                                            ├─→ T10 harness infra ─→ T11 probes ─→ T12 build/validate ─→ T13
T6 MeterFrame+TripleBuffer ─────────┘                      T9 ReferenceModel ────┘
```
T1, T2, T5, T6, T9 are parallel-authorable. T9 must be written from the spec and the deep-dive, **not** by reading T2/T7 (independence is the point of the null).

## Files

| Action | Path |
|---|---|
| create | `Source/dsp/Smoothers.h` |
| create | `Source/dsp/ShEncoder.h` |
| create | `Source/dsp/DistanceModel.h` |
| create | `Source/dsp/OrbitLfo.h` |
| create | `Source/dsp/StereoDecoder.h` |
| create | `Source/dsp/DecoderMixer.h` |
| create | `Source/dsp/MeterFrame.h` |
| create | `Source/dsp/TripleBuffer.h` |
| modify | `Source/dsp/SpatialEngine.h` |
| modify | `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` |
| modify | `CMakeLists.txt` (target_sources only) |
| create | `tests/render-harness/ReferenceModel.h` |
| modify | `tests/render-harness/main.cpp` |
| create | `.planning/stages/2-dsp/SUMMARY-2.1.md` |
| modify | `.planning/STATUS.md` |

Not touched: `Source/dsp/FixedDelay.h`, `tests/render-harness/CMakeLists.txt`, `PLUGINS.md`, `setLatencySamples`, any `ShRotator`/`BinauralConvolver`/`BinauralFilterData` (2.2 / 2.3), any editor file (Stage 3).

## Gotchas checklist (executor ticks each)

- [ ] Control grid persists across `process` calls; `k` and the ramp step come from **64**, never from the slice length (A1). P2b/P3b are the proof.
- [ ] `setParams` before `engine.prepare` in `prepareToPlay`; every smoother/ramp snapped in `prepare` (A7). P1 `out[64]` exact and P5 prove it.
- [ ] Order gains and decoder mix are **ramps** with exact arrival; one-poles snap at 1e-6 (A2). Order-fade (a) proves it.
- [ ] All blends `b + t·(a − b)` (A3); decoder literals `0.5f / 0.25f / 0.4330127f` (A4). P1 anchor `memcmp` proves it — a 1-ulp miss means a wrong literal or form, not a tolerance problem.
- [ ] `encodeSH16` takes radians; convert once per control step.
- [ ] `wrapTo180` is [−180, 180) (A6).
- [ ] Air LP cutoff clamped to 0.45·fs **before** `tan`; `clampedCutoffHz()` exposed and gated.
- [ ] No allocation in `process`/`processBlock`: `std::vector` sized in `prepare` only; no `resize` anywhere else; `TripleBuffer` is a `std::array<T,3>`.
- [ ] Engine-direct harness renders wrapped in `juce::ScopedNoDenormals` (F6).
- [ ] Reference model shares nothing with `ShEncoder.h`; its table is factorials + §2.5 polynomials (R4).
- [ ] Click gates: 100 Hz tone, ratio vs matched control, literals printed, negative control via `checkFailsAsExpected`, liveness half-circle ≥ 6 dB (A10). The order-fade click gate is on the **bus tap**, not the stereo output (vacuous there).
- [ ] Alloc counter: buffers pre-sized, warm-up unarmed, parameter changes between `processBlock` calls on the harness thread, liveness `new` == 1 (A11).
- [ ] `meterFrames` public; `hasNewData()`-gated reset at the top of `processBlock`; one `publish()` per host block (A8).
- [ ] JUCE-free `Source/dsp/` (A13); `osa_dsp` alias in the processor TU (bare `dsp::` is ambiguous).
- [ ] `build-and-install.sh` then targeted `auval -v aufx OSAm OuDv`; pluginval s10 ×2 VST3 + AU.
- [ ] Temp-index, path-scoped commit; re-check branch + status immediately before; `PLUGINS.md` untouched.

## Success criteria (verify phase checks these)

- [ ] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics-render-test` builds with 0 warnings.
- [ ] P0 / P2 / P3 / P4 / P5 still PASS; P4 latency 64 with impulse amplitude exactly 0.75; P0 `getLatencySamples() == 64`.
- [ ] P1 null < 1e-5 on a fresh instance; front / 1 m / order 1 anchor `memcmp`-equal to `0.75f·s[n−64]` in **both** decoder modes; P1b with the 0.75 factor.
- [ ] P2b / P3b bit-equal with orbit On at 2 Hz and order 3 (D5 amendment pinned).
- [ ] DSP-01 (< 1e-5, W ≡ 1, sign anchor), DSP-04 (≥ 20 dB, in-phase, L = R front), DSP-05 (A9 wording + clamp target), DSP-06a (through 180°, never near 0°), DSP-06b (ratio ≤ 2.0 at 64-block hook and at 64/4096 orbit, orbit renders bit-equal, negative control fails, liveness ≥ 6 dB), order fade (exact 0/1 arrival, bus-tap ratio ≤ 2.0, negative control fails), PERF-01 (0 allocations, liveness 1), meter frame (4/9/16, `azEff` follows the orbit, peak accumulate-then-reset), extremes finite at 48 k and 96 k — all PASS with printed literals.
- [ ] pluginval strictness 10 ×2 passes for the installed VST3 and AU; `auval -v aufx OSAm OuDv` passes.
- [ ] Installed plugin hands-on in a DAW (verify's human item): Stereo mode pans with Azimuth, +90° louder **left**; orders change nothing audible in Stereo mode but the meter frame shows 4 / 9 / 16; Distance 1 → 20 m darkens and attenuates; Orbit On moves, Off glides back the short way; default settings (Binaural) are **not silent**.
- [ ] `SUMMARY-2.1.md` records the digest, every gate literal, the allocation-coverage statement, the D6 placeholder as a 2.3 obligation and the deferred list.
