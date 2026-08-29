# O-AnalogEQ Changelog

## [1.2.0] - 2026-08-28

### Added — the PAGE speaks French (Stage K batch K2, canon v2)

- **`Source/ui/public/js/i18n.js`** — seventeen keys: ten visible captions and
  seven accessible names. Embedded in `juce_add_binary_data` SOURCES **and**
  served from a `getResource()` branch, in this same commit (check-i18n
  assertion 8 exists because a file embedded but not served is a 404 that
  presents as a page stuck in English and nothing else).
- **A settings popover** in the empty bottom-left margin carrying the language
  selector. ONE row: this plugin has no hover-help to switch on or off.
- **`getUiLanguage` / `setUiLanguage`** native functions and session
  persistence. The page PULLS once at init; nothing is pushed from the editor
  constructor, which would race the WebView's load. `uiLanguage` is a
  non-parameter property on the APVTS state tree read back through an
  `isVoid()` guard, because the XML round-trip rebuilds every property as a
  `var` over the attribute STRING.
- `plugins/O-AnalogEQ/tests/i18n-states.json` — drives the popover and the
  preset dropdown so `check-ui-labels` measures 9 of 9 keyed elements.

**No hover-help was authored.** `TIP_BINDINGS` and `I18N` are both empty, which
is this plugin's correct state and which assertion 2 reports as "0 tip(s)
bound". Authoring that copy is Stage M's job.

### Changed

- **The five native `title=` attributes on the preset bar are DELETED**
  (contract §4) and their existing English moved verbatim into
  `data-i18n-aria`. No new prose was invented. Both button names contain their
  own visible caption as a prefix, so label-in-name holds in both languages
  (WCAG 2.5.3). `#presetName` is the one place that rule cannot hold — its
  visible text is a runtime preset name, exempt under D-02.
- **`#savePreset` / `#loadPreset` pinned to 62 px.** Unpinned, SAUVER and
  CHARGER widen the right-anchored flex bar and shove `#presetBar`,
  `#prevPreset`, `#presetName` and `#nextPreset` left by 34.4 px; removed
  alone, `check-ui-labels` assertion 7 reports exactly those four. 62 px is
  O-Chorus's and O-DigiDelay's number, kept so the suite's preset bar is one
  shape. Its cost is that the ENGLISH cluster now sits 44.33 px further left,
  in a header whose middle is empty paper.
- **`.band-label { white-space: nowrap }`** — NOT a geometry pin and not
  claimed as one; its negative control passes on the shipped strings. It ships
  because it converts this page's third failure mechanism, which BOTH gate
  assertions are structurally blind to, into one that assertion 4 catches. See
  the comment in `index.html` and the note below.

### Geometry, measured at 920 x 220 over every rendered box, at 180 ms and 1.7 s

The page HOLDS STILL: zero of 155 elements differ between the two settle times
at a 0.01 px tolerance, in either language, in either popover state. Nothing
free-runs off the wall clock in the harness, so there is no assertion-7
animation NOTE on this plugin and none was expected.

- **English v1.1.11 → v1.2.0:** 6 of 147 elements moved, 8 added (the settings
  cluster), 0 vanished, document scroll extent 920 x 220 unchanged. All six
  moved are the preset bar and all six are the one pin. ZERO band labels, ZERO
  knob or notch elements, ZERO VU-meter elements, zero title movement.
- **French vs English at v1.2.0: ZERO elements moved**, in the default state
  and with the preset dropdown open — not one of 155 boxes, including the keyed
  captions themselves, because every one of them has a pinned width. With the
  settings popover open, ONE element moves and it is the keyed
  `.settings-label` shrinking 14.44 px (LANGUAGE → LANGUE); zero non-label
  elements move, which is the `.settings-popover { width: 170px }` pin working.
  Removed alone, assertion 7 reports the panel, its row and `#lang-select`.

### The three cliffs, and the one that is invisible

| | Control | Budget | Mechanism | Caught by | Blind |
|---|---|---|---|---|---|
| A | `#analog` | 57.00 px | SPILL | `[4][fr]` | `[7]` |
| B | `#savePreset` / `#loadPreset` | — | PUSH | `[7]` | `[4]` |
| C | `.band-label` | 67.00 px | WRAP | **nothing** | both |

All three were PLANTED and watched, because "no failures" is otherwise
indistinguishable from a sweep that cannot see them:

- **A.** French `ANALOGIQUE` fails `[4][fr] "ANALOGIQUE" 66.7>57.0` and moves
  nothing at all.
- **B.** Removing the 62 px pin fails `[7]` with four elements at dx=-34.4
  while every `[4]` check stays green.
- **C.** French `PLATEAU BF` on `#lf_on`, with `nowrap` removed, passes
  **EVERY ASSERTION** while the caption grows 21 → 34 px and reaches y=86 into
  the knob ring that begins at y=75. `.band-label` is `position: absolute` with
  an inline `width: 85px` and no fixed height, so a wrapped caption exceeds
  neither its content width nor its own grown content height, and being both
  the keyed element and absolutely positioned it pushes no sibling. With
  `nowrap` present the same string fails `[4][fr] "PLATEAU BF" 81.8>67.0`.

### French, all machine drafts (`reviewed: false`)

Rendered widths against each control's measured content box:

```
LF SHELF  63.03 -> LF PLAT.  57.44   9.56 spare   (67.00 box)
LMF       28.41 -> LMF       28.41   sameAsEn
HMF       29.63 -> HMF       29.63   sameAsEn
HF SHELF  64.25 -> HF PLAT.  58.66   8.34 spare
ANALOG    42.33 -> ANALOG.   45.30  11.70 spare   (57.00 box)
Level     34.16 -> Niveau    41.33  66.67 spare   (108.00 box)
SAVE      24.52 -> SAUVER    39.25   8.75 spare   (48.00 box, pinned)
LOAD      27.16 -> CHARGER   46.80   1.20 spare   TIGHTEST SHIPPED
Language  55.31 -> Langue    41.08   SHRANK
```

TWO of the ten SHRINK and TWO do not change at all — a clip-only check would
have certified this page.

`LF` / `LMF` / `HMF` / `HF` are kept verbatim: they are the band abbreviations
silk-screened on the French market's own consoles, so translating them would
make the plugin less legible, not more. Only SHELF — the filter TYPE — is a
word, and it becomes PLAT. Every fuller form is past the 67.00 px wrap cliff
(PLATEAU BF 81.77, PLATEAU HF 82.98, BAS MEDIUM 85.63, HAUT MEDIUM 97.14) and
`MED.HAUT` (70.77) stays on one line only to spill 3.77 px. ANALOGIQUE (66.70)
would need the button widened from 75 to 85 px — a layout change caused by
French, which this page did not need. OUVRIR (37.75) is the reviewer's lever if
CHARGER's 1.20 px is judged too thin on Windows metrics.

### Not changed, and why

- **`WIDE` / `MED` / `TIGHT` do not translate.** They are the three
  `lmf_q` / `hmf_q` `AudioParameterChoice` options declared verbatim as
  `juce::StringArray { "WIDE", "MED", "TIGHT" }` (`PluginProcessor.cpp:56,
  :69`) — D-01 arm 1. A French face reading MOYEN against a DAW automation lane
  offering MED is the divergence that arm exists to prevent. The consequence
  for this release is that `.three-way-option`'s `white-space: nowrap` is never
  reached by a French string.
- **Both three-way Q toggles clip their own TIGHT option in ENGLISH**, and this
  release does not fix it. `.three-way-option` is `flex: 1` without
  `min-width: 0`, so the three items sit at their min-content widths — 35.95 +
  32.83 + 39.09 plus two 1 px gaps = 109.87 px inside a 108 px content box —
  and `.three-way-toggle { overflow: hidden }` clips 1.87 px off TIGHT's right
  edge. Pre-existing since the control was authored, present on `#lmf_q` and
  `#hmf_q` alike, invisible to both gates because these nodes are exempt and
  never keyed. Fixing it means changing English geometry on an exempt control
  for a non-localization reason, which is a layout decision.
- **The vendored `modules/preset-manager.js`** still carries four native
  `title=` attributes inside a `createPresetBar()` factory this page never
  calls. Dead markup in a SHARED module, so a local edit is reverted by
  `/module-upgrade`.

No parameter IDs, ranges, types, defaults or DSP behaviour changed.

### Verification

`check-i18n --strict-v2` exits 0, 39 assertions, canon v2.
`check-ui-labels` exits 0, 75 assertions across three states, 9 of 9 keyed
elements visible, no page error, every resource served.
`boot-all-uis` 41/43 clean, verdict identical to HEAD's (O-Bowed and O-Reed
fail on an unrelated pre-existing `Unexpected token export`); O-AnalogEQ reads
text 24 → 27, title 5 → 0, aria 0 → 8, i18n 0 → 9, and the repo title total
falls 453 → 448.
`auval -v aufx OuAE OuDv`: AU VALIDATION SUCCEEDED. Both installed bundles
report 1.2.0 and the binary carries the table, both native function names and
`/js/i18n.js`.

## [1.1.11] - 2026-08-02

### Changed
- Added AGPL-3.0 license notice headers to all Ouaricon-authored source files. No functional changes.

## [1.1.10] - 2026-06-30

### Fixed (code-review info items)
- **IN-01 (hidden `output_gain`):** Documented that `output_gain` is intentionally not
  surfaced in the UI (the output knob was deliberately removed in the v1.0.5 simplification).
  It stays a host-automatable parameter (default 0 dB) set by some factory presets. Added a
  code comment so it isn't mistaken for a missing binding. No behavior change. (Kept hidden.)
- **IN-02 (double-click reset):** Reset now restores each frequency knob's true APVTS default
  (100 / 500 / 2000 / 8000 Hz) via the skew inverse, instead of normalised 0.5 (which was
  ~77 Hz for LF, not the real default). Gains already reset correctly (0 dB = 0.5).
- **IN-03 (dead code):** Removed the unused `currentParamName` variable and its assignments
  from `setupDualKnob` (`currentState` already carries the reference).
- **IN-04 (unbounded poll):** `preset-manager` module — `_waitForNative()` now bounds its
  poll (100 × 50 ms = 5 s) and logs an error instead of hanging preset init forever if the
  JUCE backend never appears.
- **IN-05 (fragile `confirm()`):** `preset-manager` module — `promptDelete()` now prefers an
  optional `onConfirmDelete` hook (reliable native/in-DOM dialog) and guards the
  `window.confirm()` fallback, which is a silent no-op/throw in some JUCE WebView backends;
  aborts fail-safe (no accidental delete) and logs when no confirmation mechanism exists.

### Notes
- Closes all remaining items from the 2026-06-30 code review (`.planning/CODE-REVIEW.md`):
  CR-01 + WR-01 (1.1.8), WR-02/03/04 (1.1.9), IN-01..IN-05 (this release).
- IN-04/IN-05 fixes were made in the **shared `preset-manager` module** (bumped to 1.0.1);
  O-AnalogEQ's copy is synced. The other 10 dependent plugins can adopt it via `/module-upgrade`.

## [1.1.9] - 2026-06-30

### Fixed
- **WR-02 (zipper noise on automation):** Frequency and gain were read once per block
  and coefficients jumped straight to the new value, producing audible zipper/clicks when
  automating or dragging — worst on the skewed frequency ranges where a small knob move is a
  large Hz jump. Each band's frequency/gain is now a `juce::SmoothedValue` (30 ms linear ramp,
  seeded to the current parameter in `prepareToPlay` so nothing swoops on load). While a band
  is ramping, its coefficients are rebuilt every 32 samples so the response glides to the
  target. Root cause: unsmoothed per-block coefficient steps.
- **WR-02 / CR-01 (RT-safety preserved):** The per-chunk rebuild uses
  `juce::dsp::IIR::ArrayCoefficients::make*` (returns a stack `std::array`) assigned into the
  existing filter state, instead of the allocating `Coefficients::make*` factories. The math is
  identical (the factories just wrap `ArrayCoefficients` in a heap allocation), so the sound is
  unchanged, and the audio thread never allocates — even mid-automation. When no band is moving,
  the block still runs in a single pass with no coefficient recompute (CR-01 steady-state path).
- **WR-03 (Nyquist clamp):** Every band's cutoff is now clamped to `0.99 × Nyquist` before
  building coefficients. Previously `hf_freq` (up to 20 kHz) and `hmf_freq` (up to 8 kHz) were
  passed straight through, producing degenerate/NaN coefficients at host sample rates below
  ~40 kHz. Verified via `auval` render tests at 22050 Hz and 11025 Hz. Root cause: unbounded
  cutoff vs. sample rate.
- **WR-04 (FileChooser use-after-free):** The async save/load `FileChooser` completion lambdas
  captured `this` and dereferenced the processor. If the editor window closed while the OS
  dialog was still open, the callback fired against a destroyed editor. Both callbacks now
  capture a `juce::Component::SafePointer` and bail early if the editor was deleted. Root cause:
  raw `this` capture across an async native dialog.

### Notes
- Closes the remaining WARNING items from the 2026-06-30 code review (`.planning/CODE-REVIEW.md`).
  CR-01 and WR-01 were fixed in 1.1.8. Only the IN-* info items remain (all benign/documented).

## [1.1.8] - 2026-06-30

### Fixed
- **CR-01 (RT-safety, critical):** `processBlock` rebuilt all four bands' IIR coefficients
  every block via the allocating `IIRCoefficients::make*` factories, heap-allocating on the
  audio thread even when no parameter changed. Now each band's coefficients are recomputed
  only when its frequency/gain/Q inputs actually change (guarded against cached last-seen
  values), making the steady-state playback path allocation-free. Cached sentinels are reset
  in `prepareToPlay` so coefficients still rebuild on the first block after prepare and on a
  sample-rate change. DSP output is unchanged (same `make*` formulas on change).
  Root cause: unconditional per-block coefficient rebuild.
- **WR-01 (display correctness):** Frequency tooltips showed wrong Hz because the JS formatters
  mapped the normalised knob value linearly, ignoring the C++ `NormalisableRange` 0.3 skew
  (e.g. the 100 Hz LF default displayed as ~301 Hz). Formatters now invert the skew
  (`hz = min + (max - min) * pow(v, 1/0.3)`) so displayed Hz matches the actual filter
  frequency across all four bands. Gain readouts were already correct (linear skew) and are
  unchanged. Root cause: JS display math did not mirror the parameter's frequency skew.

### Notes
- Both issues from the 2026-06-30 code review (`.planning/CODE-REVIEW.md`). Remaining review
  items (WR-02 coefficient smoothing, WR-03 Nyquist clamp, WR-04 FileChooser lifetime, and the
  IN-* info items) are not addressed in this patch.

## [1.1.7] - 2026-02-09

### Added
- Preset system with save/load functionality

### Changed
- UI improvements: moved title left, presets right
- EQ algorithm upgrades

## [1.1.4] - 2026-02-05

### Changed
- Added licensing module integration (compile-flag gated, OFF for local dev)
- Added branding variables for company name, manufacturer code, and dev suffix
- Added WebView2 backend support for Windows compatibility

### Fixed
- Windows CI build failure - added preprocessor guards for `withResourceProvider` when WebView2 SDK is missing
- Added `JUCE_USE_WIN_WEBVIEW2=1` compile definition

## [1.1.3] - 2026-02-05

### Fixed
- **Windows CI build failure** - `withResourceProvider` is not available when WebView2 SDK is missing
  - Added `#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE` preprocessor guards around `withResourceProvider` and `getResourceProviderRoot()` calls in PluginEditor.cpp
  - Added WebView2 NuGet package installation step to Windows CI build workflow
  - No functional change on macOS/Linux where resource provider is always available

## [1.1.2] - 2026-02-05

### Changed
- Published release with code signing and Apple notarization via GitHub Actions CI/CD

## [1.1.1] - 2026-02-04

### Changed
- **Eliminated duplicated filter coefficient logic** - Removed `updateFilterCoefficients()` and 8 `previous*` change-detection members; coefficients now set directly in `processBlock()` via a shared `dBtoGain` lambda and a `constexpr qValues[]` lookup table
- **Replaced `ProcessSpec` member with `double currentSampleRate`** - Only the sample rate was needed between `prepareToPlay` and `processBlock`; `ProcessSpec` is now a local in `prepareToPlay`
- **Defaulted empty destructor and `releaseResources()`** - Removed boilerplate empty bodies
- **Added `StereoFilter` type alias** - Shortened repeated `ProcessorDuplicator<IIR::Filter<float>, IIR::Coefficients<float>>` declarations to a single alias
- **Removed unused `needsUpdate` variable** from `processBlock()`
- **Consolidated parameter layout** - Reduced verbose per-parameter comments to band-level comments; one `layout.add` call per line
- **Merged `setupBandLabelToggle` and `setupToggle`** in index.html into a single `setupToggle(element, state, className, activeWhen)` function
- **Removed `getQValueFromChoice()` helper** - Replaced with `constexpr qValues[]` array indexed directly

### Code metrics
- PluginProcessor.cpp: 399 → 195 lines (51% reduction)
- PluginProcessor.h: 82 → 55 lines (33% reduction)
- index.html JS: removed 15 lines of duplicate toggle logic

## [1.1.0] - 2026-01-24

### Changed
- **Renamed plugin** - Changed from "OuariconAnalogEQ" to "O-AnalogEQ"
  - Directory: `plugins/OuariconAnalogEQ/` → `plugins/O-AnalogEQ/`
  - DAW display name: "Ouaricon Analog EQ" → "O-AnalogEQ"
  - Binary names: Now `O-AnalogEQ.vst3` and `O-AnalogEQ.component`
  - Consistent with O-Tremolo and O-DigiDelay naming convention
- Internal CMake target remains `OuariconAnalogEQ` for preset/session compatibility

## [1.0.10] - 2026-01-11

### Changed
- **VU meter moved left** - Shifted 40px left (now at left: 758px)
- **Analog saturation retuned** - Changed from `tanh(x * 1.5) * 1.1` to `tanh(x * 0.5) * 2.0`
  - Now gain-neutral (no volume boost)
  - Adds subtle harmonic warmth/coloration without level change
  - Lower drive preserves dynamics while adding character

## [1.0.9] - 2026-01-11

### Changed
- **VU meter reduced to 80%** - Now 112x112px (was 140x140px) for better proportions
- **VU meter repositioned** - Adjusted position to fit new size
- **Analog button moved right** - Shifted 20px right (left: 620px) for better spacing

## [1.0.8] - 2026-01-11

### Changed
- **VU meter doubled in size** - Now 140x140px (was 70x70px) for better visibility
- **VU meter shifted right** - Positioned at far right edge of window
- **Analog button repositioned** - Now centered between HF shelf dial and VU meter
- **VU meter scale updated** - Larger text and arc for readability at new size

## [1.0.7] - 2026-01-11

### Changed
- **Title on single line** - Widened title container to prevent line break
- **Removed SHELF sublabels** - Cleaned up redundant labels below LF and HF dials
- **Q toggles moved down** - WIDE/MED/TIGHT buttons lowered by 10px for better spacing
- **Flower centered vertically** - Botanical overlay now vertically centered in window
- **Renamed band labels** - LF → "LF SHELF", HF → "HF SHELF" for clarity

## [1.0.6] - 2026-01-11

### Changed
- **Widened Q toggles** - WIDE/MED/TIGHT buttons increased to 110px (fully visible text)
- **Centered band labels** - LF/LMF/HMF/HF toggle buttons now centered above their dials
- **Resized botanical overlay** - Flower reduced to 75% and repositioned to end at far right
- **Updated title** - Changed from "OUARICON ANALOG EQ" to "OUARICON ANALOG EQUALIZER"

## [1.0.5] - 2026-01-11

### Changed
- **Removed output gain dial** - Simplified UI by removing the output gain control
- **Analog button moved under VU meter** - Better visual grouping of output section
- **Band labels are now toggles** - LF/LMF/HMF/HF labels function as on/off buttons
  - Green = band active, brown = band bypassed
  - Removed separate on/off buttons below each dial
- **Improved layout spacing** - Four EQ bands now evenly distributed with VU meter on right

## [1.0.4] - 2026-01-11

### Changed
- **Centered knob layout** - All controls now properly centered in the UI
- **Widened Q toggle buttons** - WIDE/MED/TIGHT labels no longer truncated (95px width)
- **Added OUTPUT/GAIN labels** - Output gain knob now has proper labeling
- **Vertical default position** - All knobs initialize at 12 o'clock (center) position
- **Double-click reset** - Double-clicking any knob returns it to default position
- **Green gradient on outer rings** - Outer frequency rings now have botanical green gradient
- **Added frequency notches** - SVG tick marks around dual-layer knobs show frequency position

## [1.0.3] - 2026-01-11

### Fixed
- **Dual-layer knobs now functional** - Implemented distance-based hit detection
  - Outer ring (>60% from center) controls frequency
  - Inner dial (<60% from center) controls gain
  - Added outer ring indicator for visual feedback
  - Added value tooltips showing both freq and gain values
- **VU meter now responds to actual audio levels** - Marimba-style implementation
  - C++ PluginProcessor calculates peak output level in processBlock
  - PluginEditor uses Timer to emit `outputLevel` events to WebView at 30Hz
  - JavaScript animates needle with ballistic motion (fast attack, slow decay)
  - Needle color interpolates from green (quiet) to red (loud)

## [1.0.2] - 2026-01-11

### Fixed
- **Missing check_native_interop.js** - Added the required JUCE WebView JavaScript bridge file
  - Root cause: JavaScript module `index.js` imports `check_native_interop.js` which sets up `window.__JUCE__.backend`
  - Without this file, no C++ ↔ JavaScript communication was possible
  - Added file to CMakeLists.txt resources and PluginEditor.cpp resource provider

## [1.0.1] - 2026-01-11

### Fixed
- **GUI controls now interactable** - Fixed type mismatch between C++ WebView relays and JavaScript state accessors
  - Root cause: Q parameters (`lmf_q`, `hmf_q`) were using `WebSliderRelay` but JavaScript expected `WebComboBoxRelay`
  - Changed `lmfQRelay` and `hmfQRelay` from `WebSliderRelay` to `WebComboBoxRelay`
  - Changed `lmfQAttachment` and `hmfQAttachment` from `WebSliderParameterAttachment` to `WebComboBoxParameterAttachment`

## [1.0.0] - 2026-01-11

### Added
- Initial release
- 4-band analog-style EQ (LF shelf, LMF bell, HMF bell, HF shelf)
- Per-band frequency, gain, and Q controls (Q on bell bands only)
- Per-band bypass toggles
- Global output gain control
- Analog warmth/saturation toggle
- VU meter display
- WebView UI with botanical paper aesthetic
