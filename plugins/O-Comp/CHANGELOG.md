# O-Comp Changelog

All notable changes to this plugin will be documented in this file.

## [1.6.0] - 2026-08-29

The page speaks French. Stage K batch K3 of the repo-wide i18n rollout, canon v2.

### Added

- **A French UI, selectable from a gear popover at the top of the frame.** 22 label
  strings, 7 accessible names and the 3 canvas-painted captions. `Source/ui/public/js/i18n.js`
  holds both languages; the canon v2 runtime block in `index.html` is byte-identical to
  `scripts/i18n-canon.js` and gated by `check-i18n` assertion 6.
- **The language choice persists with the session.** `getUiLanguage` / `setUiLanguage`
  native functions, and a non-parameter `uiLanguage` property on the APVTS state tree —
  read back with `isVoid()`, never `isBool()`, because the XML round-trip rebuilds every
  property as a string `var` (`critical_valuetree_xml_roundtrip_loses_type`). Deliberately
  not an `AudioParameterChoice`: it must not appear in an automation lane, and a preset
  must not be able to change which language somebody reads their plugin in.
- **The three strings painted into `#envelopeCanvas` are localized too** — `Envelope`,
  `Gain Reduction` and the live `GR:` readout. A canvas string has no element, so it is
  invisible to both gates: `check-i18n` walks text nodes and `textContent` writes and
  reaches `fillText` through neither. They are housed in `I18N` with an empty body (the
  O-Polystutter shape for a homeless string) and read through `trLabel()` inside the render
  loop, so they follow the selector on the next animation frame.

### Changed

- **`.preset-action-btn` now declares `padding: 0`.** These are `<button>`s with no padding
  declared, so they inherited the UA default `1px 6px`; under the universal
  `box-sizing: border-box` that left an **18px content box inside a 32px button**, and the
  English caption "Load" renders 18.5px of text. The shipped v1.5.0 button was already over
  its own content box in English. Nothing moves: border-box with a pinned width, and the
  caption is centred by `justify-content` rather than by the padding.
- The five native `title=` attributes on the preset bar are **deleted**, per the i18n
  contract §4 — a native title renders a second, untranslated OS tooltip. Their text moved
  to `data-i18n-aria`; no new prose was invented. Label-in-name (WCAG 2.5.3) now holds in
  both languages: "Ouvrir" inside "Ouvrir un préréglage", "Sauver" inside "Sauver un
  préréglage".
- The console banner said `v1.4.0` on a v1.5.0 build. It now says v1.6.0.

### Notes

- **All French is a machine draft; every entry is flagged `reviewed: false`.** No native
  speaker has read it. `label.release` in particular ships as "Relâche" rather than the
  fuller "Relâchement", because the latter measures 62.92px into a 52px knob column and
  slides that knob 5.45px out of line with the other five — proven by reverting to it and
  watching `check-ui-labels` assertion 7 name the knob, the vine arc and the value readout.
- Geometry: **zero existing elements moved** between v1.5.0 English and v1.6.0 English, and
  **zero non-label elements moved** between English and French. French SHRANK on three of
  the seven knob captions.

## [1.5.0] - 2026-07-01

Bundled resolution of code-review findings CR-01 and WR-01/02/03 (see `.planning/REVIEW.md`).

### Fixed

- **CR-01 (critical): Divide-by-zero → NaN in the soft-knee gain formula when `knee == 0`.**
  `calculateGainReduction()` divided by `2 * kneeDB` in the inside-knee branch. With a
  zero/near-zero knee — including the shipped **"Parallel Crush"** factory preset — an
  envelope landing exactly on the threshold evaluated `0/0` → NaN, which flowed into the
  output buffer (audible click at best, latched-NaN channel at worst). Root cause: the two
  guard branches (`x < 0`, `x > 0`) leave the else branch reachable at `x == 0` when the
  knee is zero. Fix: early-return a hard-knee result when `kneeDB <= 1e-6f`.
- **WR-01: Latent out-of-bounds / null-pointer deref on >2-channel layouts.** The
  channel-pointer array is sized 2, but the detection and gain-apply loops iterated the raw
  `getNumChannels()` with no cap. Fix: cap the working channel count to
  `jmin(getNumChannels(), 2)` and add an `isBusesLayoutSupported()` override that accepts
  only mono/stereo main I/O (input must match output), so a wider layout can never be
  negotiated.

### Changed

- **WR-02: Makeup/output gain is now smoothed to remove zipper noise on automation.**
  Auto-gain + `output_gain` were computed once per block and hard-multiplied per sample, so
  automating `output_gain` / toggling `auto_gain` / sweeping `threshold`/`ratio` stepped the
  gain discontinuously at block boundaries. Now wrapped in a `juce::SmoothedValue` (20 ms
  ramp), initialized in `prepareToPlay()` and advanced per sample. Shared computation
  factored into `computeMakeupGainLinear()`.
- **WR-03: Preset Prev/Next no longer jumps to the alphabetically-first preset after loading
  an imported (or deleted) preset that isn't in the Factory/User list.** When the current
  name resolves to index -1, `getPreviousPreset()` now returns the last entry (treating the
  unknown preset as "before the list") instead of the first, matching `getNextPreset()`'s
  wrap semantics. Note: fix applied to O-Comp's local copy of the shared `preset-manager`
  module — the module master carries the same latent behavior and can be updated suite-wide
  via `/modules` separately.

### Testing

- Release build + `pluginval` (strictness 8) via build-and-install; installed to system
  VST3/AU with cache clear and dual-variant sweep. Verified AU registers via `auval`.

## [1.4.3] - 2026-03-06

### Fixed

- Fixed auto-gain overcompensation making output significantly louder than input - applied 50% scaling factor to theoretical makeup gain formula (industry standard approach)

## [1.4.2] - 2026-02-13

### Fixed

- Fixed license overlay blink on plugin open - overlay now uses `addChildComponent` (hidden by default) instead of `addAndMakeVisible`, preventing brief flash before license check completes

## [1.4.1] - 2026-02-07

### Changed

- **Replaced seed-knob dials with SVG vine-arc knobs** matching O-Detune's style
  - Knobs now render as animated SVG circular arcs instead of CSS radial-gradient circles
  - Smooth `requestAnimationFrame` interpolation for vine fill animation
  - Added mouse wheel support for fine control
  - Knob size increased from 40px to 52px for better visibility
  - Colors: walnut track (`rgba(139,115,85,0.3)`) with accent green vine (`#5a7a6a`)

## [1.3.0] - 2026-02-07

### Added

- **8 Factory presets** initialized on startup via `initializeFactoryPresets()`:
  - Gentle Glue - subtle bus compression with soft knee
  - Vocal Smooth - medium vocal compression with auto-gain
  - Drum Punch - punchy drums with fast release
  - Bass Control - tight bass control with moderate ratio
  - Mastering Touch - light mastering-style compression
  - Aggressive Smash - heavy limiting-style compression
  - Natural Dynamics - transparent compression for natural sources
  - Parallel Crush - heavy compression for parallel processing (no auto-gain, +12 dB output)

### Technical Details

- Factory presets written to `~/Library/O-Comp/Presets/Factory/` as JSON files
- All presets use auto-gain except Parallel Crush (designed for parallel mix blending)
- Preset values stored as normalized (0.0-1.0) per APVTS convention

## [1.2.0] - 2026-01-24

### Changed

- **Renamed plugin** from "OuariconComp" to "O-Comp"
  - Short name "O-Comp" now appears in DAW, file system, and plugin identifiers
  - Full name "Ouaricon Compressor" remains in the UI header
  - Source folder renamed from `plugins/OuariconComp/` to `plugins/O-Comp/`
  - Preset folder changed from `~/Library/OuariconComp/Presets/` to `~/Library/O-Comp/Presets/`

### Migration Notes

- Existing presets need to be moved manually from `~/Library/OuariconComp/Presets/` to `~/Library/O-Comp/Presets/`
- DAW sessions using "OuariconComp" will need to re-add the plugin as "O-Comp"

## [1.1.1] - 2026-01-12

### Added

- **Preset dropdown menu**: Click on preset name to show dropdown list of all available presets
- Factory presets appear in dropdown with visual highlight for current preset
- Dropdown closes automatically when clicking outside or selecting a preset

## [1.1.0] - 2026-01-12

### Added

- **Preset Manager integration**: Full preset save/load functionality using the Ouaricon preset module
  - Previous/Next navigation buttons for browsing presets
  - Save button opens native file dialog to save user presets
  - Load button opens native file dialog to import presets
  - Current preset name displayed in header bar

- **8 Factory presets** covering common compression use cases:
  - Gentle Glue - subtle bus compression with soft knee
  - Vocal Smooth - medium vocal compression with auto-gain
  - Drum Punch - punchy drums with fast release
  - Bass Control - tight bass control with moderate ratio
  - Mastering Touch - light mastering-style compression
  - Aggressive Smash - heavy limiting-style compression
  - Natural Dynamics - transparent compression for natural sources
  - Parallel Crush - heavy compression for parallel processing

### Changed

- **UI layout updated**: Title shifted left, preset bar positioned alongside to the right
- Header row now uses flexbox for proper alignment between title and preset controls

### Technical Details

- Integrated `OuariconPresetManager` module for preset persistence
- Added 10 native WebView functions for preset operations
- Updated `parentHierarchyChanged()` pattern for safe WebView navigation
- Presets stored in `~/Library/OuariconComp/Presets/` (Factory and User subdirectories)
- State information now includes current preset name for session recall

## [1.0.2] - 2026-01-11

### Changed

- Default ratio changed from 4:1 to 2:1 for gentler compression out of the box
- Double-click on any knob now resets to its correct default value (not just 50%)

## [1.0.1] - 2026-01-11

### Fixed

- **Attack, Release, Output knobs not responding**: Fixed JavaScript ID mapping in WebView UI. The `updateKnob` function was incorrectly converting parameter IDs (e.g., `attack_time` → `attack-time-indicator`) when the actual element IDs used a different pattern (`attack-indicator`). Added explicit parameter-to-element ID mapping.

- **Envelope and Gain Reduction display showing fake animation**: Replaced placeholder animation (using `Math.sin()` and `Date.now()`) with real-time metering data from the DSP. The envelope display now shows actual signal envelope history as a scrolling waveform, and gain reduction is displayed as a histogram with current GR value.

- **Input/Output meters showing random animation**: Replaced `Math.random()` placeholder animation with actual input and output level monitoring. Added atomic metering variables to the processor, timer-based polling in the editor, and JavaScript-C++ bridge via `evaluateJavascript`.

### Technical Details

- Added atomic metering variables to `PluginProcessor`: `inputLevelDB`, `outputLevelDB`, `currentGainReductionDB`, `currentEnvelopeDB`
- Added 30Hz timer in `PluginEditor` to push meter data to WebView
- Added `updateMeters()` JavaScript function callable from C++
- Added rolling history buffers (300 samples) for envelope and GR visualization

## [1.0.0] - 2026-01-11

### Added

- Initial release
- WebView UI with Ouaricon Naturalist aesthetic
- Parameters: Threshold, Ratio, Attack, Release, Knee, Output Gain, Auto-Gain
- Soft-knee compression algorithm with accurate envelope following
- Transfer curve visualization
- Stereo-linked detection
