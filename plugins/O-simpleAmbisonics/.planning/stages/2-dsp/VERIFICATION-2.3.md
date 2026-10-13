# Stage 2: DSP — Phase 2.3 (binaural: MagLS filter pipeline + UPOLS engine + crossfades + CPU / latency gates) - Verification

## Verification Date

2026-10-13

## Method

Goal-backward against `CONTEXT-2.3.md` §Phase goal and §Success criteria, `PLAN-2.3.md` §Success criteria and ROADMAP §Phase 2.3. Every automated claim in `SUMMARY-2.3.md` was **re-run independently** in this verify session; nothing was taken on SUMMARY's word.

- Plugin tree clean at HEAD `fda8deab` (the 2.3 execute commit). The installed bundles `~/Library/Audio/Plug-Ins/{VST3,Components}/O-simpleAmbisonics-dev.*` are **byte-identical** (`cmp`) to the build artefacts — before and after the forced rebuild below.
- `BinauralConvolver.h` and `SpatialEngine.h` touched and the four targets (`VST3`, `AU`, `Standalone`, `render-test`) rebuilt under `juce_recommended_warning_flags`: exit 0, **0 warnings** (`grep -c warning:` on the full log, 3 TUs recompiled).
- Generator re-run from the cached, sha256-pinned SOFA (`uv run --python 3.12 --with "sofar==1.3.0,numpy,scipy<1.15,spaudiopy" scripts/generate_binaural_filters.py --no-download`): **ALL PASS (26 self-checks) in 6.9 s**; `Source/dsp/BinauralFilterData.h` regenerated **byte-identical** (sha256 `0dbc1d15ffab178072f7431877b1b7dd905fdf69694bb29d85a05148034bf19f`) and so did `tests/render-harness/BinauralFilterGoldens.h` (`0bac0968…92320`); `scripts/FILTERS.md` differed only in its "Generated … in 6.9 s" timestamp line (restored to the committed record).
- Project harness (`O-simpleAmbisonics-render-test`) run **twice**: **241 checks, 0 failures** each, the two outputs byte-identical except the PERF-02 wall-clock line (193× / 195× / 72× vs 197× / 197× / 73× — pluginval was running concurrently; gate ≥ 20); **digest-2.1 `9189d911a1fe98d5` and digest-2.2 `7f35bc9f91e321b9`** asserted and reproduced; digest-2.3 `edd1acfd21f54bcd` reproduced.
- Host checks on the **installed** binaries: targeted `auval -v aufx OSAm OuDv` (never `auval -a`); pluginval VST3 + AU at strictness 10, twice each.
- Independent measurer: the installed VST3 loaded in pedalboard (`uv run --no-project --python 3.12 --with pedalboard --with numpy --with scipy`), script kept at `tests/pedalboard/verify_2_3.py`. It shares no code with the harness, `ReferenceModel.h`, `BinauralConvolver.h` or `ShEncoder.h`: the filter banks are parsed straight out of the generated header (the artifact under test), the SH encoder is an own SN3D / ACN table from scipy's associated Legendre functions (Condon–Shortley phase removed, anchored on closed forms), the fs ≠ 48 k bank is `scipy.signal.resample_poly · down/up`. **42 checks, ALL PASS.**
- Placeholder grep: `grep -n "A14" Source/dsp/SpatialEngine.h tests/render-harness/ReferenceModel.h` finds only the two history comments in `ReferenceModel.h` (lines 37, 75) and nothing in `SpatialEngine.h`; `kStereoLevelMatch = 1.0f` with the D2 comment (`StereoDecoder.h:43`, `:63`).

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.3.md §Phase goal)

1. Replace the A14 placeholder with a real HRTF leg: SADIE II KU100 MagLS filters per order, embedded as a generated header.
2. Run them with a grid-synchronous UPOLS convolver on the **rotated** bus, with a click-free dual-bank order crossfade.
3. Level-match to the Stereo decoder so the decoder switch is a timbre change, not a loudness jump.
4. Constant 64-sample latency in both modes at every sample rate.
5. **Nothing 2.1 / 2.2 proved changes** — both Stereo digests reproduce bit-for-bit.
6. Close Stage 2 with the roll-up `VERIFICATION.md`.

### Deliverables (SUMMARY-2.3.md + code inspection)

1. `scripts/generate_binaural_filters.py` (705 lines: sha256-pinned download-or-cache, own MagLS with `spaudiopy.magls_bin` as the oracle, trim 32, level match baked into the scale, 26 aborting self-checks, `%.9g` emission with parse-back) → `Source/dsp/BinauralFilterData.h` (three `inline constexpr float` banks `kMagLsFilters{1,2,3}`, constants `kBinauralOnsetFrontSamples {0, 84, 70, 50}`, `kBinauralLevelMatchDb 0.531`, provenance strings), `tests/render-harness/BinauralFilterGoldens.h`, `scripts/FILTERS.md`.
2. `Source/dsp/BinauralConvolver.h` (491 lines): P = 64 / FFT 128 fixed-partition UPOLS, 16 shared input FFTs, set-*n* channel loops, dual-bank 20 ms fade with a queued latest-wins request, `WindowedSinc` + delta calibration at fs ≠ 48 k, every `std::vector` assigned in `prepare()`, `juce::dsp::FFT` member constructed once. `SpatialEngine`: `conv.arrive → requestSet → step` in `controlStep`, `push` / `outputAt` + the D3 copy in `renderSample (x, t, pos)`; the A14 lines deleted. Inspected: no allocation in `step` / `push` / `outputAt`, the convolver consumes `tap` (post-rotation).
3. `kStereoLevelMatch` stays exactly `1.0f` by design (D2); the +0.531 dB match lives in the filter scale, so the Stereo leg's literals and both digests are untouched.
4. `setLatencySamples` untouched; the convolver's latency is kPartition by construction (grid-synchronous, no FIFO); `static_assert (kControlGrid == kPartition)`.
5. Six 2.1 probes that anchored on the Binaural placeholder migrated to Stereo (C15) with direct-form Binaural twins; digest-2.2 promoted to a verdict (C16).
6. This document + `stages/2-dsp/VERIFICATION.md`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Real MagLS banks from SADIE II, embedded | ✅ Achieved | Generator regenerates the header byte-identical from the pinned SOFA (zip `366321fa…`, SOFA `9af7cb19…`); oracle null vs `spaudiopy.magls_bin` **0.0** on the N3D loop, ≤ 1.15e-13 on the SN3D bank; header sanity probe (fs 48 k, 256 taps, tap 0 exactly 0, all finite). pedalboard V1: the installed plugin's Binaural impulse responses at seven (az, el, order) tuples equal Σ Y·h **from the header floats** within **5.6e-8** (gate 1e-5), with the head before index 100 < 3e-8 and the tail < 2e-10 — the embedded data IS what plays. |
| 2. UPOLS on the rotated bus, click-free order fade | ✅ Achieved | Harness UPOLS null vs direct-form double: **9.5e-8 / 1.4e-7 / 1.3e-7** at orders 1 / 2 / 3 × host blocks 32 / 64 / 100 / 512 / 1024 / 4096; 96 k null 1.1e-7; FDL decays to exact 0 at 2368 / 2624. DSP-07 order 1→3 / 3→1 click ratios **0.998–1.004** at 64 and 1024 blocks vs the same-motion reference (negative controls 11–179×); spectral liveness +6.28 dB; queue null 3.1e-7 with latest-wins (final set 2); float ramp arrives exactly at step 15. pedalboard V8: yaw +90 on a front source == azimuth +90 in Binaural at order 3, **max diff 0.0** over 1 s — the convolver consumes the rotated bus; V5: order 1→3 / 3→1 mid-tone on the installed binary, residual first-difference ratio **1.53 / 1.00** on the steady window (ceiling 2.0, two-ramp bound) with the snapped control at **67.0**, pre-switch bit-equal, settled exactly onto the post-mode render. |
| 3. Level-matched decoder switch | ✅ Achieved | Generator: order 3 front A-weighted white **+0.00 dB** vs the stereo pair's 0.75 (orders 1 / 2 −0.368 / −0.030). Harness FUNC-05 +0.004 dB. pedalboard V4 on the installed VST3: **−0.005 dB** at order 3 (−0.363 / −0.026 at orders 1 / 2, printed); V5 decoder B→S / S→B mid-tone ratio **1.000** steady / 0.604 full (snapped controls 67.7 / 41.4); the post-fade level change at az +90 is the real ILD difference (+1.31 dB on L), not a jump. |
| 4. Latency 64 both modes, every rate | ✅ Achieved | Harness: `getLatencySamples() == 64` at 44.1 / 48 / 96 k in both modes; DSP-03 onsets 248 / 214 at 48 k (table onset == `kBinauralOnsetFrontSamples` exactly), 237 / 210 at 44.1 k, 332 / 263 at 96 k; P4 Stereo still exactly 64 / 0.75f. pedalboard: the Stereo impulse returns at the compensated index 100 with L = R = **0.75 exactly** at 44.1 / 48 / 96 k; the Binaural 10 % onset sits at **100 + 84 / 100 + 50 exactly** (orders 1 / 3) — same 64 in both modes; `out[0..63]` exactly 0. auval Latency PASS. |
| 5. Nothing 2.1 / 2.2 proved changes | ✅ Achieved | **digest-2.1 `9189d911a1fe98d5` and digest-2.2 `7f35bc9f91e321b9` reproduced on both harness runs and asserted as verdicts**; the six migrated probes exact in Stereo with Binaural twins 2.3e-7 … 4.0e-7; the other 154 2.1 / 2.2 checks pass with bodies untouched. pedalboard V7: the Stereo +90° anchor 0.9330127 / 0.0669873 at index 100 and yaw 37 ≡ azimuth 37 bit-equal still hold on the installed binary. |
| 6. Stage-2 roll-up | ✅ Achieved | `stages/2-dsp/VERIFICATION.md` written at this verify, every stage-2 requirement ID with its gate result. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.3 of 3 — the last)
**Requirements for this phase:** DSP-03, FUNC-05, DSP-07, PERF-02 (new); FUNC-01 and FUNC-03 remainders; PERF-01 re-gated; QUAL-01 stage-2 part

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-03: HRTF binaural decoder from an embedded precomputed filter set per order; partitioned convolution; latency via `setLatencySamples()` | must | ✅ Complete | Filters embedded as `inline constexpr float` arrays, no file I/O (the generator runs offline; the header regenerates byte-identical). Impulse latency == reported: harness onsets at every rate + `getLatencySamples() == 64` both modes; pedalboard Stereo impulse at the compensated index 100 (0.75 exactly) and Binaural onset at 100 + the header constant exactly. +90°: ILD **+12.28 / +13.87 dB** (L louder, AmbiX) and ITD **+36 / +37** samples (L leads, LP-xcorr below 0.8·f_a), mirrored at −90° (−11.89 / −14.58 dB, −36 / −37) — harness and pedalboard agree to the printed digit; pedalboard adds order 2 (+13.79 dB, +38). |
| FUNC-05: Decoder selectable Binaural / Stereo | must | ✅ Complete | Both modes produce output (Binaural ≠ Stereo by 1.3988 in the harness, 0.75 on the installed impulse); switching click-free: harness B→S→B 1.0007 / 1.0001 at 64 blocks, 1.0000 / 1.0002 at 1024 (negative 12–28×), pedalboard 1.000 steady / 0.604 full (snapped 41–68×). Reported latency 64 in both modes at 44.1 / 48 / 96 k, switching does not shift time (the stereo leg's `FixedDelay<64>` always runs; pedalboard impulses align at 100 in both modes). |
| DSP-07: Order and decoder changes click-free; crossfaded filter-bank swap; unused channels zeroed; buffers pre-sized | must | ✅ Complete | Order 1→3 / 3→1 and B→S→B mid-tone ratios ≤ 2.0 at 64 and 1024 blocks (harness 0.998–1.004 vs the same-motion reference; pedalboard 1.00–1.53 vs the two-call static pair) with negative controls failing as expected. Buffers: every `std::vector` in `BinauralConvolver` assigned in `prepare()`; spectra sized for all three sets, FDL for K(fs); the accumulate loop covers (order + 1)² channels and bands above the order are exactly 0 (2.1 `LinearRamp` gates); oversized host blocks: P3e prep-64 + 4096 feed bit-equal to prep-4096 in Binaural. |
| PERF-02: 3rd-order binaural without dropouts at 64–1024 buffers on Apple silicon; convolution sized for the maximum host block | must | ✅ Complete | Release, `steady_clock`, 480 000 samples, best of 3: **193× / 197× real time** at 48 k / 64 blocks this session (203× at execute; gate ≥ 20), 195× / 197× at 1024, 72× / 73× at 96 k (K 8). Block sizes above the prepared size: the engine is grid-sized (D1) and the processor slices — P3e bit-equal. |
| FUNC-01: Complete ambisonic chain inside one stereo plugin | must | ✅ Complete | All three boxes now hold with the real leg: stereo/mono-in → stereo-out only (auval caps); order 1 / +90° / Binaural white noise **LEFT louder by +12.31 dB** on the installed VST3 (impulse ILD +12.28 dB, ITD +36); yaw = −azimuth returns the source to front (2.2). |
| FUNC-03: Order 1 / 2 / 3 switchable while audio runs | must | ✅ Complete | 4 / 9 / 16 active channels (2.1); the bus-side switch (2.1, ratio ≤ 1.0) and now the decoder filter-bank swap (DSP-07 above) are click-free. |
| PERF-01: RT-safe; lock-free meter handoff (re-gated with the convolver) | must | ✅ Complete | Allocation counter **0** across 200 `processBlock` calls with the convolver live, order 1→3→2 and decoder B→S→B fades inside the window, orbit On, d 0.2→20, all three angles moving, one 4096 block; liveness 1. By inspection: `step` / `push` / `outputAt` touch fixed arrays and pre-sized vectors only; the FFT member is constructed once; `WindowedSinc` lives on the stack inside `prepare()`. |
| QUAL-01: No artifacts at normal ranges (stage-2 part) | must | ⚠️ Partial | No clicks on order / decoder switch (DSP-07) and no zipper on angle sweeps (DSP-06, 2.1 / 2.2) — done. No NaN / denormal: harness extremes with the HRTF leg (8 Binaural corners × 48 k / 96 k, gimbal, headroom corner at +14.5 dBFS finite) and pedalboard's 32 Binaural corners (48 k / 96 k × orders × pole sources × 0.2 / 20 m × ±180 rotation, orbit 2 Hz) finite and live; pluginval s10 ×2 both formats. **No clicks on preset load → Stage 4** (no presets exist yet). |

**Requirements Summary:**
- ✅ Complete this phase: 4 new (DSP-03, FUNC-05, DSP-07, PERF-02) + FUNC-01 and FUNC-03 closed + PERF-01 re-confirmed
- ⚠️ Partial: 1 (QUAL-01 — the preset-load clause is Stage 4's)
- ⏸️ Deferred (later stage): 12 (UI-01…08, FUNC-08, FUNC-09, COMPAT-02, QUAL-02)
- ❌ Failed: 0
- Unchanged and re-confirmed by the unchanged probe set + both digests: FUNC-02, FUNC-04, FUNC-06, DSP-01, DSP-02, DSP-04, DSP-05, DSP-06; COMPAT-01 (pluginval s10 ×2), FUNC-07

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build VST3 / AU / Standalone / render-test | ✅ Pass | forced rebuild after touching `BinauralConvolver.h` + `SpatialEngine.h`: exit 0, **0 warnings** |
| Installed == build | ✅ Pass | `cmp` on both Mach-O binaries: identical before and after the forced rebuild |
| Generator regeneration | ✅ Pass | 26 self-checks ALL PASS; `BinauralFilterData.h` and `BinauralFilterGoldens.h` byte-identical (sha256 match `FILTERS.md`); only the FILTERS.md timestamp line moved |
| Harness, run twice | ✅ Pass | **241 PASS, 0 FAIL** both runs, byte-identical except PERF-02; digest-2.1 + digest-2.2 verdicts, digest-2.3 reproduced |
| A14 placeholder gone | ✅ Pass | Binaural ≠ Stereo 1.3988 (harness) / 0.7500 (installed impulse); `kStereoLevelMatch` 1.0f with the D2 comment; no placeholder lines (grep) |
| UPOLS null / 96 k null / resampler | ✅ Pass | 9.5e-8 / 1.4e-7 / 1.3e-7 × six blocks; 1.1e-7 at 96 k; vs scipy golden 3.6e-3 / 2.3e-3 in band (gates 1.5e-2 / 6e-3), negative control 1.08e-2 FAILs; cross-rate 1 kHz +0.004 / −0.006 dB; 48 k table bit-exact |
| DSP-03 onsets / latency / ILD / ITD | ✅ Pass | 237 / 210, 248 / 214, 332 / 263; 64 at all six (rate × mode); +12.28 dB / +36, +13.87 dB / +37 (mirrored at −90) |
| DSP-07 / FUNC-05 | ✅ Pass | order clicks 0.998–1.004 at 64 / 1024 (neg 11–179×), decoder 1.0000–1.0007 (neg 12–28×), liveness +6.28 dB, endpoints +19.40 dB; level delta +0.004 dB |
| Queue / arrival / slicing / FDL / extremes / PERF-01 / PERF-02 | ✅ Pass | 3.1e-7 both block sizes, final set 2; step 15 == 1.0f exactly; P2e / P3e / P2f bit-equal; FDL exact 0 at 2368 / 2624; 8 + 8 corners + gimbal + headroom finite; 0 allocations; 193× / 197× |
| `auval -v aufx OSAm OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval VST3 strictness 10 (installed) ×2 | ✅ Pass | exit 0, SUCCESS, 0 FAILED, both runs |
| pluginval AU strictness 10 (installed) ×2 | ✅ Pass | exit 0, SUCCESS, 0 FAILED, both runs |
| pedalboard on the installed VST3 (`tests/pedalboard/verify_2_3.py`) | ✅ Pass | P0 header parse + own-encoder anchors + parameters · V1 seven IRs == Σ Y·h within 5.6e-8, out[0..63] exactly 0 · V2 placeholder gone 0.75, Stereo 0.75 at 100, Binaural onsets 184 / 150 exactly · V3 ILD / ITD at ±90 for orders 1 / 2 / 3 + white-noise +12.31 dB · V4 level delta −0.005 dB · V5 premise (hole in silence = pure +64 shift, exactly 0) + four switches ≤ 2.0 with snapped controls 41–68× · V6 44.1 k / 96 k Stereo 0.75 at 100, IRs vs `resample_poly · down/up` 1.4e-3 / 1.0e-3 in band, onsets 173 / 268, cross-rate level +0.005 / −0.006 dB · V7 Stereo anchors unchanged · V8 rotated bus feeds the convolver (diff 0.0) · V9 32 corners finite · V10 state round-trip — **42 checks ALL PASS** |

## Human Verification (non-blocking for Stage 3)

The installed-binary measurements cover the substance of every item (the embedded SADIE II banks are what plays, +90° is louder LEFT by 12–14 dB with the ear-leading ITD, both switches fade without a step, the decoder switch is level-matched to 0.005 dB). The DAW pass confirms them by ear; QUAL-02 (Stage 4) repeats the localisation check formally.

- [ ] Headphones, Binaural, order 3: front / back / left / right / overhead are audibly distinct; +90° azimuth is louder in the LEFT ear.
- [ ] The image sharpens from order 1 to 3 (side sources narrow; the front stays put).
- [ ] Order 1 → 3 and Binaural ↔ Stereo switches mid-tone are click-free; the decoder switch changes timbre, not loudness, for a front source.
- [ ] Carried from 2.2: Yaw spins the image while the Generic editor's Azimuth value stays put; Pitch + raises the front; Roll + raises the left; flick Yaw 0 → 180 at order 3 on a low tone and listen for a zip (RESEARCH-2.2 R2).
- [ ] Carried from 2.1: Distance 1 → 20 m darkens and attenuates; Orbit On moves, Off glides back the short way; default Binaural setting is NOT silent.

## Issues Found

- **None blocking.** No engine, generator, reference or harness defect surfaced; every SUMMARY-2.3 literal reproduced to the printed digit (PERF-02's wall-clock figure moved from 203× to 193–197× under a concurrent pluginval — the only non-deterministic line, never a verdict).
- Measurer lessons (not plugin defects), recorded so Stage 3 / 4 measurers do not repeat them. The first pedalboard run reported 6 FAILs, all in the measurer: (1) **pedalboard flushes the reported latency with silence at every call**, so a `reset=False` multi-call render carries a 64-sample silent hole in the input at each boundary — invisible on the pure-delay Stereo path (the 2.2 sweep with 256-sample steps read 1.000 and never saw it), but smeared over 64 + 256 samples by the FIR (a 0.40 dip on a constant input, a 48° phase shift of a 100 Hz tone vs a one-call render). The switch gates now park the call boundary in 1024 samples of silence (premise asserted: two-call == one-call after the boundary up to a +64 shift, exactly 0) and compare the switched render against two-call static renders with the same call structure (residual = fade·(s2 − s1)). (2) An integer-Hz tone crosses zero at exactly 0.5 s, so a snap at the switch — or at a 960-sample fade's arrival two periods later — is invisible; 112.5 Hz lands the arrival on a peak and the snapped control sweeps 16 positions. (3) An order switch is two simultaneous ramps (bank fade × band gain) whose product has twice the slope at one end, so the residual's first-difference ratio reads 1.53 on the steady window, not the 1.07 of a single linear fade — ceiling 2.0 (the project's click ceiling), against a snap at 67×; the harness holds the exact same-motion ratio (1.0004 / 0.9995). (4) ITD by LP-xcorr must cut below the MagLS switch frequency per order (0.8·f_a = 499 / 998 / 1497 Hz, the generator's C10): a fixed 1.5 kHz low-pass reads +16 at order 1 where the LS region reads +36. Memory: `pattern_pedalboard_multicall_inserts_latency_flush_hole`.
- Noted for Stage 3 (unchanged from 2.2): the frame carries the host's float (`elEff` −11.999999); the puck readout formats the parameter. New from 2.3: if the decoder view wants the bank-fade progress, `BinauralConvolver::bankMixCur()` / `isFading()` are the sources (no `MeterFrame` field was added).
- Noted for Stage 4 presets (SUMMARY-2.3 F7): the order-3 bank peaks at +6.5 dB (9.5 kHz, worst direction) and d = 0.2 m adds +8.9 dB — the headroom corner reads +14.5 dBFS on full-scale noise; Binaural presets must not sit near 0 dBFS (`outputLevel` exists, presets never set it).
- Deferred items stand: TripleBuffer → `core/triple-buffer` module candidate; −180 / 180 display, gimbal tooltip, readout formatting → Stage 3; BiMagLS / DFE option / second head, own Kaiser polyphase resampler (HF above 15–16 kHz at fs ≠ 48 k is the interpolator's design, inaudible) → v1.1; Windows digest parity none claimed (digest-2.3 is vDSP-only).

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.3 of 3 — Stage 2 complete; roll-up in `stages/2-dsp/VERIFICATION.md`)

**Ready for next stage:** Yes. Stage 3 (GUI) starts with the UI mockup (`/start O-simpleAmbisonics` → option 3, which finalises `parameter-spec.md` and must keep the 12-parameter set), then `/plugin-discuss O-simpleAmbisonics 3-gui`. The DSP contract Stage 3 binds to: 12 APVTS parameters unchanged; `MeterFrame` (shGain[16] post-rotation, shPeak[16], azEff / elEff pre-rotation, azRot / elRot, orderActive, decoderMix, latencySamples 64) via `TripleBuffer`; both Stereo digests and digest-2.3 are the regression verdicts for any Stage-3 change that touches the processor.

**Blockers:** None
