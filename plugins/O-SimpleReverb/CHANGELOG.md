# Changelog

All notable changes to O-SimpleReverb (formerly OuariconSimpleReverb) will be documented in this file.

## [1.5.1] - 2026-02-04

### Fixed

- **Real-time safety:** Pre-allocate dry/wet buffers as persistent members instead of allocating on the audio thread every processBlock call
- **Dead code removed:** Unused `allPassStateL/R` arrays (declared but never read in all-pass processing), unused `modDelaySamples` variable
- **Cached parameter pointers:** `getRawParameterValue()` results cached in constructor instead of looked up every processBlock
- **Cleaner initialization:** `shimmerFreq` initialized at declaration to `kDefaultShimmerFreq`, destructor defaulted

## [1.5.0] - 2026-01-24

### Changed

- **Plugin renamed:** OuariconSimpleReverb → O-SimpleReverb
- **Display name:** Now shows "O-SimpleReverb" in DAWs and plugin lists
- **Preset location:** Migrated from `~/Library/Ouaricon Simple Reverb/` to `~/Library/O-SimpleReverb/`
- **UI header:** Updated title display

### Technical Notes

- Folder renamed from `plugins/OuariconSimpleReverb` to `plugins/O-SimpleReverb`
- CMake target renamed to O-SimpleReverb
- Class names updated: OSimpleReverbAudioProcessor, OSimpleReverbAudioProcessorEditor
- Plugin code (OuSr) preserved for parameter compatibility
- Existing presets will be migrated automatically

### Migration Notes

- DAW sessions using the old plugin name will need to reload the plugin
- User presets will be migrated from old location to new location

## [1.4.1] - 2026-01-13

### Added

- **Save button:** Opens system save dialog to save current settings as a user preset
- **Load button:** Opens system file picker to load a preset from any location

### Changed

- **Title:** Restored "Ouaricon Simple Reverb" with line break after "Ouaricon"

### Technical Notes

- Added `savePresetWithDialog` and `loadPresetFromFile` native functions
- Uses JUCE FileChooser for async file dialogs
- Save targets User presets directory, Load can browse anywhere

## [1.4.0] - 2026-01-13

### Added

- **Preset system:** Integrated OuariconPresetManager module for preset persistence
- **24 factory presets:** 4 presets per reverb type (Booth, Room, Hall, Spring, Plate, Ambient)
  - **Booth:** Vocal Booth, Drum Close, Tight Room, Whisper
  - **Room:** Small Room, Live Room, Studio A, Jazz Club
  - **Hall:** Concert Hall, Cathedral, Theater, Ballroom
  - **Spring:** Vintage Spring, Surf Guitar, Dub Echo, Twang
  - **Plate:** Studio Plate, Shimmer Plate, Vocal Plate, Lush Plate
  - **Ambient:** Pad Wash, Infinite Drone, Ethereal, Cloud Nine
- **Preset navigation:** Previous/Next buttons for quick preset browsing
- **Preset dropdown menu:** Click preset name to see categorized list of all presets

### Changed

- **Header layout:** Title shifted left, preset bar positioned on the right
- **Title:** Shortened from "Ouaricon Simple Reverb" to "Simple Reverb" for space

### Technical Notes

- Uses Ouaricon Module System (`ouaricon_add_module`)
- Presets stored in `~/Library/Ouaricon Simple Reverb/Presets/`
- Factory presets in `Factory/` subdirectory, user presets in `User/`
- Native functions registered: savePreset, loadPreset, getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset, deletePreset, isFactoryPreset
- No parameter changes - fully compatible with v1.3.x sessions

## [1.3.2] - 2026-01-13

### Changed

- **Decay dial:** Now properly centered at 1.0x (0.5x left, 1.0x center, 2.0x right)
- **Low Cut dial:** Added Hz indicators (20 / 400) on left and right sides
- **Footer:** Removed "Ouaricon Audio | v1.3.x" label for cleaner look

## [1.3.1] - 2026-01-13

### Changed

- **Low Cut filter:** Renamed from "LP Filter" to "Low Cut" - now a high-pass filter (cuts low frequencies from reverb)
- **Filter toggle:** Changed from double-click on knob to clickable OFF/ON button below the dial
- **Decay control:** Now a multiplier (0.5x to 2.0x) that scales reverb tail length relative to each type's base decay
  - 0.5x = shorter, tighter reverb
  - 1.0x = default (unchanged)
  - 2.0x = longer, more sustained reverb

### Fixed

- **Decay not affecting reverb:** Decay now properly scales both room size and damping for audible difference

## [1.3.0] - 2026-01-13

### Added

- **Lowpass filter:** New LP Filter control (20-400Hz) with on/off toggle applied to wet signal
  - Double-click knob to toggle on/off
  - Dragging automatically enables the filter
  - Useful for creating darker, more muffled reverb tails
- **VU meter dB indicators:** Added tick marks and labels at -60, -30, -12, -6, and 0 dB
- **VU meter color gradient:** Needle now transitions from green (quiet) through yellow to red (loud)

### Changed

- **VU meter rotation range:** Expanded from ±45° to ±90° (180° total sweep) for better visual feedback
- **Flora background:** Increased botanical overlay size from 75% to 150% height (2x larger)
- **Top row layout:** Now 4 columns to accommodate LP Filter dial (Character, LP Filter, Wet, Dry)

### Technical Notes

- New parameters: LPFREQ (20-400Hz), LPON (toggle)
- Lowpass filter uses IIR coefficients, updated only when frequency changes
- VU meter color calculated per-frame based on normalized position

## [1.2.1] - 2026-01-13

### Changed

- **Dial alignment:** Decay now aligned beneath Character (left column), Size beneath Dry (right column)
- **VU meter size:** Enlarged from 50px to 75px diameter for better visibility
- **Layout:** Bottom row now uses 3-column grid matching top row for consistent alignment

### Fixed

- **VU meter functionality:** Connected VU meter to audio output level via C++ backend timer (30Hz updates)
- **Needle size:** Increased needle height to 30px to match larger meter

### Technical Notes

- Added `outputLevelDB` atomic in PluginProcessor for thread-safe level metering
- Added Timer inheritance to PluginEditor, emitting "outputLevel" events at 30Hz
- VU meter responds to actual audio signal with attack/decay smoothing

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
