# Stage 2 (DSP) — SUMMARY

**Plugin:** O-simpleSubtractive · **Stage:** 2 of 4 (DSP) · **Date:** 2026-06-25
**Result:** ✅ Complete — audible polyphonic subtractive synth; all 18 render-harness probes pass, AU validation SUCCEEDED, pluginval (strictness 10) clean.

## What was implemented

The silent Foundation shell is now a full subtractive instrument:
**OscillatorBank → multimode ZDF state-variable filter → VCA**, two independent ADSRs,
Poly/Mono/Legato voicing + glide, and a real-time-safe visualization tap. No oversampling
(PolyBLEP band-limits at source) → **zero added latency**.

### New files
| File | Role |
|------|------|
| `Source/OscillatorBank.h` | PolyBLEP saw/square + polyBLAMP triangle + LUT sine + sub-square (−1 oct) + xorshift noise + mixer. Steady-phase → PolyBLEP composes, no oversampling. |
| `Source/SvfZDF.h` | Cytomic ZDF state-variable core (LP/BP/HP simultaneous; Notch derived) + one-pole TPT (6 dB). Soft-knee limiter on the resonant integrator for clean bounded self-oscillation. |
| `Source/SubVoice.h` | `SubSound` + `SubVoice : SynthesiserVoice`. Per-sample osc→filter→VCA; fcEff = cutoff·keyTrack·bipolar-filter-env (octaves); dual `juce::ADSR`; slope routing (1-pole / 1×SVF / 2×SVF cascade w/ resonance on stage 1); glide; Poly + direct Mono/Legato drive paths. |
| `Source/SubVizAnalyzer.h` | `VizRing` (lock-free overwrite ring, verbatim from sibling) + message-thread FFT/scope + `SubFilterCurve::magnitudeDb` closed-form filter curve (same g/k as audio → QUAL-02 by construction). |
| `tests/render-harness/{main.cpp,CMakeLists.txt}` | Offline Stage-2 correctness gate (18 probes). |

### Modified files
- `Source/PluginProcessor.{h,cpp}` — 16-voice `juce::Synthesiser`; `MonoStack` (RT-safe held-note stack, last-note priority); `pushParamsToVoices()` block-push; **no-oversampling** `processBlock` (clear → merge UI MIDI → push → Poly `renderNextBlock` *or* `renderMonoLegato` → output gain → isfinite scrub → lead-voice display atomics → VizRing tap); `handleUiMidi`; display getters for Stage 3. `setLatencySamples(0)`.
- `CMakeLists.txt` — new headers in `target_sources`; `OUARICON_BUILD_TESTS` option + `tests/render-harness` subdir.

## Phasing (as planned)
- **2.1** Source + LINEAR SVF (4 modes × 3 slopes) + dual ADSR + VCA + Synthesiser wiring.
- **2.2** Self-oscillation (slightly-negative-k at knob top, bounded by a soft-knee limiter) + resonance make-up trim + closed-form magnitude curve.
- **2.3** Poly/Mono/Legato + glide (multiplicative) + VizRing tap + dual-ADSR env atomics.

## Key decisions / deviations
- **Self-osc recipe:** a plain `tanh(ic1eq)` every sample *damps* the lossless resonator (it never builds). Replaced with a **soft-knee limiter** (identity below the knee, soft-saturating above) + a **localized negative-k bias** (`-0.18·res⁸`, negligible below res≈0.9) so the resonator builds from silence to a clean bounded limit-cycle sine. The bias being localized keeps the magnitude curve **exact** in the normal range (curve-vs-measured = 0.00 dB error).
- **Nonlinearity gated by `k < 0.6`** (top ~10% of the resonance knob) so normal resonant sounds stay LINEAR — that's why the closed-form curve matches the running filter sample-for-sample.
- **Mono retrigger** resets the envelopes to 0 before `noteOn()` (JUCE `ADSR::noteOn()` does not zero, so an in-sustain retrigger was inaudible); Legato suppresses the retrigger (slur). Both glide.
- **Mono/Legato** bypass the Synthesiser note-allocation and drive voice 0 directly in sample-accurate slices (the only way to express legato pitch-change-without-retrigger to a JUCE voice).

## Validation
- **Render-harness: 18/18 PASS** — makes-sound, pitch, sine-vs-saw, LP/HP/BP/Notch, slope steepening, bipolar filter env, key-track, high-key aliasing, self-osc (clean+bounded), self-osc-in-tune (×2), curve-vs-measured (0.00 dB), poly-both, mono-last-note, legato-vs-mono, glide.
- **AU:** `auval -v aumu OSiS OuDv` → **AU VALIDATION SUCCEEDED**.
- **VST3:** pluginval `--strictness-level 10` → **SUCCESS**, no alloc/leak/exception/warning.
- Build: VST3 + AU + Standalone + render-test all clean (one pre-existing unused-field warning in the Stage-1 editor placeholder, replaced in Stage 3).

## Deferred to later stages
- WebView UI + parameter binding + drawing the spectrum/scope/curve → **Stage 3**.
- Factory presets / preset tour → **Stage 4**.
