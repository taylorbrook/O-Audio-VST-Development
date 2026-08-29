# Changelog

All notable changes to O-SimpleReverb (formerly OuariconSimpleReverb) will be documented in this file.

## [1.6.0] - 2026-08-29

### Added

- **The page speaks French.** Nineteen keys in a new `Source/ui/public/js/i18n.js` — eleven visible captions and eight accessible names — plus a settings popover in the bottom-left page margin carrying the language selector, a `getUiLanguage` / `setUiLanguage` native-function pair, and session persistence through a non-parameter `uiLanguage` property on the APVTS state tree. All French is machine-drafted and flagged `reviewed: false`; no native speaker has read it.
- **No hover-help was authored.** v1.5.7 carried no `data-tip` anywhere, so `I18N` and `TIP_BINDINGS` are both empty — this plugin's correct state, which `check-i18n` assertion 2 reports as "0 tip(s) bound". Tooltip copy is a later stage's job.

### Changed

- **Four native `title=` attributes DELETED, not localized.** `Previous preset`, `Next preset`, `Save preset` and `Load preset` moved verbatim into `data-i18n-aria` accessible names; a native `title` renders a second, untranslated OS tooltip. The plugin's own wording was kept rather than harmonised with its siblings'. `#preset-display` gets no accessible name, because v1.5.7 gave it no `title` and inventing one is not this change's job.
- **Both `alt` attributes keyed** through `data-i18n-alt` rather than emptied.
- **The low-cut ON/OFF toggle is now a localized label.** `#LPFREQ-value` carries the readout class but never holds a number and is a clickable control, so D-01 arm 3 was overruled with reasons recorded in `js/i18n.js`. It reads `ACT.` / `DÉS.` in French; `MARCHE` was rejected on measurement (it would move the whole low-cut knob 4.40 px).
- **Three geometry pins, all load-bearing.** `#preset-save` 41 px and `#preset-load` 44 px (the header is `space-between` with 163.58 + 290.42 = 454.00 px of content and therefore ZERO slack; the French captions SHRINK and would re-centre the whole bar), and `#CHARACTER-knob > .knob-label` 62 px (the only caption on the page past the 52 px knob floor in English). Each was removed alone and re-broke the gate. `.settings-label { white-space: nowrap }` ships as a declared GUARD, not a pin: its negative control passes.

### Geometry

- English v1.5.7 → v1.6.0: **zero of 96 elements moved** at the 0.5 px gate tolerance; at 0.01 px, ten moved by at most 0.39 px, every one attributable to a named pin, and nothing moved vertically. Document scroll extent unchanged at 500 x 350.
- French vs English at v1.6.0: **zero non-label elements moved** — in the default state, with the settings popover open, and with the preset dropdown open. The only boxes that differ are the five keyed captions that changed width.
- **No layout change was needed and French caused nothing.** Six of eleven French strings are the same width or narrower than their English.

### Known

- The footer wordmark is a hard-coded `Ouaricon Audio v1.5.5` and is STALE against this version. Deliberately not repaired here: it is a user-visible change unrelated to localization, and the right fix is the runtime-filled `id="versionLabel"` span two sibling plugins already use.

## [1.5.7] - 2026-08-02

### Changed

- **License headers:** Added AGPL-3.0 notice headers to all Ouaricon-authored source files (repo-wide licensing sweep). No functional changes.

## [1.5.6] - 2026-07-07

Resolves all Critical and Warning findings from the 2026-07-05 code review (CODE_REVIEW.md).

### Fixed

- **CR-01 — FileChooser use-after-free:** Both `savePresetWithDialog` and `loadPresetFromFile` completions captured raw `this` and the WebView-owned `complete` callback; closing the plugin window while the dialog was open could crash the host. Completions now capture a `Component::SafePointer` and bail with a bare return after editor teardown (calling `complete()` there would itself be a UAF — same fix class as O-MicrotonalSampler v1.23.5 W12).
- **CR-02 — Inverted DECAY skew:** Skew was 1.585 (the reciprocal of the intended value), putting 1.47x at knob center while the UI readout and all 24 factory presets were authored against the intended 0.6309 curve. Skew corrected to 0.6309: the plugin UI, the DAW generic parameter view, and the factory presets now all agree, and "short decay" presets are actually short. **Note:** saved sessions are unaffected (normalized values reinterpret), but user presets saved under ≤ v1.5.5 will load with a different — now correct — audible decay.
- **CR-03 — Audio-thread heap allocation (filter coefficients):** All `IIR::Coefficients::makeXXX` calls reachable from `processBlock` (character warm/bright, low-cut frequency, type-EQ on type change) replaced with stack-based `ArrayCoefficients` assignments (identical math, no allocation). `prepareToPlay` primes each filter state through the same path so audio-thread assigns never reallocate.
- **CR-04 — First-callback buffer allocation:** `dryBuffer`/`wetBuffer` are now pre-allocated in `prepareToPlay`; the `setSize` calls in `processBlock` are no-op reuse instead of a guaranteed allocation on the first audio callback (and on every block-size increase in variable-block hosts).
- **WR-01 — Preset stale-state inheritance (preset-manager v1.0.3):** `applyPresetJson` resets every parameter to its default before applying, so presets missing a key (e.g. saves from builds predating LPFREQ/LPON) no longer inherit whatever LP state was live.
- **WR-02 — CHARACTER readout mismatch:** The UI labeled the middle half of the range "neutral" while the DSP engages filtering outside ±0.5. Readout now mirrors the DSP threshold and shows a continuous amount ("warm 62%", "bright 30%", "neutral" only in the true dead zone).
- **WR-03 — Wet/dry zipper noise:** Wet and Dry gains were raw per-block values; knob drags and automation stepped at block rate. Both gains now run through 20ms `SmoothedValue` ramps applied per-sample in the mix loop.
- **WR-04 — Factory presets rewritten on every instantiation (preset-manager v1.0.3):** `initializeFactoryPresets` now checks a version-stamped `.factory-version` sentinel and only rewrites the 24 factory `.json` files when the plugin version changes — removing 24 synchronous message-thread file writes per instance/scan pass and the concurrent-construction file race.
- **WR-05 — Unconstrained bus layouts:** Added `isBusesLayoutSupported` limiting the plugin to matched mono/stereo. Previously surround hosts could negotiate 4/6-channel layouts on which channels ≥ 2 silently received dry-only output.

### Testing

- Built Release (VST3 + AU + Standalone), auval validation, installed to system folders.

## [1.5.5] - 2026-02-15

### Added

- **Version footer:** Added "Ouaricon Audio" version label in the plugin UI footer

## [1.5.4] - 2026-02-08

### Changed

- **VU meter size:** Enlarged from 80px to 100px diameter (1.25x scale) for better readability. Needle, pivot, arc, tick marks, and dB labels all scaled proportionally.
- **VU meter dB labels:** Repositioned inward along the arc and increased font from 7px to 8px for legibility.

## [1.5.3] - 2026-02-08

### Changed

- **VU meter size:** Enlarged from 80px to 100px diameter (1.25x scale) for better readability. Needle, pivot, arc, tick marks, and dB labels all scaled proportionally.
- **VU meter dB labels:** Repositioned inward along the arc and increased font from 7px to 8px for legibility.

## [1.5.2] - 2026-02-07

### Fixed

- **VU meter dB label accuracy:** Tick marks and dB labels (-60, -30, -12, -6, 0) repositioned to mathematically correct angles on the arc for the linear dB scale. Previous positions were hand-placed for visual spacing and were up to 61° off from actual dB values.

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
