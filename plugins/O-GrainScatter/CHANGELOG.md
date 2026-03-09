# Changelog

## [2.4.0] - 2026-03-09

### Added
- **Grain scan position** (`scan_position` 0-100%): sets the base grain read position in the delay buffer, mapping 0% (write head / most recent audio) to 100% (2 seconds back). Replaces the previous fixed `basePosition = grainSizeSamples` with user-controllable buffer scanning
- Spread parameter now scatters grains around the scan position instead of around `grainSizeSamples`
- In freeze mode, the full 2-second delay buffer is captured so scan position can sweep through the entire frozen buffer
- "Scan" knob in Core Engine UI group (between Density and Spread)

## [2.3.0] - 2026-03-08

### Added
- **Euclidean rotation** (`euclidean_rotation` 0-15): rotates the Euclidean pattern by reading `pattern[(step + rotation) % steps]`, shifting which pulses land on which subdivisions without regenerating the pattern
- **Swing** (`euclidean_swing` 50-75%): offsets even-numbered (off-beat) subdivision boundaries forward in time — 50% = straight, 75% = maximum shuffle
- "Rotation" and "Swing" knobs in Euclidean Rhythm UI group
- Euclidean circle visualization now reflects rotation offset: dots show the rotated pattern readout, dashed line indicates rotation origin, center label shows `r{N}` when rotation > 0

## [2.2.0] - 2026-03-08

### Added
- **Grain size randomization** (`size_random` 0-100%): each grain's duration is varied by `grainSize * (1.0 + random * sizeRandom)`, creating more organic, less mechanical grain textures
- **Per-grain amplitude randomization** (`amp_random` 0-100%): each grain's amplitude is scaled by `1.0 - random * ampRandom`, adding natural dynamic variation to the grain cloud
- Two new knobs ("Size Rnd", "Amp Rnd") in the Core Engine UI group
- Both parameters default to 0% (no change to existing behavior)

## [2.1.0] - 2026-03-08

### Added
- Grain envelope shape selection: new `grain_shape` parameter with 6 window types
  - **Hann** (default): smooth cosine bell — classic granular sound, zero at edges
  - **Triangle**: linear attack/decay — brighter, more percussive than Hann
  - **Trapezoid**: flat sustain (20-80%) with linear ramps — preserves transients
  - **Tukey** (α=0.5): cosine taper first/last 25%, flat middle — hybrid of Hann and rectangular
  - **Blackman**: narrower main lobe than Hann — reduced spectral leakage, darker tone
  - **Exp Decay**: exponential falloff — plucked/percussive character with sharp attack
- UI dropdown in Core Engine group for shape selection
- Visualization reflects selected envelope shape in real-time

## [2.0.5] - 2026-03-08

### Changed
- Removed dead code: `lastSubdivIndex` (GrainScheduler), `ppqJumped`/`didPpqJump` (TempoTracker), `scratchL`/`scratchR` (BinauralDecoder), `getActiveCount` (GrainPool), duplicate `probabilityFormatter` (app.js)
- Extracted shared `lagrangeInterpolate()` function (LagrangeInterpolation.h) used by DelayBuffer and FreezeManager — eliminates duplicated 3rd-order Lagrange interpolation code
- Consolidated duplicate degree formatters in app.js into `degreeFormatter(range, offset)` factory
- Extracted shared `resizeCanvas()` function for GrainScatterViz and EuclideanCircleViz — eliminates duplicated DPR-aware canvas sizing code
- Moved `setSpatialSmoothTime()` call from inside per-sample loop to once-per-block before the loop
- Named magic feedback constants: `kFeedbackDrive` (3.0), `kTanhCompensation` (1.00497), `kStabilityMargin` (0.95)
- Named distance attenuation constant: `kDistanceScale` (3.0) in GrainPool spatial processing
- Extracted `numHoaChannels` and `numChannels` local variables for HOA bus size expressions in BinauralDecoder and PluginProcessor

## [2.0.4] - 2026-03-08

### Fixed
- Thread safety: replaced visualization double-buffer with lock-free triple buffer to prevent torn reads when audio thread publishes faster than GUI consumes
- Thread safety: made `cachedEuclideanSteps`/`cachedEuclideanPulses` `std::atomic<int>` and moved euclidean pattern + step data into `GrainVizSnapshot` — GUI no longer holds a direct reference to audio-thread-owned `euclideanPattern` array
- Added `reset()` override to clear all DSP state (grain voices, delay buffer, feedback, freeze, scheduler, distance LPF, HOA bus) on transport stop/seek/loop — prevents stale audio artifacts after DAW transport jumps
- Root cause: double-buffer allowed audio thread to overwrite the slot GUI was reading mid-frame; euclidean data was exposed via raw `const&` across threads with no synchronization

## [2.0.3] - 2026-03-08

### Fixed
- Zipper noise on feedback/dry-wet automation in spatial mode: post-processing loop was reading raw `feedbackParam->load()` and `dryWetParam->load()` per-sample instead of using `feedbackSmoothed`/`dryWetSmoothed` SmoothedValue instances
- Root cause: stereo path correctly used SmoothedValues, but spatial post-processing loop bypassed them entirely
- Removed redundant SmoothedValue advancement in per-sample spatial branch; values now consumed in the post-processing loop where they're actually needed

## [2.0.2] - 2026-03-08

### Fixed
- Critical stack buffer overflow: replaced stack-allocated `binauralL/R[2048]` arrays with heap-allocated member buffers sized to actual `samplesPerBlock`
- Incorrect `hoaBus` and `binauralDecoder` sizing: was using `sampleRate * 0.02 + 1024` (arbitrary formula), now uses `samplesPerBlock` from host
- Root cause: `prepareToPlay` ignored its `samplesPerBlock` parameter entirely

## [2.0.1] - 2026-02-09

### Fixed
- Density parameter now uses exponential curve for perceptible control across full knob range
- Previously: 75% of knob range only varied from ~1 to ~4 grains/sec (linear interval mapping)
- Now: 50% knob = ~10 grains/sec, smooth exponential scaling from ~1/sec to ~100/sec

## [1.0.1] - 2026-02-07

### Improved
- Scale, Root Note, and Pitch Mode dropdowns now dim when Pitch Random is at 0%, with hint text "Increase Pitch Rnd to activate" — clarifies that pitch randomization must be active for scale controls to have effect

## [1.0.0] - 2026-02-07

### Added
- Granular scatter engine with 64-voice polyphonic grain pool
- Delay buffer with Lagrange 3rd-order interpolation for smooth pitched reads
- Free mode: density-controlled grain spawning (10ms to 1000ms intervals)
- Beat sync mode: 6 subdivision options (1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T)
- Euclidean rhythm patterns for gating grain triggers (2-16 steps, 1-16 pulses)
- Repeat burst system (1-16 repeats per trigger) with stutter gate
- Freeze: capture and loop audio with 5ms crossfade on engage and release
- 5 musical scales: Chromatic, Major, Minor, Pentatonic, Whole Tone
- 4 pitch modes: Random, Ladder Up, Ladder Down, Pendulum
- Scale quantizer with root note selection (C through B)
- Spread control for grain position scatter
- Pan randomization with equal-power panning law
- Reverse grain probability
- Feedback with soft-clipping (tanh) to prevent runaway
- Smoothed dry/wet mix crossfade
- Output soft-clipping to prevent digital clipping with many overlapping grains
- Standalone tempo tracker with 120 BPM fallback and DAW loop detection
- WebView UI with vintage Naturalist aesthetic (Garamond serif, parchment palette)
- Real-time grain scatter visualization (Canvas 2D, position vs pitch)
- Euclidean circle visualizer with polygon overlay and step indicator
- Freeze glow animation on toggle button
- Double-click knob reset to default values
- 18 automatable parameters across 4 groups (Core, Sync, Spread, Euclidean)
- Cross-platform WebView2 support with static linking for Windows
- State save/restore via XML serialization
