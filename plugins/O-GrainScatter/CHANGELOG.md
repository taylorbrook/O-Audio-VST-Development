# Changelog

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
