# O-Wind Changelog

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
