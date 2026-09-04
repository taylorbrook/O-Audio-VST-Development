# Changelog — O-simpleSubtractive

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.4.1] — 2026-09-03

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
### Known issue (open, deliberately not guessed)

- **The switch's two faces `ui.on` / `ui.off` still read `Activée` /
  `Désactivée` — feminine SINGULAR.** They were chosen to agree with the noun
  naming the hover-help surface, and that noun is now feminine PLURAL, so the
  agreement no longer holds; they should read `Activées` / `Désactivées`. They
  are NOT changed here. The before→after sheet the developer read was built from
  strings that CONTAIN the old phrase, and these contain no phrase at all — the
  dependency exists only in a source comment beside them — so they were never on
  the sheet, and a string nobody read must not ship at `reviewed: true`. Same in
  O-simpleSubtractive and O-SpectralShaper. Reported rather than invented.


## [1.4.0] — 2026-09-01

### Added
- **The hover-help switch, in the settings gear beside the language selector.**
  Through v1.3.1 the gear panel held the language row alone — its comment said
  a toggle would be a control for a preference that does not exist, because the
  tooltip engine had no enabled state at all. The suite's other settings panels
  (O-simpleGrain, O-simpleSampler) offer the switch, and a panel that looks
  like theirs but lacks it reads as a missing control. Same port as O-simpleFM
  v1.4.0, same commit: a `.settings-toggle` button (`#help-toggle`,
  `aria-pressed`) whose face is written through `setLabel` (`ui.on` /
  `ui.off`, two calls behind an if/else — check-i18n assertion 13);
  `applyTipsEnabled()` / `setupTipsToggle()` in `app.js`; `show()` returns
  early when `tipsEnabled` is false, and `hideTooltip` is published so
  switching off dismisses a tip already showing. Browser-side preference under
  localStorage `ossub.tipsEnabled` (default on) — no C++ state, no bridge, no parameter.
  A `<div class="settings-row">`, not a `<label for>`: a button is labelable,
  and a label wrapping one re-dispatches the click and toggles twice.
- **Copy.** `help-toggle` (tip), `aria.helpToggle`, `ui.on`, `ui.off` — the
  French is O-simpleGrain's, byte-identical to entries already `reviewed: true`
  there, and carried as reviewed here on that basis. The `gear-btn` tip body
  now names both controls in the panel.
- **Styling** stays in this plugin's own vocabulary: the selector's box beside
  it at rest, the gear's `--btn-active` / `--btn-border-active` lit state when
  pressed.
- `tests/i18n-states.json` gains the "hover help switched OFF" state so the
  `ui.off` face is measured by check-ui-labels in both languages.

### Validation
- check-i18n: 15/15 PASS, 0 / 97 entries unreviewed (36 tooltip, 61 label).
- i18n-fr-lint: CLEAN, exit 0.
- boot-all-uis `--strict-tips`: clean, 0 dead / 0 late tip bindings.
- check-ui-labels: ALL CHECKS PASSED; every `[data-i18n]` element visible in at
  least one state.
- Built + installed (VST3 + AU). WebView-only change — no DSP, parameter, or
  state-format change.

## [1.3.1] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **44 of 93 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint: 16 terminology, 39 typography,
  3 grammar. The visible ones are **Relâche → Relâchement** on both envelope
  groups, **Glissando → Portamento** for Glide, **Enveloppe d'ampli →
  Enveloppe d'amplitude**, **Enveloppe de filtre → Enveloppe du filtre**,
  **Sous → Sub** for the sub-oscillator, straight apostrophes replaced by
  typographic ones throughout, and no-break spaces before `% : ; ?` and
  between every number and its unit. Nothing in English changed, and every
  entry still reads `reviewed: false` — no native speaker has read the French.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

## [1.3.0] — 2026-08-27

**The page speaks French, not only the hover help.** Every caption, heading,
button face, hint, routing label and tooltip switches with a language selector
in a new header gear. Value readouts and the four drop-down menus stay in
English. Third of the `O-simple*` family onto canon v2 (Stage I, batch I2).

### Added
- **Interface language, English + French.** A `⚙` gear in the header opens a
  small panel holding the language selector. 93 French entries — 35 tooltip
  pairs and 58 labels — every one flagged `reviewed: false`. No native speaker
  has read them; `node scripts/check-i18n.js` prints the worklist.
- **The gear panel holds the SELECTOR ALONE.** This plugin has no tooltips
  bridge and never had a hover-help toggle — not a C++ one, not a localStorage
  one — so its help layer is always on and a toggle row would be a control for a
  preference that does not exist.
- `Source/ui/public/js/i18n.js` — the copy table for both languages, added to
  the `juce_add_binary_data` SOURCES block and served from a new
  `getResource()` branch in the same commit. A branch missing there is a BLANK
  page, not an English one: `app.js` imports the table, and an import that fails
  to resolve takes the whole module down.
- **`getUiLanguage` / `setUiLanguage` native functions.** Plain
  `withNativeFunction`, no relay: the language is not a parameter and must not
  reach a DAW automation lane, and a lesson preset must not be able to change
  which language somebody reads their interface in. Pulled once by the page at
  init; nothing pushes, and `applyFactoryPreset` sets parameters only.
- **The choice persists with the session.** `uiLanguage` rides the APVTS tree as
  a plain property, written as the STRING `"en"` / `"fr"` and restored behind an
  `isVoid()` gate — the ValueTree→XML round-trip rebuilds every property as a
  string var, so an `isInt()` predicate would never fire
  (`critical_valuetree_xml_roundtrip_loses_type`). A pre-1.3.0 session has no
  property and English stands.
- `tests/i18n-states.json` — two states the label gate cannot reach on its own:
  the gear panel open, and a lesson preset picked (the tour caption is chosen by
  a click and is never the resting string).

### Changed
- **Tooltip copy MOVED out of `js/app.js`'s `TIPS` object into `js/i18n.js`.**
  All 33 entries were extracted mechanically and compared back to v1.2.5
  byte-for-byte with entities decoded, not re-typed. The renderer now reads the
  anchor's own `data-tip-title` / `data-tip`, which `applyI18n` rewrites per
  language — one code path, and no way for a tip to be stranded in the previous
  language after the selector fires.
- **The 33 markup anchors moved off `data-tip`.** `applyI18n` WRITES `data-tip`
  as the tip BODY, so the key and the copy would have fought over one attribute.
  The 20 parameter cells gained `data-param` naming the APVTS parameter they
  drive; the three panels and two envelope canvases gained an id; the eight
  lesson buttons are addressed by the `data-preset` they already carried.
- **The tooltip listeners are DELEGATED on the document.** The old setup-time
  `document.querySelectorAll("[data-tip]")` would now bind nothing at all — no
  anchor carries `data-tip` until the first sweep runs. `pointerover` /
  `pointerout` / `focusin` / `focusout` are used because, unlike
  `pointerenter` / `pointerleave` and `focus` / `blur`, they bubble; a
  `pointerout` whose `relatedTarget` is inside the same anchor is ignored or the
  tip flickers on every child boundary.
- **The tip bodies lost their `<strong>` / `<em>` emphasis tags.** The WORDS are
  unchanged. The renderer builds the tip with `createElement` + `textContent`
  now rather than `innerHTML`, because the copy is table-sourced and localized
  rather than a fixed literal, and localized copy must never reach a markup
  path. `check-i18n` assertion 9 rejects an angle bracket in an `i18n.js` string
  literal for the same reason.
- **The eight lesson captions are table entries, written through `setLabel`.**
  Through v1.2.5 a `LESSONS` table in `app.js` held them and `applyLesson` wrote
  one with a raw `cap.textContent =`. A string written that way is stranded in
  the language it was picked in the instant the selector fires — and it is the
  one string on this page chosen by a click. What is left in `app.js` is a
  dispatch from the C++ preset name to a writer naming its key as a plain string
  literal: a `KEYS[name] || "…"` map reads better and `check-i18n` assertion 13
  rejects it twice over, correctly.
- **Four text nodes were split into their own spans** so `applyLabel`'s
  `textContent` write cannot delete a sibling: the two `.viz-label` captions
  beside their hint spans, the two `.group-title` captions beside their
  `.group-route` suffix, the `Play ·` caption beside the keyboard hint, and the
  `Res` abbreviation between the two routing readouts.
- The keyboard hint's HAIR SPACES (U+200A) are carried as `\u200a` escapes in
  the table. `applyLabel` writes the table string over the authored markup, so a
  plain space there would have silently widened the QWERTY key run in BOTH
  languages — invisible to an English-vs-French geometry diff, and a change to
  the shipped English nobody asked for.

### Fixed
- **16 knobs, 4 combos and 2 focusable canvases had `role="slider"` /
  `tabindex="0"` and no accessible name at all** — pre-existing since the UI
  shipped, and audible to a screen reader as an unnamed control. `data-i18n-aria`
  on each one resolves through `trLabel`'s I18N fallback to the control's own
  tooltip title, which fixes it in both languages for free. 26 keyed attributes
  now, all 26 confirmed changing language by the gate.

### Geometry — four fixes, each measured, each reverted ALONE to confirm the gate re-breaks
D-04 forbids auto-shrink fonts and short-variant fallbacks; every move below is
a widened container, a pinned box, or a reserved line.

| Fix | Measured cause | Effect |
|---|---|---|
| `.title-block { flex: 1 1 auto; min-width: 0 }` | the block shrink-wrapped its widest child, the subtitle: **445.9 → 518.3px** in French | the block and the `<h1>` inside it stopped moving; the gear stays at x=1131 w=22 in both |
| `.routing-label { flex: 0 0 77px }` (was `min-width: 70px`) | the caption's own max-content width, **76.1px** English vs **113.1px** French, is what positions `#routingSvg` — the whole diagram and its **eleven** children moved **37.0px** right | pinned; "CHAÎNE DU SIGNAL" wraps to two lines inside the box, which the row affords at zero cost (`.routing-panel` is 106px in both because `#routingSvg` is a fixed 92px and `align-items:center` re-centres) |
| `.tour-label { flex: 0 0 99px }` + `.tour-buttons { flex: 1 1 auto; min-width: 0 }` | French is **SHORTER** here — 98.8px → 46.1px — so `.tour-buttons` started **52.7px further left** and shrink-wrapped **89.1px wider** around its French captions | both boxes pinned; 99px is the English caption's own measured width, so the English row is unchanged to within 0.2px |
| `.group-filter .knob-label { min-height: 2.2em }` | exactly **one** caption of the twenty wraps: "KEY TRACK" is 59.0px on one line, "SUIVI CLAVIER" is 60.0px in a 60px cell, so that cell grew 10.4px and pushed `#val-keyTrack` down by one line-height | the line is reserved in both languages. SCOPED to the Filter group rather than all twenty cells — the whole-rack version would charge every group for one cell's wrap |

**Frame cost: zero.** The scroll extent inside the 1180 x 820 editor is
**1118px in an 814px client area — identical at v1.2.5 and v1.3.0, and
identical in both languages.** The frame has scrolled since the UI shipped;
nothing here made that worse. Row 1 of `.controls` is 281.9px and the keyboard
bottom is 1100.8px in every combination measured.

**The 1180 x 820 frame is a Locked Decision. Nothing here moves it.**

### Testing
- `check-i18n.js --plugin O-simpleSubtractive`: exit 0, on **canon v2**.
- `check-i18n.js --strict-v2`: exit 0 repo-wide — **10 canon v2, 0 canon v1**.
- `check-ui-labels.js --plugin O-simpleSubtractive`: **ALL CHECKS PASSED**
  across three states (default, gear panel open, lesson preset picked) with
  **ZERO** non-label elements moved between English and French; vacuity
  48/52 labels (92%) and 26/26 attributes confirmed actually switching;
  `dataset.label === textContent` holding after init, after the switch and
  after a state pass in both languages; 53 of 52 `[data-i18n]` elements visible
  in at least one state; no uncaught page error; every resource served.
- Negative controls: each of the five CSS rules above reverted ALONE re-breaks
  assertion 7 with the exact deltas tabled — none of them is decoration.
- `boot-all-uis.js`: clean, 52 i18n elements, 0 native `title=`.
- Render harness: **ALL PASS, 19 probes**, with v1.2.5's numbers unchanged
  (`noteoff-click` preRms 0.1555 / tailRms 0.1058, `curve-vs-measured`
  maxErrDb 0.00, `self-osc-in-tune` ratio 2.016).
- `auval -a` lists `aumu OSiS OuDv`.

### Not verified
- **The C++ persistence round-trip has not been executed by hand on this
  plugin.** That a language choice survives a session save/quit/reopen is
  reasoned from `getStateAsXml()`/`setStateFromXml()` and the `isVoid()` guard,
  not measured.
- **Windows / WebView2.** WebView2 font metrics differ from WebKit's, so a
  French label measured as fitting on macOS could clip on Windows. Carried as a
  named deferral, blocked on hardware — the same one
  `.github/workflows/ci-tests.yml` already records.

## [1.2.5] — 2026-08-25

### Fixed
- **Clicks on note-off, at any settings** (ported from O-simpleFM v1.2.5; found
  by a suite-wide sweep of the per-block ADSR push pattern). Root cause: the
  processor pushed ADSR parameters into the live `juce::ADSR` amp + filter envelopes every
  block via `setParameters()`, whose `recalculateRates()` recomputes the release
  slope from the SUSTAIN level — clobbering the envelope-value-based rate that
  `noteOff()` had just computed. With sustain = 0 the recomputed rate is 0, and
  `recalculateRates()` treats a zero-rate release as finished: it hard-resets the
  envelope one block after every note-off, truncating the ringing tail to
  silence instantly — the click. (JUCE's ADSR docs explicitly forbid changing
  parameters during playback.)
  Fix in `SubVoice.h`: envelope params are cached each block but only pushed to
  the live envelope(s) when their values actually change AND the voice is not in
  its release phase; changes made mid-release apply at the next note-on. The
  release therefore always completes at the rate captured at note-off.
- Render-harness: new `noteoff-click` probe (sustain 0, slow decay, note-off
  mid-decay) asserting the release tail still rings after note-off.

### Testing
- Render harness: ALL PASS including the new probe (preRms 0.1555 / tailRms 0.1058).
- Negative control: probe re-run against v1.2.4 voice code fails as expected
  (preRms 0.1555 / tailRms 0.0000 — the tail is truncated to exact silence one block after note-off).

## [1.2.4] — 2026-08-08

First published release (version aligned with the O-simple pedagogical suite).

### Changed
- Cross-platform release builds (macOS VST3+AU, Windows VST3, Linux VST3) via GitHub Actions
- AGPL-3.0 notice headers added to all Ouaricon-authored sources

## [1.0.0] — 2026-06-25

First release. A teaching subtractive synth for the Ouaricon pedagogical suite:
the canonical **oscillator → filter → VCA** voice with two independent ADSR
envelopes, built around the O-simpleFM / O-simpleAdditive north star — a tight
loop between gesture and *visible* consequence. Lower the cutoff and watch the
upper harmonics fall away under the live filter curve; push resonance to
self-oscillation and hear the filter whistle on its own.

### Synth engine
- **Source:** one waveform-selectable oscillator — Saw / Square / Triangle / Sine
  — band-limited (PolyBLEP saw & square, polyBLAMP triangle, LUT sine) so it never
  buzzes at high notes. Plus a sub-oscillator (square, −1 octave) and a white-noise
  source, each with its own level.
- **Filter:** a zero-delay-feedback state-variable filter — **LP / HP / BP / Notch**
  at **6 / 12 / 24 dB/oct** — with cutoff, resonance, bipolar filter-envelope amount
  (octaves) and cutoff key-tracking. Pushed to max resonance it **self-oscillates**
  into a clean, bounded sine that plays in tune with key-track (soft-knee limiter +
  resonance-dependent make-up). Zero added latency (no oversampling).
- **Envelopes:** two independent `juce::ADSR` envelopes — one sweeps the cutoff, one
  shapes the VCA and voice lifetime — so brightness and loudness move separately.
- **Voicing:** 16-voice **Poly**, plus **Mono** and **Legato** modes with **glide**
  (portamento) — the same voice serves the classic monosynth and the polysynth.

### Live teaching UI (JUCE 8 WebView)
- Headline **filter-response-over-spectrum** visual: the closed-form filter magnitude
  curve overlaid on the live output spectrum on one shared log-frequency axis — you
  watch the curve subtract harmonics in real time (matches the audio filter to 0.00 dB
  by construction).
- Output **scope**, a **dual-ADSR** display with live envelope markers, an
  **oscillator → filter → amplifier** signal-path diagram that highlights the active
  stage, **30 hover tooltips**, and an on-screen QWERTY/click keyboard.

### Concept-preset tour (FUNC-06)
- Eight named factory patches, each isolating ONE idea, loadable from the UI:
  **Saw Sweep**, **Pluck**, **Brass Stab**, **Sweep Pad**, **Acid Bass** (303),
  **Square Bass** (hollow), **Noise Wind** (filtered noise), **Self-Oscillation**
  (the in-tune whistle). Selecting a preset writes the whole APVTS through the host
  API, so every on-screen control and visual snaps to the new patch automatically.
- Playable enough (FUNC-07) to double as a simple subtractive instrument — bass,
  lead, pluck and pad starting points are one click away.

### Platform / packaging
- VST3 + AU (macOS); Windows WebView2 flags set (`NEEDS_WEBVIEW2 TRUE` +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`).
- Full APVTS state persistence (20 parameters round-trip).

### Validation
- **auval: AU VALIDATION SUCCEEDED.**
- **pluginval strictness-level 10: ALL TESTS PASSED (SUCCESS).**
- Stage-2 offline render-harness gate green (band-limiting / self-oscillation-in-tune /
  closed-form-curve-vs-measured-response).
