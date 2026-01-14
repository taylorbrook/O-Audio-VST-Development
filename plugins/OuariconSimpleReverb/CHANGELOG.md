# Changelog

All notable changes to OuariconSimpleReverb will be documented in this file.

## [1.2.0] - 2026-01-13

### Changed

- **Knob design:** Replaced simple gradient knobs with Ouaricon aesthetic botanical seed cross-section style (matching Ouaricon Tremolo)
- **Type dropdown:** Removed "Type" label, centered text inside dropdown menu
- **Dial layout:** Moved dial labels above dials, keeping value readings below
- **Character display:** Now shows "warm" (0-25%), "neutral" (25-75%), "bright" (75-100%) instead of percentage
- **Background image:** Changed to full-size centered positioning (no scaling/cropping)
- **Bottom row layout:** Reorganized with Decay and Size dials flanking central VU meter

### Added

- **VU meter:** Small circular level meter (50px) positioned between Decay and Size dials, matches Ouaricon Analog EQ style
- **Knob indicators:** Triangle-style position indicators on each dial

### Technical Notes

- UI-only changes, no DSP modifications
- No parameter changes - fully compatible with v1.1.x presets
- VU meter listens for `outputLevel` events from JUCE backend

## [1.1.0] - 2026-01-13

### Added

- **Type-specific DSP processing:** Each reverb type now has distinct sonic character through dedicated DSP chains:
  - **Booth:** Very short pre-delay (3ms), minimal early reflections, narrow stereo, high-pass EQ at 150Hz
  - **Room:** Natural 15ms pre-delay, balanced early reflections, full stereo width, neutral EQ
  - **Hall:** Long 50ms pre-delay, spacious spread-out reflections, subtle modulation, gentle high roll-off
  - **Spring:** 20ms pre-delay, all-pass dispersion chain for metallic chirp, 4.5Hz flutter modulation, resonant mid peak at 800Hz
  - **Plate:** Short 8ms pre-delay, dense early reflections, ring modulation shimmer effect, bright high shelf at 5kHz
  - **Ambient:** 35ms pre-delay, maximum diffusion, slow 0.4Hz dreamy modulation, softened highs for washy ethereal sound

- **Pre-delay lines:** Type-specific pre-delay times (3-50ms) create distinct spatial separation
- **Early reflection network:** 4 comb filters per channel with prime-number delay times, scaled per type
- **All-pass dispersion:** 3-stage all-pass chain for Spring's characteristic metallic chirp
- **Modulation LFO:** Configurable rate/depth for Spring flutter and Ambient movement
- **Plate shimmer:** Subtle ring modulation effect for Plate's bright shimmering character
- **Type-specific EQ:** Each type has tailored frequency shaping (high-pass, shelves, peak filters)

### Fixed

- **Root cause:** All six reverb types previously used identical Freeverb algorithm with only subtle parameter differences. Types are now dramatically different through dedicated DSP processing chains.

### Technical Notes

- Sample-by-sample processing for pre-delay, early reflections, all-pass, and modulation
- Block processing for main reverb, type EQ, and character filter
- Tail length updated to 10.0 seconds for proper DAW handling
- No breaking changes - all parameters remain compatible with v1.0.x presets

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
