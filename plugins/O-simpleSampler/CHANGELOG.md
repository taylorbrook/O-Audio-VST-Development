# Changelog — O-simpleSampler

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.4.4] — 2026-09-03

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
### Fixed — the switch faces now agree

- **The switch's two faces `ui.on` / `ui.off` now read `Activées` /
  `Désactivées`** — feminine PLURAL, agreeing with the noun this switch governs,
  which is `les infobulles` as of this release. They were singular because they
  agreed with the singular noun the suite used before it. That dependency is
  stated only in a source comment beside them: the two faces carry no occurrence
  of the noun, so the sweep that rewrote every string mentioning it did not reach
  them. They kept `reviewed: true` — these are the grammar-forced number of a
  word the developer read and approved, not authored wording.
- `scripts/i18n-fr-glossary.js` accepts `activées` / `désactivées` on the
  `on` / `off` rows alongside the existing renderings. The singulars stay: every
  other plugin's toggle agrees with a singular antecedent. **Positive control
  fired before the glossary moved** — the plural on an unchanged glossary made
  `i18n-fr-lint` exit **2** with two G1 findings naming both faces; with the
  glossary landed the same file exits **0**.

## [1.4.3] — 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide
i18n rollout.

### Fixed

- **item 55a — Pitch Mode tooltip:** `tip.pitchMode` and the tour lesson
  `tip.lessonRepitchStretch` both opened "The headline A/B." (`js/i18n.js`,
  the two `b:` lines). The lesson keeps the phrase — it is the tour's framing,
  and `label.captionRepitchStretch` echoes it. The CONTROL tip now opens with
  what the control is: **"Two ways to move pitch."** / **"Deux façons de
  déplacer la hauteur."** — the two claims after it are unchanged, no new
  claim. French `reviewed: false`. Tip height measured on the real
  `#tooltip` at 980×720: en 100.59 px → 100.59 (unchanged), fr 100.59 →
  116.78 (one more line; bottom at y 546, 174 px inside the frame).
- **item 55b — Repitch/Stretch readout wrapping in French:**
  `#pitchModeReadout` in the Stretch state read *Stretch — durée conservée,
  hauteur indépendante* on TWO lines under `.pitch-mode-readout
  { max-width: 150px }` (`css/styles.css`): 189.59 px of text nowrap, +11.88 px
  of row height, 96 non-label elements moved between English and French and
  `label.play` / `label.kbdHint` pushed 8.7 px past the 720 px frame — in a
  state no gate drove. `tests/i18n-states.json` now drives Pitch Mode to
  Stretch through the stub's own combo state (`setChoiceIndex(1)`, the
  O-Bowed shape), and `check-ui-labels` went RED on the unchanged CSS (3
  FAILED: [5], [6], [7] 96 moved) before the fix. The fix is WIDTH, not copy:
  the readout is the second flex line of its wrapping `.region-row` (104 px
  select cell + 16 px gap + even the narrowest 115.30 px face exceeds the
  215.50 px row), so the whole row is its line and the 150 px cap was
  arbitrary. `max-width: 200px` — 10.41 px over the widest face, 15.50 px
  inside the row. Readout width (nowrap, px): en Repitch 115.30, en Stretch
  147.39, fr Repitch 124.80, fr Stretch 189.59; element height 11.88 in all
  four (was 23.75 in fr/Stretch); row height 69.31 in all four (was 81.19).
  `check-ui-labels` 0 moved in all five states after, both languages. The
  readout is a `[data-i18n]` element written by `setLabel` from
  `label.pitchStretch`, not a Choice face — a copy fix was possible but the
  row had the room.
- **item 58 — focus latch:** a pointer click on a focusable anchor
  (`#toggle-reverse`, `#btnLoad`, the two combos) fired `pointerdown → hide`
  and then `focus → focusin → show`, so the tip came straight back under the
  pointer. Ported the Stage M `lastInputWasPointer` latch (O-Comp v1.7.0
  `setupTooltips`) into `js/app.js`: `pointerdown` latches, ANY `keydown`
  releases, `focusin` opens only while released, `focusout` hides, Escape
  hides. The page's one programmatic `.focus()` (`gearBtn.focus()` in the
  popover's Escape handler) is covered by construction. Probe, hover-help
  driven ON through the page's own gear → toggle, body gated: click →
  tip on 3 of 4 anchors before, 0 of 4 after (the knob took no focus either
  way); Tab → tip with matching body on all six tour buttons before and
  after; click `#toggle-reverse`, Shift+Tab, Tab back → tip *Reverse* /
  *Inversion* after; Escape → hidden. Both languages.

### Not changed

- No DSP: `Source/PluginProcessor.cpp`, `SampleVoice.h`, `SampleSound.h`,
  `dsp/*.h` byte-identical (sha256 recorded); render harness ALL PASS.
- No parameter ID, preset format or state-tree key.

## [1.4.2] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **68 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint: 20 terminology, 31 typography,
  7 grammar and register, 10 meaning. The lint went **71 findings → 0**, and
  `--strict` exits 0.

  The visible ones: `Relâche` → **`Relâch`** (a theatre closure became the
  textbook ADSR term); `Inverse` → **`Invers`** on the toggle with
  **`Inversion`** in its tooltip; `Oui`/`Non` → **`Activée`/`Désactivée`** on
  the hover-help switch; `Affinage` kept over the glossary's `Fin` because the
  End knob is already `Fin` two groups away; `Note de réf.` → **`Note de réf`**
  and `Début boucle`/`Fin boucle`/`Fondu boucle`/`Mode boucle`/`Mode hauteur`
  → the full **`… de boucle`** / **`Mode de hauteur`** forms, so every knob's
  caption is now a substring of the tooltip title that `data-i18n-aria` makes
  its accessible name; straight `'` apostrophes → typographic `’` throughout;
  no-break spaces before `; :` and between a number and its unit.

- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Not changed

- **No English copy, no key, no `TIP_BINDINGS` row, no selector, no CSS rule
  and no `I18N_EXEMPT` entry.** A control imported both revisions of
  `js/i18n.js` and compared every `en` value, the key sets and their order,
  `TIP_BINDINGS`, `I18N_EXEMPT` and `LANGUAGES`: **0 leaks**, 68 `fr` values
  moved.
- `reviewed: false` stays `false` on all 109 entries. That flag means a native
  speaker read the string; this pass is a second machine reading against a
  glossary and a lint, and the file header records it instead.
- No DSP, parameter, APVTS or state-format change. Presets and sessions are
  unaffected.

### Terms measured and NOT applied

Each was measured on its real node at the shipping 980×720 frame with
`Range.selectNodeContents`, not inherited from the previous header:

| Glossary root | Measured | Pinned box | Kept |
|---|---|---|---|
| `Relâchement` | 77.33 px | 54 px `.knob-cell`; gap to `Vél→Ampli` −2.14 px | `Relâch` |
| `Inversion` (caption) | grows `#toggle-reverse` 87.00 → 97.09 px | `min-width: 87px` | `Invers` |
| `Note de référence` | 59.77 px | 54 px `.knob-cell` | `Note de réf` |
| `Nappe bouclée` | takes the chip row 441.91 → 477.83 px | `min-width: 478px` | `Nappe` |
| `Fin` (for Fine) | 17.11 px — fits | not width: `label.end` is already `Fin` | `Affinage` |

The last row and its tooltip twin are the file's two `termNote` exemptions.

### Testing

- `node scripts/i18n-fr-lint.js --plugin O-simpleSampler --verbose` — **71 → 0**
  (41 T1, 12 T5, 8 G1, 6 T4, 2 T7, 2 F1 all closed); `--strict` exit **0**, with
  both `termNote` exemptions printed with their reason.
- `node scripts/check-ui-labels.js --plugin O-simpleSampler` — ALL CHECKS
  PASSED. **`[7]` 0 non-label elements moved between English and French**, the
  same 0 as before the pass; the `[8b]` decoration counts hold at 30 and 32;
  the `[2]` vacuity guard still measures 43/47 labels differing.
  **Negative control:** with `Relâchement` planted in place of `Relâch`, `[8]`
  fails on `label.release x label.velToAmp` — the gate targets the branch this
  change turns on, rather than passing either way.
- **Scratchpad hover-help probe** (this plugin has no committed
  `tests/ui_tip_render_check.js`; the gap is reported, not filled in Stage N):
  all **37 `TIP_BINDINGS` anchors** driven in **both languages** — every one
  opens, every tip lands inside the 980×720 frame, and every French BODY
  differs from its English. That includes all seven tour buttons, the seventh
  (`Filtered & Enveloped`, fixed at 1.4.1) among them, and the two anchors
  inside the gear popover, reached through the gear rather than by stripping
  its `hidden`. Tip heights read before and after: one moved,
  `ampRelease` 84.41 → 100.59 px, with 94.80 px of bottom clearance left.
  **Negative control:** with the English body planted on the Reverse tip the
  probe fails on `#toggle-reverse`; restored from a namespaced copy, the file
  is byte-identical (`sha256 8f098f05…`).
- `node scripts/check-i18n.js --strict-v2 --plugin O-simpleSampler` — ALL
  CHECKS PASS, canon v2.
- `node scripts/boot-all-uis.js --strict-tips` — **43/43 clean**, 0 warn,
  0 failed, **DEAD bindings 0**; this plugin's late-binding count stays 0.
- Content height is unchanged at **821.83 px** in both languages against the
  714 px viewport (`.frame` scrollHeight 835). The on-screen keyboard is still
  below the fold at y 721.83 — the standing open item from Stage I is neither
  fixed nor worsened here.

### Scope

`Source/ui/public/js/i18n.js` (French values, the header comment) and
`CMakeLists.txt`. Nothing else.

## [1.4.1] — 2026-08-31

**The seventh tour button gets its hover-help, which it has never had.** One
character class in one selector; the copy for it was written at v1.4.0 and has
been sitting in `I18N` unreachable ever since, in both languages.

### Fixed

- **`.tour-btn[data-preset="Filtered & Enveloped"]` now binds.** The
  `TIP_BINDINGS` row in `js/i18n.js` carried the selector as
  `data-preset="Filtered &amp; Enveloped"` — the HTML entity, copied verbatim
  out of the markup.

  **Root cause.** `index.html:51` writes `data-preset="Filtered &amp;
  Enveloped"`, which is *correct HTML*: `&` must be escaped in an attribute
  value. But the HTML parser DECODES that entity when it builds the DOM, so the
  live attribute value is `Filtered & Enveloped`. A CSS attribute selector
  matches against the decoded value, and the JS string carried the literal
  characters `&`, `a`, `m`, `p`, `;` — so `querySelector` returned `null`,
  `applyI18n()` logged `i18n: tip target not found`, and the button silently
  carried no `data-tip-title` and no `data-tip`. Six of the seven tour buttons
  worked; the seventh never did, in either language.

  The markup is untouched — its entity is right. Only the selector changed.

### Why this took three versions to find

The warning was real and was emitted on every single page load. Nothing read
it. `check-i18n` is static and proves only that the *key* exists in `I18N`;
`check-ui-labels` has no tooltip awareness; and `boot-all-uis` filtered the
console to `type() === 'error'`, dropping every `console.warn` before examining
it. O-simpleSampler has no `tests/ui_tip_render_check.js`, so no per-plugin
gate covered it either.

That blindness was fixed repo-wide the same day in `56cdbb37`, and this defect
is what the new census found on its first run across all 43 plugins — the only
DEAD binding in the suite.

### Testing

- `node scripts/boot-all-uis.js --plugin O-simpleSampler --strict-tips` —
  **exit 2 → exit 0**, `DEAD bindings: 1 → 0`. The gate names the selector
  before the fix and reports no i18n diagnostics after it.
- Both languages, all seven buttons: every one carries a `data-tip-title` and a
  non-empty body. The seventh reads `Filtered & Enveloped` (131 chars) in
  English and `Filtré et mis en enveloppe` (157 chars) in French.
- **Negative control:** with the fix reverted in place, that button reports
  `title=null, body=0ch` in both languages while the other six stay green — so
  the check targets the branch the fix changed rather than passing either way.
- `node scripts/check-i18n.js --strict-v2 --plugin O-simpleSampler` — ALL
  CHECKS PASS, canon v2.
- No DSP, parameter, APVTS or state-format change. Presets and sessions are
  unaffected.

### Scope

A repo-wide sweep confirmed this is the **only** `TIP_BINDINGS` selector across
all 43 plugins carrying an HTML entity. Four other attributes in the suite hold
entities (`data-label="Bore &amp; Resonance"` and similar), but nothing selects
on `data-label`, so they are display-only and correct as written.

## [1.4.0] — 2026-08-28

**The page speaks French, not only the hover help.** Every caption, heading,
button face, hint, toast and tooltip switches with a language selector in a new
header gear. Readouts stay English (D-03) and so do the Loop Mode / Pitch Mode
menu entries, which come from the C++ `AudioParameterChoice` and are the host
automation contract (D-01). No DSP change, no parameter change: the APVTS
contract stays at **20 parameters** with identical string IDs, ranges and
defaults, so sessions, presets and automation written by 1.3.x restore unchanged.

109 French entries — 36 tooltips and 73 labels — **all machine-drafted and
flagged `reviewed: false`**. No native speaker has read them.
`node scripts/check-i18n.js` prints the worklist.

### Added
- **A language selector.** English and French, explicit, no locale sniffing. It
  rides the APVTS state tree as a `uiLanguage` attribute on the SAME custom
  `<UI>` child that has carried `tipsEnabled` since 1.3.0 — deliberately not an
  APVTS parameter, for both of the reasons that flag is not one: a preference
  must not appear in a host automation lane, and `applyFactoryPreset` (which
  writes parameters only) must not be able to reset which language somebody
  reads their interface in. Persisted as the CODE (`"en"` / `"fr"`), restored
  behind an `isVoid()` gate.
- **`Source/ui/public/js/i18n.js`**, the copy table for both languages, embedded
  in `O-simpleSampler_UIResources` and served from `getResource()`.
- **Render-harness probe `language-state-roundtrip`**: both languages survive a
  save/restore, a fresh instance is English, the value is persisted as the code
  rather than the runtime int, and a 1.3.x tree — which HAS a `<UI>` child but no
  `uiLanguage` attribute on it — restores to English. That last case is the one a
  child-presence check gets wrong: the child is valid and the property is VOID.

### Changed
- **The "?" chip moved into the gear panel** as a labelled Hover help row, rather
  than being duplicated. It occupies the same 21px circle in the same header
  slot, so the header silhouette is unchanged and only the glyph differs. One
  place for the two settings that decide what the hover help says and whether it
  says it; a second settings surface beside a gear would be two answers to one
  question. The C++ bridge (`getTipsEnabled` / `setTipsEnabled` and the
  `tipsEnabledChanged` push) is untouched.
- **The native `title=` hover fallback is gone** (canon v2 §4). 1.3.0 installed a
  plain-text `title=` on all 34 tip anchors and had to strip them when tips went
  off; on an element that already carries a rich tip it is a second, untranslated
  OS tooltip competing with the measure-then-pin renderer.

  Deleting it also deleted the save/restore path the toggle needed. That path
  parked the authored English in `data-tip-title` and put it back verbatim when
  tips came on again — and `applyI18n` WRITES `data-tip-title` on every sweep, so
  restoring the parked copy would have resurrected English text after a French
  switch. The bug never shipped because the layer it belonged to is gone.
- **Tooltip copy moved out of `js/app.js`'s `TIPS` object into `js/i18n.js`.** The
  renderer reads the anchor's own `data-tip-title` / `data-tip`, which `applyI18n`
  rewrites per language — one code path, so no tip can be stranded in the previous
  language after the selector fires. The 34 markup anchors moved off `data-tip`
  (which now carries the tip BODY) onto `data-param`, an id, or the `data-preset`
  the lesson buttons already carried. Listeners are delegated on the document,
  because no anchor carries `data-tip` until the first sweep has run.
- **The tip bodies lost their `<em>` / `<strong>` / `<code>` markup.** The renderer
  builds the tip with `createElement` + `textContent` now, not `innerHTML`. The
  WORDS are unchanged, and the tag-free form is not new copy: 1.3.0 already
  installed exactly this string as the native `title=` fallback, so it is a
  sentence this plugin has been shipping since then.
- **The lesson caption is the header's own full-width row** instead of a wrapped
  row inside the preset bar. It always was a full-width row; it was just 100% of
  the 554px preset bar rather than 100% of the 926px header. At 554px several
  French lesson captions took a second line where their English counterparts took
  one (SP-1200: 13.2px against 26.4px). At the header's full width all eight
  captions are a single line in both languages, so no line has to be reserved —
  which is what the alternative would have cost 13.2px of English page height for.
- **The knobs have accessible names.** Each `.knob` carries `data-i18n-aria`
  naming its parameter, resolved through the label table's fallback to the
  tooltip TITLE. Through 1.3.1 the knobs had `role="slider"` and `tabindex` but no
  name at all.

### Fixed
- **The seven per-group flex declarations in the rack have been dead since the
  rack was written.** `.rack .group { flex: 1 1 auto }` scores (0,2,0); a bare
  `.group-source` scores (0,1,0), so the shorthand won on specificity whatever
  the source order and every group has been sized by its own max-content. That is
  what made the rack language-dependent: with an `auto` basis, the French
  drop-zone sentence widened the Source group by 170.8px and the rack folded from
  two rows into three, growing 194.4px. The seven rules now use the child
  combinator `.rack > .group-x`, which also scores (0,2,0) and comes later, so
  the authored bases apply. An explicit basis takes content out of the
  line-breaking decision entirely.

  This is a visible change to the ENGLISH layout, and the only one in this
  release: the groups take their authored widths (Vintage 144.7 -> 110, Filter
  204.5 -> 232, Amp 408.5 -> 424, Region 366.2 -> 403, Pitch 275.5 -> 241.5)
  rather than the widths their own text happened to produce.

### Geometry (D-04)
Six pins. Each was measured, and each was reverted ALONE to confirm
`check-ui-labels` re-breaks without it — a fix that passes both ways is
decoration.

| Pin | Measured | Negative control |
|---|---|---|
| `.title-block` 372px | French subtitle 370.8px against English 246.2 — the block widened 124.6px and pushed the whole preset bar sideways | assertion 7: 3 moved |
| `.preset-bar-tour` `min-width: 478px` | the seven French chips measure 441.9px against 477.1 — French getting SHORTER moved the fleuron and gear | assertion 7: 2 moved |
| `.rack > .group-*` child combinator | rack 382.8px -> 577.2px in French, three rows instead of two | assertions 5, 6 and 7: 139 moved |
| `min-height: 2.2em` on Region + Pitch knob captions | "Note de réf." wraps where "Root Key" does not, taking the Pitch group and everything under it down 10.4px; Region's "Fin boucle" / "Fondu boucle" move their own readouts 10.4px inside a row whose height never changes | assertion 7: 105 moved (Region half alone: 4 moved) |
| `#toggle-reverse` `min-width: 87px` | "Reverse" 86.1px against "Inverse" 83.8 — the button's right edge and the fleuron pinned to it both move | assertion 7: 3 moved |
| the caption row move (above) | SP-1200 caption 13.2px against 26.4 at 554px | assertion 7: 145 moved |

The `min-width` pins are `min-width` and not `width` on purpose: on a platform
whose Garamond metrics run wider, the box grows instead of clipping. That failure
mode is a geometry difference the gate reports; a cap is what orphaned "Filtered"
onto a second row at 460px in 1.2.0.

**Frame cost, measured.** The page's scroll extent inside the fixed 980x720
editor grows from 796px (1.3.1, English) to **835px, identical in both
languages**. The frame has scrolled since the rack was built and the on-screen
keyboard was already 60% below the fold; it is now entirely below it, and the
keyboard is reached by scrolling. 17px of that is the rack basis repair, 10.4px
the reserved caption line, the rest the header's caption row. The alternative —
trimming eight French lesson captions to a character budget so they hold their
English line count — was rejected as copy that any later edit re-breaks.

### Known gaps
- **Canvas text is not localized.** `drawWaveformEditor` paints "drop or load a
  source to see its waveform" and the `root C3` marker with `ctx.fillText`. A
  2D-context string is not a DOM node: neither the canon sweep nor either gate can
  reach it, and localizing it would need a repaint hook outside the canon block.
  Both strings are recorded in `I18N_EXEMPT` with that reason rather than left
  silent. O-Orbit ships FRONT / ELEV and O-MultiBandCompressor its analyzer
  placeholder the same way; this is the suite's existing position on canvas text,
  carried into Stage M rather than discovered there.
- **French is unreviewed.** All 109 entries are machine drafts.
- **Windows/WebView2 is unverified.** WebView2 font metrics differ from WebKit, so
  a French label that fits on macOS could clip on Windows. Named deferral, owner
  none, blocked on hardware.

### Testing
- `check-i18n --plugin O-simpleSampler`: exit 0, canon v2.
- `check-i18n --strict-v2`: exit 0 — 9 plugins on canon v2, 0 on canon v1.
- `check-ui-labels --plugin O-simpleSampler`: ALL PASS across four states
  (default, gear open, hover help off, a concept preset picked). ZERO non-label
  elements moved between English and French; vacuity 43/47 labels (91%) and 24/25
  attributes actually changed language; `dataset.label === textContent` holds
  after init, after the switch and after a state pass; 48 of 47 `[data-i18n]`
  elements visible in at least one state.
- `boot-all-uis`: clean. (O-Bowed and O-Reed fail repo-wide with
  `Unexpected token 'export'` — pre-existing, unrelated, untouched.)
- Render harness: ALL PASS, 12 probes, 1.3.1's numbers unchanged.
- `auval -a` lists `aumu OsSm OuDv`.

## [1.3.1] — 2026-08-25

### Fixed
- **Clicks on note-off, at any settings** (ported from O-simpleFM v1.2.5; found
  by a suite-wide sweep of the per-block ADSR push pattern). Root cause: the
  processor pushed ADSR parameters into the live `juce::ADSR` amp envelope every
  block via `setParameters()`, whose `recalculateRates()` recomputes the release
  slope from the SUSTAIN level — clobbering the envelope-value-based rate that
  `noteOff()` had just computed. With sustain = 0 the recomputed rate is 0, and
  `recalculateRates()` treats a zero-rate release as finished: it hard-resets the
  envelope one block after every note-off, truncating the ringing tail to
  silence instantly — the click. (JUCE's ADSR docs explicitly forbid changing
  parameters during playback.)
  Fix in `SampleVoice.h`: envelope params are cached each block but only pushed to
  the live envelope(s) when their values actually change AND the voice is not in
  its release phase; changes made mid-release apply at the next note-on. The
  release therefore always completes at the rate captured at note-off.
- Render-harness: new `noteoff-click` probe (sustain 0, slow decay, note-off
  mid-decay) asserting the release tail still rings after note-off.

### Testing
- Render harness: ALL PASS including the new probe (preRms 0.0274 / tailRms 0.0116).
- Negative control: probe re-run against v1.3.0 voice code fails as expected
  (preRms 0.0274 / tailRms 0.0000 — the tail is truncated to exact silence one block after note-off).

## [1.3.0] — 2026-08-02

Header layout repair plus a tooltip on/off switch. UI-only: no DSP, no parameter,
and no preset change. The APVTS contract stays at **20 parameters** with identical
string IDs, ranges and defaults, so sessions, presets and automation written by
1.2.0 restore unchanged.

### Added
- **A "?" button in the header that switches the hover explanations off and on.**
  Tooltips remain **on by default** — a fresh instance behaves exactly as 1.2.0 —
  and the choice is **saved with the DAW session**, so a project reopens the way it
  was left instead of switching the explanations back on every time. Pressed/green
  means explanations are on.

  The button gates **both** hover surfaces: the floating rich tooltip *and* the
  native `title=` fallback the same layer installs. Stripping only the first would
  leave the OS tooltip popping up on hover with the switch visibly "off". The
  authored copy is parked in `data-tip-title` and restored verbatim when tips are
  switched back on (34 elements, round-trip verified).

  The flag is stored as a custom `<UI tipsEnabled="…"/>` child of the saved state
  tree, next to the existing `<SOURCE identity="…"/>` — deliberately **not** an
  APVTS parameter. That keeps the automatable parameter count at 20 (a 21st
  parameter would be a contract change and would show up in every host's automation
  list for a preference), and it keeps `applyFactoryPreset` — which writes
  parameters only — from resetting the user's choice on every concept-preset click.
  State written before 1.3.0 has no `<UI>` child and restores to the default (on).

  The button deliberately carries no `data-tip` of its own: the tooltip layer owns
  the `title` attribute of every `[data-tip]` element and strips it when tips are
  off, which would leave this button unlabelled in exactly the state it exists to
  undo. Its own `title` is authored in the HTML and never touched.

### Fixed
- **The "O – simpleSampler" title no longer breaks across two lines**, and neither
  does the "Keyboard Sampler · A Field Guide" subtitle. `.title-block` is a flex
  item beside the preset bar, which claimed the rest of the header row and squeezed
  the title to 207 px; the hair spaces around the en-dash are break opportunities,
  so the title folded after "O –". Sizing the block to its content (`flex: 0 0 auto`)
  and pinning both lines with `white-space: nowrap` holds the title at its natural
  246 px.
- **All seven concept-preset chips now sit on one row.** The chip bar was capped at
  `max-width: 460px` — 17 px short of the 477 px the row actually measures — which
  orphaned "Filtered" onto a second row. The cap is removed and the bar is
  `flex-wrap: nowrap`.
- **The header row is right-aligned explicitly.** The chip bar used to be padded out
  to its fixed 460 px cap and right-aligned its own contents, which parked it near
  the header's right edge as a side effect. With the bar now exactly its content
  width, `.preset-bar-container` needs `justify-content: flex-end` or the row packs
  left and strands ~140 px of dead space before the header edge.

Net effect: the header is **56.7 px tall instead of 94 px**, and the 37 px it gives
back is why the on-screen keyboard is no longer clipped at the bottom of the 980×720
editor.

### Testing
- Rendered against a JUCE-bridge stub at the editor's real 980×720 and measured:
  title 29.5 px (one line, was 59), subtitle 13 px (one line, was 26), 7 chips on
  1 row, "?" flush to the header's right edge (0 px gap), no horizontal overflow.
- Tooltip toggle, all verified in the rendered page: default on; off strips all 34
  `title` attributes and suppresses the floating tip; back on restores all 34
  verbatim; an already-open tip retracts the moment tips are switched off; a session
  restored with tips off boots with tips off and can still be switched back on; the
  C++ `tipsEnabledChanged` push applies in both bare and array-wrapped `var` shapes;
  clicking a concept preset does not reset the choice.
- WebView bridge grep-diff: 10 `getNativeFunction` calls ↔ 10 `withNativeFunction`
  handlers, no orphans either way; `tipsEnabledChanged` is emitted and listened for.
- **New render-harness test `tips-state-roundtrip`** (harness is now 10/10, was 9/9).
  Neither auval nor any render test can observe a non-parameter state child, so the
  save/restore contract is asserted directly: off→off, on→on, a fresh instance
  defaults to ON, pre-1.3.0 state with no `<UI>` child restores to ON rather than
  leaking the instance's current value, and a witness parameter still round-trips.
- auval: **VALIDATION SUCCEEDED**, 20 Global Scope Parameters.
- pluginval strictness 10, 3 consecutive runs: **SUCCESS** (exit 0, no failures).
- Offline render harness: **10/10 PASS**, 0 failures.
- AU bundle version verified 1.3.0 / 66304.

## [1.2.0] — 2026-08-02

First published release. Neither 1.0.0 nor 1.1.0 was ever distributed — no tag, no
GitHub Release, no binary shipped to anyone. 1.2.0 is the version under which the
1.1.0 content withdrawal reaches its first actual release. No code, DSP, parameter,
or UI change since 1.1.0: the 20-parameter APVTS contract, the piano-only built-in
source set, and the 7 factory presets are exactly as verified for 1.1.0 (auval
20 params, pluginval strictness 10 ×6, offline render harness 9/9).

### Added
- `Source/samples/LICENSE.md` — provenance and licensing for the built-in sample
  set: the surviving piano source is documented as procedurally generated and
  self-authored, and the three commercial-library sources withdrawn in 1.1.0 are
  recorded as removed rather than replaced.

## [1.1.0] — 2026-08-01

Content and contract change ahead of the repository going public. No release ever shipped
v1.0.0, so nothing distributed is affected by any of the removals below.

### Removed
- **Three built-in sources — `cello`, `pizz`, and `hit`.** These assets originated in a
  commercial sample library whose redistribution rights were never established, so they
  could not remain embedded in a plugin binary that is about to be published. They have
  been **removed rather than replaced**: substituting generated or CC0 audio was
  considered and deliberately declined. The built-in source set is now **piano only**
  (recorded root 48), which is procedurally generated and provably self-authored — see
  `Source/samples/LICENSE.md`. No release of this plugin ever shipped the withdrawn
  assets; they were present only in the repository.
- **The `sourceSample` Source parameter** (APVTS contract **21 → 20 parameters**). With a
  single built-in remaining, keeping the selector as a one-entry choice is not an option:
  JUCE's `AudioParameterChoice` asserts `choices.size() > 1`, and a one-entry choice builds
  the degenerate `NormalisableRange {0, 0}` whose `convertTo0to1` is `0/0` — a NaN that
  `jlimit` does not clamp, so the parameter would be born holding NaN in a Release build.
  The parameter is therefore dropped outright. The surviving 20 parameters keep their
  string IDs and versioned `ParameterID`s unchanged, so VST3/AUv2 automation IDs (hashed
  from the string ID, not the index) are stable. A v1.0.0 session carrying a `sourceSample`
  entry restores harmlessly — the orphan child is inert.
- The Source group's built-in `<select>` combo in the WebView UI, which had nothing left
  to select. Two combos remain (Loop Mode, Pitch Mode).

### Changed
- The plugin now **starts on its one built-in source** with no user action; the recorded
  root (48) is still seeded on a fresh instance so it plays in tune immediately.
- **`Load…` and drag-and-drop are now the only way to change the source**, and are
  otherwise unchanged — WAV / AIFF / FLAC, resampled to the engine rate, 30 s cap. The
  Source group's status line is now seeded at boot with the active built-in's name so a
  fresh instance reads as loaded rather than blank.
- The 7 factory presets are unchanged. All of them were already source-agnostic (none set
  the removed parameter), and the central post-reset root re-seed still runs, so none play
  octave-flat.

## [1.0.0] — 2026-06-26

First release. O-simpleSampler is the sampler sibling to O-simpleFM / O-simpleAdditive /
O-simpleGrain / O-simpleSubtractive — a deliberately simple, pedagogical keyboard
sampler that strips the software sampler down to its spine: **a recording, a region of
it, a root key, a loop, and an envelope**, each with a visible consequence on the
waveform. Built for the MUSC319 wk05 sampling session. Cross-platform WebView UI
(macOS AU + VST3, Windows VST3 via WebView2).

The 21-parameter APVTS contract is frozen for v1.0.

### Added — instrument (Stages 1–3)
- **Polyphonic keyboard sampler** (16-voice) — the active source read through a
  fractional-read varispeed head, region-isolated, anti-aliased, shaped by a per-voice
  amp ADSR + VCA + velocity sensitivity, tuned to the live **Root Key**.
- **Curated built-in source set** (FUNC-02): **piano** (root 48), **cello** (root 69),
  **pizz** (pizzicato strings, root 69), and **hit** (percussive one-shot, neutral root
  60). Selecting a source seeds its recorded-pitch root so it plays in tune. Roots were
  probed via YIN f0 estimation. Embedded as a second `juce_add_binary_data` target with
  a distinct `BinaryData` namespace (UI resources use `UIBinaryData`).
- **Load your own sound** (FUNC-03) — drag-and-drop (macOS WKWebView content-streaming
  bridge) **and** a file-picker fallback; WAV / AIFF / FLAC, resampled to the engine
  rate, capped at 30 s.
- **Region** — Start/End to isolate the useful region; **Loop** (Off / Forward /
  Ping-Pong) with equal-power crossfade + zero-cross marker snapping; **Reverse**.
- **Pitch** — **Repitch** (honest varispeed) ↔ **Stretch** (SOLA pitch/time
  independence), plus Root Key, semitone Tune, and fine cents.
- **Vintage** — SP-1200-style sample-rate decimation + bit-crush (clean at 0).
- **Filter** — resonant TPT low-pass with a live-matching display curve.
- **Interactive waveform editor** — draggable start/end + loop handles (two-way via
  relays), shaded loop band, live playhead, root-key indicator, filter curve, amp-ADSR
  scope. 34/34 controls tooltipped.
- **Concept preset tour** (FUNC-07) — 7 named factory presets, each isolating one
  sampler concept: Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell,
  Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped. Each resets to defaults,
  re-seeds the active source's recorded root (so nothing plays octave-flat), then sets
  only the parameters that isolate its concept; the WebView relays resync every control.

### Changed / hardened (Stage 4 — Polish)
- **RT-safe source-swap handoff.** The audio thread now reads the live source through a
  raw `std::atomic<AudioBuffer*>` (acquire) and never holds a `shared_ptr`, so a swap can
  never free a buffer in the render path. The previous buffer is held one generation
  (`retiredSource`) and freed off the audio thread; the owning shared_ptrs are guarded by
  a `CriticalSection` taken only off the audio thread (replaces the deprecated
  C++20 `std::atomic_load/store(shared_ptr)` helpers).
- **Deferred fresh-instance root seed.** The one-time per-source root seed no longer calls
  `setValueNotifyingHost` inside `prepareToPlay` (discouraged — prepare can run off the
  message thread / during scans); it is deferred to the existing AsyncUpdater
  (guaranteed message thread).
- **Render-harness re-armed.** Dropped `PluginEditor.cpp` from the harness target so the
  offline DSP correctness gate stays buildable now that the editor uses WebView types
  under `JUCE_WEB_BROWSER=0` (`createEditor()` falls back to `GenericAudioProcessorEditor`).

### Validation
- Offline render-harness: **9/9 PASS** (makes-sound, Repitch tuning, Stretch
  pitch-tracks-key, Stretch time-independence, loop-seam continuity fwd + ping-pong,
  region-end declick, Vintage clean-at-zero, anti-alias up-transpose, stress bound).
- `auval` SUCCEEDED — **21** Global Scope Parameters.
- pluginval strictness 5 — exit 0 on **both** VST3 and AU.
- Native-fn WebView bridge: 8 JS ≡ 8 editor ≡ 8 processor, 0 orphans.

### Notes
- **Windows (COMPAT-02):** WebView2 wiring is in place (`NEEDS_WEBVIEW2` +
  static-linking flag, dual `BinaryData`/`UIBinaryData` namespaces). Runtime
  verification on a Windows host/DAW is pending user testing — not a CI gate for v1.0.
