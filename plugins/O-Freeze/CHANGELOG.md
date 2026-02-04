# Changelog

All notable changes to O-Freeze will be documented in this file.

## [1.2.2] - 2026-02-03

### Fixed
- **Drift clicking eliminated** - Two-part fix for granular synthesis artifacts
  1. Removed per-sample normalization that caused amplitude modulation (dividing by fluctuating `windowSum`)
  2. Lock drift offset when freeze engages - all grains now share identical position offset for proper COLA phase alignment
  - Root cause: Drift smoothing continued while frozen, causing each new grain to start at a slightly different buffer position. This broke COLA's phase alignment requirement, creating progressively worse clicking as drift wandered.

### Changed
- Replaced custom trapezoidal window with standard Hann window (pre-scaled by 0.25 for unit COLA sum)
- Drift applied at READ time (not activation time) so all grains shift together

### Technical Notes
- COLA identity: 8 Hann windows offset by N/8 each sum to exactly 4.0 at every sample point
- Window scaled by 0.25 so overlapping grains sum to 1.0 without per-sample division
- All grains must read from phase-aligned positions - shared drift offset ensures this

## [1.2.1] - 2026-02-03

### Fixed
- **Drift clicking eliminated** - Replaced per-grain random offsets with smoothed drift
  - Root cause: Each grain received an independent random position offset, causing phase discontinuities between overlapping grains that Hann windowing couldn't smooth
  - Solution: All grains now share a slowly-interpolating drift offset that wanders organically over ~500ms, maintaining phase coherence while preserving texture variation

### Changed
- Reduced grain count from 12 to 8 (87.5% overlap, proper COLA compliance)

### Technical Notes
- `currentDriftOffset` smoothly interpolates toward `targetDriftOffset` (new random target every 500ms)
- Smooth coefficient ~0.0005 gives ~50ms convergence for click-free transitions
- Drift parameter still controls range of movement, but movement is now continuous rather than discontinuous

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
