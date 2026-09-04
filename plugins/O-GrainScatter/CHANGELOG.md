# Changelog

## [2.7.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `ogs.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 39..94, 170 x 55 px — byte-identical in English and French** — inside a 900 x 800 frame. The switch face measures **42.00 x 16.00 px in both languages**.
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

### Also driven

- **The renderer here is `js/app.js`, not the inline module the sibling plugins
  use**, so the `show()` gate, the `hideTip` publication and the guarded
  `initializeTipsToggle()` call all live in that file.
- `tests/ui_tip_render_check.js` [8] counts **three** chrome anchors, not two.


## [2.6.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout — the 38 hover-help entries and 49 captions drafted at v2.6.0 were read one by one against their English and against the suite glossary (`scripts/i18n-fr-glossary.js`), which did not exist when they were written.

### Changed
- **43 French entries revised** against the suite glossary and lint — 16 terminology, 21 typography, 5 grammar/register, 1 meaning, out of 87. The visible ones: **Dispersion → Étalement** for *Spread* and **Disp. az. / Disp. él. → Étal. az. / Étal. él.**, because the draft spent one French word on both *Spread* and *Scatter* while this page has a real Scatter (the spatial-mode option and the grain-canvas caption); **Inverse → Inversion** for *Reverse*; **Réinject. → Réinj.**, the glossary's listed abbreviation for *Feedback*; and typographic no-break spaces before `%` `:` `;` and between every number and its unit (*0 à 100 %*, *10 à 500 ms*, *20 kHz*), 51 in all.
- **Six sentences rewritten for French rather than for the lint.** `tip.scan` had dropped the control's own name (the English opens "Scan Position moves the read point"); `tip.doppler`'s "une sirène qui passe monte puis descend" was a garden path; `tip.spread`'s "Un peu estompe l'attaque" was a calque with no French subject; `tip.reverse` was missing a partitive ("mêlent grains"); `tip.syncMode` read "qu'une fois une division choisie"; `tip.feedback` carried a comma splice.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so assistive technology reads the page in the language it is displayed in.
- **`Grain Size` keeps the caption `Taille`**, with the measurement recorded on the entry as a `termNote`: both glossary forms — *Taille de grain* (76.94 px) and *Taille grain* (62.28 px) — wrap to two 8.80 px lines inside an 18.00 px content box, leaving 0.20 px against `check-ui-labels` assertion 4 on a page whose Windows metrics are unmeasured.

### Fixed
- Nothing behavioural. No English copy, key, tip binding, exemption, selector, CSS rule, parameter, preset or DSP path was touched, and `reviewed: false` stays on all 87 entries — that flag records a native speaker, and none has read this file yet.

### Testing
- `i18n-fr-lint --plugin O-GrainScatter --strict`: **52 findings → 0, exit 0** (17 T3, 15 T4, 7 T5, 5 T7, 8 G1 closed). Two `termNote` exemptions, both on Grain Size, both carrying their measured px.
- **Scope control, not a diff read:** both revisions imported as ES modules and compared field by field — **0 `en` values changed**, `TIP_BINDINGS` / `I18N_EXEMPT` / `LANGUAGES` byte-identical, no key added or removed, no `reviewed` or `sameAsEn` flag moved. All 51 U+00A0 land inside a French `t:`/`b:` value; none in a key, a selector, a comment or a `termNote`.
- `check-ui-labels`: **output structurally identical to the pre-change run** — 78 PASS, 0 FAIL, `[7]` no non-label element moved in either, same `[8b]` decoration counts of 65 / 27 / 59, same 900 x 800 scroll extent in both languages. The five renamed captions were measured in this page's own node first: Étalement 53.83 px (0.19 *narrower* than Dispersion), Inversion 49.63, Réinj. 29.39, Étal. az. 41.30, Étal. él. 41.17, all in 62.00 px cells whose min-content driver is the longest word.
- `tests/ui_tip_render_check.js`: **796 / 796**, unchanged, 38 anchors driven in `en → fr → en` with both feature gates opened through the page's own listeners. Per-anchor tip heights read before and after: **2 of 38 moved** — `scan_position` 108.5 → 123.9 px (the restored control name) and `spatial_width` 108.5 → 93.2 px — tallest tip unchanged at 139.3 px in an 800 px frame, 19 flips and 0 clamp-edge contacts in both languages, and 0 English tip heights changed.
- `check-i18n`: ALL CHECKS PASS, canon v2, 38 tips bound. `boot-all-uis`: **43 / 43 clean, 0 warn, 0 failed, 0 DEAD bindings**, and O-GrainScatter contributes none of the 19 late bindings in the suite.
- `auval -v aufx OuGS OuDv`: **AU VALIDATION SUCCEEDED**. The installed bundle was verified to carry a changed French **value** (`t: 'Étalement'`) with `LC_ALL=C grep -a`, and its `Info.plist` was read for the new version — `strings` splits on the multi-byte `é`, and a re-embedded binary-data blob can ship beside a stale `Info.plist`.

## [2.6.0] - 2026-08-30

Hover-help, in both languages. Every one of the 36 parameters now carries a tooltip, as do the gear and the language selector — 38 entries, English and French, drafted for review. **And the copy is not the whole change: this page had no way to paint a tooltip at all**, so a renderer and a surface land in the same commit.

### Added
- **38 `I18N` hover-help entries in `js/i18n.js`**, one per on-page control plus two for the chrome, each with an English and a French `{t, b}`. `I18N` and `TIP_BINDINGS` were both empty through v2.5.0, which was that version's correct state; `check-i18n` assertion 2 reported it as "0 tip(s) bound". It now reports **38**.
- **`setupTooltips()` in `js/app.js` and the `.tooltip` rules + `#tooltip` surface in `index.html`.** `applyI18n()` writes `data-tip-title` and `data-tip` onto the bound anchors and stops there; the code that *reads* those attributes and paints a surface is per-plugin, and this page had none. Authoring the copy alone would have shipped **38 invisible strings past three green gates** — `check-i18n` only counts bindings, `check-ui-labels` has no tooltip awareness at all, and `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. Ported from O-simpleFM's delegated renderer and styled in this plugin's own parchment palette.
- **`tests/ui_tip_render_check.js`** — the only gate in this repo that can see a *rendered* tooltip. 796 assertions at the shipping 900 x 800 frame, in `en → fr → en`.

### Changed
- **Tip titles are the PAGE's caption, not the parameter's name.** Twelve of the 36 differ, and six of those are abbreviations forced by the 62 px `.knob-container` cap (`Size Rnd`, `Traj Speed`, `Dist LPF`, and the French `Alé. haut.`, `Vit. traj.`, `PB dist.`). A 280 px tooltip has no such cap, so every abbreviated title's **body opens by naming the control in full** — the title matches what the user is looking at, the body says what it is.
- **All 36 units were recovered from the page's own formatters**, because all 36 rows of `.planning/params.tsv` carry an **empty `label` column**. `pctFormatter` (`app.js:249`) gives `%`, `grainSizeFormatter` (`:250`) and `spatialSmoothFormatter` (`:266`) give `ms`, `degreeFormatter` (`:259`) gives degrees, and `repeatsFormatter` (`:251`) gives a bare integer. Nothing was invented.
- **Option words stay English inside French sentences.** `Free`, `Hann`, `Scatter`, `Trajectory` and the rest are `AudioParameterChoice` options, exempt on the page under D-01 arm 1 so the host automation lane agrees character for character — a French sentence that renamed them would be an instruction the user cannot follow. The page already made this choice once, in `label.spatialHint` at v2.5.0.

### Fixed
- Nothing. No existing behaviour changed, and no parameter ID, range, type, default, preset or DSP path was touched.

### Testing
- `check-i18n --strict-v2`: canon v2, ALL CHECKS PASS; **`[2] 38 tip(s) bound`**, every key resolved, zero orphans, zero dangling. Repo-wide, 43 plugins, ALL CHECKS PASS.
- **`check-ui-labels` output is BYTE-IDENTICAL before and after this change** — same three states, same `moved=0`, same `[8b]` decoration counts of 65 / 27 / 59. That is the number that matters rather than the verdict: the O-Emulator defect was caught by an `[8b]` count moving 7 → 9 inside a *passing* run, and the focus latch below is why it does not move here.
- `boot-all-uis`: 43 / 43 clean, 0 warn, 0 failed. O-GrainScatter's row is unchanged — `text=75 aria=3 title=0 i18n=47` — confirming the hidden surface adds no text node, and repo-wide native `title=` stays at **0**.
- `tests/ui_tip_render_check.js`: **796 / 796**, 38 anchors driven in `en → fr → en`, every rendered title and body **byte-equal** to the table (not "contains"), every tip rect inside 900 x 800.
- **French grows 25 of the 36 tips** against the 280 px cap — `93 → 109` px on most, `109 → 139` on `pitch_random`, `pitch_mode`, `spatial_mode` and `spatial_smooth`. None shrank. So the French pass measures different boxes from the English one rather than repeating it.
- **19 of 36 tips place by FLIP and none needed the clamp**, on shipped copy — which is why the gate carries a *positive* control for the clamp as well as a negative one: a 616 px plant fits on neither side of the cursor, flips to `top = −180`, and is clamped to exactly **8.0 px**. Without the clamp-after-flip it would leave the frame (O-Bass's carried finding: one flip is not enough).
- **Negative control:** a 4800-character plant renders **280 x 1570.7 px in an 800 px frame** and assertion 4 reports the overflow; restored from the table and green again at the same anchor. Sized against *this* frame deliberately — O-Tremolo's habit-sized plant fitted inside a 400 px frame and reported nothing.
- **The focus latch, controlled both ways.** Deleting only `if (lastInputWasPointer) return;` — the declaration, the `pointerdown` write and the `keydown` clear all survive, so every static grep still matches — makes the gate report the gear's own tip **covering the settings popover by 3650 px²**, with the keyboard half still green. Deleting the gate's `activeElement.blur()` as well turns the suite back to **796 / 796**, which proves the blur line is load-bearing rather than tidying. Both plants restored from a namespaced scratchpad copy and checksum-verified — never `git checkout`, which is how this plugin lost a whole uncommitted edit in Stage K.
- All 38 French entries are machine drafts, every one `reviewed: false`. Scanned for a decimal point, an angle bracket and a missing space before `%`: zero of each.

### Notes
- **FOURTEEN OF THE 36 CONTROLS CANNOT BE HOVERED IN THE PLUGIN'S DEFAULT STATE, and that is shipped behaviour rather than a tooltip bug.** `setupPitchGate` (`app.js:326-345`) dims Scale, Root Note and Pitch Mode while `pitch_random` is below 0.01 — via `.dimmed`, which is `opacity: .25` **plus `pointer-events: none`** (`index.html:287`) — and `setupSpatialGate` (`:351-375`) sets `pointerEvents = 'none'` on the ten spatial knobs and the Trajectory dropdown while Spatial Mode is Off. Both are the defaults. The information is not lost: `#pitch-hint` and `#spatial-hint` are visible in exactly those states and say what to raise. The gating was **not changed to suit the tooltips** — the render gate drives the page out of both states through the page's own `valueChangedEvent` listeners, and pins the rest state as an assertion so a future change to either gate is visible.
- **`pointer-events: none` does not remove an element from the tab ring**, so the three gated `<select>`s still open their tip from the keyboard while being unreachable by mouse. Recorded, not fixed.
- **The 27 knobs are `<div>`s and are not focusable**, so the keyboard half of hover-help reaches the 7 dropdowns, the 2 toggles and the 2 chrome controls — 11 of 38. Adding `tabindex` would change the page's tab order, which is a feature decision rather than a Stage M side effect.
- **No hover-help on/off toggle**, matching the other 19 tooltip plugins in the suite; only O-Tapestop and O-Bitrot have one.
- No geometry pin was added, so no negative control is owed for one.


## [2.5.0] - 2026-08-29

The PAGE speaks French. Every visible caption on the UI is now localized, English or French, chosen from a gear popover and remembered with the session. No hover-help copy was authored — this plugin has none, and inventing it is a separate job.

### Added
- **`Source/ui/public/js/i18n.js`** — 49 keys: 47 visible captions and 2 accessible names. `I18N` and `TIP_BINDINGS` are both **empty**, which is this plugin's correct state: v2.4.4 carried zero `title=`, zero `aria-label`, zero `placeholder` and zero `data-tip` anywhere in `index.html`. `check-i18n` assertion 2 reports that as "0 tip(s) bound" rather than passing silently.
- **A settings popover carrying the language selector**, absolutely positioned in the header's empty right margin so that not one existing element moves to make room for it. Styled in this plugin's own visual system — Garamond, the `#8B7355` rules and `#FFF8DC` cream of the knobs, and the olive `rgba(107,142,78)` the freeze toggle already uses.
- **`getUiLanguage` / `setUiLanguage`** native functions and session persistence. `uiLanguage` is a `std::atomic<int>` behind a two-function string codec, written onto the APVTS state tree as a plain non-parameter property so it never appears in a DAW automation lane and no preset can change which language somebody reads their plugin in.
- **`plugins/O-GrainScatter/tests/i18n-states.json`** — two states for the label render gate, so all 47 captions are measured rather than 46.

### Changed
- **Canon v2 i18n runtime in `js/app.js`**, placed at the TOP of the module rather than the bottom. `init()` is the last statement in this file and is the page's only reader of `trLabel()`; a block placed below it would be a TDZ `ReferenceError` that takes the whole UI (`pattern_module_toplevel_init_tdz`). `initI18n()` is still called last, inside `init()`, guarded by try/catch so a throw there cannot cost the page a knob.
- **`#stutter-gate-btn { width: 110px }`** — a load-bearing pin, not decoration. The button is a `[data-i18n]` element so the geometry gate does not measure it, but its parent flex wrapper is not keyed and shrink-wraps it. 110 px is the English border box (109.95) rounded up: English moves 0.05 px, and the rectangle becomes language-invariant. Removed alone, the gate reports the wrapper at `dw=-16.9`.
- **`.settings-popover { width: 170px }`** — the same pin for the same reason, on the new panel: LANGUAGE → LANGUE *shrinks*, so an auto-width panel would contract in French. Removed alone, 2 elements move at `dx=14.6 dw=-14.6`.

### Testing
- `check-i18n --strict-v2`: 39 assertions, canon v2, ALL CHECKS PASS.
- `check-ui-labels`: all assertions pass in three states (default, settings popover open, pitch gate closed); 47 of 47 keyed elements measured; no page error; every resource served.
- `boot-all-uis`: 43 / 43 clean, 0 warn, 0 failed. O-GrainScatter reads text 72 → 75, aria 0 → 3, title 0 → 0, i18n 0 → 47.
- **English v2.4.4 → v2.5.0: zero of 192 elements moved** at the gate's 0.5 px tolerance. At 0.01 px exactly one moved, by `dw=+0.05`, and it is the stutter-gate wrapper taking the named pin. Nothing moved vertically; document scroll extent stays 900 x 800.
- **French vs English at v2.5.0: zero non-label elements moved.** Measured independently of the gate at 0.01 px over all 195 boxes — the only 32 rectangles that differ in the whole document are keyed captions whose own width changed, and 13 of those 32 are *narrower* in French.
- **The page holds still**: zero of 193 elements differ between a 180 ms and a 1.7 s settle, in either language, before or after the change, despite two `requestAnimationFrame` canvas loops. Canvas drawing does not enter `getBoundingClientRect`.
- Three geometry cliffs found by planting, each caught by an assertion blind to the other two: a caption overflowing its knob box into a neighbour (74 px, caught by `[8b]` only), one overflowing the 900 px frame from the last spatial knob (116 px, caught by `[5][6][8][8b]`), and one wrapping to a third line out of the fixed 18 px caption box (caught by `[4]` only, with zero `[7]`).
- Every pin was reverted alone and confirmed to re-break the gate. The one guard whose control passes — `.settings-label { white-space: nowrap }` — is labelled a guard, not claimed as a fix.
- No parameter IDs, ranges, types, defaults, presets or DSP behaviour changed.

### Notes
- **The 62 px knob container does NOT grow.** `.knob-container` carries an explicit `width: 62px`, so its automatic minimum size is clamped to that specified size and a caption wider than 62 px simply overflows it symmetrically, pushing nothing. This page therefore has only two shrink-wrapping push sites — the stutter-gate wrapper and the settings popover — and both are pinned.
- **`GrainScatterViz.draw()` paints `"<n> grains"` with `ctx.fillText`, and it stays English.** A 2D-context string is not a DOM node: the canon sweep cannot reach it and neither gate can see it. Recorded as a reasoned `I18N_EXEMPT` entry, matching O-Orbit, O-MultiBandCompressor and O-simpleSampler. Carried to Stage M.
- All 49 French strings are machine drafts, every entry `reviewed: false`. No native speaker has read them.

## [2.4.4] - 2026-08-19

UI layout fix. The Spatial Audio section was clipped by the bottom edge of the editor window; the control grid is now content-sized and the window is 50 px shorter.

### Fixed
- **Spatial Audio section clipped off the bottom of the editor:** at the 900 x 850 editor size the page content ran to y=884.5, so the last ~34 px — the bottom row of Spatial Audio knob readouts and the "Set Mode to Scatter or Trajectory to enable" hint — were cut off by `body { overflow: hidden }` and unreachable. *Root cause:* `.controls-area` used `grid-template-rows: 1fr 1fr`, which forces the second row (Beat Sync / Euclidean Rhythm) to match the height of the first (Core Engine / Pitch & Scale). Row 2 needs only 111 px of content but was rendered at 199.5 px, and that 88.5 px of dead space pushed Spatial Audio past the window. Changed to `grid-template-rows: auto auto` so each row sizes to its own content — Beat Sync and Euclidean Rhythm are now 112 px (56 % of the top row) and the dead space is gone.

### Changed
- **Editor height 850 -> 800 px** (`PluginEditor.cpp`): with the grid fix the natural content height is 797 px, so the window was shortened by 50 px rather than leaving the reclaimed space empty. Width is unchanged at 900 px. Measured content stack at 900x800: header 42 + viz 243 + freeze 33.5 + fleuron 16.5 + controls 331.5 + spatial 133.5 = 800 exactly, nothing clipped.
- **`.plugin-container` sizes from the editor instead of hard-coding 900x850** — now `width: 100%; height: 100%`, so `setSize()` in `PluginEditor.cpp` is the single source of truth for the window size and the CSS can't drift from it.
- **`.viz-area` absorbs the layout slack** — the fixed `height: 240px` became `flex: 1 1 auto; min-height: 200px`. The visualizations render at 243 px at the current size, and because the viz is the only flex-grow item, any font-metric difference between WKWebView/WebView2 and the layout-test engine is taken out of the viz height rather than clipping the bottom of the page.

### Testing
- Layout measured headlessly (Chromium, 900x800) against the shipped `index.html`: Spatial Audio bottom edge = 800.0, spatial hint bottom = 787.0, last knob readout bottom = 775.5 — zero overflow.
- Negative control: re-injecting the old `grid-template-rows: 1fr 1fr` at the new 800 px height puts the Spatial Audio bottom back at 884.5 (84.5 px clipped), confirming the grid change is what fixes the layout.
- No DSP, parameter, preset, or state changes — CSS and one `setSize()` call only.

### Notes
- This fix was authored as "2.4.3" on a branch cut before the 2.4.3 licensing release shipped; renumbered to 2.4.4 at merge time since 2.4.3 was already published.

## [2.4.3] - 2026-08-19

Licensing release — no audible or behavioral change.

### Changed
- Added AGPL-3.0 license notice headers to all Ouaricon-authored source files (repo relicensed to AGPL-3.0 on 2026-08-01; JUCE used under AGPLv3).

## [2.4.2] - 2026-07-09

Info-finding cleanup sweep (CODE_REVIEW.md v2.4.0 review, IN-* items — the 2 critical + 12 warning findings were resolved in v2.4.1). No audible or behavioral change; dead-code removal, a per-block micro-optimization, and defensive state resets.

### Changed
- **IN-01 — dead `FreezeManager::getCrossfadeGain()` removed:** the method was never called (the freeze crossfade is implicit — grains switch source at spawn and `advanceCrossfade` delays `active=false` by ~5 ms). Removed it and the now-write-only `crossfadeDirection` member. No behavior change.
- **IN-02 — dead `TempoTracker::lastPpq` removed:** written in both update branches, never read.
- **IN-05 — grain voices now cleared in `prepareToPlay`:** added `GrainPool::clearVoices()` and call it from `prepareToPlay` (previously voices were deactivated only in `reset()`), so a sample-rate/block-size change without a host `reset()` can't leave stale grains active. `reset()` now shares the same `clearVoices()` path.
- **IN-09 — HOA write pointers cached per block:** the spatial inner loop stored via `hoaBus.setSample(ch, i, …)` (a `getWritePointer` + bounds check per channel per sample); now caches the 16 write pointers once per block and indexes directly.
- **IN-10 — distance split-semantics documented:** added a comment noting grain gain uses the spawn-frozen `v.distance` while the distance-LPF uses the live per-block value (intended: per-grain gain snapshot, continuous filter tone).
- **IN-11 — `reset()` now resets `TempoTracker`:** `reset()` re-`prepare()`s the tempo tracker so the standalone `manualPpq` counter doesn't keep advancing across a transport stop/seek (completeness gap in "clear all DSP state").
- **IN-14 — dead `.dimmed-spatial` CSS rule removed:** the spatial gate dims via inline `style.opacity`/`pointerEvents`, never applying this class.
- **IN-15 — `timerCallback` early-returns when hidden:** the 30 Hz viz JSON (String allocations) was built every tick even when the WebView wasn't showing (the emit was already visibility-gated). Now skips construction entirely when `!webView->isShowing()`.

### Reviewed — no change needed
- **IN-03** (Euclidean generator is a rotation of canonical Bjorklund) — valid maximally-even pattern, not a bug.
- **IN-04** (`isEvenSubdiv` misnamed) — already resolved by the WR-02 scheduler rewrite (now `isOffBeat`).
- **IN-06** (harden `repeatIntervalSamples` divide) — already done in v2.4.1 alongside WR-09 (`jmax(1.0, bpm)`).
- **IN-07** (repeat grains re-trigger at future subdivisions) — intended stutter/repeat-burst behavior (a shipped core feature); left as-is.
- **IN-08** (grain envelope never reaches phase 1.0) — ~5e-7 error at typical grain lengths; changing the phase formula would alter the grain sound for no audible benefit.
- **IN-12** (`releaseResources()` empty) — acceptable; JUCE re-`prepareToPlay`s before reuse.
- **IN-13** (Doppler uses smoothed SH `current[1]` as previous-azimuth proxy) — documented heuristic, not a defect.

## [2.4.1] - 2026-07-08

Code-review resolution pass (CODE_REVIEW.md, v2.4.0 deep review). All 2 critical + 12 warning findings fixed.

### Fixed
- **CR-01 — dead "Scan" knob:** `scan_position` (the v2.4.0 flagship control) had a param, DSP, DOM knob, and JS binding but **no editor relay/attachment**, so it was uncontrollable from the UI *and* host automation. Added the `WebSliderRelay` + `.withOptionsFrom` + `WebSliderParameterAttachment` triplet. *Root cause:* the relay/attachment pair was never added when the param was introduced.
- **CR-02 — RT reallocation in `reset()`:** `reset()` called `delayBuffer.prepare()` and `freezeManager.prepare()`, which `setSize()` the 2 s delay + freeze buffers (~6 MB of free/alloc) on a thread hosts may run in real time. Added alloc-free `clear()` methods (zero the already-sized buffers, no `setSize`) and call those instead.
- **WR-01 — spatial-mode feedback was a block-held constant:** in Scatter/Trajectory mode `feedbackL/R` were updated only in the post-decode loop, so every input sample of a block was fed the *same*, one-block-late feedback value → DC offset + block-rate buzz. Feedback is now derived **per-sample inside the main loop** from the HOA omni (W) channel — a true per-sample recursion. (Spatial feedback is now mono; the grains re-spatialize it on the next pass.)
- **WR-02 — swing dropped every off-beat + desynced Euclidean:** the swing gate reused the straight-boundary crossing and could only *reject*, so swung (odd) subdivisions were never spawned, and `euclideanStep` advanced only past the gate → pattern drift. Rewrote the scheduler to detect each division's **own** trigger time (straight for on-beats, swing-offset for off-beats) and advance the Euclidean step once per division, in order → phase-locked. Straight-grid behaviour (swing = 50 %) is bit-identical to before.
- **WR-03 — freeze-engage click/xrun:** `engage()` copied up to ~176 k samples element-by-element (`getSample`/`setSample` + modulo) on the audio thread. Replaced with two contiguous `AudioBuffer::copyFrom` memcpys spanning the ring wrap.
- **WR-04 — `spawnRequests` could exceed its `reserve(128)`** on large/offline blocks → audio-thread realloc. Added a hard `kMaxSpawnsPerBlock = 128` cap at every push site (free + sync + repeats).
- **WR-05 — no NaN/Inf guard on recursive state:** a single non-finite sample could latch `feedbackL/R` or `distanceLpfState` to NaN → permanent silence. Added `isfinite` flush-to-zero on both feedback paths and both LPF states.
- **WR-06 — per-sample SH trig in Trajectory mode:** `encodeSH16` (16-coeff trig) ran for every active voice every sample. It now updates the SH *target* only at a 16-sample control rate; the existing one-pole SH smoother interpolates between updates (trajectory position + Doppler stay per-sample, unchanged).
- **WR-07 — no block-size clamp:** `hoaBus`/`binaural` buffers are sized to `samplesPerBlock`; added a `jassert` + defensive `jmin` clamp on `numSamples` so an over-sized host block can't write past the allocations (the class v2.0.2 fixed).
- **WR-08 — distance-LPF zipper:** the cutoff coefficient was recomputed per block with no smoothing → stepping on Distance / Distance-LPF automation. Wrapped it in a per-sample `SmoothedValue`.
- **WR-09 — `bpm <= 0` silently stopped Sync scheduling:** `TempoTracker` took a host-reported non-positive BPM verbatim (ppqPerSample = 0 → no subdivision crossings). It now falls through to the 120 BPM fallback; the repeat-interval divide is additionally hardened with `jmax(1.0, bpm)` (IN-06).
- **WR-10 — `spatial_smooth` reset default wrong:** the hardcoded JS normalized default `0.1` ignored the 0.4 skew (reset snapped to ≈1.6 ms instead of 5 ms). Now sourced from C++ (see WR-11).
- **WR-11 — JS re-implemented C++ ranges/skew:** knob readouts and reset defaults duplicated each `NormalisableRange` (incl. skew) in JS — a latent drift class (WR-10 was the first crack). Readouts now use `state.getScaledValue()` and double-click resets pull skew-correct defaults from a new `getParameterDefaults` native function. No hand-coded ranges/defaults remain.
- **WR-12 — `CMakeLists` version drift:** `VERSION` was pinned at `2.1.0` (three minors behind the shipped 2.4.0 features) → binaries reported the wrong version. Bumped to `2.4.1`.

## [2.4.0] - 2026-03-09

### Added
- **Grain scan position** (`scan_position` 0-100%): sets the base grain read position in the delay buffer, mapping 0% (write head / most recent audio) to 100% (2 seconds back). Replaces the previous fixed `basePosition = grainSizeSamples` with user-controllable buffer scanning
- Spread parameter now scatters grains around the scan position instead of around `grainSizeSamples`
- In freeze mode, the full 2-second delay buffer is captured so scan position can sweep through the entire frozen buffer
- "Scan" knob in Core Engine UI group (between Density and Spread)

## [2.3.0] - 2026-03-08

### Added
- **Euclidean rotation** (`euclidean_rotation` 0-15): rotates the Euclidean pattern by reading `pattern[(step + rotation) % steps]`, shifting which pulses land on which subdivisions without regenerating the pattern
- **Swing** (`euclidean_swing` 50-75%): offsets even-numbered (off-beat) subdivision boundaries forward in time — 50% = straight, 75% = maximum shuffle
- "Rotation" and "Swing" knobs in Euclidean Rhythm UI group
- Euclidean circle visualization now reflects rotation offset: dots show the rotated pattern readout, dashed line indicates rotation origin, center label shows `r{N}` when rotation > 0

## [2.2.0] - 2026-03-08

### Added
- **Grain size randomization** (`size_random` 0-100%): each grain's duration is varied by `grainSize * (1.0 + random * sizeRandom)`, creating more organic, less mechanical grain textures
- **Per-grain amplitude randomization** (`amp_random` 0-100%): each grain's amplitude is scaled by `1.0 - random * ampRandom`, adding natural dynamic variation to the grain cloud
- Two new knobs ("Size Rnd", "Amp Rnd") in the Core Engine UI group
- Both parameters default to 0% (no change to existing behavior)

## [2.1.0] - 2026-03-08

### Added
- Grain envelope shape selection: new `grain_shape` parameter with 6 window types
  - **Hann** (default): smooth cosine bell — classic granular sound, zero at edges
  - **Triangle**: linear attack/decay — brighter, more percussive than Hann
  - **Trapezoid**: flat sustain (20-80%) with linear ramps — preserves transients
  - **Tukey** (α=0.5): cosine taper first/last 25%, flat middle — hybrid of Hann and rectangular
  - **Blackman**: narrower main lobe than Hann — reduced spectral leakage, darker tone
  - **Exp Decay**: exponential falloff — plucked/percussive character with sharp attack
- UI dropdown in Core Engine group for shape selection
- Visualization reflects selected envelope shape in real-time

## [2.0.5] - 2026-03-08

### Changed
- Removed dead code: `lastSubdivIndex` (GrainScheduler), `ppqJumped`/`didPpqJump` (TempoTracker), `scratchL`/`scratchR` (BinauralDecoder), `getActiveCount` (GrainPool), duplicate `probabilityFormatter` (app.js)
- Extracted shared `lagrangeInterpolate()` function (LagrangeInterpolation.h) used by DelayBuffer and FreezeManager — eliminates duplicated 3rd-order Lagrange interpolation code
- Consolidated duplicate degree formatters in app.js into `degreeFormatter(range, offset)` factory
- Extracted shared `resizeCanvas()` function for GrainScatterViz and EuclideanCircleViz — eliminates duplicated DPR-aware canvas sizing code
- Moved `setSpatialSmoothTime()` call from inside per-sample loop to once-per-block before the loop
- Named magic feedback constants: `kFeedbackDrive` (3.0), `kTanhCompensation` (1.00497), `kStabilityMargin` (0.95)
- Named distance attenuation constant: `kDistanceScale` (3.0) in GrainPool spatial processing
- Extracted `numHoaChannels` and `numChannels` local variables for HOA bus size expressions in BinauralDecoder and PluginProcessor

## [2.0.4] - 2026-03-08

### Fixed
- Thread safety: replaced visualization double-buffer with lock-free triple buffer to prevent torn reads when audio thread publishes faster than GUI consumes
- Thread safety: made `cachedEuclideanSteps`/`cachedEuclideanPulses` `std::atomic<int>` and moved euclidean pattern + step data into `GrainVizSnapshot` — GUI no longer holds a direct reference to audio-thread-owned `euclideanPattern` array
- Added `reset()` override to clear all DSP state (grain voices, delay buffer, feedback, freeze, scheduler, distance LPF, HOA bus) on transport stop/seek/loop — prevents stale audio artifacts after DAW transport jumps
- Root cause: double-buffer allowed audio thread to overwrite the slot GUI was reading mid-frame; euclidean data was exposed via raw `const&` across threads with no synchronization

## [2.0.3] - 2026-03-08

### Fixed
- Zipper noise on feedback/dry-wet automation in spatial mode: post-processing loop was reading raw `feedbackParam->load()` and `dryWetParam->load()` per-sample instead of using `feedbackSmoothed`/`dryWetSmoothed` SmoothedValue instances
- Root cause: stereo path correctly used SmoothedValues, but spatial post-processing loop bypassed them entirely
- Removed redundant SmoothedValue advancement in per-sample spatial branch; values now consumed in the post-processing loop where they're actually needed

## [2.0.2] - 2026-03-08

### Fixed
- Critical stack buffer overflow: replaced stack-allocated `binauralL/R[2048]` arrays with heap-allocated member buffers sized to actual `samplesPerBlock`
- Incorrect `hoaBus` and `binauralDecoder` sizing: was using `sampleRate * 0.02 + 1024` (arbitrary formula), now uses `samplesPerBlock` from host
- Root cause: `prepareToPlay` ignored its `samplesPerBlock` parameter entirely

## [2.0.1] - 2026-02-09

### Fixed
- Density parameter now uses exponential curve for perceptible control across full knob range
- Previously: 75% of knob range only varied from ~1 to ~4 grains/sec (linear interval mapping)
- Now: 50% knob = ~10 grains/sec, smooth exponential scaling from ~1/sec to ~100/sec

## [1.0.1] - 2026-02-07

### Improved
- Scale, Root Note, and Pitch Mode dropdowns now dim when Pitch Random is at 0%, with hint text "Increase Pitch Rnd to activate" — clarifies that pitch randomization must be active for scale controls to have effect

## [1.0.0] - 2026-02-07

### Added
- Granular scatter engine with 64-voice polyphonic grain pool
- Delay buffer with Lagrange 3rd-order interpolation for smooth pitched reads
- Free mode: density-controlled grain spawning (10ms to 1000ms intervals)
- Beat sync mode: 6 subdivision options (1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T)
- Euclidean rhythm patterns for gating grain triggers (2-16 steps, 1-16 pulses)
- Repeat burst system (1-16 repeats per trigger) with stutter gate
- Freeze: capture and loop audio with 5ms crossfade on engage and release
- 5 musical scales: Chromatic, Major, Minor, Pentatonic, Whole Tone
- 4 pitch modes: Random, Ladder Up, Ladder Down, Pendulum
- Scale quantizer with root note selection (C through B)
- Spread control for grain position scatter
- Pan randomization with equal-power panning law
- Reverse grain probability
- Feedback with soft-clipping (tanh) to prevent runaway
- Smoothed dry/wet mix crossfade
- Output soft-clipping to prevent digital clipping with many overlapping grains
- Standalone tempo tracker with 120 BPM fallback and DAW loop detection
- WebView UI with vintage Naturalist aesthetic (Garamond serif, parchment palette)
- Real-time grain scatter visualization (Canvas 2D, position vs pitch)
- Euclidean circle visualizer with polygon overlay and step indicator
- Freeze glow animation on toggle button
- Double-click knob reset to default values
- 18 automatable parameters across 4 groups (Core, Sync, Spread, Euclidean)
- Cross-platform WebView2 support with static linking for Windows
- State save/restore via XML serialization
