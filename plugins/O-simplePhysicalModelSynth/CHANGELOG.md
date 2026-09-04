# Changelog — O-simplePhysicalModelSynth

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.2.3] — 2026-09-03

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
- **The stale width comment in `css/styles.css` re-measured, not scaled.**
  From `check-ui-labels --verbose`: `Infobulles` renders **71.13 px** where the
  superseded caption rendered 96.0. It is now 0.82 px NARROWER than English
  `Hover help` (71.95) rather than 24 px wider. The row is 210 px, the switch
  78 px, the gap 12 px — 120 px of clearance. **The deliberate absence of a
  width pin here is unchanged**: a min-width was written for this row once, its
  negative control passed, and it was removed rather than kept and claimed.
- A Stage-N history note records that `aria.helpToggle` originally said
  *les infobulles* before the Stage-N pass replaced it. This release restores
  the plugin's own wording.
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


## [1.2.2] — 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n
rollout. No parameter, preset-format or audio change; the render harness is
byte-for-byte the same on every check.

### Fixed
- **item 53 — String Model dropdown:** the `stringModel` parameter is never
  `load()`ed anywhere in `Source/` — `StringResonator.h:33` is a single-rail
  Karplus-Strong loop and the "dual-rail Waveguide deferred to v1.1" it names was
  never written — so the dropdown moved an automation lane and nothing else, while
  its tooltip promised the Waveguide "arrives in v1.1" on a 1.2.x plugin. The
  control is now HIDDEN (`hidden` on its `.select-cell`, `index.html:232`; new
  `.select-cell[hidden] { display: none }` in `styles.css`). The parameter, its
  ID, choice list, relay and tooltip binding all stay so existing sessions and
  automation load unchanged; the tooltip body (both languages) now says the
  control is reserved. The Waveguide was not implemented: there is no DSP behind
  the deferred `load()`, so wiring it was not "a few lines" — it is a new engine.
- **item 54 — hover-help switch `aria-label`:** "Toggle tooltips" → **"Toggle hover
  help"**, the family's settled name (the tip title already read "Hover help";
  O-simpleGrain shipped the same fix in batch O3). The French already said
  *Activer ou désactiver l’aide au survol* and is unchanged.

### Changed
- Hiding the String Model cell reflows column 2 of the rack: Inharmonicity and
  Mode Bright move up one row. Identical in both languages — `check-ui-labels`
  still reports 0 non-label elements moved between English and French.
- The render-harness `JucePlugin_VersionCode` literal tracks the bump (`0x10202`).

## [1.2.1] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout — the second reading of
the machine-drafted French, against `scripts/i18n-fr-glossary.js` (the settled
French for ~230 recurring terms) and `scripts/i18n-fr-lint.js` (ten mechanical
typography and terminology checks). No English, key, binding, selector, exemption
or CSS change; no parameter or audio change. `reviewed: false` stays `false` on all
77 entries — that flag records a native speaker, and this pass is not one.

### Changed
- **33 French entries revised** of 77, against the suite glossary and lint: 9
  terminology, 21 typography, 1 grammar, 2 meaning/register. The visible ones:
  the DAMPING knob reads **AMORTISSEMENT** rather than the invented "Amortis.",
  RELEASE reads **RELÂCHEMENT** rather than *Relâche* (a theatre closure), the
  EXCITE diagram node reads **EXCITATION** rather than the bare verb *Exciter*
  beside its noun siblings RÉSONATEUR and MATÉRIAU, the two Amp tooltips spell out
  **d'amplitude** rather than the clipped *d'ampli*, *mailloche* becomes **maillet**
  in two bodies (a mailloche beats a bass drum), Fine Tune is measured in **cents**
  rather than *centièmes*, the keyboard hint says **cliquez sur les touches** (one
  clicks *on* something in French), and the hover-help switch has one French name
  instead of two. Typographically the whole file converts: straight apostrophes to
  **U+2019**, and no-break spaces before `:` `;` and between a number and its unit.
- **`<html lang>` now follows the language selector** (canon change, all plugins),
  so assistive technology reads the page in the language it is displayed in.
- The render-harness `JucePlugin_VersionCode` literal tracks the bump (`0x10201`);
  its `JucePlugin_VersionString` already inherited the plugin version.

### Not changed, and why
- **Modèle corde** stays over the glossary root *Modèle de corde*: measured at the
  shipping 1040x860 frame, the root is 92.00px and wraps to two lines in the 92px
  `.select-label` box, adding 10.44px of cell height and moving the row. The short
  form is 80.16px on one line and is the glossary's own second listed rendering.
- Two **English** defects found by reading the French, and left for a separate fix:
  the String Model tooltip promises the Waveguide "arrives in v1.1" while the plugin
  ships 1.2.1 and `stringModel` is never `load()`ed anywhere in `Source/`; and the
  hover-help switch's `aria-label` reads "Toggle tooltips" where its own tip title
  reads "Hover help".

## [1.2.0] — 2026-08-28

**The PAGE speaks French, not only the hover help.** Sixth of the seven `O-simple*`
plugins and the sixth of batch I2 (canon v2). Every caption, column heading, button
face, diagram node, hint, dropdown group heading and tooltip switches with a language
selector in a new header gear. Value readouts, the factory preset names and the three
`AudioParameterChoice` drop-downs stay English per D-03/D-02/D-01. No parameter or
audio change.

### Added
- **Interface language, English + French.** `Source/ui/public/js/i18n.js` — 24 tooltip
  entries (21 MOVED verbatim out of the `TIPS` object in `js/app.js`, 3 new for the
  settings panel), 53 label entries, 8 reasoned `I18N_EXEMPT` exclusions, 24
  `TIP_BINDINGS`. 39 `[data-i18n]` elements and 29 keyed attributes in the markup.
  **77 French strings, every one `reviewed: false` — machine-drafted, no native
  speaker has read it.** `node scripts/check-i18n.js` prints the worklist.
- **Settings gear in the header**, in the slot the "?" chip held. The hover-help
  switch MOVED into it rather than sitting beside it — a plugin should not grow a
  second settings surface, and the two settings that decide what the hover help says
  and whether it says it belong in one place. Its `localStorage` key
  (`opms.tipsEnabled`) is unchanged, so a preference set before this version survives
  the move. Styled in this plugin's own aged-paper vocabulary: the gear keeps the
  chip's 22px paper circle exactly, and the panel wears `.preset-dropdown`'s plate.
- **An accessible name on all fourteen knobs.** Pre-existing gap: `bindKnob()` gives
  each knob `role="slider"` and `tabindex="0"` and the markup gave it no name at all.
  `data-i18n-aria` naming the parameter resolves to the control's own tooltip title
  and fixes it in both languages for free.
- **`tests/i18n-states.json`** — two states the label gate cannot reach on its own:
  the settings popover open, and the hover-help switch in its Off position.
- C++ `getUiLanguage` / `setUiLanguage` native functions and a `std::atomic<int>
  uiLanguage`, persisted as a plain `uiLanguage` property on the APVTS root — the
  string `"en"`/`"fr"`, restored behind an `isVoid()` gate. Not an
  `AudioParameterChoice`: it must not reach a DAW automation lane, and loading a
  preset must not change which language somebody reads their interface in.
  **`setProperty` runs BEFORE `getStateAsXml()` and `getProperty` AFTER
  `setStateFromXml()`** — opposite ordering on the two sides, because
  `OuariconPresetManager::getStateAsXml()` starts from `parameters.copyState()` (a
  property written after the copy never reaches the XML) and `setStateFromXml()` ends
  in `replaceState()` (`parameters.state` is the OLD tree until it returns).

### Changed
- **The tooltip anchors moved off `data-tip`.** Canon v2's `applyI18n` WRITES
  `data-tip` as the tip BODY, so the key and the copy would have fought over one
  attribute. The seventeen control cells carry a new `data-param`; the four diagram
  boxes carry an id (`#diagExciteBox`, `#diagResonatorBox`, `#diagMaterialBox`,
  `#diagOutBox`).
- **The tooltip listeners are DELEGATED on the document.** No anchor carries
  `data-tip` until the first `applyI18n` sweep runs, so v1.1.0's
  `querySelectorAll("[data-tip]")` at setup time would have bound nothing at all.
  `pointerover`/`pointerout` and `focusin`/`focusout` because — unlike
  `pointerenter`/`pointerleave` and `focus`/`blur` — they bubble.
- **The tip is built with `createElement` + `textContent`, not `innerHTML`.** The tip
  bodies lost their `<strong>`/`<em>` tags; **the words are unchanged.** Localized
  copy must never reach a markup path.
- Five native `title=` attributes deleted (contract §4 — a native title renders a
  second, untranslated OS tooltip). Two (`#presetPrev`, `#presetNext`) duplicated an
  `aria-label` already there; three (`#presetName`, `#presetSave`, `#presetDelete`)
  were the only help their button had and moved to `data-i18n-aria`.
- `addGroup()` in `buildPresetDropdown()` takes a one-line WRITER instead of a label
  string, so the "Factory"/"User" headings can be localized at the call site with a
  plain literal key. The preset NAMES it lists are untouched — a preset name is its
  JSON filename (D-02). **The vendored `modules/preset-manager.js` was not edited.**

### Fixed
- **PRE-EXISTING: the render harness's version had drifted two releases.**
  `tests/render-harness/CMakeLists.txt` hard-coded `JucePlugin_VersionString="1.0.0"`
  while the plugin shipped 1.1.0. The processor ctor runs
  `initializeFactoryPresets()`, whose `.factory-version` sentinel compares against
  `JucePlugin_VersionString`, so **every harness run rewrote the user's real
  factory-preset directory with a two-release-stale stamp** — observed live, the
  sentinel at `~/Library/O-simplePhysicalModelSynth/Presets/Factory/.factory-version`
  read `1.0.0`. Both files now derive the version BY REFERENCE from a single
  `set(OSIMPLEPHYSICALMODELSYNTH_VERSION ...)` in the plugin's own CMakeLists, the
  shape O-simpleFM and O-simpleGrain already use. `JucePlugin_VersionCode` was stale
  the same way (`0x10000`) and is now `0x10200`.
- **PRE-EXISTING: the resonator-/exciter-aware grey-out would have died silently.**
  `setDisabled()` built its selector from a TEMPLATE LITERAL —
  ``document.querySelector(`[data-tip="${id}"]`)`` — which a grep for the static form
  never finds. Once the anchors moved it matched nothing, with no error and no
  warning: the four gated controls simply stop dimming. Re-pointed at `data-param`
  and verified by driving both engine selectors in a headless page (String/Pluck →
  Modal/Pluck → Modal/Bow → back, all four cells following correctly). **Negative
  control: with the old selector restored, nothing dims in any of the four states.**

### Geometry — two measured rules, four measured copy decisions
D-04 forbids auto-shrink fonts and short-variant fallbacks. The 1040 x 860 frame is a
Locked Decision and is untouched. Each rule was reverted ALONE and the gate re-broke;
each copy decision was reverted alone and the gate re-broke.

| Rule | Measurement |
|---|---|
| `.title-block { flex: 1 1 0; min-width: 0 }` | The header is a two-item space-between row, so the block's used width WAS its own max-content: 520.7px English, 640.5px French. It reported a 113.5px width change AND had to shrink to the 634.2px the preset bar left it, wrapping the strapline and pushing the whole page down 12px. At basis 0 it is 626.3px in both. Costs English nothing — the h1 and strapline are left-aligned blocks in a transparent box. |
| `.preset-act { min-width: 64px }` | SAVE 49.2 / ENREG. 61.4 and DELETE 63.4 / SUPPR. 58.7 — the pair disagrees about which language is longer, net +7.5px in French, which carried the preset bar, the fleuron and the gear left with it. 10 moved elements. One shared 64px rather than two id rules: the per-element widths would be 62 and 64, two pixels saved on SAVE at the cost of an asymmetric pair. |

| French string | Measurement |
|---|---|
| `label.subtitle` **shortened** | The faithful "Synthétiseur à corde **de** Karplus–Strong et **à** résonateur modal" is 640.5px against the 626.3px the header can give. Two connecting words dropped → 605.4px, 20.9px of clearance. 146 moved elements when reverted. |
| `label.vizWaveformHint` **shortened** | "la corde ou le corps qui **résonne, en** s'éteignant" is 203.6px against the 177.3px it has beside its 93.6px caption in a 273.7px viz block; it wrapped and shrank the scope canvas 11px in French only. → "la corde ou le corps qui s'éteint", 135.5px, 41.8px of clearance. |
| `label.knobBowForce` **shortened** | "Force archet" is 76.5px in a 60px cell and wraps to two lines, making its cell 10.5px taller than the English "Bow Force" (59.5px, one line) — the only caption on the page whose LINE COUNT differed. → "Pression", 48.9px. The knob is greyed unless Excitation = Bow and its tip title spells out "Pression d'archet". |
| `label.knobModeBright` **kept at two words** | "Mode Bright" (71.2px) is the one English caption that already wraps in its 60px cell. "Brillance modes" (96.4px) wraps the same way — line one "BRILLANCE" at 57.8px clears the cell — so the cell is the same height in both. A one-word "Brillance" would fit on ONE line and make the French cell 10.5px SHORTER, the same defect in the other direction. |

**One attempted pin was DECORATION and was removed rather than kept and claimed.**
`.settings-label { min-width: 86px }` was written for the popover captions ("Language"
63.5 / "Langue" 47.1, "Hover help" 72.0 / "Aide au survol" 96.0); its negative control
PASSED. The row is space-between with a fixed-width right-hand control, so the
caption's own box positions nothing. `.settings-toggle { min-width: 78px }` is KEPT but
is a DESIGN pin, not a geometry fix — its negative control also passes; what it buys is
that clicking the switch does not resize it ("Activée" 65.0px → "Désactivée").

**FRAME COST ZERO.** The `.frame` scroll extent is 854px inside an 854px client area at
v1.1.0, and 854px in both languages at v1.2.0 — the page did not scroll before and does
not scroll now. Measured by swapping HEAD's files in and out from an in-memory copy,
never `git checkout`.

### Copy decisions recorded
- **`pluck·strike·bow`, `string · loop` and `modal · stems` stay ENGLISH** and are
  `I18N_EXEMPT` under D-01. The first is the `excitationType` choice list verbatim
  (Pluck / Strike / Bow), the other two are the `resonatorType` state line written by
  `applyDiagramSkin()`. Both mirror `AudioParameterChoice` values the combos display
  in English, and translating the diagram without translating the automation lane
  would make the page and the host disagree about what the plugin is SET to. The node
  CAPTIONS above them — EXCITE, RESONATOR, MATERIAL — ARE keyed: a caption names the
  box, these name what is selected inside it.
- The split wordmark (`O – simple` + `PhysicalModel`), the `Default` preset face and
  the two endonyms are the other five exemptions.

### Gates
`check-i18n --plugin` exit 0 on **canon v2**; `check-i18n --strict-v2` exit 0 with
**13 v2 / 0 v1**; `check-ui-labels` **ALL CHECKS PASSED** across three states with
**ZERO** non-label elements moved, vacuity 36/39 labels + 29/29 attributes,
`dataset.label === textContent` after init, after the switch and after a state pass in
both languages, **no coverage hole**, no page error, every resource served;
`boot-all-uis` clean (39 i18n, 0 title); render harness **ALL PASS 22/22**; `auval`
lists `aumu OsPM OuDv`.

**THIS RENDER HARNESS IS DETERMINISTIC, and that was established BEFORE any
cross-version comparison.** Three consecutive runs of one binary are digit-identical,
at v1.1.0 and at v1.2.0 alike. Comparing the two builds: every DSP number is
unchanged, and exactly one number moved — `state-roundtrip stateBytes` 852 → 868, the
16 bytes of the ` uiLanguage="en"` XML attribute this version adds.

### Not verified
- **The C++ persistence round-trip has never been executed by hand on this plugin.**
  The claim that a language choice survives a session is reasoned from the source, not
  measured.
- Windows / WebView2 font metrics remain a named deferral: a French label measured as
  fitting on macOS could clip on Windows. The tightest margin here is the strapline's
  20.9px on 605.4px (3.5%).
- **PRE-EXISTING, reported not fixed: the Delete button deletes without asking.**
  `setupPresetManager()` calls `presetManager.deletePreset()` directly rather than
  `promptDelete()`, and passes no `deleteButton` to the module, so neither the
  module's own confirmation path nor an `onConfirmDelete` hook is ever reached. The
  module's `Delete preset "X"?` sentence and its dialog buttons are unreachable code
  in this plugin — which is why this commit localizes neither. Adding a confirmation
  is a design change, not a localization change.

## [1.1.0] — 2026-08-09

UI enhancement. No parameter/state-format changes.

### Added
- **"?" tooltip toggle button** in the header, right of the preset bar. Toggles
  the on-hover tooltips on/off (defaults to on). The choice persists across
  sessions via `localStorage` (`opms.tipsEnabled`). Same pattern as
  O-simpleGrain v1.2.0; styled as a circular button in the Naturalist palette
  with `aria-pressed` state.

## [1.0.3] — 2026-08-08

Maintenance patch. No parameter/state-format changes. First published release since
v1.0.1 — the v1.0.2 fix below ships in this release as well.

### Fixed
- **MSVC compatibility — SafePointer init-capture in nested lambdas.** Hoisted
  `SafePointer(this)` init-captures to locals so the Windows CI build compiles
  (MSVC rejects the nested-lambda init-capture form).

### Changed
- Added AGPL-3.0 notice headers to all Ouaricon-authored source files
  (repo-wide license compliance pass).

## [1.0.2] — 2026-07-16

Verify-pass residual (VR-01, residual of CR-03). No parameter/state-format changes — PATCH.

### Fixed
- **VR-01 — stale Material-macro AsyncUpdate can stomp a restored state.** A `material`
  automation event on the audio thread stashes damping/decay targets and queues
  `triggerAsyncUpdate()`; if `setStateInformation` ran before the update fired, the
  queued apply landed after `restoringState` cleared and overwrote the restored
  damping/decay with pre-restore values. Root cause: the CR-03 `restoringState` guard
  covered `parameterChanged`/`handleAsyncUpdate` re-entry during the restore, but not
  an update already queued before it. Fix: `cancelPendingUpdate()` in
  `setStateInformation` while the guard is still up — same pattern the destructor
  already uses. Testing: render-harness regression suite re-run post-fix — 22/22 PASS
  (incl. state-roundtrip).

## [1.0.1] — 2026-07-16

Code-review resolution: all Critical + Warning findings from the 2026-07-15 deep
review (CODE_REVIEW.md CR-01..CR-03, WR-01..WR-05). No parameter/state-format
changes — PATCH.

### Fixed
- **CR-01 — heap allocation on the audio thread.** The cached-param scheme was keyed
  by `juce::String` with a `const char*` lookup, so every `p()` call constructed two
  heap-allocating String temporaries — ~32 mallocs per `processBlock`, defeating the
  cache it implemented. Root cause: `std::map<juce::String, …>` + per-call `count/at`.
  Replaced with named `std::atomic<float>*` members assigned once in the constructor;
  zero lookups at render time.
- **CR-02 — FileChooser completion use-after-free.** Both `savePresetWithDialog` and
  `loadPresetFromFile` captured raw `this` and called `complete()` unconditionally; if
  the host destroyed the editor while the native dialog was open, the late completion
  dereferenced a dead editor AND a dead WebView-owned `complete`. Now guarded with
  `juce::Component::SafePointer` and a bare `return` on teardown (calling
  `complete(false)` on the null path would itself be a UAF — suite pattern from
  O-MicrotonalSampler v1.23.5 W12).
- **CR-03 — Material macro stomped explicitly saved Damping/Decay on user-preset load
  and DAW session restore.** Root cause: both persistence paths applied `material`
  after `damping`/`decay`, and the macro listener re-derived + overwrote the just-
  restored values. Two-part fix: (1) preset path — preset-manager module v1.0.5
  applies meta parameters (`isMetaParameter()`) first in both the reset and apply
  passes, so explicit saved values win while material-only factory presets still get
  the derivation; (2) state path — `setStateInformation` sets a `restoringState` flag
  that suppresses the macro (session XML always carries all 17 params explicitly).
- **WR-01 — hard voice stop didn't silence the voice.** `stopNote(…, false)` (CC120
  All Sound Off, `releaseResources`) called `clearCurrentNote()` but left the
  sustain=1 envelope active forever — the voice kept rendering its ringing tail and
  burning the full KS/modal DSP path indefinitely. Hard-stop now resets the envelope,
  string, and modal bank.
- **WR-02 — Material macro could call `setValueNotifyingHost` from the audio thread.**
  APVTS listeners fire on the thread that changed the value; VST3 host automation of
  `material` arrives on the audio thread, making the macro's host-edit callbacks a
  spec violation (deadlock/allocation hazard in some hosts). The listener now applies
  synchronously only on the message thread (preserving preset-load ordering) and
  defers via `AsyncUpdater` otherwise.
- **WR-03 — low notes clamped mistuned.** The KS delay line was sized for a 20 Hz
  floor (`fs/20+100`) but `setFrequency` accepts down to 8 Hz (MIDI 0–13 and
  `coarseTune −24` legitimately request below 20 Hz) — `setDelay` jasserted and
  pinned every such note at ~19 Hz. Delay now sized `fs/8+100` to match the clamp.
- **WR-04 — pitch wheel was dead.** `pitchWheelPos` was tracked but never read by
  `computeF0`. Bend (standard ±2 semitones) is now folded into `computeF0`; block-rate
  application via the existing per-block `setParams` → `setFrequency` path.
- **WR-05 — `getTailLengthSeconds()` under-reported the ring by an order of
  magnitude.** 5.0 s ("max amp release") ignored that the tail is resonator-dominated:
  feedback 0.999 → T60 ≈ 6904/f0 s (~16 s at A4). Now reports 30 s with the
  derivation documented; hosts honoring tail length no longer truncate bounces.

### Module
- preset-manager **1.0.4 → 1.0.5** (meta-params-first apply; no-op for plugins
  without meta parameters).

## [1.0.0] — 2026-06-27

First release. A teaching physical-modeling synth that makes excitation→resonator
synthesis legible: pick how a string/bar is *driven* and what *resonates*, and watch
the energy-recirculation loop, spectrum, and scope respond in lockstep with the sound.

### Added
- **Excitation→resonator architecture** — 3 exciters (**Pluck**, **Strike**, **Bow**)
  drive 2 resonators (**String**, Karplus–Strong / **Modal**). All six exciter×resonator
  combinations are live and swappable mid-note; the live "swap the resonator" gesture is
  the core demo.
- **Material macro** — a single message-thread macro that co-moves **Damping** + **Decay**
  along the steel↔nylon axis, so the most musical control over a struck/plucked string is
  one knob; the underlying sliders visibly track.
- **6 concept-isolating factory presets** (FUNC-07), each isolating one timbral idea and
  together covering all 3 exciters + both resonators:
  - **Bright Steel** — Pluck → String, low Material (bright, long ring)
  - **Muted Nylon** — Pluck → String, high Material (dark, short decay)
  - **Koto Harp** — Pluck → String, mid Material (plucky, medium decay)
  - **Struck Bar** — Strike → Modal, low Inharmonicity (near-harmonic bar)
  - **Bell** — Strike → Modal, high Inharmonicity (inharmonic, long ring)
  - **Bowed String** — Bow → String, mid bow Force (sustained)

  Presets author raw/real-unit values converted to normalized via `convertTo0to1` at
  build time. String presets set `Material` only; Modal presets set `Damping`/`Decay`
  only — never both — so the macro listener and explicit values never fight on load.
- **WebView UI** — animated energy-loop diagram (the recirculation pulse dims in lockstep
  with the audible decay; the Modal skin shows live partial stems), a real-time spectrum
  (harmonic comb for String vs inharmonic for Modal), an output scope, a preset bar
  (navigate / save / load / delete; factory presets non-deletable), and an on-screen +
  QWERTY keyboard. Controls grey out contextually as the exciter/resonator changes.
- **17 parameters** across Excitation (type, position, color, bow force), Resonator
  (type, string model, inharmonicity, mode brightness), Body (damping, decay, material),
  Tuning (coarse/fine), and Amp (attack, release, vel→brightness, output level).

### Validation
- Render-harness (offline DSP, `JUCE_WEB_BROWSER=0`): **ALL PASS** — makes-sound,
  finite/no-blowup, tuning across C1–C7, bow sustain, strike, modal pluck/strike/bow,
  inharmonicity stretch, decay tracking, no-DC, state round-trip.
- pluginval strictness-10: **SUCCESS** (VST3 **and** AU).
- `auval`: **SUCCEEDED**. Native-fn parity 12↔12; param-ID parity 17/17; `node --check` OK.

### Known limitations / deferred
- **Waveguide string model (DSP-06)** deferred to **v1.1** — v1.0 ships the Karplus–Strong
  string model only (`stringModel` defaults to KS).
- Cross-driving demonstrator presets (struck string / plucked bell / bowed bar) were
  intentionally left out of the factory set — the live resonator swap is the demo move.
