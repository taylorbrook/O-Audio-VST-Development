# Changelog

All notable changes to O-Freeze will be documented in this file.

## [1.1.0] - 2026-02-02

### Changed
- Replaced asymmetric Blackman-Harris window with symmetric Hann window for warmer, smoother frozen textures
- Implemented staggered grain activation on freeze engage (eliminates burst sound)
- Implemented soft grain deactivation on freeze release (eliminates release clicks)
- Extended release fade from 100ms to 250ms to cover grain completion time

### Technical Notes
- Domain: DSP
- Milestone: eliminate-clicks-smooth-windowing
- Window now has true zero at endpoints for artifact-free grain boundaries
- COLA compliance verified at 87.5% overlap (8 grains)
- Debug assertion added to verify COLA sum ≈ 1.0

## [1.0.1] - 2026-02-02

### Fixed
- Smooth knob animation in WebView UI

## [1.0.0] - 2026-02-01

### Added
- Initial release
- Granular freeze effect with 8 overlapping grains
- Manual and Threshold freeze modes
- Drift parameter for texture variation
- Mix control for dry/wet blend
- WebView-based UI with naturalist aesthetic
