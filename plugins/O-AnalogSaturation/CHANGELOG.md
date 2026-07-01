# Changelog

All notable changes to O-AnalogSaturation will be documented in this file.

## [1.1.4] - 2026-06-30

### Fixed
- **CR-01 — Tone filters now track Quality correctly.** Every model's tone-shaping
  filters (TRANSFORMER LF bump/HF sheen, TUBE presence, MAGNETIC head bump/HF rolloff)
  run *inside* the oversampled nonlinear path, but were designed against the base sample
  rate — so at the default MID (2x) quality every EQ corner sat an octave low, and two
  octaves low at HIGH (4x). Coefficients are now designed per Quality at the rate the
  path actually executes (base·1x/2x/4x) and the active set is swapped on Quality change
  (RT-safe `Coefficients::Ptr` swap, biquad state preserved). **Note:** this corrects the
  shipped default tonal character — MID/HIGH now sound as intended, LOW is unchanged.
- **CR-02 — Auto-gain now tracks program material.** The RMS envelope coefficient was a
  per-*sample* one-pole applied once per *block*, giving a realized time constant of ~50s
  (and drifting with host block size) instead of the intended 100 ms. The coefficient is
  now derived per block from the actual block length — `exp(-N / (0.1·fs))` — so the
  100 ms tracking holds at any block size or sample rate.
- **CR-03 — Zero-length blocks no longer corrupt output.** Both RMS routines divided by
  the sample count with no guard; a `processBlock` call with `numSamples == 0` (which some
  hosts issue) produced `0/0 = NaN`, which the `< 1e-8f` flush never cleared and which then
  poisoned every subsequent block (NaN output, no recovery until re-prepare). Added an
  early-out for empty blocks and hardened the envelope flush to catch non-finite state
  (`!std::isfinite(env) || env < 1e-8f`).
- UI footer version label corrected (was stale `v1.1.2`).

### Known follow-ups (not addressed in this release)
- WR-02 (dry signal colored/delayed by the oversampler in MID/HIGH) and WR-03 (auto-gain
  applied as a block-constant multiplier — CR-02 makes its per-block stepping more
  audible when Auto Gain is enabled) remain open warnings from the same review.
- The MAGNETIC Jiles-Atherton integrator and its `deltaH` clamp are per-sample and thus
  still rate-sensitive across Quality; the CR-01 fix corrects the tone filters only.

## [1.1.3] - 2026-02-25

### Added
- Ouaricon licensing module integration (compile-flag gated, zero impact on local builds)

## [1.1.2] - 2026-02-25

### Added
- Version number displayed in bottom-right corner of UI

## [1.1.1] - 2026-02-07

### Changed
- Removed dead state variables: `diodePrevVoltage`, `tubePrevPlateVoltage`, `TUBE_VSUPPLY`, `oversamplingLow`, `spec`
- Removed unused `iterations` parameter threading through DSP functions
- Cleaned function signatures to match actual usage
- Removed stale phase comments from implementation era

### Fixed
- Added `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` for Windows WebView2 support

## [1.1.0] - 2026-01-24

### Changed
- Renamed plugin from "OuariconSaturationModeling" to "O-AnalogSaturation"
- Updated class names: `OuariconSaturationModelingAudioProcessor` → `OAnalogSaturationAudioProcessor`
- New plugin code: OaSa (was OsSM)
- Consistent branding with O-series plugins (O-Tremolo, O-Comp, O-AnalogEQ, O-DigiDelay)

**Note:** Parameter IDs unchanged - existing presets and automation compatible.

## [1.0.1] - 2026-01-14

### Fixed
- Snake PNG opacity now transitions smoothly with intensity knob movement
- Opacity no longer snaps back when releasing the knob

**Root Cause:** Visual updates were triggered twice per frame during drag - once directly in the mousemove handler and once via the `valueChangedEvent` listener. When the two values differed slightly, it caused jitter and snap-back on release.

**Fix:** Removed direct `updateKnobVisual()` call from mousemove handler. The `valueChangedEvent` listener is now the single source of truth for visual updates, allowing the CSS transition to work properly.

## [1.0.0] - 2026-01-09

### Added
- Initial release
- Four saturation models: Magnetic, Tube, Transformer, Diode
- Intensity knob with visual feedback
- Input/Output VU meters
- Quality settings (Low, Mid, High)
- Autogain toggle
- Vintage botanical illustration theme with snake imagery
