# Changelog

All notable changes to O-Bassoon will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `obsn.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 37..103.59, 190 x 66.59 px — byte-identical in English and French** — inside a 900 x 600 frame. The switch face grows 42.00 -> 45.30 px for *Marche*, leftward into the panel's own slack; `check-ui-labels` [7] reports 0 non-label elements displaced.
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


## [1.2.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed

- **item 36 — Breath knob tooltip:** the tip said a MIDI breath controller
  (CC2) "takes over while it is moving and hands control back half a second
  after it stops". `BassoonVoice.cpp:167` composes the breath target as
  `lastUiBreath * cc2Normalised` — a product, not a takeover: with the knob at
  0 a breath controller does nothing, and with CC2 at 0 the knob does nothing.
  The tip now says what the breath meter's accessible name (`aria.breathMeter`,
  "UI breath × CC2") already said — the effective breath is knob × CC2, the
  knob sets the ceiling, CC2 scales it, both must be above zero — and keeps
  the half-second idle window after which the knob alone applies again
  (`BassoonVoice.cpp:202-207`). French rewritten in the same commit
  (`reviewed: false` — a meaning change). The i18n.js header note that
  repeated the "takes over" claim is corrected too.

## [1.2.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **16 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and lint (`scripts/i18n-fr-lint.js`): 5
  terminology, 6 typography, 4 grammar/idiom, 1 restored meaning. The lint went
  from 26 findings to 0 and `--strict` exits 0. The two most visible: the
  Release knob's caption is **Relâch.** rather than *Relâche* — the suite's
  settled term is *Relâchement*, whose 79.81 px does not fit this caption's
  78 px box, while the listed abbreviation's 45.59 px is narrower than both
  *Relâche* and English *Release* — and its tooltip title now spells
  **Relâchement** out in full. Typographically, 26 no-break spaces landed
  before every colon and semicolon and between every number and its unit
  (*0 à 10 Hz*, *2000 ms*, *100 cents*), which is French typographic practice
  and what the rest of the suite now does.
- **The attack envelope's tooltip names the textbook ADSR stages** — *étage de
  déclin* / *étage de maintien* rather than *décroissance* / *palier* — and the
  attack-character tooltip says *filtrée en passe-bas*, which is what the
  600 Hz one-pole filter on the soft onset actually is. The vibrato-depth
  tooltip regained the English's "bends the pitch", which the first draft had
  dropped.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

English copy, keys, tip bindings, exemptions, markup and CSS are unchanged, and
every French entry stays `reviewed: false` — no native speaker has read them
yet. Gates: `check-i18n` ALL CHECKS PASS; `check-ui-labels` byte-identical to
its pre-change baseline with 0 non-label elements moved in either language;
`tests/ui_tip_render_check.js` 198 PASS / 0 FAIL with all three negative
controls firing; `boot-all-uis` 43/43 clean, 0 DEAD bindings.

## [1.2.0] - 2026-08-30

Hover-help, in both languages — and the renderer that makes it visible, because
this page had no way to paint one.

### Added

- **Twelve tooltips in `Resources/ui/js/i18n.js`** — all ten APVTS parameters
  plus `#gear-btn` and `#lang-select` — each with an English and a French
  `{title, body}`, every French entry `reviewed: false`. Every body ends with
  the control's range and unit, taken from the runtime parameter dump
  (`.planning/params.tsv`), and every body describes THIS model rather than a
  generic control: the vibrato freezing into a fixed detune at 0 Hz, CC2's
  half-second takeover window, `tone` touching only partials 6–16, the
  attack-character snapshot at note-on, the missing decay and sustain stages,
  the 2.5 s modal tail a short Release truncates, and the release-tail-first
  voice stealing.
- **A hover-help RENDERER, in the same commit as the copy.** v1.1.0 had no
  `#tooltip` element, no `.tooltip` rule and no hover handler, and canon v2's
  `applyI18n()` only writes `data-tip-title` / `data-tip` onto the anchors. The
  copy alone would therefore have shipped twelve invisible strings past three
  green gates. `setupTooltips()` is ported from O-simpleFM's delegated,
  cursor-following family — delegated on `document`, `pointerover`/`pointerout`/
  `focusin`/`focusout` because those bubble, a child-boundary guard so the tip
  does not flicker inside a `.knob-control`, `createElement` + `textContent`
  never `innerHTML`, and a flip-then-clamp on all four edges at an 8 px margin.
  Styled in this page's own naturalist vocabulary: the `.settings-popover`
  plate, with the title line in `--green-dark`.
- **A last-input-device latch on the focus arm.** A mouse click on a `<button>`
  focuses it, so the reference renderer's unconditional `focusin` rule leaves a
  tip parked over whatever the click just opened. Measured here with the latch
  removed: clicking `#gear-btn` pins a 260 × 137 tip across the settings popover
  by **5280 px²**. `:focus-visible` is deliberately not the discriminator —
  Chromium reports it false for a programmatic `.focus()` after a click, so a
  gate driving focus directly would measure "no tip" and record that as correct.
- **`tests/ui_tip_render_check.js`** — 198 assertions at the shipping 900 × 600
  frame. Every binding resolves; every anchor SHOWS a tip when a DESCENDANT is
  hovered (which is what exercises the delegated `closest()` walk); the rendered
  title and body are BYTE-EQUAL to the table, not "contains"; the rect is inside
  the frame on all four edges and within the `max-width` cap; then the whole
  sweep again in French and back. `TIP_BINDINGS`, `setSize` and the `max-width`
  cap are all PARSED from the shipped files, never retyped.

### Fixed

- **The settings popover was painted UNDERNEATH the tab bar, and had been since
  it shipped in v1.1.0.** `body` is `display: flex`, so `.header-bar`,
  `.tab-bar` and `.tab-content` are flex items — and `z-index` applies to a flex
  item at `position: static`, so each opened its own stacking context.
  `.header-bar` and `.tab-bar` both carried `z-index: 10`, a tie broken by
  document order, so the tab bar painted over everything inside the header,
  including `.settings-popover` — whose own `z-index: 21` is scoped inside the
  header's context and cannot reach out of it. Measured at v1.1.0:
  `document.elementFromPoint` at the popover's top-left AND at `#lang-select`'s
  centre both returned `.tab-btn`, and only the bottom 3 px of the 40 px panel
  was reachable. **The language selector — the only control the entire i18n
  feature adds — was unclickable from the moment it shipped.** `.header-bar`
  moves to `z-index: 30`; the two bars do not overlap, so nothing that was
  visible stops being visible and no rectangle changes. No gate saw this:
  `check-ui-labels` drives a "settings popover open" state and compares rects,
  and a rect is unchanged by paint order. It surfaced only when the new render
  gate tried to HOVER `#lang-select` and got a `pointerover` on `.tab-btn`.

### Notes

- **No hover-help on/off toggle, and the gear's tip says so.** Two shipped
  plugins have one; nineteen do not. The popover keeps exactly the language
  selector it had, and `tip.gearBtn`'s body describes only that — a tip that
  promises a control the plugin does not have is worse than no tip.
- **No `tabindex` was added to `.knob-control`.** The knobs are mouse-drag only
  and have never been keyboard-operable, so ten new tab stops would be ten stops
  for controls the keyboard still could not move — and a tip popping open
  mid-drag. `#gear-btn` and `#lang-select` are natively focusable and reach the
  focus arm through `closest()`.
- **Units for four parameters were recovered from the page's own formatter, not
  invented.** `breath`, `tone`, `attack_character` and `voice_count` carry an
  empty `label` in the dump; `PARAMS` in `index.html` declares `unit: ''` for
  all four (lines 813–815, 818) and `formatValue` (line 907) appends nothing, so
  they are genuinely unitless and the bodies say "0 to 1" and "1 to 16 voices".
  `vibrato_depth`'s APVTS label is `" cents"` while the readout renders `" c"`;
  the body spells it out, because a tooltip is where the abbreviation gets
  explained.
- **Geometry: zero movement.** `check-ui-labels --plugin O-Bassoon` is
  BYTE-IDENTICAL to the v1.1.0 baseline, in all three driven states, `moved=0`
  before and after — the surface is `position: fixed` + `visibility: hidden` +
  `opacity: 0` at rest, and that gate's visibility predicate rejects all three.
  `boot-all-uis` reads `text=35 aria=6 title=0 i18n=26` before and after,
  unchanged. No geometry pin was added, so none is claimed; the one CSS change
  is the `z-index` fix above, and reverting it ALONE re-breaks the render gate
  with two FAILs.
- **Both negative controls were run in both directions.** Removing the focus
  latch makes `[NC-2a]` fail by 5280 px² while `[NC-2b]`, the keyboard half,
  stays green — which is what makes them independent rather than one assertion
  counted twice. Removing the gate's `blur()` before the click makes `[NC-2a]`
  pass with the latch still removed, so that one line is the whole reason the
  assertion can fail at all. An over-long body planted in the served `i18n.js`
  is reported as leaving the frame, and the restore is proved byte-identical by
  hash.
- **All twelve French tooltip bodies are machine drafts.** The native-speaker
  worklist for this plugin is now 44 entries (12 tooltip, 32 label).
- **The Tuning tab still stays English for a French user** — every caption in it
  belongs to the shared `scala-tuning-engine` module. `tip.langSelect`'s body
  now says so, rather than leaving a user to read it as a bug.

## [1.1.0] - 2026-08-28

The PAGE speaks French, not only a tooltip — because this plugin never had a
tooltip.

### Added

- **31 label keys and 5 accessible-name keys in a new `Resources/ui/js/i18n.js`**,
  covering the header strip, the three tabs, the four section headings, all ten
  knob captions, the Soft/Tongued end-label pair, the whole About card, and the
  one JS-written string on the page.
- **A settings popover in the header strip**, carrying the language selector.
  One row: this plugin has no hover-help to switch on or off. It opens
  downwards, dismisses on an outside `mousedown` and on Escape, and its width is
  pinned so its own rectangle is language-invariant.
- **A `getUiLanguage` / `setUiLanguage` native-function pair and session
  persistence.** `uiLanguage` is an `std::atomic<int>` behind a two-function
  string codec, saved as a readable `"en"` / `"fr"` property on the APVTS state
  tree — deliberately NOT an `AudioParameterChoice`, so it never appears in a
  DAW automation lane and no preset can change which language somebody reads
  their plugin in.
- **`plugins/O-Bassoon/tests/i18n-states.json`**, driving the About tab and the
  settings popover so their labels are measured rather than skipped.

### Changed

- **Every native `title=` is DELETED** (contract §4). All three were the only
  help their element had, so each text moved to `data-i18n-aria` unchanged —
  the vibrato dot, the breath meter and the voice-dot row. **No hover-help prose
  was invented**; authoring that copy is a later stage. `TIP_BINDINGS` and
  `I18N` are both empty, which `check-i18n` assertion 2 reports as
  "0 tip(s) bound" rather than passing silently.
- **The tuning-panel load-failure notice is built with `createElement` +
  `setLabel`** instead of an `innerHTML` string. It now declares its own key and
  becomes a `[data-i18n]` element, so a language switch after the failure
  re-renders it instead of stranding it in the previous language.
- **Two nodes were SPLIT** (contract §5). `Version 1.0.0` became a keyed caption
  beside an unkeyed number, so the shipping version never sits inside a
  translated string; `Made by <link>` had its caption keyed in its own span.
  Four section headings gained an inner `<span>` so `applyLabel`'s `textContent`
  write cannot delete the sibling it shares its flex row with.

### Notes

- **The `.knob-label` cap is the only tight box on this page** —
  `max-width: 78px` with `nowrap` + `ellipsis`, which clips SILENTLY. Every
  caption was measured as rendered at 900 × 600. The widest French is
  `PROFONDEUR` at 72.8px, 5.2px inside the cap. `Attack Char` (73.0px) became
  `Caractère` (62.0px) rather than the literal `Car. attaque` (76.2px): 1.8px of
  margin is inside the range where a Windows/WebView2 font metric decides
  whether a caption ellipsises, and Windows metrics are hardware-blocked.
- **Six of the visible labels SHRANK in French** rather than growing.
- **Geometry.** English v1.0.0 → v1.1.0: 1 of 121 elements moved — the header
  subtitle slid 32px left for the gear cluster — and the document scroll extent
  is unchanged at 900 × 600. English → French at a fixed frame: **0 non-label
  elements moved**, measured with a full, untruncated element path.
- **The About blurb is authored to the English LINE COUNT**, not merely
  translated. A shorter first draft shrank the card by one 19.4px line and
  pulled the byline up; the geometry diff caught it.
- **The Tuning tab stays English for a French user.** Every caption in it
  belongs to the shared `scala-tuning-engine` module, referenced by path from
  `CMakeLists.txt` rather than copied, so localizing it is a cross-plugin change
  and a local edit would be reverted by `/module-upgrade`.
- All French is machine-drafted and flagged `reviewed: false`. No native speaker
  has read it.
- No parameter IDs, ranges, types, DSP behaviour or preset content changed.
