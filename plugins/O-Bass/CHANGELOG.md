# O-Bass Changelog

## [1.3.3] - 2026-07-08

### Fixed
Resolves the Critical + Warning findings from the v1.3.2 deep code review (`CODE_REVIEW.md`).

- **CR-01 — FileChooser use-after-free on editor teardown:** the async Save-preset and
  Load-preset `launchAsync` completions captured raw `this` with no lifetime guard, so
  destroying the editor while a native dialog was open (close window, switch track, remove
  plugin) fired the completion against a freed editor. Both completions are now
  `juce::Component::SafePointer`-guarded and bail with a **bare `return`** on teardown —
  they must *not* call `complete()`, which is owned by the already-dead WebView Impl (calling
  it is itself a UAF). Matches the codebase `pattern_webview_launchasync_safepointer_no_complete`.
- **WR-01 — factory `crossover_freq` ignored the 0.5 skew:** the factory table authored
  `crossover_freq` as plain linear fractions, but that value is applied as a *normalised*
  APVTS value through a skewed (0.5) 40–200 Hz range, so the whole table compressed into the
  bottom quarter (e.g. "Default" landed at 50 Hz instead of the plugin's true 80 Hz). The
  table is now authored in engineering units (Hz) and converted per-param via the real
  `NormalisableRange::convertTo0to1`, so "Default" reproduces the plugin default and every
  preset lands at its intended frequency. Matches `pattern_factory_preset_normalized_ignores_skew`.
- **WR-02 — `latency_mode` (Mode) was inert during playback:** Mode was read only in
  `prepareToPlay`, so toggling Low Latency ↔ High Fidelity via automation/generic editor had
  no effect until the host re-prepared the plugin. `processBlock` now reads `latency_mode` and,
  on change, performs the RT-safe atomic `setMode()` flip on both the crossover and clean-mode
  processors (cached so it fires only on change). Known limitation retained: in High Fidelity
  mode the FIR *tap* reload for a crossover-frequency change is still deferred to the next
  `prepareToPlay()`.
- **WR-03 — `applyPresetJson` didn't reset omitted parameters:** loading a preset only wrote
  the keys present in the JSON, so a factory preset (which stores just 3 of 5 params) inherited
  stale live state — most visibly, loading a preset while Bypass was on left the plugin silently
  bypassed. `applyPresetJson` now resets every parameter to its default before applying the
  preset's values (inlined from shared `preset-manager` v1.0.3). Matches
  `pattern_preset_apply_needs_reset_to_defaults`.

## [1.3.2] - 2026-07-01

### Fixed
- Preset-manager module sync (`preset-manager` v1.0.2) — fixes from the O-DigiDelay code review:
  - **WR-04:** preset names are sanitized before use as filenames (`/\\:` → `_`) in save/load/delete/isFactory, so a name containing `/` no longer silently drops the file.
  - **IN-02:** preset JSON records the real plugin version instead of a hard-coded `"1.0.0"`.
  - **IN-03:** prev/next resume from the last in-list position instead of snapping to index 0 after loading an out-of-list preset from file.
  - **IN-01:** corrected the preset-path docstring.
- Set `VERSION 1.3.2` in CMake (was previously unset, so `JucePlugin_VersionString` reported `1.0.0`).

## [1.3.1] - 2026-01-28

### Added
- **VU Meter**: Horizontal output level meter at bottom center with smooth ballistics
- Timer-based metering from C++ (30fps, same reliable pattern as O-Comp)

### Changed
- **Improved layout**: Title on top row, preset bar below, 3 knobs in single row, meter at bottom
- Window size refined to 420×320 pixels

## [1.3.0] - 2026-01-28

### Changed
- **Compact UI**: Reduced window size from 500×450 to 400×350 pixels (better fit for 3-knob layout)
- **Display title**: Changed in-plugin title from "O-Bass" to "Ouaricon Bass"

## [1.2.1] - 2026-01-28

### Changed
- **More dramatic harmonic enhancement**: Increased DSP coefficients for noticeably stronger bass effect
  - `kInputDrive`: 2.0 → 4.0 (more saturation into waveshaper)
  - `kH2Weight`: 0.5 → 0.8 (stronger 2nd harmonic warmth)
  - `kH3Weight`: 0.3 → 0.5 (more 3rd harmonic presence)
  - `kHarmonicMix`: 0.7 → 1.2 (louder harmonic content in mix)
- Root cause: Previous coefficients were too conservative, effect was barely audible at max enhance

## [1.2.0] - 2026-01-28

### Changed
- Code quality cleanup: removed unused code, extracted magic numbers to constants, added documentation

## [1.1.1] - 2026-01-27

### Performance
- **IIR coefficient updates optimized**: Filter coefficients now update every 16 samples during parameter smoothing instead of per-sample. Reduces CPU overhead during crossover frequency automation.
- **Buffer resize checks removed**: Replaced runtime buffer size checks in processBlock() with debug-only jassert assertions. Buffers are pre-allocated in prepareToPlay() making runtime checks unnecessary overhead.

## [1.1.0] - 2026-01-27

### Removed
- **Colored Mode**: Removed entirely (parameter, UI toggle, DSP processor)
  - ColoredModeProcessor.h/.cpp deleted
  - `enhanceMode` parameter removed
  - Mode toggle removed from WebView UI
  - Factory presets simplified (no more mode-specific presets)

### Fixed
- **Dead code removed**: ~80 lines of unreachable code after early `return` in processBlock()
- **Limit indicator now works**: Output gain stage with soft clipping restored
- **Buffer validation**: Added channel check in HarmonicGenerator::process()

### Changed
- Plugin now has a single, clean processing path
- Factory presets renamed ("Aggressive Colored" -> "Maximum Enhancement")

## [1.0.2] - 2026-01-27

- Renamed from OBass to O-Bass

## [1.0.1] - 2026-01-26

- Added buffer size validation in CleanModeProcessor
- Disabled Colored mode pending further testing

## [1.0.0] - 2026-01-26

- Initial release
- Crossover filtering with LR4 topology
- Dual mode: Clean (transparent) and Colored (warm) enhancement
- WebView UI with botanical aesthetic
