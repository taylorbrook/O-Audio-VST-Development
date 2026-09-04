# O-Comp Changelog

All notable changes to this plugin will be documented in this file.

## [1.8.1] - 2026-09-03

The French rendering of the hover-help surface changes suite-wide (task
260903-ukp; O-Gain 1.3.3 was the tracer). PATCH: French strings and source
comments only — no parameter, range, type or state format changed.

### Changed

- **The French caption is now `Infobulles`** (feminine plural). The superseded
  rendering named the ACTION — help on hover; *infobulle* is the noun French
  DAW and OS interfaces use for the surface itself. The glossary root moved
  with it, ROOT-ONLY: `scripts/i18n-fr-glossary.js` now reads
  `'hover help': ['infobulles']` and
  `'toggle hover help': ['activer ou désactiver les infobulles']`, with the old
  rendering REMOVED rather than kept as an accepted alternate — so a plugin
  drifting back is a red G1 gate, not a silent pass.
- **Every sentence re-agreed from feminine singular to feminine plural**, not
  substituted: `cette …` → `ces infobulles`, `l’…` → `les infobulles`,
  `de l’…` → `des infobulles`, `toute l’…` → `toutes les infobulles`,
  `Une fois désactivée` → `Une fois désactivées`; the distributive `chaque …`
  → `chaque infobulle` is the one place the new term stays singular.
  Bare back-references that carried no occurrence of the old phrase — clauses
  reading *le réglage de l’aide*, *l’état de l’aide*, *son affichage ou non*,
  and the pronouns in *Lorsqu’elle est désactivée … la réactiver* — were
  rewritten too. A regex pass would have left every one of them pointing at an
  antecedent that no longer exists.
- Every changed body was read by the developer at a blocking checkpoint
  *before* it was written, so each ships `reviewed: true` legitimately and the
  repo-wide unreviewed-French TOTAL stays at 0.


## [1.8.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `ocomp.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 25..87, 176 x 62 px — byte-identical in English and French** — inside a 620 x 360 frame. The header note about `#lang-select`'s tip paint order is untouched. The switch face measures **42.00 x 18.00 px in both languages**: *On* and *Marche* both fit inside the 42 px floor.
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


## [1.7.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **18 French entries revised** against the suite glossary (`scripts/i18n-fr-glossary.js`)
  and its lint, which went from 39 findings to 0 with `--strict` exiting 0. Eight are
  terminology, eleven typography, one an agreement error and three a change of meaning.
  The visible ones: the preset bar's **SAUVER → ENREG.** (Sauver is a calque; the
  glossary's abbreviation measures 23.75px against the button's 30px content box, which
  is *narrower* than the 25.00px the calque was chosen for), the knob captions
  **RELÂCHE → RELÂCH.** and **GENOU → COUDE**, the gear's accessible name
  **Paramètres → Réglages** — which also removes a contradiction with the gear's own
  tooltip, that already said Réglages — and, throughout the hover-help, typographic
  apostrophes for straight ones, no-break spaces before every colon and semicolon and
  between every number and its unit, and a true minus sign in "−60 à 0 dB" and
  "−12 à +24 dB". Three sentences said something the English did not and were corrected:
  a transient is masculine in French, a release time does not pump (the compressor does),
  and "boutons" alone does not say *knobs* on a page that also has buttons.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so
  assistive technology reads the page in the language it is displayed in.

### Not changed

- **`reviewed: false` on all 34 French entries.** That flag records a *native speaker*
  reading the copy, and this pass is a second machine reading against a glossary and a
  lint. The `i18n.js` header records the pass; the flag still records the human, and
  `node scripts/check-i18n.js` still prints the 34-entry worklist.
- **No English copy, no keys, no bindings, no CSS.** The English strings are byte-identical
  to v1.7.0, proven by importing both revisions and comparing every `en` value.

## [1.7.0] - 2026-08-30

Hover-help, in both languages. Stage M batch M1 of the repo-wide i18n rollout.

### Added

- **Nine tooltips, English and French** — one for each of the seven APVTS parameters plus
  the settings gear and the language selector. The dump and the page reconcile exactly:
  `.planning/params.tsv` lists seven parameters and every one of them has a control here,
  so nothing is host-reachable but page-unreachable and nothing on the page is unbacked.
  Bound through `TIP_BINDINGS`, which walks `closest('.control-group')` so the hover area
  is the whole knob-plus-caption-plus-readout column rather than the 52px vine face.
- **A renderer, because the page had no way to show one.** Canon v2's `applyI18n()` writes
  `data-tip-title` and `data-tip` onto the anchors and stops; at v1.6.0 there was no
  `#tooltip` element, no `.tooltip` rule and no hover handler on this page. Authoring the
  copy alone would have shipped nine invisible strings past three green gates —
  `check-i18n` only counts bindings, `check-ui-labels` has no tooltip awareness at all, and
  `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. `setupTooltips()` is
  ported from O-simpleFM's delegated cursor-following renderer and styled in this page's own
  paper plate: `#F5E6D3`, a 1px `#8B7355` rule and `.settings-popover`'s 2/2/6 shadow.
- **`tests/ui_tip_render_check.js`** — 156 assertions against the real page at the shipping
  620 x 360, in English and again in French and back. Every binding resolves; every anchor
  SHOWS a tip; the rendered title and body are BYTE-EQUAL to the table (not "contains" — a
  `.tip-title` that kept the previous anchor's text passes a contains check); every tip
  rectangle is inside all four frame edges. `TIP_BINDINGS`, the `max-width` cap, the clamp
  margin and `setSize` are all PARSED out of the shipped files, never retyped.

### Changed

- **The focus arm is latched to the keyboard.** The reference renderer opens a tip on any
  `focusin`, and a mouse click on a `<button>` focuses it — so the tip `pointerdown` had
  just hidden reopened on top of whatever the click opened. Measured here with the latch
  removed: clicking `#gear-btn` pins a 250 x 115 tip at (236, 37) covering the settings
  popover it had just opened by **3800 px²**. `:focus-visible` is deliberately not the
  discriminator — Chromium reports it false for a programmatic `.focus()` after a click, so
  a gate driving focus directly would measure "no tip" and record that as correct. An
  explicit `lastInputWasPointer` latch, cleared by any keydown, is the same rule and is
  drivable with real events.
- The console banner and the `i18n.js` header now say v1.7.0, and the settings-popover
  comment no longer claims this plugin has no hover-help.

### Notes

- **All nine French bodies are machine drafts, `reviewed: false`.** The native-speaker
  worklist for this plugin is now 34 entries (9 tooltip bodies + 3 canvas captions + 22
  labels). No native speaker has read any of it.
- **Ranges keep the readout's decimal point, not French convention.** The bodies say
  "0.1 à 100 ms", not "0,1", because `.value-display` formats "0.1 ms" and "4.0:1" with a
  point in both languages and a tip that spells a number differently from the readout beside
  it describes a control the page does not have. Flagged in `i18n.js` so a reviewer changes
  both or neither.
- **`auto_gain` names its two faces in French inside the body** (ARRÊT / MARCHE). That is not
  a D-01 arm-1 problem: arm 1 exempts an `AudioParameterChoice` OPTION so the page and the
  host automation lane agree, and this plugin has no choice parameter at all — six floats and
  one bool. ARRÊT and MARCHE are the button's own French faces.
- **The three `canvas.*` entries keep their empty bodies and stay unbound.** They are
  `fillText` strings painted into `#envelopeCanvas`; they are captions with nowhere to live,
  not tips. The render gate asserts no empty-body entry is bound.
- No hover-help on/off toggle (an M1-wide decision), and no tips on the preset bar (out of
  M1 scope — those four controls took accessible names from their deleted `title=` at
  v1.6.0). No `tabindex` was added to `.control-group`: these knobs are mouse-drag,
  wheel and double-click-reset only, so a tab stop would add tab-order noise for a control
  the keyboard still could not turn.
- Geometry: **nothing moved.** `check-ui-labels --plugin O-Comp` is BYTE-IDENTICAL to the
  v1.6.0 baseline across all three driven states, `moved=0` before and after, and the `[8b]`
  inert-element count stays at 1. `boot-all-uis` counts are identical measured against HEAD
  and the working tree: text 25, aria-label 8, title 0, `[data-i18n]` 13. No pin was added,
  so none is claimed and none is owed a negative control.

### Changed
- **The French decimal separator is a comma.** Settled by the developer,
  2026-08-30, after two plugins in the same Stage M batch shipped opposite
  answers. `tip.attack`'s `0.1 à 100 ms` now reads `0,1 à 100 ms`.

  The reasoning that produced the point was that the readout formats with a
  point in both languages under D-03, so a body saying `0,1` beside a readout
  saying `0.1` names a control the page does not have. The decision went the
  other way: the comma is correct French and it is what all 21 already-shipped
  tooltip plugins write. **The readout keeps its point** — D-03 exempts the
  readout NODE and that has not moved — because a readout is a
  machine-formatted value and a tooltip body is prose.

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
