# Changelog

All notable changes to OuariconTremolo will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.1] - 2026-01-05

### Fixed

- **UI polish improvements** - Enhanced visual layout and interactivity
  - Centered text in waveform dropdown menu for better visual balance
  - Tightened spacing between knobs and their labels/values (gap reduced from 8px to 4px)
  - Adjusted depth dial position upward by 5px for improved vertical alignment
  - Waveform visualizer now responds to depth parameter (amplitude scales with depth 0-100%)

### Technical Notes

- CSS modifications only (no C++ changes required)
- Waveform amplitude calculation: `baseAmplitude * depthNormalized` where depthNormalized = 0.0 to 1.0
- Added `.depth-knob-container` class for specific positioning control
- All changes are visual-only, no parameter behavior changes

## [1.1.0] - 2026-01-05

### Added

- **Musical division display when tempo sync is enabled** - Speed dial now displays musical rhythmic values (e.g., "1/8T", "1/4Q") instead of Hz when tempo sync is ON
  - Expanded from 6 to 16 musical divisions including:
    - Straight divisions: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32
    - Triplet divisions: 1/2T, 1/4T, 1/8T, 1/16T, 1/32T (3 notes in space of 2)
    - Quintuplet divisions: 1/2Q, 1/4Q, 1/8Q, 1/16Q, 1/32Q (5 notes in space of 4)
  - Preserves musical relationship when DAW tempo changes (Option A parameter storage)
  - Hz display retained when tempo sync is OFF for continuous frequency control

### Changed

- **Tempo sync behavior** - Speed parameter now snaps to musical divisions when synced, ensuring tremolo stays locked to musical timing regardless of tempo changes
- **UI responsiveness** - Speed display automatically switches between Hz and musical notation when toggling tempo sync button

### Technical Notes

- Updated `MusicalDivision` table in PluginProcessor.cpp with 16 divisions using precise beat multipliers
- Modified WebView UI JavaScript to detect tempo sync state and format speed display accordingly
- Beat multiplier calculations:
  - Triplets: `(base_beats * 2/3)` - e.g., 1/8T = 0.5 beats * 2/3 = 0.333 beats
  - Quintuplets: `(base_beats * 4/5)` - e.g., 1/8Q = 0.5 beats * 4/5 = 0.4 beats
- No breaking changes - existing presets and sessions remain compatible

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
