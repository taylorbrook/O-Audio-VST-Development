# Changelog

All notable changes to O-AnalogSaturation will be documented in this file.

## [1.2.0] - 2026-08-29

The PAGE speaks French, not only a tooltip — because this plugin never had a tooltip.

### Added
- **Language selector, in a gear popover top-right.** Styled in this page's own
  vocabulary (olive fill over the `#3C5C1A` border, Garamond, uppercase) rather than
  pasted in from another plugin. Opens downwards: the gear is 12 px from the top of a
  450 px frame. One row, because there is no hover-help to switch on or off.
- **`Source/ui/public/js/i18n.js`** — the label table, English + French, on canon v2.
  Embedded in `juce_add_binary_data` SOURCES *and* served from a `getResource()` branch
  in the same commit: a file embedded but not served is a 404 that presents as a page
  stuck in English and nothing else.
- **The UI language persists with the session.** A non-parameter `uiLanguage` property on
  the APVTS state tree, saved as `"en"`/`"fr"` and read back through an `isVoid()` guard —
  the XML round-trip rebuilds every property as a var over the attribute STRING, so an
  `isBool()`/`isInt()` test would be false for every saved session. Deliberately not an
  `AudioParameterChoice`: the language must not appear in a DAW automation lane and a
  preset must not be able to change which language somebody reads their plugin in.
- The snake illustration's `alt` text is keyed rather than left English.

### Changed
- Six visible strings localize: IN, OUT, INTENSITY, QUALITY, AUTOGAIN and the popover's
  own Language caption. **Seven do not, and each says why in `I18N_EXEMPT`:** the four
  model captions and the three quality captions are the `AudioParameterChoice` option
  strings byte for byte, so translating the caption alone would make the page and the
  host automation lane disagree about the same setting. The title is a product name.
- The version label in the page read **v1.1.5** while `CMakeLists.txt` declared 1.1.6 —
  it had not been bumped with the v1.1.6 licensing pass. Now v1.2.0, matching.

### Notes
- All French is a machine draft, every entry flagged `reviewed: false`. No native speaker
  has read it.
- No hover-help copy was authored: `TIP_BINDINGS` and `I18N` are both empty, which is this
  plugin's correct state rather than a gap. Authoring that prose is a later stage's job.
- Zero geometry movement. The English page is byte-for-byte where it was — 0 of 48
  elements moved, 8 added (the gear cluster) — and no non-label element moves between
  English and French. Two of the five page labels get SHORTER in French, not longer.

## [1.1.6] - 2026-08-02

### Changed
- Source files now carry AGPL-3.0 license notice headers (repo-wide licensing pass;
  no functional changes).

## [1.1.5] - 2026-06-30

Resolves the remaining open findings from the v1.1.3 deep code review (WR-01…WR-05,
IN-02, IN-03) plus the MAGNETIC rate-consistency note under CR-01.

### Fixed
- **WR-01 — Latency reported off the audio thread.** `setLatencySamples()` (which calls
  `updateHostDisplay()` and can lock/reallocate in the host) was invoked from `processBlock`
  on a Quality change. The audio thread now stores the new latency and triggers an
  `AsyncUpdater`; the host notification happens on the message thread in `handleAsyncUpdate()`.
- **WR-02 — Dry path no longer colored/delayed by the oversampler.** Previously the whole
  buffer (dry + wet) was up/down-sampled, so the "dry" component picked up the oversampler's
  FIR anti-imaging/anti-aliasing coloration and latency, and LOW vs MID/HIGH sounded
  different at low Intensity. The models now output the pure wet signal; a clean base-rate
  dry copy is kept and mixed back in *after* downsampling, delayed by the oversampler
  latency (`juce::dsp::DelayLine`) so dry and wet stay phase-aligned. LOW quality (0 latency)
  bypasses the delay and stays sample-exact.
- **WR-03 — Auto-gain compensation is smoothed.** The per-block compensation gain was applied
  as a flat multiply, stepping (zipper/click) at block boundaries once CR-02 made the envelope
  actually track. It now ramps per sample toward the block target via a per-channel
  `juce::SmoothedValue` (20 ms), holding unity while disabled so re-enabling ramps cleanly.
- **WR-04 — Added `isBusesLayoutSupported`.** Accepts only mono or stereo with matching
  input/output channel sets, instead of relying on the permissive `AudioProcessor` default.
- **WR-05 — Non-zero tail length.** `getTailLengthSeconds()` now returns 0.05 s (was 0) so
  hosts don't truncate the IIR ring / hysteresis decay on offline bounce/freeze.
- **CR-01 addendum — MAGNETIC consistent across Quality.** The Jiles-Atherton `deltaH` clamp
  was an absolute per-sample limit (±0.3), so a transient split across more oversampled steps
  was clamped less — MAGNETIC changed character with Quality. The clamp is now scaled by the
  oversampling factor (±0.3 / 1·2·4) so the realized field slew limit is identical at
  LOW/MID/HIGH. (The tone-filter half of CR-01 was fixed in v1.1.4.)

### Changed (internal)
- **IN-02 — Shared Langevin small-argument threshold.** `langevinFunction` and its derivative
  branch in the magnetic model now use one `LANGEVIN_TAYLOR_THRESHOLD` (1e-4) so L and L′ use
  matching series/limit forms in the crossover window (also dodges catastrophic cancellation
  in `coth(x) - 1/x` for small x).
- **IN-03 — Named tuning constants.** Per-model drive ranges (6.0 / 7.5 / 4.5 / 3.0), diode
  hardness (0.7) and tube output normalization (1.2) are now named `static constexpr` members;
  the stale "(was X)" development comments were removed.

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
