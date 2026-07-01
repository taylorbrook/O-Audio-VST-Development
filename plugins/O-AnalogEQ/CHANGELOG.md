# O-AnalogEQ Changelog

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
