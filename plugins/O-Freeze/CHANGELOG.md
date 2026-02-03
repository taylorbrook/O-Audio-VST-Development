# Changelog

All notable changes to O-Freeze will be documented in this file.

## [1.2.0] - 2026-02-03

### Fixed
- Removed incorrect COLA normalization (dividing by active grain count caused amplitude pumping)

### Changed
- Increased grain size from 200ms to 350ms for smoother, more lush frozen textures
- Increased grain count from 8 to 12 for denser overlap (91.7% vs 87.5%)
- Extended release fade from 250ms to 400ms to accommodate longer grains

### Technical Notes
- COLA (Constant Overlap-Add) now works correctly: overlapping Hann windows sum to ~1.0 without division
- 12 grains with 91.7% overlap provides more continuous, artifact-free frozen sound
- Longer grains = slower envelope = gentler transitions

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
