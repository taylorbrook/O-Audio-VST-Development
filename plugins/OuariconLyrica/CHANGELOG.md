# OuariconLyrica Changelog

All notable changes to OuariconLyrica are documented in this file.

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
