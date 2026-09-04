# Changelog — O-simpleGrain

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.4.3] — 2026-09-03

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
- **The `OSIMPLEGRAIN_VERSION_CODE` hex mirror moved with the version**,
  `0x010402` → `0x010403`. It is the only such mirror in the suite; it was
  verified to actually mirror the old version before being touched, rather than
  assumed.
- Two Stage-N history notes in `js/i18n.js` reworded so the source states the
  current term and the CHANGELOG carries what it replaced. Worth recording: one
  of them shows this page's `aria.helpToggle` ORIGINALLY said *les infobulles*
  and the Stage-N pass replaced it. This release restores the plugin's own
  wording.


## [1.4.2] — 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed
- **item 57 — Pitched Buzz tour caption (`label.captionPitchedBuzz`):** the
  French caption wrapped and moved the whole rack down one line, in a state no
  gate drove. Measured NOWRAP on the live `#tourCaption` at the 900 × 760 frame:
  997.22 px natural in the 846 px box (the 838.58 px Stage N reported was the
  wrapped Range box — the widest LINE, not the string), English 693.00. Wrapped,
  the caption stood 26.38 px tall instead of 13.19, the header grew 80.38 →
  93.56 and `.workspace` (the rack) moved y 109.38 → 122.56 — 13.19 px, French
  only. `tests/i18n-states.json` named `captionAsyncCloud` as the longest
  caption and never picked Pitched Buzz, so `check-ui-labels` reported 0
  moved. The Pitched Buzz state is now in `tests/i18n-states.json`; with it
  and the OLD caption, assertion 7 fails with 159 non-label elements moved
  (`.header` dh +13.2, `.workspace` and everything under it dy +13.2) — the
  proof the state is driven. Caption reworded shorter, same claim, both
  languages (correction 43): en "Pitched Buzz — tiny grains fired fast and in
  sync. Their rate becomes an audible pitch (a comb): granular can make tone,
  not just texture." 606.84 px; fr "Bourdon harmonique — grains minuscules,
  déclenchés vite et en phase. Leur cadence devient une hauteur audible
  (un peigne) : le granulaire peut faire du timbre, pas que de la texture."
  813.31 px, 32.69 px of slack. After: caption 13.19 px tall, header 80.38,
  `.workspace` y 109.38 in both languages; `check-ui-labels` 0 moved across
  default + 4 states. The lesson tip body (`lessonPitchedBuzz`) keeps the full
  wording — a tip has no line budget. French `reviewed: false`.
- **item 54 — hover-help switch accessible name (`aria.helpToggle`):** English
  "Toggle tooltips" → "Toggle hover help" (`i18n.js`). The switch's own tip
  title is "Hover help" and the French already read *Activer ou désactiver
  l’aide au survol* — one name per control, settled for the whole family.
  French unchanged, `reviewed: true` kept.
- **item 58 — focus latch (`js/app.js` `setupTooltips`):** a pointer click on
  any anchor that takes focus (the eight `.tour-btn` lesson buttons, the gear,
  the language select) hid the hover tip on `pointerdown` and then re-opened
  it from `focusin`, so the tip came straight back under the pointer. Ported
  the Stage M `lastInputWasPointer` latch (O-Comp v1.7.0): `pointerdown`
  latches, any `keydown` releases, `focusin` opens only while released,
  `focusout` hides, Escape hides. The page's one programmatic `.focus()`
  (popover Escape → gear) follows a keydown, so the gear's tip still opens —
  keyboard-driven. Probe with real events, hover-help driven on through the
  page's own toggle, both languages: click `.tour-btn[Fragments]` → tip
  "Fragments" shown BEFORE (FAIL), hidden AFTER; Tab → "Smooth Cloud" /
  *Nuage lisse* shown before and after; Escape hides; Tab again → tip; click
  on a knob → no tip both ways (knobs never took focus from a click). 2 of 12
  checks failed before, 12 of 12 pass after, 0 page errors.

### Changed
- `tests/i18n-states.json` gains the Pitched Buzz state (5 states driven).

No DSP change: nothing under `Source/` outside `ui/public/` changed. The
render harness passes 15/15 before and after; its printed rms/peak fields are
NOT byte-comparable run to run — each voice's `juce::Random` (`GrainVoice.h:541`)
is default-constructed, i.e. time-seeded, so two runs of the SAME binary differ
(reported, not fixed: not a Stage O item). No parameter, preset or state-tree
change.

## [1.4.1] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **59 French entries revised** (of 115) against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint (`scripts/i18n-fr-lint.js`):
  92 findings to 0, `--strict` exit 0. 13 terminology, 36 typography,
  10 meaning, 0 grammar. The visible ones:
  - **Scatter is now *Dispersion*** on the knob and in its tooltip title (it
    read *Étalement* / *Étalement temporel*, which the glossary settles on
    *spread*). The group heading follows: *Spray & Scatter* is **Dispersions**.
  - **Ten tooltip bodies now name the FRENCH control faces.** They referred to
    *Scan*, *Freeze*, *Pitch Spray*, *Scatter*, *Overlap* and *Taper* — English
    captions that do not appear anywhere on the French page, which shows
    *Balayage*, *Gel*, *Dispersion hauteur*, *Dispersion*, *Recouvrement* and
    *Fondu*. The window-shape names (*Rectangular*, *Hann*, *Gauss*, *Tukey*)
    still stay English in both languages: they are the host automation contract
    and the user reads them on the combo.
  - **One French name per control.** The Rect Click lesson button reads
    *Clic rectangulaire* rather than *Clic rect.* (its own tooltip already said
    so, and the fixed 110.5 px tour grid had 32 px to spare); the two spray
    tooltips now use their own captions' wording; the hover-help switch's
    accessible name is *Activer ou désactiver l'aide au survol*, the same words
    as its tooltip title.
  - **Typography:** straight apostrophes to typographic ones throughout (50),
    no-break spaces before `:` and `;` (35) and between a number and its unit
    (2). The on-screen-keyboard hint got back the hair spaces between the key
    letters that the English has and the French draft had dropped.
  - *Préréglages conceptuels* to *Préréglages pédagogiques*, and *Taille du
    grain* to *Taille de grain*, both suite-settled terms.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

No English copy, key, tooltip binding, selector or CSS rule changed; no DSP
change. The French remains `reviewed: false` — no native speaker has read it.

## [1.4.0] — 2026-08-28

**Longer grains, a Tukey window with a Taper knob, the envelope beside the
control it explains, and a rectangular window that no longer clicks.**
Parameter set grows by one (`windowTaper`); the `windowShape` choice gains
"Tukey" as index 5. Both additions are backward compatible: the APVTS stores
denormalised values, so pre-1.4.0 sessions and factory presets restore the
same grain size and window they saved.

### Added
- **Tukey window** (`windowShape` = 5) — a flat top with a Hann-shaped fade at
  each edge, the fade length set by the new **Taper** knob (`windowTaper`,
  0–100 %, default 50 %). 0 % is the flat rectangular window, 100 % is a full
  Hann. Not a sixth LUT: the taper IS a Hann half, so the window is one phase
  remap into the existing Hann table (`WindowLuts::read` with a per-grain
  `taperEnd`, fixed at spawn by `WindowLuts::taperEndFor`). No transcendental
  in the grain loop. The Taper cell dims and locks for every other shape.
- **Envelope inset beside the Shape combo.** The UI-03 window inset moved from
  the waveform's corner into the Window group, between the Shape combo and the
  Taper knob, and now redraws on shape, taper AND grain-size changes (the rect
  guard is a fixed time, so its footprint depends on the grain length). Row
  budget measured at 270 px: combo 104 + inset 84 + knob 56 + 2 × 12 gap, all
  overrides scoped to `.group-window` — the shared combo min-width (132 px)
  wrapped the knob onto a second row.
- Render-harness gates `rect-guard-is-a-ramp` (direct `WindowLuts` probe;
  negative control with `kRectGuardMs = 0` FAILS it — verified),
  `tukey-between-rect-hann` (α = 30 % sits between rect and Hann in top-octave
  energy; α = 100 % lands within 15 % of Hann) and `grain-500ms-bounded`
  (500 ms × 200 g/s renders finite, peak 0.51). 15/15 PASS.

### Changed
- **Grain Size ceiling 200 → 500 ms.** Skew 0.35 (was 0.4) keeps the fine low
  end; the knob's midpoint now sits near 70 ms. Tooltips updated (EN/FR).
- **Rectangular window rounded off.** Root cause of the clicks: the rect LUT
  was flat 1.0 to the very first and last sample, so every grain edge was a
  full-scale step. It now carries a fixed **1 ms guard fade** at each edge
  (`WindowLuts::kRectGuardMs`), applied through the same Tukey remap — a time,
  not a phase fraction, so it stays inaudible at 500 ms and clamps to a full
  Hann inside a 2 ms grain. The pedagogical contrast survives: rect still
  carries 2.45× Hann's >12 kHz energy in the `window-rect-clicks` gate (unchanged
  numbers), and the "Rect Click" preset still selects it. Tooltip and preset
  captions reworded from "has no fade / clicks" to "flat with a 1 ms guard /
  starts and stops abruptly" (EN/FR, FR flagged `reviewed: false`).
- Inset caption "Window" → "Envelope" (it now lives inside the Window group).

### Testing
- Render harness 15/15 PASS (12 existing gates unchanged, incl.
  `window-rect-clicks` ratio 2.45 and `noteoff-click`). Negative control:
  `kRectGuardMs = 0` fails `rect-guard-is-a-ramp` AND the pre-existing
  `noteoff-click` gate — the guard is load-bearing for both.
- Headless UI render (scripts/serve-ui.js + Playwright): Window group lays out
  on one row at the 900 × 760 editor size; no console errors.
- auval + install (VST3 + AU) — see NOTES.md.


**The PAGE speaks French, not only the hover help** (Stage I batch I2, canon v2).
Every caption, heading, button face, hint, readout key, status line and tooltip
switches with a language selector in a new header gear. Value readouts and the
two drop-down menus stay English by decision (D-03 / D-01). No parameter, DSP or
audio change; the state format gains one optional property and stays backward
compatible in both directions.

### Added
- **Interface language, English + French, with no reload.** A gear at the right
  of the header opens a settings panel holding the language selector. 113 French
  entries (37 tooltip, 76 label), all machine-drafted and flagged
  `reviewed: false` — no native speaker has read them.
- `Source/ui/public/js/i18n.js`: `LANGUAGES`, `I18N` (37 tooltip pairs),
  `LABELS` (76 single strings), `I18N_EXEMPT` (4 reasoned exclusions),
  `TIP_BINDINGS` (37 anchors) and `tr()`. Embedded as `UIBinaryData::i18n_js`
  and served at `/js/i18n.js`.
- C++ language pair `getUiLanguage` / `setUiLanguage` over a
  `std::atomic<int> uiLanguage`, persisted as a plain `uiLanguage` property on
  the APVTS root — the string `"en"`/`"fr"`, restored behind an `isVoid()` gate
  because the ValueTree/XML round-trip rebuilds every property as a string var.
  A pre-1.3.0 session has no property and stays English; a 1.3.0 session opened
  by an older build ignores the property.
- `tests/i18n-states.json` drives the three states the gate cannot reach on its
  own: the popover open, the hover-help switch in its Off position, and a lesson
  preset picked. Coverage went 50/53 to 53/53 labels measured.

### Changed
- **The hover-help "?" chip MOVED into the settings panel.** It was a lone chip
  at the end of the preset bar; a plugin should not grow a second settings
  surface, and the two settings that decide what the hover help says and whether
  it says it belong together. Its storage is untouched — still `localStorage`
  under `osg.tipsEnabled` — so a preference set before this version survives the
  move. Its face is now written as "On"/"Off" through `setLabel()` behind an
  if/else, never a ternary in the argument.
- Tooltip copy moved out of `js/app.js`'s `TIPS` object into `js/i18n.js`; the
  renderer reads the anchor's own `data-tip-title` / `data-tip`, which
  `applyI18n` rewrites per language. All 34 entries plus the 8 lesson captions
  and the 12 toast / status strings were compared back to v1.2.1 byte-for-byte
  with entities decoded rather than re-typed — **141 English strings, 0
  mismatches**.
- The 34 markup anchors moved off `data-tip` (which now carries the tip BODY):
  the 15 knob cells and 2 select cells to `data-param`, moved up from the inner
  `.knob` div where nothing read it; the 2 toggles to a new `data-param`; the 4
  visualization cells and the readout strip to an id; the 8 lesson buttons to
  the `data-preset` they already carried. Tooltip listeners are delegated on the
  document, because no anchor carries `data-tip` until the first sweep runs.
- Tip bodies lost their `strong`/`em`/`code` emphasis tags. The words are
  unchanged; the renderer builds with `createElement` + `textContent` now rather
  than `innerHTML`.
- The 8 lesson captions were a `LESSONS` table written with a raw `textContent`
  assignment; they are table entries written through `setLabel()` now,
  dispatched from the C++ preset name so every key is a plain string literal.
- The toast and the source-status line were raw `textContent` writes of finished
  English; each call site names its own key now, so both elements join the
  language sweep instead of being stranded in whichever language raised them.
- **The lesson caption is the header's own full-width row.** It always was a
  `flex-basis:100%` row — it was just 100% of the 571px preset bar rather than
  100% of the 846px header. Costs nothing and removes a narrow box that both
  languages were fighting.
- **The 8 concept-preset chips are a fixed 4x2 grid, not a wrapping flex row.**
  See the geometry note below.

### Fixed
- **Pre-existing: 15 knobs and 2 combos carried `role="slider"` / `tabindex` (or
  a focusable select) with an accessible name that only existed if the tooltip
  loop happened to run.** They are named by `data-i18n-aria` now, resolving to
  the control's own tooltip title — 26 keyed attributes, 25 of 26 confirmed
  switching language by the gate (the 26th is `aria.langSelect`, whose English
  and French differ but which the vacuity counter reads once).

### Geometry (D-04) — six rules, each measured, each reverted alone
Every rule below was reverted on its own from an in-memory copy and the gate
re-run; all six re-break, so none is decoration. The two French copy
shortenings are an OR rather than an AND and are labelled as such.

1. `.title-block { flex: 1 1 0; min-width: 0 }` (was `flex-shrink: 0`). The
   block shrink-wrapped its widest child, the strapline: French made it 73.8px
   wider (275.0 -> 348.8) and, at 348.8 + the 481.7px preset bar + the 32px gear
   cluster = 862.5 in an 846px header, **the gear wrapped onto a second row and
   took 22px of header height with it** — pushing the workspace, the readout,
   the rail and the keyboard down 22px in French only. 156 elements moved.
   `flex: 1 1 auto` was the first attempt and changed nothing: flex line-breaking
   runs on the hypothetical main size before any shrink is applied. At basis 0
   the row measures 513.7 in both languages.
2. `.subtitle { min-height: 2.2em }`. Pinned to 332.3px by rule 1, the English
   strapline is one line and the French is two. Reserving the line is D-04's
   "let it wrap where the row can afford the height"; a shorter French strapline
   is the short-variant answer D-04 rules out.
3. `.group-spray .knob-label { min-height: 2.2em }`. Two of the six Spray &
   Scatter captions wrap in French and not in English ("Pos Spray" 1 line vs
   "Dispersion position" 2; "Pan Spray" 1 vs "Dispersion stéréo" 2), each
   pushing its own `#val-` readout down one line-height. Scoped to that group
   rather than all seventeen knob labels; the group already carries two 2-line
   captions in English, so the wrapped line they sit on costs nothing.
4. `#toggle-freeze { min-width: 78px }`. **French is SHORTER here** — "Gel"
   57.8px against "Freeze" 77.5px — so the button shrank 19.6px and the corner
   fleuron moved with it. 78px is the English width rounded up: English moves
   0.5px. Not applied to the other toggle, which reads "ADSR" in both languages.
5. `.grain-readout .readout-key[data-i18n="label.readoutOverlap"]
   { min-width: 91px }`. The three readout keys sit in one left-aligned strip,
   so any key's width positions everything to its right: "OVERLAP" is 50.8px and
   "RECOUVREMENT" is 90.1px, moving `#readoutOverlap`, the whole CPU item and
   `#cpuBar` 39.3px right in French. **The cost is paid in English and is
   visible**: a 40px gap opens between OVERLAP and its `×0.0`. The strip can
   afford it — content ended at x=411 in a 532px box — and "recouvrement" is the
   term the French granular literature uses, so shortening it would be the
   short-variant answer D-04 rules out. The other two keys are `sameAsEn` and
   need no pin.
6. `.preset-bar-tour` is a fixed 4x2 grid. As a wrapping flex row the eight
   chips shrink-wrapped their captions and **re-wrapped when the language
   changed**: "Async Cloud" sat on row 1 in English and "Nuage async" fell to
   row 2 in French, where the settings panel hanging from the gear above covered
   it. Four equal columns give every chip the same box in both languages, so the
   bar cannot reflow at all. The chips are 110.5px each now rather than 60–90px
   of shrink-wrap; the block is the same 460px wide and the same two rows tall.

**One French string was shortened rather than fitted, and is flagged for the
reviewer.** `label.vizScope` + `label.vizScopeHint`: the scope caption box is
261px, the English pair is one line (11.0px) and "Oscilloscope de sortie ·" +
"la forme d'onde après gain" was two (22.0px), which shrank the scope canvas
11px in French only and made the two spans' union rects intersect where they are
disjoint in English. The other three visualization cells are two lines in BOTH
languages already, so only this one flagged. It now reads "Oscilloscope ·" +
"l'onde après le gain". Reverting either half alone still passes; reverting both
fails. A reviewer may prefer the longer pair at a cost of 11px of scope canvas.

**Frame cost: 0.2px.** The keyboard panel's bottom edge is 716.2px at v1.2.1 and
716.4px at v1.3.0 inside the 754px client area, and identical in both languages;
the frame does not scroll in either version or either language. The header is
80.2px at v1.2.1 and 80.4px here — moving the lesson caption onto its own row
paid for the reserved strapline line almost exactly. **The 900x760 frame is
untouched.**

### Testing
- `check-i18n.js --plugin O-simpleGrain`: exit 0, all 15 assertions, canon v2.
- `check-i18n.js --strict-v2`: exit 0 — 11 plugins on canon v2, 0 on v1.
- `check-ui-labels.js --plugin O-simpleGrain`: **ALL CHECKS PASSED** across four
  states with **zero** non-label elements moved between English and French;
  vacuity 46/53 labels and 25/26 attributes confirmed switching;
  `dataset.label === textContent` after init, after the switch and after a state
  pass in both languages; 53/53 labels measured; no page error; every resource
  served.
- `boot-all-uis.js`: O-simpleGrain clean — 64 text, 26 aria, 0 title, 53 i18n.
  (O-Bowed and O-Reed still fail repo-wide with `Unexpected token 'export'`;
  pre-existing and out of scope.)
- Render harness: **ALL PASS, 12 probes**, at both v1.2.1 and v1.3.0.
  **The harness is NOT deterministic** — two consecutive runs of the same binary
  give `makes-sound` rms 0.0119 and 0.0121, `stress-bounded` peakGrains 158 and
  144, `uptranspose-stable` peak 2.187 and 2.084. Its printed numbers cannot be
  compared digit-for-digit across versions; the comparable quantity is the
  verdict, and that is 12/12 PASS in both. No DSP source was touched.
- `auval -a` lists `aumu OsGr OuDv`.

### Not verified
- The C++ persistence round-trip has never been executed by hand on this plugin:
  pick Français, save, quit the DAW, reopen, confirm the choice held, and confirm
  a fresh instance opens in English.
- Windows/WebView2 font metrics remain a named deferral — a French label measured
  as fitting on macOS could clip there.

## [1.2.1] — 2026-08-25

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
  Fix in `GrainVoice.h`: envelope params are cached each block but only pushed to
  the live envelope(s) when their values actually change AND the voice is not in
  its release phase; changes made mid-release apply at the next note-on. The
  release therefore always completes at the rate captured at note-off.
- Render-harness: new `noteoff-click` probe (sustain 0, slow decay, note-off
  mid-decay) asserting the release tail still rings after note-off.

### Testing
- Render harness: ALL PASS including the new probe (preRms 0.0068 / tailRms 0.0047).
- Negative control: probe re-run against v1.2.0 voice code fails as expected
  (preRms 0.0051 / tailRms 0.0000 — the tail is truncated to exact silence one block after note-off).

## [1.2.0] — 2026-08-09

Header layout fix and a tooltip on/off toggle. UI-only — no parameter, DSP, or
state-format changes (sessions/presets load unchanged).

### Added
- **"?" button in the header toggles tooltips on/off.** Sits at the end of the
  concept-preset tour bar; state persists across editor opens (localStorage,
  default on). Disabling also hides any tooltip currently showing.

### Fixed
- **Title "O–simpleGrain" wrapped across three lines.** The header's flex layout
  let the wide preset bar squeeze the title block. The title block no longer
  shrinks (`flex-shrink: 0`, `white-space: nowrap`) and the preset bar is pushed
  to the right edge (`margin-left: auto`), so all slack goes to the title.
- **Tooltips appeared in duplicate.** `setupTooltips()` set a native `title=`
  attribute as a fallback on every `data-tip` element, so hovering showed both
  the OS-native tooltip and the custom floating one. The copy now goes into
  `aria-label` (assistive tech keeps it; no native popup) and any stale `title`
  attributes are stripped.

## [1.1.3] — 2026-08-08

First published release (version aligned with the O-simple pedagogical suite).

### Changed
- Cross-platform release builds (macOS VST3+AU, Windows VST3, Linux VST3) via GitHub Actions
- AGPL-3.0 notice headers added to all Ouaricon-authored sources

## [1.1.2] — 2026-07-16

Resolves the nine deferred Info findings from the 2026-07-15 CODE_REVIEW.md
(IN-01..IN-09). Internal quality: no parameter IDs, ranges, or state format
changed (sessions/presets load unchanged).

### Fixed
- **IN-07: grains sprayed out of range emitted constant-value (windowed DC)
  thumps.** A spawn at `playhead ± up to 100%` of the source length could start
  outside `[0, sourceLen)`, where the clamped Lagrange taps all read the edge
  sample — at 100% position spray up to half the spawns were affected. Spawn
  positions now wrap into range, consistent with the playhead's own wrap. (Under
  heavy spray the cloud now carries real signal where the thumps were.)
- **IN-03: Position-knob glide speed depended on the sample rate.** The rest-
  ease was a fixed 0.0008/sample, ~2× faster at 96 kHz than 44.1 kHz. Now derived
  in `prepareToPlay` from a τ ≈ 28.3 ms time constant (`1 − exp(−1/(τ·fs))`),
  reproducing the shipped 44.1 kHz feel at every rate.
- **IN-04: float read positions quantized near the tail of long sources.** At
  96 kHz a 10 s source spans 960k samples where float ULP is 0.0625 samples —
  fractional increments jittered pitch/interpolation for late-reading grains.
  `Grain::readPos`, the voice playhead, and the processor→voice handoff are now
  double end-to-end.
- **IN-08: source status/thumbnail refreshed on fixed timers that raced the
  decode.** The Load… flow polled 1.2 s after the *click* (stale after a longer
  browse); combo switches polled 300 ms after the change. The processor now bumps
  a source-version counter on every successful publish; the editor timer emits a
  `sourceChanged` WebView event on change, and the JS drives the thumbnail +
  truncation status from that (a pending-label handoff keeps the drop's filename
  in the status line).

### Changed (performance / internal)
- **IN-02:** the audio thread no longer runs 2×8 RTTI `dynamic_cast`s per block
  (plus more in `prepareToPlay`) — voices are cached as typed pointers at
  construction (synth-owned for the processor's lifetime).
- **IN-09:** `prepareToPlay` skips the built-in re-decode + resample when the
  published source is already at the engine rate (hosts re-prepare on every
  buffer-size change; decoding 10 s of audio each time was a pointless stall).
  The dropped/user path already skipped via the v1.1.1 CR-01 fix.
- **IN-01:** deleted the editor's dead "reserved" `fileChooser` member (all
  picking goes through the processor's own chooser).
- **IN-05:** the JS grain meter reads `kGlobalGrainCap` pushed once via WebView
  initialisation data instead of a hardcoded 192; the window-formula/σ JS
  re-implementation is pinned by explicit CONTRACT cross-references in both
  WindowLuts.h and app.js.
- **IN-06:** `applyFactoryPreset`'s 19 parameter writes are wrapped in
  begin/endChangeGesture pairs (hosts recording automation logged the ungestured
  writes oddly; strict hosts warn).

### Tests
- All 11 render-harness gates PASS; `auval` SUCCEEDED;
  `pluginval --strictness-level 10` SUCCESS.

## [1.1.1] — 2026-07-16

Resolves the 2026-07-15 CODE_REVIEW.md findings CR-01, CR-02, WR-02, WR-03,
WR-04, WR-05 (WR-01 was already fixed in v1.0.2 — see the v1.1.0 recovery note
below). No parameter IDs, ranges, or state format changed; the `adsrEnabled`
param recovered in v1.1.0 is unchanged.

### Fixed
- **CR-01: a dropped source was silently replaced by the "fire" built-in on
  every host re-prepare.** `prepareToPlay` only special-cased `embedded:`
  identities; a `dropped:<name>` identity (no disk path — WKWebView strips it)
  fell through to the missing-file fallback, so any buffer-size change, engine
  stop/start, or offline bounce discarded the user's live sound mid-session
  (and `juce::File("dropped:…")` fired a debug `jassert`). Now: the raw dropped
  bytes are retained (≤32 MB cap) and re-decoded at a new engine rate; with the
  rate unchanged the published buffer is kept as-is; with the bytes gone the
  live buffer is kept (transposed at worst) — the built-in fallback only runs
  when nothing is realisable (fresh-instance restore of a name-only identity).
  A vanished picker-file path likewise keeps the live buffer instead of
  clobbering. `setStateInformation` got the same identity guards (no
  `juce::File` on non-path strings; same-instance restores reuse the retained
  bytes). *Root cause: the state-restore fallback ran on the live-session
  re-prepare path, where the source it discarded was still published.*
- **CR-02: data race on `currentSourceIdentity` / `currentSampleRate`.** The
  COW `juce::String` identity was written from host-controlled threads
  (`prepareToPlay`, `set/getStateInformation` — VST3 hosts may call these off
  the message thread) and the message thread (drop/picker/preset callbacks)
  with no synchronization — two unsynchronized ref-count ops on one String can
  double-release (UAF). Every access now goes through a `sourceStateLock`
  CriticalSection (never taken on the audio thread); the identity accessor
  returns by value. `currentSampleRate` (plain double, same multi-thread
  pattern) is now `std::atomic<double>`. *Root cause: the sibling state on the
  atomic-publish path was never given the same care as the buffer pointer.*
- **WR-02: whole file decoded before the 10 s cap was applied.** The cap lived
  in the resampler, after a full-file allocation + decode — a 45-min WAV cost a
  ~500 MB spike and a multi-second stall, and `lengthInSamples` (int64) was
  truncated straight to `int`, so a hostile header near INT32_MAX drove a
  multi-GB allocation (`std::bad_alloc`). The decode length is now clamped to
  the cap *before* allocating (int64 math), and the truncation notice fires for
  pre-truncated files too.
- **WR-03: lesson presets randomly kept or discarded a user-loaded source.**
  `applyFactoryPreset` reset `sourceSample` to its default with everything
  else, so pressing any concept button discarded a dropped/picked source *iff*
  the last built-in choice differed from fire — invisible state deciding
  whether your sound survived. Contract now explicit: **presets keep the
  current source** (`sourceSample` is skipped in the reset); **"Granular Fire"
  alone force-loads fire** via `loadBuiltInSource` (the old `setChoice` was
  silently suppressed by the APVTS when the choice already read fire, so the
  preset didn't actually load fire either).
- **WR-04: version drift.** Sources said 1.0.1, the harness said 1.0.1, the
  installed binary said 1.1.0, PLUGINS.md said 1.1.0 (see the v1.1.0 recovery
  note). All version sources now derive from one `OSIMPLEGRAIN_VERSION`
  variable in CMakeLists.txt (the harness's hand-rolled
  `JucePlugin_VersionString/Code` included) — reconciled at 1.1.1.

### Changed (performance / internal)
- **WR-05: the audio thread no longer takes a hidden mutex per block.**
  `std::atomic_load/store(shared_ptr&)` is not lock-free (libc++ backs it with
  a global mutex pool shared across the process) — a real priority-inversion
  risk on the RT path. The audio thread now reads one genuinely lock-free
  `std::atomic<AudioBuffer*>` view per block; shared_ptr ownership stays on the
  message/host side under `sourceStateLock`, and an outgoing source is parked
  in a retired list, freed only once ≥2 audio blocks have completed since the
  swap (per the O-MicrotonalSampler v1.24.0 pattern). Behaviour identical;
  memory bound: at most one parked 10 s buffer between publishes.

### Tests
- Added render-harness gate **11 (`adsr-bypass`)** — guards the reconstructed
  v1.1.0 feature: bypass ignores a 1.5 s attack (flat velocity level at once)
  and drains within ~a grain length of note-off, while the enabled envelope
  still ramps in and tails out. All 11 gates PASS; `auval` SUCCEEDED;
  `pluginval --strictness-level 10` SUCCESS.

## [1.1.0] — 2026-06-25 (source reconstructed 2026-07-16)

**Recovery note:** v1.0.2 and v1.1.0 were built, installed, and recorded in
PLUGINS.md but their source was never committed, and the working tree was later
reverted to v1.0.1 — the 2026-07-15 code review unknowingly reviewed the
regressed tree (its WR-01 "dead keyboard" finding was the already-fixed v1.0.2
bug resurfacing). v1.0.2 was restored from `backups/O-simpleGrain/v1.0.2/`;
v1.1.0's UI delta was recovered byte-exact from the installed binary's embedded
resources and its C++ side re-implemented from the recovered spec below.

### Added
- **`adsrEnabled` parameter (19th param, bool, default ON)** — an "ADSR" toggle
  in the envelope panel. **On** = the v1.0.x per-voice A/D/S/R behaviour,
  unchanged. **Off** bypasses the envelope: each note plays at a flat velocity
  level while held and, on release, the voice simply stops launching new grains
  so the cloud fades out over one grain length through the *Window* envelopes
  (no click) — a raw, immediate gate for the pedagogical "hear the grains
  themselves" use. The A/D/S/R knobs dim + lock while bypassed
  (`.env-bypassed`); the envelope still ticks internally so a mid-note toggle
  lands on a coherent state.

## [1.0.2] — 2026-06-25

Three user-reported bugs: a dead on-screen keyboard, a dead output scope, and an
overall-too-quiet output. No parameters, IDs, ranges, or state format changed
(sessions/presets load unchanged).

### Fixed
- **On-screen keyboard produced no notes.** The WebView keyboard calls the
  `uiMidi` native function on every key, but it was never registered on the C++
  side — and the processor had no `MidiMessageCollector` and no merge of UI notes
  into `processBlock`, so the entire UI-MIDI bridge (present in O-simpleFM) was
  missing. Keys highlighted but emitted nothing. Ported the proven O-simpleFM
  pattern: `midiCollector` member + `reset()` in `prepareToPlay` +
  `removeNextBlockOfMessages()` in `processBlock` + `handleUiMidi()` +
  `withNativeFunction("uiMidi", …)` in the editor. External MIDI was unaffected
  and still works. *Root cause: the Stage-3 UI-MIDI bridge was never wired; no
  automated gate exercised it because the render-harness injects MIDI directly.*
- **Output was ~6–12 dB too quiet on sparse/single-grain patches.** The master
  stage applied a *fixed* `kHeadroom = 0.5f` (−6 dB) sized to stop dense clouds
  clipping. But a single grain peaks near the source level, so that fixed cut —
  stacked on the equal-power pan and amp envelope — left sparse patches far too
  quiet (the deferred "overlap-aware normalization" the code comment promised
  never landed; nor did the "headroom normalisation upstream" the tooltip claims).
  Replaced it with overlap-aware normalization: `normGain = 1 / max(1, overlap×0.5)`
  where `overlap = grainSize × density`. Sparse/single grains now play at full
  level; dense clouds stay tamed below clip. Smoothed via the existing `outputGain`
  ramp (click-free). *Root cause: a fixed headroom factor cannot serve both the
  sparse and dense ends of the density axis.*
- **Output scope showed nothing.** A downstream symptom of the two bugs above —
  the scope data path (post-gain ring → analyzer → `scopeUpdate` → `drawScope`)
  was correct, but with the keyboard dead and the output very quiet there was
  nothing to draw. Restored by the keyboard + loudness fixes; no scope code changed.

### Tests
- Added render-harness gate **10 (`ui-midi-keyboard`)**: injects a held note via
  `handleUiMidi` and renders with an **empty host MIDI buffer**, asserting the
  collector drains the note and the synth sustains audible output — guards the
  UI-MIDI bridge that had no coverage and shipped silent in v1.0.1.

## [1.0.1] — 2026-06-25

Code-review fixes — two correctness bugs, two real-time hot-path simplifications,
and test coverage for the bug that had none. No parameters, IDs, ranges, or state
format changed (sessions/presets load unchanged).

### Fixed
- **Velocity → Density was 100× over-scaled** — a hard switch instead of a graded
  depth. The `velToDensity` parameter is stored 0–100 (%), but `GrainVoice` consumes
  it as a 0..1 depth; the processor pushed the raw 0–100 value, so any setting above
  ~1 % slammed the effective density to its rail (1 or 200) for any non-mid velocity.
  Now scaled ×0.01 at the push site (`PluginProcessor::processBlock`). The control is
  smooth across its full travel again. *Root cause: missing unit conversion between
  the 0–100 % param range and the voice's documented 0..1 depth contract; it escaped
  Stage-2 validation because the render-harness pinned `velToDensity` to 0.*
- **Restored "load-your-own" source could be clobbered on session reload.** A
  user/dropped source restored in `setStateInformation` could be overwritten by the
  built-in chosen by `sourceSample`, because `replaceState()` queues a deferred
  `AsyncUpdater` rebuild that ran *after* the synchronous `suppressChoiceRebuild`
  guard had already been cleared. Now the restore publishes the correct source and
  then `cancelPendingUpdate()`s the queued rebuild; the ineffective guard flag was
  removed. *Root cause: a synchronous flag cannot gate a deferred async callback.*

### Changed (performance / internal)
- **No transcendentals in the per-sample grain render loop.** Equal-power pan gains
  (`cos`/`sin`) and the anti-aliasing one-pole coefficient (`exp`) are constant for a
  grain's life but were recomputed every sample for every active grain (up to 192).
  They are now computed once on spawn and stored on `Grain` (`panL`/`panR`,
  `aaCoeff`/`aaEngaged`); the inner loop is a multiply / branch + multiply-add.
  Behaviour is equivalent — purely a hot-loop hoist (CPU win scales with cloud
  density). The AA bypass edge (`state = x` at rate ≤ 1) is preserved.

### Tests
- Added render-harness gate **9 (`velToDensity-depth`)**: asserts the grain count is
  velocity-independent at depth 0 and tracks velocity at full depth — guards the
  scaling fix above from regressing (the param was previously exercised by no gate).

## [1.0.0] — 2026-06-25

First release. A pedagogical **granular synthesizer** with a field-guide "Naturalist"
WebView UI, built to make *"oh, THAT's what granular synthesis is"* land in a few
minutes — single grains, grain clouds, freeze, and the sync→async axis, each made
visible and audible.

### Synth engine (DSP)
- 8-voice polyphonic **granular** instrument. Each `GrainVoice` schedules grains from a
  preallocated `std::array<Grain, 24>` (steal-oldest when full) against a global cap of
  **192 grains** — no allocation or locks in `processBlock`, no xruns under stress.
- **Density** scheduler (1–200 grains/s) with a derived live **overlap** readout
  (`grainSize × density`); **Grain Size** 2–200 ms is the buzz↔fragments control.
- Five precomputed 2048-pt **window** LUTs (Rect / Tri / Welch / Gauss / Hann). The
  rectangular window intentionally **clicks** — a teaching artifact, not a bug.
- 4-point **Lagrange** interpolated read + overlap-add; **key-tracked resample**
  (root C3 / note 60) combined multiplicatively with Grain Pitch (±24 st) and per-grain
  Pitch Spray (0–12 st); per-grain **rate-tracking one-pole** anti-aliasing.
- **Read head**: Position (0–100%), Scan / time-stretch (±200%, reverse), and **Freeze**
  — pins the read head on one instant and sustains indefinitely, with a smoothed,
  click-free crossfade on toggle (zipper-free).
- **Spray & scatter**: Position Spray, Pan Spray (equal-power), Pitch Spray, and
  **Scatter** (0–100%) — the sync→async axis that dissolves the discrete grain-rate comb
  into broadband noise. Velocity → Density (opt-in) and velocity → amp (always-on).
- Per-voice amplitude **ADSR**; 20 ms-smoothed **Output Level** trim with overlap-aware
  headroom. `setLatencySamples(0)`.
- **Sources**: 4 embedded built-ins (fire / voice / water / piano) via
  `juce_add_binary_data`, hot-swapped on an atomic source pointer. **Load-your-own**
  short source (≤10 s) by macOS WebView content-streaming drag-drop
  (`juce::Base64::convertFromBase64`) or file-picker fallback; loaded-source identity
  persisted as custom (non-APVTS) ValueTree state.

### Interface (WebView)
- Single-page Ouaricon-Naturalist UI; all **18 parameters** two-way bound (relative-drag
  knobs, wheel, keyboard arrow-keys; two combo boxes + Freeze toggle).
- **Four live visualizations**, pushed at 30 Hz off lock-free audio-thread taps
  (`TripleBuffer` grain events + `VizRing` samples + atomic count; FFT on the message
  thread, never the audio thread):
  - **Grain cloud** — scatter accumulates as density thickens and spray widens it.
  - **Source waveform** — live per-grain read playheads, the ❄ freeze pin, and a shaded
    spray band.
  - **Oscilloscope** + **spectrum** — discrete sidebands at scatter=0 smearing to noise.
- **Window-envelope inset** that redraws on Window-shape change, and a live
  **grain / overlap / CPU** readout counting `N / 192`.
- Plain-language **tooltips** on every control, reachable by mouse *and* keyboard focus.
- **Source loading** by drag-drop or Load… picker, both granulating a user file.

### Presets
- 8 factory **concept presets**, each isolating one granular idea — knobs, combos, and
  toggle snap together with a caption and active-state, written through the APVTS:
  **Single Grain · Pitched Buzz · Fragments · Smooth Cloud · Frozen Pad ·
  Asynchronous Cloud · Granular Fire · Rect Click**.

### Validation
- `auval -v aumu OsGr OuDv` → **AU VALIDATION SUCCEEDED** (AU).
- `pluginval --skip-gui-tests --strictness-level 10` → **SUCCESS** on the installed VST3
  (parameter automation, thread-safety, state, bus layouts, fuzz). GUI-open suite folds
  into the DAW listen.
- Offline render harness **8/8** — makes-sound, density→continuity, pitch-tracks-MIDI
  (C2/C3/C4 exact), window-rect-clicks, freeze-sustains, scatter sync→async,
  stress-bounded (≤192 grains), up-transposition stable.
- 8 factory presets desk-checked against `parameter-spec.md` — all write in-range,
  finite, denormal-free APVTS state.
- Live viz animation, audible preset character, drag-drop, and host-automation→UI
  round-trip are confirmed by a DAW listen (handed over at Stage-4 close).

### Internal
- Render-harness CMake now links `O-simpleGrain_UIResources` (NAMESPACE `UIBinaryData`),
  which the Stage-3 `PluginEditor.cpp` `getResource()` requires — fixes a test-harness
  link regression surfaced when the offline harness was re-run against the Stage-3 editor.
  No product DSP, parameter, or UI change.

### Platforms
- macOS: VST3 + AU + Standalone. Windows VST3 cross-platform flags in place
  (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` +
  `withUserDataFolder`), statically verified; Windows build **deferred to publish/CI**.
