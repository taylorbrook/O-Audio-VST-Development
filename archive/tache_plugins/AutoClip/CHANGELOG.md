# Changelog

All notable changes to AutoClip will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1] - 2025-11-15

### Fixed
- WebView UI controls (knob and switch) now respond to mouse interaction
- Root cause: Missing `check_native_interop.js` prevented WebView native bridge initialization
- Added JUCE native interop verification script to UI resources
- Updated CMakeLists.txt to include `check_native_interop.js` in binary data
- Added resource handler for native interop checker in PluginEditor.cpp

### Technical Details
- Pattern #13 from juce8-critical-patterns.md (check_native_interop.js required for all WebView plugins)
- File copied from GainKnob reference implementation
- Completes WebView C++ ↔ JavaScript bridge initialization sequence
- Enables bidirectional parameter communication (UI drag → DSP, automation → UI update)

### Testing Notes
- DAW automation already worked (C++ parameter system functional)
- Fix specifically enables UI mouse interaction and visual feedback
- Verified against TapeAge pattern (same root cause, documented solution)

## [1.0.0] - 2025-11-13

### Added
- Hard clipping algorithm with user-adjustable threshold (0-100%)
- Automatic peak-based gain matching to maintain consistent loudness
- Fixed 5ms lookahead buffer for smooth transient anticipation
- Clip solo mode (delta monitoring) to isolate clipped portion
- Input and output level meters with ballistic motion
- Real-time clipping indicator
- Vintage Bakelite aesthetic WebView UI (300×500px)
- Two-way parameter binding (UI ↔ DSP)
- Factory presets: Default, Subtle Clip, Moderate Clip, Heavy Clip, Extreme Clip
- VST3 and AU formats
- Latency reporting (5ms) for host compensation

### Technical Details
- Peak detection with 50ms gain smoothing to prevent zipper noise
- juce::dsp::DelayLine for lookahead buffer
- juce::SmoothedValue for gain compensation
- Real-time meter updates at 30 Hz
- ES6 module-based WebView with relative drag knob interaction
- JUCE 8 compatible

### Use Cases
- Drum processing with automatic gain compensation
- Aggressive clipping for transient-rich material
- A/B testing clipped vs clean signals (clip solo mode)
- Adding harmonic distortion while maintaining peak levels
