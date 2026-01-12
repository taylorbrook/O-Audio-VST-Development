# Ouaricon Digital Delay Changelog

All notable changes to this plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
