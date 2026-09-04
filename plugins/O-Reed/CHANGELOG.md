# O-Reed Changelog

## v1.4.0 (2026-09-03)

A switch for the hover help. v1.3.0 gave this plugin 35 tooltips and no way to
turn them off; twenty of the suite's forty-three plugins already carried that
switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling. It
  gates `setupTooltips`'s own `show()` — a delegated renderer has no bindings to
  unbind — and persists under the localStorage key `oreed.tipsEnabled`.
- **`data-tip-always` on `#gear-btn` and on `#tips-toggle`, and on nothing
  else.** Those two controls are the ones that REACH and RESTORE the help layer,
  so they keep explaining themselves while it is off. `#lang-select`
  deliberately does not carry it: it is only reachable through the gear, which
  already explained itself on the way in.
- **Five i18n keys, four of them settled roots copied rather than authored.**
  `label.hoverHelp`, `ui.on`, `ui.off` and `aria.helpToggle` take the French
  glossary roots verbatim from `scripts/i18n-fr-glossary.js` — *Aide au survol /
  Marche / Arrêt / Activer ou désactiver l'aide au survol*. The fifth,
  `tip.tipsToggle`, is the tooltip's own title and body.

### Measured

- **The second row costs nothing.** With the popover forced open it occupies
  **y 37..103.59, 190 x 66.59 px — byte-identical in English and French** —
  inside a 900 x 600 frame. The switch face grows 42.00 -> 45.30 px for
  *Marche*, leftward into slack the panel already had; `check-ui-labels` [7]
  reports **0 non-label elements displaced**.
- Every declaration in `.settings-toggle` above the four switch-specific ones is
  **copied from this page's own language `<select>`** — its font stack, ink,
  plate, hairline and radius — so the two controls in the popover match by
  construction rather than by a second designer re-deciding them.

### Also driven

- `tests/ui_tip_render_check.js` [1] **replaces a proxy with the property it
  stood for.** Until this version there was exactly one `.settings-row`, and the
  gate asserted that count as a stand-in for *closest() is right by
  construction, not by luck*. There are two rows now. The count is no longer the
  question: the binding resolves `#lang-select` by its unique id and then walks
  ANCESTORS with `closest()`, which cannot reach a sibling row however many
  exist. What must hold — and is now asserted directly — is that the resolved
  wrapper contains `#lang-select` and does NOT contain `#tips-toggle`.

### Decided

- **Default is ON.** v1.3.1 showed hover help unconditionally, so ON is the
  setting that leaves an existing user's plugin behaving exactly as it did.
  Default OFF would additionally have made `boot-all-uis --strict-tips` and
  `tests/ui_tip_render_check.js` measure an empty tip surface and call it
  correct.

### Not touched

- **The tuning tab is a standing open defect and is untouched here.** This
  version changes the settings popover, the tooltip renderer and the i18n table,
  and nothing else.

## v1.3.1 (2026-08-31)

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **42 of the 90 French entries revised** against the suite glossary (`scripts/i18n-fr-glossary.js`) and lint, which goes from 53 findings to 0 under `--strict`. Two captions move to the settled suite term — `Amortis.` → **Amort.** and `Réinject.` → **Réinjection**; 33 entries gain French typography (59 no-break spaces before `%`, `:`, `;` and between every number and its unit — `800 Hz`, `−60,0 à +12,0 dB`, `0,1 mm`, `16 voix`); and nine entries are corrected for grammar, idiom or meaning. The English copy, the keys, the bindings and the exemptions are byte-identical, and `reviewed: false` still stands on every entry — no native speaker has read this yet.
- **One control had two French names.** The polyphony dropdown's caption read *Mode polyphonique* while its own tooltip and the Max Voices tooltip both said *Mode de polyphonie*; the caption now matches. The mirror case went the other way — the Polyphony Mode tooltip pointed at *Voix maximales* where the knob it names reads **Voix max**, so the tooltip moved.
- **Four French sentences said less than their English.** The Embouchure tooltip had dropped the reed's *rest* opening and hung a gérondif off the wrong subject; the Flutter Tongue tooltip agreed a pronoun to the wrong noun; the Reed Mass, Reed Damping and Air Noise tooltips carried calques (*résonne sur sa propre fréquence*, *résonner à côté de la perce*, *mêlé au niveau de l'anche*) where French has a plain form.
- **Six entries carry a `termNote`** — a recorded, reasoned exemption rather than silence. *Anche dble* and *Flatt.* are width: the settled *Anche double* (67.20 px) and *Flatterzunge* (67.36 px) sit 0.80 and 0.64 px inside a 68.00 px caption cap that truncates silently. *Vib Prof.* / *Vib Vit.* are geometry: the glossary form measured 50.08 px and the label-collision gate failed on it. *Tenue inf.* / *Tenue infinie* are meaning: O-Reed has no ADSR, so the envelope term *Maintien* would name a control that does not exist.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so assistive technology reads the page in the language it is displayed in.

## v1.3.0 (2026-08-30)

### Added

- **Hover-help, in both languages.** Every control on the page now explains itself on hover: 35 tooltips — 33 of the plugin's 35 parameters plus the gear button and the language selector — each with an English and a French title and body, switched by the same gear popover the labels use. The bodies were written against the DSP that implements each control (`ReedModel.h`, `BoreWaveguide.h`, `BreathEnvelope.h`, `BreathNoise.h`, `MouthpieceChamber.h`, `ReedWindVoice.cpp`) rather than from the parameter names, and each ends with the control's range and unit.
- **The renderer that paints them.** Canon v2's `applyI18n()` writes `data-tip-title` and `data-tip` onto the anchors and stops there; the thing that reads those attributes and paints a surface is per-plugin code, and this page had none. Authoring the copy without it would have shipped **35 invisible strings past three green gates** — `check-i18n` only counts bindings, `check-ui-labels` has no tooltip awareness at all, and `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. A single `#tooltip` surface, its CSS in this page's own naturalist plate, and a delegated cursor-following handler ported from O-simpleFM land in the same release as the table.
- `tests/ui_tip_render_check.js` — 523 assertions at the real 900×600 frame against the real page. It resolves every binding including its `closest()` walk, hovers a **descendant** of each of the 35 anchors in both languages, byte-compares the rendered title and body against the table, and measures the tip rectangle on all four edges. Three negative controls: an over-long planted body (1987px tall in a 600px frame) that assertion 4 must report; the focus latch driven **both halves separately**; and the `pointerout` child-boundary rule.
- `.planning/params.tsv` — the runtime parameter inventory, produced by walking `AudioProcessor::getParameters()` on a constructed processor rather than by a regex over `createParameterLayout()`. `CMakeLists.txt` gains the `OUARICON_BUILD_TESTS` option and the `ouaricon_add_param_dump()` call behind it; a normal build is unaffected.

### Fixed

- **The unconditional focus arm would have parked a tooltip on screen after every click.** A mouse click on a `<button>` focuses it, so the reference renderer's `focusin` rule re-opens the tip that `pointerdown` has just hidden — and it sits on top of whatever the click opened. Measured here with the guard clause removed: clicking the gear pinned its own tip at `[603, 45, 260×137]` **across the settings popover the click had just opened**, overlapping it by **5280 px²**. The renderer carries an explicit last-input-device latch instead. `:focus-visible` is deliberately not the discriminator — Chromium reports it false for a programmatic `.focus()` after a click, so a gate driving focus directly would measure "no tip" and record that as correct.

### Changed

- `Source/PluginProcessor.cpp` moves `#include "PluginEditor.h"` behind `#if JUCE_WEB_BROWSER`, directly above `createEditor()`, with a `GenericAudioProcessorEditor` fallback. The param-dump console target builds with `JUCE_WEB_BROWSER=0` and does not compile the editor TU, so a top-of-file include breaks the link. Under a normal build `JUCE_WEB_BROWSER=1` and behaviour is byte-identical to v1.2.0.
- Three comments in `index.html` that said this plugin has no hover-help are rewritten, because they no longer do. The settings popover still holds the language selector alone and `tip.gearBtn`'s body says exactly that.

### Not changed, deliberately

- **`instrumentPreset` does not change the sound, and its tooltip says so.** `pInstrumentPreset` is fetched in `ReedWindVoice.cpp:50` and is never `load()`ed anywhere in `Source/` — the parameter has no consumer, so choosing an instrument in the dropdown moves the automation lane and nothing else. Wiring it is an audio change, not a hover-help one. A tip that lied about it would be worse than no tip, so the body records what the control actually does.
- **The reported latency does not follow Oversampling, and its tooltip says so.** `setLatencySamples()` is called once in `prepareToPlay()` from the default 2× oversampler (`PluginProcessor.cpp:392-394`) and is never re-reported when the control changes, so selecting 4× leaves the host compensating by the 2× figure.
- **Two of the 35 parameters get no tooltip, because they have no control on this page.** `referencePitch` is driven by `#ref-pitch-knob` inside the **shared** `scala-tuning-engine` tuning panel, which is lazy-mounted and absent from the DOM when `applyI18n()` first runs; binding it would log a `tip target not found` warning on every load, and adding an anchor to the module is a cross-plugin edit that `/module-upgrade` would revert. `tuningSystem` has no control anywhere — `bindComboBox('tuningSystem', 'tuningSystem')` resolves to `null` and warns on every load. Both are host-reachable and automatable; neither is page-reachable. Adding a control to satisfy the count is a feature change with a geometry cost.
- **Option strings stay English inside the French bodies.** The six `<select>` elements are empty in the markup and are filled at runtime from the host's own choice properties, so `Simple`, `Multi-segment`, `Lip`, `Breath`, `Throat`, `Monophonic`, `Polyphonic`, `2x`, `4x`, `Off` and `On` are what the user reads 20px from the tooltip. The prose around them is French; the tokens are not, because a translated token would name a value that appears nowhere on the control. Where a body names another *knob*, it uses that knob's localized caption, because those are keyed and the page does show the French.
- **Tooltip titles are the parameter's full name, not the page's caption.** All 27 knob captions were cut to fit the 68px box measured in v1.2.0 — `Reed Hard.`, `Inf. Sustain`, `Rev. Bore` each end in a truncating period — and a 260px tooltip is exactly where the full word belongs. It is also the name the host's automation lane shows.
- All 35 French tooltips are machine drafts flagged `reviewed: false`, joining the 55 label entries already waiting. No native speaker has read any of it.

## v1.2.0 (2026-08-29)

### Added

- **The PAGE speaks French.** Every visible string in the O-Reed UI is now a keyed label with an English and a French face, switched from a gear popover in the header bar. 55 label entries across 52 keyed elements and 3 keyed accessible names. **This release does NOT add hover-help copy** — v1.1.0 had none, and authoring it is a separate, later piece of work.
- **A UI-language preference that persists.** `uiLanguage` rides the APVTS state tree as a non-parameter property, written as a readable `"en"` / `"fr"` string in `getStateInformation()` and read back in `setStateInformation()` AFTER the preset manager has replaced the tree. Deliberately not an `AudioParameterChoice`: it must not appear in a DAW automation lane, and a preset must not be able to change which language somebody reads their plugin in. The guard is `isVoid()`, not `isBool()` — the ValueTree/XML round-trip rebuilds every property as a `var` over the attribute STRING and does not preserve the type.
- `Resources/ui/js/i18n.js`, embedded in `juce_add_binary_data` SOURCES and served from a `getResource()` branch in the same commit. A file that is one without the other is a 404 that presents as a missing panel and nothing else.
- `tests/i18n-states.json`, so the label gate can reach the 34 captions that live inside collapsed sections and the inactive FX tab.

### Fixed

- **Two knob captions were being silently ellipsised in ENGLISH.** `.knob-label` is `nowrap` + `overflow: hidden` + `text-overflow: ellipsis` capped by `.knob-control`'s 60px width, and "Embouchure" (61.44px) and "Double Reed" (60.58px) both overran it — rendering "EMBOUCHUR…" and "DOUBLE REE…" since v1.0.0. The box is now 68px. Every `.param-row` is `flex-wrap: wrap` in an 854px content box and the widest row holds seven knobs (500px), so no row rewraps and no row changes height; the knobs sit 8px further apart.
- **A broken `<script>` tag threw on every page load.** `index.html` carried a CLASSIC `<script src="/js/juce/index.js">` alongside the ES-module `import` of the same file. `js/juce/index.js` is a module, so the classic load threw `SyntaxError: Unexpected token 'export'` before defining anything, on every open of the editor, in the shipping plugin. It was a no-op that only produced an error; the tag is deleted and the module import — which is what actually provides `Juce` — is untouched. `check_native_interop.js` is a genuine classic script and is unchanged.
- **The FX tab's placeholder hugged the left edge of a 900px tab.** `.effects-placeholder` carries `align-items: center; justify-content: center; height: 100%` but had no width, so as a flex item of a `flex-start` row it shrink-wrapped its widest child instead of filling and centring its panel. `width: 100%` was added. **"Coming Soon" and its sentence now sit in the middle of the FX tab rather than at its left edge** — a visible change, on a placeholder tab with no other content.

### Changed

- The `Bore:` / `Reed:` readouts under the XY pad are split into a caption span and a value span (the caption is translated, the number is not). The caption span is pinned to 31px so the two readouts land at a language-invariant x; in English they move 9.41px and 8.00px right, into a row with roughly 570px of unused width, and they now line up with each other.
- `.toggle-control` is pinned to 126px so `DUAL BORE` -> `DOUBLE PERCE` cannot resize the control's own box. The track and the caption are left-aligned inside it and neither moves in either language.
- The tuning-panel load-failure notice is built with `createElement` + `setLabel` instead of an `innerHTML` string, so it is localized like everything else and no markup path can carry a translated string.
- The header bar gains a 22px gear button at its right-hand end; the tab group and the Ouaricon wordmark move 30px left.

### Not changed, deliberately

- **The fifteen XY-pad instrument markers stay in English.** They are the abbreviation set of the `instrumentPreset` `AudioParameterChoice`, and three of them — `Oboe`, `Suona`, `Piri` — are byte-identical to their option strings. The pad must name the same instruments the host automation lane and the dropdown beside it name, so the set is not split across two languages. A French user still reads `Oboe` and `E.Hrn` there. Recorded as an `I18N_EXEMPT` entry with that reason.
- **The Tuning tab stays in English.** Every caption in it belongs to the shared `modules/tuning/scala-tuning-engine` module, which is referenced by path rather than copied; localizing it is a cross-plugin change and a local edit would be reverted by `/module-upgrade`.
- **`CMakeLists.txt` still declares `PLUGIN_VERSION`, which JUCE ignores.** It is not a `juce_add_plugin` keyword — JUCE reads `VERSION` — so the plugin reports the project version to the host no matter what that line says. The number is bumped to 1.2.0 in place and the defect is left standing: correcting it is a host-visible change that several plugins share and it needs one decision across all of them.

### Technical Notes

- **All French is a machine draft.** Every entry is flagged `reviewed: false`; no native speaker has read any of it.
- **Four geometry cliffs, each invisible to the assertion that catches the others**, are documented with their measured numbers in the header of `Resources/ui/js/i18n.js`: the ellipsising `.knob-label`, the centre-growing XY axis caption in a pad full of markers, the `max-height: 0` collapse boxes whose clipped children still hold their rectangles, and `.section-content`'s max-height ceiling. Every pin was reverted alone and confirmed to re-break its gate; none is decoration.
- Twelve of the 27 knob captions are SHORTER in French than in English. A clip-only check would have certified all twelve.
- **Files:** `Resources/ui/index.html`, `Resources/ui/js/i18n.js` (new), `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `CMakeLists.txt`, `tests/i18n-states.json` (new).
- **Version:** 1.1.0 -> 1.2.0 (MINOR — new user-visible feature, backward compatible; a pre-1.2.0 session has no `uiLanguage` property and defaults to English).

## v1.1.0 (2026-04-26)

### Added

- **adds VST3 Note Expression microtonal support for Dorico.** O-Reed responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling microtonal playback of quarter-tones and arbitrary tuning deltas authored in Dorico's tonality system. End users must set Microtonality to "VST3 Note Expression" on the assigned expression map (see O-Lyrica 2.3.0 for procedure).
- **Shared `note-expression` module adoption.** O-Reed consumes the Ouaricon module at `modules/tuning/note-expression` (v1.0.0), same shape as O-Lyrica v2.3.0 / O-Bells v4.1.0 / O-Prism v1.17.0 / O-Wind v1.16.0 / O-IntonationPad v2.8.0.

### Technical Notes

- **First MPE consumer of the shared note-expression module.** `ReedWindVoice` extends `juce::MPESynthesiserVoice` and reads MIDI pitch via `getCurrentlyPlayingNote().initialNote` from `noteStarted()` (no parameter form). Pattern 1 (`noteId` correlation in the shared module's `updatePendingFromEvents`) holds regardless of MPE channel.
- **Helper-based composition (single source of truth).** `applyPendingTuning` is invoked INSIDE `getBaseFrequencyFromTuning(midiNote)`, so all three call sites — `noteStarted()` legato (line 141), `noteStarted()` normal (line 202), `notePitchbendChanged()` (line 374) — inherit the NE delta with one insertion. `exchange(0.0)` consume semantics correct for one-NE-per-noteOn delivery: first call in a block consumes the slot; subsequent calls return base unchanged (NE applies once per noteStarted, MPE pitch-bend updates per-block on top).
- **Composition order:** tuning engine → NE delta → MPE pitch-bend → `bore.setFrequency(freq)` (bore waveguide period derived from the final tuned frequency; Pattern 2 satisfied — first sample at tuned pitch).
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/ReedWindVoice.{h,cpp}`, `CMakeLists.txt` (added `PLUGIN_VERSION "1.1.0"` line + `ouaricon_add_module(O-Reed note-expression)`).
- **Version:** 1.0.12 → 1.1.0 (MINOR — new user-visible feature, backward compatible, no preset impact).

## v1.0.12 (2026-04-26)

### Fixed

- **Parameter-sensitive parasitic high-frequency oscillation** -- with v1.0.11 bore delay math correct (1046 Hz request -> 1062 Hz output, ~26 cents sharp), the reed-bore loop still locked to a wrong high-frequency mode at certain reed parameter combinations. Symptom: "very high-pitched tones that don't correspond with MIDI notes" reproducible by sweeping `reedHardness`/`reedMass`/`reedDamping`. Root cause: when `reedMass` is high and `reedDamping` low, reed natural frequency `f_n = sqrt(k_eff/mu_r)/2pi` falls into the audible range (1-3 kHz at extremes: e.g. `mu_r=0.06, k_eff=20e6 -> f_n=2907 Hz, Q=2.19`). With `Q = sqrt(k_eff*mu_r)/g_eff > 1`, the reed acts as a sharp bandpass resonator that dominates the bore feedback signal, locking output to `f_n` regardless of MIDI note. Real clarinets avoid this via heavy lip loading (effective reed Q ~0.5-1.5). Fix: enforce a `Q_MAX = 0.5` cap on the reed by raising `g_eff` to `sqrt(k_eff*mu_r)/Q_MAX` whenever the natural Q would exceed the ceiling. Single-line physical fix in `ReedModel::processSample()` after embouchure-effective parameters are computed; preserves natural-frequency variation (timbral character intact) while preventing reed from becoming a high-Q resonator that hijacks the bore loop. Verified across reed param sweep (`reedHardness`/`reedMass`/`reedDamping` in {0, 0.5, 1}) at MIDI 36/60/84 -- output dominant frequency now tracks bore setpoint at all combos. v1.0.11 pitch accuracy preserved.

## v1.0.11 (2026-04-16)

### Fixed

- **Notes play flat — pitch progressively wrong with higher MIDI notes** -- bore waveguide delay compensation only subtracted 1 sample (for the internal `prevBellReflection` storage) but the voice loop has a SECOND single-sample latch via `prevBoreMinus` in `ReedWindVoice` (lines 516, 542). The reed reads `p_bore_minus` from the previous iteration, so the actual round-trip delay = `4*halfDelay + 2 + bellPD + viscPD` while the code compensated for `4*halfDelay + 1 + bellPD + viscPD`. This made every note flat by a `D/(D+1)` ratio: ~17 cents at A4, ~33 cents at A5, ~67 cents at A6, >semitone at C8. Fix: change `compensatedDelay = totalDelay - viscPD - bellPD - 1.0f` to `- 2.0f` in `BoreWaveguide::setFrequency()` to compensate both storage samples (one inside the bore, one in the external feedback latch).

## v1.0.10 (2026-04-11)

### Fixed

- **No sound / silent output on default patch** -- `toneHoleCutoff` parameter defaulted to 1500 Hz instead of 8000 Hz, which opened 3 of 4 tone holes fully (scatter=-0.165 each) and 1 partially (scatter=-0.058). Each open tone hole junction radiates ~16.5% of the sum of forward and backward bore wave energy. With 4 active scattering junctions, the bore lost ~70% of its energy per round trip -- far exceeding the reed model's ability to sustain self-oscillation (reed gain ~1-3% per round trip). Fix: changed default `toneHoleCutoff` from 1500 Hz to 8000 Hz (all holes closed). Root cause: the toneHoleCutoff parameter controls progressive tone hole opening from bell-end first; at 1500 Hz (`cutoff_norm=0.167`), holes 1-3 were fully open (`opening=1.0`) and hole 4 was 33% open.

## v1.0.9 (2026-04-09)

### Fixed

- **Some notes silent, others produce glitchy high-pitched clipping tone** -- bore waveguide delay compensation computed filter group delays at DC instead of at the playing frequency. The DC formula (`sr / 2π*fc`) overestimates the viscothermal filter's group delay by up to 20x at high frequencies (e.g., 22.9 samples at DC vs 1.4 samples at 2kHz for a narrow bore). Combined with the bell allpass being hardcoded at 0.5 samples (actual: 5-30 samples depending on frequency), the `compensatedDelay` could go negative for many note/parameter combinations. When negative, all 5 bore segments clamped to the 2-sample minimum, producing a fixed ~4.4kHz parasitic tone regardless of MIDI note — or preventing self-oscillation entirely (silence). Three fixes:
  1. **Frequency-dependent viscothermal group delay**: replaced DC approximation with exact one-pole formula `GD(f) = p*(cos(ω)-p) / (1-2p*cos(ω)+p²)` evaluated at the target frequency. Stored filter pole coefficient from `updateParams()` for use in `setFrequency()`.
  2. **Frequency-dependent bell allpass group delay**: replaced static `bellGD = 0.5` with exact first-order allpass formula `GD(f) = (1-a²) / (1+a²-2a*cos(ω))`. Stored allpass coefficient similarly.
  3. **Safety clamp on compensatedDelay**: added `std::max(4.0f, compensatedDelay)` before segment division, matching O-Wind/O-Bowed pattern. Prevents negative delays from reaching Thiran interpolation. Reduced per-segment minimum from 2.0 to 1.0 for better high-frequency resolution.

## v1.0.8 (2026-04-08)

### Fixed

- **Voices not clearing after note-off (4-28s delay)** -- bore waveguide energy decayed too slowly for timely voice cleanup. With `feedbackGain=1.0` and viscothermal `g=0.995` (0.5% loss/round-trip), the bore ring-down took ~4s at 440Hz and ~28s at C2 (65Hz). Voices stayed allocated the entire time, eating polyphony and wasting CPU. Two fixes:
  1. **Post-release bore damping**: once breath envelope reaches Off state, apply exponential damping (~50ms time constant) to the bore feedback path. This drains bore energy in ~500ms regardless of pitch, while preserving the natural 150ms breath release tail.
  2. **Energy estimate tracks bore wave directly**: changed `energyEstimate` from tracking `|lastRadiatedOutput|` (highpass-filtered at ~3400Hz) to `|seg_bwd[0]|` (full-spectrum backward wave at reed). The radiation highpass severely underreported bore energy for low notes where most energy is below the cutoff. Energy estimate now reflects actual bore content.

## v1.0.7 (2026-04-08)

### Fixed

- **Continuous tone at -12 dB after note-off** -- reed model self-excited at zero mouth pressure due to Bernoulli flow through the full rest opening (H_eff ≈ 0.54mm). With Z_c ≈ 1.08e6 and S_opening ≈ 5.4e-6 m², the reed junction reflection gain exceeded 1.0 for bore pressures below ~14 Pa, creating a negative resistance that sustained oscillation indefinitely. Root cause: `dp = -p_bore_minus` drives flow through the open reed channel even without breath, and `Z_c * u_reed` exceeds the incoming wave amplitude at low levels. Fix: gate the reed opening by mouth pressure (threshold = 1% of closure pressure) so the reed rests closed against the mouthpiece when breath is off, matching physical behavior. The bore now rings down naturally through viscothermal loss (~0.5%/round-trip).

## v1.0.6 (2026-04-07)

### Fixed

- **Silent output from voice DSP** -- four interacting bugs prevented the reed model from producing audible sound:
  1. **Reed force unit mismatch**: `dp * A_reed` used total force (N) but stiffness/damping/mass params are per-unit-area. Removed `A_reed` multiplier so pressure (Pa) drives the reed directly.
  2. **Reed force sign inversion**: `+dp` opened the reed under pressure (wrong). Changed to `-dp` so positive mouth pressure closes the reed, enabling negative-resistance self-oscillation.
  3. **Numerical instability**: explicit Euler integration diverged because damping rate (~270 kHz) >> sample rate (~88 kHz). Replaced with semi-implicit Euler (implicit damping) for unconditional stability.
  4. **Startup deadlock**: hard reed closure (`S_opening = max(x+H, 0)`) gave zero flow when closed, preventing bore excitation. Added 0.5% soft minimum opening for startup leakage.
- **Mouth pressure overdriving reed**: fixed 12000 Pa scaling (3x the reed's closure pressure) which permanently overblew the reed. Now scales to `reedModel.getClosurePressure()` so the reed operates in the self-oscillation regime at any parameter setting.
- **Output normalization**: removed impedance-based normalization (`1/2.67e6`) that crushed signal by ~5000x. Replaced with `1/500` matching O-Wind/O-Bowed approach.
- **Radiation filter killing fundamental**: output was taken from highpass-filtered bore radiation (3400 Hz cutoff), attenuating the fundamental by ~22 dB. Now uses bore pressure at reed (full spectrum), consistent with other physical model plugins.
- **Turbulence seeding**: added 2% breath-proportional noise to seed bore resonance during reed closure.

## v1.0.5 (2026-04-07)

### Fixed

- **Bore waveguide self-oscillation** -- removed `feedbackGain` compensation from `BoreWaveguide` that was introduced in v1.0.4. The formula `1 / max(0.5, viscGain * 0.98)` yielded ~1.026 feedback gain, giving a round-trip gain of ~1.02 (>1.0). This caused the bore to self-excite after any note, producing a continuous high-pitched tone at ~-12 dB that never decayed. The bell allpass filter has unity gain (no energy loss to compensate for), and the viscothermal filter's 0.5% loss is correct physical damping. Root cause: v1.0.4 feedbackGain overcompensation.

## v1.0.4 (2026-04-06)

### Improved

- **Bore feedback gain compensation** -- added `feedbackGain` to `BoreWaveguide` that compensates for cumulative viscothermal filter and bell reflection losses. Computed as `1 / max(0.5, viscGain * 0.98)` clamped to 2.0x, applied to the backward-traveling wave returning to the reed. Prevents energy decay from damping sustained tones, similar to O-Wind's mechanism.

## v1.0.3 (2026-04-06)

### Fixed

- **Mouthpiece chamber damping** -- increased Helmholtz resonator damping from 0.001 to 0.05, bringing Q from ~164 down to ~3. The original Q~164 produced an unrealistic 4.6 kHz spike that colored all patches using the chamber. New damping models realistic mouthpiece cavity losses.
- **Mouthpiece chamber bypassed by default** -- changed `mouthpieceVol` parameter default from 0.3 to 0.0 so the chamber is off unless explicitly enabled. All 24 factory presets updated to 0.0.

## v1.0.2 (2026-04-06)

### Fixed

- **Tone hole energy conservation** — scatter coefficient formula changed from `-holeStrength / (1 + holeStrength)` to `-2*holeStrength / (2 + holeStrength)` (normalized Kelly-Lochbaum). The old formula destroyed ~24% of wave energy at each three-port junction. Applies to all 4 tone holes and the register hole.

## v1.0.1 (2026-04-06)

### Fixed

- **Bell radiation output model** — replaced allpass-difference output tap (`th4_fwd + p_reflected`) with direct forward-wave tap through a first-order highpass radiation filter. The allpass difference cancelled ~99.6% of fundamental energy at 440 Hz. New model uses `radiationFilter.processSample(th4_fwd)` matching O-Wind's radiation approach. Bell allpass retained for reflection path only.

## v1.0.0 (2026-04-06)

Initial release of O-Reed -- physical modeling reed wind instrument synthesizer.

### Features

- **Physical Modeling Engine** -- Guillemain-style reed ODE with symplectic Euler integration, Bernoulli flow model with Psi confinement (single-to-double reed), mass-spring-damper reed dynamics
- **Conical Bore Waveguide** -- Strategy C dual delay-line bore with Thiran allpass interpolation, viscothermal loss modeling, bell radiation filter, scale-dependent smoothing
- **Breath Envelope** -- Attack/sustain/release with velocity-scaled chiff overshoot, configurable air noise mix
- **Mouthpiece Chamber** -- Helmholtz resonance simulation for pitch correction and spectral coloring
- **Tone Hole Model** -- Lattice lowpass filter simulating open tone holes, skewed frequency control (200-8000 Hz)
- **Expression System** -- Vibrato (lip/breath/throat sources), growl (vocal fold coupling), flutter tongue, subtone mode
- **Impossible Physics** -- Infinite sustain, reverse bore, dual bore (parallel waveguide drone), cross-modulation feedback path
- **Dual Bore Drone** -- Second parallel waveguide with +/-2400 cent pitch offset for arghul/launeddas/mijwiz drone effects
- **Legato Engine** -- Portamento with configurable glide time, voice stealing with energy-based cleanup
- **Oversampling** -- 2x (default) and 4x modes with JUCE dsp::Oversampling
- **Microtonal Tuning** -- Scala/TUN file support, MTS-ESP client, 12-TET, configurable reference pitch (220-880 Hz) via scala-tuning-engine module
- **MPE Support** -- Per-note pressure, slide, and glide via MPESynthesiser (16 voices)
- **35 Parameters** across 8 categories: Primary, Secondary, Advanced, Expressive, Impossible Physics, Tuning, Voice Config, Output
- **WebView UI** -- 3-tab layout (Instrument, Expression, Advanced) with collapsible sections, XY pad, tuning panel, 28 slider knobs, 6 comboboxes, 1 toggle
- **24 Factory Presets**:
  - Western (9): Bb Clarinet, Bass Clarinet, Alto Sax, Tenor Sax, Soprano Sax, Baritone Sax, Oboe, English Horn, Bassoon
  - Non-Western (9): Duduk, Shehnai, Suona, Hichiriki, Zurna, Piri, Arghul, Launeddas, Mijwiz
  - Sound Design (6): Glass Reed, Metal Wind, Impossible Bore, Breath Drone, Giant Clarinet, Micro Reed
- **Preset System** -- OuariconPresetManager with factory/user presets, save/load, navigation, FileChooser dialog

### Validation

- pluginval level 10 PASS (VST3 + AU)
- auval PASS (aumu ORed OuDv)
- Zero build errors, zero warnings (excluding JUCE/module upstream)

### Formats

- VST3
- AU (Audio Unit)
- Standalone
