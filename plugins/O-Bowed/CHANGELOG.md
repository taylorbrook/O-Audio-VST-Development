# O-Bowed Changelog

All notable changes to O-Bowed will be documented in this file.

## [1.7.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `obow.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 42..109.19, 200 x 67.19 px — byte-identical in English and French** — inside a 900 x 600 frame. The switch face grows 42.00 -> 46.97 px for *Marche*, leftward into the panel's own slack; `check-ui-labels` [7] reports 0 non-label elements displaced.
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


## [1.6.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.
PATCH: two UI defects, no param IDs, ranges, defaults, presets or DSP changed — the render
harness renders the canonical preset bit-identical to the committed golden.

### Fixed

- **item 41 — the three visualisation canvases:** 17 English strings at 13 `ctx.fillText`
  sites (`index.html` v1.6.1:1468-1866 — *Bridge*, *Nut*, *Speed: 0.20 m/s*, *Pressure: 0.50 N*,
  *Position: 12%*, *Playing*, *Silent*, *Frequency*, *Membrane*/*Wood*/*Metal*/*Glass*,
  *Bow Position (β)*, *Bow Pressure (N)*, *P_max (raucous)*, *P_min (slip)*, *Helmholtz*)
  painted in English on the French page. They are now 17 `I18N` entries with an empty body
  (`canvas.*`, the O-Comp shape), read through `tr()` at paint time, so the French page
  paints *Chevalet*, *Sillet*, *Vitesse : 0.20 m/s* (the number keeps its point, as every
  readout on this page does — only the label and the unit are localized), *En jeu*, *Au repos*,
  *Fréquence*, *Bois*/*Métal*/*Verre*, *Position d’archet (β)*, *Pression d’archet (N)*,
  *P_max (rauque)*, *P_min (glissement)*. *Membrane* and *Helmholtz* (a name) stay. The
  spectrum and Schelleng canvases paint on demand, so a `MutationObserver` on `<html lang>`
  — the attribute the i18n canon writes on every switch — now redraws them; the bow-string
  canvas repaints every frame and follows by itself. Every French string was measured with
  `measureText` at its canvas font, in this page, against the space the English occupied
  (498 × 398 canvas): the widest are *Pression d’archet (N)* 93.69 px (English 77.65)
  rotated on a 338 px plot height, *P_min (glissement)* 78.71 px (50.32) in a 120 px legend
  slot, and *Chevalet* 34.40 px centred on the canvas edge (half-width 17.20 in 40). No
  abbreviation, no font change. A `fillText`-recording probe driven en → fr → en confirms
  every keyed string paints French under `fr` and English under `en`, and that no English
  literal from the list paints under `fr`.
- **item 42 — Sympathetic Decay at Count 0:** `updateSympVisibility()` (`index.html:1395`)
  hid `#sympAmount-ctrl` only, so Decay stayed on screen at Count 0 where the engine has zero
  strings and the knob does nothing. Both knobs now hide and show together; `tip.count` says
  "its Amount and Decay knobs are hidden" and `tip.decay` says the knob is only on screen while
  Count is above 0 (both French bodies follow and are `reviewed: false` again). Probed: at
  Count 0 both controls have a null `offsetParent` and a 0 × 0 rect; at Count 1 both are
  62 × 80.6. `tests/i18n-states.json` gains a state that drives Count to 0 through the stub,
  so `check-ui-labels` measures that state explicitly.

### Not changed

- The 19 unreviewed French entries (17 new, 2 rewritten) are the developer's worklist
  (`node scripts/check-i18n.js --plugin O-Bowed`).
- `tests/render-harness/CMakeLists.txt` still hardcodes `JucePlugin_VersionString="1.3.0"`;
  it names the harness binary, not the plugin, and has been stale since 1.4.0.

## [1.6.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout — Checkpoint 5 run as a QA
pass against a suite glossary and a lint, not as a re-translation. PATCH: copy and one
canon behaviour only. No param IDs, ranges, defaults, English copy, keys or DSP changed.

### Changed

- **24 French entries of 71 revised** against `scripts/i18n-fr-glossary.js` and
  `scripts/i18n-fr-lint.js` — 7 terminology, 8 typography, 8 grammar/agreement, 1 meaning.
  The lint went 27 findings → 0 (`--strict` exits 0). The visible ones:
  - **Matière → Matériau** on the Body section's caption and its tooltip title, the
    suite's settled word for what a resonator is made of.
  - **"la corde archetée" → "la corde frottée"**, and "une note archetée" → "une note
    jouée à l'archet": *archeté* is not a French word, while *cordes frottées* is the
    organology term for bowed strings.
  - **No-break spaces** before every `:` and `;` and between every number and its unit —
    *8 Hz*, *220,0 à 880,0 Hz*, *−60,0 à +12,0 dB*, and also *2,00 m/s* and *5,00 N*,
    which the lint's unit list does not yet carry.
  - **A dropped clause restored** in Sub-Harmonics: the French said "an octave lower and
    below" with nothing for *lower* to be lower than; it now says *une octave sous la note
    jouée*, which is what the English says.
  - Four calques and dangling referents rewritten — *Un peu redonne / beaucoup pousse*,
    *une houle lente*, *le bouton est incurvé*, *Elle change la part d'énergie*.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so
  assistive technology reads the page in the language it is displayed in.
- **Two glossary terms kept against the lint, with a recorded reason** (`termNote`), and
  both alternatives measured rather than argued: **Fréq.** for the Humanize Rate knobs
  (`HumanizeEngine.h:83` maps them to a 0.15–8 Hz corner, so it really is a frequency, and
  they sit under a column captioned *Vitesse*), and **Tenue** for Infinite Sustain (this
  page carries no envelope, so the glossary's *Maintien* — which settles the ADSR sustain
  segment — names something that does not exist here).
- **A stale width number corrected in `js/i18n.js`'s header.** It still described
  `.knob-label` as a 62 px cap; `index.html:463` raised `max-width` to 64 px at v1.5.0.

### Not changed

- `reviewed: false` on all 71 entries. The flag means a native speaker has read the
  string; this pass was a second machine reading against a glossary, and the header
  records it instead.
- The 15 English strings the three visualisation canvases paint with `ctx.fillText`
  (*Bridge*, *Nut*, *Frequency*, *Playing*, *Silent*, *Helmholtz*, the four material
  names, …). They were reported at Stage K3 and named a Stage-M backlog; they are still
  hardcoded in `index.html` and have no French value to review.

## [1.6.0] - 2026-08-30

Hover-help, in both languages. Stage M batch M2 of the suite-wide i18n rollout.
MINOR — a user-visible feature arrives. No param IDs/ranges/defaults changed and no DSP:
the render harness renders the canonical preset bit-identical to the committed golden
before and after.

### Added

- **Hover-help on 30 controls, English and French.** Every one of the 28 parameters that
  has a control on the page, plus the gear and the language selector. Each entry is a
  TITLE and a body of at most three sentences ending in the range and unit. All French is
  a MACHINE DRAFT flagged `reviewed: false` — no native speaker has read it.
- **A tooltip renderer, because the table alone shows nothing.** Canon v2's `applyI18n()`
  writes `data-tip-title` and `data-tip` onto each bound anchor and stops there; the code
  that reads those attributes and paints a surface is per-plugin and did not exist here.
  Measured across all 22 bare plugins before this stage: `id="tooltip"` 0, `.tooltip {` 0,
  `closest("[data-tip]")` 0. Ported from O-simpleFM's delegated, cursor-following family
  (~80 lines) rather than O-Tapestop's measure-then-pin engine (~180), which exists to
  serve a flip-above/below design with an arrow this page does not have.
  - Delegated on `document`, not `querySelectorAll('[data-tip]')` at setup: no anchor
    carries `data-tip` until `applyI18n()` has run, so a setup-time query binds nothing.
  - Built with `createElement` + `textContent`, never `innerHTML` — localized copy must
    not reach a markup path.
  - Flip THEN clamp again, on both axes, 8 px margin. Measured over 60 hovers across both
    languages on this 900 x 600 frame: 20 tips placed left of the cursor, 30 above it.
  - **A last-input-device focus latch.** A mouse click on a `<button>` focuses it, so the
    reference implementation's unconditional `focusin` rule reopens the tip `pointerdown`
    just hid and parks it over whatever the click opened. `:focus-visible` is not the
    discriminator — Chromium reports it false for a programmatic `.focus()` after a click.
  - **A mid-drag guard.** A knob drag begins on `mousedown` and is tracked on `document`
    mousemove, so a drag crossing into a neighbouring cell would otherwise open that
    neighbour's tip mid-gesture. `pointerdown` alone cannot cover it: `pointerover`
    arrives after it.
  - Styled in this page's own vocabulary — `--brown-frame` on `--bg-paper` is `.preset-bar`
    inverted, with `.settings-popover`'s 6 px radius and 1 px border, and `--green-accent`
    on the title.
- **`tests/ui_tip_render_check.js`** — 457 assertions, the gate that can see a rendered
  tooltip, because no gate in this repo could. It drives the real page at the shipping
  900 x 600, hovers a DESCENDANT of every anchor in both languages, byte-compares the
  rendered title and body against the table, and measures all four edge clearances.
  **Measured, not asserted: with `setupTooltips()` disabled, `check-i18n` and
  `check-ui-labels` both print ALL CHECKS PASS while this gate fails 70.**
- **`.planning/params.tsv`** — the runtime parameter inventory, from a walk of
  `AudioProcessor::getParameters()` on a constructed processor. A regex over
  `createParameterLayout()` is not authoritative: eight of this plugin's parameters come
  from a factory lambda that CONCATENATES their IDs (`humanizeSpeed` + `Range`), which is
  exactly the case static parsing cannot see. The `ouaricon_add_param_dump()` call joins
  the render harness inside the existing `OUARICON_BUILD_TESTS` block rather than
  declaring a second `option()`.

### Changed

- `PluginProcessor.cpp` no longer includes `PluginEditor.h` at the top of the translation
  unit. The include moved behind `#if JUCE_WEB_BROWSER` directly above `createEditor()`,
  with a `GenericAudioProcessorEditor` fallback, so the param-dump console target — which
  builds this TU with `JUCE_WEB_BROWSER=0` and no editor sources — links. Under a normal
  build `JUCE_WEB_BROWSER=1` and behaviour is byte-identical to v1.5.0. The render harness
  also compiles with `JUCE_WEB_BROWSER=1` and is unaffected.

### Findings, recorded and deliberately NOT fixed

- **`tuningSystem` has no control anywhere in the WebView.** It is a real
  `AudioParameterChoice` with three options, it is automatable and host-reachable, and
  `PluginEditor.cpp:78` even builds a `WebComboBoxRelay` for it — but no `<select>` on the
  page is bound to it, the page's own `bindComboBox()` helper is never called, and the
  shared tuning panel does not carry one either. So 29 dumped parameters produce 28
  controls and 28 parameter tips. Adding a selector is a feature change with a geometry
  cost and a host-visible surface; authoring a body for it without a control would be an
  ORPHAN that `check-i18n` assertion 2 fails by design.
- **`tests/render-harness/CMakeLists.txt` still hardcodes `JucePlugin_VersionString="1.3.0"`
  and `JucePlugin_VersionCode=0x10300`**, three minors stale. Reported in Stage K and still
  a standing decision item; the harness is a console binary and nothing shipped reads it.
- **The Tuning page is English in both languages.** It is the shared
  `scala-tuning-engine` module consumed by reference, the same verdict Stage K recorded for
  O-Wind. `tip.language`'s body says so rather than overpromising.

## [1.5.0] - 2026-08-29

The page speaks French. Stage K batch K3 of the suite-wide i18n rollout, canon v2.
MINOR — no param IDs/ranges/defaults changed, and no DSP: the render harness renders
the canonical preset bit-identical to the committed golden before and after.

### Added

- **French UI, 41 keyed strings.** Every visible caption on the page is now owned by its
  element through `data-i18n` and rendered from `Resources/ui/js/i18n.js`: the six Bow
  knobs, the Humanize grid, the three visualisation tabs, the Impossible row, Body,
  String, Sympathetic, the footer, and the two header buttons. All French is a MACHINE
  DRAFT flagged `reviewed: false` — no native speaker has read it.
- **A language selector**, in a gear popover at the end of the preset bar, styled in this
  plugin's own header vocabulary (28 px, 4 px radius, the translucent border its two
  preset-nav siblings already use). The two options are ENDONYMS and are never translated.
- **The language persists with the session.** `uiLanguage` is a non-parameter property on
  the APVTS state tree — deliberately NOT an `AudioParameterChoice`, so it cannot appear
  in a DAW automation lane and no preset can change which language somebody reads their
  plugin in. It rides through `OuariconPresetManager::getStateAsXml()`'s `copyState()`,
  the idiom this processor already uses. Read back with `isVoid()`, because the XML
  round-trip rebuilds every property as a string `var`
  (`critical_valuetree_xml_roundtrip_loses_type`).
- **Accessible names for the three header controls.** The text is the text v1.4.1 carried
  in its native `title=` attributes, MOVED not rewritten. No hover-help prose is invented
  here; that is a later stage.

### Fixed

- **The page threw `SyntaxError: Unexpected token 'export'` on every load, in English, at
  rest, in the plugin as shipped.** `js/juce/index.js` is an ES module and was ALSO being
  loaded by a second, CLASSIC `<script src>` tag. The UI still worked — the module
  `<script>` below it imports the same file correctly — so nothing surfaced it until the
  repo-wide boot sweep counted O-Bowed as a failed boot. The redundant tag is deleted.
  `check_native_interop.js` stays a classic script, which is what it is.
- **`Rev. Friction` has been rendering with an ellipsis.** `.knob-label`'s `max-width` was
  62 px, the same number as `.knob-control`'s width, and that caption needs 63.0. A
  clipped caption pushes nothing, so no layout check could see it; the label gate's own
  per-string measurement is what named it. The cap is now 64 px, which touches no other
  caption — the widest French string on the page is `Colophane` at 52.70.

### Changed

- **Three geometry pins, so that no non-label element moves between the two languages.**
  Each was reverted alone and confirmed to re-break the gate; none is decoration.
  - `.humanize-grid` is `repeat(4, minmax(0, 1fr))` and `.humanize-col` is `width: 100%`.
    `repeat(4, 1fr)` means `minmax(auto, 1fr)`, so the four tracks were content-sized at
    38 / 44 / 41 / 38 and summed to 164 inside a 162 px box. `Pressure` -> `Pression`
    SHRINKS 1.69 px, which moved the column and everything in it.
  - `.preset-save-btn` is pinned to 54 px and `.tuning-btn` to 62 px, both the English box
    rounded up. `.preset-name-display` is `flex: 1`, so every pixel either button gained
    was taken from it and moved all four elements to its right.
- **English geometry moves once, for the gear.** The settings cluster costs
  `.preset-name-display` 36 px, so `#preset-next`, `#preset-save`, `#tuning-toggle` and
  the maker's name sit ~39 px further left than in v1.4.1. That is a markup change, not a
  language one.

### Known Limitations

- **The three visualisation canvases still draw ENGLISH.** ~15 prose strings are painted
  with `ctx.fillText` — `Bridge`, `Nut`, `Playing`/`Silent`, `Frequency`, `Membrane` /
  `Wood` / `Metal` / `Glass`, `Bow Position`, `Bow Pressure`, `P_max (raucous)`,
  `P_min (slip)`, `Helmholtz`, and the three `Speed:` / `Pressure:` / `Position:` readout
  prefixes. Canvas text is not a DOM node, so no scanner in this repo reports it and no
  geometry check can measure whether a longer French string would clip. Deliberately left
  English: canon v2 has no mechanism for it, and the obvious one (a `LABELS` key read
  through `trLabel()`) is rejected by `check-i18n` assertion 15 as a dead key.
- **The tuning panel is module-owned** (`modules/tuning/scala-tuning-engine`) and stays
  English. Localizing it is cross-plugin work and a local edit would be reverted by
  `/module-upgrade`.
- **All French is unreviewed.** `node scripts/check-i18n.js` prints the worklist.

## [1.4.1] - 2026-07-08

Resolves the three runtime-affecting Info findings from the v1.3.0 review (`CODE_REVIEW.md`).
PATCH — no param IDs/ranges/defaults/state format changed.

### Fixed

- **IN-06 — BodyResonator biquad bank now NaN-guarded.** A transient non-finite sample reaching the
  8-mode parallel bank would latch every biquad state to NaN permanently (sticky silence,
  `pattern_biquad_nan_guard_sticky_silence`). `processStereo` now checks the accumulated resonance
  with `std::isfinite`, and on failure `reset()`s the bank and drops the sample. Complements the
  v1.4.0 WR-01 source reset on the voice path.
- **IN-07 — Humanize drift rate now uses the actual block size.** `HumanizeEngine` derived its
  update rate (`sampleRate / blockSize`) from the *max* block size in `prepare()`, so under smaller
  host buffers (e.g. 64 vs a 512 max) the random walk advanced faster than its labeled 0.15–8 Hz
  (~8× at that ratio). `update()` now takes the real `numSamples` and computes the rate per callback.
- **IN-09 — Master-path state cleared on (re-)prepare.** `dcBlockX/Y` persisted across
  `prepareToPlay` calls (and `releaseResources` is a no-op), leaking a startup transient on
  sample-rate changes. `prepareToPlay` now zeroes the DC-blocker state and resets the body /
  sympathetic engines.

### Build

- **CMake `VERSION` keyword corrected.** `juce_add_plugin` was given `PLUGIN_VERSION "1.4.1"`, which
  JUCE does not recognize — the keyword was silently ignored and every bundle shipped
  `CFBundleShortVersionString`/`CFBundleVersion` `1.0.0` instead of `1.4.1`. Renamed to the correct
  `VERSION "1.4.1"` so the built VST3/AU bundles now report `1.4.1`. Build metadata only; no code,
  param, or state change.

### Known Limitations

- Remaining Info findings IN-02, IN-04, IN-05, IN-08, IN-10, IN-11, IN-12, IN-13 (dead code, comment
  fix, dead UI/native-fn cruft, unconditional viz poll) remain deferred — cosmetic / non-behavioral.
  See NOTES.md Known Issues.

## [1.4.0] - 2026-07-08

Resolves the Critical + Warning findings from the v1.3.0 deep code review (`CODE_REVIEW.md`).
MINOR bump: `bowHairStiffness` becomes an audible control (new behavior), backward compatible
(no param IDs/ranges/state format changed; presets load unchanged).

### Added

- **`bowHairStiffness` now does something (WR-02 + CR-04).** The elasto-plastic bristle friction
  model was fully wired but never evaluated, and `bristleBlend` (= `bowHairStiffness`) was never
  read — the knob was doubly dead (no UI binding *and* no DSP effect). The render loop now blends
  the Hyperbolic (Core) and elasto-plastic (bristle) reflection coefficients:
  `rho = (1-blend)*core + blend*bristle`, with the bristle model receiving the sample period `dt`
  it needs. The bristle displacement state `z` is reset on every note-on and `R_s` is floored
  away from zero (IN-03, required companions so activating the path can't emit NaN).
  **The default was changed 0.5 → 0.0** so new instances and factory presets (which don't set
  this param) keep the pre-v1.4.0 timbre (pure Core friction); the bristle character is now an
  additive enhancement as the knob is raised. (Note: sessions saved under ≤1.3.0 that stored the
  old 0.5 default will now play with 50% bristle — unavoidable when activating a previously-inert
  parameter.)
- **Four previously-uncontrollable parameters are now bound to their UI knobs (CR-04):**
  `sympatheticDecay`, `bodyAmount`, `stringGauge`, `bowHairStiffness`. They had real
  `NormalisableRange`s and (three of them) drove DSP, but the editor created no
  `WebSliderRelay`/`WebSliderParameterAttachment` and the JS `PARAMS` table omitted them, so the
  knobs rendered frozen. Added relays, attachments, `withOptionsFrom`, and `PARAMS` entries.
- **Tuning panel is fully functional (CR-03).** Registered the eight native functions the shared
  `tuning-panel.js` calls that O-Bowed's editor was missing: `getEmbeddedTuningList`,
  `loadEmbeddedTuning`, `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`,
  `saveScalaFile`, `saveKBMFile`, and `exportTuningHTML` (the last renamed from the drifted
  `getTuningHTML`). Previously the factory-tuning library was empty, every generator was dead
  (all funnel through the missing `applyGeneratedScale`), and Save .scl / .kbm / Export HTML did
  nothing — all failures swallowed by the panel's try/catch. `loadEmbeddedTuning` appends the
  tuning period before `setCustomIntervals` (guards `pattern_embedded_tuning_period_dropped`).
- **Skew-correct knob readouts + double-click reset (WR-07).** Added a `getParameterDefaults`
  native function; the UI now reads displayed values from `SliderState.getScaledValue()` (honors
  the real C++ `NormalisableRange` incl. skew) and resets to the true normalized default. The old
  hardcoded linear map read skewed params wrong — `brightness` ~8× off at mid-travel,
  `bowSpeed`/`bowPressure`/`stringGauge` ~2× off — and reset landed off-default.

### Fixed

- **No audio-thread heap allocation on bridge loss-filter updates (CR-01).** `updateBridgeFilterCoeffs`
  constructed a temporary `Coefficients<float>` (heap-allocating its internal array, then copy-
  assigning it — a second alloc + free) on **every note-on** and on **every Brightness / Infinite-
  Sustain** change. Now the coefficient storage is seeded once in `prepare()` and the update assigns
  a stack `std::array` in place, reusing the storage (verified against JUCE `assignImpl`:
  `clearQuick` + `ensureStorageAllocated(>=8)` → no realloc). Root cause: fresh `Coefficients`
  object instead of writing into the pre-allocated storage.
- **No audio-thread heap allocation on body Material/Size automation (CR-02).** `BodyResonator::
  updateCoefficients` rebuilt 8 `Coefficients::makePeakFilter` reference-counted objects (8 `new`
  + up to 16 `delete`) inside `processBlock` on any live Material/Size move. Now one shared
  coefficient object per mode is pre-allocated in `prepare()` and mutated in place from
  `ArrayCoefficients::makePeakFilter` (a stack `std::array`); JUCE only reallocs filter state on an
  order *change*, which no longer happens. (`pattern_arraycoefficients_rt_safe_iir`, same class as
  the O-Formant EQ regression.)
- **FileChooser completions no longer use-after-free on editor teardown (CR-05).** All six
  `launchAsync` completions (`savePresetWithDialog`, `loadScalaFile`, `loadKBMFile`, and the new
  `saveScalaFile`/`saveKBMFile`/`exportTuningHTML`) captured `this` + the WebView `complete`
  callback with no liveness guard. Closing the plugin window while a native dialog was open would
  run the lambda after `~OBowedAudioProcessorEditor`, touching a destroyed WebView-owned
  `complete` → host crash. Now each captures a `Component::SafePointer` and bails with a bare
  `return` on the null path (does NOT call `complete()` — that is itself a UAF, per the
  O-MicrotonalSampler W12 fix).
- **A single non-finite excitation no longer silences a note mid-sustain (WR-01).** The `std::min`
  rho clamps don't filter NaN (`min(NaN,x)==NaN`) and `tanh` preserves it, so one bad sample would
  poison the delay line and drive `energyEstimate` to NaN → `clearCurrentNote()`. Added an
  `std::isfinite` guard at the write boundary that resets the *source* (waveguide + both friction
  models), not just the sample.
- **Sympathetic "Decay" knob now actually controls ring time (WR-05).** `lossCoeff` was used as the
  pole of a one-pole lowpass (DC gain = 1) instead of a sub-unity loop gain, so the fundamental rang
  ~forever regardless of the knob and any loop DC never drained. Split into a fixed-pole damping
  lowpass (tone) plus an explicit sub-unity feedback `decayGain` (0.990–0.9995) derived from the
  Decay param — guarantees exponential decay and drains DC.
- **No zipper on continuous MPE-timbre / Brightness moves (WR-04).** `bowPosition` (driven by CC74)
  and `brightness` were pushed to the waveguide once per block, stepping the fractional delay
  lengths and one-pole corner at block boundaries. Both are now advanced per sample via
  `SmoothedValue` (15 ms), primed to the actual value on note-on so the attack doesn't sweep.
- **Belt-and-suspenders denormal protection on the voice render path (WR-03).** Added
  `ScopedNoDenormals` to `renderNextBlock` (the bridge loss-filter's ~15 s tail decays through the
  denormal range; the processor's master guard covers the plugin but the voice runs unprotected in
  the render harness).

### Changed

- **`processBlock` reads parameters via cached atomic pointers (WR-06).** Replaced ~25 per-callback
  `getRawParameterValue("id")->load()` string-keyed map lookups with `std::atomic<float>*` members
  resolved once in `prepareToPlay`. Also removed 8 dead reads (bowSpeed/bowPressure/bowPosition/
  rosin/brightness/infiniteSustain/stringGauge/bowHairStiffness were read on the processor thread
  but only ever used inside the voice).

### Known Limitations

- Info-level findings IN-02, IN-04, IN-05, IN-06, IN-07, IN-08, IN-09, IN-10, IN-11, IN-12, IN-13
  were out of scope for this pass (Critical + Warning only). IN-01 and IN-03 were resolved as
  required companions to WR-02. Notably IN-06 (BodyResonator biquad NaN guard) complements WR-01
  and remains a recommended follow-up. The per-voice `updateParametersFromAPVTS` still uses string
  lookups (WR-06 only covered the processor as flagged).

## [1.3.0] - 2026-04-26

### Added

- **adds VST3 Note Expression microtonal support for Dorico.** O-Bowed responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling microtonal playback of quarter-tones and arbitrary tuning deltas authored in Dorico's tonality system. End users must set Microtonality to "VST3 Note Expression" on the assigned expression map (see O-Lyrica 2.3.0 for procedure).
- **Shared `note-expression` module adoption.** O-Bowed consumes the Ouaricon module at `modules/tuning/note-expression` (v1.0.0), same shape as O-Lyrica v2.3.0 / O-Bells v4.1.0 / O-Prism v1.17.0 / O-Wind v1.16.0 / O-IntonationPad v2.8.0 / O-Reed v1.1.0.

### Technical Notes

- **Second MPE consumer of the shared note-expression module.** `BowedStringVoice` extends `juce::MPESynthesiserVoice` (via `BowedMPESynthesiser`) and reads MIDI pitch via `getCurrentlyPlayingNote().initialNote` from `noteStarted()` and `notePitchbendChanged()` (no parameter form). Pattern 1 (`noteId` correlation in the shared module's `updatePendingFromEvents`) holds regardless of MPE channel — same as O-Reed v1.1.0.
- **Helper-based composition (single source of truth).** `applyPendingTuning` is invoked INSIDE `getBaseFrequencyFromTuning(midiNote)` so BOTH call sites — `noteStarted()` (line 32) and `notePitchbendChanged()` (line 71) — inherit the NE delta with one insertion. `exchange(0.0)` consume semantics correct for one-NE-per-noteOn delivery: first call (in `noteStarted`) consumes the slot; the `notePitchbendChanged` call during a held note returns base unchanged (NE applies once per noteStarted; MPE pitch-bend updates per-block on top).
- **Composition order:** tuning engine → NE delta → MPE pitch-bend → `waveguideString.trigger(currentFrequency)` (waveguide string period sized to the final tuned frequency on sample 0; Pattern 2 satisfied — first sample at tuned pitch, no attack zipper).
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/BowedStringVoice.{h,cpp}`, `CMakeLists.txt` (added `PLUGIN_VERSION "1.3.0"` line + `ouaricon_add_module(O-Bowed note-expression)`).
- **Version:** 1.2.1 → 1.3.0 (MINOR — new user-visible feature, backward compatible, no preset impact).

## [1.2.1] - 2026-04-19

### Fixed
- **Humanize panel layout** — column labels (Speed/Pressure/Position/Rosin) were overflowing their 42px-wide dial cells in the 200px-wide left panel, causing visual crowding. Reduced column-label font from 9px → 8px, tightened letter-spacing from 0.5px → 0.3px, narrowed dial cells from 42px → 38px, and reduced grid gap from 4px → 1px so all four labels and their dial pairs fit cleanly within the section box

## [1.2.0] - 2026-04-17

### Added
- **Humanize** section with per-parameter random-walk variation on the four primary bow controls (Speed, Pressure, Position, Rosin). Each parameter exposes two dials:
  - **Amt** (range) — peak deviation. At 0 the walk is inactive; at 1 the parameter drifts across its full musically-tuned deviation (speed ±0.30 m/s, pressure ±0.60 N, position ±0.05, rosin ±0.20).
  - **Rate** — drift speed, mapped 0→0.15 Hz to 1→8 Hz as the corner frequency of a 1-pole smoother fed with uniform-noise targets (sum of 3 uniforms for a Gaussian-ish distribution).
- The walk state lives in a new processor-level `HumanizeEngine` (`Source/DSP/HumanizeEngine.h`), updated once per `processBlock` and shared across all 8 voices so the instrument breathes coherently. State is continuous — never reset on note-on — matching the natural drift of a live player
- Default Rate values (0.20–0.35) chosen to feel like subtle natural bow fluctuation (≈2–3 Hz). All Amt dials default to 0 so existing presets are unchanged

## [1.1.2] - 2026-04-16

### Fixed
- Audible thump at the start of every new note whenever Reversed Friction was non-zero. Root cause: the reversal formula `rho = rho + reversedAmount * (1 - 2*rho)` evaluates to `reversedAmount` when base `rho ≈ 0` (which it is while `F_bow` is still ramping through the attack envelope). That pinned `rho` from the very first sample of the note, making `frictionVelocity = 2ρ/(1-ρ)` large enough for the sticking branch (`|v_delta| < frictionVelocity`) to always hold — so the bow-velocity attack ramp was injected directly into a freshly reset waveguide as a velocity step, producing a broadband click. Fixed with a per-voice one-pole smoother on `reversedAmount` that resets to 0 on `noteStarted()` and ramps to the knob value over ~25 ms at the oversampled rate, letting physical friction build up first before the reversal takes effect. Sustain-time character is unchanged
- `BowModel::startBow()` now zeros `v_bow`/`F_bow` before setting new targets, so retriggering a note while a previous note's release envelope is still decaying no longer carries residual bow state into the new attack

## [1.1.1] - 2026-04-11

### Fixed
- Enhanced (elasto-plastic) and Quality (thermal) friction models produced no sustained tones. Root cause: bristle stick-slip thresholds (`z_ss`, `z_ba`) were hardcoded constants that didn't scale with bristle stiffness (`sigma_0`). The steady-state bristle displacement (`mu*F_bow/sigma_0 ≈ 6.7e-5`) fell far below the stick threshold (`z_ss = 5e-4`), trapping the model in the elastic region where no stable equilibrium exists — bristle state oscillated erratically instead of converging. Fixed by making thresholds scale inversely with `sigma_0` so the transition zone always surrounds the physical equilibrium point
- Friction state now resets on note-on to prevent stale bristle displacement from previous notes affecting attack

## [1.1.0] - 2026-04-11

### Removed
- Drone string functionality entirely — removed `stringCount`, `stringTuning1-4` parameters, `DroneStringEngine` class, and all associated DSP/UI code. Drone strings were a persistent source of bugs (v1.0.3, v1.0.4, v1.0.7) and redundant with simply playing the desired note
- "Metal Drone" factory preset (drone-dependent)

### Changed
- Factory presets reduced from 11 to 10 (7 realistic + 3 sound design)
- Nyckelharpa preset: removed drone string, retains sympathetic strings for authentic character
- Impossible Strings preset: removed drone dependency, retains sympathetic strings + impossible physics
- Full 8-voice polyphony always available (no longer dynamically capped based on drone count)

## [1.0.7] - 2026-04-11

### Fixed
- Phantom second note playing in harmony alongside every MIDI note. Root cause: `stringCount` parameter range was 1-4 (minimum 1), forcing at least one drone string to always be active. Drone strings play at the fixed reference pitch (440Hz default), not the MIDI note frequency, producing an audible second tone at a different pitch from the played note. The drone persisted beyond note-off due to natural waveguide energy decay
  - Changed `stringCount` range from 1-4 to 0-4 with default 0 (no drones)
  - Updated `DroneStringEngine::setStringCount()` to accept 0
  - Recalculated all factory preset drone counts: realistic instruments (Violin, Cello, Viola, Double Bass, Erhu, Sarangi) now default to 0 drones; Nyckelharpa keeps 1 drone (authentic); Metal Drone and Impossible Strings keep 2 drones (intentional)

## [1.0.6] - 2026-04-11

### Fixed
- Sound design presets (Glass Bow, Metal Drone, Impossible Strings, Breath of Strings) produced saturated infinite tones with overtone dominance instead of musical bowed sounds. Three root causes:
  1. Loop gain reached 1.0 at high `infiniteSustain` (zero energy loss), causing waveguide saturation and harmonic distortion from tanh limiter. Capped max loop gain at 0.9995 (~15s decay at 440Hz)
  2. No feedback mechanism to reduce bow excitation at high waveguide energy. Added energy-aware excitation limiting: automatically scales down reflection coefficient when waveguide energy exceeds a sustain-dependent threshold (physically motivated — bow loses grip on strongly oscillating string). Zero effect when `infiniteSustain` = 0
  3. Reversed friction could push rho beyond stable range, causing excessive velocity injection. Clamped post-reversal rho to max 0.85
- Retuned all 4 sound design preset values for stability: Glass Bow (infSustain 0.8→0.45), Metal Drone (infSustain 0.5→0.3, reversed 0.4→0.2, subHarm 0.6→0.3), Impossible Strings (infSustain 0.7→0.4, reversed 0.6→0.3, subHarm 0.8→0.35), Breath of Strings (infSustain 0.3→0.15)

## [1.0.5] - 2026-04-09

### Fixed
- Output clipping at +6dB even with quiet settings. Root cause: `BodyResonator::processStereo()` summed 8 parallel peaking EQ filters without averaging — each filter outputs ~unity at non-peak frequencies, inflating the baseline by 8x (~+18dB). Fixed by dividing the parallel sum by `NUM_MODES`, restoring unity baseline gain while preserving relative resonant peak character

## [1.0.4] - 2026-04-09

### Fixed
- Drone strings produced continuous sound even with no MIDI notes held. Root cause: `DroneStringEngine::setStringCount()` unconditionally called `startBow()` on activation and never stopped bowing. Added MIDI note-activity gating via `setNotesActive()` — drones now only bow while at least one synth voice is active, with natural release decay when all notes are released

## [1.0.3] - 2026-04-09

### Fixed
- No sustained tone: `newVelocity = rho * v_delta` produced a monotonically increasing effective friction curve (no negative slope), making Helmholtz self-excitation impossible. Replaced with stick-slip injection model: reconstruct friction velocity from rho (`2*rho/(1-rho)`), clamp to `|v_delta|` for sticking limit. Creates the required friction peak + negative slope for sustained oscillation
- One-sided bow injection (bridge-only) starved the nut-bound wave of energy. Restored symmetric injection to both outgoing waves per standard scattering junction
- Hard clipping (`jlimit ±1.5`) inside the waveguide feedback loop generated DC offset. Replaced with `tanh` soft saturation at ±4.0 (odd-symmetric, no DC generation)

## [1.0.2] - 2026-04-06

### Fixed
- Critical waveguide scattering junction bug: outgoing waves were pushed back into the same delay line they came from instead of crossing to the opposite side, causing incorrect wave propagation in both `processSample()` and `writeJunction()`
- Bridge output now correctly reads from nut reflection (wave traveling toward bridge)
- Critical sustained oscillation failure: bow injection was added to BOTH outgoing delay lines (common-mode), causing the termination reflections to cancel the injection after each round trip — only a transient click was audible. Fixed by injecting only into the bridge delay line (one-sided injection), which breaks the cancellation and allows energy to accumulate across round trips

## [1.0.1] - 2026-04-06

### Fixed
- All 11 factory presets produced near-silent output due to inverted skew normalization formula for bowSpeed and bowPressure parameters (used `pow(proportion, 1/skew)` instead of `pow(proportion, skew)`)
- Presets now produce correct bow speed (e.g., Violin: 0.2 m/s instead of 0.02 m/s) and pressure values

## [1.0.0] - 2026-04-05

### Added
- Initial release
- Physical modeling bowed string synthesis via digital waveguide + nonlinear friction junction
- Tiered friction model: Core (hyperbolic), Enhanced (elasto-plastic), Quality (thermal)
- Morphable body resonator with Material and Size controls (membrane/wood/metal/glass)
- 1-4 active bowed strings with per-string tuning offsets
- Sympathetic string coupling (0-12 passive waveguide strings)
- Impossible physics: Infinite Sustain, Reversed Friction, Sub-Harmonics
- MPE support (per-note pitch bend, pressure, slide)
- Microtonal tuning: Scala/TUN import, MTS-ESP, 12-TET
- WebView UI with Naturalist aesthetic
- 11 factory presets (7 realistic instruments + 4 sound design)
- Passes pluginval level 10 (VST3 + AU)

### Technical Notes
- 2x oversampling on friction junction
- 8-mode parallel biquad body resonator
- Zero algorithmic latency (waveguide is causal)
