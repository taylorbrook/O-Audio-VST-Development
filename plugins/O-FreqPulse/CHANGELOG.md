# Changelog

All notable changes to O-FreqPulse will be documented in this file.

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
