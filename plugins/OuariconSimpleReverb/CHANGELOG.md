# Changelog

All notable changes to OuariconSimpleReverb will be documented in this file.

## [1.0.1] - 2026-01-13

### Fixed

- **Missing check_native_interop.js:** Added required JUCE WebView interop file that was missing from resource provider. This was the root cause of all WebView parameter binding failures.
- **JavaScript rewrite:** Rewrote parameter binding to match working DriveVerb pattern - removed DOMContentLoaded wrapper, use lastY tracking instead of startValue pattern.
- **Knobs not interactable:** Fixed by adding check_native_interop.js and correcting JS API usage.
- **Reverb type switching had no effect:** Fixed by proper ComboBox state binding with getChoiceIndex/setChoiceIndex methods.

### Changed

- **Title:** Changed from "Simple Reverb" to "Ouaricon Simple Reverb" for brand consistency.

## [1.0.0] - 2026-01-13

### Added

- Initial release
- 6 reverb types: Booth, Room, Hall, Spring, Plate, Ambient
- Character control (Warm/Neutral/Bright tonal shaping)
- Independent Wet and Dry level controls
- Decay time control (0.1s to 10s)
- Room Size control
- Ouaricon Naturalist aesthetic (botanical theme)
- WebView-based GUI with custom knob interactions
