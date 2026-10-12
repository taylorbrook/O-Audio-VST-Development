# O-simpleAmbisonics - Implementation Plan

**Date:** 2026-10-11
**Complexity Score:** 5.0 (Complex — capped; raw 10.0)
**complexity_score: 5.0**
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 12 parameters (12/5 = 2.4, capped at 2.0) = **2.0**
- **Algorithms:** 6 DSP components = **6**
  - SH encoder (AmbiX ACN/SN3D, orders 1–3) with smoothed order gating
  - Distance model (soft-knee 1/d gain + one-pole air LP)
  - SH rotator (Ivanic–Ruedenberg recursion, per-sample matrix lerp)
  - Binaural decoder (offline MagLS filter pipeline + fixed-partition UPOLS convolver with dual-bank crossfade)
  - Stereo virtual-cardioid decoder + latency-matched decoder mixer
  - Orbit LFO + shortest-path angle-smoothing system
- **Features:** 2 points = **2**
  - FFT / frequency domain (+1) — partitioned convolution on `juce::dsp::FFT`
  - Modulation systems (+1) — orbit LFO driving azimuth
- **Total (raw):** 2.0 + 6 + 2 = **10.0** → **capped at 5.0**

> No feedback loops, no MIDI, no multiband. The genuinely new work vs the siblings is the **MagLS filter pipeline + UPOLS engine** (never built in-house — O-GrainScatter uses synthetic HRIRs through a direct-form FIR) and the **SH rotation recursion**. Everything else (encoder, viz handoff, WebView editor, harness, CMake) is a port.

---

## Stages

- Stage 0: Research ✓
- Stage 0: Planning ✓
- Stage 1: Foundation ← Next (after mockup finalisation produces `parameter-spec.md`)
- Stage 2: DSP (phased — 3 phases)
- Stage 3: GUI (phased — 3 phases)
- Stage 4: Validation / Polish

---

## Requirements → Stage Traceability

| Stage | Requirements covered |
|-------|----------------------|
| Stage 1 Foundation | COMPAT-01 (shell passes pluginval VST3+AU), FUNC-07 (mono/stereo-in → stereo-out bus layout + input conditioner), 12-param APVTS + state, `setLatencySamples(64)` reported from day one |
| Stage 2 DSP | FUNC-01, FUNC-02, FUNC-03, FUNC-04, FUNC-05, FUNC-06; DSP-01…DSP-07; PERF-01, PERF-02; QUAL-01 |
| Stage 3 GUI | UI-01…UI-08 |
| Stage 4 Polish | FUNC-08 (preset tour), FUNC-09 (creative panner usability), COMPAT-02 (Logic + Ableton hands-on, auval), QUAL-02 (listening check), validation sweep |

Every `must` requirement (FUNC-01/02/03/04/05/07, DSP-01/02/03/06/07, UI-01/02/07, PERF-01/02, COMPAT-01/02, QUAL-01/02) has a named phase and a gate below.

---

## Pre-Stage-1: Mockup + parameter-spec

`parameter-spec-draft.md` → `parameter-spec.md` happens at mockup finalisation (`/start O-simpleAmbisonics` → option 3). ARCHITECTURE.md fixes the v1.0 set at **12 parameters** with the ranges in its Parameter Mapping table; the mockup must not add parameters (spread / weighting / solo are v1.1).

---

## Stage 1: Foundation

**Goal:** Buildable, pluginval-clean stereo effect shell with the full APVTS, correct bus layout, cross-platform CMake, and the latency contract already in place.

- `juce_add_plugin` effect (not synth): `FORMATS VST3 AU Standalone`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`, `JUCE_VST3_CAN_REPLACE_VST2=0`; link `juce_dsp`; `juce_generate_juce_header()` after `target_link_libraries()`. Single UI BinaryData target (default namespace) — the HRTF data is a generated header, no second target.
- `BusesProperties` stereo in / stereo out; `isBusesLayoutSupported`: in ∈ {mono, stereo}, out = stereo.
- APVTS: the 12 parameters exactly as ARCHITECTURE.md Parameter Mapping (IDs, ranges, skews, defaults, choice strings).
- `processBlock`: input conditioner (`inputMode`) → 64-sample delay → output gain (seeded) → out; `setLatencySamples(64)` in `prepareToPlay`. (Pass-through with the final latency so Stage 2 never changes host-visible timing.)
- `Source/dsp/` directory + empty `SpatialEngine.h` seam; `tests/render-harness/` CMake scaffold (sibling pattern, `OUARICON_BUILD_TESTS`).
- **Test criteria:** builds VST3 + AU; pluginval strictness 10 passes (COMPAT-01); loads on a mono and a stereo track; audio passes with 64 samples latency reported; all 12 parameters visible to the host with correct ranges; state round-trips.

---

## Stage 2: DSP Phases

### Phase 2.1: Source → Encoder → Stereo decode (audible chain, no rotation, no HRTF)
**Requirements:** FUNC-01 (partial), FUNC-02, FUNC-03 (encoder side), FUNC-06, FUNC-07, DSP-01, DSP-04, DSP-05, DSP-06, PERF-01, QUAL-01 (partial)

**Components:**
- `ShEncoder.h` (lift `encodeSH16` from O-GrainScatter + 3 smoothed order band gains + per-sample 16-vector lerp).
- `DistanceModel.h` (`g(d) = 1.25/(d+0.25)`, `FirstOrderTPTFilter` air LP with Nyquist clamp, both smoothed, gain seeded).
- `WrapSmoother.h` (shortest-path one-pole, τ = 20 ms, block rate) + plain one-pole for elevation/gain/cutoff.
- `OrbitLfo.h` (phase accumulator; reset on Off→On; CCW).
- `StereoDecoder.h` (±60° cardioid pair, W/Y/X, `kStereoLevelMatch` placeholder = 1 until 2.3) + `DecoderMixer.h` with the 64-sample stereo delay (Binaural input silent for now).
- `SpatialEngine.h` composition; processor wiring; `MeterFrame`/`TripleBuffer` tap (post-"rotation" — identity matrix in this phase).
- Harness gates: **DSP-01** coefficient table (13×7 grid, < 1e-5, W ≡ 1); **DSP-06** +170→−170 trajectory through 180° + 1 s full-circle click gate; **DSP-05** monotonic level + HF ratio vs distance; **DSP-04** front L = R, +90° ≥ 20 dB separation, in-phase; **PERF-01** allocation counter = 0 in `processBlock`; order 1→3→1 encoder band fade click-free.

**Test criteria:**
- [ ] Source audibly pans with Azimuth in Stereo mode; +90° is louder LEFT (AmbiX sign)
- [ ] Orders 1/2/3 light 4/9/16 channels in the meter frame; unused bands fade to exactly 0
- [ ] Distance 1 → 20 m: level falls, top end darkens, monotonic
- [ ] Orbit On: continuous motion at `orbitRate`; Off: glides back the short way
- [ ] All gates above green; latency still 64

### Phase 2.2: SH Rotation (yaw/pitch/roll) + rotated meter tap
**Requirements:** FUNC-04, DSP-02, FUNC-01 (meters follow rotation)

**Components:**
- `ShRotator.h`: Cartesian `R = Rz(yaw)·Ry′(pitch)·Rx(roll)` with the signs fixed in ARCHITECTURE.md; order-1 block permuted to ACN (Y,Z,X); Ivanic–Ruedenberg recursion for the 5×5 and 7×7; 83-element contiguous storage; per-sample lerp between previous and current block matrices.
- Wrap-aware smoothing on all three angles; `shGain = R·c_eff` and `azRot/elRot` published in the `MeterFrame`.
- Harness gate **DSP-02**: 200 random (az, el, ψ, θ, φ) at orders 1/2/3 — `R_SH·encode(d)` vs `encode(R·d)` < 1e-4; three cardinal sign assertions; yaw sweep click gate.

**Test criteria:**
- [ ] Yaw spins the decoded image while `azEff` (the puck) is unchanged in the frame
- [ ] Yaw = −Azimuth returns the source to front: audibly, and the frame's `shGain` matches the front encode
- [ ] Pitch/roll move elevation as specified (+pitch: front rises; +roll: left rises)
- [ ] DSP-02 gate green at every order; no zipper on a fast 360° yaw sweep

### Phase 2.3: Binaural — MagLS filter pipeline + UPOLS engine + crossfades + CPU/latency gates
**Requirements:** FUNC-03 (filter-bank swap), FUNC-05, DSP-03, DSP-07, PERF-02, QUAL-01

**Components (in this order):**
1. `scripts/generate_binaural_filters.py` (SADIE II D1 KU100, 48 kHz; SN3D SH matrix; MagLS per order with LS below `f_a`, magnitude-only above, half-octave blend, group-delay continuation; 256-tap window-truncation; common normalisation; `kStereoLevelMatch`; emits `Source/dsp/BinauralFilterData.h` + `.npz` goldens + `FILTERS.md`). Offline self-checks: ILD sign and ITD at ±90°, W LF flatness. **Fallback:** synthetic spherical-head HRIRs through the same path.
2. `BinauralConvolver.h`: fixed P = 64, FFT 128, K = taps/64; three spectra sets built in `prepare` (WindowedSinc resample when fs ≠ 48 kHz); 16×P input FIFO / 2×P output FIFO; shared input FFTs; dual-bank 20 ms crossfade with queued set requests. **Write the UPOLS-vs-offline-convolution gate first** (block sizes 32/64/100/1024/4096, < 1e-5).
3. Wire into `DecoderMixer` (binaural ↔ 64-delayed stereo, 20 ms ramp); set `kStereoLevelMatch`; processor slice loop for host blocks > prepared maximum; always-run both decoders.
4. Gates: **DSP-03** impulse latency = `getLatencySamples()` = 64 at 44.1/48/96 kHz, both modes, orders 1/3; +90° ILD sign ≥ 6 dB and ITD 20–40 samples; **DSP-07** order 1→3 / 3→1 and Binaural→Stereo→Binaural mid-tone click gates at 64 and 1024-sample blocks, decoder-switch level delta < 1 dB; **PERF-02** ≥ 20× real time at order 3 / 64-sample blocks (Release) + oversized-block gate; **PERF-01** allocation gate re-run with binaural active and during fades; **QUAL-01** NaN/denormal sweep across parameter extremes.

**Test criteria:**
- [ ] Headphones: front / back / left / right / overhead audibly distinct at order 3; image sharpens 1 → 3
- [ ] Order and decoder switches are click-free mid-tone; no loudness jump on decoder switch
- [ ] Latency constant and equal in both modes at every tested sample rate
- [ ] All Stage 2 gates green; `VERIFICATION.md` lists each requirement ID with its gate result

---

## Stage 3: GUI Phases

### Phase 3.1: Layout + basic controls + cross-platform wiring (UI-07, UI-08 scaffold)
- Copy the finalised mockup to `Source/ui/public/`; `index.js` + `check_native_interop.js`; `type="module"`; relative-drag knobs; explicit bare-path resource provider; EB Garamond embed per suite.
- PluginEditor member order relays → WebView → attachments (3-arg attachments, `nullptr` undoManager); `WebSliderRelay` ×8, `WebComboBoxRelay` ×3, `WebToggleButtonRelay` ×1; Windows `withUserDataFolder(tempDir)`.
- Left-to-right signal-path layout: Source | Encoder | Rotation | Decoder | Output; preset-tour buttons present (wired in 3.3).
- [ ] All 12 controls bound both ways (host automation → UI, UI → host); projector-readable at the mockup size

### Phase 3.2: Live visuals — B-format channel view, sphere view, signal-path diagram (UI-01, UI-02, UI-04)
- Editor Timer 30 Hz: `TripleBuffer<MeterFrame>::read()` → `meterFrame` event (hash-gated, `emitEventIfBrowserIsVisible`, resend on `uiReady`).
- B-format view: 4/9/16 rows by `order`, ACN index + letter + SH pattern icon (16 static SVGs), bar = signed `shGain` (polarity colour) with `shPeak` overlay, rAF ballistics (attack 0.4 / decay 0.15).
- Sphere view: top-down + side canvases; puck at `azEff/elEff` (drag sets `azimuth`/`elevation` through the slider states with gesture begin/end); radius from `distance`; head with nose; compass rose rotated by `yaw` (and an elevation tilt mark for pitch); the puck never moves with yaw.
- Signal-path diagram: active stage highlight (encoder when the source moves, rotation when angles move, decoder on mode change), channel count = `orderActive`.
- [ ] UI-01/02/04 acceptance: moving front → left swaps X → Y with W steady; dragging the puck updates host params; yaw turns the compass, not the puck

### Phase 3.3: Polar plot, decoder view, tooltips, presets, i18n (UI-03, UI-05, UI-06, UI-08)
- Polar plot: `g_N(γ)` beam from `order`, centred on `azRot` in the top-down view; visibly sharpens 1 → 3.
- Decoder view: Binaural = KU100 head + "MagLS filter bank, 64-sample latency"; Stereo = two cardioid mics at ±60°.
- Tooltips on every control (W/X/Y/Z meaning, order ↔ channels, scene vs source rotation, ACN/SN3D, front/back confusion, why Stereo ignores orders 2–3, the exaggerated air model); suite i18n (glossary first, lint gates).
- `applyFactoryPreset` native function wired to the 8 tour scenes (content filled in Stage 4; never sets `outputLevel`).
- [ ] UI-03/05/06/08 acceptance; Standalone hands-on; UI gates (labels, i18n, tooltip clamp) green

---

## Stage 4: Polish

- Factory presets: Point Source (FOA), Order 1 → 2 → 3, Behind You, Overhead, Rotate the Room, Far Away / Up Close, Slow Orbit, Speakers vs Headphones (FUNC-08); creative-panner sanity (FUNC-09).
- COMPAT-02: Logic Pro mono + stereo tracks, Ableton Live, `auval -v`, no I/O configuration; Windows VST3 build with WebView2 static.
- QUAL-02 human listening check on headphones (front/back/left/right/above at order 3; +90° louder left).
- pluginval s10 VST3 + AU re-run; CHANGELOG `## [1.0.0]`.

---

## Implementation Notes

### Thread Safety
- Audio thread: no alloc/lock/FFT-plan/file I/O; parameters read once per block; `TripleBuffer` publish per block.
- Message thread: Timer read + JSON emit; native functions only touch APVTS.
- `prepareToPlay` owns every allocation (FFT plans, spectra, FIFOs, resampled FIRs) and re-seeds all smoothers.

### Performance
- ≈ 10 MFLOPS chain + ≈ 82 MFLOPS UPOLS at 48 kHz / order 3 (≈ 190 at 96 kHz); ×2 MAC during a bank fade. Target ≤ 3 % of one Apple-silicon core; gate ≥ 20× real time at 64-sample blocks.

### Latency
- Constant 64 samples (`kPartition`), all modes/orders/rates; `setLatencySamples(64)` in `prepareToPlay` (`getLatencySamples` non-virtual). Stereo path delayed by the same constant.

### Denormal Protection
- `ScopedNoDenormals` + block `isfinite` scrub; FDL and TPT pole decay to exact zero.

### Known Challenges
- JUCE `FFT::performRealOnlyForwardTransform` needs a 2N-float buffer with real input in the first half; the inverse returns reals in the first half — get the overlap-save indices right in the UPOLS gate before integration.
- MagLS phase continuation: use average-group-delay continuation above `f_a`; naive previous-bin phase disperses at HF.
- Three concurrent fades (encoder bands, filter bank, decoder mix) — all ramps, never steps; set requests mid-fade are queued.
- Host blocks larger than `samplesPerBlock` (project memory) — slice loop in the processor + FIFO-based convolver.
- Gain/output ramps seeded in `prepareToPlay` (no fade-in from silence).
- `WebSliderParameterAttachment` dtor leaves a gesture open on editor close (project memory) — apply the O-simpleWavetable v1.0.1 fix in Phase 3.1.
- ASan hangs at init on macOS 26 — lifetime/allocation gates are harness counters, not ASan.

---

## References

- Creative brief: `plugins/O-simpleAmbisonics/.planning/BRIEF.md`
- Requirements: `plugins/O-simpleAmbisonics/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-simpleAmbisonics/.planning/parameter-spec-draft.md` → final at mockup finalisation
- DSP architecture: `plugins/O-simpleAmbisonics/.planning/research/ARCHITECTURE.md`
- Research: `research/ambisonics-encoding-deep-dive.md`, `research/ambisonics-binaural-decoding-deep-dive.md`
- Prior art: O-GrainScatter (`Source/dsp/SpatialEncoder.h`, `BinauralDecoder.h` fallback, `TripleBuffer.h`, `scripts/generate_hrir_header.py`)
- Siblings: O-simpleSubtractive (editor/CMake/harness template), O-simpleWavetable (latest WebView + gate conventions), O-simpleFM (viz Timer pattern)
