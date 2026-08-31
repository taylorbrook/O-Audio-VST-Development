# O-Chorus Changelog

## [1.4.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **19 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint, taking O-Chorus from 43 lint
  findings to 0. Six terminology moves, ten casing, eight typography, four
  grammar and register. The visible ones:
  - **DOSAGE → MIX** and **ÉCART → ÉTAL.** on the knob captions. *Mix* is what
    French DAWs show; *Dosage* was elegant French nobody else in the suite used.
    *Écart* was doing double duty for both Detune and Spread across 43 plugins,
    and the glossary gives it to Detune — Spread is *Étalement*, abbreviated
    here because the full word would widen the knob cell.
  - **SAUVER → ENREG.** on the Save button. *Sauver* is a calque; the French for
    saving a preset is *Enregistrer*, which needs a 78.52 px button against a
    62 px one, so the caption carries the standard abbreviation and the
    accessible name carries the full *Enregistrer le préréglage*.
  - **Typographic spacing throughout the tooltips** — a no-break space before
    every `%` and `;` and `:`, and between every number and its unit, so
    *0 à 100 %*, *10 ms* and *2 kHz* can no longer break across a line.
  - Four sentences repaired: *Plus de voix épaissit* → *épaississent*
    (agreement), *changée en jouant* → *changé pendant le jeu* (a gerund whose
    subject was the wrong noun), two verbs given the object French requires, and
    Drive's advice put back into the imperative the rest of the page uses.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Notes

- `reviewed: false` still stands on all 28 entries. That flag means *a native
  speaker read this*, and none has; this pass is a second machine reading
  against a glossary and a lint, recorded in the `i18n.js` header instead.
- Nothing moved by a pixel. Every caption node on this page carries
  `text-transform: uppercase`, so lower-casing the table is invisible on screen
  — *VITESSE* and *Vitesse* both render and measure at 41.61 px — and the two
  abbreviations were chosen against the same measured cliffs the captions were.
  `check-ui-labels` reports the same zero moved elements as v1.4.0.

## v1.4.0 (2026-08-30)

### Added

- **Hover-help, in English and French.** Every one of the eight parameters now
  has a tooltip, plus the settings gear and the language selector — ten entries,
  ten `TIP_BINDINGS` rows, each with an `en` and an `fr` `{t, b}`. Hover a knob
  cell and a surface follows the cursor with the control's name and a short
  description ending in its range.
- **The renderer that makes them visible.** v1.3.0 had no `#tooltip` node, no
  `.tooltip` rule and no hover handler anywhere on the page, so authoring the
  copy alone would have shipped ten INVISIBLE strings past three green gates.
  That is not a guess — it was measured on this plugin: with `setupTooltips()`
  commented out and everything else identical, `check-i18n --plugin O-Chorus`
  reported ALL CHECKS PASS and `check-ui-labels --plugin O-Chorus` reported ALL
  CHECKS PASSED with zero FAILs, while the new render gate reported 158
  failures. `check-i18n` assertion 2 only counts bindings, `check-ui-labels` has
  no tooltip awareness at all, and `boot-all-uis` counts `aria-label` and
  `title` and never `data-tip`.
- `tests/ui_tip_render_check.js` — 240 assertions, the seat where an invisible
  tooltip becomes loud. It drives the real page at the shipping 700 x 125 frame
  read out of `PluginEditor.cpp`, hovers all ten anchors in `en`, then `fr`,
  then `en` again, and asserts the rendered title and body are BYTE-EQUAL to the
  table (not "contains" — a stale `.tip-title` passes a contains check) and that
  the tip rectangle is inside all four viewport edges every time.

### Notes

- **THE CLAMP IS THE NORMAL PATH ON THIS FRAME, AND THE NUMBER SAYS SO.** All
  20 placements (ten anchors x two languages) overflow the naive `cursor + 16`
  offset, and 17 of the 20 are outside on BOTH sides of the flip and land on the
  SECOND clamp. A renderer that flipped once and stopped would put a tooltip
  partly off-screen at 17 of 20 anchors here.
- **All eight parameters have an EMPTY `label` in the runtime dump.** Not one
  calls `withLabel()`, so `params.tsv` carries no unit and its
  `textAtMin`/`textAtMax` are raw parameter values. Seven of the eight disagree
  with what the user reads, because the page's own formatter rescales them —
  `depth` dumps `0.00 .. 1.00` and renders `50%`; `tone` dumps `-1.00 .. 1.00`
  and renders `+0%` with a formatter-added sign. Every range in the tooltip
  bodies was recovered from the `params` array at `index.html:686-693`, and
  `rate` is the only parameter whose dumped numbers could be quoted as they
  stand.
- **Not one of the eight knobs carries an id**, so all eight bindings are
  `.knob[data-param="..."]` attribute selectors walking up to
  `.knob-container`. The only id inside a knob is on the SVG arc, and
  `.knob-vine` is `fill: none` with `stroke-width: 3` — walked with
  `elementFromPoint`, 147 of 4526 points inside the cell land on it. A tip bound
  to the id would have a hover target 3.2 % the size of the cell, and one that
  changes size with the parameter value.
- **The two chrome anchors bind BARE, with no wrapper**, because `#gear-btn`
  and `#settings-popover` share `.settings-cluster`: a wrapper walk from
  `#lang-select` would resolve to the gear's anchor and show the gear's tip.
- **The focus arm is latched to the keyboard.** A mouse click on a `<button>`
  focuses it, so an unconditional `focusin` rule re-opens the tip `pointerdown`
  just hid, on top of whatever the click opened. Measured here by deleting the
  latch: clicking `#gear-btn` left a 384 x 52 tip covering the 170 x 32 settings
  popover it had just opened by **4672 px2**. `:focus-visible` is deliberately
  not the discriminator — Chromium reports it false for a programmatic
  `.focus()` after a click, so a gate driving focus directly would measure "no
  tip" and record that as correct.
- **A `pointerdown`-only guard is not enough for a knob drag**, so `drag.active`
  suppresses a show outright. A drag long enough to cross into the neighbouring
  `.knob-container` would otherwise open that neighbour's tip mid-gesture, over
  the readout the user is dragging to reach; the `pointerover` arrives AFTER the
  `pointerdown`.
- **A claim in this version's own first draft was false, and measuring it caught
  it.** The tooltip CSS comment originally said these ten bodies "would run
  10-12 lines at O-Bass's 208 px cap and none could be shown". They run four to
  five lines there, and the tallest is 90.7 px inside a 109 px well — it fits.
  What the shipped 384 px cap buys is headroom: 64.7 px tallest, 44.3 px spare
  instead of 18.3, which is the room a native-speaker review needs to lengthen a
  sentence. The comment now says what was measured.
- **Geometry: nothing moved.** `check-ui-labels --plugin O-Chorus` is identical
  before and after in both driven states, `moved=0` both, once the `#lfo-dot`
  NOTE is set aside — that dot is animated by `requestAnimationFrame` and its
  sampled position differs between two runs of the SAME tree. The `[8b]`
  inert-element count stays at **3**, not 5: the latch plus `pointerdown ->
  hide()` means the gate's state-driver click leaves no tip open. That count is
  what moved 7 -> 9 on O-Emulator with the defect present. **No geometry pin was
  added, so none is claimed and no negative control is owed.**
- The eight `.knob-value` readouts stay English forever (D-03 binds to NODES).
  A number inside a localized tooltip body is ordinary prose, so `0 to 100%`
  becomes `0 à 100 %`.
- Every French string is a machine draft, `reviewed: false`. The
  native-speaker worklist for this plugin is now 28 entries (10 tooltip,
  18 label).

### Changed

- `CMakeLists.txt` gains the `OUARICON_BUILD_TESTS` option and the
  `ouaricon_add_param_dump(OuariconChorus ...)` call, and
  `PluginProcessor.cpp` moves `#include "PluginEditor.h"` behind
  `#if JUCE_WEB_BROWSER` directly above `createEditor()` with a
  `GenericAudioProcessorEditor` fallback. The param-dump console target builds
  with `JUCE_WEB_BROWSER=0` and does not compile the editor TU, so a
  top-of-file include breaks the link. Under a normal build
  `JUCE_WEB_BROWSER=1` and behaviour is byte-identical.
- `.planning/params.tsv` — the runtime parameter inventory this version's
  ranges were reconciled against, committed alongside the wiring that produces
  it.

### Not done, deliberately

- **No hover-help on/off toggle.** Two shipped plugins have one; the other 19
  tooltip plugins do not. Adding one means a second control in the gear
  popover, a persisted preference through C++ and a `data-tip-always` bypass so
  the toggle's own tip works when tips are off — a separate pass across 41
  plugins, not a side effect of this one.
- **No preset-bar tips.** Those four controls took accessible names from their
  deleted `title=` attributes at v1.3.0 and are self-describing.
- **The gear tip describes ONLY the language selector**, because that is all the
  popover holds. O-Tapestop's wording promises a hover-help toggle this plugin
  does not have, and a tip that lies is worse than no tip.
- **The French decimal separator is left as the suite's comma** (`0,05 à
  5,00 Hz`), matching all 21 shipped tooltip plugins, and flagged as an open
  decision: O-Comp's Stage M executor kept the readout's POINT instead, on the
  grounds that the readout prints `1.00 Hz` in both languages under D-03.
  Exactly one string on this page is affected.
- **An already-open tip does not re-render on a language change.** `applyI18n()`
  rewrites the anchors' attributes; nothing re-reads them for the surface
  currently on screen. This is canon behaviour shared with all 21 shipped
  tooltip plugins — reported, not worked around.

## v1.3.0 (2026-08-29)

### Added

- **The page speaks French.** Every visible caption on the UI is now keyed and
  carries an English and a French string: the eight knob captions, the LFO ring
  heading, the two preset buttons and the new language caption. A settings gear
  in the bottom-left corner opens a popover holding the language selector, and
  the choice is persisted with the session as a non-parameter property on the
  APVTS state tree (`uiLanguage`, read back through an `isVoid()` guard — the
  XML round-trip rebuilds every property as a `var` over the attribute STRING,
  so `isBool()`/`isInt()` would be false for every saved session).
- New `Source/ui/public/js/i18n.js`, embedded in `juce_add_binary_data` SOURCES
  **and** served from a `getResource()` branch, in the same change. A file
  embedded but not served — or served but not embedded — is a 404 that presents
  as a page stuck in English and nothing else.
- Two native functions, `getUiLanguage` / `setUiLanguage`. The page PULLS once
  at init; nothing is pushed from the editor constructor, which would race the
  WebView's load.

### Changed

- **The four native `title=` attributes on the preset bar are DELETED, not
  localized.** A native `title` renders a second, untranslated OS tooltip. Their
  existing English text moved verbatim into `data-i18n-aria` accessible names —
  no hover-help prose was invented, and this plugin still has none.
- **`.preset-action` is pinned to 62 px.** The preset bar is a shrink-to-fit
  flex row flush against the header's right edge, so an unpinned button that
  grows in French drags the arrows and the preset name with it. The pin makes
  the row's geometry language-invariant; it widens the two buttons in English
  by 23 and 25.66 px, moving the preset cluster 48.66 px left inside a header
  that has ~290 px of empty middle. Nothing else on the page moved.

### Not changed

- No parameter IDs, ranges, types, defaults or DSP behaviour. All French is
  machine-drafted and flagged `reviewed: false`; no native speaker has read it.

## v1.2.3 (2026-06-30)

### Changed

- **UI legibility — darker, slightly larger text.** The knob labels/values and section
  labels used low-contrast tan/beige tones (`#8B7355`, `#a08870`) on the cream paper
  background (`#F5E6D3`), making them hard to read. Darkened to the brown family and bumped
  each text element +1px:
  - Knob values (`1.00 Hz`, `50%`…): `#a08870` → `#5C4A32`, 9px → 10px (value box 12px → 13px).
  - Knob labels (Rate/Depth…): `#8B7355` → `#4A3B2A`, 8px → 9px.
  - Section labels (MODULATION/CHARACTER) + LFO label: `#8B7355` → `#5C4A32`, 7px → 8px, opacity 0.7 → 0.9.
  - Preset bar (name/Load/Save/nav arrows): `#4A3B2A` → `#3C2F2F`, +1px, opacity → 0.95–1.0.
  - Title: 15px → 16px.
- Pure cosmetic CSS change in `Source/ui/public/index.html`; no DSP, parameter, or state
  changes. Aesthetic (vintage naturalist / paper texture) preserved.

## v1.2.2 (2026-06-30)

### Fixed

- **WR-01 — Per-voice delay collapse at high Spread.** At moderate-to-high Spread the
  per-voice base delay went negative (voice 0 at Spread 1.0 = base 10ms − spread 15ms =
  −5ms). JUCE `DelayLine::popSample` silently ignores negative delays (reuses the last
  clamped value) and `setDelay` clamps to 0, so the affected voice collapsed toward ~0ms
  and stopped modulating symmetrically — thinner, lopsided chorusing (worst on the factory
  **Ensemble** preset, voices=8/spread=1.0). Fix: clamp each voice's modulated delay to
  `[1 sample, maxDelaySamples]` before `popSample`. No crash existed (verified against
  `juce_DelayLine.cpp` — no OOB read/NaN), so this was a quality/correctness fix.
- **WR-02 — Double delay-line push during voice-count crossfade.** During the ~50ms
  voice-count crossfade both the old-count and new-count passes ran `popSample` **and**
  `pushSample` on each overlapping voice, advancing that delay line's read/write pointers
  at 2× the real sample rate and writing the input into two adjacent buffer slots — an
  audible pitch/doubling glitch on every Voices change. Fix: unified the two passes into a
  single per-voice loop that multi-taps overlapping voices (`popSample(..., updateReadPointer=false)`
  for the first tap, `true` for the last) and pushes exactly once per voice per sample.
- **WR-03 — Tone filter unstable at low sample rates.** `updateToneFilter` computed
  `1/tan(pi·cutoff/fs)` with cutoff up to 20kHz and no Nyquist guard; at sample rates
  ≤ ~40kHz (e.g. 22.05kHz/32kHz, exercised by pluginval's SR sweep) `tan()` blew up or went
  negative, pushing the biquad poles outside the unit circle → NaN/Inf output. Fix: clamp
  cutoff to `0.49 × Nyquist` before computing coefficients.

### Notes

- Root causes from the 2026-06-30 deep code review (`O-Chorus-CODE-REVIEW.md`, 3 warnings).
- No parameter IDs, ranges, or state format changed — presets and sessions load unchanged.

## v1.2.1 (2026-02-25)

### Added

- Compile-flag gated licensing module (OUARICON_LICENSING)
  - OuariconLicense manager in PluginProcessor
  - License overlay UI in PluginEditor (activation gate)
  - License status listener toggles WebView visibility
  - OFF by default for local dev builds

## v1.2.0 (2026-02-08)

### Added

- **Preset system** via Ouaricon preset-manager module
  - Factory/user preset persistence (JSON-based, stored in ~/Library/O-Chorus/Presets/)
  - Preset navigation (prev/next arrows) with dropdown menu
  - Save/load preset dialogs (native file chooser)
  - DAW session state includes current preset name
  - Program API (getNumPrograms/setCurrentProgram) for DAW preset browsing
- **6 factory presets:**
  - **Classic** — Vintage 2-voice chorus (0.5 Hz, subtle)
  - **Lush** — Rich 6-voice ensemble (slow, deep, wide spread)
  - **Shimmer** — Bright sparkling 4-voice chorus (2 Hz, bright tone)
  - **Ensemble** — Dense 8-voice string ensemble (full spread, full width)
  - **Vibrato** — Pure vibrato effect (3 Hz, 100% wet, single voice)
  - **Warm** — Warm analog-style 3-voice chorus (dark tone, high drive)

## v1.1.0 (2026-02-08)

### Added

- **Spread parameter** (0.0–1.0): Offsets each voice's base delay time across ±15ms range
  - At 0%: All voices share the same base delay (original behavior)
  - At 100%: Voices are distributed symmetrically across a 30ms delay range
  - Makes the Voices parameter audibly meaningful — more voices = richer, thicker sound
  - Inspired by classic multi-voice chorus designs (Juno-60, Dimension D)

### Root Cause

- The Voices parameter previously had minimal audible effect because all voices shared
  the same 10ms base delay time. Only LFO phase offset and tiny depth variation (0.85–1.15x)
  differentiated voices, producing nearly identical tonal results regardless of voice count.

## v1.0.1 (2026-02-08)

### Changed

- Renamed UI display title from "O-Chorus" to "Ouaricon Chorus"

## v1.0.0 (2026-02-08)

### Initial Release

- 8-voice BBD-style chorus engine with Lagrange3rd interpolated delay lines
- 7 parameters: Rate, Depth, Voices, Width, Tone, Mix, Drive
- Per-voice LFO phase offset with seeded depth variation for organic modulation
- Tanh saturation with asymmetric drive for analog warmth
- One-pole tone filter (2kHz-20kHz range)
- Equal-power stereo panning with width control
- Voice count crossfade (50ms) for click-free transitions
- Naturalist-styled WebView UI (700x250) with paper texture background
- LFO ring animation with frame-rate-independent timing
- Knob interaction: vertical drag, shift for fine control, double-click reset, mouse wheel with gesture brackets
- Cross-platform: VST3 + AU, WebView2 static linking for Windows
