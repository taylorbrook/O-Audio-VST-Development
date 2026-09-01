# Changelog — O-Bitrot

All notable changes to O-Bitrot are documented here.

## [1.15.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

**No parameter, preset, state, DSP or layout change.** Every knob, range,
default and all 28 factory presets are bitwise what v1.15.0 shipped, and the
geometry gate reports the same single moved element it did before this pass.

### Changed

- **49 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and lint (`scripts/i18n-fr-lint.js`): 22
  terminology, 17 typography, 5 grammar/agreement, 5 meaning. The visible ones:
  *Dosage* → **Mix** on the Blend caption, the Codec blend tip and the global
  Mix; *Germe* → **Graine** on the Seed readout and its tip, with the Reseed
  tip title *Retirer un germe* (which reads "remove a seed") becoming
  **Nouvelle graine**; *Voile* → **Déform.** on the Warp caption and
  **Déformation** in its tip; *Ampleur* → **Prof.**; *Masquage* → **Dissim.**;
  *Fondu* → **Déclin** in the concealment list; *Dithering* → **Dither**; the
  AGC tip title *Gain automatique* → **AGC**, so the caption and the tip name
  the control the same thing. Typography: no-break spaces before `%`, `:`, `;`
  and `?` and between every number and its unit (*20 ms*, *500 Hz*, *20 kHz*,
  *10–400 ms*).
- **Four grammar fixes** the drafts carried: *sont asservis* → *sont asservies*
  (Vinyl Speed), *est jamais touché* → *est touché* (Flip severity), the bare
  *Si … ou* fragment on the Clock tip, and the missing elision in the Splices
  marginal note (*lorsque allumé* → *lorsqu'allumé*).
- **Five bodies now say what the English says**: the concealment tip names the
  four faces the user can see (Silence / Répéter / Déclin / Substituer), the
  Mains tip names its own caption (*Bruit*, not *Bruit de ligne*), and the four
  noise-bed tips say *nappe* rather than the calque *lit*.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Measured

| Caption | Glossary root | Measured | Box | Shipped |
|---|---|---|---|---|
| Depth | Profondeur | 75.36 px | 76 px `.mix-text` | **Prof.** (31.39) — 0.64 px is not a fit |
| Conceal | Dissimulation | 88.97 px | 82 px `.ctl` | **Dissim.** (42.98) |
| Warp | Déformation | 81.30 px | 64 px `.ctl` | **Déform.** (50.38) |
| Rate | Vitesse | 46.41 px (fits) | 64 px `.ctl` | **Fréq.** — kept on MEANING, not width: the control is a sample rate in Hz (`termNote`) |
| Pop | — | *Craquements* 84.47 px | 64 px `.ctl` | **Clics**; the tip title is now *Clics et craquements* |
| Splices note | — | *lorsqu'il est allumé* 117.95 px but two lines | 132 px `.annot` | **lorsqu'allumé** (124.80, one line) |

`reviewed: false` stays on all 117 entries — this was a second machine reading
against a glossary and a lint, not a native-speaker review.

## [1.15.0] — 2026-08-27 — the PAGE speaks French, not only the hover help

**No parameter, preset, state or DSP change.** Every knob, every range, every
default and all 28 factory presets are bitwise what v1.14.0 shipped. The layout
DID change — see the measured table below — and every one of those changes
applies identically in both languages.

Third plugin on canon v2, after O-Tapestop v1.6.0 and O-MultiBandCompressor
v1.11.0, and the first with an inline `<script type="module">` controller rather
than a `js/app.js`.

### Added

- **65 label keys** in a new `LABELS` table in `js/i18n.js`, plus a reasoned
  `I18N_EXEMPT` covering the product name, the company name, the loaded preset
  name, the eight `Tab. N` plate numbers and the two line-coding standards
  (μ-law, GSM).
- **`data-i18n` on 71 elements and `data-i18n-aria` on 8.** The element owns its
  caption; the authored English stays in the markup as the fallback that renders
  if `applyI18n()` never runs.
- **`tests/i18n-states.json`** so the label gate opens the settings popover and
  measures the two captions and the toggle inside it.

### Changed

- **Canon v1 → canon v2**, verbatim from `scripts/i18n-canon.js`, keeping the
  depth-adjusted `'./js/i18n.js'` import this inline module needs.
- **The i18n block MOVED, to above the first binding that reaches it.** Canon v1
  only ever ran from the guarded `initI18n()` call near the foot of the module;
  canon v2 is reached from BINDING time too — the seven panel buttons, the
  delete button and the hover-help toggle all call `setLabel()`, and every one
  of those blocks runs at module top level. Left where it was, `uiLanguage` sat
  in its temporal dead zone and each call threw a ReferenceError
  (`pattern_module_toplevel_init_tdz`). This module has no `init()` to isolate a
  failure, so that throw takes the whole UI down.
- **The three `data-*`-authored captions became keys.** `data-on` / `data-off`
  on the hover-help toggle and `data-label` / `data-confirm` on the delete
  button were the right answer while the page was English-only — they kept the
  copy out of the JS, which is what
  `pattern_js_state_updater_overwrites_html_labels` asks for. They are the wrong
  answer with two languages: an attribute holds ONE string, so switching to
  Français mid-session restored an English "On" or an English armed face.
- **The seven panel enable buttons** likewise: two `setLabel()` calls behind an
  `if`/`else`, never a ternary in the argument.
- **Two false sentences fixed, in both languages.** `lang-select`'s tip said
  "the labels on the page itself do not change"; the language selector's
  `aria-label` said "Hover help language" while the control now sets the
  language of the whole page.
- **`tests/ui_tooltip_clamp_check.js`: the toggle-caption assertion was
  rewritten a second time.** v1.13.0 pinned the literal "?"; v1.14.0 compared
  the rendered caption against the authored `data-on` / `data-off`. Those
  attributes are gone by design, so the assertion now checks the element's own
  KEY and the `dataset.label === textContent` ownership mirror — the systemic
  form of the same rule, and the same assertion `check-ui-labels` makes
  repo-wide. Confirmed by negative control: reinstating a JS literal caption
  fires both halves.

### Fixed — layout, from the measured English↔French geometry diff

`node scripts/check-ui-labels.js --plugin O-Bitrot` reported **78 non-label
elements moved** at the shipping 900 × 740 before any fix, and **zero** after.
D-04 forbids an auto-shrink font and a short-variant fallback, so every fix pins
a container that was shrink-to-fit.

| Moved | By | Cause | Fix |
|---|---|---|---|
| the preset band, both nav arrows and the name readout | dx −23.0, dw +17.4 | SAVE 43 → ENREG. 53.7, LOAD 45.3 → OUVRIR 55, DELETE 54.6 → SUPPR. 51.6 | the three text buttons pinned to 55 px |
| the imprint block and the brand line above it | dx −28.8, dw +51.8 | `.hdr-right` was sized by the keyed plate line | `.hdr-right { width: 224px }`, and the French line shortened to `Catalogue des supports défaillants · Pl. XLVII` (221.4 px) so it fits |
| all seven panel captions and their spacers | dw up to +45.6 | the caption was content-sized and the `.gap` spacer absorbed the difference | `.p-head .caption { flex: 1 1 auto }`, `.gap { flex: 0 0 0 }`, and `.p-head .en { width: 62px }` (ON 36.3 → MARCHE 60.6) |
| three knob columns in the Tape panel and the knobs and readouts in them | dw +9.2 | `.ctl` is `align-items: center` and so shrink-to-fit; WOW 28.4 → PLEURAGE 59.2 made the caption wider than its 50 px knob | `.ctl:has(> .knob) { width: 64px }`, and the Vinyl panel's second row re-tuned 56% → 67% to keep its designed 14 px gaps |
| the Rot plate's four caption+readout stacks and the global Mix stack | dw up to +27.5 | same shrink-to-fit shape; GARBLE 45.2 → BROUILLAGE 72.7 | `.mix-text { width: 76px }` |
| the clock group, the seed group and both separators beside them | dx up to −6.9, dw +21 | the Sync/Free pair is sized by its own captions | `#clockModeSeg { width: 112px }` with `flex: 1` on its two buttons |
| the Splices group | dw +19 | HARD EDGES → FRONTS FRANCS (+14.6) and its marginal note 109.3 → 128.3 | `#edgeBtn { width: 118px }` and the note pinned to 132 px, centred |
| the Rot plate's caption | dw −39.2 | its marginal note grew and the flexible caption gave up the room | the caption fixed at 150 px, the note takes the slack instead |

**One of these corrects an English inconsistency nothing was measuring:** the
Comfort column already rendered 53.8 px against its neighbours' 50, because its
own English caption was wider than its knob.

`.tooltip { max-width }` is untouched at 230 px.

### Known, and left alone

- **Five `<option>` elements are keyed but can never be measured by the label
  gate.** A native `<select>` popup is UA-rendered and its options have no box
  in the document. The selects themselves are unchanged in width.
- The nine panel-caption spans are non-replaced INLINE boxes, so the label
  gate's text-spill check skips them and says so — `clientWidth` is 0 by
  definition for an inline box. Their rects are still measured by assertions 5,
  6 and 7.
- **The clamp count moved 14 → 15 per language** (Stage D's figure). That is a
  consequence of the deliberate English layout changes above moving the anchors,
  not an unreported French one: the count is identical in both languages apart
  from the +1 French already recorded. Anchors stay 55, and French still costs
  +3 vertical flips and is 14.8 px taller at its tallest.
- All 120 French entries (55 tooltip, 65 label) are machine drafts flagged
  `reviewed: false`. `Enreg.` / `Ouvrir` / `Suppr.`, `Clics` for Pop and
  `Ampleur` for Depth were picked with width as a constraint and should be the
  first a native speaker challenges.

## [1.14.0] — 2026-08-26 — English/French hover help; the clamp gate now sweeps both languages

**No parameter, preset, state, DSP or layout change.** Every knob, every range,
every default and all 28 factory presets are bitwise what v1.13.0 shipped. What
changed is where the hover-help copy LIVES, and the addition of a language it
can be read in.

### Hover-help copy moved out of the markup

All 53 tooltips left `index.html` and now live in a new `js/i18n.js` as a key
table, `{ key: { en: {t, b}, fr: {t, b, reviewed} } }`. **The English was moved,
not rewritten** — every `en` entry was extracted mechanically rather than
re-typed, and a comparison against the original markup confirms all 53 are
byte-identical, with HTML entities decoded (`&amp;` → `&`) because
`setAttribute` + `textContent` do not decode them.

The renderer is UNCHANGED. `showTip()` still reads `data-tip-title` / `data-tip`
off the anchor; those attributes are simply written at runtime by `applyI18n()`.

**Keys are the parameter ID, not the element id.** 41 of the 53 anchors on this
page carry no id at all — they are `.ctl` and `.g-group` wrappers around a
`[data-param]` knob — so binding by id was never available here. The canonical
`[selector, key, wrapper]` triple addresses them exactly: the selector finds the
knob and `closest(wrapper)` walks back up to the cell the tip belongs on.

### A settings popover, and the hover-help toggle moved into it

The gear replaces the v1.12.0 `?` in the same header slot, at the same 22 px
circle geometry and palette, so the header silhouette is unchanged — asserted by
measurement, not assumed. The panel carries two rows: Language (English /
Français) and Hover help. **The toggle MOVED; it is not duplicated.**

It now reads On/Off rather than showing a static `?`, so its caption is written
from script for the first time. The copy it shows is authored in `data-on` /
`data-off` **in the markup**, never as a literal in the JS — the rule that
`pattern_js_state_updater_overwrites_html_labels` exists for, and the gate now
compares the rendered caption against those attributes rather than against a
pinned glyph.

The language persists with the session as a non-parameter property on the APVTS
state tree, beside `tooltipsEnabled`, guarded on restore by `isVoid()` and read
with `toString()` — the same trap, handled the same way
(`critical_valuetree_xml_roundtrip_loses_type`). Bridge 13 → 15.

### All French is machine-drafted and UNREVIEWED

All 55 entries carry `reviewed: false`. No native speaker has read them.
`node scripts/check-i18n.js` prints the worklist. The terms most likely to want
a native speaker's judgement: `ROT_ENABLE` ("Corruption"), `CRUSH_ENABLE`
("Écrasement"), `VINYL_WARP` ("Voile"), `PACKET_BURST` ("Groupement") and
`seedRo` ("Germe").

### The clamp gate is parameterised by language, not duplicated

All three existing sweep passes now run once for `en` and once for `fr`, in one
process against one page load, driven through `window.__setLanguage()`.
Assertion 3 (vertical) is the language-sensitive one: French wraps to more lines
inside the **unchanged** 230 px cap, so tips get TALLER and the above/below flip
has to catch what no longer fits above.

`max-width` is now PARSED from the page's own CSS rather than hard-coded,
because the cap differs per plugin; the literal survives as the drift guard. The
anchor count is now derived from `TIP_BINDINGS` rather than by counting
`data-tip=` literals in the markup — that count is zero now, so the old
assertion would have passed vacuously against nothing. A fourth sweep pass opens
the settings popover so `#lang-select` and the moved toggle are measured rather
than skipped; both sit in the top-right corner, where the clamp and the flip bite
hardest. Two vacuity guards, both per-language: the clamp must engage at least
once, and every anchor's copy must actually differ between `en` and `fr`.

### French geometry, MEASURED at the shipping 900 × 740

| | anchors | clamped | flipped below | widest | tallest |
|---|---|---|---|---|---|
| `en` | 55 | 14 | 10 | 230.0 px | 119.1 px |
| `fr` | 55 | 14 | **13** | 230.0 px | 133.9 px |

**French costs three extra vertical flips here and zero extra clamps.** This is
the first frame in the suite where French changes flip behaviour at all —
O-Octagon at 1100 × 720 cost one, O-ReverseDelay at 940 × 768 cost none — and
every one of the three was caught by the existing flip logic, so all 55 anchors
in both languages sit fully inside the viewport with an 8 px margin.
`.tooltip { max-width }` was NOT touched.

### Not verified

* **Nothing has been checked in a real DAW.** Everything above is headless
  Chromium against the repo's own ui-stub, `auval`, and the offline C++ harness.
* **The C++ persistence path was never executed.** Nothing wrote `"fr"` into a
  session and read it back. It is a stronger inference here than on most — the
  property sits beside `tooltipsEnabled`, which has shipped and worked since
  v1.12.0 and has its own round-trip probe — but it is still an inference.
* **The native bridge was exercised only against the ui-stub.**
* **No cross-platform check.** Windows/WebView2 has not been exercised.

## [1.13.0] — 2026-08-19

The preset readout is now a **click-to-open menu**, and the menu restores the
factory bank's **narrative grouping** — Showcases, Tape, Vinyl, CD, Phone &
Network, Rot, Disintegration — which had been authored into the bank since
v1.11.0 and visible nowhere.

**This release changes no DSP and no parameter.** Nothing under `Source/dsp`
moved, the layout is the same 45 IDs in the same order, and no preset's values
changed. Every saved session and preset renders exactly as under v1.12.0.

### Added

- **A preset menu on the name readout.** Clicking the plate opens all 28
  factory presets in seven labelled sections; clicking a name loads it.
  Escape, a click outside, or selecting an item closes it, and reopening
  scrolls the loaded preset into view rather than always opening at the top.
- **The ◀ ▶ arrows now step through the MENU order.** They are kept — stepping
  to a neighbour and browsing the bank are different gestures, and both stay
  one click — but they no longer walk the alphabet. See *Fixed* below.
- **`getPresetListGrouped`, an eleventh preset native fn** (registered surface
  **12 → 13**, parity gate clean both directions). It returns an **array** of
  `{category, presets}`, deliberately not an object keyed by category: an
  object would make section order depend on JS string-key insertion order
  surviving the C++ → JSON → JS round-trip, and the alternative — a
  `CATEGORY_ORDER` list on the JS side, which is what O-Prism carries — is a
  mirror of the C++ that drifts the first time a category is added. The array
  carries the order in the data itself.
- **`tests/ui_preset_menu_check.js`**, a browser render gate on the real page
  at the real 900x740. The grouping has **three** descriptions — the C++
  spans, the browser stub, and the rendered DOM — and any two can agree while
  the third drifts, so the file parses the expected grouping out of
  `PluginProcessor.cpp` and holds the other two to it. **33 checks.**

### Changed

- **The narrative grouping is now derived from declaration order, not stored
  twice.** `getPresetList()` ends in `presets.sort(true)`, so the bank has
  always come back alphabetically and the grouping the v1.11.0 constructor
  comment describes ("nine showcases, then tape 10-13, vinyl 14-16…") existed
  only in that comment. It is now a `categorySpans` table of inclusive index
  ranges over the factory vector — **never a second list of names**. A repeated
  name literal would go stale the first time a preset was renamed, and the
  failure would be silent: the preset would simply fall into "User" beside the
  user's own saves (`pattern_test_fixture_mirrors_drift_silently`). The spans
  must tile `[0, size)` exactly; the constructor asserts it, and the render
  gate re-asserts it where a Release build can still catch it.
- **The readout markup was restructured, and `#preset-name` stays childless.**
  The sunken-plate styling moved from `.preset-name` out to a new
  `.preset-select` button, so the caret can share the plate as a **sibling** of
  the readout. It cannot be a child: `_updateDisplay()` assigns `textContent`,
  which would erase it on the first preset change
  (`pattern_js_state_updater_overwrites_html_labels`). The gate asserts the
  caret is still there, and still outside the readout, after several loads.
- **The 53 hover-help anchors are unchanged.** The tip that was on
  `#preset-name` moved to the trigger that now wraps it and gained the browse
  affordance in its copy. Still 53/53, still 52/53 reachable in the shipped
  state, edge clamp still engaged for 12.
- **The menu sits at `z-index: 1200`, above `.tooltip`'s 1000.** A tip anchored
  to the trigger places itself *below* it, which is exactly where the open menu
  is; without this the tip floats over the list.

### Fixed

- **The ◀ ▶ arrows walked the alphabet while the menu showed categories.**
  `PresetManager`'s `prevButton` / `nextButton` options bind to the native
  `selectNextPreset` / `selectPreviousPreset`, which walk
  `OuariconPresetManager::getPresetList()` — one case-insensitive alphabetical
  list. Every plugin's **flat** dropdown renders that same order, so the
  buttons and the visible list have always agreed **by coincidence, not by
  construction**, and grouping this one broke the coincidence with nothing to
  warn you: ▶ from "Thrift-Store Turntable" (first in Vinyl) landed on "Total
  Media Failure" (seventh in Showcases), the highlight jumping backwards
  across two sections. The options are now deliberately **not** passed, and
  the arrows step a `presetWalkOrder` derived by flattening the rendered
  sections — so the walk order cannot describe a different sequence from the
  list. An out-of-list preset ("Default" on a fresh instance, or one loaded
  from a file) enters at the top going forward and the bottom going back
  rather than snapping to index 0 both ways.
  (`pattern_grouping_preset_dropdown_breaks_prev_next`, found first by
  O-MultiBandCompressor v1.7.0.)
- **The preset name was 7.5px left of its plate's centre.** The caret is a flex
  child, so the name centred on what was left after it rather than on the plate
  — visibly off beside the symmetrical Save / Load / Delete row. A spacer of
  the caret's exact width now sits opposite it; measured at **0.00px** and
  asserted at a 1.0px limit, because an offset this size reads as sloppy rather
  than as a bug and would never be caught by eye in review.

### Notes

- Bumping the version rewrites the 28 factory preset files — writes are
  sentinel-gated on `JucePlugin_VersionString`. No values changed, so they
  rewrite identically.
- **Every assertion in the new gate was checked in both directions.** Breaking
  a category span fails 3 checks; making the menu items inert fails 2; removing
  the centring spacer fails 1 at the measured 7.50px; restoring the module's
  arrow binding fails 2, reproducing the exact "Total Media Failure" landing.
  A probe that passes with the fix reverted is decoration
  (`pattern_probe_must_target_the_branch_the_fix_changed`).
- **The arrow check had to be made positional to catch anything.** The first
  version asserted only that the highlight *moved*, which passes even when the
  arrows walk a completely different order — and it did pass, on the broken
  build, printing the wrong landing in its own output. It now asserts ▶ from
  item *i* lands on item *i+1* of the rendered list, that ◀ is its exact
  inverse, and that ▶ from the last item wraps to the first.
- Gates: render harness **109/109**, tooltip gate **53/53 anchors**, preset menu
  gate **33/33**, auval PASS, pluginval strictness-10 SUCCESS, zero console
  errors.

## [1.12.0] — 2026-08-18

Hover help on **53 controls** and a header **"?"** toggle, with the preference
persisted in the session state. Ported from the pattern shipped in O-Tapestop
v1.4.0.

**This release changes no DSP and no parameter.** Nothing under `Source/dsp`
moved, the layout is the same 45 IDs in the same order, and no preset's values
changed. Every saved session and preset renders exactly as under v1.11.0.

### Added

- **A `?` toggle in the header**, and a hover-help layer over every control:
  the 7 family enables, 30 knobs, 4 segmented switches, 2 selects, Hard Edges,
  the seed ledger and its die, the 6 preset-band controls, and the toggle
  itself — **53 anchors**, which is every hoverable control on the page.
- **The preference survives a session reload.** It is **not a parameter**: an
  `AudioParameterBool` would appear in every host's automation lane and in
  every one of the 28 factory presets, and would move the layout off the 45 IDs
  every saved session is keyed to. It rides the APVTS state tree as a plain
  property and round-trips through two new native fns, taking the registered
  surface from 10 to **12** (parity gate 12↔12, clean both directions).
  `getTooltipsEnabled` is **pulled** by the page at init, never pushed — a push
  from the editor constructor or the 30 Hz tick fires before the inline module
  has evaluated, so it would silently never arrive and the toggle would read
  OFF on every reopen (the O-FreqPulse WR-01 bug, avoided by construction).

### Changed

- **`.panel.off .p-body` no longer blanket-blocks pointer events.** The block
  moved onto the interactive elements themselves (`.knob`, `.seg`, `select`,
  `button`). `pointer-events: none` suppresses `mouseover` along with dragging,
  which left the four families that ship OFF — Packet, Codec, Crush and Rot,
  **18 of the 53 anchors** — unable to raise a tip at all. That is exactly
  backwards: a control's description is most wanted *before* its family is
  switched on. Equivalent for interaction — everything the old rule made inert
  is still inert, and the cursor is unchanged, because `.knob`'s `ns-resize`
  does not apply while the knob itself is `pointer-events: none`.
- The `?` is a **fourth flex child of `.hdr`**, not an absolute overlay.
  O-Tapestop needed `position: absolute` because its header is a centred title;
  O-Bitrot's is `justify-content: space-between` with roughly 160 px of slack,
  so flow placement costs the layout nothing and keeps clear of
  `.corner-fleuron.tr`, which owns the frame's top-right corner. Measured, not
  assumed: `.plugin` still renders 900×740, the page does not scroll, and the
  header's four children still read left-to-right with no overlap.

### Fixed

- **The restore guard.** `getStateInformation` writes a bool var, so
  `isBool() || isInt()` reads as the obviously-correct test. It is wrong: the
  XML round-trip does not preserve the type —
  `NamedValueSet::setFromXmlAttributes` rebuilds every property as
  `var (value)` over the attribute **string** — so what returns is a var
  holding `"1"`, every type test is false, and the preference would have
  restored as OFF forever while build, auval and pluginval all passed. Fixed to
  `!isVoid()` and gated by probe `T1`.

### Testing

Two defects were found by writing the probes rather than by reading the code,
and **both were reverted and re-run in each direction** — a probe that passes
with the fix reverted is decoration
(`pattern_probe_must_target_the_branch_the_fix_changed`):

1. The `!isVoid()` guard above. Against `isBool() || isInt()`, `T1` reports
   `restored=false`.
2. **The clamp check did not discriminate on its own.** Reverting `showTip()`
   to the naive measure-at-previous-offset form left **all 53 anchor
   assertions passing** — most tips hit the 230 px `max-width`, and the
   horizontal clamp's fixed point at 900 px leaves exactly 238 px, so the
   collapse can never start. That safety is a property of the **copy**, not of
   the code. A stress stage now manufactures the condition: short copy on the
   right-most control places a tip at left=812.4, then long copy back renders
   **230.0 px with the fix and 95.6 px without**.

New: `tests/ui-stub/` (all 45 params with both skews **derived** from the same
`setSkewForCentre` call rather than transcribed, 6 choice lists, 8 toggles, the
backend event bus, all 12 native fns) and `tests/ui_tooltip_clamp_check.js` —
the browser render gate this plugin did not have.

Gates: **53/53 anchors** measured at the real 900×740 across the shipped state
and both clock-slot positions; **52 of 53 reachable in the shipped state**,
which is the assertion that the `.panel.off` fix holds (only the swap-slot pair
may defer); edge clamp actually engaged for **12** controls; right-most tip (the
`?`) ends at **892.0 of 900** — exactly the 8 px limit. Render harness
**109/109**, 0 failures (107 pre-existing DSP probes untouched + 2 new). auval
**PASS**, pluginval strictness-10 **SUCCESS**, zero console errors.

## [1.11.0] — 2026-08-18

The **factory bank, 9 → 28 presets** — improvement brief item 31, and the last
item the brief schedules on purpose ("schedule LAST so presets exercise the
v1.2/v1.3 features"). Ten releases of DSP have landed since the bank was
authored at Stage 4, and the bank had been growing by exactly one preset per
release — the new family's showcase — while everything else the releases added
was reachable only by building a patch by hand. For a 45-parameter stochastic
plugin the presets *are* the discoverability layer, and a nine-preset bank was
not one.

**This release changes no DSP and no parameter.** Not one line under
`Source/dsp` moved, the layout is the same 45 IDs in the same order, and no
existing preset's values changed. Every saved session and every saved preset
renders exactly as it did under v1.10.0. The one on-disk effect is the module's
version-stamped sentinel: the nine existing factory `.json` files are rewritten
with identical parameter values and a `"version": "1.11.0"` stamp.

### Added

- **Nineteen factory presets**, organised by media narrative:
  - **Tape** — "Warped C-90" (the brief's own example: a dashboard mixtape,
    heavy warble and bare oxide), "Basement Reel", "Chewed Tape",
    "Dictaphone Memo".
  - **Vinyl** — "Thrift-Store Turntable" (the brief's example), "Dusty 45",
    "Shellac 78".
  - **CD** — "Scratched CD-R", "Stuck Disc".
  - **Phone and network** — "Last Voicemail" (the brief's example),
    "Answering Machine", "Transatlantic Line".
  - **Rot** — "Bad Blocks", "Wrong Byte Offset", "Frozen Decoder": one per
    v1.10.0 kind, with the share ladder set to isolate it (flip-only is
    `ROT_STICK` 0 + `ROT_GARBLE` 0; the other two push one share to 90–95).
  - **A four-rung severity ladder** — "Disintegration Loop I–IV", the brief's
    named example. Every axis climbs together across the four: tick rate,
    event probability, dropout share, wow depth, hiss, and from rung II on,
    digital rot underneath the tape.
- **Curated `SEED` per preset**, so the demo glitch pattern ships *with* the
  preset rather than being whatever the last patch left behind. All 28 seeds
  are distinct except the Disintegration ladder, which deliberately shares one
  (6060). The four do not render the same pattern — their parameters differ, so
  their RNG consumption diverges inside the first tick — but they start from one
  stream, which is the narrative: one tape, four stages of decay.

### Changed

- **The bank now exercises every choice value on every choice parameter.** It
  did not before. Newly reached by a factory preset for the first time:
  `VINYL_RPM` **"45"** (Dusty 45) and **"78"** (Shellac 78) — all nine previous
  presets sat on 33 1/3, so the exact value v1.7.0's preset-migration gate
  exists for had no factory coverage at all; `PACKET_CONCEAL` **"Substitute"**
  (Last Voicemail); `CODEC_MAINS` **60 Hz** (Last Voicemail); and the
  `CLOCK_SYNC_DIV` rungs **1/8T, 1/4T, 1/2 and 1 bar**. Both clock modes, both
  codec modes and both `HARD_EDGES` states are covered.
- **Three continuous parameters come off their rails for the first time.**
  `CODEC_AGC` was 100 in all nine previous presets — Answering Machine sets 45
  and Transatlantic Line 80, so v1.8.0's AGC is now audible as a *range* rather
  than a fixed part of the codec. `CODEC_MIX` below 100 (Transatlantic Line, 75)
  and `MIX` below 100 (Wrong Byte Offset, 65) give the bank its blend examples.
  `CRUSH_ENV_AMT` gets its first positive value (Shellac 78, +30 — crushing
  harder on peaks, which is what groove distortion does).
- **`CRUSH_RATE` finally lands mid-skew.** The previous bank used 20000, 11025
  and 6000; Dictaphone Memo sits at 5000 and Shellac 78 at 4500, near the
  geometric centre the range is skewed around (3162 Hz).

### Notes

Three constraints shaped the authoring, all of them properties of code that
already existed:

- **The nine existing names are carried forward verbatim.**
  `initializeFactoryPresets()` writes files and never prunes, so renaming a
  factory preset would strand its old `.json` in the Factory directory
  permanently and the bank would list both. Adding is safe; renaming is not.
  This is why the bank is not prefixed by family.
- **Declaration order here is not browse order.** `getPresetList()` ends with
  `presets.sort(true)`, so all 28 present alphabetically no matter how this
  vector is ordered. The media-narrative grouping lives in the *content*. With
  28 presets reachable only through the header's prev/next arrows, a preset
  browser is the natural follow-up and is filed as such — it is a UI change, not
  a content one, and this release is content.
- **Names are ASCII-only and slash-free.** `juce::String`'s `const char*`
  constructor is ASCII-only, and `sanitizePresetName()` rewrites `/` to `_` — a
  preset name *is* a filename, so a slash would silently change the file the
  preset round-trips through. Hence "Disintegration Loop I-IV" with a hyphen.

`TAPE_PROB` 0 with `TAPE_ENABLE` 1 (Basement Reel) is deliberate, not an
oversight. The beds are gated by their family's ENABLE, not by its probability
(`PluginProcessor.cpp`, the `tapeBed`/`wowFlutter` snapshot), so that
combination is the continuous wow-and-hiss layer with the discrete tape events
switched off — v1.4.0's and v1.5.0's work in isolation, which nothing in the
bank previously demonstrated.

### Testing

Render harness **107/107**, unchanged and expected to be: no probe reads the
factory bank, and no DSP moved. The cross-version anchors (A3×3, V1, N7, N8)
still match their recorded digests. Structural audit of the bank, run against
the parsed source: 28 presets, all 45 parameter IDs present in each with no
duplicates, every authored engineering value inside its parameter's
`NormalisableRange` (an out-of-range literal would be silently clamped by
`convertTo0to1` rather than rejected), every choice/bool literal an exact
integer, all names ASCII and slash-free, no name collisions, no unintended seed
collisions. pluginval strictness 10 and `auval` pass.

## [1.10.0] — 2026-08-18

The **Rot family** — improvement brief item 8, and the plugin finally doing
what its name says. Research §3.5 lists four corrupt-file mechanisms and the
engine shipped exactly one of them (buffer shuffling, via the read-head
machinery); a grep for `xor`, `bitflip`, `sticky` or `byte` across `Source/dsp`
returned nothing. Meanwhile BRIEF.md:12 claimed "the full physical-to-digital
spectrum of broken playback". This release closes that gap with the other
three, as a seventh clocked family:

- **Flip** — a rate-limited window over which individual samples are XORed in
  the 16-bit domain and clipped. One flipped bit is one impulse: low bits are a
  faint granular crackle, the sign bit is a full-scale spike. A PCM file with
  bad blocks.
- **Stick** — the decoder hangs and one sample is held for 10–80 ms. A short DC
  plateau, which is what a stalled playback pointer actually emits.
- **Garble** — a wrong-decode stretch of 30–300 ms: the decoder is reading at
  the wrong byte offset, so it emits garbage *at the programme's own level*.
  The envelope match is the artifact — noise at a fixed level reads as a broken
  plugin, noise that swells and ducks with the music reads as a corrupt file.

No competitor in the README's market snapshot ships wrong-decode stretches.

**This release is bit-identical to v1.9.0 for every existing session and
preset.** `ROT_ENABLE` defaults off, and the rot gate short-circuits *before*
its RNG draw while disabled, so the family costs nothing, perturbs no stream
and touches no sample until it is switched on. That is asserted with no
tolerance (probe R5) and confirmed by every cross-version anchor — A3×3, V1,
N7, N8 — still matching its recorded digest.

### Added
- **`RotStage`** (`Source/dsp/RotStage.h`) — the three kinds above. Pure
  **OVERLAY** class in the v1.9.0 arbitration vocabulary: rot changes no head
  position and no transport rate, so it never enters the ownership contest and
  never touches the `arbitration` stream. It is the first family that is
  overlay-only, which is what makes adding it a pure append rather than a
  rewrite of the contest.
- **Five parameters, appended to the end of the layout** — `ROT_ENABLE` (off),
  `ROT_PROB` (25%), `ROT_DEPTH` (50%), `ROT_STICK` (25%), `ROT_GARBLE` (25%).
  Appended for the fifth time for the reason every previous append gave: layout
  order *is* the automation-slot order, so inserting a block of five anywhere
  but the end would silently repoint every saved automation lane behind it.
  `ROT_STICK`/`ROT_GARBLE` are a share ladder in the same shape as the tape
  family's stop/dropout shares — sticky is tested first, garble takes a share
  of what is left, and whatever survives both is a flip window.
- **`RngBank::rot`** — one new stream, appended. Unusually it is drawn at both
  tick instants and per sample, which is the shape `scratch` was split out to
  avoid. It is safe here because rot is a *single subsystem* whose draws all
  happen inside one per-sample loop iteration in fixed order, making the whole
  sequence a pure function of the absolute sample index. Probe R4 measures that
  rather than trusting it.
- **Tab. VII — Rot**, a full-width plate between the family grid and the global
  strip, with its own spore-print LED on activity bit 4. The window grew
  620 → 740 to hold it. A four-column grid would have kept the height but
  squeezed every plate from 274px to 209px, which the vinyl RPM switch, the
  packet conceal dropdown and the two codec switches do not fit inside; every
  existing panel is now pixel-identical to v1.9.0. Global is renumbered
  Tab. VIII.
- **"Corrupt Archive"** factory preset — the rot family's showcase, which is
  what keeps the bank's one-showcase-per-family claim true rather than
  aspirational. The bank is now 9 presets × 45 parameters.

### Changed
- **"Gentle Rot" and "Total Media Failure" now use the family they were named
  for** — light (prob 15, depth 30) and severe (prob 70, depth 85)
  respectively. The other six factory presets carry the five new IDs at
  `ROT_ENABLE` 0 and render exactly as they did under v1.9.0.
- **The flip bit field includes bit 15, the sign.** This was excluded in the
  first cut and it was wrong twice over: physically, a corrupt block does not
  know which bit is the sign; numerically, excluding it made the brief's
  "post-clip" unreachable, because a single flip of bit ≤ 14 on a word already
  inside ±32767 can never leave ±32768. The clip had nothing to do, and the
  probe that claimed to gate it **passed with the clip deleted**. With bit 15
  in range the XOR reaches ±2.0 FS and deleting the clip takes the measured
  peak to 1.5 — see "Negative controls".

### Render-affecting
Only with `ROT_ENABLE` on. Everything else is bit-identical, by construction
and by measurement.

### Testing
Render harness: **107/107 probes pass**, up from 102. Five new probes, all
running with tape/CD/vinyl disabled so the wet path is a pure integer delay and
the residual against `input[n - kComp]` is the rot bus with nothing else in it:

| Probe | Measured |
|-------|----------|
| R1 flip impulses | depth 100: 2832 samples > 0.0625 from reference; depth 0: **0**; peak exactly 1.0000 |
| R2 sticky hold | longest bit-identical run 1199 samples (25.0 ms), inside the specified 10–80 ms; STICK 0 control: 1 |
| R3 garble env-match | 293 garbled windows, median loud/quiet RMS ratio **10.00** against inputs exactly 10× apart |
| R4 blocksize identity | {512} == {4096} == ragged, bit-identical with all three kinds live |
| R5 rot-off null | bit-exact null with all four ROT knobs at 100 |

### Negative controls
Every new probe was verified to **FAIL** against the code it gates, by
reverting that one behaviour and re-running:

| Reverted | Result |
|----------|--------|
| post-clip in `flipSample` | R1 FAILS — peak 1.5000, "THE XOR OVERFLOWED" |
| sticky blend forced to 0 | R2 FAILS — longest run 1 sample |
| garble amplitude fixed at 0.35 | R3 FAILS — ratio 1.00 instead of 10.00 |
| one extra `rot` draw per *block* | R4 FAILS — "RENDERS DIVERGE" |
| gate ignores `ROT_ENABLE` | R5 FAILS — first mismatch @24960 |

R1's first version did **not** discriminate — it passed with the post-clip
deleted, which is what exposed the bit-15 design error above. The probe and the
implementation were both corrected; the entry is here rather than quietly fixed
because a probe that passes both ways is decoration.

## [1.9.0] — 2026-08-18

Overlay-class arbitration — improvement brief item 6. Until now exactly one
family could act per tick: every win called `release()` on the other two, and
a tape+vinyl double-fire dropped one event outright. That is the right rule
for a transport and the wrong one for everything else. Real broken media fail
in parallel — research §4.2's RSBrokenMedia rolls its categories
independently — and the engine already half-admitted it, because
`CDSkip::release` only ever touched the `Loop` state, so a bounded
conceal/mute rung could *finish* under a tape win even though it could never
*start* under one.

Events are now split into two classes:

- **OWNER** — drives the transport: a rate (tape stop, tape bend) or a head
  position (CD loop, vinyl jump, vinyl locked groove). There is one read head,
  so these still contend **single-winner**, picked uniformly by the
  `arbitration` stream exactly as before. A coherent playback position is the
  entire reason the arbitration exists.
- **OVERLAY** — gain, filter and artifact domain, applied orthogonally to
  position and rate: the CD conceal dip, the CD mute + residual tick, the tape
  dropout, and the vinyl standalone pop. These fire **whenever their family's
  roll succeeds, regardless of who owns the tick.**

This is what makes the "Total Media Failure" preset sound like everything
failing *at once* rather than like a queue.

**This release changes the render on ticks where two or more families fire.**
It changes nothing on any other tick, and that boundary is proven rather than
asserted — see "Render-affecting". No parameter was added, removed, renamed or
re-ranged, and the state format is untouched, so every saved session and preset
loads unchanged.

### Added
- **Standalone vinyl pop** — the vinyl family's overlay residue, and the one
  genuinely new event here. A vinyl roll that succeeded and then lost the tick
  used to produce *nothing*, which is not what the physical model says
  happened: the stylus met the debris either way, and the only difference is
  whether the groove skipped. A lost roll now fires the pop alone
  (`VinylTransport::applyStandalonePop`) while the transport stays wherever the
  winning family put it. It draws the same five `artifactSynth` values every
  other pop takes, so it inherits the v1.7.0 tick/pop/scratch taxonomy for
  free — this is surface crackle with variety, not one sample retriggered.

### Changed
- **The CD conceal and mute rungs are overlays** (the headline). They are
  filter and gain domain, applied to the rendered pair and orthogonal to head
  position, so nothing about them ever needed to own the transport. They now
  layer under a tape bend or a groove jump instead of waiting for a tick the CD
  family happened to win.
- **The tape dropout is classified as an overlay explicitly.** It was already
  one in substance — v1.4.0 installs no rate event, so a bend keeps ramping
  underneath it — but it still required tape to *win* the tick. It now needs
  tape only to *roll* one, and layers under a CD loop or a groove jump the same
  way it always layered under a bend.
- **Kind rolls moved out of the winner's branch.** Each family now classifies
  every firer through a pure `rollKind`/`rollRung` step, then applies. That
  split is what made overlays reachable at all: the sub-rolls used to live
  inside the winner's `case`, so a losing family never even decided what its
  event *was*. `CDSkip::onWin` became `rollRung` + `applyLoop` + `applyOverlay`;
  `VinylTransport::onWin` became `rollKind` + `applyOwner` +
  `applyStandalonePop`; the tape stop/dropout/bend roll became
  `Arbitration::rollTapeKind`.
- **Releases are class-aware.** A family yields its transport state when it does
  not own the tick — *unless* it fired an overlay and nobody owns, in which case
  there is nothing to yield to and whatever it was already doing keeps running.
  That exception is not new: v1.8.0 already left a bend ramping under a
  tape-dropout win. This is the general form of that rule.
- The `arbitration` stream's draw is now taken when there is more than one
  **owner**, rather than more than one firer.

### Fixed
- A CD overlay landing while its own loop runs exits that loop through
  `recoveryJump`, never through `release()`/`endLoop`. `endLoop` can enter the
  100–400 ms servo-seek stage (v1.6.0, item 18), which would have swallowed the
  very rung being installed. The old code had this right for a rung change and
  it had to survive the split — a conceal arriving under a foreign owner is the
  same situation seen from the other side. `Arbitration` therefore does not call
  `cd.release()` on a family that fired an overlay.

### Render-affecting
Yes, on multi-firer ticks, and provably nowhere else.

The mechanism is confined by construction. A tick with **at most one firer**
consumes the same draws, makes the same release calls and installs the same
event it did at v1.8.0: a lone firer either owns the tick or is a lone overlay,
and either way the two silent families release exactly as the old winner
released them. So single-family renders — and multi-family renders on ticks
that happen not to collide — are bit-identical.

That is asserted, not hoped for. New probe **`A3`** renders three
single-family configs (tape with `TAPE_DROP 40` so the conditional dropout draw
and the bend-interval draw are both in play; CD at severity 0.5 so all three
rungs are reachable; vinyl with both kinds) and pins each against a digest
recorded by compiling that probe against the **v1.8.0** tree at git `627f8afb`
**before the first v1.9.0 edit** — the only ordering under which a
cross-version digest means anything. All three match.

**`V1` moved on purpose and is re-anchored a third time.** Its config runs
`TAPE_PROB 100` against `CD_PROB 60`, so ~60% of its ticks have two firers, and
at `CD_SEVERITY 0` the rung roll can only ever reach rung 0 — CD's event there
is *always* a conceal, i.e. always an overlay. Under single-winner arbitration
that conceal landed on roughly half the collision ticks; it now lands on all of
them. The digest went `0x44a5de77d572facd` → `0x70e744c93cbcc2a3`. The v1.3.0
number is retained and asserted **negatively**, the discipline `N7` uses: a
build where the overlay split silently failed to land reads as a failure rather
than as a pass. The probe is re-purposed as a forward anchor pinning the v1.9.0
collision render.

`N7` and `N8` are untouched and pass unchanged — `configureCanonicalPostRender`
disables all three transport families, so the packet and codec chains never see
this edit.

Block-size invariance and seeding are unaffected: overlay decisions are drawn at
tick instants in the fixed order tape → cd → vinyl, never per-block and never
per-sample on a shared stream (`pattern_rng_stream_interleave_blocksize`). `N`
still renders bit-identically at `{1, 7, 64, 333, 4096}` against `{512}` with
all three families at 100%.

### Testing
102/102 render-harness probes pass (up from 97). Three are new, and the two
behavioural ones were **run against the v1.8.0 tree first and fail there** —
without that they would be decoration rather than gates
(`pattern_probe_must_target_the_branch_the_fix_changed`).

- **`A1 cd-overlay-under-foreign-owner`** — `VINYL_PROB 100` (vinyl owns every
  tick; chosen over tape because a groove jump leaves the read rate at exactly
  1.0 and so cannot tilt a noise spectrum and confound the measurement) against
  `CD_PROB 100` at `CD_SEVERITY 0`, where the rung roll is forced to conceal.
  Counts tick periods containing an HF-ratio dip. **31/31** at v1.9.0;
  **12/31** at v1.8.0.

  One trap worth recording: the first draft rendered 8 s and read 0/31 with a
  median HF ratio of **0.00000**. `VINYL_PROB 100` is punishing on the
  transport — every backward groove jump ages the head a whole revolution while
  the write head advances one tick period, so the lag walks out to the ring's
  budget within seconds and parks there, and early in a render the material 8–9 s
  behind live is the ring's *pre-history*, i.e. zeros. The dip bar was being
  applied to silence. The probe now renders 20 s, starts its scan at 12 s, and
  gates on the scan region's own magnitude **and** on the median being
  non-degenerate, so that failure cannot re-present itself as a pass.
- **`A2 standalone-vinyl-pop`** — `TAPE_PROB 100` with `TAPE_STOP_PROB 0` (every
  tape event a bend, so tape owns every tick) against `VINYL_PROB 100`. Twin
  renders differing only in `VINYL_POP` (50 vs 0), which isolates the pop bus
  exactly, because `triggerPop` consumes its five draws at level 0 too and the
  head schedule is therefore identical between them. **30/31** tick periods
  carry a pop at v1.9.0; **12/31** at v1.8.0.
- **`A3 single-firer-identity`** — the containment gate described above.

## [1.8.0] — 2026-08-17

Codec chain depth — improvement brief items 16, 29 and 7. All three are the
same complaint from three angles: `CodecStage` was a *filter chain with a
quantizer in it*, not a telephone link. It had no dynamics, its mu-law was a
curve rather than the table the standard specifies, and it never lost a
frame — the packet stage dropped PCM *before* the encoder, which is not how
cellular loss works and not what it sounds like.

**This release changes the render of the codec family whenever
`CODEC_ENABLE` is on.** It changes nothing anywhere else, and that claim is
measured rather than asserted (see "Render-affecting"). `CODEC_ENABLE`
defaults to off, so a v1.7.0 session that never opened the Codec panel is
bit-identical.

### Added
- **`CODEC_AGC` — the missing half of the phone chain** (item 16). Research
  §3.3's chain is "μ-law OR GSM → fast ~4:1 comp (AGC) → optional noise
  bed", and `CodecStage` ended at the post-LPF with no dynamics anywhere.
  The AGC's pumping is a large part of why a phone sounds like a phone;
  without it a quiet source stays quiet instead of being dragged up into the
  codec noise. A feed-forward compressor now sits **after the mode blend and
  before `post1`**: per-sample one-pole envelope, **1 ms attack, 50 ms
  release, −20 dBFS threshold, 4:1, fixed +10 dB makeup**. Measured on a
  1 kHz tone: a −26 dBFS source is lifted **+10.0 dB**, a −6 dBFS source is
  held to **−0.45 dB** — a 10.45 dB spread, which is the stage. Unity lands
  near −6.7 dBFS.

  The new knob is a **depth**, not a switch: gain is assembled in dB and
  scaled there, so `CODEC_AGC 0` is `exp(0)` — exactly `1.0f`, a
  bit-transparent multiply that restores the v1.7.0 gain structure to the
  bit. Default **100**: on with the section, as the brief asks.

- **Codec-domain GSM frame loss** (item 7). The chain order is Packet →
  Codec, so until now PCM was lost *before* encoding. Real cellular loss
  drops **encoded** frames, and the decoder conceals by repeating the last
  frame's parameters with attenuation — the metallic warble everyone knows
  as bad reception. With **GSM and PACKET both enabled**, a lost frame now
  re-feeds the decoder the previous frame's *bytes*. The encoder still runs
  on the source frame, because it lives at the far end of the link and loss
  happens in transit; the decoder's own LTP history keeps evolving on each
  repeat, which is where the warble comes from — it is not a buffer being
  looped. Per-repeat attenuation is **−3 dB**, ramped across the frame it
  applies to (a step every 20 ms would click), with a hard mute at **16
  consecutive frames — 320 ms, GSM 06.11's own mute point**.

  The PCM-domain stage keeps running underneath for the codec-off / VoIP
  case. The two do not stack: during a lost frame the GSM decoder's output
  does not depend on its input at all, so the codec repeat *masks* whatever
  concealment the packet stage produced for that frame.

### Changed
- **Mu-law is now the segmented G.711 codec** (item 29). `muLawRoundTrip`
  was the continuous `log1p`/`pow` curve with a uniform companded-domain
  quantizer; real landlines quantize on **8 piecewise-linear chords of 16
  steps**. This is the ITU-T G.711 / Sun reference encoder (14-bit domain,
  bias 33, clip 8159) against a 256-entry decode LUT built at compile time
  from the 16-bit-domain bias 132. Pure lookup — RNG-free, zero-latency, so
  the mu-path alignment ring is untouched.

  Two landmarks are properties of the standard, not normalisation choices:
  digital silence encodes to `0xFF` and decodes to **exactly 0.0f**, and
  full scale encodes to `0x80` and decodes to **32124/32768 = −0.17 dBFS**,
  which is where a real landline clips rather than at 1.0. The audible
  difference is the sub-−40 dB tail fizz the brief predicted; the clip point
  is the part you can measure.

### Notes on how these were built

**The two 20 ms grids are coupled by lookup, not by counting.** The packet
grid is `ceil(0.020·fs)` and a GSM frame is 160 slots of the 8 kHz latch —
960 samples each at 48 kHz, nominally identical. They are still not
interchangeable: the latch is **fractional**, so the frame boundary rides
its crossings and is phase-offset from the packet grid, and any scheme that
indexes loss by counting packets drifts against it at other sample rates and
block sizes. Instead the processor hands `CodecStage` the packet's loss flag
on **every** sample and the stage consumes whichever value arrives on the
sample that closes a frame. The flag is read *before* `PacketLossStage::
processSample`, because that call advances the grid at its end — read after,
the value on a boundary sample already describes the *next* packet.

`C4` is the probe this design exists for: the coupled GSM + packet-loss
render is bit-identical at block 512 and at ragged 1/7/64/333/4096. A shared
counter passes `C3` at a fixed block size and fails here.

**The AGC envelope is primed, not zeroed.** A 1 ms attack meeting a cold
envelope sits at full makeup, so the codec's first signal — which arrives
~20 ms in, past the enable fade, and therefore *after* the fade could cover
it — met the whole +10 dB and overshot to **+3.7 dBFS** on a −6 dBFS tone.
The envelope now starts at its unity-gain level (10^((−20 + 10/0.75)/20)),
which makes a cold start flat and lets the AGC *release* up into its makeup
over 50 ms if the material is quiet. What remains is the ordinary compressor
transient — **+1.7 dB**, fed by the 20 ms of structural-delay silence the
envelope releases through before signal arrives. `C1` bounds it at +3 dB, a
number chosen as a principle rather than fitted to the measurement: an
unprimed envelope scores +9.2 dB and fails hard.

Re-entry after a PLC mute still overshoots, and that one stays: it is the
compressor doing its job on a real 60 dB step, and it is part of how a line
coming back sounds.

**The AGC sits downstream of the frame loss, and partly fights it.** A −6 dB
PLC step above threshold comes back ~4.5 dB smaller after 4:1, so the
attenuation ramp is shallower than −3 dB/repeat on loud material. This is
kept rather than worked around — a real handset has exactly this
arrangement, and the mute at 16 frames is absolute regardless.

### Migration
None required. `CODEC_AGC` is a **new** parameter appended at the end of the
layout — the fourth release running, and for the reason the previous three
state: layout order is the automation-slot order a host presents, so
slotting it into the CODEC block where it belongs visually would repoint
every saved automation lane behind it. The WebView puts it in the Codec
panel regardless, because the UI is keyed by parameter ID. No existing
parameter's range, type or choice list moved, so no preset-migration gate is
needed and none was added.

Unlike the v1.4.0, v1.5.0 and v1.7.0 appends, this one does **not** default
to its transparent value. Stated plainly: a v1.7.0 session or preset with
`CODEC_ENABLE` **on** renders differently in v1.8.0 — from the AGC, and
independently from item 29's segmented mu-law, which has no opt-out at all.
Defaulting `CODEC_AGC` to 0 would have preserved nothing, because item 29
moves the same renders either way. `CODEC_AGC 0` restores the v1.7.0 gain
structure exactly for anyone who wants it.

### Render-affecting
Yes, for the codec family, and only there. Both halves of that were
measured.

`N7` — the canonical post-render (packet on at 45% loss, codec on, mu-law,
`CODEC_MIX 100`) — **moved on purpose**, by all three items at once. It was
introduced at v1.5.0 carrying `0x1cf2f80d1f71674c`, produced against the
**v1.4.0** tree at git `2160dd66`, and it held through v1.5.0, v1.6.0 and
v1.7.0. That claim is now **retired rather than re-fitted**: the probe is
re-anchored to `0xb473105611e3ea78` and re-purposed as a *forward* anchor
pinning the v1.8.0 codec chain for every release after this one. It also
asserts the digest is no longer the v1.4.0 one, so a build where items 16
and 29 silently failed to land reads as a failure rather than a pass.

The half of the old claim that survives untouched is now carried by a new
probe, **`N8`**, which runs the identical config with `CODEC_ENABLE` **off**
— exercising the GE chain, all four concealment modes' machinery, comfort
noise and the entire transport engine while `CodecStage` sits on its
bit-transparent bypass rail. Its anchor `0x8eb6e29da5ec2692` was recorded
against the **v1.7.0** tree at git `4d52377e` **before the first v1.8.0
edit**, which is the only ordering that makes a cross-version digest mean
anything. It passes: **the codec edit did not leak outside the codec.**

### Testing
97/97 render-harness probes pass (up from 93). Four are new:

- **`C1 codec-agc-compresses`** — band gain at 1 kHz between `CODEC_AGC` 0
  and 100 on the same input, so the codec's own colour cancels and only the
  compressor survives. +10.00 dB on a −26 dBFS tone, −0.45 dB on a −6 dBFS
  tone, 10.45 dB spread, peak bounded at +3 dB over the AGC-off render. At
  `CODEC_AGC 0` both gains are exactly 0 dB, so plumbing that leaves the AGC
  out of circuit reads as 0/0 rather than as a near miss.
- **`C2 mulaw-segmented-chords`** — reads the encoder and decode table
  directly rather than inferring them from a render, because the post-LPF
  smears the quantizer lattice and a signal-domain probe could only ever
  measure "something changed". Asserts the structure that makes G.711 a
  segmented law: 8 chords of 16 **uniform** steps, each chord's step
  **exactly double** the one below, plus the two landmarks. A continuous log
  curve has neither uniform runs nor exact doublings, so this separates the
  implementations rather than merely detecting an edit.
- **`C3 gsm-frame-loss-mutes`** — GSM at 90% loss with `PACKET_CONCEAL` set
  to **Repeat**, which replays the last good packet verbatim and never
  decays, on a constant-amplitude 1 kHz tone. The PCM stage therefore feeds
  the encoder a full-level signal for the entire render, and silence can
  come from one place only: the new mute. 9562 samples (~200 ms) of it, with
  the render otherwise peaking at 0.96 — and **zero** silent samples in the
  same config with `PACKET_ENABLE` off, which is the negative control that
  keeps the result attributable to the coupling.
- **`C4 frame-loss-ragged`** — described above.

One earlier finding is recorded because the config that produced it is worth
knowing about: at `PACKET_LOSS 100` the GE chain gives `pGB = 1` and
`pLossBad = 1`, so per-packet loss is ~98.75% and the codec sits past its
mute point essentially forever — the line is dead, which is correct for
total failure but makes a useless probe. `C3` runs at 90% for that reason.

## [1.7.0] — 2026-08-17

Vinyl authenticity — improvement brief items 17 and 27. Three of the four
changes here are about the same underlying complaint: the vinyl family was
built from a *single* physical gesture repeated. Every pop was the same
doublet with a random gain. Every forward skip teleported to the present,
whatever the stylus had to work with. The disc itself was perfectly flat and
spun at one of two speeds. Item 17 gives the pop a taxonomy, item 27 gives
the disc physics — a second direction for the groove jump, a warp, and a
third speed.

**This release changes the render of the vinyl family at its defaults.**
`VINYL_POP` defaults to 50, so the pop taxonomy is live out of the box, and
forward revolution-quantization needs no parameter at all. No other family
moves a bit. See "Render-affecting" for what is preserved and how that was
established.

### Added
- **Pop taxonomy: tick / pop / scratch** (item 17). A one-pole cannot ring,
  so no pop in the first six releases had the damped stylus resonance that
  reads as "the stylus hit something" — the whole family was one sample with
  a random gain and ±3 dB / 1–3 kHz of variation. `triggerPop` now draws a
  class at ~**70/25/5**:
  - **tick** — the historical doublet through the `FirstOrderTPT` lowpass,
    cutoff biased **high** (2–4 kHz), so it reads as the dry surface-debris
    click it always was;
  - **pop** — the same doublet exciting a damped 2nd-order resonator
    (`StateVariableTPT` bandpass, 900–1800 Hz, Q 4–8);
  - **scratch** — a 2–8 ms band-limited noise burst under a raised-cosine
    window, plus a 40–80 Hz decaying sine: the tonearm thump under the rasp.

  The brief specified "rings 1–3 ms" alongside "Q 4–8", and those two are
  not compatible: Q 4–8 at 900–1800 Hz decays to −30 dB in **2.4–9.8 ms**
  (measured across the corners of the (f0, Q) box, not asserted from the
  spec). Q is the parameter that was specified, so Q is what shipped, and
  the ring time is reported here rather than restated from the brief. The
  click peak is normalised to the same base level for all three classes —
  `1/g` is within 3% of the true doublet-response peak across the whole box
  — so the ring sits 7–13 dB under the transient, which is the shape a
  stylus impact actually has.

- **`VINYL_WARP`** (item 27b) — a warped disc, default 0. One sinusoid at
  exactly the platter rate (1/1.8 Hz at 33⅓, 0.75 at 45, 1.3 at 78), peak
  rate deviation 0.6% at 100%.

- **78 RPM** (item 27c) — appended to `VINYL_RPM`. Revolution quantum
  0.769 s. The surface comes with it: shellac ticks are 1.6× denser and the
  tick band drops and narrows to 1.5–4 kHz, matching a playback chain that
  rolled off hard above ~5 kHz. Gated on the index, so 33⅓ and 45 keep their
  numbers to the bit.

### Changed
- **Forward groove jumps are revolution-quantized** (item 27a). Research
  §4.1 specifies "exactly ±1 revolution" in *both* directions; the engine
  only ever did it backward, and every forward skip teleported to live. A
  groove that skips backward by a revolution and then returns to the present
  is half a physical model — the stylus jumping a ridge forward lands one
  groove on, not at the present. Forward jumps now move exactly
  `+revSamples` whenever `lag >= revSamples + minLag`, with the historical
  return-to-live as the fallback when the buffer genuinely has no revolution
  of past to spend. Rate is untouched, so BRIEF:16's "pitch never changes"
  holds exactly as it does for the backward jump this mirrors. Draw count
  unchanged.

### Notes on how three of these were built

**The warp is a read offset, not a rate multiplier.** The brief says
"sinusoidal rate multiplier". That is not available in this engine, for the
two reasons `WowFlutter.h` documented in v1.4.0 and which apply here
verbatim: `ReadHead`'s lag-overflow clamp is suppressed while a loop
transport owns the read rate, and the proof that the suppression is safe is
"such a loop holds rate at **exactly** 1.0" — a rate multiplier falsifies
that premise, and a *locked groove* is precisely the case item 27b is about.
Separately, at the engine's lag-0 steady state any rate above 1.0 drives the
head into `ReadHead`'s write-slot pin, which is a zero-order hold: a
stutter, not a warp. Modulating a non-negative read **offset** is the same
physics from the other end — pitch deviation is the derivative of delay — so
`VinylWarp` specifies its excursion by the rate deviation it should produce
and inverts the relation. Every lag budget, loop-wrap test and clamp proof
in the engine is untouched, and the offset simply sums with the wow/flutter
bed's at the single `renderSample` call site.

**The warp LFO's period is an integer, and it is the same integer as the
jump.** The brief's claim is that a warped locked groove wobbles identically
every pass. That is true only if the LFO period equals the groove-jump
distance *exactly*: a locked groove jumps back `revSamples` each pass, so
`pos(t + revSamples) == pos(t)`, and the read repeats only if
`offset(t + revSamples) == offset(t)` too. The phase increment is therefore
`2π / revolutionSamples()`, not `2π·f/fs` — deriving it from the un-rounded
seconds would leave a 0.077-sample residue per pass at 78 RPM / 48 kHz.
Small, but the identity can be exact, so it is. The new `VinylGeometry.h`
holds the one definition of the RPM→samples mapping that the transport, the
warp and the noise bed all read, because three copies of
`rpmIndex == 1 ? 45.0 : 100.0/3.0` is the shape that drifts silently the
next time a speed is added.

**The scratch class needed its own RNG stream.** A 2–8 ms noise burst draws
once per *sample*, and the `artifactSynth` stream it would otherwise share
is consumed at *tick* instants — mixing the two on one stream is the
block-size interleave hazard verbatim. `RngBank` gains an **appended**
`scratch` id (appended, never inserted: stream *k* is seeded from a function
of *k* alone, so every pre-existing stream's seed, and every render made
with an old `SEED`, is bit-identical). `triggerPop` itself keeps the
fixed-draw discipline, now **five** draws on every call — before any branch,
regardless of class, and regardless of level 0 — so a probe that silences
the pops still gets an identical event sequence.

### Migration
`VINYL_RPM` gained a third entry, and appending to an `AudioParameterChoice`
**repoints saved presets**. Presets store the normalised fraction, and index
*i* over *N* choices encodes as *i*/(*N*−1): "45" went out as 1.0 against an
end of 1, and 1.0 against an end of 2 decodes as "78". A pre-1.7.0 preset
that used 45 RPM would silently load as 78.

Handled with the preset-manager **v1.0.6** `setMigrationCallback` hook (the
same one O-Tapestop v1.1.0 added for its `MODE` append): pre-1.7.0 presets
have their `VINYL_RPM` fraction re-decoded against the old two-choice end
and re-encoded against the new three-choice one, before the module's
reset-to-defaults and apply passes. Factory presets regenerate at the
version bump and are stamped with the current version, so they never reach
the gate.

APVTS **session** state needs no equivalent and gets none: it persists the
choice *index*, and 0/1 still mean 33⅓ and 45. Presets are normalised,
sessions are denormalised — only the presets move.

`VINYL_WARP` is appended at the **end** of the parameter layout, not into
the vinyl block where it belongs visually, for the third release running:
layout order is the automation-slot order a host presents, and inserting
would repoint every saved automation lane behind it. The WebView puts it in
the Vinyl panel regardless, because the UI is keyed by parameter ID.

### Render-affecting
Yes, for the vinyl family, at defaults. Two independent changes do it and
neither is a regression: the pop taxonomy widened `triggerPop` from two
`artifactSynth` draws to five, so every pop after the first differs even at
an unchanged level; and forward jumps became revolution-quantized. The
canonical digest probe `V1` runs `VINYL_PROB 60`, so it legitimately moved
(`0x972a5d3807538393` → `0xf8c2080db4e69ec1`).

`V1` was **re-anchored, not quietly re-recorded** — the same treatment
v1.6.0 gave it. It now pins `VINYL_ENABLE 0` in addition to `CD_SEVERITY 0`,
and the new anchor `0x44a5de77d572facd` was produced by compiling the probe
*with those two lines* against the **v1.6.0 tree** at git `16e63620`, in a
detached worktree, and reading the number it printed. That run scored 86/87
— the single failure being `V1` itself measuring the freshly narrowed render
against the anchor the narrowing had just superseded, which is exactly the
run that produces the new number. The current tree then matches it.

What that preserves is the claim worth having, now across four releases:
**tape bends, the post-stop recovery jump, the CD conceal rung and the
lag-overflow clamp are bit-unchanged** — and so is the read path with
`VinylWarp` wired into it and silent, which is the v1.7.0 addition that
claims to be exactly transparent at its default. The vinyl transport's own
invariants are asserted directly by `M`/`M4`/`M5`/`M8` on the revolution
grid rather than through a digest, which is the better place for them.
`N7` still matches its v1.4.0 anchor, so packet and codec are untouched.

### Testing
Harness **92/92**; 5 probes added. auval PASS; pluginval strictness-10
SUCCESS on three consecutive runs.

Every new probe was verified to FAIL against the code it gates, by reverting
that single behaviour and re-running:

| Probe | Gates | Reverted → |
|---|---|---|
| `M5` | item 27a's threshold, both sides | branch removed → "lag 3 rev moved 192000, expect 64000"; gate removed → "lag 65400 moved 64000, expect return-to-live" |
| `M6` | the warp's locked-groove identity | LFO period pinned to 33⅓ regardless of RPM → pass-to-pass diff **0.861** against a 1e-6 bound |
| `M7` | the pop class rings | pop routed back to the one-pole → **0** zero crossings in 5 ms, against 13 |
| `M8` | the 78 RPM quantum | index 2 removed from the table → quantum reads 86400 |
| `M9` | the preset migration gate | version gate removed → a v1.7.0 preset's "78" loads as "45" |

`M8`'s revert is the interesting one, because it is the case a naive probe
would miss: the probe takes its expected quantum from the *same function*
the engine uses, so deleting index 2 changes both together and every event
still lands "on quantum". The `distinct` guard — the quantum must not equal
45's 64000 or 33⅓'s 86400 — is what catches it, and it is the reason the
probe is written that way rather than against a literal 36923.

`M9` runs end-to-end through the module's public `loadPresetFromFile()`, so
the version gate, the reset pass and the apply pass are all in the path, and
it carries its own negative control: a preset stamped 1.7.0 with the same
1.0 must load as "78", because at that version 1.0 genuinely means 78.

`M` changed shape rather than gaining a check. Its forward assertion used to
be "every forward event lands at live", which *was* the whole of the old
behaviour; it now accepts either shape and additionally requires that the
revolution-quantized forward actually fires, so the pre-v1.7.0 engine fails
it at "0 fwd+rev". Its scan window also moved 400 samples past the warmup:
the warmup boundary lands exactly on a clock tick, so a jump fired on the
window's first sample and `scanSawEvents` read that event's `vPre` from
inside its own smear — measuring 63556 where every other event reads
64000 ± 3.2 (the ±3.2 is the +2% re-approach trim over the 160-sample
read-ahead, and is why the tolerance is ±8). That one event was a
measurement artifact, not a transport defect, and it is now skipped rather
than mis-measured.

## [1.6.0] — 2026-08-17

CD skip authenticity — improvement brief items 14 and 18. The CD ladder's
top two rungs were built to the right *shapes* and then flattened by
constants that ignored the one knob meant to drive them. A mute was 2–20 ms
whether the disc was lightly scuffed or dying; a loop window was whatever
`CD_SEGMENT` said, to the sample, which no anti-shock buffer has ever done;
every loop wrap was smoothed by the same 3 ms crossfade a *recovery* jump
uses; and a loop ended by snapping to live in one sample. Four changes, all
in the CD family, all keyed to `CD_SEVERITY`, no new parameters.

**This release changes the render at the default `CD_SEVERITY` of 0.5** —
the first O-Bitrot release that does. Item 14b's mute ceiling is live at
every severity above 0, and 0.5 is where the knob starts. See
"Render-affecting" below for exactly what is and is not preserved.

### Changed
- **Loop wraps splice HARD** (item 14a). A wrap is the artifact; a recovery
  is the player getting its act together. They were both taking the single
  global 3 ms crossfade, which at the Skipping Disc preset's 45 ms segment
  spent **6.7% of every repeat inside the splice** — the buzz sanded off the
  thing that makes it a CD skip. `ReadHead::clampAndScheduleJump` now takes
  an optional per-jump fade length; CD loop entries and wraps pass 0.5 ms
  (24 samples at 48 kHz, 1.1% of that segment), and vinyl revolutions,
  recovery jumps and the overflow clamp keep the 3 ms default.

  The trap here is the **mid-fade fold**, added in v1.2.1 to stop a jump
  arriving mid-crossfade from dropping the outgoing head as a step. Its
  arithmetic divides `fadeCount` by the fade length — so with a per-jump
  length, the *running* fade's length is the only valid denominator. A
  splice request arriving 20 samples into a 144-sample fade must therefore
  SPEND the running length, not adopt the short one: rescaling `t` from
  20/144 to 20/24 collapses the outgoing head's weight from 0.861 to 0 in a
  single sample. Measured, by building it the wrong way on purpose: a
  **0.357 output step against a 0.042 bound**, an 8.5x click. The fade
  length in flight is now a separate member from the two prepared lengths,
  and probe `L5` pins both halves.
- **Mutes lengthen with severity** (item 14b). The rung-1 mute was pinned at
  2–20 ms regardless of `CD_SEVERITY`; a real E32 mute grows as the disc
  worsens. The *span* scales (the 2 ms floor does not), from 18 ms at
  severity 0 to 148 ms by severity 0.6 — where the loop rung takes over —
  for a 150 ms ceiling. Measured 61.2 ms longest at severity 0.2 against
  that severity's 63.3 ms ceiling, and 141.9 ms at 0.6 against 150.0. The
  severity-0 expression is the v1.5.0 one **bit-for-bit**, because
  `0.130 * 0.0` is exactly `0.0`.
- **Loop windows lock to CD sectors above severity 0.5** (item 14c).
  `CD_SEGMENT` is a free 10–400 ms, but an anti-shock buffer re-reads whole
  sectors, and the 1/75 s quantum is why a skipping CD buzzes at a
  75 Hz-family rate rather than at whatever the knob says. Above
  `kSectorSeverity` the window snaps to the nearest multiple of `fs/75`:
  100 ms asks for 4800 samples and gets 5120. Implicit rather than a toggle
  — "how far gone is the disc" is what `CD_SEVERITY` already means. At or
  below 0.5 the free value is used verbatim, and both sides are asserted
  (probe `L3`), because the gate is the claim.

### Added
- **Servo-seek terminal stage above severity 0.85** (item 18). `CD_SEVERITY`
  had three audible regions and its top third was just "region 3, but more
  often". The real end-stage of a dying player is not a loop — the buffer
  runs dry, the sled loses tracking, and there are **hundreds of
  milliseconds of dead silence** before playback resumes somewhere ahead
  with a re-lock chirp. A loop released above `kSeekSeverity` now mutes
  fully for 100–400 ms and only then takes the recovery jump, with a chirp.
  Measured 338.2 ms on the canonical render, tail tracking live to 0.000000
  after it.

  The duration is drawn **at loop entry, not at release**. Both paths that
  end a loop — `release()` and the lag-budget self-release inside
  `processSample` — run without an `RngBank`, and the harness's block-size
  invariance rests on RNG being consumed only at ticks and at deterministic
  jump instants. Drawing at entry also means the draw is **skipped entirely
  at or below 0.85**, so the cd stream's pattern is untouched there.

  It is deliberately NOT entered when a loop gives way to a *different rung*
  on a fresh CD win: the family is still winning ticks, so that is the disc
  changing failure mode, not the terminal release — and 100–400 ms of
  silence would swallow the very rung being installed.

### Render-affecting
Yes, at `CD_SEVERITY` > 0. The canonical cross-version digest probe `V1` was
**re-anchored** rather than quietly re-recorded: it now pins `CD_SEVERITY 0`,
the one severity at which this release is exactly transparent (the rung roll
can only reach conceal, both new thresholds are below their gates, and every
jump still takes the default fade). The new digest `0x972a5d3807538393` was
produced the same way the old one was — by compiling the probe against the
v1.3.0 tree at git `a22ff7c3`, where it ran 57/57 — and the current tree
matches it. What that preserves is the claim worth having: **the per-jump
fade refactor changed nothing for tape bends, vinyl revolutions, the
post-stop recovery jump, the CD conceal rung or the overflow clamp, across
three releases.** `N7` still matches its v1.4.0 anchor, so the packet and
codec chain is likewise untouched.

### Testing
Harness **87/87**, stable over three runs; 6 checks added (`K2`, `L3` x2,
`L4` x2, `L5` x2 — 7 checks across 5 probes). Every one was verified to FAIL
against the code it gates by reverting that single behaviour and re-running:
disabling the sector lock takes `L3` to "0 wraps at 5120, 374 at 4800";
disabling the seek takes `L4` to "mute 0.0 ms"; pinning the mute span takes
`K2` to 18.1 ms at severity 0.6; ignoring the fade argument takes `L5` to
"splice 143, expect 23"; and removing the fold guard produces the 0.357 step
described above.

`L5` measures the read head **directly**, not through the plugin, and that
is not a shortcut: every loop wrap fires a chirp over exactly the window the
splice occupies, three orders of magnitude louder than the marker step being
faded (the existing chirp probe reads a ratio of 2.3e12). Driving `ReadHead`
and its ring from the harness removes the chirp, the clock and the rung roll
and leaves the one number in question.

`L`'s recovery check accepts a build with or without the seek — verified,
not assumed — because its claim is "the loop ends and the head returns to
live". `L2`'s post-recovery window is now clipped to the next clock tick and
its **width is asserted**: the seek pushed the recovery to ~1300 samples
before that tick, and the old fixed 10000-sample window was measuring the
next loop's entry jump (0.018311 — exactly one segment over the saw period)
and calling a working engine broken.

## [1.5.0] — 2026-08-17

Media noise beds — improvement brief items 4 and 19. The engine
synthesised only *event-triggered* artifacts: `ArtifactSynth`'s pop, tick
and chirp all fire at jump instants, and between clock ticks nothing ran
at all. A "dying media" patch was therefore a clean signal punctuated by
breakage, and the illusion collapsed the moment the events stopped. It is
the floor, not the events, that carries the sense of a machine.

Five new parameters, **every one default 0 and exactly transparent
there**. A v1.4.0 session or preset loads and renders bit-identically —
pinned by two digest probes against two different earlier trees, not
asserted. **There are no render-affecting changes in this release.**

### Added
- **`TAPE_HISS` — the tape noise floor** (0–100%, default 0). Decorrelated
  stereo white behind a gentle −9 dB shelf above 3.5 kHz and a 60 Hz
  highpass; **−48 dBFS RMS at full knob** (measured −47.99), roughly a
  Type I cassette with no noise reduction.

  Stereo, unlike every other artifact in this engine, and deliberately so:
  two tape tracks carry two *independent* noise sources, and a mono bed
  added to both channels hits the level target exactly while sounding like
  a fault in the plugin rather than a floor on the tape. Measured L/R
  correlation −0.000, and the probe asserts it — level alone cannot tell
  the two implementations apart.

  It also **rides the transport speed**. A tape head is a `dΦ/dt`
  transducer, so hiss is recorded material like everything else: when
  v1.4.0's `TapeStopGain` takes the programme down to silence, hiss that
  kept running at full level would announce that the noise is synthetic.
  `TapeStopGain` now publishes its current gain and the bed multiplies by
  it. A *bend* must not do this — the bend table's 0.5× interval sits below
  the gain law's threshold — and the discriminating probe for that lives
  next to the one that proves stops do.
- **`VINYL_WEAR` — the record surface** (0–100%, default 0). A Poisson rain
  of micro-ticks over pinked bearing rumble.

  Tick amplitude is a **cubic power law** — `peak · u³`, many tiny and rare
  large, which is the shape real surface noise has. Cubing a uniform is
  *bounded*; an inverse-CDF Pareto draw would not be, and nothing on the
  audio thread should be able to draw an arbitrarily large impulse. The
  harness asserts the law through the tick band's **crest factor** (10.9
  measured, bound ≥ 8): Gaussian noise crests near 4, so a bed emitting
  uniform-amplitude clicks would pass a rate check and fail this one.

  Rumble is **−42 dBFS RMS at full knob** (measured −42.00), three poles
  below 55 Hz, amplitude-modulated once per platter revolution at the
  `VINYL_RPM` rate — bearing rumble is eccentric by construction, and that
  beat is what separates "low noise" from "a turntable".

  Mono, and for a more literal reason than `ArtifactSynth`'s "the failure
  is the player, not the channels": one platter bearing makes one rumble
  and one stylus rides one groove.
- **`CODEC_NOISE` + `CODEC_MAINS` — the phone line** (0–100%, default 0;
  50 Hz / 60 Hz, default 50 Hz). Mains hum — fundamental plus 2nd and 3rd
  harmonics at −40 dBFS RMS full-knob (measured −37.87 for the fundamental,
  against an analytic −37.86) — with Poisson crackle bursts, 3–25 ms,
  band-limited to 300–3000 Hz.

  The hum takes **no RNG draws at all**: three partials off one phase
  accumulator, a pure function of the sample count. Mains hum is the one
  artifact in this plugin that is genuinely not random, and a hum whose
  phase depended on the seed would drift against the programme between
  renders of the same session.

  It is injected **after `CodecStage`, not on the artifact bus with the
  other two**. `CodecStage` is a 300–3400 Hz phone chain, so 50 Hz in front
  of it is annihilated by the passband and the hum would have been
  inaudible at every setting. That is also where the physics puts it — hum
  is induced on the line, not recorded at the source — so it is scaled by
  `CODEC_MIX` as well: Blend is how much phone you are hearing, and the hum
  *is* the phone.
- **`PACKET_COMFORT` — comfort noise under extended concealment**
  (0–100%, default 0). `PacketLossStage` hard-floors Decay to exact silence
  by the end of the third repetition (~60 ms) because that is what real PLC
  does. What real PLC does *next*, which this engine did not, is fill the
  hole: G.711 Appendix II CNG and GSM SID frames both substitute low-level
  spectrally-shaped noise matched to the background. That hiss floor is
  precisely the cue that says the call is still up but dying — without it a
  long burst is indistinguishable from the far end hanging up.

  Two one-pole trackers run on **good packets only** — a level estimate and
  a tilt estimate, the energy split either side of 1 kHz — so a dark source
  gets a dark floor and a bright one gets a bright floor. Each half of the
  noise split is divided by its exact analytic gain, so the tilt weights
  mean what they say. A silent source estimates zero and emits exactly
  nothing: comfort noise under silence would be the plugin talking.

  The bed is **additive under all four concealment modes**, not a
  replacement. Under Decay and Substitute — already at or near silence when
  it arrives — that reads as the crossfade the brief specifies; under
  Repeat, which repeats a packet verbatim forever, replacing the output
  would have dissolved the machine-gun edge that *is* that mode's identity,
  so the bed sits 33 dB beneath it instead (measured). The knob is squared
  before scaling so its useful range spans the travel: 100% = −30 dB
  relative to the tracked programme, 50% = −42 dB (the G.711 figure), 25% =
  −54 dB.

  **Honest limit:** the level tracker follows the *programme*, not the
  background, because this stage never sees a speech/silence decision.
  Sitting the bed far below it is what makes the approximation work.

### Changed
- Four RNG streams — `tapeBed`, `vinylBed`, `codecBed`, `comfort` —
  **appended** to `RngBank`, never inserted. Stream *k* is seeded from a
  function of *k* alone, so every pre-existing stream, and therefore every
  render ever made with an old `SEED`, is bit-identical.
- Vinyl, Packet and Codec panels moved to the dense two-row layout Tape and
  Crush already used. Only CD is still a flat single row.

### Notes
- **Levels are sample-rate invariant, and that took work.** Filtering white
  noise to a fixed bandwidth in Hz gives output power proportional to that
  bandwidth over `fs`, so a bed calibrated with a bare constant is 3 dB
  quieter every time the rate doubles — inaudible in the one render anybody
  tests, and wrong for every user at 96 kHz. Each bed normalises its first
  (and only white-fed) filter stage by that stage's exact analytic noise
  gain `sqrt(a/(2−a))`; everything downstream operates on an
  already-shaped signal whose spectrum is fixed in Hz. Normalising the
  *later* stages the same way would have reintroduced the dependence it
  exists to remove. **Measured 48 kHz vs 96 kHz: 0.13 dB.**
- **Where each bed is injected is load-bearing.** Tape and vinyl join the
  mono artifact bus upstream of the packet stage, so a lost packet conceals
  the media noise along with the programme — on real media they are the
  same signal. The codec bed goes after the codec, for the passband reason
  above.
- **Determinism.** Each bed draws a fixed count per sample from its own
  stream on its own sample schedule, which is block-size invariant; the
  interleave hazard is two subsystems *sharing* a stream at different
  block-relative instants, not per-sample draws as such. That is also why
  the conditional extra draws taken when a tick or a crackle burst fires
  are safe — they depend on the private stream's own position, never on a
  block boundary.

### Testing
- **80/80 render-harness probes pass** (66 → 80; 14 new).
- **`V1` still matches the v1.3.0 digest** `0x3ee4e028900e47ca`, and a new
  **`N7` matches the v1.4.0 digest** `0x1cf2f80d1f71674c` for a canonical
  render with packet *and* codec active. V1 runs with both post-stages off,
  so on its own it says nothing about the two places v1.5.0 actually
  touched downstream code; N7 covers exactly those. The v1.4.0 digest was
  produced by compiling that probe against the v1.4.0 tree (git `2160dd66`)
  in a throwaway worktree — re-rendering the new engine twice would prove
  nothing.
- `N1b` asserts the vinyl bed is **bit-exact zero**, not merely quiet, at
  `VINYL_WEAR` 0 — that exactness is what keeps the FUNC-02 null intact at
  the shipped defaults.
- `N6` re-runs same-seed and ragged-block-size bit-identity with all four
  new streams drawing at once (`{1, 7, 64, 333, 4096}` vs `{512}`).
- auval PASS; pluginval strictness-10 **3/3 SUCCESS**.

## [1.4.0] — 2026-08-17

Tape authenticity — improvement brief items 2, 3 and 11. The tape family
was the engine's most conspicuous gap against the brief's core promise:
between events it was bit-clean, its stop froze a held sample at full
level, and the most common audible fault of real failing tape had no
event kind at all.

Two new parameters, **both default 0 and both exactly transparent
there**. A v1.3.0 session or preset loads and renders bit-identically —
pinned by a new cross-version digest probe, not asserted. The one
deliberate render change is the tape stop; see "Render-affecting".

### Added
- **`TAPE_DROP` — the dropout event** (0–100%, default 0). Oxide shed
  and creased tape lift the coating off the head for a few milliseconds:
  the level dips *partway* — 10–70%, never to silence — and the top end
  goes with it, over 5–150 ms. It is the third kind of tape event, taking
  a share of the non-stop tape wins.

  The shape is `CDSkip`'s interpolation-conceal rung reused verbatim: a
  triangular log-frequency cutoff sweep, now with a triangular gain dip
  multiplied alongside it. Both endpoints are the exact identity because
  `tri(0) == tri(1) == 0`, so the event is click-free by construction
  with no ramp bookkeeping.

  Unlike a stop or a bend it installs **no rate event**, so a bend
  already in flight keeps ramping underneath it — the first instance in
  this engine of the OVERLAY class that brief item 6 generalises. It
  changes no position and no rate, so it needs no ring headroom and
  touches no part of the `ReadHead` contract.
- **`TAPE_WOW` — the continuous wow/flutter bed** (0–100%, default 0).
  Real decks wow (0.5–6 Hz, capstan and pinch-roller eccentricity) and
  flutter (6–100 Hz) *all the time* — 0.1–1% WRMS on consumer gear,
  several percent when dying. This engine had none: `TapeTransport`
  returns exactly 1.0 while idle and nothing else modulated the
  transport, so a "worn cassette" was a perfect deck that occasionally
  broke. The "Worn Cassette" preset's own comment has claimed wow since
  1.0.0, and the CHANGELOG has marketed the family as "Tape (wow, drag,
  full stops)" for as long.

  Three quasi-periodic partials — two fixed wow frequencies (0.73 Hz,
  2.31 Hz) and one flutter partial whose frequency random-walks across
  7–55 Hz — with slowly drifting amplitudes from a **new dedicated
  `wow` RNG stream**. Measured peak deviation at full knob: **1.14%**,
  against a 2.0% design budget that the partial table asserts against
  itself at `prepare()`.

  The stream is appended to `RngBank`, never inserted: stream *k* is
  seeded from a function of *k* alone, so every pre-existing stream —
  and therefore every render made with an old `SEED` — is untouched.

### Changed
- **A tape stop now dies with speed instead of freezing to DC.** A tape
  head is a `dΦ/dt` transducer: its output is proportional to tape
  *speed*, the high end dies first because the reproduce cutoff scales
  with speed at fixed recorded wavelength, and decks mute at transport
  stop. Before this, the rate ramped to exactly 0 and the read head then
  re-read one held sample forever at full amplitude, feeding a DC step
  straight into Codec and Crush. The stop sounded like a freeze.

  Output is now scaled by `g = (rate / 0.9)^0.8` below a rate of 0.9 —
  exactly 0 at rate 0 — with a speed-tracking one-pole whose cutoff
  falls as `fMax · rate / 0.9`. Measured on a forced-stop render, quietest
  32 ms window: **0.0216 → 0.000000**. (The held DC level is whatever the
  source happened to be worth at freeze time, not its peak — 0.0216 is the
  luckiest of the three freezes in that render, and the probe's 1e-3 bound
  sits an order of magnitude below even that.)

  The law is armed by a stop being *installed*, not by a rate test, and
  that distinction is the whole design. Rate alone cannot tell a stop
  from a bend: the interval table's 0.5× down-bend sits below the same
  0.9 threshold, and a rate-keyed law would have quietly taken 6 dB off
  every down-bend — the tape family's melodic voice. Probe `S2` pins
  this, and pins that the render it checks actually *visited* a
  sub-threshold rate (measured lowest bend ratio 0.500).

### Fixed
- **A one-pole entered at `fMax` is not transparent — it clicks.** Both
  new filters set their cutoff to `fMax` at the endpoints on the
  assumption that a lowpass there is effectively bypassed. It is not: at
  `fMax = 0.45·fs` the TPT gain coefficient is
  `tan(π·0.45)/(1 + tan(π·0.45)) ≈ 0.79`, and at the 0.9·fMax the stop
  law actually enters at, ≈ 0.70 — so a filter engaged with zero state
  drops 30% of the first sample it is handed. Measured as **0.150 and
  0.126 output steps on a 0.5-amplitude sine**, i.e. a click at the exact
  instant each event is supposed to be inaudible. Both filters are now
  *blended* in by the same shape that drives them (`tri` for the
  dropout, `1 - rate/0.9` for the stop), which makes entry, exit and
  bypass one identity. Both probes measured 0.144 → **0.0144** after the
  fix, which is the sine's own maximum derivative — the floor.

### Render-affecting
Anything that fires a **tape stop** renders differently from v1.3.0:
that is item 11, and it is the point. `TAPE_STOP_PROB` defaults to 10%,
so any patch with the tape family enabled is affected.

Everything else is bit-identical, and probe `V1` says so rather than
claiming it: a canonical 4 s render of forced tape *bends* over CD and
vinyl at their defaults digests to `0x3ee4e028900e47ca` under both
v1.3.0 (git `a22ff7c3`, built with this probe injected) and v1.4.0. The
two new parameters are transparent at 0 by two separate mechanisms —
the wow bed returns an offset of exactly `0.0` (and `pos - 0.0` is
bit-identical to `pos`, so `CaptureRing`'s exact-integer fast path still
fires), and the dropout roll is short-circuited before it can consume a
draw from the tape stream, so the bend sequence is unchanged.

### Notes
- **The wow bed modulates a read OFFSET, not the read rate.** The brief
  specifies a rate multiplier; that implementation breaks two
  load-bearing invariants. `ReadHead`'s lag-overflow clamp is suppressed
  while a CD loop or locked groove owns the rate, and the proof that the
  suppression is safe is "such a loop holds rate at *exactly* 1.0" — a
  multiplier falsifies the premise. And the engine's steady state is lag
  0, so any rate above 1.0 drives `pos` into the write-slot pin: at 1.5%
  and 0.7 Hz that is ~8 ms of zero-order hold per wow cycle, which is a
  stutter, not wow. Offsetting the read is the same physics from the
  other end — pitch deviation is the derivative of delay — while `pos`,
  the lag budget and the whole jump contract stay exactly as they were.
  Peak offset is ~5.5 ms, well inside the ring's 100 ms safety margin.
- **`TAPE_DROP` and `TAPE_WOW` are appended to the end of the parameter
  layout**, not inserted into the tape block. Layout order is the
  automation-slot order a host presents; inserting would have shifted
  all 23 later parameters by two slots and silently repointed saved
  automation lanes. APVTS state, the preset bank and the WebView
  bindings are keyed by ID and are order-independent.
- **Found, not fixed:** `CDSkip`'s interpolation-conceal rung has the
  same latent filter-entry click described under "Fixed" — it resets
  `concealFilter` and starts the sweep at `fMax` (`CDSkip.h:137-138`).
  No probe currently measures conceal click, which is why it has gone
  unnoticed since Phase 2.2. Left alone deliberately: fixing it changes
  CD renders, which is brief item 14's territory, and it would also
  invalidate the `V1` digest this release is gated on.

### UI
The Tape panel goes to five controls in a 3 + 2 two-row layout, matching
the existing Crush panel's dense form (`k50` knobs): Prob · Stop · Drop
on the first row, Wow · Ramp on the second.

### Presets
`TAPE_DROP` and `TAPE_WOW` are authored in all eight factory presets.
Three dial them in: **Worn Cassette** (35 / 55 — the preset whose
comment has promised wow since 1.0.0), **Gentle Rot** (20 / 25) and
**Total Media Failure** (45 / 80). The rest hold 0, which for the five
tape-disabled presets is what they already effectively were.

### Testing
`66/66` render-harness probes green, stable over three consecutive runs
(was 57/57); `pluginval --strictness-level 10` SUCCESS three times;
`auval` VALIDATION SUCCEEDED. The FUNC-02 nulls (`B`, `M1`, `M3`) and
every block-size-invariance probe (`F`, `G`, `N`, `Q`, `S2`, `Z2`)
stayed bit-identical throughout. Factory presets regenerated at 1.4.0
with 33 parameters each; `Worn Cassette` verified on disk carrying
`TAPE_DROP` 0.35 and `TAPE_WOW` 0.55 normalised.

Nine probes added:

| Probe | Gates | Result |
|---|---|---|
| `V1 v1.3.0-bit-identity` | defaults are transparent, cross-version | `0x3ee4e028900e47ca` under both versions |
| `W1 wow-live-and-bounded` | bed modulates, inside its budget | 1.138% peak, bound [0.4%, 3.0%] |
| `W2 wow-zero-bit-exact` | the 0 case keeps the integer read path | 9 s bit-exact, tape family ENABLED |
| `D1 dropout-dips-and-returns` | dips partway, returns to unity, no click | ratio [0.168, 1.000], maxDelta 0.0144 |
| `D2 dropout-zero-no-dip` | negative control for `D1`'s floor | min windowed peak 0.5000 |
| `S1 stop-dies-with-speed` | the stop reaches silence, no click | quietest 32 ms peak 0.000000 |
| `S2 bends-keep-loudness` | the gain law does not leak onto bends | peak 0.5000, lowest bend ratio 0.500 |
| `S3 v1.4-determinism` / `-ragged` | block-size invariance with all three on | bit-identical, 512 vs 1,7,64,333,4096 |

`S3` is the one that matters for the wow bed specifically: it is the
only RNG consumer in this engine that is not tick-aligned, drawing on
its own sample counter, so anything tied to a block boundary rather than
a sample count surfaces there as a mismatch under ragged chopping.

Every new positive-claim probe was **run against the reverted code**, in
one build with all three new paths disabled — a probe that passes both
ways is decoration:

| Probe | Reverted result |
|---|---|
| `W1 wow-live-and-bounded` | 0.000% deviation — "BED IS FLAT" |
| `D1 dropout-dips-and-returns` | ratio [1.000, 1.000] — "NO DROPOUT FIRED" |
| `S1 stop-dies-with-speed` | quietest 32 ms peak 0.0216 — "STOP STILL FREEZES TO DC" |

`D1` and `S1` additionally failed on their click bounds before the
filter-blend fix (0.150 and 0.126 against 0.03). `V1`, `W2`, `D2` and
`S2` correctly stayed green through the same revert, which is what they
are for: `V1`'s canonical render touches none of the three paths, and
`W2`/`D2`/`S2` are the negative controls that stop `W1`'s lower bound
and `D1`'s floor bound passing against an engine that attenuates or
modulates for some unrelated reason.

One existing probe was **re-annotated, not re-recorded**: `D DSP-01
stop-no-click` asserts a long run of bit-identical samples during the
hold, and under the new gain law that run is a run of zeros — so the
probe no longer discriminates a working stop from a silenced one on its
own. Its assertions are unchanged and still pass; `S1` and `S2` are what
now pin the stop's amplitude behaviour.

## [1.3.0] — 2026-08-17

Engine quality foundations — improvement brief items 5 and 12. No new
parameters and no state-format change; presets and automation load
unchanged. Long-loop and tape-bend **renders do change** — see
"Render-affecting" below.

### Changed
- **Capture ring 2.5 s → 10 s.** The ring span was the ceiling on every
  sustained loop, and the arithmetic was unforgiving: a locked groove
  re-jumps only while `lag + revolution <= maxLag - 50 ms`, which at
  2.5 s required a *negative* starting lag for a second pass. The
  headline "Locked Groove" preset therefore released after exactly one
  re-pass, every time. At 10 s the groove now runs **6 revolution-spaced
  re-passes** measured (>= 4 guaranteed by the static_assert at 33 1/3
  RPM), and a sustained CD loop runs **24 passes deep** where it
  previously managed 5. Cost is one `prepareToPlay` allocation: stereo
  float, 3.8 MB at 48 kHz, 15.4 MB at 192 kHz.
- **The ring's static_assert now constrains the constant it guards.** The
  old form (`>= one revolution + tape ramp + safety`) is satisfied by
  2.5 s and by 10 s alike, so it could never have caught the one-pass
  ceiling it was nominally protecting against. It is now stated as the
  multi-pass budget a locked groove actually spends, against a
  `kMinLockedGroovePasses` floor — at 2.5 s it fails to compile.
- **Read-head interpolation is 4-point Catmull-Rom** (was a 2-point
  lerp). A mid-sample lerp read retains only `|cos(pi f / fs)|` of the
  amplitude — 0.309 at 0.4x the sample rate, and about −16 dB at 0.9x
  Nyquist — and the tape family runs its entire melodic voice through
  this path at bend rates of 0.5–2.0x. Measured mid-sample retention at
  0.4·fs: **0.294 → 0.427**. The `frac <= 0` exact-integer fast path is
  untouched and still bypasses the polynomial entirely, which is what
  keeps the all-off passthrough bit-transparent (FUNC-02). Cost is two
  extra ring reads and a few FMAs, on the fractional path only; the
  worst-case render ratio was unmoved (0.0045 → 0.0043).

### Fixed
- **Sustained CD loops no longer slip.** A loop ages the read head by one
  segment per pass and had no lag budget of its own, so it kept wrapping
  until `ReadHead`'s lag-overflow clamp teleported the head forward —
  while `CDSkip` still read `state == Loop`, so the very next wrap
  re-jumped from the teleported position. The result was an audible
  unplanned slip attributable to no family. `CDSkip` now gates its wrap
  on the same lag budget the vinyl locked groove uses and self-releases
  through its own forward recovery jump when the ring is spent. Measured
  on the 400 ms-segment probe: **13 recovery jumps landing 68404 samples
  behind live → exactly 1 landing at lag 1**, with the render tracking
  live material afterwards (post-recovery error 4e-1 → 1e-6).
- **The lag-overflow clamp no longer fires under a loop.** It is
  suppressed while a CD loop or locked groove owns the read rate, which
  those transports hold at exactly 1.0 while gating their own jumps on
  the same budget — so the clamp could only ever fire spuriously there.
  The suppression additionally requires the tape transport to be idle: a
  CD or vinyl win starts a tape *release* ramp that keeps running
  underneath the loop for up to `TAPE_RAMP` ms, and during that ramp the
  rate is not 1.0, so lag can genuinely drift and the clamp must stay
  armed.
- **Overflow recovery lands at a fixed 1.2 s, not half the ring.** The
  landing distance was `0.5 * maxLag`, which was 1.2 s behind at the
  2.5 s ring but would have become a 4.95 s teleport into stale material
  once the ring grew. Pinning it as a duration keeps the last-resort
  safety net's behaviour exactly what it has always been.

### Render-affecting
Anything driving a **CD buffer loop**, a **vinyl locked groove**, or a
**tape bend** renders differently from v1.2.1 — deeper loops, and a
cleaner interpolator on every fractional read. Bit-exact passthrough with
all families off is unchanged and still verified by the null probes.

### Testing
`57/57` render-harness probes green, stable over three consecutive runs;
`pluginval --strictness-level 10` SUCCESS three times; `auval` PASS.
FUNC-02 nulls (`B`, `M1`, `M3`) and every block-size-invariance probe
(`F`, `G`, `N`, `Q`, `S2`, `Z2`) stayed bit-identical throughout.

Three probes added, each verified to FAIL against the code it gates
(a probe that passes both ways is decoration):

| Probe | Gates | Reverted result |
|---|---|---|
| `C2 item-12 catmull-rom` | interpolator response + fast-path bit-exactness | lerp scores 0.294, bound 0.40 |
| `L2 item-5 cd-loop-budget` | loop depth, single intentional recovery, lands live | 13 recoveries, lag 68404, err 0.398 |
| `M4 item-5 locked-groove-multipass` | revolution-spaced re-passes | 0 re-passes at the 2.5 s ring |

Two existing probes were **re-recorded**, both for fixture reasons rather
than DSP ones:

- `M DSP-03 vinyl-jumps` / `M2 DSP-03 vinyl-pitch` — the saw position
  marker's period (262144) carried a stated invariant, "> 2x the ring's
  maximum lag", that nothing enforced. Growing the ring took max lag from
  120000 to 475200 samples and silently broke it: backward jumps measured
  from a deeply lagged head wrapped into `(-131072, 131072]` and were
  misread or discarded, which presented as "vinyl stopped jumping." The
  period is now 2^20 with the invariant as a `static_assert`, the
  detection thresholds derive from it instead of being literals tuned to
  the old value, and both probes get a 12 s ring-fill before measuring so
  the backward ladder reads real material rather than pre-history.
  `M` also asserts the deeper ladder it can now see (>= 8 backward jumps,
  measured 13); `M2`'s trackable-hop count went 277/599 → 641/641.

## [1.2.1] — 2026-08-17

Engine-robustness pass — improvement brief items 9 and 13. No new
parameters, no state-format change; presets and automation are untouched.

### Fixed
- **Sync mode is no longer inert while the host transport is stopped.**
  `CLOCK_MODE` defaults to Sync, and `MediaClock` emitted ticks only when
  `isPlaying && wasPlaying`, so out of the box the plugin was pure
  passthrough whenever the DAW was parked — auditioning live input read
  as "the plugin is broken." A stopped transport now falls back to the
  same free-run accumulator already used for a missing playhead /
  position / PPQ. The free phase is rewound on the stopped→playing edge
  so the next stop starts from phase 0. **Playing behaviour is byte-for-
  byte unchanged** (both sync-grid probes still land on their BPM grid).
  Also deleted the dead `lastPPQ` member — written every block, read
  nowhere.
- **Jump-during-crossfade no longer clicks.** `ReadHead::clampAndSchedule`
  `Jump` overwrote `oldPos`/`oldRate` and zeroed `fadeCount` even with a
  fade in flight, so the outgoing head's contribution vanished as an
  output step of `(1 - t) * |newHead - oldHead|` — the *full* jump
  discontinuity when the fade had barely started. Reachable in a single
  tick: `Arbitration`'s kVinyl branch runs `cd.release()` (recovery jump)
  then `vinyl.onWin()` (second jump) with no render between, so `t` was
  exactly 0 and the material actually playing was discarded outright.
  A mid-fade jump now FOLDS into the running crossfade: whichever head
  currently dominates the mix is carried over as the outgoing head at the
  gain already reached. The residual step is bounded by half the
  discontinuity in every case, and is zero in the same-tick collision.
  Measured on the antiphase probe: **0.85 → 0.0145** (the latter being
  just the test sine's own derivative).
- **Deep stops no longer strand the read head.** A tape stop left seconds
  of lag that only the +2% re-approach trim could recover — ~50x the
  stall duration — until the `ReadHead` lag-overflow clamp teleported the
  head mid-normal-playback, attributable to no family. A release ramp
  that lands back on NORMAL with more than 250 ms of lag now takes ONE
  intentional crossfaded jump to live through the same choke point (the
  CD-recovery pattern): "content lost while the transport was stalled."
  Below 250 ms the gentle trim is unchanged.

### Notes
- The recovery jump fires on roughly **39% of tape releases** at stock
  settings (8 seeds x 120 s, all three transports on: 31–52%, mean lag at
  release 500–850 ms). It is not tripping on ordinary bends — the lag
  distribution at release sits well above the threshold, and the observed
  **maximum reached 2.0–2.3 s**, i.e. the pre-fix hidden clamp really was
  being hit in default use. Sub-250 ms releases keep the trim.

### Testing
- Harness 54/54 green (51 pre-existing + 3 new). All FUNC-02 nulls,
  FUNC-04 determinism and QUAL-02 block-size/ragged bit-identity probes
  unchanged and green — the fixes add no RNG draws and are per-sample
  state only.
- New probes, each verified to FAIL against the pre-fix code so none is
  decoration: `I sync-stopped-free-runs` (inert → onset @24064),
  `N2 jump-fade-collision same-tick` (0.85262 → 0.01450) and `mid-fade`
  (0.58809 → 0.22122), `N3 post-stop recovery-jump` (tail correlation vs
  live input −0.0056 → 1.0000, on noise so period-aliasing cannot fake
  alignment).
- The old `I sync-stopped` probe asserted `onset == -1` and **inverts**
  under this change; it was rewritten as a positive free-run probe, and
  its former negative-control role was replaced by a new
  `I sync-stopped-all-off-silent` case (stopped transport, every family
  disabled) so the deviation detector is still proven non-spurious.

## [1.2.0] — 2026-08-16

### Changed
- **PACKET_LOSS now spans clean → true total failure.** Root cause of the
  old ceiling: the Gilbert–Elliott mapping capped stationary Bad occupancy
  at `piB = loss01·0.6` and hard-coded loss probabilities 0.5 (Bad) /
  0.01 (Good), so full knob delivered only ~30% actual loss — and the
  unscaled 1% Good-state floor dropped one packet every ~2 s even at
  PACKET_LOSS = 0 while merely enabled. New mapping: `piB = 0.95·loss01`,
  Bad loss `0.5 + 0.5·loss01`, Good loss `0.01·loss01` plus a top-quartile
  ramp to 0.90 at full knob (the Markov clamp caps Bad occupancy at ~0.89
  even at BURST 100, so near total failure the Good state must drop
  packets too). Measured: 0 lost packets at knob zero; 98.6% at full knob
  (was ~30%). Knob feel changes across the range; the determinism
  convention (exactly 2 packet-stream draws per packet) is untouched.
- **Decay concealment now mutes out like real PLC.** Was −3 dB per
  repetition with no floor (never silent, imprinting a 50 Hz packet buzz
  indefinitely). Now −6 dB per repetition as a per-sample gain ramp,
  hard-flooring to exact silence by the end of the 3rd repetition
  (~60 ms). Decay repeats are also pitch-aligned via the existing AMDF
  path (previously Substitute-only) when the last good packet is
  periodic, with the same auto-degrade to packet-aligned replay when not.
- **Substitute cycle joints are OLA-spliced.** Cyclic replay previously
  wrapped with a bare index reset, landing the −1 dB step exactly at the
  wrap. Each joint now gets a ~1 ms raised-cosine tail→head crossfade
  (capped at period/3) with the gain step inside the fade; the resume
  index skips the pre-blended head samples so the period is preserved.
- **Presets re-tuned for the honest loss range:** Dropped Call
  PACKET_LOSS 45 → 65 (~51% true loss — the call actually drops);
  Total Media Failure 55 → 90 (~86% true loss, up from the ~17% the old
  mapping delivered).

### Testing
- Probe O (GE statistics) bounds re-derived for the new mapping
  (lostFrac 0.268, r̂ 0.472 at LOSS 40 / BURST 30) — measured 0.263 /
  0.469. Three new probes, 50/50 green: O2 knob-zero clean (0 lost of
  1474 packets at PACKET_LOSS 0), O3 full-knob true failure (lostFrac
  0.986), P2 decay mute-out (113 masked runs ≥ 5 lost packets: rep 1
  audible, reps 4+ exactly silent). All QUAL-02 block-size/ragged
  bit-identity and determinism probes unchanged and green.

### Added
- **Mono compatibility** — the plugin now loads on mono→mono and
  mono→stereo bus layouts in addition to stereo→stereo. Root cause of the
  previous behavior: the layout was hard-locked stereo in three places
  (bus constructor, `isBusesLayoutSupported`, and a `< 2`-channel
  early-return in `processBlock` that passed audio through untouched).
  Mono input is captured dual-mono into the ring; mono output takes the
  left engine channel; mono→stereo duplicates the input and runs the
  stereo path unchanged. Stereo→mono remains rejected (no downmix rule).

### Changed
- Nothing in the stereo path — verified bit-identical (all 44 pre-existing
  harness probes unchanged and green).

### Testing
- Three new harness probes (47/47 green): M1 mono→mono delay-compensated
  bit-exact null; M2 mono ch0 bit-identical to a dual-mono stereo render
  under maximum degradation (all families forced, GSM codec, crush with
  jitter/dither — proves RNG stream alignment between layouts); M3
  mono→stereo null on both output channels with junk-filled ch1 input the
  processor must discard.

## [1.0.0] — 2026-08-16

Initial release.

### Added
- **Six degradation families** over a shared capture-ring engine — Tape
  (wow, drag, full stops), CD Skip (buffer loops, restart chirps), Vinyl
  (revolution jumps, pops, locked grooves), Packet Loss (Gilbert–Elliott
  bursty loss with four concealment modes), Codec (Mu-law / GSM 06.10),
  and Crush (fractional bit quantize + sample-rate reduce with jitter,
  envelope depth, and TPDF dither).
- **Shared-buffer stochastic engine** — one media clock arbitrates Tape /
  CD / Vinyl events per tick; Packet, Codec, and Crush run as serial
  post-stages.
- **Seeded determinism** — a 0–9999 SEED parameter drives all 8 RNG
  streams; identical seed + input renders bit-identical output. Dice
  button rerolls from the UI.
- **Sync / free clocking** — tempo-synced divisions (1/16 – 1 bar) or free
  rate (0.1 – 20 Hz); HARD EDGES toggles splice crossfades off.
- **WebView UI** (900 × 620, Ouaricon Naturalist) — 3×2 family plate grid
  with per-family event LEDs, global strip with clock swap slot, seed
  ledger, and mix.
- **Preset system** — shared preset-manager module v1.0.5: save / save-as /
  load / load-from-file / prev / next / two-click delete, factory + user
  banks under `~/Library/Ouaricon Bitrot/Presets/`.
- **Factory bank** — 8 presets: Worn Cassette, Skipping Disc, Locked
  Groove, Dropped Call, Cellphone 1998, Eight-Bit Ruin, Total Media
  Failure, Gentle Rot.
