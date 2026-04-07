# O-Wind Changelog

## [1.10.0] - 2026-04-06

### Added — Allpass Inharmonicity Filters

Bore waveguide allpass inharmonicity for natural partial detuning and conical bore approximation per RESEARCH-realism-v2.md §2.6:

**New APVTS Parameter:**
- `inharmonicity` (0.0-1.0, default 0.2) — controls allpass partial detuning amount

**DSP Implementation (BoreWaveguide — processSample):**
- Two cascaded first-order allpass filters in bore backward delay path (after end reflection, after backward delay pop)
- Allpass coefficient `a = effective * 0.05`, where `effective = APVTS_param * preset.inharmonicityBase`
- Frequency-dependent phase delay detunes upper harmonics by ~5-15 cents — matching measured flute inharmonicity
- Coefficients updated once per block via `setInharmonicity(effective)` with change detection
- Allpass phase delay included in `getFilterPhaseDelay()` for dynamic loop delay compensation
- Zero CPU cost when inharmonicity = 0 (filters bypassed)

**Per-Instrument Preset Base Values (InstrumentPresets — inharmonicityBase):**
- Concert Flute: 0.15 (cylindrical bore, minimal inharmonicity)
- Shakuhachi: 0.5 (conical bore, high inharmonicity)
- Bansuri: 0.35 (bamboo bore irregularities)
- Native Am. Flute: 0.4 (dual-chamber conical bore)
- Recorder: 0.3 (slightly tapered bore)
- Pan Flute: 0.35 (closed-end cylindrical, end correction effects)
- Piccolo: 0.2 (short conical bore)
- Ocarina: 0.4 (Helmholtz resonator, inherently inharmonic)

**Files Modified:** BoreWaveguide.h, InstrumentPresets.h, PluginProcessor.cpp, FluteSynthVoice.cpp

## [1.9.0] - 2026-04-06

### Added — Register-Dependent Spectral Shaping

Automatic pitch-aware timbral adaptation per RESEARCH-realism-v2.md §2.5:

**Bore Loss Filter — Register Cutoff Modulation (FluteSynthVoice — updateParametersFromAPVTS):**
- Cutoff multiplier formula: `0.6 + (currentMidiNote / 127.0) * 0.8`
- Applied to toneColor-derived cutoff after material scaling, before `boreWaveguide.updateBoreLossFilter()`
- Low notes (C3, MIDI 48): ~0.9x multiplier — richer harmonic content relative to fundamental
- High notes (C7, MIDI 96): ~1.2x multiplier — purer tone relative to fundamental
- Smooth continuous scaling across full MIDI range

**Breath Noise — Inverse Register Scaling:**
- Noise multiplier formula: `1.4 - (currentMidiNote / 127.0) * 0.8`
- Applied to breathNoise parameter before `jetExciter.setBreathNoise()`
- Low notes: ~1.1x noise gain — more audible breath turbulence
- High notes: ~0.8x noise gain — cleaner, less breathy tone
- Symmetric inverse of bore loss scaling

**No new APVTS parameters** — fully automatic behavior driven by current MIDI note number. Zero additional CPU cost (two multiply-adds per block).

## [1.8.0] - 2026-04-06

### Added — Material Macro Parameter

Continuous wood-to-metal timbral blending macro per RESEARCH-realism-v2.md §2.3:

**New APVTS Parameter:**
- `material` (0.0-1.0, default 0.5) — timbral macro (0 = dark wood/bamboo, 1 = bright metal)

**DSP Implementation (FluteSynthVoice — updateParametersFromAPVTS):**
- Bore loss filter cutoff multiplier: 0.6x (wood) to 1.4x (metal) of base toneColor cutoff
- Strouhal noise bandpass center freq: same 0.6x-1.4x scaling (darker/brighter turbulence)
- Radiation filter cutoff: 0.7x (wood) to 1.3x (metal) of preset base cutoff
- End reflection filter cutoff: same 0.7x-1.3x scaling as radiation
- End reflection coefficient: nudged +/-0.05 from base (metal = more reflective, wood = more damped)
- No new DSP components — multipliers applied in updateParametersFromAPVTS before passing to existing filters
- Works as additive offset on top of instrument presets (material=0.5 = no change from preset base)

**WebView UI:**
- New "Material" knob added to Resonator section on SOUND tab (first position)
- All 8 factory presets updated (material=0.5 default — neutral, no timbral offset)

## [1.7.0] - 2026-04-06

### Added — Headjoint Formant Resonance Filter

Models the characteristic resonant peak of the headjoint/embouchure cavity in the radiation output path per RESEARCH-realism-v2.md §2.4:

**New APVTS Parameter:**
- `formant` (0.0-1.0, default 0.5) — formant resonance prominence (0 = flat/0dB, 1 = full/+6dB)

**DSP Implementation (PluginProcessor):**
- Stereo IIR biquad parametric EQ (Q = 1.5) applied post-StereoWidth, pre-output
- Center frequency is preset-dependent via new `formantCenterHz` field in InstrumentPreset:
  - Piccolo: 4000 Hz, Recorder: 3000 Hz, Concert Flute: 2500 Hz
  - Pan Flute: 2200 Hz, Bansuri: 2000 Hz, Ocarina: 2000 Hz
  - Shakuhachi: 1800 Hz, Native Am. Flute: 1500 Hz
- Gain mapped linearly: formant=0 → 0dB (flat), formant=1 → +6dB
- Coefficients updated only on parameter or preset change (not per-sample)
- Processing skipped entirely when gain < 0.05dB (formant near 0)

**WebView UI:**
- New "Formant" knob added to Output section on SOUND tab
- All 8 factory presets updated (formant=0.5 default)

## [1.6.0] - 2026-04-06

### Added — Growl Effect (Vocal-Fold Coupling)

Models growl/roughness via secondary low-frequency sawtooth oscillator modulating bore feedback per RESEARCH-realism-v2.md §2.2:

**New APVTS Parameter:**
- `growl` (0.0-1.0, default 0.0) — growl depth (0 = off, 1 = full roughness)

**DSP Implementation (FluteSynthVoice):**
- Sawtooth oscillator (70-120 Hz, randomized per-note) modulates bore feedback signal
- Formula: `boreFeedback *= (1.0 - growl * 0.6 * sawPhase)` where `sawPhase` ramps 0→1
- At growl=0: multiplier is 1.0 (no effect, zero CPU cost via early-exit)
- At growl=1: bore feedback reduced up to 60% at sawtooth peak, creating characteristic roughness
- Per-note frequency randomization (70-120 Hz range) + random start phase via per-voice RNG
- Simulates vocal-fold coupling where sub-glottal turbulence modulates the air column

**WebView UI:**
- New "Growl" knob added to Expression section on SOUND tab
- All 8 factory presets updated (growl=0 — articulation effect, not default sound)

## [1.5.0] - 2026-04-06

### Added — Flutter Tongue Articulation

Models flutter tongue (Flatterzunge) via amplitude modulation of breath pressure per RESEARCH-realism-v2.md §2.1:

**New APVTS Parameters:**
- `flutterTongue` (0.0-1.0, default 0.0) — flutter tongue AM depth (0 = off, 1 = full modulation)
- `flutterRate` (15-30 Hz, default 22 Hz) — flutter tongue oscillation rate

**DSP Implementation (JetExciter):**
- In `processSample()`, effectivePressure is multiplied by `(1 - ft + ft * (0.5 + 0.5 * sin(phase)))` where `ft` = flutterTongue
- At flutterTongue=0: multiplier is 1.0 (no effect, zero CPU cost via early-exit)
- At flutterTongue=1: full AM from 0 to 1× breath pressure at flutter rate
- Per-cycle rate randomization (+/-5%) applied at each phase wraparound for naturalism
- Jittered rate stored per-voice, no per-sample random calls

**WebView UI:**
- Two new knobs added to Expression section on SOUND tab: "Flutter" and "Flut Rate"
- All 8 factory presets updated (flutterTongue=0, flutterRate=22 Hz — articulation, not default sound)

## [1.4.0] - 2026-04-06

### Added — Per-Note Humanization System

Eliminates "machine gun" effect on repeated notes per RESEARCH-realism-v2.md §1.3:

**New APVTS Parameter:**
- `humanize` (0.0-1.0, default 0.3) — master scale for all per-note randomization amounts

**Per-Note Random Offsets (drawn at each noteOn via per-voice juce::Random):**

| Offset | Range | Effect |
|--------|-------|--------|
| Attack time | +/-20% of base | No two attacks identical |
| Noise burst amplitude | +/-30% of chiff level | Varied chiff intensity |
| Embouchure delay | +/-1% of bore delay | Slight timbre shift per note |
| Strouhal noise center | +/-10% of center freq | Subtle breath color variation |
| Vibrato onset delay | +/-50ms | Natural onset variation |

**Implementation Details:**
- Offsets stored as member variables in FluteSynthVoice, applied continuously per note
- Attack time and noise burst scales passed to JetExciter::startNote()
- Embouchure offset applied as bore delay multiplier in render loop
- Strouhal freq scale passed to JetExciter::updateStrouhalBandpass()
- Vibrato onset offset added to base vibratoOnset parameter (clamped >= 0)
- All offset magnitudes scale linearly with humanize parameter (0 = no randomization)
- Zero per-sample CPU cost — random numbers drawn only at noteOn

## [1.3.0] - 2026-04-06

### Improved — Vibrato Humanization

Replaces mechanical sine-wave LFO vibrato with organic, human-like modulation per RESEARCH-realism-v2.md §1.2:

**Delayed Onset (FluteSynthVoice):**
- Vibrato depth ramps linearly from 0 to target over configurable onset delay (0-1000ms)
- Prevents instant vibrato on note attack — matches real flautist technique where vibrato develops after tone stabilizes

**Rate Drift:**
- Slow oscillator (~0.47 Hz) modulates LFO rate by +/- 0.75 Hz
- Per-note random drift phase eliminates locked periodicity between notes

**Depth Drift:**
- Independent slow oscillator (~0.31 Hz) modulates vibrato depth +/- 25%
- Creates natural amplitude variation in vibrato intensity

**Shape Asymmetry:**
- Vibrato waveform changed from `sin(phase)` to `sin(phase) + 0.1*sin(2*phase)`
- Adds slight second-harmonic content matching diaphragm-driven vibrato asymmetry

**Random Initial Phase:**
- Vibrato LFO starts at random phase on each noteOn
- Eliminates phase-locked vibrato across simultaneous or sequential notes

**New APVTS Parameter:**
- `vibratoOnset` (0-1000ms, default 300ms) — delay before vibrato ramp-in after noteOn

**Factory Preset Values:**
- Shakuhachi: 500ms, Pan Flute: 450ms, Native Am. Flute: 400ms, Bansuri: 350ms, Ocarina: 350ms
- Concert Flute: 300ms (default), Piccolo: 250ms, Recorder: 200ms

## [1.2.0] - 2026-04-06

### Added — Attack Transient "Chiff" Modeling

Models the four-phase flute onset (noise burst, vortex shedding, edge-tone, pipe-tone) per Auvray et al. (2014):

**Chiff Noise Burst (JetExciter):**
- On noteOn, turbulence noise is boosted 3-6x above steady-state level (scaled by `attackChiff * velocity`)
- Exponential decay over 20-40ms (faster at higher velocity) back to normal noise floor
- Near-zero CPU cost — envelope-gated, active only during first ~40ms per note

**Pitch Overshoot (FluteSynthVoice):**
- Bore delay starts 1-2% shorter than target at noteOn (sharper pitch), scaled by `attackChiff * velocity`
- Settles to target pitch over 50-100ms via one-pole smoothing filter
- Models jet-bore coupling delay before pipe-tone regime locks

**New APVTS Parameter:**
- `attackChiff` (0.0-1.0, default 0.5) — controls transient noise burst intensity and pitch overshoot amount

**Factory Preset Values:**
- Shakuhachi: 0.70 (prominent chiff), Pan Flute: 0.65, Bansuri: 0.55
- Concert Flute: 0.50, Piccolo: 0.45, Native Am. Flute: 0.40
- Recorder: 0.25, Ocarina: 0.15 (minimal chiff)

## [1.1.0] - 2026-04-06

### Improved — Noise Model & Spectral Realism

Three-pass physical model refinement for more realistic flute timbre:

**Pass A: Jet-Bore Energy Balance**
- Replaced single 2nd-order lowpass bore loss filter with two cascaded 1st-order lowpass filters (~2kHz base damping + ~8kHz harmonic rolloff) for frequency-dependent viscothermal loss — higher harmonics now lose more energy per round trip, creating natural spectral thinning
- Lowered radiation filter cutoff across all presets (concert flute 300→150Hz, proportional for others) to retain more fundamental energy in low notes

**Pass B: Register Transitions**
- End reflection filter replaced with high-shelf (-6dB above ~2kHz) — higher frequencies reflect less from the open bore end, modeling realistic radiation impedance and improving octave transition behavior

**Pass C: Noise & Spectral Realism**
- Replaced single lowpass noise filter with 2nd-order bandpass centered on Strouhal frequency (f_s = 0.2 × jet_velocity / jet_diameter) — turbulence spectrum now physically tracks breath pressure
- Added `jetDiameter` to InstrumentPreset struct (~7mm piccolo to ~15mm shakuhachi) for per-instrument Strouhal tuning

## [1.0.1] - 2026-04-05

### Fixed

- **Stuck voice / infinite sustain bug:** Notes triggered on note-off and sustained forever
  - **Root cause 1:** `JetNonlinearity` velocity floor (`max(0.01, vel)`) provided residual drive to bore waveguide even after breath stopped, keeping the feedback loop alive indefinitely
  - **Root cause 2:** Voice clearing required waveguide silence (`silentSampleCount >= 512`), which never occurred due to the residual drive, trapping voices in permanent "releasing" state
  - **Fix:** Gate nonlinearity output to zero when jet velocity < 0.001 (breaks feedback loop on release); add 10ms release tail fade with guaranteed voice clearing after breath envelope completes

## [1.0.0] - 2026-04-05

- Initial release: Physical modeling flute synthesiser (Verge 1995 jet-drive model)
