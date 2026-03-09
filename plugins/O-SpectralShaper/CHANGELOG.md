# O-SpectralShaper Changelog

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
