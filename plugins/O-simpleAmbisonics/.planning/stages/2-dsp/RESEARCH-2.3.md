# Stage 2 — DSP · Phase 2.3 (binaural: MagLS filter pipeline + UPOLS engine + crossfades + CPU / latency gates) — Research

**Plugin:** O-simpleAmbisonics
**Stage:** 2 of 4 — DSP
**Phase:** 2.3 of 3 — the real binaural leg
**GSD phase:** research
**Date:** 2026-10-12
**Branch:** `main` @ `d6a1b51c` (CONTEXT-2.3 committed)
**Inputs:** `CONTEXT-2.3.md` (D1–D14, R1–R10), `research/ARCHITECTURE.md` §Binaural Decoder / §Fixed-partition UPOLS convolver / §Implementation Risks / Decisions 1–4, 8 / §Sample Rate Handling / §Latency (immutable), `research/ambisonics-binaural-decoding-deep-dive.md` §2.2–2.6, §3.5, §4.2–4.4, §6.5, `Source/dsp/{SpatialEngine,StereoDecoder,DecoderMixer,Smoothers,ShEncoder}.h`, `tests/render-harness/{main.cpp,ReferenceModel.h,CMakeLists.txt}`, `tests/pedalboard/verify_2_2.py`, JUCE 8.0.15 source (`juce_FFT.cpp` AppleFFT + FFTFallback, `juce_Interpolators.h`, `juce_GenericInterpolator.h`), spaudiopy `decoder.magls_bin` source (read as the oracle, D8), O-GrainScatter `generate_hrir_header.py` + `BinauralDecoder.h` (header-emission shape, direct-form loop), project memory (`index_dsp_gate_design`, `index_git_shared_checkout`)
**Numerics:** every number below was computed, not estimated. Scratch sources and their logs are kept in `stages/2-dsp/research-2.3/` (`r6_upols_scratch.cpp` + `r6_CMakeLists.txt` + `r6_run.log` — a JUCE console app; `r1_sofa.py`, `r2_magls.py`, `r2b_debug.py`, `r2c_objective.py` + `.log`, `r2d_final.py`, `r5_compare.py`, `r5b_kernel.py`, `r5c_calib.py`). The SADIE archive is cached, gitignored, under `plugins/O-simpleAmbisonics/.cache/` (root `.gitignore` `*.cache`, confirmed with `git check-ignore`).

---

## Entry check

| Item | State |
|---|---|
| Contract drift | None. ARCHITECTURE.md, ROADMAP.md §2.3, REQUIREMENTS.md unchanged since the 2.3 discuss |
| Engine seam | `SpatialEngine::renderSample` A14 lines (`SpatialEngine.h:370-376`), `controlStep()` at `gridPos == 0`, `mixer.delayStereo` / `mixer.blend` split API, `kStereoLevelMatch = 1.0f`, `static_assert (kControlGrid == kPartition)` — all as CONTEXT-2.3 §Entry check recorded |
| Reference seam | `ReferenceModel.h:412` `const double binL = dL, binR = dR;` — the model's ramps (`Ramp og[3], mix`) are **double**; see F1 before reusing that type for the bank fade |
| Harness | `renderHooked` / `renderEngine` / `renderRef`, `worstCurvature` + `ClickReading` + `kClickRatioCeiling = 2.0` + `checkFailsAsExpected`, `binMagnitude`, rtcheck, digests at `main.cpp:2651/2653` — unchanged since 2.2 verify |
| Build type | `build/CMakeCache.txt` `CMAKE_BUILD_TYPE=Release` → the harness compiles with `NDEBUG` (R9) |
| Tooling | `uv run --python 3.12 --with "sofar,numpy,scipy<1.15,spaudiopy"` — **spaudiopy is incompatible with scipy ≥ 1.17** (`scipy.special.sph_harm` removed; the first run died with `AttributeError`). With `scipy<1.15` it resolves (sofar 1.3.0, numpy 2.x, scipy 1.14.x). The generator's own path needs only sofar / numpy / scipy; the pin exists for the oracle self-check |
| JUCE FFT engine | macOS: `AppleFFT` (vDSP). Windows / Linux without IPP / MKL: `FFTFallback`. Both honour the layout below (R6) |

Scope discipline: no algorithm in ARCHITECTURE was re-opened. Two contract *wordings* are read differently from their literal text on the strength of measurements, and both are flagged for Taylor in §Open items: the "half-octave blend" (R2 / F8) and the "onset guard ≈ 8" of D9 (R3 / F6).

---

## Answers to the open questions

### R1 — SADIE II D1: 8802 directions, SOFA azimuth CCW with +90° = left, receivers at ±0.09 m, the set is already diffuse-field equalised, onset lead 64–120 samples at 48 k

`D1_HRIR_SOFA.zip` from `https://zenodo.org/records/12092466/files/D1_HRIR_SOFA.zip?download=1`: 38 151 804 bytes (the server's `content-length`, matched), **sha256 `366321fa78f211bacc0ec6bea96701625b196a5db54ec16748b0eab0b9705f75`**. It unpacks to three SOFA files in `D1_HRIR_SOFA/`:

| File | Size | fs | taps | sha256 |
|---|---|---|---|---|
| `D1_44K_16bit_256tap_FIR_SOFA.sofa` | 36 310 650 | 44 100 | 256 | `efffbf7f8e963c8162f24f44944ed6f8a44f5d3c546e76a4d74a650a0e3766d2` |
| **`D1_48K_24bit_256tap_FIR_SOFA.sofa`** (the generator's input) | 36 310 650 | 48 000 | 256 | `9af7cb19531e52fb7ae8ec92621e6ab62b1d5fe584b3742be36699a0ddb0ccd4` |
| `D1_96K_24bit_512tap_FIR_SOFA.sofa` | 72 366 402 | 96 000 | 512 | `49c46541517cd8c43927297fc9106b8a9aee7aec25c3a6ba6d144b6da2f98983` |

`sofar.read_sofa` (1.3.0) facts, identical across the three:

- Convention `SimpleFreeFieldHRIR 1.0`, `Data.IR` shape `(8802, 2, taps)` float64, `Data.Delay` = 0 everywhere, `Data.SamplingRate` as named.
- `SourcePosition` spherical in degrees, **azimuth 0…359 counter-clockwise from the front, elevation −90…90, radius 1.2 m** (measurement distance). `ListenerView = (1, 0, 0)`, `ListenerUp = (0, 0, 1)`, `ReceiverPosition` = `(0, +0.09, 0)` for receiver 0 (**left**) and `(0, −0.09, 0)` for receiver 1 — so SOFA +y is the left ear, which is AmbiX `+y` = left = the `ShEncoder` azimuth +90° (`ShEncoder.h:106`). Confirmed on the data: the az 90° measurement has rms ILD **+15.7 dB left**, az 270° −16.1 dB, front −1.1 dB, back +0.5 dB.
- Grid: 24 unique elevations (−90, −81, −75, −64.8, … 64.8, 75, 81, 90), one point at each pole, up to 400 points per ring — a Gaussian-like layout, 8802 points.
- `GLOBAL_License`: Apache-2.0 (full text in the file); `GLOBAL_History`: **"Measurement, Low Frequency Extension, Diffuse Field Equalization, Windowing, Diffuse Field Equalization"** — the shipped set is already DFE'd (→ R4); `GLOBAL_References` doi 10.3390/app8112029; `GLOBAL_Comment`: KU100, Genelec 8010, overlapped swept sine.
- 48 k onsets (first sample above 10 % of the response peak): **min 64, median 90, max 120**; peaks at 90–137; raw left(90°) onset L 73 / R 104 (31 samples ITD), peak |IR| 0.96 over the set, mean rms 0.040. Energy within the 256 taps: 100 % (the file is already windowed to 256).

Cache / verify flow for the generator: `plugins/O-simpleAmbisonics/.cache/D1_HRIR_SOFA.zip` + `.cache/sadie/D1_HRIR_SOFA/*.sofa`; the script downloads if the zip is missing, **verifies the zip sha256 and the 48 k SOFA sha256 before reading**, and refuses to run on a mismatch. `FILTERS.md` records the three sha256s above, the Zenodo record, the license string and the resolved Python package versions. The 44.1 / 96 k files are listed for provenance only (D5 alternative rejected, R5).

### R2 — Own MagLS == spaudiopy to 1.2e-13 once the group-delay estimator is the same; MagLS taps are not unique, the magnitude objective is the oracle metric; the half-octave blend is harmful as a complex blend and costly as a target blend — recommend the hard switch

Own implementation (`r2d_final.py` `magls()`, SN3D / ACN / AmbiX matrix from the generic triple product — the same table the harness `refSh16` uses — at the 8802 SOFA directions; 1024-point rfft; `k_a` = nearest bin to `f_a`; LS `Y⁺·H` below `k_a`; above it, per bin, phase = `angle (Y · h_nm[k−1]) + Δφ`, magnitude = measured, one LS fit; 'avg' continuation Δφ from the W-channel LS phase just below `k_a`; `irfft`, 256-tap truncation, spaudiopy's taper):

| Check | Order 1 | Order 2 | Order 3 |
|---|---|---|---|
| `f_a = N·343/(2π·0.0875)` → bin on the 1024 grid | 623.9 Hz → bin 13 (609 Hz) | 1247.8 → 27 (1266) | 1871.7 → 40 (1875) |
| Own loop with spaudiopy's own N3D `Y` vs `decoder.magls_bin` | **0.0 (bit-identical)** | 0.0 | 0.0 |
| Own SN3D filters vs spaudiopy's N3D filters × per-order factor | **1.2e-13** (peak 0.56) | 4.1e-14 | 9.7e-14 |
| Per-order N3D → SN3D factors found | 0.282095 (= 1/√4π), 0.488603 (√3/√4π) | + 0.630783 (√5/√4π) | + 0.746353 (√7/√4π) |
| Projector `Y·Y⁺` SN3D vs N3D | 2.1e-17 | — | 1.4e-16 |

So the own generator and the outside oracle are **the same filters** up to the SN3D convention — but only after one detail was matched: spaudiopy's 'avg' group-delay estimator is `mean (diff (unwrap (angle (W_LS))[k_a−6 : k_a−1]))` — **slice first, then diff (4 differences)**. The natural reading "diff then slice" (5 differences) gives Δφ = −0.757819 vs a slightly different value, and that alone made the taps differ by 23 % (order 1) to 140 % (order 3) at the first attempt while the *magnitude objective agreed to three decimals*:

| Band | own | spaudiopy | LS-only | | own | spaudiopy | LS-only |
|---|---|---|---|---|---|---|---|
| **order 1** 50–600 Hz | 0.897 | 0.897 | 0.926 | **order 3** 50–600 Hz | 0.565 | 0.565 | 0.565 |
| 600–2 k | 1.379 | 1.379 | 5.139 | 600–2 k | 1.527 | 1.527 | 1.713 |
| 2–5 k | 2.976 | 2.976 | 14.426 | 2–5 k | 1.869 | 1.869 | 7.248 |
| 5–10 k | 5.418 | 5.418 | 18.412 | 5–10 k | 3.571 | 3.571 | 14.592 |
| 10–20 k | 6.929 | 6.929 | 24.554 | 10–20 k | 4.901 | 4.901 | 19.189 |

(RMS over 8802 directions × bins of the reconstructed-vs-measured magnitude in dB, after truncation + taper; order 2: 0.580 / 1.420 / 2.425 / 4.166 / 5.652, identical for both.) **Lesson:** the MagLS phase recursion is a phase-retrieval-like iteration — a constant delay offset or a different roundoff path yields a *different but equally valid* filter set with the same magnitude fit. The generator's oracle self-check therefore has two parts: (a) the tap-level null against `spaudiopy.decoder.magls_bin (hrirs, N, f_trans = f_a, hf_cont = 'avg')` scaled by the per-order factor, **≤ 1e-9** (observed 1.2e-13) — valid only because the estimator is deliberately spaudiopy's; and (b) the objective table above printed per band with the LS-only column (MagLS buys 5–14 dB at HF).

**Half-octave blend (ARCHITECTURE wording, deep-dive §2.3 step 3).** Two forms were measured:

- *Complex blend of the two solutions* `α·h_LS + (1−α)·h_MagLS` (the deep-dive's literal formula): the LS and MagLS solutions carry different phases in the SH domain, so the blend **cancels** — reconstructed |H| moves by up to **27.6 dB** at order 3 inside `[f_a/2, 2f_a]` (2.6 dB order 1, 4.1 dB order 2). Must not be used.
- *Own target-domain blend* (interpolate the unit phasors of the measured and the continued phase with α, renormalise to the measured magnitude, one LS fit per bin): well-behaved, but it costs magnitude accuracy exactly in the transition band — order 1 600–2 k: 1.675 vs 1.379 dB; order 3 2–5 k: 2.446 vs 1.869 dB — for a phase benefit nothing in the gate set measures.

The published MagLS (Schörkhuber / Zotter & Frank) and spaudiopy use the **hard switch with phase continuation**: the first MagLS bin inherits the LS phase of bin `k_a − 1`, which is the continuity the blend was meant to provide. **Recommendation: hard switch, no blend** (own == oracle), with ARCHITECTURE's "half-octave blend" read as satisfied by the continuation. Flagged in §Open items as a wording deviation for Taylor; the target-blend variant stays in `r2c_objective.py` as a one-flag alternative.

Other R2 facts: spaudiopy's default `f_trans` is **`N·600`** (CONTEXT said `N·500` — corrected): 600 / 1200 / 1800 Hz → bins 13 / 26 / 38 vs the ARCHITECTURE 13 / 27 / 40 — the ARCHITECTURE values stand (passed explicitly). The 8802 × 16 pseudo-inverse is well-conditioned (projector agreement 1e-16; `pinv` 2e-16 after scaling). Determinism: the recursion amplifies roundoff, so **byte-identical regeneration is a same-machine / same-BLAS property** — the committed header is the canonical artifact, pinned by its own sha256 in `FILTERS.md`; a regeneration check elsewhere should compare on the objective table, not bytes.

### R3 — Truncation loses 0.001–0.004 %; low-level pre-ringing starts at tap 0; a common trim of 32 samples keeps everything above −40 dB; the onset is order-dependent (front 50 / 70 / 84 after the trim) — D9's "guard ≈ 8" is overturned

From the untruncated 1024-point IRs of the own bank:

| | Order 1 | Order 2 | Order 3 |
|---|---|---|---|
| Energy outside taps [0, 256) | 0.0010 % | 0.0022 % | 0.0043 % |
| Energy outside [0, 192) | 0.0125 % | 0.0166 % | 0.0152 % |
| Circular wrap-around (taps 900–1023) | 5.5e-4 % | 1.5e-3 % | 3.1e-3 % |
| Bank envelope (max over channels / ears) first > 1 % of peak at tap | 53 | 37 | **34** |
| … first > 0.1 % at tap | 8 | 0 | 0 |
| Envelope peak at tap | 124 | 116 | 98 |
| Peak \|h\| (before the level scale) | 0.535 | 0.449 | 0.406 |

Pre-ringing at the −60 dB level exists from tap 0 at every order — the LS fit of delayed HRIRs plus the circular IFFT spread it over the whole buffer; it is inherent, not a bug, and it drives two definitions below (F6).

**Trim.** One common trim `T` for all three sets (the bank fade mixes two sets sample by sample, so they must stay aligned). Energy discarded from the head: `T = 16`: 0.0015 / 0.0012 / 0.0028 %; **`T = 32`: 0.0099 / 0.0100 / 0.0141 % (≈ −38.5 dB)**; `T = 48`: 0.039 / 0.058 / 0.073 %; `T = 64`: 0.11 / 0.12 / 0.19 %. The tail outside `[T, T + 256)` stays ≤ 0.0044 % for any `T` in that range (it only improves). **`T = 32` is the largest trim that keeps every sample ≥ 1 % of the peak in all three banks** (the order-3 envelope crosses 1 % at 34). Recommendation: `kBinauralFilterTrimSamples = 32`, taking taps `[32, 288)` of the 1024-point IR, then the taper (last 32 taps × the second half of a 64-point Hann, first 4 taps × the rising quarter of an 8-point Hann — spaudiopy's shape, which keeps the oracle null exact). Bank group delay drops by 0.67 ms. Alternative recorded: `T = 0` (no trim, zero loss, 2 ms bank delay).

**Onsets after `T = 32`** (first sample above 10 % of that response's own peak, through the truncated bank): over all directions min / median / max = order 1 **61 / 79 / 87**, order 2 **47 / 66 / 79**, order 3 **22 / 49 / 58**; front source L / R = **84 / 84**, **70 / 75**, **50 / 56**; front 1 %-cumulative-energy onset 72 / 66 / 50. The onset is a property of each set (lower orders smear the arrival), so D9's single guard "≈ 8" cannot exist; the generator emits **per-order front onsets** `kBinauralOnsetFrontSamples[4] = {0, 84, 70, 50}` (left ear, 10 % threshold — the generator computes them, these are the measured values) and DSP-03 gates `onset₁₀% (front, order n) == 64 + kBinauralOnsetFrontSamples[n] ± 2` at 48 k (±2 absorbs the engine's float path; at 44.1 / 96 k the expectation scales by `fs/48000`, rounded, ±3). Exactness stays with the D7 null, as D9 says.

**Headroom.** After the level-match scale (R4), the maximum `|H(f)|` over directions × bins at order 3 is **2.117 (+6.5 dB) at 9.56 kHz**; peak tap 0.432. A full-scale sine there from the worst direction clips by 6.5 dB in Binaural mode (the stereo leg's maximum is 0.933) — the D2 headroom note with its number; `outputLevel` is the user's tool; QUAL-01 gates finite.

### R4 — The shipped set is DFE'd (no own EQ); level match = +0.53 dB on the filters (order 3 front, A-weighted white); orders 1 / 2 land at −0.37 / −0.03 dB; the W channel is flat within +0.5 dB over 50–200 Hz and rolls off above (a property of the head, not the generator)

**DFE:** SADIE's history string (R1) says diffuse-field equalisation was applied in production; the raw front HRTF is flat to +0.16 / −0.06 dB at 47 / 94 Hz re 200 Hz (the "Low Frequency Extension" step). **v1.0 uses the set as shipped**; no own DFE and no generator option for one — recorded in `FILTERS.md` as `DFE: as shipped by SADIE II (GLOBAL_History)`.

**Level match, defined exactly:** with the 256-tap truncated + tapered order-3 filters, front source `c = Y(0°, 0°)` (SN3D: W = 1, X = 1, rest 0 up to the order), `H_ear(f) = Σ_acn c_acn·FFT₁₀₂₄(h_acn,ear)(f)`; level = `sqrt (Σ_k A²(f_k)·½Σ_ear |H_ear(f_k)|² / Σ_k A²(f_k))` with the IEC 61672 A-weighting magnitude formula normalised at 1 kHz, DC bin excluded, i.e. the RMS gain of white noise through the front response, A-weighted, power-mean of the ears. The stereo decoder's front response is 0.75 flat, so its level is 0.75.

| | Order 1 | Order 2 | Order 3 |
|---|---|---|---|
| A-weighted white gain, front (before scale) | 0.6761 | 0.7031 | **0.7055** |
| unweighted | 0.6560 | 0.7006 | 0.7416 |
| re order 3 | −0.37 dB | −0.03 dB | 0 |

**Common scale `s = 0.75 / 0.7055 = 1.0631` (+0.53 dB)** applied to all three sets → `kBinauralLevelMatchDb = +0.53`; after it the decoder switch at order 3 / front is 0.00 dB by construction and the order change is −0.37 dB (1 vs 3) / −0.03 dB (2 vs 3): "one common scale so an order change does not change loudness" holds within 0.4 dB. The generator asserts the order-3 match to < 1e-6 dB and orders 1 / 2 within ±1 dB, prints all three; the engine-level FUNC-05 gate (D10) re-measures the switch on the running engine (< 1 dB).

**W-channel LF flatness, measured (order 3, L / R, re 200 Hz):** 46.9 Hz +0.53 / +0.54 dB; 93.8 Hz +0.36 / +0.37; 140.6 Hz +0.17; 281 Hz −0.40; 375 Hz −1.29 / −1.34; 469 Hz −2.43 / −2.57; 984 Hz −5.22 / −5.71. The roll-off above 250 Hz is the KU100's monopole (diffuse) response as the LS fit sees it — real, not a defect. Self-check definition: `|H_W,ear(f)|` within **±1 dB of its 200 Hz value over 50–200 Hz**, both ears (observed +0.53 / 0.00).

### R5 — `WindowedSinc` has a DC gain of 0.990 (−0.087 dB), otherwise ≤ 0.023 dB from scipy below 16 kHz; calibrate the gain in `prepare` with a resampled delta; delay `(100 + zpre)/ratio`, integer with a 60-sample prefix for the 44.1 k family; lengths 256 / 512 / 512 / 1024 / 1024

Measured on a 256-tap HRIR-like test FIR dumped from the scratch app (`r6_upols_scratch.cpp` Part C) and compared in Python (`r5_compare.py`, `r5b_kernel.py`, `r5c_calib.py`):

- **Ratio 1.0 (`process (1.0, …)`) is not the identity: output = 0.990027 × input**, flat from 20 Hz to 22 kHz (−0.0871 dB everywhere), group-delay error 0.00 samples after the 100-sample base latency, residual after dividing by that gain 6.2e-5. The 10001-point lookup table's windowed sinc does not sum to 1. This is the whole of the "1 % error" the CONTEXT anticipated.
- At the real ratios the kernel's DC gain is 0.9889–0.9901 (ratio-dependent, `fs 44 100`: 0.98894). Raw max |err| vs `scipy.signal.resample_poly` (default Kaiser β = 5): 1.0–1.4e-2 of the peak; **after dividing by the measured DC gain: 96 k 3.0e-3 (rel 3.3e-3), 88.2 k 2.4e-3, 44.1 k 7.2e-3 (rel 8.1e-3), 192 k 3.0e-3**; |H| difference ≤ **0.023 dB (96 k), 0.041 (88.2 k), 0.018 dB (44.1 k) below 16 kHz** — 0.85 dB at 44.1 k only between 16 and 20 kHz, where the two anti-alias designs' transition bands differ (`resample_poly` with β = 14 is 0.7 dB away from the default there too: the golden itself is only defined to that tolerance near Nyquist).
- **Delay:** the base latency is 100 input samples → `(100 + zpre)/ratio` output samples, where `zpre` zeros are prepended to the FIR. For the 48 k family (96 / 192 k) `zpre = 0` gives 200 / 400; for the 44.1 k family the delay is fractional (91.875 at 44.1 k) unless `zpre = 60`: 160·147/160 = **147** (44.1 k), **294** (88.2 k), **588** (176.4 k). General rule: the smallest `zpre ≥ 0` with `(100 + zpre)·fs/48000` an integer. Feed `[zpre zeros | 256 taps | ≥ 400 zeros]` with the `numInputSamplesAvailable` overload (`wrapAround = 0` feeds zeros past the end), take `out[delay … delay + L64)`. Pre-ring before the delay ≤ 6.5e-5, content beyond `L64` ≤ 6.8e-4 (the taper's sinc tail) — negligible and documented.
- **Lengths:** `ceil (256·fs/48000)` → multiple of 64: 44.1 k 236 → **256 (K 4)**; 88.2 k 471 → **512 (K 8)**; 96 k **512 (K 8)**; 176.4 k 941 → **1024 (K 16)**; 192 k **1024 (K 16)**. `prepare` must size the FDL for the largest K the sample rate implies (K = 16 at 176.4 / 192 k) — allocation in `prepare` only; at 48 k the header floats are used unmodified (bit-exact table, no resampler call).
- **Golden header size:** two rates (44.1 k: 58 × 256, 96 k: 58 × 512) = 44 544 floats ≈ 0.5 MB of text, test-only — acceptable; 96 k only would save half and lose the downsampling case, keep both. **Native-rate SADIE sets** would be 3× the header for two of the five rates and still need the resampler for 88.2 / 176.4 / 192 k — confirmed not worth it.

**D5 amendment (recommended):** in `prepare`, when `fs ≠ 48000`, first resample a Kronecker delta with the same ratio / prefix / lengths, sum the output (`g_fs`, ≈ 0.99), then divide every resampled FIR by `g_fs`. Store `g_fs` for the gates (`resamplerDcGain()`). **Gate tolerances** (≈ 2× observed, on the DC-normalised table vs the scipy golden): max |err| ≤ 6e-3·peak at 96 k, ≤ 1.5e-2·peak at 44.1 k; |H| within 0.1 dB below 16 kHz. The golden generator uses `scipy.signal.resample_poly (h, up, down)` with the reduced fraction (`147/160`, `2/1`), default window, emitted with the same `zpre`-free alignment (the engine's `delay` offset removes the prefix). Alternative recorded: an own Kaiser windowed-sinc resampler in `prepare` would match scipy more closely but replaces a verified JUCE class with new DSP for no audible gain.

### R6 — JUCE FFT contract pinned by measurement; UPOLS (D1 shape) nulls to 7e-7 against direct-form double at five block sizes and two K's, bit-equal across ragged slicing; the dual-bank fade + queue nulls to 8e-7 once the reference ramp is float; 273× real time for the scalar scratch convolver

**FFT contract** (`juce::dsp::FFT (7)`, N = 128; AppleFFT measured, FFTFallback read at `juce_FFT.cpp:184-198`):

| Probe | Result |
|---|---|
| A1 `performRealOnlyForwardTransform (buf, true)`, input in floats [0, 128), rest 0; impulse at n = 3 | bins 0…64 **interleaved re / im in floats [0, 130)**: bin k at `[2k, 2k+1]` = `e^{−2πi·3k/128}` to 1.3e-7, **no scale**; DC `(1, 0)` at [0, 1]; **Nyquist at floats [128, 129]** = `(−1, 0)`; floats 130… untouched (0) |
| A2 `(buf, false)` | bins 65…127 are the conjugates of 63…1 (exact) |
| A3 alternating ±1 | bin 64 = `(128, 0)`, bin 0 = `(0, 0)` |
| A4 forward (true) → `performRealOnlyInverseTransform` | == x to 4.1e-7: **the inverse carries 1/N**; floats [128, 256) are zeroed on return |
| A5 garbage (1234.5) written into bins 65…127 before the inverse | == x to 4.1e-7: **the inverse reads bins 0…64 only** (Apple repacks the Nyquist real into DC's imaginary slot itself; the Fallback rebuilds the conjugate half from the first half) |
| A6 `IFFT (X ⊙ H)[64…127]` with the complex MAC over bins 0…64 only | == linear convolution of `[prev | cur]` with the 64-tap `h` to 3.7e-7 |

So the executor's complex MAC runs over **65 bins (130 floats) per spectrum**, the spectra store 130 floats each (not 256), the Nyquist bin is an ordinary complex entry with imaginary 0, and no explicit 1/N or 0.5 appears anywhere. The flag `onlyCalculateNonNegativeFrequencies = true` is correct for every forward transform here.

**UPOLS scratch gate** (`r6_upols_scratch.cpp` Parts B and D — the structure the executor transcribes into `BinauralConvolver.h`; the scratch keeps `std::vector` tables and a 3-bank table so its bookkeeping can be read as the spec):

| Check (random 16-ch bank, 0.5-rms white input, 8256 samples) | Result |
|---|---|
| taps 256 (K 4) vs direct-form double, host blocks 32 / 64 / 100 / 1024 / 4096 | **max \|err\| 7.13e-7** at every block size (ref peak 3.80), `out[0…63] == 0` |
| taps 512 (K 8), same five block sizes | **8.82e-7** (ref peak 3.62) |
| ragged slicing {1, 7, 100, 4096, 33} vs 64 | **bit-equal**, both K |
| dual-bank fade A→B requested at 1024, C queued at +7 grids, A queued at +9 (latest wins), C at 4096 (idle → immediate), blocks 64 and 1024 | **8.06e-7** vs the direct-form fade in double *with a float ramp* (F1); final state `cur = C, idle`; liveness \|L − A\| 3.5, \|L − C\| 3.4 mid-fade |
| negative control: snapped A→B vs the faded reference | 3.16 ≫ 1e-5, fails as expected |
| throughput, Release, M4 Max, scalar loops: 480 000 samples at K 4 / K 8 | **273× real time (48 k)** / **91× (96 k audio seconds)** — convolver alone |

Margin to the 1e-5 gate: 14× at a 3.8 peak, i.e. ≈ 50× for unit-peak signals. The two negative-frequency facts (A5) mean the Fallback engine on Windows runs the identical bookkeeping — only roundoff differs (D6: digest-2.3 is informational).

**The step, as validated** (per-grid; `P = 64`, `NB = 65`, spectra `H[set][ch][ear][k]` 130 floats, FDL `[ch][slot]` 130 floats, `prev[ch][64]`, `in[ch][64]`, `out[ear][64]`, `outB[ear][64]`):

```
step():                                   // from controlStep(), AFTER requestSet, BEFORE the grid's first sample
  prevMix = curMix; if (fading) curMix = bankMix.step (1, kRampStep)        // float LinearRamp, 0 → 1
  head = (head + K − 1) mod K                                               // newest FDL slot
  for ch < 16:  tmp = [prev[ch] | in[ch] | 0 × 128];  prev[ch] = in[ch]
                FFT.performRealOnlyForwardTransform (tmp, true);  FDL[ch][head] = tmp[0..130)
  accumulate (set, dst):  dst[ear] = 0 (130 floats);  for ch < (set+1)², k < K:  X = FDL[ch][(head + k) mod K]
                          dst[ear][2b] += Xr·Hr − Xi·Hi;  dst[ear][2b+1] += Xr·Hi + Xi·Hr   (b < 65, both ears)
                          performRealOnlyInverseTransform (dst[ear]);  copy dst[ear][64..128) → out/outB[ear]
  accumulate (cur, acc) → out;   if (fading) accumulate (pending, accB) → outB
```

`in[ch][pos]` is written by `push (tap16, gridPos + j)` during the grid and consumed by the next `step()`; `out` written at step *s* is read during grid *s* — 64 samples of structural latency, `out[0…63] == 0` on the first grid (R6 table). The accumulate loop over `(set+1)²` channels is D4's order-1 saving; the 16 input FFTs always run.

### R7 — Click stimulus: a 100 Hz tone at a SIDE source (az +90°); the snapped negative control is predicted at 240–345× and doubles as the liveness proof; spectral liveness on a 3 kHz partial (R ear +6.2 dB between orders 1 and 3); a front source would be nearly blind to the bank change

Through the level-matched banks at az +90°, el 0:

| 100 Hz | Order 1 | Order 3 | ΔH |
|---|---|---|---|
| \|H\| L / R | 0.7844 / 0.5826 | 0.8055 / 0.6164 | 0.033 / 0.037 |
| phase L / R | −44.1° / −72.1° | −45.9° / −73.4° | 1.8° / 1.3° |

A snapped bank switch on a 0.5-amplitude 100 Hz tone steps the output by up to `0.5·|ΔH|` = 1.6e-2 (L) / 1.8e-2 (R) while the static curvature is `0.5·|H|·ω²` = 6.9e-5 / 5.3e-5 → **predicted negative-control ratio ≈ 239 (L) / 345 (R)** against the ≤ 2.0 ceiling — the LS region differs between orders enough for the second-difference metric to see a snapped switch by two orders of magnitude, so `checkFailsAsExpected` on the snapped ReferenceModel is itself the proof that the switch is a live event at this stimulus (a vacuous gate would show the negative control *passing*). The faded switch is a 20 ms linear blend between two sinusoids 0.3 dB and 1.5° apart: expected ratio ≈ 1.0 against the same-motion reference, ≈ 1.0 against the static endpoints too (no step), both printed.

Why not a two-tone or a front source: a 3 kHz partial of amplitude 0.25 carries its own curvature `0.25·(2π·3000/48000)²` = 9.6e-3 per sample², which would bury a 1.6e-2 step at a ratio ≈ 1.4 — the click metric needs the low tone alone. A front source sees the banks differ by only −0.22 / +1.5 / +0.2 / −0.5 dB at 100 / 1000 / 3000 / 6000 Hz (orders 1 vs 3), and the LF difference at the front is at the metric's floor. **Separate spectral liveness:** two short static renders at az +90° of a 3 kHz tone, exact-bin DFT (`binMagnitude`): order 1 L 1.167 / **R 0.212**, order 3 L 0.903 / **R 0.435** → R ear +6.2 dB (assert ≥ 3 dB), L −2.2 dB. For Binaural → Stereo → Binaural the same 100 Hz side stimulus works (Stereo R = 0.067 vs Binaural R ≈ 0.6 — the metric's event is large) and the FUNC-05 **level delta is measured at the front** (where D2 defines the match) as A-weighted broadband level of two static renders of white noise, |Δ| < 1 dB (generator says 0.00 dB at order 3).

Gate layout for the plan: processor-level `renderHooked` at 64 and 1024 blocks (the hook sets `order` / `decoder` at a block boundary), skip = 64 + 20 ms + 256 taps, event window one full fade plus a margin; controls = same-motion `ReferenceModel` with the float bank ramp (F1), negative control = the model with `snapBankFade = true`; both static endpoints printed.

### R8 — ILD through the order-3 bank at ±90°: ±13.9 / −14.6 dB broadband (≥ 6 dB with 8 dB to spare), sign left-positive; ITD must be measured by low-passed cross-correlation (37 samples at order 3), never by an onset threshold (which reads the wrong sign through a MagLS bank)

Through the truncated, level-matched banks (az ±90°, el 0):

| Order | Broadband ILD +90 / −90 | Per band +90 (200–1 k / 1–3 k / 3–8 k / 8–16 k) | ITD: LP < 300 / 500 / 800 / 1500 Hz xcorr | Interaural group delay, median 100–600 Hz |
|---|---|---|---|---|
| 1 | +12.3 / −11.9 dB | +4.7 / +11.4 / +15.8 / +12.6 | 39 / **36** / 27 / 15 | 29.1 samples |
| 2 | +13.8 / −14.0 | +4.8 / +8.6 / +14.9 / +16.7 | 42 / 42 / 40 / **32** | 39.3 |
| 3 | **+13.9 / −14.6** | +5.1 / +6.2 / +17.3 / +20.0 | 42 / 43 / 41 / **37** | 39.3 |

(Left leads at +90° in every cross-correlation reading; the sign is the AmbiX one.) Two methods are **invalid** here and must stay out of the harness and the measurer: (i) the 10 %-of-peak onset difference reads **R leading by 21 samples at +90°, order 3** — the contralateral response has a lower peak, so its 10 % threshold falls into the −40 dB pre-ringing (R3); on raw SADIE HRIRs the same method gave the correct 31 samples because raw HRIRs have no pre-ringing; (ii) broadband cross-correlation reads 0–3 samples (dominated by HF, where MagLS carries no ITD by design).

**Method for both the harness and `verify_2_3.py`:** cross-correlate the two ears after a brick-wall low-pass at **`min (1500 Hz, 0.8·f_a (order))`** on a 4096-point FFT (order 1: 499 Hz → 36; order 2: 998 Hz → ≈ 38; order 3: 1497 Hz → 37), lag of the maximum; **window 25–45 samples at 48 k** (scaled by `fs/48000` elsewhere), L leading at +90°, R at −90°. The KU100's low-frequency ITD at 90° is ≈ 0.78 ms = 37 samples (Woodworth `3r·sinθ/c` = 36.7), so the readings are physically right; ARCHITECTURE's "20–40 samples" and REQUIREMENTS' "onset lead" are read as this measurement. ILD gate: broadband rms ratio ≥ 6 dB with the correct sign at orders 1 and 3 (12.3 / 13.9 dB measured), per-band values printed.

### R9 — PERF-02: a `steady_clock` probe over 10 s of audio at 64-sample host blocks through the processor, best of three, asserted ≥ 20× only under `NDEBUG`, printed otherwise, never ANDed; expected ≥ 150× on this machine

Method: `makeProc (2, 64)` at order 3 / Binaural / az 37, el −12, d 3.3, yaw 25 (the digest scenario plus the convolver live), white noise; 1 s warm-up excluded; then `std::chrono::steady_clock` around exactly 480 000 samples of `processBlock` at 64-sample blocks, repeated three times, **best (minimum) wall time** → `ratio = 10 s / wall` (a one-sided "can it" bound, so the best run is the right statistic — `pattern_wallclock_inside_a_stability_verdict`, `pattern_one_shot_latency_ratio_does_not_cancel_runner_load`). Under `#ifdef NDEBUG` (CMake's Release flags define it; the harness builds in the same Release tree) `check ("PERF-02 ≥ 20× real time …", ratio ≥ 20)`; otherwise an `info:` line "debug build — PERF-02 printed, not asserted". The probe prints 1024-sample blocks and 96 k (K 8) as information. Nothing else reads the timer. Expected: the scratch convolver alone runs 273× (48 k) / 91× (96 k) with scalar loops; the rest of the engine is ≈ 20 MFLOPS → ≥ 150× at 48 k, ≈ 70× at 96 k. A pathology (per-sample FFT, allocation storm, a K-sized loop per sample) drops below 20× by an order of magnitude; a 10 % regression does not register, as D11 intends. Keep the probe last in `main()` so its wall time cannot be confused with any other output, and keep `rtcheck` disarmed during it.

### R10 — Shared control-step extension: arrival → sample `order` → `requestSet` → convolver step, all inside `controlStep()`; the reference runs the same machine with a float ramp; band ramps and the bank fade start on the same step and arrive within one grid of each other; `decoder` and `order` on the same step do not interact

The PLAN-2.1 / 2.2 spec extends as follows (op-for-op between `SpatialEngine` and `ReferenceModel`):

```
snapToParams():                              (prepare / reset — after the 2.2 snaps)
  conv.reset();  conv.cur = params.order;  conv.pending = −1;  conv.fading = false;  conv.queued = −1;  bankMix.snapTo (0)
  (prepare only: conv.prepare (fs) — spectra for the three sets, resampled + DC-calibrated when fs ≠ 48 k (R5), FDL for K(fs))

controlStep():                               (the 2.1 / 2.2 steps as now, then:)
  conv.arrive():   if (fading && curMix == 1):  cur = pending; pending = −1; fading = false; curMix = prevMix = 0
                                                 if (queued ≥ 0): q = queued; queued = −1; requestSet (q)
  conv.requestSet (params.order):   if (fading): queued = n                    // latest wins, even n == cur
                                     else if (n != cur): pending = n; fading = true; bankMix.snapTo (0); prevMix = curMix = 0
  conv.step()                                   (R6 block: ramp, 16 FFTs, accumulate cur [+ pending], 2 [+ 2] IFFTs)
  binauralOnly = (mixer.prevMix == 1 && mixer.curMix == 1)                     // D3, one bool per grid
  frame: as 2.2 (+ nothing — no new MeterFrame field)

renderSample (x, t):
  … bus → tap (2.2) → shPeak
  conv.push (tap, gridPos + j)                                                  // this grid's input, consumed by the NEXT step
  binL/R = conv.fading ? out[ear][p] + m·(outB[ear][p] − out[ear][p]) : out[ear][p],   p = gridPos + j,
           m = prevMix + t·(curMix − prevMix)                                    // A3 form, float
  stereo decode → delayStereo (unchanged)
  binauralOnly ? (outL = binL, outR = binR) : mixer.blend (dL, dR, binL, binR, t, outL, outR)
```

Order inside `controlStep()` matters: **arrive before requestSet before step** — if arrival were checked inside `step()`, a request sampled on the arrival step would see `fading == true` and be queued for nothing. `push` and the read of `out` are independent buffers, so their order within `renderSample` is free. `endOfGrid` in the scratch (`gridPos == 64 → 0`) is the same instant as `arrive()` at the top of the next `controlStep()`.

Ramp arrivals at 48 k (float `LinearRamp`, step 1/15): 0 → 1 arrives at step **15**, 1 → 0 at step **16** (descending float rounding), both 20 ms ± one grid. The bank fade is always 0 → 1 (15 steps); the band ramps `orderGain` go 0 → 1 on an order increase (15, same step as the bank fade) and 1 → 0 on a decrease (16 — one grid after the bank fade arrives). Both are fades, not steps, and the one-grid offset is invisible to every gate (the engine output during the overlap is a blend of two blends); recorded so nobody "fixes" it. At 44.1 k both arrive at 14, at 96 k at 30 (double: 31 — F1). `decoder` and `order` changing on the same step: the mixer ramp and the bank fade are independent `LinearRamp`s on independent legs; the convolver fades regardless of `decoder` (D12); no interaction, and the PERF-01 motion set already exercises the pair.

**ReferenceModel:** gains `double h[3][16][2][256]` from the header (widened), a direct-form convolution of *its own* 16-channel `tap` (delayed by exactly 64 via its existing ring idea, extended to 256 + 64), the state machine above, and **a float `Ramp` for `bankMix` only** (F1) — `og[3]` and `mix` stay double so every 2.1 / 2.2 reference literal is untouched. At `fs ≠ 48 k` it reads `conv.filterTaps (set, ch, ear)` (D7). A `snapBankFade` switch (negative control) joins `snapCoefficientsPerGrid` / `snapOrderGains`.

---

## Findings beyond R1–R10

### F1 — A float `LinearRamp` and its double twin arrive one grid apart: 15 vs 16 steps at 48 k (30 vs 31 at 96 k); any null spanning a fade needs the reference ramp in float *(HIGH — the scratch fade null read 0.25 before, 8e-7 after)*

`cur += 1/15` fifteen times in float32 lands within one step of 1 and snaps; in double it needs sixteen. The existing `ReferenceModel::Ramp` is double, which has never mattered because no 2.1 / 2.2 null spans a fade (click gates are ratios). The 2.3 fade nulls (bank fade vs direct-form fade, D7 during fades) *do* span one: the reference's bank ramp must be `float` arithmetic (`r6_upols_scratch.cpp` `refFade`, after the fix). Keep `og[3]` / `mix` double in the reference (R10) so no recorded 2.1 / 2.2 literal moves.

### F2 — `juce::Interpolators::WindowedSinc` has a DC gain of 0.990 (−0.087 dB), flat, at every ratio including 1.0 *(MEDIUM — drives the D5 amendment)*

See R5. Without the calibration the resampled banks at 44.1 / 88.2 / 96 / 176.4 / 192 k would sit 0.087 dB below the 48 k bank — inaudible, but it would break the "same level at every rate" reading of FUNC-05 and the golden gate would need a 1.5 % tolerance that hides real errors. One delta resample in `prepare` fixes it exactly.

### F3 — spaudiopy needs `scipy < 1.15`; the generator's oracle call must be pinned, and the tap-level null is only meaningful with spaudiopy's slice-then-diff Δφ estimator *(MEDIUM — generator tooling)*

See R1 / R2. `FILTERS.md` records the resolved versions; the generator's `--with` list pins `scipy<1.15` and the exact spaudiopy / sofar versions the header was produced with. The oracle call sits in the self-check section and fails the run (not skips) if the import fails.

### F4 — MagLS filters are not unique; compare on the objective, keep the header as the artifact *(MEDIUM — gate design)*

See R2. Consequence for the success criterion "regenerates byte-identical": true on this machine with the pinned environment (the generator is deterministic — same BLAS, same order of operations); on another machine the check is the objective table and the oracle null, not bytes.

### F5 — Onset-threshold ITD is wrong through a MagLS bank; broadband cross-correlation is blind to it; low-passed cross-correlation is the method *(MEDIUM — gate design)*

See R8. The pre-ringing (R3) is why: the contralateral ear's 10 % threshold lands in it.

### F6 — "First nonzero output sample" cannot be the DSP-03 gate: the banks are nonzero (−60 dB) from tap 0; the gate is a 10 %-of-peak onset against generator-emitted per-order constants *(MEDIUM — gate definition)*

See R3. With `T = 32`, `kBinauralOnsetFrontSamples = {0, 84, 70, 50}` (left ear, front); the gate asserts `onset₁₀% == 64 + const ± 2` per order at 48 k and the scaled value at 44.1 / 96 k; the structural 64 is proven by the D7 null (any grid slip reads ≫ 1e-5) and by P4 in Stereo mode (unchanged, exactly 64).

### F7 — Headroom: the order-3 bank peaks at +6.5 dB (9.6 kHz, worst direction) after the level match *(LOW — note for D2 and Stage 4)*

See R3. Stage 4 presets should not sit near 0 dBFS in Binaural mode; `outputLevel` exists.

### F8 — The deep-dive's complex half-octave blend cancels (27 dB dips at order 3) — the contract's "half-octave blend" wording must not be transcribed literally *(HIGH — correctness of the generator)*

See R2. Hard switch with phase continuation recommended; the target-domain blend is the safe alternative if a blend is insisted upon, at a measured accuracy cost.

### F9 — Header emission must round-trip float32 exactly: `%.9g` (9 significant digits) and a parse-back self-check *(LOW — generator detail)*

O-GrainScatter's `{val: .8e}f` happens to give 9 significant digits too; make it explicit: emit `np.float32` values with `'%.9g'`, re-parse the written header with the same regex the pedalboard measurer will use, and assert bit-equality with the float32 arrays before declaring success (the header is the artifact, D8).

---

## Reuse — existing code and precedent

| Need | Reuse | Notes |
|---|---|---|
| SH conventions at the SOFA directions | generic triple product (harness `refSh16`, O-GrainScatter `real_sh`) | the generator's own numpy version is in `r2d_final.py` `sn3d_matrix` — verified against spaudiopy's N3D up to the per-order factor (R2) |
| Header emission shape | O-GrainScatter `scripts/generate_hrir_header.py:318-358` | nested `constexpr float` arrays; add provenance constants, `%.9g`, parse-back (F9) |
| Direct-form reference loop | O-GrainScatter `BinauralDecoder.h` (ring + 256-tap MAC) | the D7 reference in double, per channel, +64 delay |
| Fades | `LinearRamp` (`Smoothers.h`) with `kRampStep` | bank fade = the same type; reference twin in float (F1) |
| Decoder mixer | `DecoderMixer::delayStereo` / `blend`, `prevMix` / `curMix` | expose `bothEndpointsAtOne()` for D3's `binauralOnly` |
| FFT / resampler | `juce::dsp::FFT (7)`, `juce::Interpolators::WindowedSinc` | contract in R6, calibration in R5; both constructed / run in `prepare` only; `FFT` objects are allocation-free in `perform*` (PERF-01 re-run is the proof) |
| Harness drivers and metrics | `renderHooked` / `renderEngine` / `renderRef`, `worstCurvature`, `ClickReading`, `checkFailsAsExpected`, `binMagnitude`, `energy`, rtcheck | no new driver; one helper to low-pass + cross-correlate (R8) and one to A-weight a spectrum (R4) |
| Validated bookkeeping | `research-2.3/r6_upols_scratch.cpp` (`Upols`, `Driver`, `refFade`) | transcribe; the scratch's `std::vector` tables become fixed-size members sized in `prepare` |
| Pedalboard measurer | `tests/pedalboard/verify_2_2.py` (`setp` by `raw_value`, `fresh()`, compensated index 100) | `verify_2_3.py` adds a header parser, numpy convolution, the R8 ITD method |

---

## Pitfalls carried from the knowledge base

- `pattern_wallclock_inside_a_stability_verdict`, `pattern_one_shot_latency_ratio_does_not_cancel_runner_load` → R9: timing printed, asserted alone under `NDEBUG`, best of three, never ANDed.
- `critical_juce_oversampling_4x_fir_latency_is_fractional` → R5: the resampler's latency is fractional at 44.1 k; the 60-sample prefix makes it integer.
- `pattern_test_fixture_mirrors_drift_silently` → the reference convolves with the header floats directly (the artifact), never with a re-declared copy; the pedalboard measurer parses the same header.
- `pattern_golden_tracked_as_checksum_only` → the header *is* the golden; `FILTERS.md` carries sha256s as provenance, not as the artifact.
- `pattern_gate_stimulus_below_threshold_is_vacuous` → R7: the snapped negative control is the liveness proof; a front source would make the bank gate near-vacuous.
- `pattern_step_click_gate_needs_same_motion_control` (2.2) → R7: verdict vs the same-motion reference, static endpoints printed.
- `pattern_oversampler_overruns_on_oversized_host_block` → the convolver holds no host-sized buffer (D1); the processor slice loop is unchanged; the oversized-block gate extends to Binaural mode.
- `pattern_verify_installed_binary_with_pedalboard_via_uv`, `pattern_vst3_raw_value_is_cached_unsnapped_host_value_gate_roundtrip_on_decoded` → D14 as planned, `raw_value` for every numeric parameter.
- `pattern_executor_watchdog_stall_run_long_commands_in_background` → the generator (≈ 10 s), the harness (now longer with the PERF-02 probe) and `build-and-install` run in the background with logs.
- `project_no_unit_test_framework_ci_never_runs_tests` → the harness is a dev-machine gate; digest-2.3 is informational (D6).
- `index_git_shared_checkout` → temp-index, path-scoped commits (`plugins/O-simpleAmbisonics`, `PLUGINS.md` only at the roll-up).

---

## Open items handed to the plan phase

1. **Trim:** adopt `kBinauralFilterTrimSamples = 32` (R3) — or `T = 0`; both are one constant in the generator. Recommendation: 32.
2. **Blend:** hard switch with phase continuation (R2 / F8) — a wording deviation from ARCHITECTURE / ROADMAP "half-octave blend", justified by the 27.6 dB cancellation of the literal form and the measured accuracy cost of the safe form. **Taylor's call at plan time**; the plan should quote the numbers.
3. **D5 amendment:** delta-resample DC calibration in `prepare` (R5 / F2), `resamplerDcGain()` accessor, golden tolerances 6e-3 / 1.5e-2 of peak and 0.1 dB below 16 kHz, `zpre` rule, K(fs) sizing up to 16.
4. **D9 amendment:** per-order `kBinauralOnsetFrontSamples[4]` instead of one guard; DSP-03 onset gate as F6.
5. **Reference ramp in float** for the bank fade only (F1 / R10); `snapBankFade` negative-control switch.
6. **Control-step order** `arrive → requestSet → step` (R10); `binauralOnly` as one bool per grid (D3).
7. **Generator pins and self-checks:** `uv run --python 3.12 --with "sofar==<r>,numpy==<r>,scipy<1.15,spaudiopy==<r>"` recorded in `FILTERS.md`; self-checks = sha256s, oracle null ≤ 1e-9 + objective table, level match (order 3 < 1e-6 dB, orders 1 / 2 within ±1 dB — expect −0.37 / −0.03), W 50–200 Hz ±1 dB, ILD ≥ 6 dB with sign at ±90° (expect 13.9 / 14.6), ITD by the R8 method 25–45 (expect 37), no NaN, peak gain report (expect +6.5 dB), header parse-back bit-equal (F9), trim / onset constants printed.
8. **Header constants:** `kBinauralFilterSampleRate = 48000`, `kBinauralFilterTaps = 256`, `kMagLsFilters1[4][2][256]` / `2[9]` / `3[16]`, `kBinauralFilterTrimSamples`, `kBinauralOnsetFrontSamples[4]`, `kBinauralLevelMatchDb`, `kBinauralAliasingHz[4] = {0, 623.9, 1247.8, 1871.7}`, `kBinauralFilterSource` / `License` / `Sha256` strings, `kBinauralFilterDfe = true`; `kStereoLevelMatch` stays `1.0f` with the D2 comment.
9. **PERF-02 probe** per R9, last in `main()`.
10. **ITD method** per R8 in both measurers; ILD broadband ≥ 6 dB with sign.
11. **Click gates** per R7: 100 Hz at az +90° for the ratio, 3 kHz at az +90° for spectral liveness, front white noise for the FUNC-05 level delta.
12. **Goldens header:** 44.1 k and 96 k, `scipy.signal.resample_poly` defaults, test-only `tests/render-harness/BinauralFilterGoldens.h`.
13. **Research scratch** lives in `stages/2-dsp/research-2.3/`; the executor transcribes `Upols` from `r6_upols_scratch.cpp` and the SH / MagLS functions from `r2d_final.py`.
14. **Correction to CONTEXT-2.3:** spaudiopy's default `f_trans` is `N·600`, not `N·500` (no effect — `f_a` is passed explicitly).

## Still-open manual gates

Unchanged from CONTEXT-2.3 §Success criteria (hands-on at verify, non-blocking): headphones at order 3 — front / back / left / right / overhead distinct; +90° louder LEFT; image sharpens 1 → 3; order and decoder switches click-free mid-tone; no loudness jump on the decoder switch (expect 0.0 dB at the front, −0.4 dB at order 1).
