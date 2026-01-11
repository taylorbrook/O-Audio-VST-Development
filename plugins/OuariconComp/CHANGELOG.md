# OuariconComp Changelog

All notable changes to this plugin will be documented in this file.

## [1.0.1] - 2026-01-11

### Fixed

- **Attack, Release, Output knobs not responding**: Fixed JavaScript ID mapping in WebView UI. The `updateKnob` function was incorrectly converting parameter IDs (e.g., `attack_time` → `attack-time-indicator`) when the actual element IDs used a different pattern (`attack-indicator`). Added explicit parameter-to-element ID mapping.

- **Envelope and Gain Reduction display showing fake animation**: Replaced placeholder animation (using `Math.sin()` and `Date.now()`) with real-time metering data from the DSP. The envelope display now shows actual signal envelope history as a scrolling waveform, and gain reduction is displayed as a histogram with current GR value.

- **Input/Output meters showing random animation**: Replaced `Math.random()` placeholder animation with actual input and output level monitoring. Added atomic metering variables to the processor, timer-based polling in the editor, and JavaScript-C++ bridge via `evaluateJavascript`.

### Technical Details

- Added atomic metering variables to `PluginProcessor`: `inputLevelDB`, `outputLevelDB`, `currentGainReductionDB`, `currentEnvelopeDB`
- Added 30Hz timer in `PluginEditor` to push meter data to WebView
- Added `updateMeters()` JavaScript function callable from C++
- Added rolling history buffers (300 samples) for envelope and GR visualization

## [1.0.0] - 2026-01-11

### Added

- Initial release
- WebView UI with Ouaricon Naturalist aesthetic
- Parameters: Threshold, Ratio, Attack, Release, Knee, Output Gain, Auto-Gain
- Soft-knee compression algorithm with accurate envelope following
- Transfer curve visualization
- Stereo-linked detection
