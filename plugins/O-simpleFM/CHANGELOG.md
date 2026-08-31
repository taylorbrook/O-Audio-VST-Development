# Changelog — O-simpleFM

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.3.1] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout — a second reading of
every French entry against the suite glossary (`scripts/i18n-fr-glossary.js`) and
the French lint (`scripts/i18n-fr-lint.js`). **No native speaker has read it yet:
all 80 entries stay `reviewed: false`, which is what that flag means.**

### Changed
- **41 of 107 French strings revised** against the suite glossary and lint,
  which went **63 findings → 3**. By dominant change: 10 terminology, 6 meaning,
  5 grammar/register, 20 typography.
  - *Terminology.* The ADSR family took the settled French: **Chute → Déclin**
    and **Relâche → Relâchement** on the shared envelope captions, and the four
    amplitude tip titles spell the parameter out (*Attaque d'amplitude*,
    *Déclin d'amplitude*, *Maintien d'amplitude*, *Relâchement d'amplitude*).
    *Chemin du signal* → **Chaîne du signal** on the routing caption and its tip.
  - *Typography.* Straight apostrophes → **typographic ’** throughout: this
    plugin's French was drafted with `'` where thirty-two others already used
    `’`, and all 26 converted. No-break spaces before `: ; ? %`, between a
    number and its unit (*0,45 s*, *3 s*, *60 %*), and inside the guillemets of
    the delete confirmation.
  - *Meaning.* "Pitched" was rendered *accordé* (in tune) in four bodies and is
    now **à hauteur définie** — an inharmonic bell is not out of tune, it has no
    definite pitch, and that distinction is the lesson. The Tubular Bell tip
    named the control by its function (*arrondi désactivé*) rather than by the
    caption on the page, and now says **rapport entier désactivé**. The Clang
    tip said the sidebands themselves were *non entières*; they sit at
    non-integer multiples.
  - *Register.* One instruction style on the page: the settings tip is now
    imperative (*Choisissez la langue…*) like the tour caption and the keyboard
    hint. *cliquez les touches* → **cliquez sur les touches**.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Kept, on a measurement
- **The carrier-null badge stays "porteuse nulle"** rather than taking the
  glossary's *extinction de la porteuse*. The settled term measures **160.7 px**
  against the **102 px** the badge is pinned to, and the badge is the last inline
  box on a right-aligned readout line — it would drag both live numbers and the
  readout column 58.7 px left (x 616 → 557.3). The glossary lists no
  abbreviation for the term; the reading is reported so it can grow one. The
  badge's own tooltip title does carry the settled term.
- **"Fréq. fixe" stays**, with a `termNote`: this control is an absolute
  frequency in Hz, which is the one case the glossary's own gloss allows.
- **"Leçons" stays** — the glossary's short form; the root wraps to two lines
  inside the 99 px `.tour-label` pin.

### Verified
- `i18n-fr-lint` 63 → 3, and the three that remain are structural: two are a
  contradiction between the glossary's TERMS table and its FORBIDDEN list, one
  is the badge width above. Reported, not worked around.
- `check-ui-labels` all assertions pass, with **0 non-label elements moved**
  between English and French at the 760 × 980 frame — unchanged from before the
  pass. Negative control: a caption 51 px wider than the shipped *Relâchement*
  fails assertions [8] and [8b] in both envelope racks.
- 27 tooltips hovered with a real pointer in both languages, 336 assertions,
  every tip opening inside the frame with its right-edge flip exercised.
- `check-i18n` ALL CHECKS PASS; `boot-all-uis` 43/43 clean.

## [1.3.0] — 2026-08-28

**The PAGE speaks French, not only the hover help.** (Stage I batch I2, canon v2 —
fifth of the seven `O-simple*` plugins.)

Every caption, heading, button face, hint, diagram node, status line and tooltip
switches with a language selector in a new header gear. Value readouts, the
factory preset names and the five lesson-button faces stay English per D-03/D-02.
The language rides the APVTS tree as a plain `uiLanguage` property (the string
`"en"`/`"fr"`, restored behind an `isVoid()` gate). **80 French entries — 27
tooltip, 53 label — all `reviewed: false`.** No native speaker has read them.

### Added
- `Source/ui/public/js/i18n.js` — the interface copy table in both languages:
  `LANGUAGES`, `I18N` (27 tooltip pairs), `LABELS` (53), `TIP_BINDINGS` (27),
  `I18N_EXEMPT` (10 reason-bearing entries), `tr`. Embedded through
  `juce_add_binary_data` and served from `getResource()` at `/js/i18n.js` — both
  halves in this commit, because either one alone is a blank page rather than an
  English one.
- Canon v2 in `js/app.js`, verbatim from `scripts/i18n-canon.js`, directly under
  the imports and above every other declaration (`uiLanguage` is a module-level
  `let`, and this file has eager top-level work below it).
- A settings gear in the header holding the language selector alone. This plugin
  has no hover-help bridge and never had a "?" toggle — not a C++ one, not a
  localStorage one — so a toggle row here would be a control for a preference
  that does not exist. Styled in this plugin's own vocabulary: the `.group`
  plate, the `.toggle`'s green border and lit state, the preset-bar button's
  paper fill and radius at panel scale.
- `getUiLanguage` / `setUiLanguage` native functions and a `std::atomic<int>
  uiLanguage` on the processor, persisted as a ROOT property on the APVTS tree.
  Not an `AudioParameterChoice`: it must not reach a DAW automation lane, and a
  lesson preset must not be able to change which language somebody reads their
  interface in. `getStateAsXml()` starts from `parameters.copyState()`, so the
  property is set BEFORE that call; `setStateFromXml()` ends in
  `replaceState()`, so it is read AFTER.
- `tests/i18n-states.json` — three states the resting page cannot reach: the
  gear popover, a lesson caption, and the carrier-null badge.
- **Accessible names on the fifteen knobs.** `js/app.js` gives each knob
  `role="slider"` and `tabindex`, and until now none of them had a name at all.
  `data-i18n-aria` naming the parameter resolves to the control's own tooltip
  title and fixes it in both languages for free. 29 keyed attributes, 29 of 29
  confirmed switching by the gate. **Pre-existing bug, fixed here.**

### Changed
- **Tooltip copy moved out of `app.js`'s `TIPS` object into `js/i18n.js`.** The
  renderer reads the anchor's own `data-tip-title` / `data-tip`, which
  `applyI18n` rewrites per language, and its listeners are DELEGATED on the
  document — no anchor carries `data-tip` until the first sweep runs, so the old
  `querySelectorAll("[data-tip]")` at setup time would have bound nothing at all.
- **The 24 markup anchors moved off `data-tip`**, which now carries the tip BODY:
  the fifteen knob cells onto a new `data-param`, the two toggles and the routing
  panel / readout / badge onto an id, the five lesson buttons onto the
  `data-preset` they already had.
- **`js/app.js:222` read `.knob-cell[data-tip="modFixedHz"]`** to dim the Fixed Hz
  cell when Fixed Mode is off. That selector matches nothing once the anchor
  moves — silently, with no error. Re-pointed at `[data-param="modFixedHz"]` and
  verified end to end by driving the toggle in a headless page: opacity goes
  0.4 → 1 → 0.4. Negative control: with the old selector restored, the opacity is
  never set at all and no error is raised.
- 101 English strings were compared back to v1.2.5 byte-for-byte with entities
  decoded rather than re-typed — 98 verbatim, 3 legitimately new (`carrierNull`'s
  tip TITLE, and the gear's two `aria.*` names).
- **Three deliberate English changes**, each recorded because it is a copy edit
  and not a translation:
  1. The tip bodies lost their `strong`/`em` tags. The WORDS are unchanged; the
     renderer writes `textContent` now, so a tag would render as literal
     characters (and `check-i18n` assertion 9 forbids an angle bracket in an
     `i18n.js` string literal).
  2. The routing meta line was `harmonic · ≈ 1 sideband` / `≈ 4 sidebands`, an
     inline English plural. Contract §6 forbids engineering plural inflection for
     one plugin, so the count moved to the end of a count-neutral phrase:
     `harmonic · sidebands ≈ 4`. It is written through two literal-keyed
     `setLabel` writers behind an `if`/`else`, never a ternary in the argument.
  3. The carrier-null badge's explanation was a native `title=`. Contract §4
     deletes native titles; this page already owns a tooltip renderer, so the
     copy moved into it verbatim rather than becoming an `aria-label` nobody
     would hear. The badge is now the 25th tip anchor.
- **All six native `title=` attributes deleted.** Two (`presetPrev`,
  `presetNext`) duplicated an `aria-label` that was already there and simply
  went. Three (`presetName`, `presetSave`, `presetDelete`) were the only help
  those buttons had and moved to `data-i18n-aria`. The sixth is the carrier-null
  badge above.
- The five lesson captions, the two preset-dropdown group headings
  (`Factory`/`User`) and the three strings in the delete-confirmation dialog were
  raw `textContent` writes of finished English. Each call site names its own
  literal key now. The confirmation takes the preset NAME rather than the
  sentence the vendored preset-manager module composes, so the sentence is
  localized here without editing a shared module — and the name itself is
  substituted verbatim, because a preset name is its JSON filename (D-02).
- **The five lesson-button faces stay English, and this is a deliberate
  departure from O-simpleGrain, which translated its chips.** Here the face IS
  the factory preset name, and this plugin — unlike O-simpleGrain — DISPLAYS
  that name in its header preset bar. A French face over an English bar entry
  would be exactly the page-versus-preset disagreement D-02 exists to prevent.
  They are `I18N_EXEMPT` with that reason; their tooltips are translated.

### Fixed — French label geometry (D-04)
Eight CSS rules, each measured, each reverted ALONE to confirm the gate
re-breaks. Eight rules, eight negative controls, none decoration.

| Rule | What it holds |
|---|---|
| `.preset-act { min-width: 64px }` | "SAVE" 49.2 / "ENREG." 61.4 / "DELETE" 63.4 / "SUPPR." 58.7. The two buttons net +7.5px in French, which moved the whole preset bar, the gear, and the title block that absorbs their shrink. 10 elements moved. Cost lands in English: SAVE now sits in a 64px box. |
| `.routing-label { flex: 0 0 77px }` (was `min-width: 70px`) | **`min-width` on the FIRST item of a flex row is not a pin** — the used width is still max-content, 76.1px for "Signal Path" and 114.8px for "Chemin du signal", and as the first item its width ALONE positioned the routing SVG. 38.7px of drift. The French wraps to two 11px lines inside the pin, which the 64px-tall SVG beside it already absorbs. |
| `.routing-readout-col { min-width: 156px }` | Both children are `nowrap`, so the column shrink-wrapped to the longer of them: 116.9px English, 155.4px French. Right-aligned, so nothing visibly collided — but the BOX moved, and the box is what the diff measures. The panel had 229px of slack. |
| `.knob-label { white-space: nowrap }` | A `.knob-label` in a `column` + `align-items: center` cell SHRINK-WRAPS: its box is text-width, not the cell's 62px, so a long caption already overflows symmetrically into the 24px row gap. What is not free is WRAPPING — "Rapport P:M" and "Indice mod." broke at their space, made their cell 10.4px taller and pushed the readout below them down a line, in French only. 103 elements moved. nowrap costs ZERO English page height (no English caption wraps) and makes the cell height language-independent by construction. Worst French overflow is "Véloc→Indice" at 80.6px in a 62px cell — 9.3px each side into a 24px gap, widest adjacent pair still clearing by 12px. |
| `#toggle-ratioSnap { min-width: 130px }` `#toggle-modFixedMode { min-width: 106px }` | "Ratio Snap" 101.5 / "Rapport entier" 129.5; "Fixed Mode" 105.4 / "Mode fixe" 97.4. Pinned per button rather than both to one width — one shared min-width would have grown the English Fixed Mode by 24px for nothing. |
| `.tour-label { flex: 0 0 99px }` | "Lesson Presets" 98.8 / "Leçons" 46.1, and as the first item of the tour row its width alone positioned all five lesson buttons. 52.7px of drift. Costs English nothing. |
| `.tour-caption { flex: 0 0 100% }` (was `max-width: 48%`) | At 48% of the row (325px) the caption was ONE line in English and TWO in French, pushing the whole keyboard panel down 12px. It already fell onto its own flex line in both languages, so the full row costs English nothing (the text is right-aligned, the glyphs do not move) and makes the line count deterministic: the longest of the eleven captions across both languages measures ~507px inside 678px. Basis 100% also means it can never rejoin the button row on a copy edit. |
| `.carrier-null-badge { min-width: 102px; text-align: center }` | "carrier null" 88.5 / "porteuse nulle" 101.1. The badge is the LAST inline box on a right-aligned readout line, so its width alone moved both live numbers and the whole readout column 12.7px whenever it lit. Cost: a 13.5px wider pill in English, with the word centred inside it. |

**One French string was shortened rather than fitted, and is flagged for the
reviewer:** `label.vizWaveformHint`. The faithful "la forme de la porteuse qui en
résulte" measures 226.1px against the ~165px this hint has beside its caption in
a 240px viz block, so it wrapped and shrank the scope canvas 11px in French only.
It ships as "la porteuse qui en résulte" — "shape" is dropped, and the caption
immediately to its left already reads "Forme d'onde". The alternative, reserving
the second line in BOTH languages, costs 11px of English page height on a frame
that already scrolls.

**One attempted fix was DECORATION and was removed rather than kept and claimed.**
The first reading of the diff was that `.title-block` shrink-wrapped its own
strapline — 361.7px English against 354.2px French — and a `flex: 1 1 0` pin was
written for it. Its negative control PASSED: reverting the pin alone left the
gate green. The 7.5px was never the strapline; it was the preset bar one item to
the right, and `.title-block` is the only item here that can absorb shrink, so it
reported the difference its neighbour caused.

**Frame cost: ZERO.** `.frame` scroll extent is 1027px inside a 974px client area
at v1.2.5, 1027px at v1.3.0 in English, and 1027px at v1.3.0 in French. The page
scrolls by 53px in every one of those three cases — it did before this work and
it does after, identically in both languages. The 760 × 980 frame is untouched;
it is the narrowest in the batch and nothing here moved it.

### Testing
- `check-i18n --plugin O-simpleFM`: exit 0, canon **v2**, all 15 assertions.
- `check-i18n --strict-v2`: exit 0 — **12 canon v2, 0 canon v1**.
- `check-ui-labels --plugin O-simpleFM`: **ALL CHECKS PASSED** at 760 × 980 across
  four states, with **ZERO** non-label elements moved between English and French,
  vacuity 37/38 labels + 29/29 attributes actually changing language,
  `dataset.label === textContent` after init, after the switch and after a state
  pass in both languages, **38 of 38 labels measured** (no coverage hole), no
  page error, every resource served.
- `boot-all-uis`: clean — 38 i18n, 0 native title. (`O-Bowed` and `O-Reed` still
  fail this repo-wide with `Unexpected token 'export'`; pre-existing, untouched.)
- Render harness: **ALL PASS, 8/8**, and every printed number is digit-identical
  to the pre-change C++. Unlike O-simpleGrain's, **this harness is
  deterministic** — that was established first, by running the same binary twice
  and getting the same digits, before any cross-version comparison was made.
- `auval -a` lists `aumu OSiF OuDv`.
- The re-pointed `modFixedHz` selector was verified by driving the toggle in a
  headless page, with a negative control against the old selector.

### Not verified
- The C++ persistence round-trip has never been executed by hand on this plugin:
  that a language choice survives quitting and reopening a DAW session is a
  strong inference from the code, not a measurement.
- Windows / WebView2 font metrics remain a named deferral, blocked on hardware.
  A French label measured as fitting on macOS could clip under WebView2.
- The French is machine-drafted. All 80 entries are `reviewed: false`.

## [1.2.5] — 2026-08-25

### Fixed
- **Clicks on note-off, at any settings.** Root cause: `pushParamsToVoices()`
  pushed ADSR parameters into the live `juce::ADSR` envelopes every block via
  `setParameters()`, whose `recalculateRates()` recomputes the release slope
  from the SUSTAIN level — clobbering the envelope-value-based rate that
  `noteOff()` had just computed. With amp sustain = 0 (four factory presets),
  the recomputed rate is 0, and `recalculateRates()` treats a zero-rate release
  as finished: it hard-resets the envelope one block after every note-off,
  truncating the ringing tail to silence instantly — the click. (JUCE's ADSR
  docs explicitly forbid changing parameters during playback.)
  Fix in `FMVoice`: envelope params are cached each block but only pushed to
  the live envelopes when their values actually change AND the voice is not in
  its release phase; changes made mid-release apply at the next note-on. The
  release therefore always completes at the rate captured at note-off.
- Render-harness: new `noteoff-click` probe (sustain 0, slow decay, note-off
  mid-decay) asserting the release tail rings on after note-off and the
  post-release waveform has no sample-to-sample discontinuity. Verified to
  FAIL against the v1.2.4 voice code (negative control) and pass with the fix.

### Testing
- Render harness: all 8 checks pass (7 existing + new noteoff-click probe).
- Negative control: probe re-run against v1.2.4 `FMVoice.h` fails as expected.

## [1.2.4] — 2026-08-08

First public release on the O-Audio-VST-Development repo, built and signed
through the cross-platform CI pipeline. No DSP, parameter, or state-format
changes.

### Changed
- Relicensed under AGPL-3.0: license notice headers added to all
  Ouaricon-authored source files.

## [1.2.3] — 2026-07-15

Resolves the four Info findings from the 2026-07-15 full-plugin code review
(`CODE_REVIEW.md`, IN-01..IN-04). Hardening + suite-standard UX affordance —
no DSP behavior, parameter, or state-format changes.

### Fixed
- **IN-01 — Index taper/range constants deduplicated.** The perceptual taper
  `baseIndex = 20·(I/20)^1.7` was independently hardcoded in `FMVoice::setParams`,
  `pushParamsToVoices` (the `/20` re-normalization), the modIndex parameter range,
  the render-harness carrier-null test, and the JS carrier-null badge — a future
  range/taper change would silently desynchronize them. Root cause: no single
  source of truth. Now `OSimpleFM::kIndexMax` / `kIndexTaper` in `FMVoice.h` feed
  all four C++ sites; the JS mirrors them as `INDEX_MAX` / `INDEX_TAPER` with
  cross-referencing comments on both sides (a WebView page cannot include a C++
  header).
- **IN-02 — `handleUiMidi` now range-checks the note number.** The WebView-supplied
  note went straight into `juce::MidiMessage::noteOn`, which jasserts in debug and
  builds malformed MIDI in release for values outside 0–127. The JS guards its own
  range, but the native boundary must not trust the page:
  `juce::jlimit (0, 127, noteNumber)` (velocity was already clamped).
- **IN-03 — `scaledMidi` headroom raised 4 KB → 16 KB.** A pathological block
  (dense sysex/CC flood plus UI notes) exceeding the 4096-byte pre-allocation
  would make `MidiBuffer::addEvent` reallocate inside `processBlock` — the one
  remaining audio-thread allocation path. `ensureSize (16384)` in `prepareToPlay`.

### Added
- **IN-04 — Double-click a knob to reset it to its default.** Suite standard
  (O-MicrotonalSampler v1.23.7): a new `getParameterDefaults` native fn returns
  `{ paramID: normalisedDefault }` from `RangedAudioParameter::getDefaultValue()`
  (the `propertiesChanged` payload carries the range but no default), and
  `bindKnob` runs the full dragStarted → setNormalisedValue → dragEnded gesture
  on `dblclick` so hosts record the automation touch.

## [1.2.2] — 2026-07-15

Resolves the Critical and all six Warning findings from the 2026-07-15 full-plugin
code review (`CODE_REVIEW.md`, CR-01 + WR-01..WR-06). Bug fixes only — no DSP,
parameter, or state-format changes.

### Fixed
- **CR-01 — Use-after-free in the preset file dialogs.** Both
  `FileChooser::launchAsync` completions (`savePresetWithDialog`,
  `loadPresetFromFile`) captured raw `this` and the WebView-owned `complete`
  callback; closing the plugin window while the native dialog was open could fire
  the completion against a destroyed editor. Root cause: async completion outliving
  the editor (the W12 pattern from O-MicrotonalSampler v1.23.5). Now captures a
  `Component::SafePointer` and bails with a bare return when the editor is gone —
  deliberately NOT calling `complete(false)`, which is owned by the dead WebView
  and would itself be a UAF.
- **WR-01 — Stuck notes on the on-screen keyboard.** Releasing the mouse outside
  the plugin window never delivered `pointerup` (no implicit capture for mouse
  pointers), and a held QWERTY note's `keyup` was lost when the WebView lost focus
  mid-hold. Root cause: release events not guaranteed to reach the listener. Now
  uses `setPointerCapture` on pointerdown (guaranteeing `pointerup`/`pointercancel`),
  glide tracks via `elementFromPoint` (captured events retarget to the container),
  and a window-`blur` sweep releases all held notes.
- **WR-02 — QWERTY notes fired while focus was on UI controls.** The global
  `keydown` note handler ignored the event target, so letter keys pressed while
  navigating the preset bar/dropdown (or any future text input) played notes and
  swallowed the keystroke. Now bails when focus sits on
  `input, textarea, [contenteditable], .preset-bar, .preset-dropdown`.
- **WR-03 — Preset Delete had no confirmation.** The header Delete button removed
  the current user preset file immediately and irreversibly. Now routes through
  the module's `promptDelete()` with an in-DOM confirm dialog supplied via
  `onConfirmDelete` (`window.confirm` is unreliable in JUCE WebViews). Escape,
  backdrop click, and Cancel all abort; Cancel takes initial focus.
- **WR-04 — Save dialog silently ignored the chosen folder.** The save handler
  extracted only the filename and always wrote to `Presets/User/`; saving to e.g.
  Desktop produced no file there, with no indication. Now saves through
  `OuariconPresetManager::savePresetToFile()`, which honors the caller-chosen
  directory (symmetric with `loadPresetFromFile`).
- **WR-05 — Render harness rewrote the user's REAL factory presets with a stale
  version.** The harness hardcoded `JucePlugin_VersionString="1.0.0"` while the
  plugin was at 1.2.1; the processor ctor runs `initializeFactoryPresets()`, so
  every harness run rewrote `~/Library/O-simpleFM/Presets/Factory/` stamped
  1.0.0 and flipped the `.factory-version` sentinel — defeating it permanently on
  any machine that ran the test. The version now lives in one
  `set(OSIMPLEFM_VERSION …)` variable in the plugin CMakeLists; both
  `juce_add_plugin(VERSION …)` and the harness macro derive from it.
- **WR-06 — Spectrum axis and sideband markers used a boot-time Nyquist.** The JS
  fetched the sample rate exactly once at boot; opening the editor before the
  first `prepareToPlay`, or a host rate switch (44.1k → 96k) mid-session, left the
  log-frequency axis and fc/sideband markers mapped against the wrong Nyquist
  while the analyzer bars used the right one. The editor's 30 Hz timer now emits
  `sampleRateUpdate` whenever the processor's rate changes; the page re-maps the
  axis and redraws.

### Validation
- Built VST3 + AU, installed with cache clear, `auval` pass. Render harness
  rebuilt and re-run (now stamps the correct version). No parameter or
  state-format changes — presets and sessions from 1.2.x load unchanged.

## [1.2.1] — 2026-06-21

A small teaching-copy addition: hover explanations on the Lesson Preset buttons.
No DSP, parameter, or mechanism changes.

### Added
- **Hover tooltips on the five Lesson Preset buttons** (E-Piano, Tubular Bell, Brass,
  Clarinet, Clang Bell). Each explains *how that voice is built* in FM terms — the
  carrier:modulator ratio (harmonic vs inharmonic), the modulation index and how the
  envelope drives it, feedback, and the resulting spectrum — so the buttons teach the
  synthesis rather than just loading a sound. Copy mirrors the actual values in
  `FactoryPresets.cpp` (e.g. E-Piano 1:1 with a fast index sweep; Tubular Bell's
  inharmonic 1.41; Clang Bell's index-14 + 60% feedback smear).
- **Implementation:** reuses the existing `setupTooltips()` engine — each `.tour-btn`
  gained a `data-tip="lesson…"` attribute and a matching entry in the `TIPS` table in
  `app.js`. No new tooltip mechanism, so the buttons inherit the same pointer-hover,
  keyboard-focus, and Escape-to-dismiss behaviour as every other annotated control.

### Validation
- WebView-only change (HTML `data-tip` + JS `TIPS` copy); no C++, DSP, or parameter
  surface touched. Verified the build and `auval`/`pluginval` still pass (see build log).

## [1.2.0] — 2026-06-21

Two teaching visuals that make the FM math legible on the live spectrum. No DSP
or parameter changes — viz-only overlays driven from the message thread.

### Added
- **Sideband markers on the spectrum.** The spectrum now overlays the predicted FM
  component frequencies — the carrier f_c (solid amber, labelled) and the sidebands
  f_c ± k·f_m for k = 1..8 (faint dashed sage) — so peaks visibly land where
  Chowning's math says. f_m mirrors the engine exactly: the fixed Hz in Fixed Mode,
  otherwise f_c·ratio with integer snap applied. Markers are mapped onto the existing
  log-frequency axis (20 Hz → Nyquist) and only drawn while a note is sounding;
  anything below 20 Hz or above Nyquist is skipped.
  - **New plumbing:** the carrier frequency reaches JS via a `carrierUpdate` event
    emitted from the editor's 30 Hz timer, *just before* `spectrumUpdate`, so the
    markers stay in sync with the frame they annotate. The processor tracks the
    most-recently-started note's pitch (from the merged host + on-screen-keyboard MIDI
    stream) and whether any voice is still audible (`getCarrierHz()` returns 0 when
    silent). All marker math runs on the message thread — the audio thread only does
    two relaxed atomic stores, preserving the PERF-01 real-time-safety model.
- **Carrier-null indicator.** A green "carrier null" badge lights next to the Signal
  Path I readout when the modulation index reaches the first Bessel J₀ zero. It is
  gated on the *effective* radian index β = 20·(I/20)^1.7 (the perceptual taper the
  DSP applies), matching the render-harness `carrier-null@2.405` test — so the badge
  fires exactly when the carrier marker sits on a nulled peak (≈ readout I 5.75), not
  at a literal readout value of 2.405 (which would contradict the spectrum).

### Validation
- Render harness 7/7 (incl. `carrier-null@2.405`), `pluginval --strictness-level 10`
  (VST3), and `auval` (AU) re-run after the changes (see build log).

## [1.1.0] — 2026-06-21

A teaching-and-cleanup release: an on-screen keyboard so the plugin makes sound
without external MIDI, two new live readouts, and a round of code de-duplication.

### Added
- **On-screen keyboard** (C3–C5). Click/glide with the mouse or play the computer
  keyboard (A S D F G H J K naturals, W E T Y U sharps → C4–C5). Notes are queued
  through a `juce::MidiMessageCollector` and merged into `processBlock`'s MIDI
  stream, so the Standalone build (and any host) makes sound — and the live
  spectrum/scope respond — without an external MIDI source. Keys light up on play.
- **Spectrum frequency-axis labels** (100 Hz / 1k / 10k) drawn on the log-frequency
  axis, so peaks read against actual pitch. Sample rate is pulled from C++ via a new
  `getSampleRate` native function.
- **Harmonic / inharmonic + sideband-count readout** under the Signal Path numbers:
  flags whether the current C:M ratio is harmonic, and shows Carson's rule estimate
  of significant sidebands (≈ I + 1).

### Changed
- **Lesson Presets now load the factory presets by name** instead of carrying a second
  hand-maintained copy of the five sounds in JS. `FactoryPresets.cpp` is the single
  source of truth; the JS only holds the teaching captions. Picking a lesson now also
  updates the preset-bar name. (Removes a silent-divergence hazard between the tour and
  the same-named factory preset.)

### Fixed
- **Index-ceiling code path was dead and self-contradicting.** `FMVoice::renderNextBlock`
  computed the Carson anti-alias index ceiling once per block (per the comment) but then
  re-armed the smoother's target *every sample*, overwriting it — so the documented
  per-block optimization never took effect. Consolidated into a single
  `computeIndexCeiling()` helper armed once per block, and snapped via
  `setCurrentAndTargetValue` on note-on so the ceiling is enforced from sample 0 (it
  previously inited to 1e9 and only ramped down over ~10 ms, briefly under-clamping the
  first note / large pitch jumps).

### Internal
- De-duplicated the knob wheel/arrow-key fine-adjust into a shared `nudge()` helper and
  consolidated two `resize` listeners into one (resize backing store + redraw). Removed a
  dead empty `dblclick` handler.

### Validation
- Render harness 7/7, `pluginval --strictness-level 10` (VST3), and `auval` (AU) re-run
  after the changes (see build log).

## [1.0.2] — 2026-06-20

### Added
- **Explanatory tooltip for the Signal Path readout** (`1.00 : 1 · I = 0.0`). The
  numbers at the right of the routing row now have their own hover/focus tooltip
  explaining that the left value is the **C : M ratio** (modulator frequency vs. the
  played note → which harmonics appear) and the right value is **I**, the modulation
  index (→ brightness / number of sidebands). Previously the readout inherited the
  generic "Signal Path" tip, which didn't explain the numbers. Added a `cursor:help`
  affordance to signal it's hoverable.

### Fixed
- `focusin` handlers now `stopPropagation`, so a nested `[data-tip]` element (the new
  readout inside the routing panel) isn't overridden by its ancestor's tooltip on the
  event bubble — keyboard focus shows the correct tip.

## [1.0.1] — 2026-06-20

### Fixed
- **UI overlap / clipping.** At the 760×720 editor size the field-guide content
  (~880px tall) exceeded the window. `.frame` was a `height:100%` flex column and
  `.controls` had `flex:1; min-height:0`, so the controls block shrank *below* its
  intrinsic height; its children (Output knob) overflowed the collapsed box and
  collided with the **Lesson Presets** row that follows it. With `html,body`
  clipping (`overflow:hidden`), nothing could scroll.

### Changed
- `.frame` is now the scroll container (`overflow-y:auto`); the decorative border
  stays fixed framing the window while content scrolls. All direct sections are
  `flex-shrink:0` so they keep their natural height instead of squishing together.
- Added an aged-paper styled scrollbar (`::-webkit-scrollbar`).
- Editor window grown 760×720 → **760×860** so the full layout seats without
  scrolling on a normal desktop; the frame scroll handles shorter screens / hosts
  that clamp window height.

## [1.0.0] — 2026-06-20

First release. A pedagogical 2-operator FM / phase-modulation synthesizer with a
field-guide "Naturalist" WebView UI, built to make *"oh, THAT's how FM works"* land
in about five minutes.

### Synth engine (DSP)
- 16-voice polyphonic 2-operator **phase-modulation** voice (radians convention,
  1:1 with Chowning/Bessel math). MIDI instrument, audio out.
- Modulation Index `I` 0–20 with a perceptual `I = 20·norm^1.7` taper; carrier null
  reachable at I ≈ 2.405 (Bessel J₀ zero) as a teaching marker.
- C:M **Ratio** with optional integer **Snap** (harmonic ⇄ inharmonic), plus a
  **Fixed-Hz** modulator mode for key-independent, formant-like colour.
- DX7-style modulator **self-feedback** (two-sample average / Tomisawa anti-hunting,
  history clamp + NaN scrub + note-on reset) — sine → saw → noise, stable at 100%.
- Independent **mod** and **amp** ADSR envelopes; mod-env → index (depth default 1.0),
  optional velocity → index; amp envelope governs voice lifetime.
- Anti-aliasing: sine LUT + key-tracked **Carson index ceiling** + 2× polyphase-IIR
  oversampling, always on (v1.0 sine-only chain).

### Interface (WebView)
- Single-page Ouaricon-Naturalist UI; all 17 parameters two-way bound (relative-drag
  knobs, wheel, and **keyboard** arrow-key control).
- Live **spectrum** (4096-pt FFT, Blackman-Harris) + **oscilloscope**, pushed at 30 Hz
  off a lock-free audio-thread tap (no audio-thread FFT/alloc).
- Live operator **routing diagram** (MOD → CAR + feedback loop, thickness ← index/feedback).
- Plain-language **tooltips** on every parameter, reachable by mouse *and* keyboard focus.
- **Lesson Presets** tour (E-Piano, Tubular Bell, Brass, Clarinet, Clang Bell) — each
  isolates one FM concept.

### Presets
- Suite-canonical **preset manager**: factory + user presets persisted as JSON under
  `~/Library/O-simpleFM/Presets/`, surviving DAW session reloads.
- In-UI **preset browser**: factory/user list, prev/next navigation, save (native
  dialog), and delete (factory presets protected — Delete disables on them).
- 6 factory presets shipped (Default + the five lessons).

### Validation
- `auval` SUCCEEDED (AU); `pluginval --strictness-level 10` SUCCESS (VST3).
- Offline render harness: 7/7 — makes-sound, pitch, index→sidebands, carrier-null@2.405,
  feedback-stable, plus high-pitch and fixed-Hz **aliasing-budget** audits.

### Platforms
- macOS: VST3 + AU + Standalone. Windows VST3 cross-platform flags in place
  (WebView2 static-link + user-data-folder); Windows build not produced this cycle.
