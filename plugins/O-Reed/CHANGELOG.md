# O-Reed Changelog

## v1.0.11 (2026-04-16)

### Fixed

- **Notes play flat — pitch progressively wrong with higher MIDI notes** -- bore waveguide delay compensation only subtracted 1 sample (for the internal `prevBellReflection` storage) but the voice loop has a SECOND single-sample latch via `prevBoreMinus` in `ReedWindVoice` (lines 516, 542). The reed reads `p_bore_minus` from the previous iteration, so the actual round-trip delay = `4*halfDelay + 2 + bellPD + viscPD` while the code compensated for `4*halfDelay + 1 + bellPD + viscPD`. This made every note flat by a `D/(D+1)` ratio: ~17 cents at A4, ~33 cents at A5, ~67 cents at A6, >semitone at C8. Fix: change `compensatedDelay = totalDelay - viscPD - bellPD - 1.0f` to `- 2.0f` in `BoreWaveguide::setFrequency()` to compensate both storage samples (one inside the bore, one in the external feedback latch).

## v1.0.10 (2026-04-11)

### Fixed

- **No sound / silent output on default patch** -- `toneHoleCutoff` parameter defaulted to 1500 Hz instead of 8000 Hz, which opened 3 of 4 tone holes fully (scatter=-0.165 each) and 1 partially (scatter=-0.058). Each open tone hole junction radiates ~16.5% of the sum of forward and backward bore wave energy. With 4 active scattering junctions, the bore lost ~70% of its energy per round trip -- far exceeding the reed model's ability to sustain self-oscillation (reed gain ~1-3% per round trip). Fix: changed default `toneHoleCutoff` from 1500 Hz to 8000 Hz (all holes closed). Root cause: the toneHoleCutoff parameter controls progressive tone hole opening from bell-end first; at 1500 Hz (`cutoff_norm=0.167`), holes 1-3 were fully open (`opening=1.0`) and hole 4 was 33% open.

## v1.0.9 (2026-04-09)

### Fixed

- **Some notes silent, others produce glitchy high-pitched clipping tone** -- bore waveguide delay compensation computed filter group delays at DC instead of at the playing frequency. The DC formula (`sr / 2π*fc`) overestimates the viscothermal filter's group delay by up to 20x at high frequencies (e.g., 22.9 samples at DC vs 1.4 samples at 2kHz for a narrow bore). Combined with the bell allpass being hardcoded at 0.5 samples (actual: 5-30 samples depending on frequency), the `compensatedDelay` could go negative for many note/parameter combinations. When negative, all 5 bore segments clamped to the 2-sample minimum, producing a fixed ~4.4kHz parasitic tone regardless of MIDI note — or preventing self-oscillation entirely (silence). Three fixes:
  1. **Frequency-dependent viscothermal group delay**: replaced DC approximation with exact one-pole formula `GD(f) = p*(cos(ω)-p) / (1-2p*cos(ω)+p²)` evaluated at the target frequency. Stored filter pole coefficient from `updateParams()` for use in `setFrequency()`.
  2. **Frequency-dependent bell allpass group delay**: replaced static `bellGD = 0.5` with exact first-order allpass formula `GD(f) = (1-a²) / (1+a²-2a*cos(ω))`. Stored allpass coefficient similarly.
  3. **Safety clamp on compensatedDelay**: added `std::max(4.0f, compensatedDelay)` before segment division, matching O-Wind/O-Bowed pattern. Prevents negative delays from reaching Thiran interpolation. Reduced per-segment minimum from 2.0 to 1.0 for better high-frequency resolution.

## v1.0.8 (2026-04-08)

### Fixed

- **Voices not clearing after note-off (4-28s delay)** -- bore waveguide energy decayed too slowly for timely voice cleanup. With `feedbackGain=1.0` and viscothermal `g=0.995` (0.5% loss/round-trip), the bore ring-down took ~4s at 440Hz and ~28s at C2 (65Hz). Voices stayed allocated the entire time, eating polyphony and wasting CPU. Two fixes:
  1. **Post-release bore damping**: once breath envelope reaches Off state, apply exponential damping (~50ms time constant) to the bore feedback path. This drains bore energy in ~500ms regardless of pitch, while preserving the natural 150ms breath release tail.
  2. **Energy estimate tracks bore wave directly**: changed `energyEstimate` from tracking `|lastRadiatedOutput|` (highpass-filtered at ~3400Hz) to `|seg_bwd[0]|` (full-spectrum backward wave at reed). The radiation highpass severely underreported bore energy for low notes where most energy is below the cutoff. Energy estimate now reflects actual bore content.

## v1.0.7 (2026-04-08)

### Fixed

- **Continuous tone at -12 dB after note-off** -- reed model self-excited at zero mouth pressure due to Bernoulli flow through the full rest opening (H_eff ≈ 0.54mm). With Z_c ≈ 1.08e6 and S_opening ≈ 5.4e-6 m², the reed junction reflection gain exceeded 1.0 for bore pressures below ~14 Pa, creating a negative resistance that sustained oscillation indefinitely. Root cause: `dp = -p_bore_minus` drives flow through the open reed channel even without breath, and `Z_c * u_reed` exceeds the incoming wave amplitude at low levels. Fix: gate the reed opening by mouth pressure (threshold = 1% of closure pressure) so the reed rests closed against the mouthpiece when breath is off, matching physical behavior. The bore now rings down naturally through viscothermal loss (~0.5%/round-trip).

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
