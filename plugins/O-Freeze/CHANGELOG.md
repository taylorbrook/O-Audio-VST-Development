# Changelog

All notable changes to O-Freeze will be documented in this file.

## [1.9.4] - 2026-04-04

### Changed
- **Inlined `juce::MathConstants<double>::pi`** — removed duplicate `const double PI` locals in `prepareToPlay` and the grain-size-change block
- **Moved per-sample grain scratch arrays to class members** — `windowValues`, `grainPos0`, `grainPos1`, `grainFrac` (all `MAX_GRAINS`-sized) were stack-allocated and zero-initialized every sample inside the inner loop; now persistent members with no per-sample init cost

## [1.9.3] - 2026-04-04

### Changed
- **Removed dead `readPosition` member** — declared in PluginProcessor.h and assigned in `prepareToPlay` but never read; vestige of pre-granular design where a single read head traversed the freeze buffer (each grain now tracks its own position via `Grain::position` and `Grain::fractionalPosition`)

## [1.9.2] - 2026-04-04

### Fixed
- **RMS threshold calculation ~3dB too low in stereo** — `rmsSamplesPerWindow * numChannels` divided by 2x the actual buffer size since the circular buffer holds `rmsSamplesPerWindow` interleaved entries (both channels), not `rmsSamplesPerWindow` per channel
- Root cause: divisor assumed per-channel buffer size, but write index wraps at `rmsSamplesPerWindow` regardless of channel count, so stereo writes interleave into the same buffer
- Effect: threshold gate triggered ~3dB earlier than the knob indicated in stereo; mono operation was unaffected

## [1.9.0] - 2026-04-04

### Added
- **Per-grain pitch micro-detuning** (DETUNE, 0–50 cents, default 5, step 0.1) — each grain receives a random playback rate offset that spectrally decorrelates overlapping grains for richer, chorus-like frozen textures
- **Linear interpolation on grain reads** — fractional sample positions produce smooth pitch-shifted output without aliasing artifacts
- Detune knob in WebView UI between Grains and Mix knobs

### Changed
- Grain position advancement replaced from integer increment to fractional accumulator (`fractionalPosition += playbackRate` per sample)
- Editor width increased from 500px to 550px to accommodate 6-knob row

### Technical Notes
- `playbackRate` stored per-grain: `1.0 + random(±1) * (cents / 1200)` — cents-to-rate conversion
- First grain at freeze engage always gets `playbackRate = 1.0` (no detune) for clean initial capture
- `fractionalPosition` (float) accumulates playback rate; integer part selects buffer sample, fractional part drives linear interpolation between `floor` and `floor+1`
- At max detune (50 cents), playback rate varies ±0.0417 — subtle pitch shift with effective spectral decorrelation
- WSOLA tail capture updated to compute end position from `base + int(fractionalPosition)` rather than advanced integer position
- Reverse playback direction fully supported (interpolation direction flips correctly)
- No breaking changes — existing presets default to 5 cents detune; set to 0 for previous behavior
- Float precision sufficient: max `fractionalPosition` ≈ 49000 at 1000ms/48kHz, well within float mantissa range

## [1.8.0] - 2026-04-04

### Added
- **WSOLA-style best-overlap grain positioning** — when activating a new grain, searches ±(grainSize/4) samples around the nominal position to find the offset that maximizes normalized cross-correlation with the previous grain's tail, producing smoother transitions on pitched/tonal content
- Cross-correlation computed on 64-sample mono segments (both channels summed), stepping by 4 samples for CPU efficiency
- First grain after freeze engage uses nominal position (no previous tail to compare against)
- Per-grain jitter offset applied ON TOP of WSOLA-chosen position (both features stack)

### Technical Notes
- `lastGrainTail[64]` member stores final 64 samples of each completed grain (mono, playback order)
- `hasLastGrainTail` flag reset on freeze engage, set true on first grain completion
- Search window: `grainSize/4` samples in each direction, step 4 → ~1100 xcorr evaluations per grain activation at 400ms grain size
- Normalized cross-correlation: `sumXY / sqrt(sumX2 * sumY2)` with epsilon guard (1e-6)
- `sumX2` (tail energy) precomputed once per search — constant across all offsets
- Tail capture handles both forward and reverse playback directions
- No new parameters — transparent improvement to existing granular engine
- Existing presets unaffected (WSOLA only adjusts internal grain start positions)

## [1.7.1] - 2026-04-04

### Fixed
- **Stochastic grain trigger timing** — grain trigger intervals now randomized ±30% from nominal to break up periodic comb-filter artifacts caused by perfectly regular grain spacing
- Minimum trigger interval clamped to `grainTriggerInterval / 2` to prevent grain stacking

### Technical Notes
- New `nextTriggerInterval` member computed each time a grain fires: `grainTriggerInterval + random(±30%)`
- Comparison changed from fixed `grainTriggerInterval` to jittered `nextTriggerInterval`
- `nextTriggerInterval` reset to nominal when grain parameters change or `prepareToPlay` called
- No new parameters — transparent improvement to existing granular engine
- Existing presets unaffected (timing jitter is purely internal, no parameter changes)

## [1.7.0] - 2026-04-04

### Added
- **Per-grain position jitter** — each grain receives a random buffer offset at activation, decorrelating overlapping grains for richer, less phasey frozen textures
- Jitter range scales with grain size and DRIFT parameter: `jitterOffset = random * grainSize * driftValue`
- First grain activated at freeze engage always gets `jitterOffset = 0` for clean initial capture

### Changed
- **Replaced COLA normalization with per-sample window normalization** — granular sum divided by sum of active Hann window values each sample, maintaining constant amplitude regardless of grain phase alignment
- Removed COLA scaling factor (`2/N`) from Hann window; raw Hann values stored instead
- Drift parameter now serves dual purpose: controls both shared drift range and per-grain jitter spread

### Technical Notes
- `jitterOffset` stored per-grain in Grain struct (int, computed once at activation)
- Jitter uses existing `driftValue` (0–1) so drift=0 means zero jitter (backward compatible)
- Window normalization: `frozenSample = granularSum / windowSum` with epsilon guard (1e-6) to avoid division by zero
- No new parameters — reuses DRIFT for jitter control
- Existing presets unaffected when drift=0 (jitter range is zero)

## [1.6.0] - 2026-04-04

### Added
- **Reverse playback mode** (REVERSE, toggle, default off) — reads grains backwards through the freeze buffer when enabled
- Grain start positions calculated from end of grain region, read index decrements instead of incrementing
- Hann window envelope remains forward (fade-in then fade-out) regardless of read direction — preserves COLA compliance
- Toggle button in WebView UI below the freeze button, matching existing botanical aesthetic

### Technical Notes
- REVERSE parameter read once per processBlock (atomic, real-time safe)
- Reverse affects both initial freeze grain and subsequent triggered grains
- Drift offset applied identically in both directions (read-time application preserved)
- Can toggle reverse while frozen — grains change direction immediately
- Existing presets default to off (non-breaking — playback behavior unchanged)

## [1.5.1] - 2026-04-03

### Changed
- Moved freeze button up 25px for better visual balance
- Lightened Drift LFO group box background for better visibility
- Darkened all font colors for richer, more saturated text appearance

## [1.5.0] - 2026-04-03

### Added
- **LFO modulation for Drift parameter** with 3 new controls:
  - **LFO Rate** (LFO_RATE, 0.01–10 Hz, default 0.5 Hz) — skewed range for fine control at low frequencies
  - **LFO Depth** (LFO_DEPTH, 0–100%, default 50%) — scales LFO influence on drift offset
  - **LFO Shape** (LFO_SHAPE, Sine/Triangle/Random) — waveform selector; Random uses sample-and-hold (new value each cycle)
- LFO group in WebView UI: two small knobs (Rate, Depth) + shape toggle, grouped below existing knob row with "Drift LFO" label
- Editor height increased from 450px to 530px to accommodate LFO group

### Technical Notes
- LFO computed once per processBlock (per-block, not per-sample) for efficiency
- LFO phase accumulates by `rate * blockSize / sampleRate` per block
- LFO output (-1 to +1) scaled by depth and added to frozen drift offset, clamped 0–1
- COLA phase alignment preserved — all grains still share the same modulated drift offset
- LFO only active while frozen and depth > 0 (no unnecessary computation)
- Existing presets default to 50% depth with Sine shape (non-breaking — drift behavior unchanged when depth is 0)

## [1.4.0] - 2026-04-03

### Added
- **Grain Count parameter** (GRAIN_COUNT, 2–32, default 8, integer steps) — replaces hardcoded 8-grain system with dynamic grain pool
- Rotary knob in WebView UI matching existing botanical aesthetic (labeled "Grains")

### Changed
- Granular engine dynamically recalculates hop size (`grainSize / grainCount`) and COLA scaling factor (`2.0 / grainCount`) on parameter change
- Grain array expanded to MAX_GRAINS=32; only `grainCount` grains are activated from the pool
- Grains beyond active count are cleanly deactivated on count reduction; `nextGrainIndex` clamped to new range
- Editor width increased from 450px to 500px to accommodate 5 knobs
- Grain Size knob label shortened from "Grain" to "Size" to avoid confusion with new "Grains" knob

### Technical Notes
- COLA identity preserved for all grain counts: N Hann windows at (1−1/N) overlap sum to N/2, scaled by 2/N → unity
- Existing presets default to 8 grains (non-breaking — matches previous hardcoded behavior)
- No audio-thread allocations: window buffer pre-allocated for MAX_GRAINS=32, Hann window pre-allocated for max grain size (1000ms)

## [1.3.1] - 2026-04-03

### Fixed
- `releaseResources()` now properly clears freeze buffer, resets all grain states, and resets crossfade gain (was empty stub from Stage 2)
- MODE relay changed from `WebToggleButtonRelay` to `WebComboBoxRelay` to correctly match the `AudioParameterChoice` type; JS updated from boolean `getToggleState` to index-based `getComboBoxState`
- DRIFT parameter default changed from 25% to 0% to match documentation
- STATUS.md Known Issues updated to reflect drift clicking was resolved in v1.2.2

## [1.3.0] - 2026-04-03

### Added
- **Grain Size parameter** (GRAIN_SIZE, 50ms–1000ms, default 400ms) — replaces hardcoded 400ms grain size
- Rotary knob in WebView UI matching existing botanical aesthetic

### Changed
- Granular engine dynamically recalculates grain size, hop size (grainSize/8), and Hann window on parameter change
- COLA scaling factor computed from NUM_GRAINS (2/NUM_GRAINS) for unity gain at any grain size
- Release fade time now tracks current grain size instead of hardcoded 400ms
- Hann window pre-allocated for max grain size (1000ms) — no audio-thread allocations on parameter change
- Active grains past new grain boundary are deactivated cleanly on size reduction

### Technical Notes
- COLA identity preserved: 8 Hann windows at 87.5% overlap sum to NUM_GRAINS/2 = 4.0, scaled by 0.25 → unity
- Overlap ratio (87.5%) is grain-size-independent — only depends on NUM_GRAINS
- Parameter change detection via `lastGrainSizeMs` comparison avoids per-block recalculation

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
