# Stage 2 — DSP · Phase 2.3 (binaural: MagLS filter pipeline + UPOLS engine + crossfades + CPU / latency gates) — SUMMARY

**Plugin:** O-simpleAmbisonics · **Stage:** 2 DSP · **Phase:** 2.3 of 3 · **GSD phase:** execute · **Date:** 2026-10-13
**Inputs:** `PLAN-2.3.md` (T1–T9, C1–C18, the shared binaural control-step spec), `CONTEXT-2.3.md` (D1–D14), `RESEARCH-2.3.md` (R1–R10, F1–F9) + `research-2.3/` scratch, `research/ARCHITECTURE.md`, the 2.1 / 2.2 source + harness
**Executor:** orchestrator with Bash (generator, harness, installer, validators run here; T2 / T3 / T4 authored in the same session)
**Branch:** `main` (trunk-based; `plugins/O-simpleAmbisonics/**` only; `PLUGINS.md` untouched)
**Gate:** `run-gate.sh O-simpleAmbisonics 1-foundation 2-dsp` → build PASSED, pluginval PASSED, schema SKIPPED (no `HANDOFF.json` at Stage 1 — standing pattern, as in 2.2)

## Result

**The A14 placeholder is gone: the binaural leg is real.** `scripts/generate_binaural_filters.py` turns the SADIE II D1 (KU100) 48 k SOFA into three MagLS filter banks (orders 1 / 2 / 3, ACN / SN3D / AmbiX, 256 taps, hard switch with the 'avg' phase continuation, trim T = 32, level match +0.531 dB baked into the scale) emitted as `Source/dsp/BinauralFilterData.h`; `Source/dsp/BinauralConvolver.h` is the grid-synchronous fixed-partition UPOLS convolver (P = 64, FFT 128, shared input FFTs, set-*n* channel loops, dual-bank 20 ms order fade with queued latest-wins requests) stepped from `controlStep()` on the rotated bus; `SpatialEngine::renderSample` copies the convolver output in Binaural mode (D3) and blends across the decoder switch; `WindowedSinc` resampling with delta calibration at fs ≠ 48 k; latency exactly 64 in both modes at every rate; `ReferenceModel` convolves the same bus in direct-form double with the header floats and runs the same state machine with a **float** bank ramp.

**Nothing 2.1 / 2.2 proved changed: digest-2.1 `9189d911a1fe98d5` and digest-2.2 `7f35bc9f91e321b9` reproduce and are both asserted** (the Stereo path did not move by a bit). The six Binaural-anchored 2.1 probes migrated to Stereo mode per C15 and each gained a direct-form Binaural twin (2.3e-7 … 4.0e-7).

Harness: **241 checks, ALL PASS** (160 from 2.1 / 2.2 — 6 migrated per C15, digest-2.2 promoted — plus 81 new), two runs byte-identical except the PERF-02 wall-clock line; **digest-2.3 `edd1acfd21f54bcd`** (Binaural null render, informational). Build: **0 warnings** on all targets. Generator: **26 self-checks ALL PASS**, header regenerates byte-identical (sha256 `0dbc1d15…bf19f`). PERF-02: **203× real time** (48 k / 64 blocks, order 3 Binaural, best of 3), 206× at 1024 blocks, 76× at 96 k. Installed bundles `cmp`-identical to the build; `auval -v aufx OSAm OuDv` **SUCCEEDED**; pluginval strictness 10 **SUCCESS ×2 on VST3 and AU**.

Covers (gated below; verify closes them): DSP-03, FUNC-05, DSP-07, PERF-02 (must); FUNC-01 first two boxes re-gated with the real leg; FUNC-03 remainder (filter-bank swap); PERF-01 re-gated with the convolver live; QUAL-01 stage-2 part.

**Two execute findings corrected the plan's C3** (details in §Deviations): (1) the delta calibration's divisor is `kernel area / ratio` (1.98 at 96 k, 0.91 at 44.1 k), not ≈ 0.99 — dividing by it is exactly what keeps the resampled bank's frequency response (an amplitude-preserving resample would have shipped a +6 dB bank at 96 k; a new cross-rate 1 kHz level gate now pins it to ±0.006 dB); (2) the scipy golden had to be scaled by `down/up` for the same reason, and the golden comparison is taken below 15 / 16 kHz where the two anti-alias designs agree (±0.02 dB) — above that they differ by design (up to +5 dB at 18–20 kHz at 44.1 k, inaudible HRTF content, printed).

## Files

| Action | Path | Content |
|---|---|---|
| create | `scripts/generate_binaural_filters.py` | 705 lines. Download-or-cache with the zip + 48 k SOFA sha256 verified before reading (refuses on mismatch); `sn3d_matrix` / `magls` / `trunc` transcribed verbatim from `research-2.3/r2d_final.py` (hard switch, slice-then-diff Δφ; `--blend target` = the r2c target-domain alternative); oracle self-check at T = 0 against `spaudiopy.decoder.magls_bin` (N3D loop bit-identical, SN3D × per-order factor ≤ 1.2e-13); objective table per band with spaudiopy + LS-only columns; trim + spaudiopy taper; A-weighted level match (R4 definition), common scale on all three sets; ILD / ITD (C10 LP-xcorr with a synthetic sign anchor), W flatness, onsets, headroom; `%.9g` emission with comment-stripping parse-back (F9); goldens via `scipy.signal.resample_poly · down/up`; `FILTERS.md`; `.npz`. Any FAIL aborts before emitting |
| generate | `Source/dsp/BinauralFilterData.h` | 277 KB. `namespace OSimpleAmbisonics::dsp`: `kBinauralFilterSampleRate 48000`, `kBinauralFilterTaps 256`, `kBinauralFilterTrimSamples 32`, `kBinauralOnsetFrontSamples {0, 84, 70, 50}`, `kBinauralLevelMatchDb 0.531`, `kBinauralAliasingHz {0, 623.9, 1247.8, 1871.7}`, `kBinauralFilterDfe true`, source / license / sha256 / blend strings, `kMagLsFilters1[4][2][256]` / `2[9]` / `3[16]` (`inline constexpr float`, 8 per line). sha256 `0dbc1d15ffab178072f7431877b1b7dd905fdf69694bb29d85a05148034bf19f` |
| generate | `tests/render-harness/BinauralFilterGoldens.h` | 816 KB, test-only, `namespace OSimpleAmbisonics::test`: `kBinauralGolden44100_{1,2,3}[·][2][256]` (236 → 256 zero-padded) and `kBinauralGolden96000_{1,2,3}[·][2][512]`, DC-gain preserving. sha256 `0bac0968…92320` |
| generate | `scripts/FILTERS.md` | Zenodo record + the four sha256s, license, GLOBAL_History (DFE as shipped), resolved versions (sofar 1.3.0, numpy 2.2.6, scipy 1.14.1, spaudiopy 0.2.0, Python 3.12.12), trim / blend choice with the C2 numbers, every self-check line, objective tables, level match, W flatness, ILD / ITD table, onsets, headroom, artifact sha256s, regeneration note (F4) |
| generate | `.cache/binaural_filters.npz` | gitignored (root `*.cache`): the three float32 banks, the six goldens, the self-check numbers as JSON |
| create | `Source/dsp/BinauralConvolver.h` | 491 lines; the ONE JUCE-including DSP header (D6). `prepare (fs)`: at 48 k the header floats copied bit-exact; else `WindowedSinc` per FIR with the `zpre` rule (0 / 60 / 220), `delay = (100 + zpre)·fs/48000` (integer-asserted), pad grown until `nOut ≥ delay + L`, delta calibration FIRST (divide by Σ delta out), `L` / `K` from fs; spectra `[order][ch][ear][k][130]`, FDL `[ch][slot][130]` — every `std::vector` assigned here only; `juce::dsp::FFT fft { 7 }` member. `reset()`, `snapToSet`, `arrive`, `requestSet` (every step, latest wins), `step (rampStep)` (ramp → `head = (head + K − 1) % K` → 16 forward FFTs of `[prev | in | 0]`, `prev = in` after → accumulate cur [+ pending] over `(order+1)²` channels × K slots × 65 bins → 2 [+ 2] inverse → `out` / `outB` = `[64, 128)`), `push (tap16, pos)`, `outputAt (ear, pos, t)` (exact copy when not fading, A3 blend otherwise). Accessors `filterTaps`, `tapsAtRate`, `partitions`, `resamplerPrefix` / `Delay` / `DcGain` / `KernelGain`, `activeSet` / `pendingSet` / `queuedSet` / `isFading` / `bankMixCur`, `channelsFor` |
| modify | `Source/dsp/SpatialEngine.h` | `#include "BinauralConvolver.h"`; members `BinauralConvolver conv; bool binauralOnly`; `prepare` (no longer `noexcept` — it allocates): `conv.prepare (sampleRateHz)` after `rot.prepare()` before `snapToParams()`; `snapToParams`: `conv.reset(); conv.snapToSet (params.order); binauralOnly = mixer.bothEndpointsAtOne()`; `controlStep`: after `rot.build` — `conv.arrive(); conv.requestSet (params.order); conv.step (kRampStep); binauralOnly = …` then `fillFrameState()`; `renderSample (x, t, pos, outL, outR)` (`process` passes `gridPos + j`): after the peak scan `conv.push (tap, pos)`, `binL/R = conv.outputAt (ear, pos, t)`, stereo decode → `delayStereo` (always runs), `binauralOnly ? copy : mixer.blend`; the A14 lines deleted; `convolver()` accessor. Header comment: chain with the convolver, D1 / D3 / D4 block, D2 level-match note, "JUCE reaches this header only through BinauralConvolver.h" |
| modify | `Source/dsp/DecoderMixer.h` | `bothEndpointsAtOne()`; header comment: placeholder gone, the split API stays for the D3 copy at the call site |
| modify | `Source/dsp/StereoDecoder.h` | Comment only: `kStereoLevelMatch = 1.0f` BY DESIGN — the match lives in `kBinauralLevelMatchDb` (D2) |
| modify | `Source/PluginProcessor.cpp` | Header comment only: the chain diagram with the convolver and the mixer's copy / blend |
| modify | `CMakeLists.txt` | `target_sources` + `BinauralConvolver.h`, `BinauralFilterData.h` |
| modify | `tests/render-harness/ReferenceModel.h` | `#include "dsp/BinauralFilterData.h"` (never `BinauralConvolver.h`); `snapBankFade` / `snapDecoderMix` switches; `FloatRamp` (the engine's `LinearRamp` text in float, C5); `setExternalFilters (order, ch, ear, taps, len)`; `prepare`: the header floats widened at 48 k (or the external table; `std::abort` with a message otherwise), ring `hist[16][1088]`, bank state snapped, `kRampStepF` from the engine's expression; `convArrive` / `convRequestSet` / `convStepBank` after `evaluateC()`, `binauralOnlyRef`; `direct (order, ear)` = Σ_ch Σ_t h·tap[n − 64 − t] read after the tap write at `hIdx`; `renderSample`: A14 lines replaced by the direct-form convolution, the fade `(double) (prevBank + (float) t · (curBank − prevBank))`, `snapBankFade` → the pending set, `binauralOnlyRef ? copy : A3 blend`; `og[3]` / `mix` / `dlL` / `dlR` / `refSh16` / `buildR3` untouched. `snapDecoderMix` snaps `mix` in `controlStep` |
| modify | `tests/render-harness/main.cpp` | 3820 lines (+ ~1100). Title "Phase 2.1 + 2.2 + 2.3", the C15 note and the 2.3 probe list; `#include "dsp/BinauralConvolver.h"`, `"BinauralFilterGoldens.h"`, `<chrono>`; helpers `Fft4096`, `aWeight`, `aWeightedLevel`, `itdLowPassXcorr`, `ildDb`, `ildBandDb`, `onset10`, `firstNonZero`, `loadExternalFilters`, `maxErrLr`, `refRenderStatic`, `headerTaps`, `goldenTaps`; the C15 migration (T6); the 2.3 probe block before PERF-01 (T7); digest-2.3 print; PERF-02 last. `OSA_DUMP_TABLES=<dir>` dev aid in the resampler probe (writes every engine table + golden as text). Every untouched 2.1 / 2.2 probe body and verdict string is as in 2.2 |
| create | `.planning/stages/2-dsp/SUMMARY-2.3.md` | this file |
| modify | `.planning/STATUS.md` | Phase 2.3 execute ✓, frontmatter, Current Position, Completed So Far, Next Steps, Files Created |

Not touched (per C17 / C18): `Source/PluginProcessor.h`, `Source/dsp/{Smoothers,ShEncoder,ShRotator,DistanceModel,OrbitLfo,FixedDelay,MeterFrame,TripleBuffer}.h`, `tests/render-harness/CMakeLists.txt` (the goldens header arrives via the existing include dir), `tests/pedalboard/*` (verify), `PLUGINS.md`, `REQUIREMENTS.md`, `setLatencySamples`, the 12 parameters, any editor file. `refParamsFrom` unchanged (no new `RefParams` field); `renderRef` signature unchanged (the 96 k null seeds the reference by calling `loadExternalFilters` before `renderRef`, so the optional-pointer parameter T5 suggested was not needed).

## The generator run (T1) — every self-check number

`uv run --python 3.12 --with "sofar==1.3.0,numpy,scipy<1.15,spaudiopy" scripts/generate_binaural_filters.py --no-download` → **ALL PASS (26 self-checks) in 6.7 s**; resolved sofar 1.3.0, numpy 2.2.6, scipy 1.14.1, spaudiopy 0.2.0, Python 3.12.12.

| Self-check | Value |
|---|---|
| sha256 zip / 48 k SOFA / 44.1 k / 96 k | `366321fa…9705f75` / `9af7cb19…0ccd4` / `efffbf7f…3766d2` / `49c46541…2f98983` — all pinned |
| SOFA | fs 48000, `Data.IR (8802, 2, 256)` |
| f_a → bin | 623.9 Hz → 13 (609 Hz), 1247.8 → 27 (1266), 1871.7 → 40 (1875) |
| oracle (own loop on spaudiopy's N3D Y vs `magls_bin`) | **0.0** at every order (bit-identical) |
| oracle (own SN3D bank vs `magls_bin` × per-order factor) | **1.15e-13 / 4.08e-14 / 9.70e-14** (factors 0.282095, 0.488603, 0.630783, 0.746353) |
| objective, RMS dB (own at T = 32 / spaudiopy at T = 0 / LS-only): order 1 | 50–600: 0.935 / 0.897 / 0.954 · 600–2 k: 1.393 / 1.379 / 5.148 · 2–5 k: 2.977 / 2.976 / 14.443 · 5–10 k: 5.418 / 5.418 / 18.431 · 10–20 k: 6.929 / 6.929 / 24.544 |
| … order 2 | 0.617 / 0.580 / 0.612 · 1.426 / 1.420 / 2.880 · 2.426 / 2.425 / 9.575 · 4.166 / 4.166 / 16.347 · 5.652 / 5.652 / 20.987 |
| … order 3 | 0.592 / 0.565 / 0.594 · 1.520 / 1.527 / 1.717 · 1.871 / 1.869 / 7.248 · 3.570 / 3.571 / 14.592 · 4.901 / 4.901 / 19.188 |
| A-weighted white front gain before scale (unweighted) | order 1 0.6762 (0.6559), order 2 0.7031 (0.7007), order 3 **0.7055** (0.7418) |
| common scale | s = 0.75 / 0.7055 = **1.063085 (+0.531 dB)** |
| level after the scale vs stereo 0.75 | order 3 **+0.00e+00 dB**; orders 1 / 2 **−0.368 / −0.030 dB** (expect −0.37 / −0.03) |
| trim loss (head / tail) | order 1 0.0099 / 0.0008 %; order 2 0.0100 / 0.0020 %; order 3 0.0141 / 0.0044 % |
| no NaN / inf; tap 0 exactly 0 | PASS / PASS |
| peak \|tap\| | 0.5949 / 0.4153 / 0.3734 |
| W 50–200 Hz re 200 Hz (order 3) | L +0.19 / +0.00 dB, R +0.19 / +0.00 dB |
| ILD +90 / −90 (per band +90: 200–1 k / 1–3 k / 3–8 k / 8–16 k) | order 1 **+12.28 / −11.89** (+4.8 / +11.4 / +15.8 / +12.6); order 2 +13.79 / −13.98 (+4.8 / +8.6 / +14.9 / +16.7); order 3 **+13.87 / −14.58** (+5.1 / +6.2 / +17.3 / +20.0) |
| ITD by LP-xcorr (LP < 499 / 998 / 1497 Hz) | order 1 **+36 / −36**, order 2 +38 / −38, order 3 **+37 / −37** samples (positive = L leads) |
| ITD helper sign anchors | synthetic delayed pair reads +17; raw SADIE az 90° HRIR reads +38 |
| front onsets (10 %) L / R | **{84, 70, 50}** / {84, 75, 56} → `kBinauralOnsetFrontSamples = {0, 84, 70, 50}` |
| headroom (order 3, all directions × bins) | max \|H\| 2.119 = **+6.52 dB at 9469 Hz** |
| goldens | 44.1 k: 147/160, 236 → 256, scale 1.088435; 96 k: 2/1, 512, scale 0.5; DC gain of order-3 W/L preserved: 0.69966 / 0.69980 vs 0.69971 |
| parse-back (plugin header, goldens header) | bit-equal / bit-equal |

Two generator runs produced the **same** `BinauralFilterData.h` sha256 (`0dbc1d15…bf19f`) — byte-identical regeneration on this machine (F4). The "own" objective column is at T = 32 (the shipped trim) while spaudiopy's is at its own T = 0, hence the few-thousandths differences in the 50–600 Hz band; the T = 0 tap null (≤ 1.2e-13) is the identity proof.

## The binaural control step, as implemented (PLAN-2.3 shared spec — engine and reference op-for-op)

```
prepare (fs):     … rot.prepare(); conv.prepare (fs)       // tables (copied at 48 k, else WindowedSinc + delta calibration),
                                                           // spectra for the three sets, FDL for K(fs); then snapToParams()
snapToParams():   … (2.2 snaps) … conv.reset(); conv.snapToSet (params.order); binauralOnly = mixer.bothEndpointsAtOne()
controlStep():    … (2.1 / 2.2 steps through rot.build) …
                  conv.arrive()                    // fading && curBank == 1 → cur = pending, fading = false, banks 0, apply queued
                  conv.requestSet (params.order)   // every step: fading → queued = n (latest wins); n != cur → start fade
                  conv.step (kRampStep)            // prevBank = curBank; ramp; head = (head + K − 1) % K; 16 FFTs; accumulate cur [+ pending]
                  binauralOnly = mixer.bothEndpointsAtOne()
                  fillFrameState()                 // unchanged, no new field
renderSample (x, t, pos):
                  … bus → tap → shPeak (unchanged)
                  conv.push (tap, pos); binL/R = conv.outputAt (ear, pos, t)   // out, or out + m·(outB − out), m = prevBank + t·(curBank − prevBank)
                  StereoDecoder::decode (tap) → mixer.delayStereo (always)
                  binauralOnly ? (outL, outR) = (binL, binR) : mixer.blend (dL, dR, binL, binR, t, outL, outR)
```

Reference twin (double): the same machine with a float `bankMix`; `bin[ear] = Σ_ch<(order+1)² Σ_t<hLen h[ch][ear][t] · hist[ch][n − 64 − t]` from the header floats widened (48 k) or the engine's resampled table (`setExternalFilters`); `snapBankFade` outputs the pending set from the request step; `snapDecoderMix` snaps the mixer ramp. Ramp arrivals at 48 k: the bank fade 0 → 1 arrives at step 15 in float (asserted exactly: `bankMix == 1.0f` on the block starting at 1920 after a request at 1024, `activeSet 3 / not fading / bankMix 0` on the block at 1984).

## Harness — every 2.3 literal (ALL PASS, 241 checks; two runs byte-identical except the PERF-02 timing line)

| Probe | Result |
|---|---|
| digest-2.1 verdict | `9189d911a1fe98d5` ✓ |
| **digest-2.2 verdict (promoted, C16)** | `7f35bc9f91e321b9` ✓ |
| C15 migrated: P1 anchor Stereo (default tuple) / out[64] exact (Stereo) / engine-direct (Stereo) / P1b Left only (Stereo) / P1b mono-in (Stereo) / P4 (Stereo) | all exact: `−1` (no mismatch), 0.2463251 vs 0.2463251, first nonzero 164, 0.7500000 |
| C15 Binaural twins vs the direct-form reference | P1 defaults **2.654e-7** (peak 0.968); engine-direct 2.255e-7; P1b Left only 4.033e-7 (peak 1.103); P1b mono-in 4.033e-7 |
| A14 placeholder gone (inverted equality, D13) | max \|L_bin − L_st\| **1.3988** (> 0.05) |
| placeholder gone (D2) | `kStereoLevelMatch` 1.0000000, `kBinauralLevelMatchDb` +0.531 dB |
| header sanity | onsets {0, 84, 70, 50}, level match +0.531 dB, trim 32, blend hard-switch+avg-continuation, f_a {623.9, 1247.8, 1871.7}, DFE as shipped |
| UPOLS null (D7), processor, Binaural, rotated null tuple, orders 1 / 2 / 3 × host blocks 32 / 64 / 100 / 512 / 1024 / 4096 | **9.547e-8 / 1.355e-7 / 1.346e-7** at every block size (peaks 0.366 / 0.375 / 0.431) |
| null at 96 k (K 8, L 512), engine-direct, reference seeded with the resampled table | **1.065e-7** (peak 0.307) |
| resampler vs scipy golden at 44.1 k (both low-passed at 14 994 Hz; \|H\| below that on bins ≥ −20 dB) | worst rel err **3.642e-3** (order 3 ch 2; gate 1.5e-2), worst \|H\| diff **0.0879 dB**; design difference 15–20 kHz up to 5.17 dB (info; raw unfiltered max\|err\| 4.0e-2); kernel gain 0.990031, divisor 0.909591, prefix 60, delay 147, L 256 / K 4 |
| resampler vs scipy golden at 96 k (band 16 kHz) | worst rel err **2.267e-3** (order 3 ch 7; gate 6e-3), worst \|H\| diff **0.0480 dB**; design difference 16–20 kHz up to 0.13 dB; kernel gain 0.990000, divisor 1.980000, prefix 0, delay 200, L 512 / K 8 |
| resampler negative control at 96 k (un-calibrated × kernel gain) | raw rel err **1.078e-2** > 6e-3 → FAIL-as-expected |
| **resampler level (new gate): 1 kHz front tone, order 3 Binaural** | L amplitude 48 k 0.35164, 44.1 k 0.35180 (**+0.004 dB**), 96 k 0.35140 (**−0.006 dB**) |
| 48 k table bit-exact to the header, dcGain == kernelGain == 1.0f, L 256 / K 4 | ✓ |
| DSP-03 onset constants at 48 k | engine-table front onset₁₀% == `kBinauralOnsetFrontSamples[1]` == **84**, `[3]` == **50** exactly |
| DSP-03 onset (impulse at 100, front, d 1, Binaural; ±2; out[0..127] exactly 0; \|out[128..163]\| < 1e-6) | 44.1 k: order 1 **237** (= 100 + 64 + 73; R 241), order 3 **210** (46); 48 k: order 1 **248** (84; R 248), order 3 **214** (50; R 220); 96 k: order 1 **332** (168; R 331), order 3 **263** (99; R 275). First nonzero 128 at every rate (FFT roundoff, floor 4.6e-10 … 3.0e-8) |
| DSP-03 `getLatencySamples()` == 64 both modes, 44.1 / 48 / 96 k | 64 at all six |
| DSP-03 ILD / ITD (impulse, el 0, d 1) az +90 / −90 | order 1: **+12.28 dB / +36** and **−11.89 dB / −36**; order 3: **+13.87 dB / +37** and **−14.58 dB / −37**; per band +90 order 3: +5.1 / +6.2 / +17.4 / +20.0 dB |
| FUNC-05 level delta (front, A-weighted white, mean-of-ears) | order 3 **+0.004 dB**; order 1 −0.340, order 2 −0.025 (printed) |
| DSP-07 order 1→3 / 3→1 clicks at 64 blocks (100 Hz, az +90, Binaural) vs the same-motion reference | ratios **0.9984 / 1.0037** (1→3 L / R), **1.0004 / 0.9995** (3→1); negative control (snapped bank + snapped order gains) **72.9 / 43.3** and **179.1 / 68.4** → FAIL-as-expected; static endpoints L 6.685e-5 / 6.911e-5 |
| … at 1024 blocks | **1.0001 / 1.0004** and **1.0002 / 1.0003**; negative control **11.3 / 34.9** and **34.0 / 179.1** → FAIL-as-expected (lower than the 239 / 345 prediction because at 1024 blocks the control window includes the fade's own curvature, 2.96e-4 vs 6.7e-5 — still ≫ 2.0) |
| DSP-07 spectral liveness (3 kHz, az +90, orders 1 vs 3) | R ear **+6.28 dB** (expect +6.2), L −2.36 dB; R mags 0.1067 / 0.2200 |
| DSP-07 decoder B→S→B click at 64 / 1024 blocks | ratios **1.0007 / 1.0001** and **1.0000 / 1.0002**; negative control (snapped decoder mix) **28.2 / 12.9** and **11.9 / 14.4** → FAIL-as-expected; endpoints differ **+19.40 dB** (R ear, expect ≈ 19) |
| bank-fade queue (3 at 1024, 2 queued at +7 grids, 1 at +9 latest wins, 2 at 4096) vs the reference at 64 / 1024 blocks | **3.112e-7** both (peak 1.564); final set 2, not fading (reference agrees); liveness mid-fade \|q − o1\| 1.446, \|q − o2\| 1.500 |
| bank-fade arrival exactness (F1) | step 14 0.933333, **step 15 1.000000** (fading, set 1), after arrival **0.000000** (not fading, set 3) |
| P2e / P3e (Binaural, rotation + orbit) | ragged ≡ flat; prep-64 + 4096 feed ≡ prep-4096 ≡ flat; liveness ≠ the 2.2 Stereo flatRot |
| P2f (order / decoder / yaw stepped at 4096 boundaries, Binaural null tuple) | 64-block ≡ 4096-block (bit); liveness ≠ hook-less |
| FDL decays to exact 0 | 48 k (K 4): last nonzero **2367**, bound 2368; 96 k (K 8): last nonzero **2623**, bound 2624 |
| extremes (2.3) | 8 Binaural corners × 48 k / 96 k finite and live; gimbal θ = 90 Binaural order 3 finite and live; headroom corner (d 0.2, az +90, order 3) peak \|out\| **5.329 (+14.5 dBFS)** finite — printed, not asserted (F7: g(0.2) = +8.9 dB + bank +6.5 dB; `outputLevel` is the user's tool) |
| PERF-01 | **0 allocations** across 200 processBlock calls with the convolver live and the order / decoder fades inside the window; liveness 1 |
| digest-2.3 (informational, D6) | **`edd1acfd21f54bcd`** (Binaural null render, order 3, block 512, 8256 samples) |
| PERF-02 (Release, `steady_clock`, 480 000 samples, best of 3) | **203× real time** at 48 k / 64 blocks (gate ≥ 20); 1024 blocks 206×; 96 k / 64 blocks (K 8) 76× (second run: 203× / 205× / 75×) |

The 2.1 / 2.2 probe set (160 checks) passes unchanged apart from the six C15 migrations and the digest-2.2 promotion.

## Build / install / validation (T8)

- `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test`: **0 compiler warnings** (three `-Wshadow` hits during execute — locals `in` / `out` in `prepare`, `m` in the reference blend — renamed `rin` / `rout` / `mb`; the generated header compiles clean under `-Wfloat-conversion` / `-Wsign-conversion`).
- Harness ×2: ALL PASS, byte-identical except the PERF-02 timing line (203× / 206× / 76× vs 203× / 205× / 75×).
- `./scripts/build-and-install.sh O-simpleAmbisonics`: OK (AU cache cleared, dual-variant sweep); installed `O-simpleAmbisonics-dev.vst3` / `.component` binaries `cmp`-identical to the build.
- `auval -v aufx OSAm OuDv`: **AU VALIDATION SUCCEEDED** (targeted, never `auval -a`).
- pluginval strictness 10, `--timeout-ms 600000`, twice each on the installed `O-simpleAmbisonics-dev.vst3` and `.component`: **SUCCESS ×4**.

## Allocation / RT statement

Every `std::vector` in `BinauralConvolver` (`taps[3]`, `spec[3]`, `fdl`) is assigned in `prepare()` only; the per-grid state (`in` / `prev` / `out` / `outB` / `tmp` / `acc` / `accB`) is fixed arrays; `juce::dsp::FFT` is a member constructed once (processor construction); `juce::Interpolators::WindowedSinc` lives on the stack inside `prepare()`; nothing in `step` / `push` / `outputAt` touches `std::vector::operator[]` in the per-sample path (`in` / `out` / `outB` are arrays; the FDL and spectra are read per grid). `SpatialEngine::prepare` lost its `noexcept` (it allocates through `conv.prepare`). PERF-01 measures 0 allocations with the convolver live and the fades armed; `ReferenceModel` is a dev tool and allocates freely.

## Deviations from PLAN-2.3 (each with its reason)

1. **C3 — the calibration divisor (HIGH, physical correctness).** The plan (from R5 / F2) expected `g_fs = Σ (resampled delta) ≈ 0.990`. Measured: **1.980 at 96 k, 0.9096 at 44.1 k** = kernel area / ratio. The 1/ratio is not an error to remove: an interpolator preserves the waveform's AMPLITUDE, so a filter whose taps are merely interpolated to twice the rate has twice the DC gain — verified in scipy (`|H(1 kHz)|` 0.376 at 48 k vs 0.753 after an amplitude-preserving resample to 96 k). Dividing by the delta sum makes Σ taps rate-independent, which is the physically right normalisation for a filter. The engine does exactly that (unchanged from the plan's text); `resamplerDcGain()` returns the divisor and a new `resamplerKernelGain()` (= divisor × ratio ≈ 0.990) returns R5's number. The scipy goldens are now `resample_poly (h, up, down) · down/up` (generator self-check: order-3 W/L Σ taps 0.69966 / 0.69980 vs 0.69971). A **new gate** — a 1 kHz front tone at order 3 / Binaural reads the same amplitude at 44.1 / 96 k as at 48 k (±0.006 dB) — pins the absolute level across rates; no plan gate could see a +6 dB bank at 96 k (the 96 k null, the onset gates and the golden gate are all relative).
2. **C3 — the golden gate's band.** Research's tolerances came from one LF-heavy test FIR; the HF-rich order-3 channels of the real bank expose the two anti-alias DESIGNS' transition bands (WindowedSinc vs scipy's Kaiser β = 5): raw time-domain max|err| 0.22 of peak at 44.1 k, |H| differences up to +5 dB at 18–20 kHz (44.1 k) — while below 18 kHz the two agree within ±0.02 dB (median per band, all 58 FIRs). The gate is therefore taken where both designs are transparent: both tables brick-wall low-passed at min (16 kHz, 0.68 × Nyquist) = 14 994 Hz at 44.1 k / 16 kHz at 96 k, the research tolerances on that (3.6e-3 / 2.3e-3 measured vs 1.5e-2 / 6e-3), |H| within 0.1 dB in band on bins ≥ −20 dB re the FIR's spectral peak (the −40 dB mask of the plan admitted near-zero LS-region bins of the HF channels, where a 0.14 dB reading at 15.7 kHz was the start of WindowedSinc's 44.1 k roll-off); the in-band / out-of-band design difference is printed. The content above 15–16 kHz through the resampled banks is the interpolator's own design — inaudible HRTF content, documented, not a defect.
3. **C4 — the DSP-03 onset expectation.** `onset₁₀% == 64 + round (kBinauralOnsetFrontSamples[n] · fs/48000) ± 3` at 44.1 k read 237 on the L ear (R 241) — a 10 %-threshold crossing moves by a few samples when the waveform is resampled. The gate now derives the expectation from the ENGINE TABLE's own front response at that rate (± 2 everywhere) and asserts the table onset == the generator constant EXACTLY at 48 k (84 / 50); the scaled constant is printed. "First nonzero ≥ 164" could not be a gate either: an FFT convolver leaves ~1e-8 roundoff from sample 128 on (the grid after the impulse's) where the exact convolution is 0 — gated as `out[0..127]` exactly 0 and a 1e-6 floor on [128, 164) (measured 4.6e-10 … 3.0e-8), the F6 principle applied to the head as well.
4. **T5 — `renderRef` unchanged.** The 96 k null calls `loadExternalFilters (model, seedEngine)` before `renderRef`, so no optional-pointer parameter was needed; `EngineRender` gained no `convState` field — the queue probe reads the engine state inline per block (as the 2.1 order-fade probe does).
5. **T7 — the bank-fade queue at 1024 blocks** runs the same schedule expressed as thresholds (`n ≥ 4096 → 2, ≥ 1600 → 1, ≥ 1472 → 2, ≥ 1024 → 3`), so at 1024-sample hooks the order lands 3 → 1 → 2 (the queue is exercised at 64 blocks; at 1024 the null still binds the two state machines). The "arrival exactness" check uses a plain 1 → 3 request (no queue) so `bankMix == 0 / activeSet == 3 / not fading` can hold literally on the arrival block.
6. **T7 — extremes "outputLevel +12 dB"** does not exist (the parameter's range is −60 … 0 dB); the headroom corner is d 0.2 (g = +8.9 dB) at az +90 / order 3 / Binaural with full-scale noise, peak printed (5.33, +14.5 dBFS), finite asserted.
7. **C2 wording deviation — stated explicitly:** the banks are the HARD SWITCH with the 'avg' phase continuation, not a half-octave blend. The literal complex blend cancels (|H| moves up to 27.6 dB at order 3 inside [f_a/2, 2f_a]); the safe target-domain blend costs 0.3–0.6 dB of magnitude fit for a phase benefit no gate measures; `--blend target` remains the one-flag alternative; `FILTERS.md` records the numbers. ARCHITECTURE's "half-octave blend" is read as satisfied by the continuation — a wording deviation, not an algorithm change.
8. **C15 entry-check correction — stated explicitly:** CONTEXT-2.3 named two probes; six 2.1 probes anchored on the Binaural placeholder and were migrated (Stereo + twins) as the plan directed.

## Open items / deferred (by name)

- **TripleBuffer** → `core/triple-buffer` module candidate (3 consumers) — unchanged.
- **−180 / 180 display**, **gimbal tooltip**, **`elEff` / `azEff` readout formatting** → Stage 3.
- **Decoder-view bank-fade progress field** (Stage-3 question): `MeterFrame` has no new field (the plan's "no new field" rule); if the UI wants the fade shown, `BinauralConvolver::bankMixCur()` / `isFading()` are the sources.
- **QUAL-02 listening + presets** → Stage 4; **F7 headroom note for presets**: the order-3 bank peaks at +6.5 dB (9.5 kHz, worst direction) and d = 0.2 m adds +8.9 dB — presets should not sit near 0 dBFS in Binaural; `outputLevel` exists.
- **BiMagLS / DFE option / second head** → v1.1.
- **Windows digest parity**: none claimed (D6 — digest-2.3 is vDSP-only, informational).
- **Step-response audibility** (2.2 carry) → hands-on at verify.
- **HF above 15–16 kHz at fs ≠ 48 k** is the interpolator's own design (deviation 2); a v1.1 option would be an own Kaiser polyphase resampler matching the scipy design, for no audible gain.
- **Verify-phase deliverables (C18):** `tests/pedalboard/verify_2_3.py` on the installed VST3 (header-float convolution within 1e-5 at 48 k, ILD / ITD by C10, equal latency both modes, level delta, 44.1 / 96 k vs `resample_poly · down/up` within the band tolerances above, cross-rate 1 kHz level, corners finite, state round-trip; every numeric parameter by `raw_value`), the DAW hands-on (headphones, order 3: front / back / left / right / overhead distinct, +90° louder LEFT, image sharpens 1 → 3, switches click-free, no loudness jump), `stages/2-dsp/VERIFICATION.md` roll-up, `REQUIREMENTS.md` ticks, the `PLUGINS.md` row.
