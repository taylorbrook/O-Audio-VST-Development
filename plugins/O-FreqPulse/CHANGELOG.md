# Changelog

All notable changes to O-FreqPulse will be documented in this file.

## [1.14.0] - 2026-03-05

### Added

- **Per-band phase offset** - New `band{N}_phase_offset` parameter (integer 0-31, default 0) shifts each band's pattern read position independently. In `getTargetGainForBand`, the offset is applied as `adjustedStep = (currentStep + phaseOffset) % numSteps` before reading the step pattern, creating phase-shifted polyrhythmic patterns between frequency bands.
- **Phase slider in Band Controls panel** - The Euclidean/Band Controls panel now includes a "Phase" slider (0-31) that works in both Manual and Euclidean modes.
- **Always-visible expand button** - The ▶ expand button is now always visible on each band row (not just in Euclidean mode), since Phase and Depth are useful in both Manual and Euclidean modes.

## [1.13.0] - 2026-03-05

### Added

- **Band Mute buttons (M)** - Each band row now has a small "M" button next to the band name that toggles `band{N}_enable`. When muted, the entire band row dims to 35% opacity while the label area remains readable for interaction. The mute state syncs with DAW automation in real-time.
- **Band Solo buttons (S)** - Each band row has an "S" button for exclusive solo. Clicking solo on a band mutes all other bands by disabling their enable parameters. Clicking again restores the pre-solo enable states. Only one band can be soloed at a time.
- **Automation-aware M/S visuals** - Mute and solo button states reflect automation changes via `valueChangedEvent` listeners. If the DAW changes a band's enable parameter, the M button updates accordingly and stale solo state is cleared.

## [1.12.0] - 2026-03-05

### Changed

- **Click-and-drag velocity control** - Left-click and hold on a step, then drag up/down to set velocity. Up increases, down decreases. 100px of vertical movement covers the full 0–1 range. A quick click without dragging still toggles the step on/off.
- **Fill-bar velocity visual** - Step velocity is now shown as a bottom-up fill level instead of opacity. The green highlight fills proportionally to the velocity value, making it easy to see relative velocity levels across the grid.
- **Shift+click retained** - Shift+click still cycles through velocity levels (0 → 0.25 → 0.5 → 0.75 → 1.0 → 0) as a quick alternative.

### Removed

- **Right-click-drag velocity** - Replaced by the more intuitive left-click-drag interaction.
- **Opacity-based velocity display** - Replaced by the fill-bar visual.

## [1.11.0] - 2026-03-05

### Added

- **Step velocity support** - Each step in the sequencer grid now has a velocity value (0.0–1.0) instead of simple on/off. Velocity controls how much gain is applied on active steps, enabling accent patterns and groove variations within each frequency band.
- **Shift+click velocity cycling** - Shift+click a step cell to cycle through velocity levels: 0 → 0.25 → 0.5 → 0.75 → 1.0 → 0. Quick way to set accent patterns.
- **Right-click-drag velocity control** - Right-click and drag vertically on a step cell to set precise velocity. Top of cell = 1.0 (full), bottom = 0.0 (off).
- **Visual velocity feedback** - Step cells display velocity as opacity intensity. Full velocity (1.0) = fully opaque green, lower velocities appear progressively more transparent.

### Changed

- **Step parameters changed from AudioParameterBool to AudioParameterFloat** - `step_b{N}_s{M}` parameters now store velocity floats (0.0–1.0) instead of boolean on/off. Backward compatible: existing presets with false/true (0.0/1.0) load seamlessly.
- **Velocity-aware gain calculation** - `getTargetGainForBand()` now interpolates gain using `(1-depth) + velocity * depth`. At velocity 1.0, gain = 1.0 (same as old ON). At velocity 0, gain = 1.0 - depth (same as old OFF). Intermediate velocities produce proportional gain reduction.
- **WebView relay type** - Step grid relays changed from `WebToggleButtonRelay` to `WebSliderRelay` with corresponding `WebSliderParameterAttachment` for continuous value communication.
- **Random pattern** - Randomize button now generates random velocities (0.5–1.0 range) for active steps, creating natural accent variation instead of uniform full-velocity patterns.
- **Factory presets regenerated** - Version check forces regeneration to capture new parameter types.

### Technical Notes

- 128 step parameters: `AudioParameterFloat(0.0–1.0, step 0.01, default 0.0)` replacing `AudioParameterBool`
- Parameter IDs unchanged (`step_b{N}_s{M}`, version 1) — APVTS XML stores both types as float, enabling seamless migration
- Euclidean mode remains binary (velocity 0.0 or 1.0) since patterns are algorithmically generated
- DSP formula: `gain = (1.0f - depth) + velocity * depth` — linear interpolation between gated floor and unity
- UI opacity range: 0.3 (minimum visible) to 1.0 (full velocity) for active cells

## [1.10.0] - 2026-03-04

### Added

- **Per-band playhead highlighting** - Replaced the single scrolling playhead bar with independent per-cell highlights for each frequency band. Each band's current step glows bright green (`--playhead`) when playing, allowing bands running at different rates (e.g., Sub at 1/4, High at 1/16) to show their individual positions simultaneously.

### Changed

- **Per-band step atomics** - Added `bandStepAtomics[4]` to the processor for thread-safe per-band step communication to the GUI timer. Timer now sends 4 independent step positions instead of 1 global step.

### Removed

- **Scrolling playhead bar** - The vertical green bar that spanned all bands at a single column position. Replaced by per-cell highlighting which correctly represents polymetric sequencing.

## [1.9.0] - 2026-03-04

### Fixed

- **Attack/Release range expanded from 0-100ms to 0-500ms** - The previous 100ms maximum was too narrow for a gating effect, making parameter changes imperceptible at most tempos. Now extends to 500ms with logarithmic skew (0.4) for fine control at short times and dramatic sweeping at long times.

### Changed

- **Skewed slider response** - Attack and Release sliders now use logarithmic-style mapping: ~20ms at 25%, ~88ms at 50%, ~250ms at 75%, 500ms at 100%. Provides better resolution in the musically useful 0-50ms range.
- **Factory presets regenerated** - Version check forces preset regeneration to capture new parameter range.

## [1.8.0] - 2026-03-04

### Added

- **Separate Attack and Release parameters** - Replaced the single "Smoothing" parameter with independent Attack (0-100ms) and Release (0-100ms) controls. Attack controls fade-in time when a step turns ON; Release controls fade-out time when a step turns OFF. Fast attack + slow release creates plucky gates; slow attack + fast release creates swells. Both enforce a 2ms minimum.

### Changed

- **Custom BandEnvelope replaces SmoothedValue** - Per-band gain envelope uses a linear ramp with separate attack/release rates instead of JUCE's SmoothedValue (which only supports symmetric ramp times). One-pole LPF softener retained for smooth corners.
- **Factory presets updated** - Select presets now showcase asymmetric attack/release: Classic Sidechain (5/25ms), Trance Gate (2/5ms), Ambient Shimmer (40/60ms), Half-Time Feel (15/40ms), Euclidean Groove (5/10ms).
- **Factory preset versioning** - Presets auto-regenerate when plugin version changes, ensuring new parameters are captured.

### Removed

- **Smoothing parameter** - Superseded by Attack and Release. Old saved states will load with default 5ms attack/release.

## [1.7.0] - 2026-03-04

### Added

- **Per-band rate parameter** - Each frequency band (Sub, Low, Mid, High) now has its own Rate dropdown with 11 options: "Global" (follows the main Rate knob) plus all 10 tempo divisions (1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T, 1/4D, 1/8D). Enables polymetric sequencing — e.g., sub at 1/4, highs at 1/16.

### Technical Notes

- 4 new `AudioParameterChoice` parameters: `band0_rate` through `band3_rate` (default: "Global")
- Per-band step positions computed independently in `processBlock()` from host PPQ using each band's effective rate index
- Global playhead continues to follow the global Rate for consistent visual feedback
- Rate dropdowns appear in each band row between Clear/Random buttons and the Manual/Euclidean mode toggle
- `WebComboBoxRelay`/`WebComboBoxParameterAttachment` added to `BandRelays`/`BandAttachments` structs
- Full preset/automation compatibility: band rates saved/restored with plugin state

## [1.6.7] - 2026-03-04

### Improved

- **Cache all 128 step-cell DOM references in a 2D array** - `cachedCells[band][step]` is populated once after `renderGrid()`, replacing per-call `querySelectorAll`/`querySelector` in three hot paths: `updateStepVisibility()` (was querying all 128 cells), `updateStepVisual()` (was querying by attribute per cell), and `updateEuclideanGrid()` (was querying 32 cells per band). Also replaced `classList.add`/`remove` pairs with `classList.toggle` for cleaner conditionals.

## [1.6.6] - 2026-03-04

### Improved

- **Cache band-0 step cells for playhead positioning** - `updatePlayhead()` (called at 30Hz) no longer runs `querySelector()` and `.grid-area` lookup every tick. Band-0 cells are cached as an array after `renderGrid()` completes, and the grid area element is cached once. Cell is now accessed by index (`cachedBand0Cells[step]`) instead of a DOM query each frame.

## [1.6.5] - 2026-03-04

### Improved

- **Replace manual RMS loop with JUCE's AudioBuffer::getRMSLevel()** - Signal-presence detection in processBlock() now uses the built-in getRMSLevel() per channel with jmax, replacing the manual sumSquares loop. Same result, less code.

## [1.6.4] - 2026-03-04

### Improved

- **Eliminate redundant atomic load in processBlock()** - RMS silence detection now computes `signalPresent` as a local bool, stores it to the `hasAudioSignal` atomic for the GUI thread, and uses the local variable for audio-thread logic. Removes one unnecessary `atomic::load()` per block.

## [1.6.3] - 2026-03-04

### Fixed

- **Corrected misleading DryWetMixer comment** - The inline comment on the `dryWetMixer` member said "10ms max latency" but the constructor argument is `maximumDelayInSamples`, not milliseconds. Updated to "10 samples max delay for dry/wet alignment".

## [1.6.2] - 2026-03-04

### Removed

- **Dead `setupGlobalControls()` function** - Removed empty function (only contained a console.log) and its call from DOMContentLoaded handler. All parameter binding already handled by `initializeGlobalParameters()`. No functionality change.

## [1.6.1] - 2026-03-04

### Changed

- **Refactored per-band relays and attachments to array-based structs** - Replaced 24 individually-named relay members and 24 individually-named attachment members with `BandRelays` and `BandAttachments` structs using `std::array<..., 4>`. Constructor uses loops instead of copy-pasted blocks. Critical destruction order (relays → webview → attachments) preserved.

## [1.6.0] - 2026-02-07

### Added

- **Preset manager module** - Full preset save/load system using the Ouaricon preset-manager module. Users can save custom presets, browse factory presets via a dropdown menu, navigate with prev/next buttons, and import/export preset files via native system dialogs.
- **Preset bar in header** - Compact preset navigation bar with prev/next arrows, clickable preset name (opens dropdown), Load and Save buttons.
- **12 factory presets** - All existing factory presets (Init, Classic Sidechain, Trance Gate 16th, Dubstep Pulse, Ambient Shimmer, Polyrhythm 5-7-11, Bass Foundation, Hi-Hat Chop, Full Spectrum Gate, Euclidean Groove, Half-Time Feel, Triplet Bounce) are now available as JSON files in `~/Library/O-FreqPulse/Presets/Factory/`.
- **User presets** - Save your own presets to `~/Library/O-FreqPulse/Presets/User/`. Factory presets are read-only.

### Technical Notes

- Integrated `OuariconPresetManager` from `modules/persistence/preset-manager/` (v1.0.0)
- 9 native functions registered: `savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`, `savePresetWithDialog`, `loadPresetFromFile`
- Factory presets generated dynamically from existing `loadPreset()` method on first run
- State save/load delegates to preset manager's `getStateAsXml()`/`setStateFromXml()` while preserving tooltip state
- `FileChooser` used for native save/load dialogs (async, non-blocking)
- Preset dropdown uses `:has()` CSS selector to escape stacking context and appear above other UI elements
- JavaScript `PresetManager` class imported as ES module from `preset-manager.js`

## [1.5.1] - 2026-02-07

### Changed

- **UI title updated** - Header and page title changed from "O-FreqPulse" to "Ouaricon Frequency Pulse" for consistent branding.

## [1.5.0] - 2026-02-07

### Added

- **Tooltip toggle system** - A `?` button in the bottom-right corner toggles tooltips on/off. When enabled, hovering over any UI element displays a descriptive tooltip explaining what it does. Tooltip state is saved with the plugin preset and restored on load.
- Tooltips for all UI elements: Mix, Steps, Rate, Swing, Smoothing, crossover dividers, frequency boundaries, band labels, clear/random buttons, mode toggles, expand buttons, Euclidean panel controls (Steps, Pulses, Offset, Depth), and the step grid area.

### Technical Notes

- Tooltip toggle state persisted via `tooltipsEnabled` atomic<bool> in PluginProcessor, saved/restored in XML state
- C++ native function `setTooltipsEnabled` registered via `withNativeFunction()` for JS→C++ communication
- State synced from processor to WebView on first timer callback via `restoreTooltipState()` JS function
- Smart tooltip positioning: appears above element by default, falls below if too close to top edge, constrained within container bounds horizontally
- CSS uses naturalist theme colors (dark brown bg, warm paper text, green accent for active state)
- `data-tooltip` attributes on both static HTML elements and dynamically created elements in `createBandRow()` and `createDividerSlider()`

## [1.4.0] - 2026-02-06

### Changed

- **Variable step count (2-32)** - Steps parameter changed from a fixed 4-option dropdown (4, 8, 16, 32) to a continuous slider allowing any integer from 2 to 32. Enables odd time signatures and non-power-of-two patterns like 5, 7, 12, etc.

### Technical Notes

- Parameter type changed from `AudioParameterChoice` to `AudioParameterInt` (range 2-32, default 16)
- Parameter version bumped to 2 for the "steps" ID to handle state migration
- UI changed from `<select>` dropdown to range slider with live value display
- Editor relay changed from `WebComboBoxRelay`/`WebComboBoxParameterAttachment` to `WebSliderRelay`/`WebSliderParameterAttachment`
- All 12 factory presets updated to use direct step count values
- Processor reads step count directly (`jlimit(2, 32, ...)`) instead of indexing into a lookup array

## [1.3.2] - 2026-02-06

### Fixed

- **Two-stage gain smoothing eliminates residual step-transition clicks** - Added a one-pole lowpass filter after the SmoothedValue linear ramp to soften the discontinuous first derivative at ramp start/end into a smooth S-curve. Also enforced a 2ms minimum smoothing time to prevent instant gain jumps when the user sets smoothing to zero.

### Technical Notes

- SmoothedValue produces a linear ramp with sharp corners at onset/offset — in STFT-reconstructed audio these corners can produce brief transient clicks
- One-pole LPF with ~1.5ms time constant (`1 - exp(-1 / (0.0015 * sampleRate))`) applied per-sample after `getNextValue()` rounds the ramp into an S-curve
- `bandGainFiltered[4]` array tracks the filtered gain state per band, initialized to 1.0 in `prepareToPlay()`
- Minimum smoothing floor of 2ms prevents degenerate zero-length ramps

## [1.3.1] - 2026-02-06

### Fixed

- **Eliminated clicks at step onset/offset transitions** - Two root causes addressed:
  1. `SmoothedValue::reset()` was called every processBlock, which internally calls `setCurrentAndTargetValue(target)` — instantly snapping the gain to its target and killing any in-progress smoothing ramp. Now only called when the smoothing parameter actually changes.
  2. Step transitions were detected once per block boundary, not at the exact sample. Added sample-accurate PPQ interpolation within the processBlock loop so gain target changes align precisely with beat positions.

### Technical Notes

- `SmoothedValue::reset(sampleRate, rampLength)` calls `setCurrentAndTargetValue(target)` which sets `currentValue = target` and `countdown = 0`. Calling this every block truncated ramps shorter than the buffer size (e.g., 5ms = 220 samples truncated at 128-sample buffers).
- PPQ is now interpolated per-sample using host BPM: `samplePpq = blockStartPpq + ppqPerSample * sampleIndex`. Step transitions detected within the sample loop trigger immediate `setTargetValue()` calls at the exact transition sample.
- Free-running standalone mode also benefits from the same per-sample PPQ tracking.

## [1.3.0] - 2026-02-06

### Changed

- **Inline Manual/Euclidean mode toggle** - The "Manual" label on each band lane is now a clickable toggle that switches between Manual and Euclidean modes directly in the main UI. No longer requires opening the popup panel just to change modes.
- **Expand button visibility** - The `▶` button to open euclidean controls (Steps, Pulses, Offset, Depth) is now hidden when a band is in Manual mode and only appears when Euclidean mode is active.
- **Removed mode toggle from popup panel** - The euclidean controls panel now only shows the pattern parameters (Steps, Pulses, Offset, Depth) since mode switching is handled inline.
- **Auto-close panel** - Switching a band back to Manual mode automatically closes the euclidean panel if it was open for that band.

## [1.2.0] - 2026-02-05

### Fixed

- **Eliminated buzzing artifact at step on/off transitions** - Moved band gain application from spectral domain (per-FFT-frame) to time domain (per-sample), eliminating the ~86Hz amplitude modulation caused by applying different gains to overlapping STFT frames.

### Technical Notes

- Root cause: with 75% overlap (hopSize=512), applying band gain once per FFT frame meant overlapping frames received different gain values, creating amplitude modulation at the frame rate (44100/512 ≈ 86Hz = audible buzz)
- Fix: each band's frequency bins are now reconstructed separately via IFFT into per-band time-domain output FIFOs, then per-sample `SmoothedValue::getNextValue()` gain is applied in the time domain before summing
- Added per-band output FIFO buffers (`bandOutputFifo[4][2]`), passthrough FIFO for unassigned bins, and temporary FFT buffer for per-band IFFT
- `processFrame()` now produces 5 separate overlap-add outputs (4 bands + passthrough) instead of a single gain-weighted output
- Passthrough bins (frequencies not assigned to any band) are reconstructed once and passed through at unity gain
- COLA (Constant Overlap-Add) compliance maintained: Hann synthesis window + correction factor applied per-band

## [1.1.2] - 2026-02-04

### Fixed

- **Euclidean patterns now display on the grid** - When Euclidean mode is enabled for a band, the generated pattern visually populates the step grid cells with a distinct warm brown color. Previously, the C++ processor computed Euclidean patterns internally but never communicated them to the UI, so the grid always showed the stale manual pattern.

### Technical Notes

- Root cause: `euclideanPatterns[band]` array in C++ was used for audio processing but never written to `step_b{N}_s{M}` parameters or sent to the WebView
- Fix is UI-only: Bresenham euclidean algorithm replicated in JavaScript (`generateEuclidean()`)
- When euclidean mode is active: grid shows computed pattern, manual step clicking is disabled, clear/random buttons are blocked
- When euclidean mode is off: manual step parameters are restored to display, clicking re-enabled
- Manual patterns are preserved in step parameters and never overwritten by euclidean mode
- Euclidean grid updates reactively when euc_steps, euc_pulses, or euc_offset parameters change

## [1.1.1] - 2026-02-04

### Fixed

- **Playhead no longer moves when no audio is present** - Added input RMS detection (~-60 dB threshold) that gates playhead advancement in both host-sync and standalone modes. Playhead fades out when signal drops below threshold and reappears when audio returns.

### Technical Notes

- Root cause: playhead step counter advanced unconditionally in processBlock regardless of input signal level
- RMS computed per buffer across all channels; threshold at 0.001f (~-60 dB)
- Signal state communicated to WebView via existing timer callback (30 Hz)
- Playhead opacity animated with 150ms CSS ease transition for smooth fade

## [1.1.0] - 2026-02-04

### Added

- **Clear button per lane** - Resets all 32 steps in a band to OFF state
- **Random button per lane** - Fills steps with 50% probability pattern
- Both buttons appear in each band row between the step grid and mode indicator

### Technical Notes

- Lane actions use Unicode symbols for compact display (⌀ for clear, ⚄ for random)
- Random pattern applies to all 32 steps regardless of current step count setting
- Parameter changes propagate immediately to JUCE backend via existing toggle bindings

## [1.0.0] - 2026-02-04

### Added

- Initial release
- 4-band spectral step sequencer (SUB, LOW, MID, HIGH)
- 32-step grid with variable step counts (4, 8, 16, 32)
- Euclidean rhythm generator per band
- Tempo-synced playback with swing
- WebView UI with naturalist aesthetic
- 12 factory presets
