# O-DigiDelay Changelog

All notable changes to this plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2026-08-29

### Added

- **The page speaks French.** Every visible caption on the UI is now keyed and
  carries an English and a French string: the six knob captions, the sync
  caption and both of its faces, the output-meter caption, the two preset
  buttons, the preset dropdown's two written strings and the new language
  caption. A settings gear in the bottom-left corner opens a popover holding the
  language selector, and the choice is persisted with the session as a
  non-parameter property on the APVTS state tree (`uiLanguage`, read back
  through an `isVoid()` guard — the XML round-trip rebuilds every property as a
  `var` over the attribute STRING, so `isBool()`/`isInt()` would be false for
  every saved session).
- New `Source/ui/public/js/i18n.js`, embedded in `juce_add_binary_data` SOURCES
  **and** served from a `getResource()` branch, in the same change. A file
  embedded but not served — or served but not embedded — is a 404 that presents
  as a page stuck in English and nothing else.
- Two native functions, `getUiLanguage` / `setUiLanguage`. The page PULLS once
  at init; nothing is pushed from the editor constructor, which would race the
  WebView's load.
- `tests/i18n-states.json`, so the label gate also measures the two states whose
  captions are written by JavaScript — the settings popover and the preset
  dropdown.

### Fixed

- **The output-meter caption overflowed its own box in ENGLISH, and had since
  v1.0.0.** `.led-meter-label` was `width: 18px`, matching the 18 px meter under
  it, but the word "OUT" renders 20.91 px in this face — 2.91 px painted outside
  its content box. The label and its container are widened to 38 px and the
  container moved left 10 px, so the meter itself stays on the identical
  absolute x = 645 while the caption finally fits. Keying the node is what
  exposed it: no French string could have avoided it either, since SORTIE
  (35.77), SORT. (28.05) and even SOR (19.92) are all wider than 18 px.

### Changed

- **The five native `title=` attributes on the preset bar are DELETED, not
  localized.** A native `title` renders a second, untranslated OS tooltip. Their
  existing English text moved verbatim into `data-i18n-aria` accessible names —
  no hover-help prose was invented, and this plugin still has none.
- **`.preset-action-btn` is pinned to 62 px.** The preset bar is a shrink-to-fit
  flex row flush against the header's right edge, so an unpinned button that
  grows in French drags the arrows and the preset name with it. The pin makes
  the row's geometry language-invariant; it widens the two buttons in English by
  21.00 and 23.66 px, moving the preset cluster 44.66 px left inside a header
  that has 115.47 px of empty middle. Nothing else on the page moved.

### Not changed

- No parameter IDs, ranges, types, defaults or DSP behaviour. All French is
  machine-drafted and flagged `reviewed: false`; no native speaker has read it.

## [1.2.12] - 2026-08-02

### Fixed
- WebView/editor robustness fixes from code review (findings WR-05, WR-07)
  - **WR-05:** The Windows WebView2 user-data folder is now plugin-specific (`tempDirectory/O-DigiDelay_WebView`) instead of the bare temp root shared by every Ouaricon plugin. WebView2 places a lock on its user-data folder, so pointing every plugin/instance at the same directory could cause lock contention on Windows — a failed WebView2 construction silently falls back to IE (blank UI, no error). Matches the project-wide convention.
  - **WR-07:** Renamed the butterfly overlay asset `butterfly2_Black and white.png` → `butterfly2_bw.png` (space-free). The resource provider does an exact-string match on the request path; a filename with spaces breaks if the WebView engine percent-encodes them to `%20` (`getResource` returns 404 → the butterfly overlay and feedback echoes silently disappear). Updated the CSS references (`index.html`), the `getResource` match + `BinaryData` symbol (`PluginEditor.cpp`), and the `juce_add_binary_data` source entry (`CMakeLists.txt`). Butterfly overlay verified rendering on macOS after rebuild.

## [1.2.11] - 2026-07-01

### Fixed
- Preset-system fixes from code review (findings WR-03, WR-04, IN-02, IN-03, IN-01)
  - **WR-03:** The "Save preset" dialog no longer implies a destination it ignores. It was a folder-navigable `FileChooser`, but the chosen folder was discarded — the preset always went to the managed User folder while the UI reported success with the name. Replaced with a name-only prompt, since presets must live in the managed folder to appear in the list and navigate with prev/next.
  - **WR-04:** Preset names are now sanitized before use as filenames (`replaceCharacters("/\\:", "___")`), applied consistently in `savePreset`/`loadPreset`/`deletePreset`/`isFactoryPreset` so the same name round-trips. Previously a name containing `/` (e.g. "Koto / Harp") was interpreted as a path by `getChildFile()` and the file was silently dropped. (Shared-module regression — see propagation note.)
  - **IN-02:** Saved preset JSON now records the real plugin version (`JucePlugin_VersionString`) instead of a hard-coded `"1.0.0"`, for both user and factory presets — restores future version-migration capability.
  - **IN-03:** `getNextPreset`/`getPreviousPreset` no longer snap to index 0 after loading an out-of-list preset from a file. They now resume from the last in-list position (`lastListIndex`).
  - **IN-01:** Corrected the `OuariconPresetManager` docstring to match the actual preset path (`~/Library/{pluginName}/Presets/`). The path itself was left unchanged to avoid orphaning existing user presets; switching to Application Support would require a one-time migration.

## [1.2.10] - 2026-07-01

### Fixed
- DSP robustness hardening from code review (findings WR-01, WR-02, WR-06, IN-04)
  - **WR-02:** NaN/Inf can no longer poison the delay line permanently. `ScopedNoDenormals` does not catch NaN/Inf, so a single non-finite upstream sample used to recirculate through the feedback path forever. Recirculated feedback is now sanitized each sample (`if (!std::isfinite(x)) x = 0.f`) for both channels.
  - **WR-01:** Delay buffer now has genuine modulation headroom. It was sized `2000ms + 25ms` but spread (15ms) + mod (10ms) consumed that 25ms exactly — zero margin at 48/96/192 kHz. Added a 5ms safety pad so the max read index never reaches `maximumDelayInSamples`.
  - **WR-06:** Fixed cross-thread data race on the output meter. RMS levels (`LinearSmoothedValue`, not thread-safe) were written on the audio thread and read on the message thread by the 30 Hz editor timer. Meter values are now published through `std::atomic<float>` (relaxed) written at the end of `processBlock` and read in the getters.
  - **IN-04:** Added `isBusesLayoutSupported` — accepts mono→mono and stereo→stereo, rejects mismatched/other layouts.

## [1.2.9] - 2026-02-14

### Fixed
- Black screen on CI/release builds (overlay never displayed)
  - Root cause: `setSize()` called before license overlay was created, so `resized()` never sized the overlay — it had zero bounds (0,0,0,0) and was invisible
  - Fix: moved `setSize()` to end of constructor (after overlay creation), matching O-Tremolo pattern
  - Now the activation dialog properly displays on first launch of release builds

## [1.2.8] - 2026-02-14

### Fixed
- Pitch black UI on CI/release builds (both macOS and Windows)
  - Root cause: licensing starts in `Checking` status, hiding the WebView before page loads
  - WebView2 and WKWebView do not load content when the component is hidden
  - Fix: re-navigate WebView after license validation makes it visible (matches O-Tremolo fix 620f185)

## [1.2.7] - 2026-02-14

### Added
- Version branding footer in UI showing "Ouaricon Audio v1.2.7"
- `getPluginVersion` native function exposing version string to WebView

## [1.2.6] - 2026-02-09

### Fixed
- Licensing module fix

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
