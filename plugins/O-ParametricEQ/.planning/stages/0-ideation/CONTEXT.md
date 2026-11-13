# O-ParametricEQ - Stage 0 Context (Research and Planning)

**Date:** 2026-11-12
**Outputs:** `research/ARCHITECTURE.md`, `ROADMAP.md`, this file, `STATUS.md`
**Complexity:** 5.0 (raw 11.0), phased. Tier 6 (real-time analysis and visualization on top of a custom filter engine), DEEP research.

## What Stage 0 did

- Read the brief, the 27 requirements, the draft parameter spec and `research/parametric-eq-filter-design.md`.
- Read three Vicanek notes in full (2016 matched biquads, 2019 one-pole shelves, 2024 two-pole shelves).
- Built a float64 numerical model of nine candidate designs and compared each with the analog prototype at 44.1, 48 and 96 kHz. Every accuracy number in ARCHITECTURE.md marked [MODEL] comes from it. It is a model, not a measurement of the plugin.
- Checked JUCE facts against the local 8.0.15 headers.

## Decisions

| # | Topic | Decision | Why |
|---|-------|----------|-----|
| 1 | Bell and shelf design | Matched pole pair and matched zero pair (impulse invariance, angle clamped at π) plus a first-order corrector: DC and Nyquist gain exact ("matched low-pass ratio", MLR). | Model: bell within 0.36 / 0.66 / 1.05 dB at ±6 / ±12 / ±30 dB for fc ≤ 20 kHz at 44.1 kHz, against 0.79 / 1.44 / 2.56 dB for Vicanek's bell alone and 13 - 17 dB for the RBJ bilinear bell. Always feasible, always stable, cut is the exact inverse of boost. |
| 2 | Cuts | High cut: matched poles, numerator matched at DC, at min(fc, 0.75 Nyquist) and at Nyquist. Low cut: Vicanek's matched high-pass (double zero at DC) with the match point capped at 0.9 Nyquist. | Corner gain exact; low cut slope exact by construction. |
| 3 | First-order shelf | Vicanek 2019, used as published. | Re-verified: within 0.17 / 0.31 / 0.49 dB for any corner from 10 Hz to 30 kHz at 44.1 kHz. |
| 4 | Realisation | Trapezoidal SVF (Simper form) carrying `(g, k, mL, mB, mH)`, plus a TPT one-pole. Double state. No biquad coefficients anywhere. | The map from a stable biquad to `(g > 0, k > 0)` is a bijection, so linear interpolation of the parameters is always a stable filter. `mL` and `mH` are the DC and Nyquist gains. |
| 5 | Smoothing | 20 ms ramps on log frequency, dB gain, log Q; design on a global 16-sample grid (32 / 64 at higher rates); per-sample linear interpolation of the section parameters between grid points. | Block-size invariant; only moving bands pay. |
| 6 | Structural changes | Two slots per band, 20 ms raised-cosine crossfade for type, slope, placement, enable, band bypass; latest request latched. Gain-bearing bands at exactly 0 dB fall asleep after 50 ms. | A filter family switch on live state clicks whether or not the state is reset (measured in the suite). Sleep makes "all bands at 0 dB" bit-transparent. |
| 7 | Slope | Order = slope / 6 for Low Cut, High Cut, Low Shelf, High Shelf, Tilt (Butterworth families). Q is a resonance factor on the highest-Q section; Q 1.0 = Butterworth. Bell, Notch, Band Pass ignore slope in v1. | See "Needs the user's call" 1. |
| 8 | Placement | Serial in band order on L/R. Mid / Side bands filter `m` or `s` and add only the difference to L and R. Inactive bands are skipped. | Untouched channels and the all-off case are `memcmp`-equal. |
| 9 | Display | The message thread runs the same designer and pushes section parameters; the page evaluates two closed-form responses and composes 2x2 matrices. | Same code, same values, same sample rate as the audio; works with the transport stopped; nothing crosses from the audio thread. |
| 10 | Analyzer | Audio thread writes mono sums into two `AbstractFifo` rings; FFT 4096 / 8192 / 16384 by rate on a 30 Hz message-thread timer; 256 display points. Plugin-local (no shared module exists). | Suite precedent (O-simpleFM). |
| 11 | Auto gain | Negative of the pink-weighted mean power gain of the target curve, 64 points 40 Hz - 16 kHz, clamp -18 / +12 dB, 100 ms ramp, computed on the audio thread. | Deterministic, null-test friendly, identical offline and real time. |
| 12 | Dynamic-EQ room | 72 parameters not registered; IDs and version hint 2 reserved; `stateVersion`; `dynOffsetDb` hook in the band. | JUCE sorts AU parameters by version hint then ID hash, and VST3 IDs are ID hashes, so adding hint-2 parameters later moves no v1 parameter. |
| 13 | Parameters | 196 host parameters; global block first, then `b01_` … `b24_`; frequency skew `setSkewForCentre (1000)`, Q `setSkewForCentre (1.0)`; analyzer mode is UI state; global bypass is the host bypass parameter. | Answers to the six draft questions are in ARCHITECTURE.md. |
| 14 | Thresholds | DSP-01: bell `0.15 dB + 5 %` of the gain in dB; 12 dB/oct shelf (Q ≤ 1) `0.15 dB + 6 %`; 6 dB/oct shelf `0.10 dB + 2 %`; at 88.2 kHz and above `0.10 dB + 1.5 %`. DSP-05: residual ≤ -180 dBFS RMS in the double engine (model -234), ≤ -140 dBFS at the float output. PERF-02: ≤ 10 % of one core for 24 cuts at 96 dB/oct, stereo, 48 kHz. | Each sits above the model's worst case with margin and is failed by the bilinear control. **Amended 2026-11-13 (Stage 2 D7, `stages/2-dsp/PLAN-2.1b.md` P1):** the DSP-01 figures here bind in Phase 2.1 only. From Phase 2.1b: bell, 12 dB/oct shelf (Q ≤ 1.41) and 24–96 dB/oct shelves (Q 1) `0.05 dB + 1.2 %` at 44.1 / 48 kHz and `0.03 dB + 0.2 %` at 88.2 kHz and above; 6 dB/oct shelf unchanged. |
| 15 | SIMD | Deferred. | Scalar model is 4.6 % for the PERF-02 case. |

## Alternatives rejected, with the evidence

| Alternative | Result in the model (44.1 kHz) |
|-------------|--------------------------------|
| RBJ pre-warped bilinear | 13 - 17 dB off at ±30 dB near Nyquist. The cramping the brief rules out. |
| `juce::dsp::StateVariableTPTFilter` as the repo research doc recommends | Same bilinear magnitude; fails DSP-01. |
| Vicanek 2016 peaking filter alone | Good at high Q; 0.6 - 0.8 dB off at ±6 dB for mid Q above 10 kHz because the Nyquist end is unconstrained. No general-Q shelf. Kept as the fallback. |
| Orfanidis 1997 | Not better where it is defined; undefined when the bandwidth reaches Nyquist (wide bells, high centres). |
| Matched-Z, DC-normalised | Up to 3 dB off at ±6 dB above 18 kHz. |
| Numerator fitted at DC, centre and Nyquist on matched poles | Negative `mB²` (no real solution) for most cuts and for high shelves. |
| Five-point linear solve of all coefficients | Infeasible bands and 5 - 25 dB errors on in-band peaks; excellent only above Nyquist. |
| Separate three-point fits of numerator and denominator | Infeasible for low Q, 3 - 16 dB errors. |
| Gauss-Newton polish of the SVF parameters | 2 - 5x better than the closed form where it converged; diverged at low Q with a simple match grid. Left as v1.x accuracy backlog behind the same designer interface. |
| Oversampling, FIR correction | Excluded by the brief (latency). |
| Direct-form biquads with smoothed coefficients | Interpolated coefficient sets are not guaranteed stable; suite measurements of clicks and near-Nyquist blow-ups. |
| Registering the dynamic-EQ parameters now | Freezes ranges that have not been designed. |
| Audio thread publishes coefficients for the display | Stale when the host stops calling `processBlock`. |
| Measured-loudness auto gain | Programme dependent, pumps, breaks null tests. |

## Findings worth keeping

- The best single biquad the fit could find is about 0.08 / 0.16 / 0.37 dB off at ±6 / ±12 / ±30 dB (24 sampled settings). MLR is about 3 - 4.5x above that; Vicanek's closed-form bell is 7 - 10x above it. The remaining gap is the v1.x polish.
- The accuracy problem is always the same one: a resonance near or beyond Nyquist. Non-resonant shapes are easy.
- Low-frequency numerics are decided by how `1 + a1 + a2` is formed, not by float vs double. With `expm1` and `sin²` forms the 10 Hz, Q 40 case at 192 kHz is exact to 1e-13 dB.
- The round-off floor of the SVF section in double for that case is about -234 dBFS; in float it is about -153 dBFS. Double state is necessary and sufficient.
- JUCE AUv2 parameter order is version hint, then ID hash. Registration order only controls what VST3 hosts list.

## Unverified

- Wishnick (DAFx-14) proof of time-varying SVF stability: abstract only. Gates G3 and G5 carry the claim.
- The MLR gain section is a Stage 0 composition; no published source states it. Phase 2.1 must reproduce the model in C++.
- Vicanek 2024's "under 1 dB for a 20 dB shelf" (fallback design) was not re-measured.
- Massberg 2011 and the Orfanidis original were not read; Orfanidis was evaluated from Vicanek's restatement.
- Pro-Q behaviour (slopes on every shape, Q 1 = Butterworth on cuts, tilt convention) was not checked against a manual.
- CPU figures are cost-model targets.
- 196 WebView relays have not been tried at this scale.
- `vu-meter` 1.0.0 has no consumer listed in the registry.
- Notch above 0.95 Nyquist, band pass above 0.9 Nyquist and band pass accuracy above 10 kHz were not characterised.

## Needs the user's call before Stage 1

1. **Slope on Bell, Notch and Band Pass.** v1 as planned ignores slope on these three (classic second-order shapes) and applies 6 - 96 dB/oct to cuts, shelves and tilt. Recommended. The alternative (higher-order band shapes) adds a pole-transformation step and lands in the regime where the model is weakest; it can follow in v1.x without breaking sessions.
2. **Accuracy level for v1.** The tolerance above (about 5 - 6 % of the gain in dB at 44.1 kHz, about 1.5 % from 88.2 kHz up) is what the closed form reaches. Known weak spots: resonant shelves (Q above 1.41) and shelves of 24 dB/oct and steeper with the corner above 10 kHz at 44.1 / 48 kHz. Recommended: ship v1 at this level and treat the numerical polish as v1.x. Say so if v1 should include it; that adds a research phase before 2.2.
3. **Analyzer mode as UI state, not a host parameter** (196 parameters instead of 197). Recommended.
4. **Tilt convention.** Gain g gives -g/2 below and +g/2 above the pivot (total swing g). The alternative is ±g (total 2g, up to 120 dB with gain scale at 200 %).
5. **Q on cuts and shelves.** Q 1.0 = Butterworth (flat), Q above 1 adds a resonance at the corner, at every slope. The default Q of 1.0 then gives a clean cut.
6. **Zero-padded IDs** `b01_freq` … `b24_freq` instead of the draft's `b1_freq`. IDs are frozen at Stage 1.
7. **Above-Nyquist behaviour at 44.1 / 48 kHz.** Bells above 0.95 Nyquist keep their in-band skirt (re-centred design); notch and band pass centres clamp at 0.95 and 0.9 Nyquist.
8. **Listen filter definition** (band-pass for bell / notch / band pass, low-pass or high-pass region for shelves and cuts). Affects Stage 2.6 and the mockup.

None of these blocks the mockup; 3 and 6 change what the mockup must write into `parameter-spec.md`.

## Constraints for later stages

- ARCHITECTURE.md "Parameter Mapping" is the layout the mockup must adopt.
- Phase 2.1 (designer plus offline gate) comes before any processor code.
- No always-on stage in the signal path: every ramp snaps to its target and every unity stage is skipped, or the `memcmp` gates fail.
- The control grid is global and independent of the host block size.
- Hard controls must fire before a click row counts.
