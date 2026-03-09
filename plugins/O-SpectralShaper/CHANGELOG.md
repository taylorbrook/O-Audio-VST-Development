# O-SpectralShaper Changelog

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
