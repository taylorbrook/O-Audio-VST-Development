# OuariconAnalogEQ Changelog

## [1.0.3] - 2026-01-11

### Fixed
- **Dual-layer knobs now functional** - Implemented distance-based hit detection
  - Outer ring (>60% from center) controls frequency
  - Inner dial (<60% from center) controls gain
  - Added outer ring indicator for visual feedback
  - Added value tooltips showing both freq and gain values
- **VU meter now responds to actual audio levels** - Marimba-style implementation
  - C++ PluginProcessor calculates peak output level in processBlock
  - PluginEditor uses Timer to emit `outputLevel` events to WebView at 30Hz
  - JavaScript animates needle with ballistic motion (fast attack, slow decay)
  - Needle color interpolates from green (quiet) to red (loud)

## [1.0.2] - 2026-01-11

### Fixed
- **Missing check_native_interop.js** - Added the required JUCE WebView JavaScript bridge file
  - Root cause: JavaScript module `index.js` imports `check_native_interop.js` which sets up `window.__JUCE__.backend`
  - Without this file, no C++ ↔ JavaScript communication was possible
  - Added file to CMakeLists.txt resources and PluginEditor.cpp resource provider

## [1.0.1] - 2026-01-11

### Fixed
- **GUI controls now interactable** - Fixed type mismatch between C++ WebView relays and JavaScript state accessors
  - Root cause: Q parameters (`lmf_q`, `hmf_q`) were using `WebSliderRelay` but JavaScript expected `WebComboBoxRelay`
  - Changed `lmfQRelay` and `hmfQRelay` from `WebSliderRelay` to `WebComboBoxRelay`
  - Changed `lmfQAttachment` and `hmfQAttachment` from `WebSliderParameterAttachment` to `WebComboBoxParameterAttachment`

## [1.0.0] - 2026-01-11

### Added
- Initial release
- 4-band analog-style EQ (LF shelf, LMF bell, HMF bell, HF shelf)
- Per-band frequency, gain, and Q controls (Q on bell bands only)
- Per-band bypass toggles
- Global output gain control
- Analog warmth/saturation toggle
- VU meter display
- WebView UI with botanical paper aesthetic
