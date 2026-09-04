# Changelog — O-Emulator

All notable changes to O-Emulator are documented here.

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
  `oemu.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 322..383, 164 x 61 px — byte-identical in English and French** — inside a 620 x 430 frame. The switch face measures **42.00 x 18.00 px in both languages**: *On* and *Marche* both fit inside the 42 px floor.
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


## [1.2.1] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **9 of 22 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint, which went from 26 findings to
  3. Terminology: **Dosage → Mix** on the Mix caption and its tooltip title —
  the suite settles on *Mix*, which is what every French DAW shows and is the
  same width as the English, so nothing moved; *fondu* → **fondu enchaîné** for
  the console crossfade; *lit de bruit* → **bruit de fond**, a calque replaced
  by the term. Typography: 14 straight apostrophes became **typographic ’**,
  and **18 no-break spaces** went in before `%` and `:` and between every
  number and its unit (`30 ms`, `15 cents`, `80 %`) so a French tooltip can no
  longer break a value away from its unit. Grammar and idiom: the Mix tip said
  the hiss disappeared *avec elle*, binding it to the input — which is the one
  thing that does **not** disappear; the Crush tip's *pas de quantification*
  read as a negation until its articles were restored. Meaning: the Console tip
  had lost *d'échantillonnage* from "fixed internal sample rate", and the Reverb
  tip had lost *de console* from "available in every console mode", which is the
  whole point of that sentence.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Not changed, and why

- **Broyage** stays for Crush. The glossary's *Écrasement* measures 82.45 px in
  a 60.00 px knob column and slides all four knobs; the glossary lists no
  abbreviation and Stage N does not invent one. **Sûr ?** stays for Confirm?
  for the same reason — *Confirmer ?* is 58.59 px on one line in a 49.00 px
  pinned button. Both are reported so the glossary can grow.
- English copy, keys, tooltip bindings and CSS are untouched; `reviewed: false`
  still stands on all 22 entries, because no native speaker has read them.

## [1.2.0] — 2026-08-30

### Added

- **Hover-help, in both languages.** Every one of the five parameters plus the
  gear and the language selector now opens a tooltip on hover or keyboard
  focus, in English or French, switching live with the language selector.
  Seven `I18N` entries and seven `TIP_BINDINGS` in
  `Source/ui/public/js/i18n.js`; French drafted and `reviewed: false`
  throughout.
- **A tooltip renderer, because there was none.** Canon v2's `applyI18n()`
  writes `data-tip-title` and `data-tip` onto the bound anchors and stops
  there — the code that reads those attributes and paints a surface is
  per-plugin, and this page had no `#tooltip` element, no `.tooltip` rule and
  no hover handler. Authoring the copy alone would have shipped seven
  invisible strings past three green gates: `check-i18n` sees bindings and is
  satisfied, `check-ui-labels` has no tooltip awareness at all, and
  `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. So the
  renderer lands with the copy: `setupTooltips()` in the inline module, ported
  from O-simpleFM's delegated cursor-following family, with a `.tooltip` rule
  in the page's own paper-and-brown-ink vocabulary and a title line in
  `--accent-dark` that re-themes with the console.
- **`tests/ui_tip_render_check.js`** — the gate that can actually see a painted
  tip. Drives the real page at the shipping 620 x 430 frame, hovers all seven
  anchors in both languages, asserts the rendered title and body are
  BYTE-EQUAL to the table (not "contains" — a stale title passes a contains
  check), and asserts the rectangle is inside the frame on all four edges. Two
  negative controls: an over-long body planted in the served copy must make
  the frame assertion report the overflow (it does — 250 x 485 at y=8, bottom
  edge −63), and the focus latch below must not have killed the keyboard path.

### Changed

- **Focus opens a tip only from the keyboard.** The O-simpleFM renderer shows
  a tip on any `focusin`, and a mouse click on a `<button>` focuses it — so
  clicking the gear left its own 250 x 115 tip pinned at (320, 284) directly
  across the settings popover the click had just opened, and it stayed until
  focus moved. Measured, not assumed: it also turned the surface into a
  visible element inside `check-ui-labels`' state sweep, whose `[8b]`
  inert-element count went 7 → 9 in both driven states. A latch on the last
  input device (`lastInputWasPointer`, cleared by any keydown) gates the focus
  arm. `:focus-visible` was rejected as the discriminator: Chromium reports it
  false for a programmatic `.focus()` after a click, so a gate driving focus
  directly would measure "no tip" and record that as correct.

### Notes

- The preset bar gets no tips. Its four controls took accessible names from
  their deleted `title=` attributes in v1.1.0 and are self-describing; adding
  hover-help there is polish, not scope.
- There is still no hover-help on/off toggle, and the gear tip says so — it
  describes the language selector and nothing else, because a tip that
  promises a control the plugin does not have is worse than no tip.
- Geometry is unchanged. `check-ui-labels --plugin O-Emulator` produces output
  byte-identical to the v1.1.0 baseline: the surface is `position: fixed`,
  `visibility: hidden` and `opacity: 0` at rest, and that gate's visibility
  predicate rejects all three, so an unshown tip is neither measured nor
  swept for text. No geometry pin was added, and none was needed.
- French is machine-drafted. 22 unreviewed entries now (7 tooltip, 15 label);
  no native speaker has read any of it.

## [1.1.0] — 2026-08-28

### Added

- **The page speaks French.** A `Langue` selector in a new settings popover,
  bottom-right, switches every caption, heading and accessible name between
  English and French. The choice persists with the session. Fifteen keys in a
  new `Source/ui/public/js/i18n.js`; canon v2 of the shared i18n runtime block
  lives in the inline module and is byte-compared against
  `scripts/i18n-canon.js` by `check-i18n` assertion 6.
- **A `getUiLanguage` / `setUiLanguage` native pair**, and the language on the
  session state. It is deliberately NOT an `AudioParameterChoice`: it must not
  appear in a DAW automation lane, and a preset must not be able to change
  which language somebody reads their plugin in. It rides the state XML as a
  plain attribute beside `pluginVersion`, stamped on the document rather than
  on the live tree — `getStateAsXml()` has already taken its `copyState()`
  snapshot by the time it returns — which also keeps it out of `savePreset()`'s
  parameter sweep entirely. Read back through an `isVoid()` guard, because the
  XML round-trip rebuilds every property as a `var` over the attribute STRING
  (`critical_valuetree_xml_roundtrip_loses_type`).

### Changed

- **The delete button's two faces are now KEYS, not attributes.** `data-confirm`
  is gone and the armed face goes through `setLabel(btn, "ui.confirm")`. An
  attribute holds one string, so switching language while the button was armed
  would have restored the English "Confirm?" and left it there.
- **The three preset buttons are pinned to a fixed width** — `#preset-save` 38,
  `#preset-load` 39, `#preset-delete` 51, with the 7 px side padding moved into
  the width. See the geometry note below; this is the only pin in the commit.

### Not done, deliberately

- **No hover-help was authored.** This plugin had no `data-tip` and no native
  `title=` at v1.0.1, so there was no tooltip copy to move and none is
  invented — that is Stage M's job. `I18N` and `TIP_BINDINGS` are both empty,
  which `check-i18n` assertion 2 reports as "0 tip(s) bound" rather than
  passing silently.
- **The five console captions stay English**, with reasons in `I18N_EXEMPT`.
  SNES / PS1 / NES / Genesis are the `console` `AudioParameterChoice` option
  strings byte for byte, so D-01 arm 1 exempts them: a DAW lane reading
  `Genesis` beside a page reading `Mega Drive` — which is what that console was
  actually called in France — is a bug report, not a localization.
- **`#consoleInfo`'s spec line stays English**, on two independent arms: every
  one of the five carries a number and a unit, and it is a readout node, which
  is never a `[data-i18n]` element. What that costs a French reader is
  "Gaussian" and "wave"; translating them means splitting a centred one-line
  readout into four keyed spans, which is a markup change to a readout.

### Geometry — measured at 620 x 430, rendered, not guessed

**The header is 162 px OVER-FULL in ENGLISH, and it already was at v1.0.1.**
`.hdr` is 570 px of content holding three flex children whose max-content
widths total 732.28. `.preset-band` cannot shrink, so the whole overflow lands
on `.wordmark` — which renders "❦O-" / "EMULATOR" on TWO LINES — and on
`.hdr-right`, which wraps `.plate` to three lines. Both overflow the 48 px
header upward. **This is a pre-existing English layout defect, reported and NOT
fixed here**; it is also why a pin was unavoidable, because `.hdr-right`'s width
is exactly `570 - 159.66 - bandWidth`.

English before vs after: **2 elements moved, both of them `[data-i18n]` labels**
(`#preset-load` dw −1.48, `#preset-delete` dw +1.59, dx ≤ 1.73). 7 added, the
gear cluster. `.preset-band` 284.14 → 284.00 and `.hdr-right` 126.20 → 126.34,
both under the 0.5 px tolerance, so `.wordmark`, `.brand`, `.plate`,
`#preset-name` and both nav arrows did not move at all. Document scroll extent
620 x 430, unchanged.

French vs English: **2 elements moved, both of them the label boxes themselves**
(`label.crush`, `label.mix`). Assertion 7 reports zero non-label elements moved
in all three states.

    .ctl-label   Crush    42.69 -> Broyage 59.17   in a 60.00 column
                 Age      27.39 -> Âge     27.39   IDENTICAL
                 Reverb   50.17 -> Réverb  50.17   IDENTICAL
                 Mix      27.06 -> Dosage  51.39   in a 60.00 column
    .preset-btn  Save     22.25 -> Enreg.  32.47   in a 36 px content box
                 Load     24.48 -> Ouvrir  33.81   in a 37 px content box
                 Delete   33.41 -> Suppr.  30.38   in a 49 px content box
                 Confirm? 44.75 -> Sûr ?   26.23   same box, ARMED, one line
    .plate       widest line 91.91 -> 94.45, THREE lines in both languages

Two of the eight SHRINK or hold exactly, which is the half a clip check is
blind to. `.ctl` is shrink-to-fit around a 60 px knob, so a caption over 60 px
would slide all four columns; **Broyage's 0.83 px of clearance is the tightest
margin in this plugin** and is a margin against the knob, not against a clip.

**Negative controls — three, all fired.**

- Reverting the three width pins alone, French left in place, fails assertion 7
  in all three states: `.preset-band` dw +16.5, `.hdr-right` and `.brand` both
  dx +16.5 dw −16.5. With the delete button ARMED it is worse — "Sûr ?" WRAPS in
  the unpinned 33 px box (a space before a French "?" is a break opportunity)
  and the band grows 4 px taller, moving both nav arrows and the name plate.
  **The pin is not decoration.**
- Lengthening one French knob caption past the 60 px column ceiling moves 16
  non-label elements, so the diff genuinely sees that ceiling.
- Stripping one `data-i18n` key fails `check-i18n` assertions 10 and 15 by
  name, so the coverage sweep is live on this plugin rather than vacuous.

That wrap is also why the armed face is recorded at 26.23 and not the 17.69 a
first pass produced: 17.69 was the widest LINE of the wrapped string measured in
the wrong box — a confidently wrong number that was too SMALL.

### Verification

`check-i18n --strict-v2` 34/34, `check-ui-labels` 75/75 across three states with
9 of 9 keyed elements visible, `boot-all-uis` 41/43 clean (identical to the
pre-change baseline; O-Bowed and O-Reed fail on an unrelated pre-existing
`Unexpected token 'export'`), and the plugin's own render harness ALL PASS —
including all three cross-version digest anchors still matching their v1.0.1
values (`2798e18f`, `8742db25`, `7466614b`), so **no DSP behaviour changed**.
`auval` PASS. All 15 French strings are machine drafts, `reviewed: false`.

No parameter IDs, ranges, types or DSP behaviour changed. No `setVisible`
anywhere in `Source/`, so the hidden-WebView completion drop does not apply. No
`PLUGIN_VERSION` keyword here — the version is declared once as `VERSION`, the
render harness derives its stamp from the target property, and the host reports
1.1.0.

## [1.0.1] — 2026-08-21

### Fixed

- **Age now audibly processes the sound instead of only adding a constant
  hiss** (user report: "the age parameter seems to just be the same static
  noise addition"). Root cause was three-fold — of Age's three designed
  components, only the noise bed was audible:
  - **Pitch drift was structurally neutered.** The ±15-cent / 0.3 Hz
    rate-domain walk needed ~±220 host samples of time storage against
    per-console rails of 9–64 samples, so the offset hit its rail within
    ~100 ms, the factor was forced back to 1.0 and the walk bounced —
    clipping the wobble below 1 cent. Redesigned as an offset-domain servo:
    a ~1.2 Hz random walk (σ normalized at prepare, rate-invariant) is
    tanh-bounded into a time-offset target inside 0.85 × the active rail ×
    age, tracked by a 2.5 Hz servo whose per-chunk delta IS the read-rate
    deviation (capped ±15 cents). Bounded by construction, no rail bounces,
    exactly 1.0 at age 0. Measured (harness G4): 9.0 cents of warble at
    age 100 vs 0.01 at age 0.
  - **Dulling was near-inaudible.** Corner map changed from
    ×(1 − 0.55·age/100) to ×2^(−2·age/100) — linear in octaves, ×0.25 at
    age 100 (was ×0.45), audible from mid ages.
  - Reported latency gains a +24-host-sample (48 kHz-scaled) drift-headroom
    term so PS1's shallow priming deepens (rail ~9 → ~33 samples); wet/dry
    alignment stays exact (priming targets the reported figure).

### Changed

- **The noise/hum bed is now program-dependent** (user direction: "it should
  be dynamic with the sound, not a constant hiss"). A per-chunk envelope
  follower on the wet pre-bed peak (5 ms attack; 150 ms hiss release, 400 ms
  hum release, rate-compensated, NaN-guarded) scales the bed:
  min(1, max(0, 4·env − 0.004)) — full bed at peaks ≥ −12 dBFS, proportional
  below, hard zero under −60 dBFS peaks, so the bed breathes with the
  program and silence stays exactly silent.

### Testing

- Render harness: full suite green. G2 rewritten for the program-dependent
  bed (burst excitation, hiss isolated on the L−R diff — pipeline residue
  and hum are mono and cancel; two new clauses assert the bed decays with
  the envelope and silence stays silent). G3 moved to the same burst/diff
  measurement. Digest anchors re-anchored (2.4 values retired +
  moved-asserted, v1.0.1 anchors recorded) — every wet render's bytes moved:
  enveloped bed, new dulling map, live drift trajectory, +24-sample latency.
- No parameter IDs, ranges, or state format changed; presets and sessions
  load unchanged.

## [1.0.0] — 2026-08-21

Initial release. O-Emulator is a stereo retro-console audio emulation effect:
five complete console pipelines — SNES (BRR 4-bit, 32 kHz, Gaussian), PS1
(SPU-ADPCM, 22.05 kHz, Gaussian), NES (DPCM, 33.144 kHz, ZOH), Game Boy
(4-bit wave, 16.384 kHz, ZOH) and Genesis (8-bit DAC, 26.32 kHz, ZOH) — each
modelled as one coherent chain of codec, fixed internal rate, interpolation
and output stage, selected by a single **Console** switch that rides a 30 ms
click-safe crossfade.

Four macro knobs shape the sound: **Crush** (drive, integer step reduction
with micro-fades, anti-alias opening), **Age** (noise/hum bed, dulling,
pitch drift), **Reverb** (PS1 SPU hall send) and **Mix** (latency-compensated
dry/wet). Constant worst-case latency is reported to the host, so the dry
path stays sample-aligned at every setting.

### Added

- **Five console pipelines** behind one selector, with a per-console accent
  theme, spec readout and 5-segment switch in a naturalist
  engraved-field-guide UI (620×430 WebView).
- **16 factory presets** — two signatures per console (one clean, one
  aged/crushed) plus six cross-console utilities (Lo-Fi Drums, Parallel Grit,
  Reverb Chamber, Subtle Glue, Tape Wash, Crush Extreme). Flat alphabetical
  list; every preset stores all five parameters.
- **Preset band** (preset-manager v1.0.6): prev/next stepping, name readout,
  native save/load dialogs, and a two-click armed delete with 2.5 s
  auto-disarm. The current preset name survives a DAW session save/reload.
- **Knob interaction suite**: relative drag, shift-fine, mouse wheel,
  double-click typed value entry, Alt/Option-click reset to default.
- **Validation**: render-harness (52 checks, digest-anchored), pluginval
  strictness 10 (VST3 + AU), auval.
