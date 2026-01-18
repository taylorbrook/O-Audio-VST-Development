# OuariconLyrica Changelog

All notable changes to OuariconLyrica are documented in this file.

## [1.3.1] - 2026-01-17

### Changed (Code Simplification)

- **Extracted enum conversion helpers** - Replaced 3 duplicate switch statements with inline helper functions
  - `woodTypeFromIndex()` in BodyResonance.h
  - `techniqueFromIndex()` in PluckExciter.h
  - `glissandoModeFromIndex()` in GlissandoController.h
  - Reduces HarpSynthVoice.cpp by ~40 lines while improving maintainability

- **Centralized filter cutoff calculations** in WaveguideString.cpp
  - New `FilterCutoffs` struct and `calculateFilterCutoffs()` method
  - Eliminates duplicate calculation in `updateFilters()` and `calculateFilterGroupDelay()`
  - Ensures filter cutoffs are always computed consistently

- **Added named constants** in BodyResonance.cpp
  - `MAX_DRY_REDUCTION` (0.6f) - Maximum dry signal reduction at full body resonance
  - `WET_GAIN_MULTIPLIER` (0.7f) - Wet signal gain (v1.1.5 value)
  - Replaces magic numbers with self-documenting constants

### Removed

- **HarpSynthSound.cpp** - Empty file (implementation was header-only)
- **BridgeFilter.h/.cpp** - Unused (WaveguideString uses juce::dsp::IIR::Filter)
- **DelayLine.h/.cpp** - Unused (WaveguideString uses juce::dsp::DelayLine)
- **StringVoice.h/.cpp** - Legacy Phase 2.1 implementation replaced by WaveguideString in Phase 2.2

### Technical Notes

- Pure refactoring release - no functional changes
- Removed 7 unused files, reducing codebase size
- CMakeLists.txt updated to reflect file removals
- Build validates clean with Release configuration

## [1.3.0] - 2026-01-17

### Added

- **Attack Noise Amount** - Independent control of pluck transient noise (0-100%)
  - Overrides material default noise content for user control
  - 0% = clean attack, 100% = scratchy/noisy attack
  - Files modified: WaveguideString.h/.cpp (setAttackNoise passthrough), HarpSynthVoice.cpp, PluginProcessor.cpp

- **Sympathetic Sharpness (Q)** - Controls resonator filter Q for sympathetic resonance
  - Range: 0.1 (broad/diffuse) to 20.0 (sharp/ringing)
  - Default: 5.0 (moderate sharpness)
  - Higher Q = more defined resonant peaks, more "shimmer"
  - Lower Q = broader resonance, more diffuse coupling
  - Files modified: SympatheticResonance.h/.cpp (setResonatorQ), PluginProcessor.cpp

- **Body Mode Spread** - Controls detuning/spread of body resonance modal frequencies
  - Range: -100% to +100% (centered at 0%)
  - 0% = original uniform scaling (modes at harmonic ratios)
  - Positive = modes spread apart (wider harmonic series)
  - Negative = modes compress together (tighter harmonic series)
  - Mode 2 (600Hz base) is the pivot point; modes 0,1 shift down and 3,4 shift up
  - Files modified: BodyResonance.h/.cpp (setModeSpread, scaleFrequency), HarpSynthVoice.cpp, PluginProcessor.cpp

- **Bridge Brightness** - Direct control of bridge filter cutoff for waveguide reflection
  - Range: 0-100% (0% = very dark/damped, 50% = neutral, 100% = very bright)
  - Provides more direct waveguide control than the general brightness parameter
  - Affects the first-order lowpass filter at the bridge reflection point
  - Pitch-compensated via calculateFilterGroupDelay()
  - Files modified: WaveguideString.h/.cpp (setBridgeBrightness), HarpSynthVoice.cpp, PluginProcessor.cpp

### Technical Notes

- All 4 new parameters support real-time modulation via updateParametersFromAPVTS()
- Attack Noise uses existing PluckExciter.setNoiseAmount() with passthrough from WaveguideString
- Sympathetic Q updates all existing resonator filters when changed (setResonatorQ)
- Body Mode Spread formula: `spreadMultiplier = 1.0 + (spread * modeOffset * 0.15)` where modeOffset is -2 to +2
- Bridge Brightness formula: `modifier = 0.3 + bridgeBrightness * 1.7` (0.3x to 2.0x range)
- UI additions: 4 new sliders in appropriate sections (Pluck, Sympathetic, Body, Advanced)

## [1.2.0] - 2026-01-17

### Added

- **Advanced string parameters now affect sound (tension, gauge, length)**
  - Root cause: Parameters were defined in PluginProcessor but never connected to DSP. The UI sliders existed but did nothing - only `stringStiffness` was wired up.
  - Fix: Added setter methods to WaveguideString (`setTension`, `setGauge`, `setLength`) and connected them in HarpSynthVoice.

### Changed

- **String Tension** (0-100%)
  - Controls string brightness and resonance via bridge/nut filter cutoff frequencies
  - Low tension (0%): Dark, muted, loose sound - filter cutoffs reduced by 50%
  - High tension (100%): Bright, resonant, tight sound - filter cutoffs increased by 2x
  - Pitch remains stable (filter group delay compensation updated)

- **String Gauge** (0-100%)
  - Controls damping characteristics simulating string mass/thickness
  - Low gauge (0%): Thin string, bright, quick attack and decay
  - High gauge (100%): Thick string, dark, heavier tone with more damping
  - Affects loop damping filter (200Hz - 14kHz range, expanded from 500Hz - 10.5kHz)

- **String Length** (0-100%)
  - Controls decay envelope character without changing pitch
  - Short (0%): Punchy, quick decay (70% of base decay time)
  - Long (100%): Sustained, diffuse decay (160% of base decay time)
  - Affects feedback coefficient calculation

### Technical Notes

- Files modified: WaveguideString.h, WaveguideString.cpp, HarpSynthVoice.cpp
- Tension modifier formula: `0.5 + tension * 1.5` (0.5x to 2.0x brightness)
- Gauge modifier formula: `0.5 + gauge * 1.5` (0.5x to 2.0x damping)
- Length modifier formula: `0.7 + length * 0.9` (0.7x to 1.6x decay time)
- `calculateFilterGroupDelay()` updated to include tension/gauge for pitch stability
- Real-time modulation supported via `updateParametersFromAPVTS()`

## [1.1.5] - 2026-01-17

### Changed

- **Increased body resonance intensity for more pronounced effect**
  - Wet mix increased from 0.3 to 0.7 (more than doubled)
  - Gain values significantly increased for each wood type:
    - Spruce: 1.8x → 3.5x (~11 dB boost)
    - Maple: 1.5x → 2.8x (~9 dB boost)
    - Exotic: 2.2x → 4.5x (~13 dB boost)
    - Synthetic: 2.5x → 5.5x (~15 dB boost)
  - Dry signal now preserved at 40% even at max resonance for blend
  - Files modified: BodyResonance.cpp

## [1.1.4] - 2026-01-17

### Fixed

- **Body parameters (size, resonance, wood type) now have audible effect**
  - Root cause: `BodyResonance::updateFilterCoefficients()` created peak filters with unity gain (1.0), which meant the resonant frequencies were not actually boosted. The body size affected filter frequencies and wood type affected Q values, but without gain boosting these differences were inaudible.
  - Fix: Added `getGainForWoodType()` method that returns appropriate linear gain values based on wood type. Peak filters now boost resonant frequencies by 3.5-8 dB depending on wood type.
  - Wood type gain values:
    - Spruce: 1.8x (~5.1 dB) - Traditional, balanced resonance
    - Maple: 1.5x (~3.5 dB) - Warmer, more subtle body
    - Exotic: 2.2x (~6.8 dB) - Pronounced, rich resonance
    - Synthetic: 2.5x (~8.0 dB) - Sharp, defined peaks
  - Result: Body size now audibly shifts resonant frequencies (small=bright, large=deep), wood type creates distinct tonal characters, and body resonance controls wet/dry blend.
  - Files modified: BodyResonance.h, BodyResonance.cpp

### Technical Notes

- JUCE's `makePeakFilter()` gain parameter is linear (not dB): 1.0 = unity, 2.0 = +6dB
- Body resonance uses 5-mode modal synthesis at 300, 400, 600, 900, 1200 Hz (scaled by body size)
- Q factor still varies by wood type (2.5-5.0) controlling resonance sharpness
- Mode amplitudes also vary by wood type for additional timbral shaping

## [1.1.3] - 2026-01-17

### Fixed

- **Pitch now stable across all string materials**
  - Root cause: `calculateFilterGroupDelay()` used a hardcoded `stiffnessDelay = 0.5f` constant, but the stiffness filter is a 4-stage allpass cascade whose group delay varies dramatically with material stiffness. Materials range from 0.05 (Gut/Wire) to 0.70 (Crystal), resulting in ~14x different phase delays. This caused pitch to drift differently per material even after the v1.1.2 fix.
  - Fix: Replaced fixed constant with dynamic calculation that replicates StiffnessFilter's coefficient computation (frequency scaling + per-stage progressive scaling) and sums the group delay from all 4 allpass stages using the formula `(1 - a) / (1 + a)` samples per stage.
  - Result: All 8 material types now produce identical fundamental frequency. Stiffness affects only inharmonicity (harmonic stretch), not fundamental pitch.
  - Files modified: WaveguideString.cpp

### Technical Notes

- Allpass group delay at DC: `τ = (1 - a) / (1 + a)` samples where `a` is the coefficient
- StiffnessFilter uses 4 stages with coefficients: `stiffness * freqScaling * stageScaling * 0.8`
- Crystal (0.70) now correctly compensates ~3.2 samples vs Gut (0.05) ~0.2 samples
- This completes the filter group delay compensation system: brightness (v1.1.1), material cutoffs (v1.1.2), and stiffness allpass (v1.1.3)

## [1.1.2] - 2026-01-17

### Fixed

- **String materials no longer affect fundamental pitch**
  - Root cause: `setMaterial()` was missing delay line compensation that was added in v1.1.1 for brightness. Different materials have vastly different `brightnessCutoff` values (Gut=2000Hz, Crystal=16000Hz) and `dampingCoeff` values, which feed into `calculateFilterGroupDelay()`. Changing materials altered the filter group delay by up to ~3 samples without compensating the delay line length, causing pitch drift.
  - Fix: Added delay line recalculation to `setMaterial()` using the same pattern as `setBrightness()`. Now when material changes, the delay line length is recomputed to maintain correct pitch.
  - Result: All 8 material types now produce the same fundamental frequency while retaining their distinct timbral characteristics (damping, brightness, stiffness, noise content).
  - Files modified: WaveguideString.cpp

### Technical Notes

- This completes the filter group delay compensation system started in v1.1.1
- Both `setBrightness()` and `setMaterial()` now recalculate delay lines when their parameters change
- Materials affect timbre via: brightnessCutoff (filter color), dampingCoeff (decay rate), stiffnessAmount (inharmonicity), noiseContent (attack character)

## [1.1.1] - 2026-01-17

### Fixed

- **Brightness slider no longer affects pitch**
  - Root cause: `calculateRailDelay()` used a fixed group delay compensation constant (6.0f samples), but the actual filter group delay varies dynamically with brightness settings. Lower brightness = lower filter cutoffs = higher group delay = lower pitch (up to 1 semitone flat at brightness=0).
  - Fix: Added `calculateFilterGroupDelay()` method that computes the actual group delay from all filters (bridgeFilter, nutFilter, loopDamping) based on their current cutoff frequencies. The delay line length is now recalculated whenever brightness changes.
  - Files modified: WaveguideString.h, WaveguideString.cpp

### Technical Notes

- First-order lowpass group delay at DC: `delay_samples = sampleRate / (2 * pi * cutoffHz)`
- Bridge/nut/damping filters now contribute dynamic compensation instead of fixed 6.0f
- `setBrightness()` now updates delay line lengths in addition to filter coefficients

## [1.1.0] - 2026-01-17

### Added

- **New "Decay Time" parameter for true sustain duration control**
  - Range: 0.1s to 20s with skewed control for finer adjustment at lower values
  - Implementation: Feedback coefficient multiplier applied per waveguide cycle
  - Formula: `coefficient = 10^(-3 / (decayTime * frequency))` for -60dB decay
  - This provides uniform energy loss independent of frequency content

### Changed

- **Renamed "Sustain" parameter to "Timbre"**
  - Root cause: The original "Sustain" parameter actually controlled tonal damping (lowpass filter cutoff in the feedback loop), not decay duration. Users perceived it as affecting attack brightness rather than sustain length.
  - The parameter now more accurately reflects its function: controlling the brightness/warmth of the string tone
  - Timbre=0.0 produces darker, warmer tones; Timbre=1.0 produces brighter tones
  - Internal behavior unchanged (controls `loopDamping` filter cutoff 500Hz-10.5kHz)

### Technical Notes

- Files modified: PluginProcessor.cpp, WaveguideString.h/.cpp, HarpSynthVoice.cpp, index.html, app.js
- Feedback coefficient recalculated on note trigger and frequency change (pitch bend)
- Breaking change: "sustain" parameter ID renamed to "timbre" - existing presets/automation will need adjustment

## [1.0.4] - 2026-01-17

### Fixed

- **Master Volume fader now controls output level**
  - Root cause: `masterVolume` parameter was connected to UI but never applied in `processBlock()`
  - Fix: Added gain stage after synthesizer rendering that converts dB parameter to linear gain

- **Sustain slider now affects decay time**
  - Root cause: `setMaterial()` unconditionally overwrote `dampingAmount` with the material's `dampingCoeff`, discarding the user's sustain slider value (same bug pattern as v1.0.3 stiffness fix)
  - Fix: Added `materialDamping` and `userDampingModifier` member variables with `calculateFinalDamping()` function that combines them using a 0.5x-1.5x modifier range
  - Result: Sustain=1.0 gives 0.5x material damping (longer decay), sustain=0.0 gives 1.5x material damping (shorter decay), while preserving material-specific characteristics

## [1.0.3] - 2026-01-16

### Fixed

- **String materials now produce audibly different timbres**
  - Root cause: `WaveguideString::setStiffness()` was completely overwriting the material's stiffness value with the user's slider value, making all materials sound identical in terms of inharmonicity
  - Each material defines a unique stiffness (Gut=0.10, Crystal=0.50) that creates its characteristic harmonic structure, but this was being discarded
  - Fix: User's stiffness slider now acts as a modifier (0.5x to 1.5x) rather than an overwrite, preserving material-specific inharmonicity while still allowing user adjustment
  - Result: Gut strings now sound warm/mellow, Crystal strings sound bright/bell-like, with clear audible distinction between all 8 material types

## [1.0.2] - 2026-01-16

### Fixed

- **Tuning: Pitches were ~1 semitone flat**
  - Root cause: `WaveguideString::calculateRailDelay()` did not compensate for the group delay introduced by feedback filters (bridgeFilter, nutFilter, loopDamping, stiffnessFilter)
  - The combined filter group delay (~6 samples) effectively lengthened the delay line, lowering pitch by approximately one semitone
  - Fix: Added 6-sample group delay compensation to the delay calculation

## [1.0.1] - 2026-01-16

### Fixed

- Enable real-time parameter modulation during note playback

## [1.0.0] - 2026-01-16

### Added

- Initial release
- Physical modeling harp synthesizer with bidirectional waveguide string model
- String materials: Nylon, Gut, Wire, Carbon
- Playing techniques: Normal, Harmonic, Muted, Pres de la Table
- Body resonance with wood type selection (Spruce, Maple, Exotic, Synthetic)
- Sympathetic resonance engine
- Tuning engine with master tune and pitch bend support
- Glissando controller with free and scale-locked modes
- WebView-based GUI
