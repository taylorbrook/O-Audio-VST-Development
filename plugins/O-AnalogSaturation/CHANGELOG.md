# Changelog

All notable changes to O-AnalogSaturation will be documented in this file.

## [1.4.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `oasat.tipsEnabled`.
- **`data-tip-always` on `#gear-btn` and on `#tips-toggle`, and on nothing
  else.** Those two controls are the ones that REACH and RESTORE the help
  layer, so they keep explaining themselves while it is off. `#lang-select`
  deliberately does not carry it: it is only reachable through the gear, which
  already explained itself on the way in.
- **Five i18n keys, four of them settled roots copied rather than authored.**
  `label.hoverHelp`, `ui.on`, `ui.off` and `aria.helpToggle` take the French
  glossary roots verbatim from `scripts/i18n-fr-glossary.js` — *Aide au survol
  / Marche / Arrêt / Activer ou désactiver l'aide au survol*. The fifth,
  `tip.tipsToggle`, is the tooltip's own title and body.

### Measured

- **The second row costs nothing.** With the popover forced open it occupies **y 40..104, 186 x 64 px — byte-identical in English and French** — inside a 600 x 450 frame, clearing the frame's top by 40 px. The switch face measures **42.00 x 18.00 px in both languages**: *On* and *Marche* both fit inside the 42 px floor.
- **The switch's face is a `min-width: 42px` floor, not a pinned width**, so a
  longer French face grows LEFTWARD into slack the popover already has. The row
  is `space-between` and the button is a `[data-i18n]` node, so nothing the
  geometry gate measures moves. `check-ui-labels` [7] reports **0 non-label
  elements displaced** between English and French, and the visible element set
  identical in both.
- Every declaration in `.settings-toggle` above the four switch-specific ones
  is **copied from this page's own language `<select>`** — its font stack, ink,
  plate, hairline and radius — so the two controls in the popover match by
  construction rather than by a second designer re-deciding them.

### Decided

- **Default is ON.** The previous version showed hover help unconditionally, so
  ON is the setting that leaves an existing user's plugin behaving exactly as
  it did. Default OFF would additionally have made `boot-all-uis --strict-tips`
  measure an empty tip surface and call it correct.


## [1.3.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **5 of 15 French entries revised** against the suite glossary and lint — 16 individual
  changes: 2 terminology, 8 typography, 1 grammar, 5 meaning and idiom. The visible ones:
  the clean signal is now the *signal direct* rather than the *signal propre* (the suite's
  settled word for dry); TRANSFORMER's weight and sheen sit *dans le grave* and *dans
  l'aigu* rather than *en bas* and *en haut* (band names, not directions); QUALITY no
  longer says the oversampling stops saturation *de replier du repliement* — it stops it
  *de rabattre du repliement de spectre*; TUBE's presence lift became a *relèvement* rather
  than a *relief*; DIODE's edge got its "harder" back; and AUTOGAIN's quiet passage is now
  *un passage de faible niveau* that *il relève* — the old wording hung *sans s'emballer*
  off the passage instead of off the compensation. Typography: no-break spaces before every
  `;`, `:` and `%` in the French bodies, per the suite rule.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so
  assistive technology reads the page in the language it is displayed in.

## [1.3.0] - 2026-08-30

Hover-help, in both languages — and the renderer that makes it visible.

### Added
- **Hover-help on every control that has a parameter behind it**, plus the gear and the
  language selector: six tips, each with an English and a French title and body.
  `INTENSITY`, `MODEL`, `QUALITY` and `AUTOGAIN` are the plugin's whole parameter set,
  taken from a runtime dump of `AudioProcessor::getParameters()`
  (`.planning/params.tsv`) rather than from a regex over `createParameterLayout()`.
- **`.planning/params.tsv` and the `OUARICON_BUILD_TESTS` param-dump wiring.** The dump
  target builds with `JUCE_WEB_BROWSER=0` and compiles no editor TU, so
  `#include "PluginEditor.h"` moved from the top of `PluginProcessor.cpp` into a
  `#if JUCE_WEB_BROWSER` guard above `createEditor()`, with a
  `GenericAudioProcessorEditor` fallback. Under a normal build `JUCE_WEB_BROWSER=1` and
  the behaviour is byte-identical.
- **A tooltip renderer, because this page had none.** `applyI18n()` writes
  `data-tip-title` and `data-tip` attributes onto the bound anchors and stops there —
  the thing that reads them and paints a surface is per-plugin code, and this page had
  no `#tooltip` element, no `.tooltip` rule and no hover handler. Six bodies bound with
  no renderer would have shipped six invisible strings past three green gates, and that
  was *measured, not assumed*: with the `setupTooltips()` call commented out,
  `check-i18n` and `check-ui-labels` both still reported ALL CHECKS PASS.
- **`tests/ui_tip_render_check.js`** — the gate that actually hovers. It drives every
  anchor in both languages and asserts the tip becomes visible, that its rendered title
  and body are *byte-equal* (not "contains") to the table entry, and that its rectangle
  is inside the 600x450 frame on all four edges. It carries its own negative control: a
  2400-character body planted on a live attribute must be reported as overflowing, or
  the containment assertions are decoration.

### Changed
- The renderer is ported from O-simpleFM's `setupTooltips` — delegated on `document`
  (no anchor carries `data-tip` until `applyI18n()` has run), `pointerover`/`pointerout`
  and `focusin`/`focusout` because those bubble, `createElement` + `textContent` so
  localized copy never reaches a markup path — but it **clamps all four edges** where
  the original flips and then clamps only top and left. In a 450 px frame with
  `#autogainToggle` and `.quality-buttons` both at y=395, every tip they open is placed
  by the vertical flip, and French wraps taller.
- The tooltip is styled in this page's own vocabulary — the aged-paper fill over a 2 px
  `#3C5C1A` border that `.settings-popover` already uses — not in O-simpleFM's
  dark-on-cream.
- The gear popover is still one row. v1.3.0 has hover-help but no switch for it:
  a switch needs a persisted preference through C++ and a `data-tip-always` bypass so
  its own tip survives being switched off. The gear's tip says so rather than promising
  a control that is not there.

### Notes
- **Two of the four parameters have no `id` anywhere near them.** `MODEL` and `QUALITY`
  are rows of buttons, and the parameter is the row — `TIP_BINDINGS` uses the wrapper
  form (`['.model-button', 'tip.model', '.model-buttons']`) so the whole row, including
  the flex gaps, is the hover target. Binding one button would have left three quarters
  of it dead.
- **The seven option words stay English inside the French bodies.** MAGNETIC, TUBE,
  TRANSFORMER, DIODE, LOW, MID and HIGH are `AudioParameterChoice` options and are what
  the buttons say in both languages; a French body that translated them would send a
  reader looking for a button that is not on the page. `AUTOGAIN`'s Off/On is an
  `AudioParameterBool`'s default text, appears on no button, and *is* localized.
  `tip.lang`'s body tells the user why, so the model and quality rows do not read as a
  missed translation.
- `INTENSITY`'s unit came from the dump's `label` column (`%`). It did not have to be
  recovered from a formatter: this page renders no numeric readout for the knob at all.
- All French is a machine draft, every entry `reviewed: false`. The worklist for this
  plugin is now 15 entries — 6 tooltip, 9 label. No native speaker has read any of it.
- Zero geometry movement, before and after: `check-ui-labels` assertion 7 reports no
  non-label element moving between English and French in either driven state. The idle
  tip is `visibility: hidden`, so it enters neither the label sweep nor the geometry
  diff. No pin was needed, so none was added.
- **Known, and not this plugin's to fix: a tip that is already open does not re-render
  on a language change.** `applyI18n()` rewrites the anchors' attributes; nothing
  re-reads them for the surface currently on screen, so switching language with the
  pointer resting on `#lang-select` leaves that one tip in the old language until the
  pointer leaves and returns. That is canon behaviour, shared with all 21 already-shipped
  tooltip plugins, and a per-plugin workaround here would hide it.

### Fixed
- **A pointer click no longer leaves the tip parked over what the click just opened.**
  The renderer this stage ported opens a tip on any `focusin`, and a mouse click on a
  `<button>` focuses it — so the tip that `pointerdown` had just hidden reopened
  immediately, with the pointer still on the anchor and no further `pointerover` coming.
  Measured here: clicking the gear left its own tip covering the settings popover by
  **161 x 29 px**. Both gates stayed green throughout; `check-ui-labels` classes the surface
  as `pointer-events: none` decoration and never as a label.

  The fix is an explicit last-input-device latch cleared by any keydown.
  **`:focus-visible` is deliberately not the discriminator** — Chromium reports it false
  for a programmatic `.focus()` following a click, so a gate driving focus directly would
  measure "no tip" and record that as correct.

  `tests/ui_tip_render_check.js` now asserts **both halves separately**: a pointer click
  opens no tip, *and* a real tab-ring walk still does. Asserting only the first would let
  the feature decay into "focus never shows a tip", which passes it perfectly while
  silently removing the keyboard half of hover-help.

  **The first version of that assertion was decoration, and its negative control is what
  said so.** An earlier section of the gate leaves focus on `#gear-btn`, and clicking an
  already-focused element fires no `focusin` at all — so with the latch removed the check
  still passed. It now blurs first, and with the latch removed it fails by
  4669 px².

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
