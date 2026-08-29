# O-Bass Changelog

## [1.4.0] - 2026-08-29

### Added
**The page speaks French.** Stage K batch K2, canon v2. Nineteen keys in a new
`Source/ui/public/js/i18n.js` — twelve visible captions and seven accessible names — a
settings popover in the empty bottom-left margin carrying the language selector, a
`getUiLanguage` / `setUiLanguage` native-function pair, and session persistence through the
APVTS state tree.

- **Localized captions:** the three knob labels (FREQUENCY / ENHANCE / OUTPUT), the limiter
  indicator (LIMIT), the output-meter caption (OUT), both preset buttons (LOAD / SAVE), the
  preset dropdown's two JS-written strings (`No presets available`, `Factory`) and the
  popover's own LANGUAGE row.
- **Accessible names:** the five native `title=` attributes on the preset bar are **deleted**,
  not localized (contract §4 — a native title renders a second, untranslated OS tooltip). Their
  existing English moved verbatim into `data-i18n-aria`; no new prose was invented. The two
  decorative `<img alt>` strings are keyed through `data-i18n-alt`.
- **No hover-help was authored.** `I18N` and `TIP_BINDINGS` are both empty, which is this
  plugin's correct state and which `check-i18n` assertion 2 reports as "0 tip(s) bound".
  Authoring hover-help copy is Stage M's job.
- **Language persistence:** `uiLanguage` rides the APVTS state tree as a non-parameter
  property, written as a readable `"en"` / `"fr"` string before `getStateAsXml()` and read back
  after `setStateFromXml()` through an `isVoid()` guard — the XML round-trip rebuilds every
  property as a `var` over the attribute STRING, so `isBool()` would never fire
  (`critical_valuetree_xml_roundtrip_loses_type`). It is deliberately **not** an
  `AudioParameterChoice`: a preset must not be able to change which language somebody reads
  their plugin in. The page PULLS it once at init; nothing pushes from the editor constructor,
  which would race the WebView's load.

### Changed
**Six width pins, so the page's geometry no longer depends on the language.** At 420 x 320
this is the narrowest frame in its batch and nothing on it had a fixed-width text box: every
row is `justify-content: center` over shrink-to-fit children, so any caption that changed
width — in *either* direction — moved its neighbours. Half the French below is NARROWER than
its English, which a clip check cannot see at all.

- `#loadPreset` → **49 px**, `#savePreset` → **46 px** (each its own English border box rounded
  up). NOT the 62 px the rest of the batch used: `.preset-row` already measured **374.16 px
  inside a 374.00 px content box** in English at v1.3.3, so 62 px would have added 31.39 px to
  a row with zero slack.
- `.limit-label` → 36 px, `.controls-row .knob-container:nth-child(1) .knob-label` → 86 px,
  `:nth-child(2)` → 68 px, `.settings-popover` → 170 px. Each was reverted alone and confirmed
  to re-break `check-ui-labels` assertion 7.
- `.knob-container:nth-child(3)`'s label is deliberately **unpinned**: OUTPUT (56.02) and
  SORTIE (50.94) are both under the 65 px knob floor, so that container measures 65.00 px in
  either language.
- `.meter-label` → 24 px and `.settings-label { white-space: nowrap }` ship as declared
  **guards**, not pins: each one's negative control PASSES on the shipped strings.

**English geometry:** 30 of 38 elements moved, 2 added (the gear cluster), 0 vanished, document
scroll extent unchanged at 420 x 320. The largest move is 1.42 px, on the preset bar, and every
one of the 30 is attributable to a named pin. **French vs English: zero non-label elements
moved**, in the default state, with the popover open and with the preset dropdown open.

**No parameter IDs, ranges, types, defaults or DSP behaviour changed.**

### Notes
- All nineteen French strings are **machine drafts**, every entry `reviewed: false`. No native
  speaker has read them.
- The French on this page is abbreviated where the frame demanded it, and the alternatives are
  recorded with their measured cost in `js/i18n.js`: CHARGER (51.30) and SAUVER (43.02) do not
  fit the preset row, LIMITE (43.27) does not fit the limiter indicator, and SORTIE (40.41) on
  the meter caption would move the meter 8.42 px in **both** languages. `label.out` therefore
  ships `sameAsEn` — keyed rather than exempted, so the judgement stays on the reviewer's
  worklist.
- **Found and deliberately not fixed:** `.preset-row` overflows its content box by 0.16 px in
  ENGLISH and has since the layout was authored. It is sub-pixel, invisible, and repairing it
  would change English geometry on a row this commit only pinned.


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
