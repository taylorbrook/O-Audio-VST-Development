# O-Formant Changelog

All notable changes to O-Formant will be documented in this file.

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
