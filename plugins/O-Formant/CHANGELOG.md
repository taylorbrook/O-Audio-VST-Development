# O-Formant Changelog

All notable changes to O-Formant will be documented in this file.

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
