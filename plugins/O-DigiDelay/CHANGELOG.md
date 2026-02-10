# O-DigiDelay Changelog

All notable changes to this plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.5] - 2026-02-09

### Changed
- Migrated internal project files from `.ideas/` to `.planning/` directory structure

## [1.2.4] - 2026-02-08

### Added
- Licensing module integration (compile-flag gated, OFF for local dev)
- Ouaricon shared module system support via OuariconModules.cmake

### Changed
- Extracted `getChoiceIndexSafe()` helper for safe combo box index access
- Removed unused `formatters` object from JavaScript UI code

## [1.2.3] - 2026-02-07

### Changed
- Knobs replaced with SVG vine-arc style (matching O-Detune visual system)
  - Removed old CSS conic-gradient knobs with physical indicator lines
  - New SVG circular arc knobs with smooth animated fill
  - 270-degree arc with stroke-dashoffset rendering
  - RequestAnimationFrame interpolation (0.15 smoothing factor)
  - Mouse wheel support added to all knobs
  - Time knob sync/division logic preserved unchanged
- Track color: rgba(139, 115, 85, 0.3), vine color: #5a7a6a

## [1.2.2] - 2026-02-07

### Changed
- UI title changed from "O-DIGIDELAY" to "OUARICON DIGITAL DELAY"
- HTML document title updated to match

## [1.2.1] - 2026-01-24

### Changed
- Renamed plugin from "Ouaricon Digital Delay" to "O-DigiDelay" across:
  - DAW display name and window title
  - Binary files (O-DigiDelay.vst3, O-DigiDelay.component)
  - Source folder (plugins/O-DigiDelay/)
  - Preset folder path (~\~/Library/O-DigiDelay/)
- Plugin codes (OuDD) unchanged for session compatibility

### Migration Notes
- Existing presets migrated from `~/Library/Ouaricon Digital Delay/` to `~/Library/O-DigiDelay/`
- DAW sessions will recognize plugin (same plugin code OuDD)

## [1.2.0] - 2026-01-12

### Added
- Preset Manager integration with save/load functionality
  - Save presets to `~/Library/O-DigiDelay/Presets/User/`
  - Load presets from file or browse available presets
  - Navigate presets with previous/next buttons
  - Click preset name to see dropdown of all presets
- 12 factory presets:
  - Short Slap, Long Ambient, Stereo Wide, Subtle Doubler
  - Tape Echo, Eighth Note Sync, Dotted Eighth, Triplet Feel
  - Swell Pad, Lo-Fi Drift, Clean Repeat, Ping Pong Style

### Changed
- Header layout restructured: title moved to left, preset bar added on right
- Preset bar positioned 50px from right edge to avoid output meter label overlap
- WebView navigation now uses `parentHierarchyChanged` pattern for JUCE 8 stability
- State serialization now includes preset manager (preserves current preset name in DAW sessions)

### Technical Notes
- Added `OuariconPresetManager` C++ integration
- 9 native functions registered for preset operations
- Preset Manager JavaScript module imported from module system

## [1.1.3] - 2026-01-12

### Fixed
- Time dial now responds to dragging when Sync is enabled
  - Root cause: JavaScript was calling non-existent JUCE WebComboBoxState methods
  - `getSelectedId()` → `getChoiceIndex()` (JUCE 8 API)
  - `setSelectedId()` → `setChoiceIndex()` (JUCE 8 API)
  - All 5 occurrences corrected in index.html

## [1.1.2] - 2026-01-12

### Changed
- Time dial sync completely rewritten following Tremolo pattern
  - Uses virtual normalized position for smooth drag feel
  - Snaps to nearest division (12 discrete positions) when Sync is ON
  - Same sensitivity (0.005) as Tremolo for consistent control feel
  - Proper initialization of drag position from current division
  - Display shows rhythmic values (1/4, 1/8, 1/4D, 1/8T, etc.)

## [1.1.1] - 2026-01-12

### Changed
- Butterfly image shifted up additional 30 pixels (total 70px from v1.0.0)
- Output meter sensitivity increased significantly
  - Applied 6x gain boost to RMS signal
  - Added logarithmic curve for better response to quieter signals

## [1.1.0] - 2026-01-12

### Added
- Output LED meter now displays actual signal level (RMS) via timer-based C++ to WebView messaging
- Musical divisions table in JavaScript for tempo-synced time display
- Time knob now shows musical division names (1/4, 1/8, 1/4T, etc.) when Sync is enabled

### Changed
- Replaced division dropdown with integrated tempo-synced time dial
  - When Sync is OFF: Time dial controls delay time in ms (1-2000ms)
  - When Sync is ON: Time dial selects from 12 musical divisions
- Shifted butterfly image up by 40 pixels for better visual balance
- Time knob display contextually updates based on Sync toggle state

### Removed
- Division dropdown selector (functionality moved to time dial when Sync is ON)

### Technical Notes
- Added `getRmsLevelLeft()` and `getRmsLevelRight()` public getters to PluginProcessor
- Added Timer inheritance to PluginEditor for 30Hz RMS meter updates
- Exposed `updateLEDMeter()` function globally (window scope) for C++ evaluateJavascript calls
- Division parameter still used internally (controlled by time knob when Sync is ON)

## [1.0.0] - 2026-01-12

### Added
- Initial release
- Clean digital delay with Lagrange3rd interpolation
- Tempo sync with 12 musical subdivisions (straight, dotted, triplets, quintuplets)
- Stereo spread via Haas effect (0-15ms offset)
- Delay time modulation (chorus-like movement)
- Feedback with stability limiting (max 95%)
- Separate Wet/Dry controls
- WebView UI with botanical paper aesthetic
- LED output meter (visual only in v1.0.0)
