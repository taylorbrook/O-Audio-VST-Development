# Changelog

All notable changes to OuariconSimpleReverb will be documented in this file.

## [1.0.1] - 2026-01-13

### Fixed

- **Knobs not interactable:** WebView JavaScript was using non-existent property `state.normalisedValue` instead of correct methods `getNormalisedValue()` and `setNormalisedValue()`. Knobs now respond to mouse drag interactions.
- **Reverb type switching had no effect:** WebView JavaScript was using non-existent property `state.selectedId` instead of correct methods `getChoiceIndex()` and `setChoiceIndex()`. Switching between Booth/Room/Hall/Spring/Plate/Ambient now audibly changes the reverb character.
- **Added proper drag lifecycle:** Added `sliderDragStarted()` and `sliderDragEnded()` calls for proper DAW automation recording.

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
