# O-Formant Changelog

All notable changes to O-Formant will be documented in this file.

## [1.19.0] - 2026-04-11

### Added
- **Effects tab** — New dedicated Effects tab with four signal processors matching O-Lyrica's effects chain:
  - **Chorus** — JUCE built-in chorus with Rate (0.1–10 Hz), Depth, and Mix controls
  - **Delay** — Stereo delay with Lagrange interpolation, 8 kHz feedback filter, Normal/PingPong modes, Time (1ms–2s), Feedback (0–95%), and Mix
  - **Reverb** — 8-channel FDN plate reverb with input diffusion, Householder feedback matrix, multi-LFO modulation, shimmer (octave-up pitch shifter with HP filter), configurable pre-delay (0–200ms), Size, Damping, Mod, Shimmer, and Mix
  - **EQ** — 3-band parametric EQ with Low Shelf (200 Hz), Mid Peak (200–8000 Hz), High Shelf (8 kHz), all ±12 dB
- **Per-effect bypass buttons** — Each effect section has an On/Off toggle
- **22 new APVTS parameters** for full effects control and automation
- Effects chain order: Chorus → Delay → Reverb → EQ → Output Gain

### Technical Notes
- New DSP files: `DSP/DelayProcessor.cpp`, `DSP/EQProcessor.cpp`, `DSP/ReverbProcessor.cpp`
- Effects processing inserted between synthesiser output and output gain stage
- All effects use thread-safe atomic parameter targets with dirty-flag coefficient updates
- Tail length updated to 5.0s to account for reverb/delay tails
- No existing parameter IDs changed — existing presets and automation unaffected

## [1.18.0] - 2026-04-11

### Added
- **Tuning module with dedicated tab** — Full microtonal tuning system integrated via the scala-tuning-engine module v2.0.0. UI converted from single-page to tabbed layout (Synth + Tuning tabs). Tuning tab provides interval editing, pitch circle/polar/matrix/TrueKeys visualizations, embedded tuning library (24+ scales across Historical, Just Intonation, EDO, Non-Octave, and World categories), scale generator (EDO, Harmonic Series, Rank-2), Scala file I/O (.scl/.kbm), adjustable master tune (A4 reference), octave stretch for physical modeling, and tonic selection.
- **5 new APVTS parameters:** `tuning_masterTune` (400–480 Hz), `tuning_tuningMode` (12-TET/Custom/MTS-ESP), `tuning_octaveStretch` (0.95–1.25), `tuning_pitchBendRange` (1–48 st), `tuning_temperamentPreset` (11 built-in temperaments + Custom).
- **Tuning state persistence** — Custom intervals, scale name, and tonic are saved/restored with DAW sessions via ValueTree child node.

### Changed
- FormantVoice now uses TuningEngine for MIDI-to-frequency conversion instead of standard 12-TET `getFrequencyInHertz()`. Affects note onset frequency, spectral tilt reference, and source-filter coupling estimation.

### Technical Notes
- New C++ files: `TuningEngine.cpp`, `ScaleGenerator.cpp`, `TuningExporter.cpp`, `EmbeddedTunings.cpp` (from scala-tuning-engine module)
- New JS: `tuning-panel.js` (self-contained TuningPanel component)
- 25 native functions registered for C++ ↔ JS tuning bridge
- No existing parameter IDs changed — existing presets and automation unaffected.

## [1.17.0] - 2026-04-11

### Added
- **Velocity-to-amplitude dynamics** — Voice output now scales with MIDI velocity (~12 dB dynamic range). Low velocity produces softer notes, high velocity produces full amplitude. Previously velocity only affected glottal Rd (timbral character) with no amplitude response, making soft playing nearly silent.

### Fixed
- **Cascade normalization too aggressive** — Relaxed formant cascade gain compensation from `1/maxPeakGain` to `1/sqrt(maxPeakGain)`, recovering ~12 dB of headroom. The v1.14.1 normalization prevented clipping but over-attenuated the signal, making `tanh()` soft-clipper a no-op instead of providing gentle saturation. Peaks now reach 2–4× into tanh for natural warmth.
- **Nasality amplitude compensation** — Reduced excessive volume drop when `nasalCoupling` is active. Direct attenuation reduced from -8 dB to -3 dB at full coupling; formant bandwidth widening scaled from 2× max to 1.6× max; aspiration suppression reduced from 80% to 50%.

### Technical Notes
- `FormantVoice.cpp`: block-rate `velocityGain = 0.25 + 0.75 * noteVelocity` applied to both voiced source and consonant noise before formant filters
- `CascadeFormantBank.h` line 139: normalization changed from `1.0f / maxPeakGain` to `1.0f / std::sqrt(maxPeakGain)`
- `FormantVoice.cpp` line 236: `nasalAmpGain` changed from `-8.0f * nasalCouplingVal` to `-3.0f * nasalCouplingVal`
- `FormantVoice.cpp` line 243: aspiration suppression coefficient changed from `0.8f` to `0.5f`
- `FormantVoice.cpp` line 392: `nasalBWScale` changed from `1.0f + nasalCouplingVal` to `1.0f + nasalCouplingVal * 0.6f`
- No parameter ID changes — existing presets and automation unaffected.

## [1.16.0] - 2026-04-11

### Added
- **Nasal consonant support (/m/, /n/, /ŋ/)** — Klatt-style pole-zero filter added to the voice signal chain with two new parameters: `nasalCoupling` (0–1 velum opening) and `nasalPlace` (0=bilabial /m/, 0.5=alveolar /n/, 1.0=velar /ŋ/). The pole-zero chain consists of a fixed nasal-cavity resonator at 270 Hz and two anti-formant notches whose center frequencies interpolate across the place axis (/m/ 800/2700 Hz → /n/ 1700/3500 Hz → /ŋ/ 3200/4800 Hz). Two new knobs labeled **Nasality** and **Nasal Place** added to the Voice Character section of the UI. Default `nasalCoupling = 0` means existing presets are unaffected.
- **Nasal-aware voice shaping** — When `nasalCoupling > 0`, aspiration noise is suppressed (up to 80% at full coupling, since nasals are purely voiced), formant bandwidths widen up to 2× (nasal-cavity wall damping), and overall amplitude is reduced by up to 8 dB (nasals are ~6–10 dB quieter than vowels). All scaled linearly with coupling.

### Technical Notes
- New file `Source/dsp/NasalPoleZero.h` — three `FormantBiquad` stages in series (fixed-frequency nasal pole via `makeResonator` + two `juce::dsp::IIR::ArrayCoefficients<float>::makeNotch` anti-formants). `SmoothedValue` ramps for coupling (20 ms) and place sweeps (50 ms).
- **Transparency at coupling=0 via wet/dry mix**, not pole/zero cancellation: `output = input + mix * (wet - input)` where `mix = smoothedCoupling`. Guarantees bit-exact passthrough when the parameter is at default and eliminates the risk of imperfect cancellation in the filter chain. Cost: ~1 multiply-add per sample beyond the 3 biquads.
- **Integration at FormantVoice layer, not CascadeFormantBank** (deviation from original plan): the nasal stage runs on the final filter output after either the cascade or parallel path completes, so all three topologies (Cascade / Parallel / Hybrid) are handled uniformly without requiring a duplicate NasalPoleZero instance for the parallel path. Cleaner than dual-instance routing.
- APVTS: two new `AudioParameterFloat`s in a new "Nasal (2)" section of `createParameterLayout`; plugin now has 34 parameters (up from 32). No breaking changes — existing presets load with defaults (`nasalCoupling=0`, `nasalPlace=0.5`).
- WebView bindings: `nasalCouplingRelay` + `nasalPlaceRelay` in PluginEditor, matching JS relays in `main.js`, two new `knob-wrap` elements in `index.html` Voice Character section.
- `noteStarted()` calls `nasalPoleZero.reset()` and `snapToTargets()` for click-free onset.
- CPU: ~15 FLOPS/sample/voice (3 biquads + wet/dry mix), ≈0.3% single-core at 16 voices/48 kHz — negligible.
- CMakeLists VERSION 1.15.0 → 1.16.0.

## [1.15.0] - 2026-04-11

### Added
- **Liquid consonants /r/ and /l/ as morph targets** — Extended the 2D vowel morph space from 5 to 7 anchor points. Retroflex /r/ placed at (0.12, 0.72) with its characteristic very-low F3 (1600 Hz) signature, and dark (velarized) /l/ at (0.55, 0.85) with low F2 (900 Hz) and raised F3. Both are sonorants driven by the glottal source — no new DSP filters or parameters needed. Shepard IDW interpolation in `VowelMorpher` auto-adapts via `kNumVowels`, so sweeping the XY pad now smoothly transitions between vowels and liquid consonants.

### Technical Notes
- `VowelData.h`: `kNumVowels` 5→7, added R and L VowelEntry structs. /r/ formants F1=340, F2=1050, F3=1600, F4=3500, F5=4300 Hz (BW 60/90/130/250/280, gains 0/-8/-14/-24/-30 dB from Espy-Wilson 1992). /l/ dark F1=400, F2=900, F3=2600, F4=3400, F5=4200 Hz (BW 80/120/150/250/280, gains 0/-6/-16/-22/-28 dB from Stevens 1998).
- `VowelMorpher.h`: unchanged — already iterates over `VowelData::kNumVowels`.
- `Source/ui/public/js/main.js`: added 'r' and 'l' labels to `vowelLabels` array (XY pad rendering) and matching R/L entries to the `VOWELS` JS mirror array (spectrum display).
- No APVTS changes (32 params unchanged), existing presets load unaffected.
- CPU impact: negligible — Shepard IDW is O(N) over vowels (7 vs 5) at block rate.
- CMakeLists VERSION bumped 1.12.2 → 1.15.0 (CMake version field had drifted from CHANGELOG — aligning it).

## [1.14.1] - 2026-04-10

### Fixed
- **Cascade formant bank clipping** — Added dynamic gain normalization to the cascade (series) resonator bank. All-pole resonators with narrow bandwidths produced peak gains of 10–20× at formant frequencies, driving the per-voice `tanh()` soft-clipper into heavy saturation that sounded like hard clipping. The normalization estimates each resonator's peak gain from its bandwidth/frequency ratio and scales the cascade output by the inverse of the maximum, keeping levels in a range where `tanh()` provides gentle limiting rather than audible distortion. Smoothed over 10ms to prevent clicks during vowel transitions.

### Technical Notes
- Root cause: `makeResonator()` uses unity DC gain (`A = 1 − 2r·cos(θ) + r²`), but peak gain at resonance ≈ `A / (2(1−r)|sin(θ)|)` — ranges from 9× (F1, BW=60Hz) to 20× (F3–F5, BW=100–130Hz). Singer's Formant narrowing BW by 40% pushed peak gains to ~33×
- Fix in `CascadeFormantBank::updateCoefficients()`: computes `maxPeakGain` across all cascade stages, sets `normGainSmoothed` target to `1/maxPeakGain`
- Fix in `CascadeFormantBank::process()`: multiplies cascade output by `normGainSmoothed.getNextValue()`
- `normGainSmoothed`: 10ms ramp, snapped on note onset via `snapToTargets()`
- No new APVTS parameters (32 total unchanged), no breaking changes

## [1.14.0] - 2026-04-10

### Changed
- **Asymmetric triangular glottal noise envelope** — Replaced symmetric cosine window with a piecewise linear envelope matching real dual-peak glottal noise patterns. Linear ramp from 30% floor to peak over the open phase (0–0.6), brief noise burst at the glottal closure instant (0.6–0.65 rapid decay), then floor during closed phase. Produces more realistic aspirated-to-closed transitions than the previous smooth cosine.
- **Breathiness-dependent spectral tilt filter** — Added one-pole lowpass on aspiration noise output whose cutoff varies with the breathiness parameter: high breathiness → 2kHz cutoff (warm, airy turbulence), low breathiness → 6kHz cutoff (hissy, pressed character). Previously the noise was spectrally flat (white) before reaching the formant bank.
- **Stochastic breath drift** — Added slow random walk (~75ms update interval) that modulates noise amplitude by ±1–2dB and spectral tilt cutoff by ±200Hz. Two independent walks with SmoothedValue interpolation prevent the frozen-noise quality of deterministic envelopes. Real breath turbulence is non-stationary — this models that.

### Technical Notes
- All changes contained in `AspirationNoise.h` — no interface changes, `FormantVoice.cpp` unmodified
- Triangular envelope: `noiseFloor=0.3`, `openPhaseEnd=0.6`, `burstWidth=0.05` (5% of cycle)
- Spectral tilt: `cutoff = 6000 - breath * 4000 + driftHz`, one-pole α from `exp(-2πf/sr)`
- Drift: independent Xorshift random walks — amplitude (±0.5dB steps, ±2dB clamp), tilt (±50Hz steps, ±200Hz clamp), SmoothedValue ramps (50ms) for click-free interpolation
- No new APVTS parameters (32 total unchanged), no breaking changes

## [1.13.0] - 2026-04-10

### Added
- **Envelope-aware breath amplitude modulation** — Aspiration noise now varies by ADSR phase to model realistic vocal onset and release behavior:
  - **Attack onset (0–50ms):** Breath boosted +4.5dB (exponential decay, τ=15ms) simulating aspirated vocal fold engagement
  - **Sustain:** Breath at user-set level (unchanged behavior)
  - **Release onset (0–40ms):** Breath boosted +3dB (exponential decay, τ=12ms) simulating vocal fold disengagement, then fades with main envelope

### Technical Notes
- New `releaseSampleCount` member tracks ADSR phase (noteOn resets to -1, noteOff sets to 0)
- `breathEnvMul` computed per-block from elapsed time, applied to `effectiveBreath` before `setBreathiness()`
- Clamped to [0, 1] after multiplication — high breathiness settings saturate naturally at pure noise during onset
- No new APVTS parameters (32 total unchanged), no breaking changes

## [1.12.1] - 2026-04-07

### Changed
- **Consonant Level parameter range doubled** — Extended from 0.0–1.0 to 0.0–2.0 so consonants can be mixed louder. At max (2.0), consonant noise is twice the previous maximum amplitude. Existing presets (all ≤ 0.8) load unchanged. Signal path protected by per-voice `tanh()` soft-clip and brickwall limiter.

### Technical Notes
- `consonantLevel` NormalisableRange max changed from 1.0f to 2.0f in APVTS layout
- No new parameters (32 total unchanged), no breaking changes — stored parameter values remain valid

## [1.12.0] - 2026-04-07

### Added
- **Singer's Formant control** — Models the trained operatic singing voice phenomenon (Sundberg) where the pharynx-to-epilarynx tube ratio causes F3, F4, and F5 to cluster into a single reinforced spectral peak around 2.5-3.5 kHz, enabling vocal projection over orchestral accompaniment.
- **New APVTS parameter: `singersFormant`** (float 0-1, default 0) — At 0 no effect. At 1.0, F3/F4/F5 frequencies are pulled toward 3000 Hz with per-formant cluster strengths (F3: 0.7, F4: 0.8, F5: 0.6 — F4 clusters most strongly as the primary contributor). Bandwidths of F3-F5 narrow by up to 40% to sharpen the cluster peak. Gains of F3-F5 boosted by up to +4 dB to model acoustic reinforcement.
- **UI: Singer's F knob** — Added to the Character parameter group in the WebView UI.
- **Factory presets updated** — 5 presets include characterful singer's formant values: Overtone Chant=0.7, Pressed Baritone=0.7, Natural Tenor=0.5, Sci-Fi Choir=0.4, Breathy Soprano=0.3. Remaining presets default to 0 (speech-like character).

### Technical Notes
- Clustering applied in FormantVoice::renderNextBlock at block rate (every 32 samples) AFTER vowel morph + dynamic bandwidth variation, BEFORE passing to filter banks — both parallel and cascade topologies benefit
- Formula: `F_clustered = F_base + singersFormant * (3000 - F_base) * clusterStrength[i]`
- Bandwidth narrowing: `BW *= (1.0 - singersFormant * 0.4)`
- Gain boost: `gain *= dB_to_linear(4.0 * singersFormant)`
- 1 new APVTS parameter (32 total), no breaking changes

## [1.11.1] - 2026-04-07

### Fixed
- **Held tone now audible in Cascade/Hybrid topology** — The cascade formant bank was using `makeBandPass` (zero-pole BPF) instead of all-pole resonators for series-chained filters. Bandpass filters at different center frequencies (F1=700Hz, F2=1200Hz, etc.) have non-overlapping passbands, so cascading them produced ~60+ dB cumulative attenuation — effectively silence for the voiced path. Consonant attack was still audible because consonants route through the parallel bank, which sums BPF outputs correctly.
- **Root cause:** `CascadeFormantBank` used `juce::dsp::IIR::ArrayCoefficients::makeBandPass` which creates a filter with zeros at DC and Nyquist, explicitly cutting frequencies outside the passband. Klatt cascade synthesis requires all-pole resonators that add peaks without cutting other frequencies.
- **Fix:** Replaced `makeBandPass` with custom `makeResonator()` — all-pole second-order resonator with unity DC gain normalization (`A = 1 - 2r·cos(θ) + r²`). Cascade filters now correctly shape the broadband glottal excitation into vowel spectra. Hybrid mode parallel filters (F4-F5) retain BPF for proper band isolation.
- **Removed 12 dB compensation gain** — No longer needed since resonators don't attenuate the signal like bandpass filters did.

### Technical Notes
- `CascadeFormantBank::makeResonator(sr, freq, bw)` — static method computing all-pole resonator coefficients: `r = exp(-π·BW/Fs)`, `θ = 2π·F/Fs`, feedback coeffs `a1 = -2r·cos(θ)`, `a2 = r²`, feedforward `b0 = A, b1 = b2 = 0`
- Pole radius clamped to r ≤ 0.9999 for stability; frequency and bandwidth clamped to safe ranges
- Both `updateCoefficients()` and `process()` (smoothing path) use resonators for cascade indices and BPF for parallel indices
- Existing `tanh()` soft-clip in FormantVoice handles resonator peak amplitudes
- No parameter changes (31 total unchanged), no breaking changes

## [1.11.0] - 2026-04-07

### Added
- **Source-filter coupling** — Subtle harmonic reinforcement approximation inspired by Titze (2008, JASA). At block rate, checks if harmonics 2f0–4f0 fall within ±bandwidth of F1 or F2. When a harmonic is near a formant peak, applies a smoothed gain boost (up to +2 dB) scaled by proximity: `boost = 2dB * (1 - |harmonic - formantFreq| / bandwidth) * coupling`. Also increases pitch jitter by up to +0.3% when harmonics cross formant boundaries, simulating the f0 instabilities documented in source-filter interaction research.
- **New APVTS parameter: `sourceFilterCoupling`** (float 0-1, default 0.3) — Scales both the harmonic reinforcement gain and the jitter modulation. At 0 the effect is disabled (pure linear source-filter model). At 1 full +2 dB boost and +0.3% jitter when harmonics align with formants.
- **Factory presets updated** — All 16 presets include characterful coupling values (e.g., Robotic Speech=0.0 disabled, Overtone Chant=0.7 strong reinforcement, Formant Bass=0.6, Glitch Vocal=0.1 minimal).

### Technical Notes
- FormantVoice: block-rate proximity detection (every 32 samples) scans harmonics 2-4 against F1/F2 using `formantFreqs[]`/`formantBWs[]`; best proximity across all pairs drives both gain and jitter
- Gain applied via `SmoothedValue<float>` (10ms ramp) between formant filter output and `tanh()` soft-clip
- Jitter boost additive to existing VibratoLFO micro-jitter offset in per-sample pitch computation
- F0 estimate for block-rate check uses `currentlyPlayingNote.getFrequencyInHertz()` (MIDI note frequency, adequate accuracy for proximity detection)
- 1 new APVTS parameter (31 total), no breaking changes

## [1.10.0] - 2026-04-07

### Changed
- **Pitch-synchronous aspiration noise** — Breathiness is no longer constant white noise. Aspiration amplitude is now modulated by the glottal cycle phase, peaking during the open phase (~0.0–0.6) and dipping during the closed phase (~0.6–1.0). Uses a cosine window centered at phase=0.3 with a 30% floor: `noiseGain = 0.3 + 0.7 * (0.5 + 0.5 * cos(2π(phase - 0.3)))`. This makes breathiness sound throaty and organic rather than like added static.
- **Upgraded aspiration noise filter** — Replaced single-pole IIR lowpass at 4kHz with a biquad bandpass filter centered at 3kHz (Q=1.0, ~3kHz bandwidth) to better match real aspiration spectra. Uses transposed direct form II for numerical stability.

### Technical Notes
- LFGlottalSource: added `getPhase()` public getter exposing the [0,1) phase accumulator
- AspirationNoise: added `setGlottalPhase(float)` for per-sample phase injection; replaced `lpCoeff`/`prevFilterOutput` with biquad state (`bpB0/B1/B2`, `bpA1/A2`, `bpZ1/Z2`); `computeBandpassCoeffs()` derives coefficients from Audio EQ Cookbook BPF formula
- FormantVoice: calls `aspirationNoise.setGlottalPhase(glottalSource.getPhase())` each sample after `getNextSample()`
- No new parameters (30 total unchanged), no breaking changes

## [1.9.0] - 2026-04-07

### Added
- **Smooth formant transitions with configurable timing** — Per-formant `SmoothedValue<float>` objects (5 frequencies + 5 bandwidths = 10 total) provide sample-rate interpolation when vowel morph position changes. Eliminates step-function jumps from the previous 32-sample block-rate updates. Transition times differ per formant to mimic real articulatory dynamics:
  - F1: up to 50ms (jaw — fastest articulator)
  - F2-F3: up to 80ms (tongue body — medium)
  - F4-F5: up to 120ms (slowest articulatory gestures)
- **New APVTS parameter: `transitionTime`** (float 0-1, default 0.4) — Scales per-formant ramp durations. At 0 = instant transitions (backward compatible with previous behavior). At 1 = maximum transition times (F1: 50ms, F2-F3: 80ms, F4-F5: 120ms). The formula: `rampTime = transitionTime * perFormantMaxTime`.
- **Zero-overhead steady state** — Biquad coefficients are only recomputed per-sample when `SmoothedValue::isSmoothing()` returns true. When formants are stable, processing cost is identical to previous version.
- **Click-free note onset** — SmoothedValues snap to current targets via `setCurrentAndTargetValue()` on `noteStarted()`, preventing formant smearing at note attack.
- **Both filter topologies supported** — Smoothing applied to both `FormantFilterBank` (parallel) and `CascadeFormantBank` (cascade/hybrid) with identical per-formant timing schedules.
- **UI: Transition knob** — Added to the Character parameter group in the WebView UI.
- **Factory presets updated** — All 16 presets include characterful `transitionTime` values (e.g., Robotic Speech=0.0 instant, Ethereal Drone=0.8 slow morph, Glitch Vocal=0.1 near-instant).

### Technical Notes
- FormantFilterBank: `setTransitionTime(float)` configures 10 SmoothedValues with per-formant ramp durations; `snapToTargets()` for note onset; `updateCoefficients()` sets targets instead of direct application; `process()` advances smoothing and recomputes biquad coefficients per-sample only during active transitions
- CascadeFormantBank: identical smoothing treatment for cascade/hybrid topologies
- FormantVoice: reads `transitionTime` at block rate, calls `setTransitionTime()` on both banks each block, snaps on noteStarted
- 1 new APVTS parameter (30 total), with WebView relay + attachment, pluginval pending

## [1.8.0] - 2026-04-07

### Added
- **Cascade formant filter topology** — Klatt-style series resonator bank as alternative to existing parallel topology. Cascade chains 5 bandpass filters in series (F1→F2→F3→F4→F5), automatically producing correct relative formant amplitudes without per-formant gain control (Klatt, 1980). This is the most physically accurate model for vowel synthesis.
- **New APVTS parameter: `formantTopology`** (Choice: Cascade/Parallel/Hybrid, default Cascade) — Three topology modes:
  - *Cascade* (0): All 5 formants in series for voiced path. Correct 1/f spectral envelope slope, natural amplitude relationships. Consonant noise routed through parallel bank.
  - *Parallel* (1): Legacy behavior — voice and consonant mixed through 5 parallel bandpass filters with explicit per-formant gains. Preserves backward compatibility with existing presets.
  - *Hybrid* (2): F1-F3 in cascade (low formants benefit most from series topology), F4-F5 in parallel. Best of both worlds.
- **Split voiced/consonant signal paths** — In Cascade and Hybrid modes, glottal source routes through cascade bank while consonant noise routes independently through parallel bank. Consonants need per-formant gain control that parallel provides. Spectral tilt applied only to voiced path (physically correct — models glottal spectral slope, not vocal tract).
- **Gain compensation** — Cascade output boosted ~12 dB (5 stages) or ~7 dB (3 stages in hybrid) to compensate for series filter attenuation. Uses `2^(numStages * 0.4)` scaling.
- **UI: Topology selector** — Segmented control (Cascade/Parallel/Hybrid) in the Character parameter group.
- **Factory presets updated** — All 16 presets include `formantTopology` set to Cascade for improved vowel naturalness.

### Technical Notes
- New class `CascadeFormantBank` in `Source/dsp/CascadeFormantBank.h` — reuses `FormantBiquad` struct, shares coefficient computation with `FormantFilterBank` but sets per-filter gain to unity
- `CascadeFormantBank::process()` chains first N filters in series, sums remaining in parallel (supports both full cascade and hybrid)
- `setNumCascadeStages(n)` configures cascade/hybrid split and auto-computes compensation gain
- FormantVoice topology routing: block-rate parameter read, per-sample branched signal flow
- Topology=1 (Parallel) uses exact legacy code path for backward compatibility
- WebView: `WebComboBoxRelay` + `WebComboBoxParameterAttachment` for topology selector
- 1 new APVTS parameter (29 total), pluginval validated at strictness 5

## [1.7.0] - 2026-04-06

### Added
- **Spectral tilt filter** — Independent voice brightness/darkness control decoupled from Rd voice quality. One-pole IIR filter between source and formant filter bank shapes the H1-H2 balance (the #1 perceptual correlate of phonation type per Kreiman et al., 2015).
- **New APVTS parameter: `spectralTilt`** (float -12 to +12 dB, default 0, label "Spectral Tilt") — Positive values attenuate upper harmonics (darker/breathier), negative values boost upper harmonics relative to fundamental (brighter/more pressed). Enables combinations like "breathy but bright" or "pressed but dark" that the Rd parameter alone cannot produce.
- **UI: Tilt knob** — Added to the Glottal Source parameter group in the WebView UI.
- **Factory presets updated** — All 16 presets include characterful spectralTilt values (e.g., Creature Growl=-3dB bright+pressed, Alien Whisper=+4dB dark+breathy, Breath Texture=+5dB very dark).

### Technical Notes
- One-pole lowpass: `lp = (1-alpha)*x + alpha*prev`, cutoff at 2*f0, alpha updated at block rate from current pitch
- Unified tilt formula: `output = x - tiltNorm * (x - lp)` handles both positive (blend toward lowpass) and negative (boost highpass complement) in a single expression
- tiltNorm = tiltDdB / 12.0 normalizes range to [-1, +1]; at 0 the filter is transparent (output = input)
- Filter state reset on noteStarted() for click-free onset
- 1 new APVTS parameter (28 total), with WebView relay + attachment

## [1.6.0] - 2026-04-06

### Added
- **Dynamic formant bandwidth variation** — Bandwidths now modulate based on vowel openness and breathiness instead of remaining fixed from VowelData interpolation. Two modulation sources applied at block rate after Shepard interpolation:
  - *Vowel openness:* F1 frequency used as proxy for jaw opening. B1 scaled by `(1.0 + 0.4 * (F1 - 400) / 800)`, clamped to [40, 200] Hz. B2-B5 receive 30% of B1's scaling. Open vowels (/a/) get wider B1 (~+12%), closed vowels (/i/) slightly narrower (~-4%).
  - *Breathiness coupling:* All BW1-BW5 scaled by `(1.0 + breathiness * 0.5)`, giving breathy voices up to 50% wider bandwidths for a more diffuse, airy resonance.

### Technical Notes
- No new parameters — uses existing `breathiness` (0-1) and derived F1 from vowel morpher output
- Modulation inserted in FormantVoice::renderNextBlock between vowelMorpher.compute() and filterBank.updateCoefficients(), preserving existing shift/spread/Q pipeline
- Perceptual effect: breathy voices sound more diffuse; open vowels have warmer, less resonant quality (ref: Fleischer et al., 2015 vocal tract measurements)
- 27 APVTS parameters unchanged

## [1.5.0] - 2026-04-06

### Added
- **Dynamic Rd (vocal effort) modulation** — Glottal source Rd now responds automatically to pitch, velocity, and expression instead of remaining static per voice. Three modulation sources combined at block rate:
  - *Pitch tracking:* -0.3 Rd per octave above middle C (higher pitch = more pressed voice)
  - *Velocity mapping:* MIDI velocity 0-127 scales to 0 to -0.5 Rd offset (harder attack = more effort)
  - *Expression:* MPE pressure / aftertouch maps 0-1 to +/-0.4 Rd offset (continuous effort control)
- **New APVTS parameter: `rdModDepth`** (float 0-1, default 0.5) — Master depth control that scales all three Rd modulation sources. At 0 the voice quality knob behaves exactly as before (static Rd). At 1 full dynamic modulation is applied.
- **20ms SmoothedValue ramp** — Per-sample linear smoothing on the effective Rd prevents clicks during rapid modulation changes.
- **UI: Rd Mod knob** — Added to the Glottal Source parameter group in the WebView UI.
- **Factory presets updated** — All 16 presets include characterful rdModDepth values (e.g., Robotic Speech=0.0, Natural Tenor=0.6, Pressed Baritone=0.7).

### Technical Notes
- FormantVoice: effective Rd computed at block rate from base knob + modDepth * (pitch + velocity + expression offsets), clamped to valid LF model range [0.3, 2.7]
- Per-sample `glottalSource.setRd(rdSmoothed.getNextValue())` replaces previous block-rate static setRd
- Note onset initializes rdSmoothed with `setCurrentAndTargetValue()` for click-free first block
- 1 new APVTS parameter (27 total), with WebView relay + attachment

## [1.4.0] - 2026-04-06

### Added
- **Jitter and shimmer modeling** — Per-cycle random perturbation of f0 (jitter) and amplitude (shimmer) in the LF glottal source for vocal naturalness. Uses 1/f noise approximation via EMA-filtered random values (~50ms time constant). Perturbations applied once per glottal cycle at phase wrap, not per-sample.
- **Inverse scaling** — Both jitter and shimmer scale inversely with pitch (more perturbation at low f0, ref 200Hz) and inversely with velocity (controlled singing = less perturbation).
- **Two new APVTS parameters** — `jitter` (0-1, default 0.15, maps to 0-2% relative f0 perturbation) and `shimmer` (0-1, default 0.1, maps to 0-5% relative amplitude perturbation). Defaults sit within healthy voice norms (<1% jitter, <3% shimmer).
- **Per-voice RNG seeding** — Each voice gets a unique Xorshift32 seed for uncorrelated perturbation patterns across polyphonic notes.
- **UI: Jitter and Shimmer knobs** — Added to the Glottal Source parameter group.
- **Factory presets updated** — All 16 presets now include characterful jitter/shimmer values (e.g., Robotic Speech=0/0, Creature Growl=0.3/0.3, Natural Tenor=0.15/0.1).

### Technical Notes
- LFGlottalSource: `setJitterShimmer()` and `setSeed()` methods, cycle detection via phase accumulator wrap
- Jitter modifies `phaseIncrement` multiplier per cycle; shimmer modifies output gain multiplier per cycle
- EMA alpha computed from cycle period: `alpha = 1 - exp(-cyclePeriod / 0.050)`
- 2 new APVTS parameters (26 total), with WebView relays + attachments

## [1.3.0] - 2026-04-06

### Added
- **Manual consonant envelope parameters** — Three new knobs (Cons Attack 1-100ms, Cons Hold 0-200ms, Cons Decay 5-200ms) for user control of the consonant envelope when Auto is off. When Auto is on, timing is derived from manner parameter as before.
- **Consonant envelope always triggers at note onset** — Both auto and manual modes now trigger the consonant envelope on every note. Auto toggle switches between manner-derived timing and user knob timing.
- **UI: consonant envelope knobs** — Three small knobs appear in the consonant section, dimmed when Auto is active (timing from manner), fully interactive when Auto is off.

### Changed
- **autoConsonant parameter behavior** — Now toggles between auto-derived envelope timing (from manner) and manual envelope timing (from knobs). Both modes have transient consonant behavior. Previously, turning auto off meant continuous noise with no independent envelope.

### Technical Notes
- ConsonantEngine: new `setManualEnvelope()` overrides cached attack/hold/decay sample counts
- `getNextSample()` signature simplified — removed `autoConsonant` parameter, envelope always applied
- `triggerBurst()` always called at note onset regardless of auto setting
- 3 new APVTS parameters: consonantAttack, consonantHold, consonantDecay (24 total)

## [1.2.0] - 2026-04-06

### Changed
- **Dedicated consonant envelope independent of main ADSR** — Consonants now have their own 3-phase envelope (Attack/Hold/Decay) triggered at note onset, with timing derived from manner parameter. Plosives: 1ms attack, 0ms hold, 15ms decay (~16ms total). Fricatives: 40ms attack, 60ms hold, 40ms decay (~140ms total). Values informed by Klatt synthesis research and measured speech data.
- **Split voice/consonant envelope paths** — Main ADSR envelope now applies only to the voiced glottal source. Consonant noise uses its own internal envelope when autoConsonant is enabled, preventing slow ADSR attacks from destroying consonant transients. When autoConsonant is off, consonant noise still follows the main ADSR for continuous texture use.
- **Consonants are now transient speech events** — With autoConsonant enabled, consonant output decays to zero after the envelope completes (16-140ms depending on manner), matching natural speech consonant durations instead of sustaining indefinitely.

### Technical Notes
- ConsonantEngine: new `EnvPhase` state machine (Off/Attack/Hold/Decay) with `advanceEnvelope()` per-sample processing
- Envelope timing computed in `updateCoefficients()` from manner parameter, cached as sample counts
- FormantVoice signal path: ADSR applied to voiceSource before mixing with consonant, both still routed through shared formant filter bank for vocal tract resonance
- ADSR before filter is more physically accurate — models glottal amplitude variation feeding into vocal tract resonator

## [1.1.1] - 2026-04-05

### Fixed
- **Output safety: Q clamping + bandwidth scaling** — Formant filter Q was unbounded (reaching 549 at extreme shift/spread), producing ~148 dB above 0dBFS. Bandwidth now scales proportionally with shift factor, Q clamped to [0.5, 25].
- **Per-voice soft clip** — Added `tanh()` saturation after formant filtering to prevent extreme amplitudes from resonant filters. Transparent at normal levels.
- **Consonant output hard clamp** — ConsonantEngine output now clamped to [-1, 1] before entering formant filter bank.
- **Brickwall limiter in processBlock** — Final hard clip at 0 dBFS after output gain as last line of defense against dangerous levels.

## [1.1.0] - 2026-04-05

### Changed
- **Consonant engine replaced with place/manner articulation model** — Previous LP/HP crossfade + sibilance bandpass produced indistinct continuous noise. New engine uses dual resonant bandpass filters shaped by place-of-articulation (X axis: Labial 500Hz -> Alveolar 3kHz -> Palatal 6kHz -> Velar 2kHz) with manner-dependent temporal profiles (Y axis: Plosive short burst with glottal mute -> Fricative sustained noise). Root cause: old architecture had no spectral place modeling and fixed temporal shape regardless of consonant type.
- **Consonant XY pad replaces Tone/Sibilance knobs** — New interactive 2D pad for place (X) and manner (Y) control with frequency readout, place labels (Lab/Alv/Pal/Vel), and manner labels (Plos/Fric). Matches vowel XY pad visual style.
- **Parameter display names updated** — "Consonant Tone" -> "Place", "Sibilance" -> "Manner" (APVTS IDs unchanged, existing automation compatible)
- **All 16 factory presets updated** — Each preset now has meaningful place/manner values matching its character (e.g., Creature Growl = velar plosive, Robotic Speech = alveolar fricative)
- **Default sibilance (manner) changed from 0.0 to 0.5** — Mid-manner default produces a balanced blend of burst and sustained noise instead of pure plosive

### Technical Notes
- Onset suppression now scales with (1 - manner): full glottal mute for plosives, none for fricatives
- Burst duration varies with manner: 8ms (plosive) to 80ms (fricative)
- Burst decay rate varies with manner: exp(-12t) (plosive, sharp) to exp(-2t) (fricative, gentle)
- Place Q varies: Labial Q=1.5 (broad) -> Alveolar Q=4.0 (tight) -> Palatal Q=3.0 -> Velar Q=2.0
- Preserves v1.0.1 consonant-through-formant routing and onset suppression architecture

## [1.0.1] - 2026-04-05

### Fixed
- **Consonant noise now routed through formant filter bank** — Previously consonant noise bypassed the vocal tract model and was mixed directly into the output, producing raw noise instead of speech-like consonants. Now passes through the same 5-formant parallel BPFs as the glottal source, giving consonants proper vocal tract resonance.
- **Plosive burst sharpened** — Reduced burst duration from 15ms to 8ms with faster exponential decay (exp(-10t) vs exp(-5t)) for more realistic plosive transients.
- **Glottal source suppression during plosive onset** — Added 25ms onset envelope that briefly suppresses the glottal source (70% at peak) during auto-consonant bursts, letting noise dominate at attack before voice takes over. Simulates vocal fold closure during /p/, /t/, /k/ consonants.

## [1.0.0] - 2026-04-05

### Added
- **Physical model vocal synthesizer** - LF glottal pulse model with Fant 1995 Rd voice quality control (0.3-2.7)
- **5-formant parallel bandpass filter bank** - Vocal tract modeling with per-formant frequency, bandwidth, and gain
- **2D vowel morph pad** - XY cursor with 5 cardinal vowels at acoustic positions, Shepard IDW interpolation
- **Consonant noise engine** - KLATT dual-branch topology with tone shaping, sibilance, and auto-consonant plosive burst
- **Vibrato system** - Per-voice sine LFO with rate, depth, onset delay, and micro-jitter
- **Pitch glide** - Exponential portamento between notes (0-1000ms)
- **ADSR envelope** - Per-voice amplitude envelope with full parameter automation
- **16-voice polyphony** - MPE-ready via juce::MPESynthesiser with legacy MIDI fallback
- **MPE support** - Pressure->breathiness, slide->vowel Y, velocity->attack character
- **Formant shift and spread** - Semitone-based frequency shifting and center-of-mass spacing control
- **Output stage** - Smoothed output gain (dB) and per-voice stereo width (equal-power pan by MIDI note)
- **WebView UI** - Naturalist aesthetic with XY vowel pad, formant overlay, organized parameter sections
- **16 factory presets** - 4 categories (Cinematic, Electronic, Ambient, Speech), 4 presets each
- **Preset browser** - OuariconPresetManager with prev/next, category dropdown, save/load
- **Mipmapped glottal wavetable** - 128 Rd steps x 10 mipmap levels, FFT-based anti-aliasing

### Technical Notes
- Domain: C++ DSP + WebView UI
- 21 parameters across 7 groups (Vowel, Glottal, Consonant, Envelope, Character, Output, Control)
- JUCE 8.0.4, CMake + Ninja build system
- VST3 + AU formats, macOS
- Pluginval validated at strictness level 10
