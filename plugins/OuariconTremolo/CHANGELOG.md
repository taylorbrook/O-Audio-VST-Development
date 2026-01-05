# Changelog

All notable changes to OuariconTremolo will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1] - 2026-01-05

### Fixed

- **Critical crash on plugin load** - Fixed initialization order bug that caused segmentation fault during editor construction
  - **Root Cause #1**: `setSize(600, 400)` was called in constructor BEFORE `webView` was created. When JUCE tried to resize child components, it dereferenced a nullptr causing crash at `Component::setBounds()`.
  - **Root Cause #2**: `WebBrowserComponent::goToURL()` was called in constructor before component was attached to a native window. In JUCE 8, WebKit requires valid window context for navigation.
  - **Solution #1**: Moved `setSize()` to END of constructor, after all components are created and added
  - **Solution #2**: Moved WebView navigation from constructor to `parentHierarchyChanged()` callback
  - **Impact**: Plugin now loads successfully in all DAWs (Logic Pro, Ableton, Reaper) and passes AU/VST3 validation

### Technical Notes

- Fixed component initialization order: relays → webView → attachments → addAndMakeVisible → setSize
- Added `parentHierarchyChanged()` override for safe WebView navigation
- Implemented one-time navigation guard using static flag
- Added safety checks (`isShowing()` and `webView != nullptr`) before navigation
- Exception type: `EXC_BAD_ACCESS (SIGSEGV)` at `KERN_INVALID_ADDRESS 0x0000000000000040`

## [1.0.0] - 2026-01-04

### Added

- Initial release of OuariconTremolo
- Tremolo effect with 6 waveform types (Sine, Triangle, Phasor, Noise, Square, Pulse)
- Speed control (0.1-20.0 Hz)
- Depth control (0-100%)
- Smoothing filter (0-100%)
- Pan Sync mode (stereo modulation with 180° L/R phase offset)
- Tempo Sync (locks to DAW BPM with note division quantization)
- Botanical vintage WebView UI with real-time waveform visualizer
- VST3 and AU formats
- macOS support

### Known Issues

- Plugin crashes on load (fixed in v1.0.1)
