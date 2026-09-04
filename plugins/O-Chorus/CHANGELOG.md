# O-Chorus Changelog

## [1.5.0] - 2026-09-03

Simplified Chinese. O-Chorus is the pilot for the zh-Hans rollout — every
structural question the remaining 42 plugins will ask was answered here first.

### Added

- **A third language, `zh-Hans`.** All 28 entries — 18 labels and 10 tooltips
  with a title and a body each — plus the endonym 简体中文 in the selector,
  written as the numeric references `&#31616;&#20307;&#20013;&#25991;` to match
  the existing `Fran&ccedil;ais` convention. The stored session/preset property
  round-trips the BCP-47 tag through a three-branch, **pure-ASCII** C++ codec;
  not one Han character exists anywhere under `Source/**/*.{h,cpp}`.
- **Terminology from the suite glossary, not from this file.** Every English
  string that is a `TERMS` key takes that term's root rendering, settled across
  552 shared strings in Stage 1 — *Rate 速率, Depth 深度, Voices 复音数, Spread
  扩散, Width 宽度, Tone 音色, Mix 混合, Drive 驱动, Load 载入, Save 保存*. Lint
  rule Z5 holds the file to them. *LFO* ships as the English token keyed
  `sameAsEn`, so a human still has to agree with it rather than it being
  silently exempted.

### Changed

- **Character budgets: none needed, and the measurement is the point.** The
  three measured O-Chorus cells are unchanged — *depth* 62 px wrap cliff / 10 px
  = 6 characters, *save* the same 6, *spread* 50 px gate cliff / 10 px = 5 — and
  every Chinese caption fits at **2 characters**. Every one is *narrower* than
  its English original, by 0.3 to 19.7 px in the real `.knob-label` node. This is
  the exact inverse of French, where three of eight captions had to be
  abbreviated. **Chinese buys width and spends height.**
- **Three line-height pins, and one width pin.** `line-height: normal` is the
  font's own metrics, and Han faces carry taller ones: 26 elements moved on the
  first Chinese run. Each pin is the node's measured English line box over its
  font size, written unitless so it is font-independent:
  - `.knob-label` — **EN 10.00 px / 9 px → 1.1111**. Closed 24 of the 26: its
    growth to 13.00 px grew `.knob` and `.knob-container` and pushed
    `.knob-value` down 3 px on all eight knobs.
  - `.preset-action` — same 10.00 px / 9 px ratio (the LOAD/SAVE buttons went
    14.00 → 17.00 px border-box).
  - `.settings-label` — same ratio. **Assertion 7 never named this one and could
    not:** the settings popover is `hidden` at rest, so the gate cannot measure
    it. Forced open it is the identical defect. It happens not to propagate today
    only because `.settings-select` is 16 px and taller than both — luck, not
    design, since the row is `space-between` with a `nowrap` caption.
  - **The 26th mover was not a line height.** `#lang-select` measured 65 px in
    English and French and 64 px in Chinese. The three endonyms are
    language-*invariant* (27.501 / 30.489 / 36.792 px in every pass), so the
    widest option cannot explain it — with `appearance: auto` Chromium derives
    the control's intrinsic width from the **selected** option's font run, and
    the Han endonym resolves through PingFang SC a pixel narrower. Pinned to
    65 px, its existing English intrinsic, so English and French are unchanged
    and only the Chinese pass moves.

  No global `line-height` was added: a global one moves English geometry, which
  would be a regression rather than a fix.
- **CJK font tail on five of the page's eight `font-family` declarations**
  (`, 'PingFang SC', 'Microsoft YaHei', sans-serif`). The set was **measured** —
  `getComputedStyle().fontFamily` read on every node that holds or can receive a
  Han codepoint — not derived from the `[data-i18n]` list, which would have
  missed two of the five: `.tooltip` carries no keyed attribute and is filled
  from `data-tip` at hover time, and `.settings-select` holds the only Han in the
  markup. The other three took the tail: `.container` (the inherited stack),
  `.preset-action`, `.settings-label`. **The three omitted, each justified by
  what it renders:** `.preset-nav` (the glyphs U+25C0/U+25B6, whose names live in
  `aria-label` and are never rendered text), `.preset-dropdown-item` (preset
  names are the JSON filenames on disk, ASCII by contract), and `#gear-btn` (the
  single gear glyph U+2699; its tip paints in `#tooltip`). Latin still resolves
  to Garamond first, so English geometry is unmoved.
- **`tests/ui_tip_render_check.js` drives the table's own `LANGUAGES`** instead
  of a hard-coded `['en','fr','en']`, and fails rather than falling back to a
  default pair. Re-run unchanged it would have passed *vacuously* for Chinese —
  and it is the only gate that measures a tooltip against the 125 px frame.
  A second hard-coded pair was found below the first: tip heights were recorded
  only while `drivenStates.length <= 2`, so the Chinese pass rendered and
  asserted correctly while recording nothing. Its assertion 5 now gates on the
  pass **differing** from English rather than *growing*: French wraps to more
  lines, Chinese to fewer (6 tips shrank, 0 grew), so a growth assertion would
  have failed a correct Chinese table.

### Quality — read this before trusting the Chinese

- **The ship bar is an independently back-translated draft, and there is NO
  NATIVE CHINESE READER on this project.** Nobody who reads Chinese as a first
  language has looked at any string here. All 28 entries are at the second of
  three review levels: drafted, then read back against the English triple by
  triple, all 38 rows. The third level — native review — remains open and is not
  a gate. This is a *disclosed* quality level, not a hidden one; the lint prints
  the count below the bar on every run.
- **What made the back-translation independent.** The batch is emitted with the
  English deliberately withheld, and the reverse pass was run by a different
  model in a fresh session with the row **ids blinded** to `r01..r38` — an id
  like `label.depth` leaks the English word it is meant to recover. Both
  provenance strings are recorded so the bar is auditable:
  - forward: `claude-opus-5 gsd-executor forward draft, 2026-09-03`
  - reverse: `claude-sonnet-5 independent reverse pass, blind row ids r01-r38
    rejoined by orchestrator, 2026-09-03`

  The ingest tool refuses a provenance that is missing or byte-identical to the
  forward one; **both refusal shapes were fired deliberately as positive controls
  before the real run was trusted**, because a refusal that never fires proves
  nothing.
- **38 triples read: 24 exact, 14 accepted as synonyms, 0 corrected.** The
  lexical score is a sort key, not a verdict — the lowest-scoring triple of all
  38 (*Voices → 复音数 → "Voice Count"*, score 0.00) is correct, because 复音数
  states the count sense the English leaves implicit and the parameter is
  literally an integer 1 to 8. Each accepted drift and its reason is recorded in
  the `i18n.js` header.
- **≤9 px LEGIBILITY TIER — a disclosed limitation.** Han glyphs carry far more
  stroke detail into the same em than Latin, and at or below 9 px they are at the
  legibility floor. Five of this page's localized nodes render at 9 px. The suite
  **ships at parity size anyway**, because raising the Chinese size would move
  English geometry and break the zero-shift guarantee the gates enforce. Chinese
  users on this page read captions at the floor. Known, accepted, not hidden.
- **Risk A5 was NOT closed, and is not silently dropped.** The research doc's
  `line-height: normal` +30% figure was reconfirmed on the real page in Chromium
  (10.00 → 13.00 px at 9 px, exactly the predicted row), but the re-measurement
  **in the shipped WKWebView was not taken because it cannot be**: this plugin's
  WebView bridge exposes twelve native functions and no `evaluateJavascript`
  path, so nothing can read a computed style from inside the host without
  shipping a debug hook in a release build. The figure remains a
  headless-Chromium number. Recorded in `research/i18n-zh-hans-localization.md`
  §3.4 and its A5 risk row.

### Known issues

- **The lint's Traditional-only set and the glossary contradict each other on
  像 (U+50CF).** The set is derived from OpenCC as
  `keys(TSCharacters) \ keys(STCharacters)`, and that difference contains 像 — a
  standard simplified character, and the one the glossary's own root rendering
  for *pan* uses (声像). Any plugin with a Pan control will have rule Z5 *require*
  a rendering that rule Z3 then *flags*. Two drafts here tripped it and were
  reworded to 声场 / 立体声场, which is legitimate for a chorus but is a route
  around the defect, not a fix — a plugin with a Pan knob cannot reword past it.
  Reported for the rollout; not fixed here.
- **`tip.language`'s English and French bodies still name two of three
  languages** ("English or Français" / "English ou Français"), authored when the
  selector had two options. The Chinese body names all three, and the
  back-translation surfaced the mismatch. Correcting the other two means editing
  a French string a human has already signed off, which needs a French review
  pass this release does not carry.

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
