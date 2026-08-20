# Changelog

## [2.4.4] - 2026-08-19

UI layout fix. The Spatial Audio section was clipped by the bottom edge of the editor window; the control grid is now content-sized and the window is 50 px shorter.

### Fixed
- **Spatial Audio section clipped off the bottom of the editor:** at the 900 x 850 editor size the page content ran to y=884.5, so the last ~34 px — the bottom row of Spatial Audio knob readouts and the "Set Mode to Scatter or Trajectory to enable" hint — were cut off by `body { overflow: hidden }` and unreachable. *Root cause:* `.controls-area` used `grid-template-rows: 1fr 1fr`, which forces the second row (Beat Sync / Euclidean Rhythm) to match the height of the first (Core Engine / Pitch & Scale). Row 2 needs only 111 px of content but was rendered at 199.5 px, and that 88.5 px of dead space pushed Spatial Audio past the window. Changed to `grid-template-rows: auto auto` so each row sizes to its own content — Beat Sync and Euclidean Rhythm are now 112 px (56 % of the top row) and the dead space is gone.

### Changed
- **Editor height 850 -> 800 px** (`PluginEditor.cpp`): with the grid fix the natural content height is 797 px, so the window was shortened by 50 px rather than leaving the reclaimed space empty. Width is unchanged at 900 px. Measured content stack at 900x800: header 42 + viz 243 + freeze 33.5 + fleuron 16.5 + controls 331.5 + spatial 133.5 = 800 exactly, nothing clipped.
- **`.plugin-container` sizes from the editor instead of hard-coding 900x850** — now `width: 100%; height: 100%`, so `setSize()` in `PluginEditor.cpp` is the single source of truth for the window size and the CSS can't drift from it.
- **`.viz-area` absorbs the layout slack** — the fixed `height: 240px` became `flex: 1 1 auto; min-height: 200px`. The visualizations render at 243 px at the current size, and because the viz is the only flex-grow item, any font-metric difference between WKWebView/WebView2 and the layout-test engine is taken out of the viz height rather than clipping the bottom of the page.

### Testing
- Layout measured headlessly (Chromium, 900x800) against the shipped `index.html`: Spatial Audio bottom edge = 800.0, spatial hint bottom = 787.0, last knob readout bottom = 775.5 — zero overflow.
- Negative control: re-injecting the old `grid-template-rows: 1fr 1fr` at the new 800 px height puts the Spatial Audio bottom back at 884.5 (84.5 px clipped), confirming the grid change is what fixes the layout.
- No DSP, parameter, preset, or state changes — CSS and one `setSize()` call only.

### Notes
- This fix was authored as "2.4.3" on a branch cut before the 2.4.3 licensing release shipped; renumbered to 2.4.4 at merge time since 2.4.3 was already published.

## [2.4.3] - 2026-08-19

Licensing release — no audible or behavioral change.

### Changed
- Added AGPL-3.0 license notice headers to all Ouaricon-authored source files (repo relicensed to AGPL-3.0 on 2026-08-01; JUCE used under AGPLv3).

## [2.4.2] - 2026-07-09

Info-finding cleanup sweep (CODE_REVIEW.md v2.4.0 review, IN-* items — the 2 critical + 12 warning findings were resolved in v2.4.1). No audible or behavioral change; dead-code removal, a per-block micro-optimization, and defensive state resets.

### Changed
- **IN-01 — dead `FreezeManager::getCrossfadeGain()` removed:** the method was never called (the freeze crossfade is implicit — grains switch source at spawn and `advanceCrossfade` delays `active=false` by ~5 ms). Removed it and the now-write-only `crossfadeDirection` member. No behavior change.
- **IN-02 — dead `TempoTracker::lastPpq` removed:** written in both update branches, never read.
- **IN-05 — grain voices now cleared in `prepareToPlay`:** added `GrainPool::clearVoices()` and call it from `prepareToPlay` (previously voices were deactivated only in `reset()`), so a sample-rate/block-size change without a host `reset()` can't leave stale grains active. `reset()` now shares the same `clearVoices()` path.
- **IN-09 — HOA write pointers cached per block:** the spatial inner loop stored via `hoaBus.setSample(ch, i, …)` (a `getWritePointer` + bounds check per channel per sample); now caches the 16 write pointers once per block and indexes directly.
- **IN-10 — distance split-semantics documented:** added a comment noting grain gain uses the spawn-frozen `v.distance` while the distance-LPF uses the live per-block value (intended: per-grain gain snapshot, continuous filter tone).
- **IN-11 — `reset()` now resets `TempoTracker`:** `reset()` re-`prepare()`s the tempo tracker so the standalone `manualPpq` counter doesn't keep advancing across a transport stop/seek (completeness gap in "clear all DSP state").
- **IN-14 — dead `.dimmed-spatial` CSS rule removed:** the spatial gate dims via inline `style.opacity`/`pointerEvents`, never applying this class.
- **IN-15 — `timerCallback` early-returns when hidden:** the 30 Hz viz JSON (String allocations) was built every tick even when the WebView wasn't showing (the emit was already visibility-gated). Now skips construction entirely when `!webView->isShowing()`.

### Reviewed — no change needed
- **IN-03** (Euclidean generator is a rotation of canonical Bjorklund) — valid maximally-even pattern, not a bug.
- **IN-04** (`isEvenSubdiv` misnamed) — already resolved by the WR-02 scheduler rewrite (now `isOffBeat`).
- **IN-06** (harden `repeatIntervalSamples` divide) — already done in v2.4.1 alongside WR-09 (`jmax(1.0, bpm)`).
- **IN-07** (repeat grains re-trigger at future subdivisions) — intended stutter/repeat-burst behavior (a shipped core feature); left as-is.
- **IN-08** (grain envelope never reaches phase 1.0) — ~5e-7 error at typical grain lengths; changing the phase formula would alter the grain sound for no audible benefit.
- **IN-12** (`releaseResources()` empty) — acceptable; JUCE re-`prepareToPlay`s before reuse.
- **IN-13** (Doppler uses smoothed SH `current[1]` as previous-azimuth proxy) — documented heuristic, not a defect.

## [2.4.1] - 2026-07-08

Code-review resolution pass (CODE_REVIEW.md, v2.4.0 deep review). All 2 critical + 12 warning findings fixed.

### Fixed
- **CR-01 — dead "Scan" knob:** `scan_position` (the v2.4.0 flagship control) had a param, DSP, DOM knob, and JS binding but **no editor relay/attachment**, so it was uncontrollable from the UI *and* host automation. Added the `WebSliderRelay` + `.withOptionsFrom` + `WebSliderParameterAttachment` triplet. *Root cause:* the relay/attachment pair was never added when the param was introduced.
- **CR-02 — RT reallocation in `reset()`:** `reset()` called `delayBuffer.prepare()` and `freezeManager.prepare()`, which `setSize()` the 2 s delay + freeze buffers (~6 MB of free/alloc) on a thread hosts may run in real time. Added alloc-free `clear()` methods (zero the already-sized buffers, no `setSize`) and call those instead.
- **WR-01 — spatial-mode feedback was a block-held constant:** in Scatter/Trajectory mode `feedbackL/R` were updated only in the post-decode loop, so every input sample of a block was fed the *same*, one-block-late feedback value → DC offset + block-rate buzz. Feedback is now derived **per-sample inside the main loop** from the HOA omni (W) channel — a true per-sample recursion. (Spatial feedback is now mono; the grains re-spatialize it on the next pass.)
- **WR-02 — swing dropped every off-beat + desynced Euclidean:** the swing gate reused the straight-boundary crossing and could only *reject*, so swung (odd) subdivisions were never spawned, and `euclideanStep` advanced only past the gate → pattern drift. Rewrote the scheduler to detect each division's **own** trigger time (straight for on-beats, swing-offset for off-beats) and advance the Euclidean step once per division, in order → phase-locked. Straight-grid behaviour (swing = 50 %) is bit-identical to before.
- **WR-03 — freeze-engage click/xrun:** `engage()` copied up to ~176 k samples element-by-element (`getSample`/`setSample` + modulo) on the audio thread. Replaced with two contiguous `AudioBuffer::copyFrom` memcpys spanning the ring wrap.
- **WR-04 — `spawnRequests` could exceed its `reserve(128)`** on large/offline blocks → audio-thread realloc. Added a hard `kMaxSpawnsPerBlock = 128` cap at every push site (free + sync + repeats).
- **WR-05 — no NaN/Inf guard on recursive state:** a single non-finite sample could latch `feedbackL/R` or `distanceLpfState` to NaN → permanent silence. Added `isfinite` flush-to-zero on both feedback paths and both LPF states.
- **WR-06 — per-sample SH trig in Trajectory mode:** `encodeSH16` (16-coeff trig) ran for every active voice every sample. It now updates the SH *target* only at a 16-sample control rate; the existing one-pole SH smoother interpolates between updates (trajectory position + Doppler stay per-sample, unchanged).
- **WR-07 — no block-size clamp:** `hoaBus`/`binaural` buffers are sized to `samplesPerBlock`; added a `jassert` + defensive `jmin` clamp on `numSamples` so an over-sized host block can't write past the allocations (the class v2.0.2 fixed).
- **WR-08 — distance-LPF zipper:** the cutoff coefficient was recomputed per block with no smoothing → stepping on Distance / Distance-LPF automation. Wrapped it in a per-sample `SmoothedValue`.
- **WR-09 — `bpm <= 0` silently stopped Sync scheduling:** `TempoTracker` took a host-reported non-positive BPM verbatim (ppqPerSample = 0 → no subdivision crossings). It now falls through to the 120 BPM fallback; the repeat-interval divide is additionally hardened with `jmax(1.0, bpm)` (IN-06).
- **WR-10 — `spatial_smooth` reset default wrong:** the hardcoded JS normalized default `0.1` ignored the 0.4 skew (reset snapped to ≈1.6 ms instead of 5 ms). Now sourced from C++ (see WR-11).
- **WR-11 — JS re-implemented C++ ranges/skew:** knob readouts and reset defaults duplicated each `NormalisableRange` (incl. skew) in JS — a latent drift class (WR-10 was the first crack). Readouts now use `state.getScaledValue()` and double-click resets pull skew-correct defaults from a new `getParameterDefaults` native function. No hand-coded ranges/defaults remain.
- **WR-12 — `CMakeLists` version drift:** `VERSION` was pinned at `2.1.0` (three minors behind the shipped 2.4.0 features) → binaries reported the wrong version. Bumped to `2.4.1`.

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
