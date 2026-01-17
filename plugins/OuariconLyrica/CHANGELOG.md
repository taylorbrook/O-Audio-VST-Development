# OuariconLyrica Changelog

All notable changes to OuariconLyrica are documented in this file.

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
