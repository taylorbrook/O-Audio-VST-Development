# O-Reed Changelog

## v1.0.6 (2026-04-07)

### Fixed

- **Silent output from voice DSP** -- four interacting bugs prevented the reed model from producing audible sound:
  1. **Reed force unit mismatch**: `dp * A_reed` used total force (N) but stiffness/damping/mass params are per-unit-area. Removed `A_reed` multiplier so pressure (Pa) drives the reed directly.
  2. **Reed force sign inversion**: `+dp` opened the reed under pressure (wrong). Changed to `-dp` so positive mouth pressure closes the reed, enabling negative-resistance self-oscillation.
  3. **Numerical instability**: explicit Euler integration diverged because damping rate (~270 kHz) >> sample rate (~88 kHz). Replaced with semi-implicit Euler (implicit damping) for unconditional stability.
  4. **Startup deadlock**: hard reed closure (`S_opening = max(x+H, 0)`) gave zero flow when closed, preventing bore excitation. Added 0.5% soft minimum opening for startup leakage.
- **Mouth pressure overdriving reed**: fixed 12000 Pa scaling (3x the reed's closure pressure) which permanently overblew the reed. Now scales to `reedModel.getClosurePressure()` so the reed operates in the self-oscillation regime at any parameter setting.
- **Output normalization**: removed impedance-based normalization (`1/2.67e6`) that crushed signal by ~5000x. Replaced with `1/500` matching O-Wind/O-Bowed approach.
- **Radiation filter killing fundamental**: output was taken from highpass-filtered bore radiation (3400 Hz cutoff), attenuating the fundamental by ~22 dB. Now uses bore pressure at reed (full spectrum), consistent with other physical model plugins.
- **Turbulence seeding**: added 2% breath-proportional noise to seed bore resonance during reed closure.

## v1.0.5 (2026-04-07)

### Fixed

- **Bore waveguide self-oscillation** -- removed `feedbackGain` compensation from `BoreWaveguide` that was introduced in v1.0.4. The formula `1 / max(0.5, viscGain * 0.98)` yielded ~1.026 feedback gain, giving a round-trip gain of ~1.02 (>1.0). This caused the bore to self-excite after any note, producing a continuous high-pitched tone at ~-12 dB that never decayed. The bell allpass filter has unity gain (no energy loss to compensate for), and the viscothermal filter's 0.5% loss is correct physical damping. Root cause: v1.0.4 feedbackGain overcompensation.

## v1.0.4 (2026-04-06)

### Improved

- **Bore feedback gain compensation** -- added `feedbackGain` to `BoreWaveguide` that compensates for cumulative viscothermal filter and bell reflection losses. Computed as `1 / max(0.5, viscGain * 0.98)` clamped to 2.0x, applied to the backward-traveling wave returning to the reed. Prevents energy decay from damping sustained tones, similar to O-Wind's mechanism.

## v1.0.3 (2026-04-06)

### Fixed

- **Mouthpiece chamber damping** -- increased Helmholtz resonator damping from 0.001 to 0.05, bringing Q from ~164 down to ~3. The original Q~164 produced an unrealistic 4.6 kHz spike that colored all patches using the chamber. New damping models realistic mouthpiece cavity losses.
- **Mouthpiece chamber bypassed by default** -- changed `mouthpieceVol` parameter default from 0.3 to 0.0 so the chamber is off unless explicitly enabled. All 24 factory presets updated to 0.0.

## v1.0.2 (2026-04-06)

### Fixed

- **Tone hole energy conservation** — scatter coefficient formula changed from `-holeStrength / (1 + holeStrength)` to `-2*holeStrength / (2 + holeStrength)` (normalized Kelly-Lochbaum). The old formula destroyed ~24% of wave energy at each three-port junction. Applies to all 4 tone holes and the register hole.

## v1.0.1 (2026-04-06)

### Fixed

- **Bell radiation output model** — replaced allpass-difference output tap (`th4_fwd + p_reflected`) with direct forward-wave tap through a first-order highpass radiation filter. The allpass difference cancelled ~99.6% of fundamental energy at 440 Hz. New model uses `radiationFilter.processSample(th4_fwd)` matching O-Wind's radiation approach. Bell allpass retained for reflection path only.

## v1.0.0 (2026-04-06)

Initial release of O-Reed -- physical modeling reed wind instrument synthesizer.

### Features

- **Physical Modeling Engine** -- Guillemain-style reed ODE with symplectic Euler integration, Bernoulli flow model with Psi confinement (single-to-double reed), mass-spring-damper reed dynamics
- **Conical Bore Waveguide** -- Strategy C dual delay-line bore with Thiran allpass interpolation, viscothermal loss modeling, bell radiation filter, scale-dependent smoothing
- **Breath Envelope** -- Attack/sustain/release with velocity-scaled chiff overshoot, configurable air noise mix
- **Mouthpiece Chamber** -- Helmholtz resonance simulation for pitch correction and spectral coloring
- **Tone Hole Model** -- Lattice lowpass filter simulating open tone holes, skewed frequency control (200-8000 Hz)
- **Expression System** -- Vibrato (lip/breath/throat sources), growl (vocal fold coupling), flutter tongue, subtone mode
- **Impossible Physics** -- Infinite sustain, reverse bore, dual bore (parallel waveguide drone), cross-modulation feedback path
- **Dual Bore Drone** -- Second parallel waveguide with +/-2400 cent pitch offset for arghul/launeddas/mijwiz drone effects
- **Legato Engine** -- Portamento with configurable glide time, voice stealing with energy-based cleanup
- **Oversampling** -- 2x (default) and 4x modes with JUCE dsp::Oversampling
- **Microtonal Tuning** -- Scala/TUN file support, MTS-ESP client, 12-TET, configurable reference pitch (220-880 Hz) via scala-tuning-engine module
- **MPE Support** -- Per-note pressure, slide, and glide via MPESynthesiser (16 voices)
- **35 Parameters** across 8 categories: Primary, Secondary, Advanced, Expressive, Impossible Physics, Tuning, Voice Config, Output
- **WebView UI** -- 3-tab layout (Instrument, Expression, Advanced) with collapsible sections, XY pad, tuning panel, 28 slider knobs, 6 comboboxes, 1 toggle
- **24 Factory Presets**:
  - Western (9): Bb Clarinet, Bass Clarinet, Alto Sax, Tenor Sax, Soprano Sax, Baritone Sax, Oboe, English Horn, Bassoon
  - Non-Western (9): Duduk, Shehnai, Suona, Hichiriki, Zurna, Piri, Arghul, Launeddas, Mijwiz
  - Sound Design (6): Glass Reed, Metal Wind, Impossible Bore, Breath Drone, Giant Clarinet, Micro Reed
- **Preset System** -- OuariconPresetManager with factory/user presets, save/load, navigation, FileChooser dialog

### Validation

- pluginval level 10 PASS (VST3 + AU)
- auval PASS (aumu ORed OuDv)
- Zero build errors, zero warnings (excluding JUCE/module upstream)

### Formats

- VST3
- AU (Audio Unit)
- Standalone
