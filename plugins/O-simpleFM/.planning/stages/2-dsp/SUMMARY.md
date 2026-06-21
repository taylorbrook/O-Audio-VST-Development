# Stage 2 (DSP) — SUMMARY

**Plugin:** O-simpleFM · **Stage:** 2 DSP · **Date:** 2026-06-20
**Result:** ✅ Complete — the silent Stage-1 shell is now a polyphonic, MIDI-playable 2-operator phase-modulation synth. Builds clean (VST3 + AU), auval SUCCEEDS, and an offline render harness proves the FM math to Bessel-function accuracy.

## What was implemented

All 3 ROADMAP DSP sub-phases, delivered together (express mode), built in two passes to isolate the oversampling risk.

### Files created
- `Source/Operator.h` — `OSimpleFM::fastSine(float)`: shared 1024-pt `juce::dsp::LookupTableTransform` sine (linear, ~97 dB SNR) with **mandatory floor-modulo phase wrap** before lookup (the LUT clamps, not wraps; the PM argument swings to many ×2π at high index).
- `Source/FMVoice.h` — `FMSound` + `FMVoice : juce::SynthesiserVoice`:
  - **PM core in radians** — `carOut = fastSine(carPhase + effIndex·modOut)`; `modPhase += 2π·fm/fs`; floor-wrapped each sample.
  - Modulator freq mode: `fm = fixedMode ? fixedHz : fc·ratioEff` (Ratio key-tracks; Fixed holds constant Hz).
  - **DX7 two-sample-average self-feedback** — `modOut = fastSine(modPhase + coeff·½(prev1+prev2))`; `isfinite` scrub at source; **clamp history**; reset history (not phase) on note-on; coeff max ≈ π with `x^1.5` taper.
  - **Multiplicative mod-env → index** — `I_inst = baseIndex·((1−depth)+depth·modEnv)·velFactor`, depth default 1.0; perceptual taper `baseIndex = 20·norm^1.7`.
  - **Key-tracked index ceiling** (Carson) — `(0.9·Nyq − fc)/fm − 1`, smoothed (crossfade so bright patches dull gracefully when played high).
  - Independent amp + mod `juce::ADSR`; **voice lifetime gated on amp env only**; velocity→amplitude always on; `velToIndex` opt-in.
  - Non-virtual JUCE-8 `prepareToPlay(double,int)` (setSampleRate before setParameters); `SmoothedValue` on index/feedback/ceiling + multiplicative on ratio.
- `Source/FmVizAnalyzer.h` — `VizRing` (lock-free power-of-two `std::array<std::atomic<float>,8192>`, copy-only audio-thread write) + `FmVizAnalyzer` (message-thread 4096/Blackman-Harris `dsp::FFT` log-freq dB bins with rise-fast/fall-slow smoothing + 1024→128 max-abs scope; **scope copied before the in-place FFT**).
- `tests/render-harness/{CMakeLists.txt,main.cpp}` — gated offline DSP correctness gate (`-DOUARICON_BUILD_TESTS=ON`).

### Files modified
- `PluginProcessor.h/.cpp` — 16 pre-allocated `FMVoice` + `FMSound`; per-voice prepare dispatched via `dynamic_cast` **at the oversampled rate**; `pushParamsToVoices()` (read APVTS atomics once/block, unconditional push); 2× `dsp::Oversampling<float>` (`filterHalfBandPolyphaseIIR`) — **synth renders at 2× into the up-block, decimated via `processSamplesDown`** (the only way OS fights FM aliasing); MIDI timestamps scaled ×2 into a reused member buffer; output gain (`SmoothedValue`, dB→lin); block NaN scrub; post-gain mono-sum → `VizRing`. `getTailLengthSeconds` 0→5 s. `releaseResources` resets OS + all-notes-off. `setLatencySamples` from `getLatencyInSamples()`.
- `PluginEditor.h/.cpp` — editor is now a `juce::Timer` (30 Hz) owning an `FmVizAnalyzer`; `timerCallback` pumps the ring→FFT/scope path. Body still `GenericAudioProcessorEditor` (Stage 3 swaps it for WebView + `emitEventIfBrowserIsVisible` using this exact callback).
- `CMakeLists.txt` — new headers added to `target_sources`; `OUARICON_BUILD_TESTS` option + harness subdir.

## Build & validation
- `ninja O-simpleFM_VST3 O-simpleFM_AU` — clean, **zero warnings** (both passes).
- AU cache clear + dual-variant sweep + install (per CLAUDE.md).
- `auval -v aumu OSiF OuDv` → **AU VALIDATION SUCCEEDED** (render @ 6 sample rates, 1-ch, MIDI, **Latency** property, 17-param set/schedule/ramp, bad-max-frames, reset-retention all PASS).
- **Offline render harness — ALL PASS:**
  - makes-sound: rms=0.445
  - pitch=fundamental: a440=0.63, a220/a880≈0 (pure carrier at index 0)
  - index→sidebands: sb@2200 Hz 0.000→0.030 with index
  - **carrier-null@I≈2.405: carrier/sideband ratio = 0.0001** (Bessel J₀ zero reproduced)
  - feedback-stable @100%: peak=0.93, rms=0.375, finite, bounded

## Decisions / deviations
- **Direct orchestrator authoring** (not delegated to dsp-agent) — matches the Stage-1 precedent; the dsp-agent cannot build, and tight build→fix iteration + offline-harness validation was lower-risk for DSP this dense.
- **Synth renders at 2× then decimates** (vs the effect-style up→process→down) — required because a synth has no input; this is how oversampling actually band-limits generated FM sidebands. MIDI scaled ×2 for sample-accurate timing.
- **`filterHalfBandPolyphaseIIR`** as specced (suite elsewhere uses FIR equiripple) — lower latency.
- **Unconditional per-block param push** (no change-detection) — 16 voices × a few floats is trivial; change-detection deferred as premature.
- **FFT/scope on the editor Timer** (ARCHITECTURE), not processBlock (the O-MultiBandCompressor deviation). Stage 2 runs the path headlessly; Stage 3 emits to the WebView.
- **Render harness kept** as a permanent regression gate (off by default).
- Pitch-bend range fixed at ±2 semitones (no APVTS param — matches v1.0 scope).

## Carryover to later stages
- Stage 3: swap `GenericAudioProcessorEditor` for the WebView; emit `vizAnalyzer.getSpectrum()/getScope()` via `emitEventIfBrowserIsVisible` in the existing `timerCallback`; wire 17 relays/attachments; optional `Δf = I·f_m` readout + on-screen keyboard MIDI injection.
- Stage 4: factory presets (preset tour recipes in ARCHITECTURE), pluginval sweep, aliasing/artifact audit, changelog.
