# O-SpectralShaper Changelog

## [1.3.2] - 2026-07-07

### Fixed
- **CR-01 — Preset file-dialog use-after-free:** the `savePresetWithDialog` and `loadPresetFromFile` `FileChooser::launchAsync` completions captured raw `this` + the WebView-owned `complete` callback. If the plugin window closed while a dialog was open, the completion dereferenced the freed editor and invoked a dead callback. Now capture a `juce::Component::SafePointer` and bail with a bare `return` on teardown (matching the correct pattern already used in `parentHierarchyChanged`).
- **CR-02 — Factory presets ignored the ATTACK_TIME/SUSTAIN_TIME skew:** preset values were authored as linear fractions that ignored the 0.3 skew, so every preset recalled attack/sustain times ~10–30× too short and the "Default" preset did not match the plugin's power-on state. Factory preset values are now authored in engineering units (ms/dB/fractions) and converted through each parameter's own `NormalisableRange` via `convertTo0to1()` at init, so the skew is applied correctly. "Default" now reproduces the APVTS defaults (ATTACK_TIME 10 ms, SUSTAIN_TIME 100 ms).
- **WR-01 — Attack/Sustain knob readouts showed wrong numbers:** the JS `formatValue` callbacks re-derived the skewed range with an incorrect exponential formula (attack displayed ~0.3 ms at the 10 ms default). They now read the real engineering value from JUCE via `Juce.getSliderState(id).getScaledValue()`.
- **WR-02 — Curve-less presets left stale curves applied:** loading a preset without `customState` (Default, Gentle Shaping, Aggressive Bite, Sustain Lift) left the previously-loaded preset's attack/sustain curves in effect. These four presets now carry an explicit flat (all-0.0) curve `customState`, so loading them resets both curves to neutral.
- **WR-03 — Latency re-reported from the audio thread every block:** `processBlock` called `setLatencySamples()` on every block, continuously signalling a (potentially changed) latency from the audio thread — which many hosts glitch on or ignore. Latency is now cached in `lastReportedLatency` and re-signalled only when it actually changes (Lookahead toggled or its time changed). Only the latency-thrash bug is fixed here; the Lookahead control's underlying no-op behaviour is a known limitation deferred to a dedicated DSP pass.

### Known Limitations
- **Lookahead is currently inert:** enabling Lookahead delays both the transient detection and the shaped signal by the same amount, so gain and signal stay time-aligned and nothing audible changes (it only adds reported latency). True lookahead requires splitting detection from application in `STFTProcessor` and is deferred to a future release.

## [1.3.1] - 2026-07-01

### Fixed
- Preset-manager module sync (`preset-manager` v1.0.2) — fixes from the O-DigiDelay code review:
  - **WR-04:** preset names are sanitized before use as filenames (`/\\:` → `_`) in save/load/delete/isFactory, so a name containing `/` no longer silently drops the file.
  - **IN-02:** preset JSON records the real plugin version (`JucePlugin_VersionString`) instead of a hard-coded `"1.0.0"`.
  - **IN-03:** prev/next resume from the last in-list position instead of snapping to index 0 after loading an out-of-list preset from file.
  - **IN-01:** corrected the preset-path docstring.

## [1.3.0] - 2026-03-08

### Added

- **Real-time spectrum overlay on curve editors:** Toggle "Spectrum" button on each curve editor to display the live input FFT magnitude as a semi-transparent filled shape behind the editable curve. Maps 257 FFT bins to the logarithmic X-axis with 60dB dynamic range. Uses accent-colored fill (blue for attack, orange for sustain) at ~15% opacity so the curve remains clearly visible. Spectrum state persists across freehand/node mode switches.
- **Spectrum toggle buttons:** Each curve editor (attack, sustain) has a "Spectrum" button in the control bar. Active state shows accent-colored highlight matching the curve type.

## [1.2.0] - 2026-03-08

### Added

- **Undo/redo for curve editors:** Both freehand and node curve modes now support undo (Ctrl/Cmd+Z) and redo (Ctrl/Cmd+Shift+Z) with a 30-step snapshot stack per editor. Snapshots capture at action boundaries (stroke start, node add/move/delete, reset) so undo reverts entire gestures, not intermediate frames. NodeCurve snapshots include node positions for full state restoration.
- **Undo/redo UI buttons:** Small arrow buttons added to each curve editor's control bar with automatic enable/disable state tracking.
- **Focus-aware keyboard routing:** Keyboard shortcuts route to whichever curve editor (attack or sustain) was last clicked, with proper cleanup on editor destroy/mode switch.

## [1.1.5] - 2026-03-08

### Fixed

- **Add synthesis window to STFT overlap-add reconstruction:** Previously only the analysis Hann window was applied before the forward FFT. Now a Hann synthesis window with per-sample WOLA normalization is applied after the inverse FFT, smoothing frame-boundary discontinuities caused by spectral modification. Reduces metallic ringing and "musical noise" artifacts during aggressive shaping. Root cause: Hann² is not COLA at 50% overlap (sum varies 0.5–1.0), so a precomputed normalized synthesis window `w[i] / (w²[i] + w²[i+H])` is used for correct reconstruction. Zero additional runtime cost (single multiply, precomputed table).

## [1.1.4] - 2026-03-08

### Changed

- **Remove dead code:** `hopTime` member in STFTProcessor (set but never read), empty `loadCurvesFromProcessor()` in app.js, redundant `#include <juce_dsp/juce_dsp.h>` in PluginProcessor.h (already included via STFTProcessor.h).
- **Encapsulation:** `handleAttackCurveUpdate`, `handleSustainCurveUpdate`, `sendAttackCurveToJS`, `sendSustainCurveToJS` moved from public to private in PluginEditor.h.
- **Deduplicate curve handlers:** Identical `handleAttackCurveUpdate`/`handleSustainCurveUpdate` merged into single `handleCurveUpdate(args, setter)` using member function pointer.
- **Deduplicate dialog result creation:** Extracted `makeDialogResult` helper lambda for preset save/load file dialog callbacks.
- **Remove FreehandCurve.drawCurve() indirection:** Eliminated wrapper that just called `drawDataCurve()`; renamed `drawDataCurve` to `drawCurve` directly.
- **Fix JS event listener leaks:** Store bound references for CurveEditor resize handler, Spectrogram WebGL context lost/restored handlers, and NodeCurve keydown listener; add `destroy()` methods for proper cleanup.

## [1.1.3] - 2026-03-08

### Changed

- **Cache APVTS parameter pointers:** `getRawParameterValue()` pointers now cached as class members in constructor instead of 7 string-hash lookups per `processBlock` call.
- **Replace magic number 512:** Dry delay buffer size and modulo operations now use `STFTProcessor::FFT_SIZE` constant.
- **Reuse lastMagnitudes[] in detectTransients():** Band magnitude loop reuses pre-computed magnitudes instead of recomputing `sqrt(r²+i²)` for ~257 bins per frame.
- **Pre-allocate visualization JSON:** `emitVisualizationFrame()` uses `preallocateBytes(4096)` and `<<` operator instead of repeated `+=` at 60fps.
- **Cache getBandFrequencies() and log constants:** Band frequencies computed once in constructor. `logMinFreq`/`logMaxFreq`/`logFreqRange` cached for `freqToX()`/`xToFreq()` (called hundreds of times per render frame).

## [1.1.2] - 2026-03-08

### Fixed

- **Dangling pointer crash in parentHierarchyChanged:** `callAfterDelay(100, [this]{...})` could fire on a destroyed editor if the DAW closed/recreated the UI within 100ms. Now guarded with `juce::Component::SafePointer`.
- **Unclamped/NaN curve values from WebView:** `handleAttackCurveUpdate` and `handleSustainCurveUpdate` now clamp values to [-1.0, 1.0] and replace NaN with 0.0 before passing to DSP.
- **Division by zero in NodeCurve.interpolateAndSample():** When two nodes share the same frequency, the denominator `(rightNode.freq - leftNode.freq)` was zero, producing NaN that propagated through curve data. Now guards the denominator and falls back to `t = 0.0` (left node's gain).

## [1.1.1] - 2026-03-08

### Fixed

- Attack/sustain time knobs not responding
- Curve data race condition
- Lookahead latency reporting

## [1.1.0] - 2026-02-07

### Added

- Initial release with WebView UI

## [1.0.0] - 2026-02-03

### Added

- Per-frequency transient shaping with 32 logarithmic bands
- Freehand and node-based curve editing
- Real-time spectrogram with transient heat overlay
