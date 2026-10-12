# Stage 2 — DSP · Phase 2.1 (Source → Encoder → Stereo decode) — SUMMARY

**Plugin:** O-simpleAmbisonics · **Stage:** 2 DSP · **Phase:** 2.1 of 3 · **GSD phase:** execute · **Date:** 2026-10-12
**Inputs:** `PLAN-2.1.md` (T1–T13, A1–A15, the shared control-step spec), `CONTEXT-2.1.md` (D1–D9), `RESEARCH-2.1.md` (R1–R7, F1–F9), `research/ARCHITECTURE.md`, `ROADMAP.md` §Phase 2.1, Stage-1 source + harness
**Executor:** dsp-agent role (with Bash for T12)
**Branch:** `main` (trunk-based; `plugins/O-simpleAmbisonics/**` only; `PLUGINS.md` untouched)

## Result

The Stage-1 pass-through is replaced by the **audible ambisonic chain minus rotation and HRTF**, exactly as PLAN-2.1 specifies: conditioned mono → distance model (soft-knee gain + TPT air LP with the D7 wet mix) → SN3D/ACN encoder with linearly-ramped order gating → identity rotation slot → 16-channel bus with the meter tap → ±60° virtual-cardioid stereo decode → `FixedDelay<64>` → decoder mixer (binaural leg = A14 placeholder) → seeded output gain. Every control quantity moves on the **persistent 64-sample control grid** (the D5 amendment, A1), so the render is bit-identical across any host slicing with parameters moving and orbit On (P2b / P3b). Latency is exactly `kPartition = 64`; `setLatencySamples` has its single Stage-1 call site, untouched.

Harness: **118 checks, ALL PASS**, reproduced on four runs of a fresh binary (digest `9189d911a1fe98d5`). Build: **0 warnings** on all four targets. Installed bundles: `build-and-install.sh` OK, `auval -v aufx OSAm OuDv` **SUCCEEDED**, pluginval strictness 10 **SUCCESS ×2 VST3, ×2 AU**.

One real defect was found and fixed inside the test-and-fix loop (§Incidents): the APVTS raw **default** of the skewed, stepped `distance` range is *not snapped* (a documented JUCE legacy), so the processor handed the engine 1.0000001 m and the whole output was 1 ulp low until a host touched the parameter.

Covers (gated below): FUNC-01 (partial), FUNC-02, FUNC-03 (encoder side), FUNC-06, DSP-01, DSP-04, DSP-05, DSP-06, PERF-01, QUAL-01 (partial).

## Files

| Action | Path | Content |
|---|---|---|
| create | `Source/dsp/Smoothers.h` | `kControlGrid = 64`, `kSmoothingTauSeconds`, `wrapTo180` ([−180, 180), A6), `OnePoleSmoother` / `WrapSmoother` (A2 snap at 1e-6·max(1, \|t\|)), `LinearRamp` (exact arrival). τ = 20 ms contract and "k / step from the grid, never the slice" documented |
| create | `Source/dsp/ShEncoder.h` | `kSqrt3_2 / kSqrt15_2 / kSqrt5_8 / kSqrt3_8`, `AmbiTrigCache`, `encodeSH16` lifted **verbatim** from O-GrainScatter `SpatialEncoder.h:33–103` with provenance (R4: 4.7e-16 double / 2.8e-08 float); `kNumSh`, `kDegToRad` (the one literal), `shBand`, `applyOrderGains`. `SmoothedSHCoeffs` not lifted |
| create | `Source/dsp/DistanceModel.h` | `gainFor`, `cutoffFor` (clamp to 0.45·fs **before** tan), `tptCoefficient`, `mixFor`; one-pole smoothed `g / G / m` at the control step, per-sample A3 lerp, TPT one-pole (D4), D7 wet mix; `snapTo`, `controlStep`, `processSample`, `reset`, `clampedCutoffHz()` (the clamp **target**). A15 number (−0.06 dB at 10 kHz) in the header |
| create | `Source/dsp/OrbitLfo.h` | double phase, `wasOn`; Off→On resets phase; advances `rate·64/fs` per control step; `wrapTo180(az + 360·φ)`; never writes host parameters |
| create | `Source/dsp/StereoDecoder.h` | `kStereoLevelMatch = 1.0f` placeholder (2.3 generates the real value); `decode` with the exact literals `0.5f / 0.25f / 0.4330127f` (A4); F7 properties documented |
| create | `Source/dsp/DecoderMixer.h` | `template <int N> DecoderMixer` — `FixedDelay<N> ×2`, `LinearRamp mix`; `snapTo`, `controlStep`, `delayStereo`, `blend` (A3 form `b + t·(a − b)`), `processSample`, `currentMix` |
| create | `Source/dsp/MeterFrame.h` | the Stage-3 reader contract (R7): `shGain[16]` signed, `shPeak[16]` peak-since-the-UI-last-read, `azEff/elEff`, `azRot/elRot` (== eff until 2.2), `orderActive` 4/9/16, `decoderMix`, `outPeakL/R`, `latencySamples` 64; `static_assert(is_trivially_copyable)` |
| create | `Source/dsp/TripleBuffer.h` | verbatim body of O-GrainScatter's, O-simpleGrain-form provenance comment, `static_assert(is_trivially_copyable_v<T>)`, one-reader comment; placed in `OSimpleAmbisonics::dsp` |
| modify | `Source/dsp/SpatialEngine.h` | the composition (T7): `kPartition`, `DecoderDelay`, `Params`, public signature and `latencySamples()` kept; `static_assert(kControlGrid == kPartition)`; `prepare` computes `kOnePole`/`kRampStep` and snaps everything from `Params` (A7); `reset`; `process` = the control-step spec; `resetPeaks()`, `frame()`, `busTap()`, `maxBlockSize()`. **No 16 × maxBlock bus buffer; the engine allocates nothing.** D5-amendment rationale with the R3 numbers, the 2.2 rotation slot and the A14 placeholder are in the header and at the call site |
| modify | `Source/PluginProcessor.h` | all 12 cached atomics; `distanceRange` / `orbitRateRange` pointers; `fillParams()`; `engine`, `outputGain`, `monoScratch`, `rightScratch`, `outPeakAccL/R`; **public** `meterFrames` (`TripleBuffer<MeterFrame>`); `delay` removed |
| modify | `Source/PluginProcessor.cpp` | header comment = the 2.1 chain; `fillParams()` (order = index + 1, decoder = index, orbit > 0.5, **distance / orbitRate snapped through their ranges**); `prepareToPlay`: `setLatencySamples(kPartition)` (unchanged, the one call site) → scratch `assign` → `engine.setParams` → `engine.prepare` → seeded gain → peak accumulators; `processBlock`: `ScopedNoDenormals`, `hasNewData()`-gated peak reset, `setParams` once, slice loop (conditioner → `monoScratch` for the whole slice → `engine.process` → per-sample `outputGain.getNextValue()` + peak accumulation), finite scrub unchanged, extra channels cleared, one `publish()` |
| modify | `CMakeLists.txt` | `target_sources` lists the eight new headers; nothing else |
| create | `tests/render-harness/ReferenceModel.h` | F9 / T9: double transcription of the control-step spec; generic SN3D table (`factorial`, §2.5 Legendre polynomials, `T_m`, ACN = n(n+1)+m); switches `snapCoefficientsPerGrid`, `snapOrderGains`; `refSh16`, `currentC`, `currentAzEff`, `busTap` |
| modify | `tests/render-harness/main.cpp` | header = the 2.1 probe set; `rtcheck` (O-Octagon `:111–241` verbatim with provenance and the coverage statement); `checkFailsAsExpected`; `fmt`; `makeProcAt`; `refParamsFrom`; `Sine`, `tone100`, `twoTone`; `renderWith` (+ total), `renderHooked` (fixed blocks, hook before each), `renderEngine` / `renderRef` (ScopedNoDenormals, optional per-sample bus taps); `worstCurvature`, `ClickReading`, `kClickRatioCeiling = 2.0`, `clickVerdict`; `binMagnitude`, `energy`, `rms`; all T11 probes |
| create | `.planning/stages/2-dsp/SUMMARY-2.1.md` | this file |
| modify | `.planning/STATUS.md` | Phase 2.1 execute ✓, frontmatter, progress, Current Position, Completed So Far, Next Steps, Files Created |

Not touched (per plan): `Source/dsp/FixedDelay.h`, `tests/render-harness/CMakeLists.txt`, `PLUGINS.md`, `setLatencySamples`, any `ShRotator` / `BinauralConvolver` / `BinauralFilterData` file, any editor file.

## The control step, as implemented (the D5 amendment, A1)

`SpatialEngine::process` and `ReferenceModel::process` carry the **same loop, op for op**:

```
process(mono, outL, outR, n):
  i = 0
  while i < n:
    if gridPos == 0: controlStep()
    run = min(n − i, kControlGrid − gridPos)
    for j in [0, run): renderSample(mono[i+j], t = (float)(gridPos + j + 1) · (1/64))
    i += run; gridPos += run; if gridPos == 64: gridPos = 0            // persists across calls

controlStep():
  azTarget = orbit.targetAzimuthDeg(on, rate, azimuthDeg, fs)         // Off→On → φ = 0; On → φ += rate·64/fs
  az.step(azTarget, kOnePole); el.step(elevationDeg, kOnePole)        // one-poles, A2 snap
  dist.controlStep(d, kOnePole)                                        // g, G (coefficient), m — prev ← cur, step
  orderGain[b].step(b+1 ≤ order ? 1 : 0, kRampStep), b = 0..2         // LinearRamps, exact arrival
  mixer.controlStep(decoder, kRampStep)                                // LinearRamp, prev ← cur
  prevC ← curC; curC = encodeSH16(az·kDegToRad, el·kDegToRad) · orderGain[band]
  frame: shGain = curC, azEff/elEff = az/el, azRot/elRot = az/el, orderActive = (order+1)², decoderMix, latency 64

renderSample(x, t):
  gS/GS/mS = prev + t·(cur − prev);  u = x·gS;  v = (u − z)·GS;  lp = v + z;  z = lp + v;  y = u + mS·(lp − u)
  bus[acn] = y·(prevC + t·(curC − prevC));  shPeak[acn] = max(shPeak, |bus|)      // busTap[16]
  (identity rotation slot)
  stL = 0.5f·bus[0] + 0.25f·bus[3] + 0.4330127f·bus[1];  stR = … − 0.4330127f·bus[1]
  dL = delayL(stL); dR = delayR(stR);  binL = dL; binR = dR  (A14, at the engine call site)
  outL = dL + mix·(binL − dL);  outR = dR + mix·(binR − dR)
```

Constants from the grid, computed once in `prepare`: `kOnePole = 1 − exp(−64/(0.02·fs))` = 0.0645 at 48 kHz, `kRampStep = 64/(0.02·fs)` = 1/15. The processor applies `outputLevel` per sample after the engine (`LinearSmoothedValue`, seeded in `prepareToPlay`).

Why the grid and not the slice (R3): a 2 Hz orbit at 4096-sample slices moves 61° per call and the lerp chord dips band 3 by −5.56 dB mid-block; on the 64 grid the dip is ≤ 0.002 dB and the render is slicing-invariant with parameters moving. P2b / P3b are the gate.

## Harness — the probe table (every literal, run 2 of 4 identical runs)

Binary: `build/plugins/O-simpleAmbisonics/tests/render-harness/O-simpleAmbisonics-render-test_artefacts/Release/O-simpleAmbisonics-render-test` (≈ 0.5 s).

| Probe | Gate | Result |
|---|---|---|
| P0 | 12-param contract; `getLatencySamples() == 64` | **PASS** (45 checks) |
| P1 null | processor (az 37°, el −12°, d 3.3 m, order 3, Stereo, Mono sum, 0 dB) vs `ReferenceModel`, 8192 samples × 2 ch, fresh instance | **PASS** — max \|err\| **4.619e-08** (gate 1e-5), peak \|L\| 0.286 |
| P1 anchor | front / 1 m / order 1: `out == 0.75f·s[n−64]` `memcmp` both ch; `out[0..63] == 0`; `out[64]` exact | **PASS** Binaural (default), **PASS** Stereo, **PASS** Stereo render ≡ Binaural render (A14 exact in both modes), **PASS** engine-direct; `out[64]` = 0.2463251 exact |
| P1b | Left only → `0.75f·L[n−64]`; mono-in bus → `0.75f·in0[n−64]` | **PASS** / **PASS** |
| P2 / P3 | defaults: ragged ≡ flat; prep-64 + 4096 ≡ prep-4096 ≡ flat | **PASS** ×3 |
| P2b / P3b | orbit On 2 Hz, order 3, Stereo, az 45°, d 2.5 m: ragged ≡ flat; prep-64 + 4096 ≡ prep-4096 ≡ flat (bit) | **PASS** ×3; liveness: orbit changes L energy between halves by −2.69 dB |
| P4 | impulse at 100 → first nonzero at 100 + 64; amplitude exactly 0.75f | **PASS** — first nonzero **164**, reported 64; amplitude **0.7500000** bit-equal |
| P5 | −60 dB → exact silence | **PASS** |
| DSP-01 | `encodeSH16` (float, deg·`kDegToRad`) vs `refSh16` (double) on 13 × 7; W ≡ 1; sign anchor; Appendix-B spots | **PASS** — max \|err\| **2.073e-07** (gate 1e-5); W == 1.0f everywhere; Y(+90°) = 1.000000 > 0; front ACN 13/15 = −0.6124 / 0.7906; left ACN 9/11 = −0.7906 / −0.6124 |
| DSP-04 | front L == R (bit); +90° ≥ 20 dB and L > R; e = ±90° L == R == 0.5f (bit, az 0 / 180); 72 directions non-negative; processor Stereo at +90° rms ≥ 20 dB; normalised correlation with the delayed source ≥ −1e-6 over 36 directions | **PASS** ×6 — front 0.7500000 / 0.7500000; +90°: L 0.9330 R 0.0670 = **22.88 dB**; poles 0.50000000 ×8; min pattern value 0.000000; processor **22.88 dB**; min normalised correlation −3.03e-09 (analytic minimum 0 at the ∓120° null) |
| DSP-05 | 25 log-spaced d + 1 m, two-tone 200 Hz + 12 kHz, 1 s window after 0.1 s settle, engine-direct: level strictly decreasing; d ≤ 1 m bit-exact `0.75f·(g·x[n−64])`; HF/LF strictly decreasing on (1, 20]; anchors ±0.1 dB; liveness | **PASS** ×8 — level **+8.874 / 0.0000 / −24.198 dB** at 0.2 / 1 / 20 m; HF/LF **−10.788 dB** at 20 m; d ≤ 1: 10 distances bit-exact, measured \|HF/LF\| ≤ 1.12e-07 dB; table (d: level / HF−LF dB): 0.2: 8.87/0.00 · 0.632: 3.02/0.00 · 1: 0.00/−0.00 · 1.12: −0.83/−0.11 · 2: −5.11/−1.95 · 4.31: −11.24/−4.59 · 9.28: −17.65/−7.60 · 20: −24.20/−10.79 (26 rows printed by the harness) |
| DSP-05 clamp | `cutoffFor(1, 44100) == 19845` exactly; `cutoffFor(1, 48000) == 20000`; `clampedCutoffHz()` == 19845 at d 1.0 and 0.2 / fs 44100; engine at fs 22050, d 1.5 (clamp engaged, m = 0.5): finite, HF/LF < 0 | **PASS** ×4 — 19845.0 / 20000.0 / 19845.0 / 19845.0 Hz; clamp target 9922.5 Hz, HF/LF (4 kHz − 200 Hz) **−0.033 dB** |
| DSP-06a | +170° → −170°, 188 control steps: every step within 10° + 1e-3 of ±180, never \|az\| < 90, within 1e-3 of −170 from 0.2 s; Off→On departs from the puck; On→Off from +170 glides through 180 | **PASS** ×3 — max \|wrap(az − 180)\| 10.000°, residual at 0.2 s **8.39e-04**, final −170.00000 (snapped); after 0.15 s On: 124.56°, after 0.5 s Off: 30.00000, first frame after re-On: 30.0619; On→Off started at 170.315° (step 369), same trajectory numbers |
| DSP-06b (i) | hook-driven 64-sample blocks, +0.48°/block, 100 Hz / 0.5, Stereo, order 3; control = parked +60° (L) / −60° (R) | **PASS** — L event 8.5711e-05, control 8.5682e-05, **ratio 1.0003**; R 8.5682e-05 / 8.5682e-05, **ratio 1.0000**; liveness L energy half-circles differ by **8.35 dB**; info: energy outside 100 Hz ± 2 bins −26.5 dB re in-band; info: 1024-sample blocks (7.68°/block) ratio 1.0650 / 1.0744 — no verdict (automation granularity) |
| DSP-06b (ii) | orbit 1 Hz at 64 and 4096 host blocks: bit-equal; both ratios ≤ 2.0; liveness | **PASS** ×6 — 64 ≡ 4096 (bit); ratios L 1.0003 / R 1.0000 at both block sizes; liveness **8.14 dB** |
| DSP-06b negative control | `ReferenceModel` with `snapCoefficientsPerGrid = true` over the same orbit vs the model parked | **PASS (FAIL-as-expected)** — L ratio **24.84**, R ratio **24.81** (R1 predicted ≥ 24); the lerped model reads 1.0000 / 1.0000 |
| Order fade (a) | engine-direct, az 30°, 100 Hz, order 1 → 3 at 0.5 s → 1 at 1.0 s: `shGain[4]` / `[9]` reach **exactly** Y_4 / Y_9 within 16 steps, exactly 0.0f within 16 steps, monotonic; 0 before the switch | **PASS** ×2 — Y_4 0.750000, Y_9 0.790569; exact arrival **14 steps** after 1→3, exactly 0 **15 steps** after 3→1, monotonic yes/yes |
| Order fade (b) | per-sample **bus tap** ACN 4 / ACN 9: engine vs the model's tap (same ramps) ≤ 2.0; negative control `snapOrderGains = true` | **PASS** — ACN 4 event 6.4254e-05 / control 6.4261e-05, **ratio 0.9999**; ACN 9 6.7741e-05 / 6.7737e-05, **ratio 1.0000**; negative control **66.9 / 66.9** FAIL-as-expected; liveness: ACN 4 energy 1549.3 while order 3, exactly 0 otherwise |
| Order fade (c) | after the fade back, `resetPeaks()` + one block: `shPeak[4..15] == 0` exactly | **PASS** — max 0.000e+00, `shPeak[0]` 0.3672 |
| Meter frame | order 1/2/3 set *after* prepare from order 3 → `orderActive` 4/9/16 and `shGain[active..15] == 0` after 32 ms; `latencySamples == 64`; `azRot == azEff`; orbit 2 Hz: 10 reads over 0.5 s advance monotonically mod 360, span ≥ 300°; peaks accumulate across 8 un-read blocks then reset after a read (`shPeak[0]` and `outPeakL` bit-equal to the maxima) | **PASS** ×7 — 4 / 9 / 16 with max \|shGain\| above = 0; reads 23.7 59.1 95.5 132.0 168.5 −155.0 −118.6 −82.1 −45.6 −9.1 (span **327.2°**); accumulate 0.926415 / outPeakL 0.593303; reset 0.080262 |
| Extremes | fs 48 k and 96 k × 16 corners (az ±180, el ±90, d 0.2 / 20, decoder both, order 3, orbit 2 Hz, outputLevel −60 / 0, inputMode both) × 4096 noise samples: finite; not all-zero when unmuted | **PASS** ×2 — all 16 ok at both rates |
| PERF-01 | processor prepared at 64; buffers + `midi.ensureSize(256)` before `arm()`; unarmed warm-up; 200 armed blocks with order 1→3→2, decoder B→S→B, orbit On 2 Hz, d 0.2→20, one 4096-sample block | **PASS** — **0 allocation(s)**, `foreignNote()` empty (no foreign-thread allocations); liveness: one deliberate `new` counts exactly **1** |
| Digest | FNV-1a 64 of the P1-null render (informational, new 2.1 baseline) | **`9189d911a1fe98d5`** (8192 × 2 ch, flat 512, az 37 el −12 d 3.3 order 3 Stereo) |

All 118 checks PASS; four consecutive runs identical (digest reproduced each time).

## Build / install / validation (T12)

| Step | Result |
|---|---|
| `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test` | EXIT 0, **0 warnings** under `juce_recommended_warning_flags` (incl. `-Wshadow-all`, `-Wconversion`, `-Wsign-conversion`, `-Wfloat-equal`, `-Wmissing-prototypes`) — harness compiled clean on the first attempt |
| `./scripts/build-and-install.sh O-simpleAmbisonics` | OK — old `-dev` bundles removed, no alternate-variant orphan reported, `O-simpleAmbisonics-dev.{vst3,component}` installed, DAW caches cleared |
| `auval -v aufx OSAm OuDv` | **AU VALIDATION SUCCEEDED**; Latency / Tail Time / Bypass verified; channel caps `[1, 2] [2, 2]` |
| pluginval strictness 10, `--timeout-ms 600000`, installed VST3 ×2 | **SUCCESS ×2**, 0 failures |
| pluginval strictness 10, installed AU ×2 | **SUCCESS ×2**, 0 failures |

pluginval's "Reported latency: 0" is its pre-`prepareToPlay` query, suite-consistent (Stage-1 SUMMARY).

## Allocation-gate coverage statement (R2 wording)

Covered = every new-expression on the audio thread: the harness replaces all eight global `operator new` / `delete` variants (both `std::align_val_t` overloads included) and counts on the arming thread only, reporting foreign-thread allocations beside the verdict. **Not covered** = `HeapBlock` / `malloc` paths (`AudioBuffer`, `Array`, `MidiBuffer`, `MemoryBlock`) and the `AsyncUpdater` post path — grep + inspection. Inspection of the 2.1 scope: the engine is JUCE-free and owns no dynamic storage at all (`busTap[16]`, `std::array` delays, a `std::array<T,3>` triple buffer); the processor's only dynamic storage is two `std::vector<float>` `assign`ed in `prepareToPlay` (which *is* `operator new` and would be counted if it resized in `processBlock`); `processBlock` touches the host-owned `buffer`, the raw atomics, `NormalisableRange::snapToLegalValue` (pure arithmetic) and `TripleBuffer` (atomics). Nothing in 2.1 scope lives in the uncovered set.

## Incidents in the test-and-fix loop

1. **Processor / parameter defect (fixed in source).** First run: P1 anchor, P1b, P4 and the meter-peak probes failed by exactly 1 ulp through the processor while the engine-direct anchor was bit-exact; P4 read `0.7499999`. Cause (read in JUCE 8.0.15 `juce_AudioProcessorValueTreeState.cpp`, `ParameterAdapter` ctor: *"For legacy reasons, the unnormalised value should \*not\* be snapped on construction"*): the raw default is `NormalisableRange::convertFrom0to1(defaultNormalised)` — the skew round-trip `exp(log(p)/skew)` — **without** `snapToLegalValue`, so `distance` read **1.00000012** and `g(d)` = 0.99999988. Every host-set value goes through `RangedAudioParameter::convertFrom0to1`, which snaps; only the construction default is unsnapped. Fix: `fillParams()` snaps `distance` and `orbitRate` (the two stepped, skewed ranges) through the parameters' own `NormalisableRange` — one `floor` each per block, noexcept, no allocation — so the construction default and the host path are identical and "1 m = exactly no effect" holds from block 0. The harness prints the evidence line (`distance raw default 1.00000012 (APVTS, unsnapped), snapToLegalValue → 1`). Worth a project memory: *any* sibling with a skewed+stepped range whose default is DSP-exact-sensitive carries this; the continuous-range (interval 0) degree parameters are unaffected, and the Stage-1 "interval 0" deviation is why 0° was already exact.
2. **Probe error — DSP-04 correlation.** `Σ L·s ≥ 0` failed at min −7.4e-07 (absolute): the cardioid has an *analytic* null at az = ∓120°, e = 0 (`V = 0.5 + 0.5·cos 180°`), where the float pattern value is a ±1e-8 residual. Rewritten as the correlation normalised by the source energy with a −1e-6 floor (reads −3.0e-09; analytic minimum 0). The engine was right.
3. **Probe error — meter `outPeakL`.** The expectation `0.75f·max|s|` ignored the 64-sample delay (block 8's input reaches the output in block 9). Now taken from the output samples themselves (`max|out|` over the un-read blocks), which is the actual contract. The engine/processor were right.

## Deviations from PLAN-2.1 (with reasons)

| Deviation | Reason |
|---|---|
| `DecoderMixer` API is `delayStereo()` + `blend()` (plus `processSample()` chaining both) instead of a single `processSample(stL, stR, binL, binR, t, …)` | A14 asks for the placeholder to be visible **at the engine call site**; a single call that delays internally cannot receive its own delayed output as `binL/binR`. The engine reads `dL/dR`, assigns `binL = dL; binR = dR` with the 2.3-obligation comment, then blends. 2.3's replacement is the two-line diff the plan wanted. `processSample` is kept for the real 2.3 use |
| `DecoderMixer` is `template <int N>` (`DecoderMixer<kPartition>` in the engine) | `kPartition` / `DecoderDelay` stay in `SpatialEngine.h` as the plan requires, and `SpatialEngine.h` includes `DecoderMixer.h`; a non-template mixer would need the constant before it is defined (circular include) |
| `TripleBuffer` lives in `OSimpleAmbisonics::dsp` (the origin is at global scope) | The plan names the processor member as `OSimpleAmbisonics::dsp::TripleBuffer<…>`; the body is verbatim |
| `fillParams()` snaps `distance` / `orbitRate` through their ranges (not in the plan) | Incident 1 — a real defect, fixed in the processor |
| DSP-04 "e = ±90° → L == R == 0.5f" asserted at az 0 and 180 (four points), not at an arbitrary azimuth | The float encoder's `cos(π/2)` residual is −4.37e-8; at az 0 / 180 it enters through `0.25·X` (±1.1e-8, rounds to 0.5f), but at az 90 it enters through `0.433·Y` (−1.9e-8, rounds to 0.5f − 1 ulp). The bit anchor is honest where it is exact; the 72-direction non-negativity sweep covers the rest |
| DSP-04 correlation gate normalised with a −1e-6 floor | Incident 2 — analytic null of the cardioid |
| DSP-05 "HF/LF exactly 0 dB for d ≤ 1 m" asserted as the bit-level identity `out == 0.75f·(g(d)·x[n−64])` on every d ≤ 1, with the DFT ratio printed (≤ 1.12e-07 dB) | A DFT ratio of a float signal is never *exactly* 0 dB in double; the plan's own parenthetical names the bit-level fact (`m == 0 → y == x`), which is what the identity asserts |
| DSP-01 prints 2.07e-07, not R4's 2.8e-08 | The harness feeds the float encoder `deg · kDegToRad` in float (as the engine does) while the table converts in double, so the angle quantisation is included; R4 compared the formulas at identical double angles. Gate 1e-5, 48× margin |
| Meter-frame `outPeakL` expectation taken from the output samples | Incident 3 |
| `renderHooked` takes an optional block size (default 64) | Needed for the plan's informational 1024-block run |
| P2b liveness check added | A slicing-invariance gate with a dead orbit would be the static P2 twice; the −2.69 dB half-energy difference proves the orbit moved the image inside the render |

No scope was narrowed. Every task T1–T12 is done; T13's commit is left to the orchestrator (temp-index, path-scoped).

## Deferred (carried forward by name)

- **A14 / D6 placeholder** — `binL = dL; binR = dR` in `SpatialEngine::renderSample` (and the matching two lines in `ReferenceModel`). **Phase 2.3 obligation**: replace with the UPOLS convolver output; the 2.3 verify must assert the placeholder is gone (the P1 anchor in Binaural mode will then differ from Stereo by design).
- **`kStereoLevelMatch`** — `1.0f` placeholder in `StereoDecoder.h`; 2.3's generator emits the real value into `BinauralFilterData.h` and the header reads it from there; verified by the 2.3 decoder-switch level gate (< 1 dB).
- **TripleBuffer → `core/triple-buffer` module candidate** — three verbatim consumers now (O-GrainScatter, O-simpleGrain, O-simpleAmbisonics); extraction is repo-level housekeeping after Stage 2 (`/module-create`).
- **`ShRotator` slot** — the identity pass is marked in `renderSample` ("ShRotator (Phase 2.2) applies R to tap[] here"); `azRot/elRot == azEff/elEff` until then. The `ReferenceModel` extends with a rotation matrix at 2.2.
- **−180 / 180 display** — `wrapTo180` is [−180, 180) (A6); a source parked at exactly +180° publishes −180°. Stage 3 may format −180 as 180.
- **A15 number for lesson text** — the TPT (bilinear) pole parked at the 0.45·fs clamp is **−0.06 dB at 10 kHz** (−0.11 dB at 12 kHz); the −0.85 dB figure in CONTEXT-2.1 D7 is the analog prototype. The wet mix exists for bit-exactness at ≤ 1 m, not for tone. Written into `DistanceModel.h`.
- **Human item for verify** — installed plugin hands-on in a DAW (Stereo mode pans with Azimuth, +90° louder **left**; orders change nothing audible in Stereo mode but the meter frame shows 4 / 9 / 16; Distance 1 → 20 m darkens and attenuates; Orbit On moves, Off glides back the short way; default Binaural settings are **not silent**).
- **Memory candidate** — the unsnapped APVTS raw default of a skewed+stepped range (Incident 1).

## Gotchas checklist (PLAN-2.1) — all ticked

- [x] Control grid persists across `process` calls; `k` and the ramp step come from **64**, never from the slice length (A1). P2b/P3b prove it (bit-equal with orbit On 2 Hz / order 3).
- [x] `setParams` before `engine.prepare` in `prepareToPlay`; every smoother/ramp snapped in `prepare` (A7). P1 `out[64]` exact and P5 prove it.
- [x] Order gains and decoder mix are **ramps** with exact arrival; one-poles snap at 1e-6 (A2). Order-fade (a) proves it (exact Y_4 / Y_9 in 14 steps, exact 0 in 15).
- [x] All blends `b + t·(a − b)` (A3); decoder literals `0.5f / 0.25f / 0.4330127f` (A4). P1 anchor `memcmp` proves it — and the 1-ulp miss on the first run was a wrong *parameter value*, not a tolerance problem.
- [x] `encodeSH16` takes radians; convert once per control step (`kDegToRad`, the one literal shared with the harness).
- [x] `wrapTo180` is [−180, 180) (A6).
- [x] Air LP cutoff clamped to 0.45·fs **before** `tan`; `clampedCutoffHz()` exposed and gated (19845 at 44.1 k; audible-path render at fs 22050).
- [x] No allocation in `process`/`processBlock`: `std::vector` sized in `prepare` only; no `resize` anywhere else; `TripleBuffer` is a `std::array<T,3>`. PERF-01 counter 0, liveness 1.
- [x] Engine-direct harness renders wrapped in `juce::ScopedNoDenormals` (F6) — `renderEngine`, DSP-06a, order fade.
- [x] Reference model shares nothing with `ShEncoder.h`; its table is factorials + §2.5 polynomials (R4).
- [x] Click gates: 100 Hz tone, ratio vs matched control, literals printed, negative control via `checkFailsAsExpected` (24.8 / 66.9), liveness half-circle ≥ 6 dB (8.35 / 8.14). The order-fade click gate is on the **bus tap**.
- [x] Alloc counter: buffers pre-sized, warm-up unarmed, parameter changes between `processBlock` calls on the harness thread, liveness `new` == 1 (A11).
- [x] `meterFrames` public; `hasNewData()`-gated reset at the top of `processBlock`; one `publish()` per host block (A8). Meter-frame accumulate-then-reset probe proves it.
- [x] JUCE-free `Source/dsp/` (A13; headers: `<algorithm> <array> <atomic> <cmath> <cstddef> <type_traits>`); `osa_dsp` alias in the processor TU.
- [x] `build-and-install.sh` then targeted `auval -v aufx OSAm OuDv`; pluginval s10 ×2 VST3 + AU.
- [ ] Temp-index, path-scoped commit — **left to the orchestrator** (re-check branch + status immediately before; `PLUGINS.md` untouched).

## Success criteria (for the verify phase)

- [x] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics-render-test` builds with 0 warnings.
- [x] P0 / P2 / P3 / P4 / P5 PASS; P4 latency 64 with impulse amplitude exactly 0.75; P0 `getLatencySamples() == 64`.
- [x] P1 null < 1e-5 on a fresh instance (4.6e-08); front / 1 m / order 1 anchor `memcmp`-equal to `0.75f·s[n−64]` in **both** decoder modes; P1b with the 0.75 factor.
- [x] P2b / P3b bit-equal with orbit On at 2 Hz and order 3 (D5 amendment pinned).
- [x] DSP-01 (2.07e-07, W ≡ 1, sign anchor), DSP-04 (22.88 dB, in-phase, L = R front), DSP-05 (A9 wording + clamp target), DSP-06a (through 180°, never near 0°), DSP-06b (1.0003 / 1.0000 at the 64-block hook and at 64 / 4096 orbit, orbit renders bit-equal, negative control 24.8, liveness 8.35 dB), order fade (exact 0/1 arrival, bus-tap 0.9999 / 1.0000, negative control 66.9), PERF-01 (0 allocations, liveness 1), meter frame (4/9/16, `azEff` follows the orbit, peak accumulate-then-reset), extremes finite at 48 k and 96 k — all PASS with printed literals.
- [x] pluginval strictness 10 ×2 for the installed VST3 and AU; `auval -v aufx OSAm OuDv` passes.
- [ ] Installed plugin hands-on in a DAW — verify's human item (list above).
- [x] This SUMMARY records the digest, every gate literal, the allocation-coverage statement, the D6 placeholder as a 2.3 obligation and the deferred list.
