# Changelog

All notable changes to O-FreqPulse will be documented in this file.

## [1.18.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed
- **item 40 — hover-help toggle (`.settings-toggle`, `#tips-toggle`):** the button resized between its own two French faces — `min-width: 40px` (styles.css:924, border-box) covered *Off* / *On* (15.34 / 14.69 px of text) but not v1.18.1's *Arrêt* / *Marche* (25.52 / 36.97 px), so the button was content-sized at 43.52 px on *Arrêt* and 54.97 px on *Marche*, an 11.45 px jump every time hover help was toggled in French → `min-width: 57px`, the measured wider face plus 2.03 px slack. Every face now measures 57.00 px in both languages; the popover holds its 186 px and no non-label element moves. The CSS comment carries the four measured faces and the date.

### Verification

- Toggle-width probe (scratchpad, live node, 850 × 550, `Range.selectNodeContents` + `getBoundingClientRect`): **before** fr Arrêt 43.52 / Marche 54.97 (FAIL, Δ 11.45), en 40 / 40; **after** all four faces 57.00 (PASS), popover 186 before and after in both languages.
- `node scripts/check-ui-labels.js --plugin O-FreqPulse` — ALL CHECKS PASSED, 0 non-label elements moved between English and French, before and after, all six states.
- `node scripts/check-i18n.js --plugin O-FreqPulse` — ALL CHECKS PASS. `node scripts/i18n-fr-lint.js --plugin O-FreqPulse --strict` — exit 0 (no French changed).
- `node scripts/boot-all-uis.js` — 43/43, 0 DEAD, O-FreqPulse late count unchanged.
- `auval -v aufx OFPu OuDv` — PASS; installed `Info.plist` reads 1.18.2; `min-width: 57px` is in the installed bundle.

## [1.18.1] - 2026-08-31

PATCH: the French copy is revised — a second reading of all 67 machine drafts against the suite glossary and lint. No English copy, no keys, no bindings, no CSS, no audio-path, parameter or DSP changes.

### Changed

- **19 French entries revised** against `scripts/i18n-fr-glossary.js` and `scripts/i18n-fr-lint.js`: 9 terminology, 6 typography, 2 grammar/register, 2 straight-copy declarations (four entries carry more than one). The lint went **21 findings → 0**, `--strict` exit 0, with **no `termNote` exemption** — every settled term fitted on this page. The visible ones: **Mixage → Mix** on the footer caption, the four band-mix cells and two tip titles; **Oui / Non → Marche / Arrêt** on the hover-help switch (*Oui/Non* are answers, not states); **Aide → Aide au survol** in the settings popover, now the same string as that control's own tip title; **Enreg. → Enreg**, the period dropped so the caption is a substring of its accessible name `Enregistrer les réglages actuels` (WCAG 2.5.3 label-in-name) at zero geometry cost — the button is pinned at 50 px and measures 50.00 either way.
- **15 no-break spaces** (U+00A0) in French prose: six before `%`, seven before `;`, two between a number and its unit — *0 %*, *droite ;*, *0-500 ms*. French typography, and the one place a Stage N pass can lengthen an unbreakable run inside a 220 px tooltip. It did not: **all 56 tip anchors were re-measured before and after, in both languages, and not one tip box changed size or position.**
- **The settings tip lost a clause its English has and got it back**, and now uses the page's own imperative voice: *Choisir la langue… et l'affichage de l'aide au survol* → *Choisissez la langue… et l'affichage **ou non** de l'aide au survol*. Ten sibling bodies were already imperative (*Cliquez*, *Faites glisser*, *Choisissez*); this was the only infinitive.
- **One control, one French name.** `aria.solo` was *Isoler cette bande* while the caption and the tip title said *Solo*; it is *Mettre cette bande en solo* now. `euc-phase`'s body says *en mode Manuel qu'Euclidien*, following the page's own `label.manual` / `label.euclidean` captions, the way `band.mode`'s body already did.
- **`<html lang>` now follows the language selector** (a canon change, all 43 plugins), so assistive technology reads the page in the language it is displayed in.

### Verification

- `node scripts/i18n-fr-lint.js --plugin O-FreqPulse --strict` — **exit 0**, 0 findings across all ten codes (baseline 21: 5 G1, 3 F1, 7 T5, 4 T3, 2 T7). Straight copies 9 of 9 flagged `sameAsEn: true`, 0 `termNote`.
- `node scripts/check-i18n.js --plugin O-FreqPulse` — ALL CHECKS PASS, canon v2.
- `node scripts/check-ui-labels.js --plugin O-FreqPulse` — ALL CHECKS PASSED across the same six states. **Zero non-label elements moved**, before and after, and the visible element set is identical in both languages. Vacuity moved 27/30 → **22/30 (73 %, floor 25 %)**: `label.mix` is bound to five elements and the settled French for *Mix* is *Mix*.
- **Hover-help was verified by RENDERING.** This plugin predates `tests/ui_tip_render_check.js`, so the arm was driven from a scratchpad probe: all **56** `TIP_BINDINGS` rows resolved the way `applyI18n` resolves them (11 of the 56 decorate a `.control` / `.control-group` **wrapper**, so an enumeration of id'd `[data-tip]` nodes covers only 45), three states, both languages — **1766 assertions, 0 failures**. Every tip opened with a non-empty title and body, at or under the 220 px cap, fully inside the 850 × 550 frame, arrow still inside its anchor. Anchors an open overlay paints over are hit-tested with `elementFromPoint` and driven in another state rather than counted as reachable.
- **The bottom clamp is still load-bearing, and the French tip is still the one that proves it.** Removing v1.18.0's `if (top > maxTop)` line from the served copy reproduces exactly one failure — the French `#grid-area` tip, 220 × 97, at `top` 468 in a 550 px frame, **15.00 px off the bottom** — and no English one. Clamped, it sits at 445 with the full 8 px margin and 0 px to spare.
- `node scripts/boot-all-uis.js` — 43/43 clean, 0 warn, 0 failed, **0 DEAD** bindings, 0 late on this plugin, native `title=` still 0 repo-wide.
- `auval -v aufx OFPu OuDv` — PASS. The installed VST3 and AU both carry the revised French values.

### Notes

- **`reviewed: false` is unchanged on all 67 entries.** That flag means a native speaker read the string; this pass is a second machine reading against a glossary and a lint, and it is recorded in the `i18n.js` header instead.
- **Three width comments in `styles.css` are now stale** and are corrected in a separate comment-only commit: `.preset-action-btn` cites `Enreg. 48.2`, `.band-mix label` cites `Mixage 25.8`, and `.settings-toggle` claims its 40 px `min-width` covers the widest of *On / Off / Oui / Non*. It no longer does — `Marche` measures 36.97 px in a 22 px content box, so the switch becomes content-sized in French and **resizes 11.45 px between its own two faces**. It grows leftward inside a right-anchored popover, so nothing else moves and the popover stays at 186 px. Changing the CSS is out of a copy pass's scope.
- **Not verified:** no DAW test — `auval` and headless Chromium only, never a real WKWebView. Windows/WebView2 font metrics remain the standing hardware-blocked deferral, and French no-break spaces are new surface for it: a wider face shows first inside a fixed 220 px tooltip.

## [1.18.0] - 2026-08-28

MINOR: the PAGE speaks French, not only the hover help — and this plugin's second tooltip renderer is deleted, not disabled. No audio-path, parameter, or DSP changes. One new non-parameter state-tree property (`uiLanguage`).

### Added

- **A language selector, in a new gear popover at the top-right of the header.** English and French. The hover-help on/off switch **moves into the same popover** from the floating `?` that sat over the grid's bottom-right corner — one place for the two things that decide what the help says and whether it says it. The gear and the switch both carry `data-tip-always`, so the two controls that reach and restore the help layer keep explaining themselves while help is off.
- **`Resources/ui/js/i18n.js`** — 33 hover-help entries and 34 label entries, English and French. Landed in ONE commit across all four places the file has to exist or the page 404s at runtime with no other symptom: on disk under the **served** root (`Resources/ui`, not `Source/ui/public`), in `CMakeLists.txt`'s `juce_add_binary_data SOURCES`, in a `getResource()` branch, and in `js/app.js`'s import.
- **The language persists with the session**, as a plain `uiLanguage` property (the string `"en"` / `"fr"`) on the APVTS state tree — never an `AudioParameterChoice`, so it cannot appear in a DAW automation lane and a preset cannot change which language somebody reads their interface in. Restored behind an `isVoid()` gate: `NamedValueSet::setFromXmlAttributes` rebuilds every property as a `var` over the attribute STRING, so `isBool()`/`isInt()`/`isString()` are false for every session ever saved. A session written before v1.18.0 has no such attribute and simply stays English.
- **Accessible names on the five runtime-built glyph controls** — Mute (M), Solo (S), Clear, Randomize and Expand — plus the panel close button. They are keyed and switch with the language; previously four of them had an English `title` and two had nothing.

### Changed

- **ONE tooltip renderer.** The measure-then-pin runtime from O-ReverseDelay / O-MultiBandCompressor replaces this plugin's own positioner **entirely**; there is no second code path left. What the port brings that v1.17.0 did not have: a **title/body pair** built from `data-tip-title` + `data-tip` instead of one flat string, a **120 ms dwell delay**, an **arrow whose offset is recomputed AFTER the horizontal clamp** so a clamped tip still points at its control, **delegated listeners on the document** so the 36 runtime-built anchors need no re-binding, and viewport-relative arithmetic matching the `position: fixed` box the browser actually lays out. `.tooltip` moves from `position: absolute` to `fixed`; the two rectangles coincide on this page, so nothing moved.
- **Every tooltip string was SPLIT, not rewritten.** v1.17.0 authored its help as a single `data-tooltip` string shaped `"Label: sentence."`; each is now the `t`/`b` pair the renderer wants, byte-identical either side of the first `": "`. **Exactly one did not split cleanly** and was hand-split: the step-sequencer grid tip carried no colon at all, so its title `Step Grid` is the one new English string in the tooltip half of the table.
- **Three controls carry new English copy** — the gear, the language selector and the hover-help switch. Two did not exist before and the third had only a native `title`. Authoring hover help for controls that have none is a separate job and is not done here.
- **Band captions are localized**: SUB / LOW / MID / HIGH become SUB / GRAVE / MÉDIUM / AIGU. **The same four keys are the `{band}` token** substituted into all seven per-band tips, so one static binding per band renders correctly in both languages — storing a localized name in the bindings table would have frozen it at whichever language loaded the module.
- **Every native `title=` is gone**, ten of them: six in the markup and four written from `createBandRow`. On an element that also has a `data-tip` a native title renders a second, untranslated OS tooltip competing with the renderer. Where a title was an element's only accessible name its text moved to `data-i18n-aria`; no new prose was invented.
- The preset dropdown's empty row is built with `createElement` + a keyed label instead of an `innerHTML` fragment.

### Fixed

- **A tall tip on a tall anchor fell off the bottom of the window.** The ported renderer prefers *above*, flips *below*, and — as written in O-ReverseDelay — stops there, because every anchor on that page is knob-sized. This page has `#grid-area`, 376 px tall, where neither placement fits: the French grid tip is 97 px and landed at `top` 468 in a 550 px frame, **15 px off-screen**. v1.17.0's own positioner clamped here, so porting without it would have re-broken something this plugin had already fixed. **The same gap is latent in every other copy of this renderer** and is reported rather than swept, because this release is scoped to one plugin.
- **The two 16 px band buttons could be squashed by a longer caption.** They are flex items with no `flex-shrink: 0`, so at the old 80 px column the label gate measured Solo at **8.5 px instead of 16 px** in French. `flex: 0 0 auto` now. Stated plainly: **this rule is not independently gated at the shipped 100 px column** — reverting it alone leaves the gate green, because the widened column already keeps the caption clear. It is kept as a Windows/WebView2 guard, where no gate runs and font metrics are this work's named hardware-blocked deferral.
- **`.control-group label`'s authored `width: 80px` was a basis, not a pin.** All five band-panel labels were shrinking to 53.3 px to make room for the slider, and `Impulsions` (59.9 px min-content) could not shrink that far and pushed its slider 6.6 px right in French alone.

### Geometry — 13 non-label elements moved before fixes, ZERO after

Measured by `scripts/check-ui-labels.js` across six states (default, band panel on SUB, band panel on MID, a band in Euclidean mode, popover open, hover help on), at the parsed 850 × 550 frame, in both languages. **30 of 30 keyed elements were visible in at least one state — no coverage hole.**

| Rule | Was | Now | Why, measured | Moved when reverted |
|---|---|---|---|---|
| `.preset-action-btn` `min-width` | none | 50 px | `.header-top` is `justify-content: center`, so a wider preset bar moves the bar AND the h1 by half the delta. Load 40.2 → Ouvrir 48.3, Save 38.5 → Enreg. 48.2 | **5** |
| `.band-row` column 1 | 80 px | 100 px | The caption shares its row with two 16 px buttons and two 3 px gaps inside 8 px of padding: 80 px left 34.0 px, enough for HIGH at 30.8 and 14.8 short of MÉDIUM at 48.8. 100 px gives 54.0 px — 5.2 px of clearance | **1** (assertion 5, `bandName.mid` spilling 4.8 px) |
| `.band-row` column 5 + `.band-mix label` | 60 px, unpinned | 68 px, `min-width: 26px` | Mix 13.8 → Mixage 25.8 pushed the slider 12.0 px right in all four bands | **4** |
| `.control-group label` | `width: 80px` | `flex: 0 0 80px` | see Fixed, above | **1** |
| `.crossover-divider`, `.freq-boundary` width | 80 px | 100 px | tracks column 1 — these are flex rows, not grid cells, so the two numbers must change together or the crossover handles stop lining up under the captions | *alignment only; not independently gated* |
| `.ms-btn` `flex: 0 0 auto` | — | added | see Fixed, above | *0 at 100 px — stated rather than claimed* |

Each rule was reverted **alone** and the gate re-run, restoring from an in-memory copy in a `finally` rather than `git checkout --`, which would have wiped the uncommitted work alongside the mutation. Four of the six re-broke the gate; the two that did not are named as such above rather than presented as fixes.

**English geometry against v1.17.0**, measured the same way by swapping HEAD's six files in and back out: the header shifts left 10.67 px (the preset-button pin widens the bar 21.3 px) and the 32-cell step lane loses 28 px — **0.88 px per cell**. Nothing else in the page moved, the document's scroll extent is 850 × 550 in both builds and both languages, and neither build logs a page error.

### Verification

- `node scripts/check-i18n.js --plugin O-FreqPulse` — 37 assertions, all pass, on canon v2.
- `node scripts/check-i18n.js --strict-v2` — exit 0 across **15** localized plugins, 0 on canon v1.
- `node scripts/check-ui-labels.js --plugin O-FreqPulse` — ALL CHECKS PASSED. Vacuity 27/30 labels (90 %) and 34/34 attributes actually change language; `dataset.label === textContent` after init, after the switch and after a real state pass in both languages, driven through the stub's slider states; zero non-label elements moved; no page error; every requested resource served.
- **The port was verified by RENDERING, not by inspection.** All **56** tip anchors — enumerated from the DOM, not transcribed — were hovered with a real pointer in both languages, **112 measurements**: every tip visible, with a non-empty title and body, at or under the 220 px cap, **fully inside the 850 × 550 frame**, and with its arrow still horizontally inside its anchor even where the tip is clamped to the window edge. Removing the new bottom clamp alone reproduces exactly one failure — the French grid tip, 15 px off the bottom — and no English one.
- `auval -a` lists `aufx OFPu OuDv`. The build adds no new compiler warning; the 135 that remain are pre-existing sign-conversion warnings in the relay and attachment loops.

### Notes

- **Value readouts, note divisions and preset names stay English.** None of the `toFixed`/`Math.round` sites is touched, the ten `rate` and eleven per-band-rate `<option>` texts are `AudioParameterChoice` entries and therefore the host automation contract, and a preset name IS its JSON filename. `Global` is exempt for both reasons at once: it is the per-band-steps value mirror **and** a choice entry, so translating the readout alone would make this page and the automation lane disagree.
- **All 67 French strings are machine drafts, every one `reviewed: false`.** No native speaker has read them. `node scripts/check-i18n.js` prints the worklist.
- **Not verified:** the C++ language round-trip (pick Français, close the session, reopen, confirm it held) has not been executed by hand — it is reasoned from the `isVoid()` guard, not measured. Windows/WebView2 font metrics remain the named hardware-blocked deferral; the tightest French margin measured here is **5.2 px**, on MÉDIUM in the band caption cell.

## [1.17.0] - 2026-08-13

MINOR: tooltip surfaces now render at their intended size and sit clear of the control they describe. No audio-path, parameter, or state changes.

### Fixed

- **Tooltip geometry is measured at a neutral origin before the surface is placed.** `.tooltip` is `position: absolute` with `width: auto` and `max-width: 220px`, so its shrink-to-fit width resolves against *(containing-block width − `left`)*. The old code read `offsetWidth` while the surface still sat at its **previous** `left`, so hovering anything near the right edge measured an already-squeezed box, computed the new `left` from that wrong number, and then let the box re-flow again once `left` was applied. The failure was self-reinforcing: a squeezed width resolved `left` straight back against the right edge, so it never recovered on later hovers. Measured at the plugin's real 850×550 window, the band Expand buttons (`expand-0..3`, centred at x≈804) collapsed to **66 × 154 px** — a 77-character string in a 66 px ribbon — instead of the correct **220 × 42 px**. 10 of 53 tooltips were squeezed below `max-width` and 5 rendered outside the container entirely. The surface is now reset to `0,0` with `width: auto`, measured, and its width pinned before the final placement is applied.
- **The pinned width is the fractional `getBoundingClientRect().width`, not the integer `offsetWidth`.** A natural width of 208.48 px rounds to 208, and pinning that makes the box 0.48 px narrower than its own shrink-to-fit — enough to push the last word onto a second line. Height is only stable once the width is pinned, so it is now read after: the Clear-band tooltip was being placed using a measured height of 28 px while it actually rendered 42 px, overlapping its button by 14 px.
- **Tooltips sit above their control instead of covering it.** `top` was set to *(control top − 8)* and used directly as the CSS `top` with no `translateY(-100%)`, so the surface rendered on top of the control it described — **46 of 53 tooltips** overlapped their own anchor. The guard `top - height < 0` was written as though `top` were a bottom edge and so fired on the wrong condition; the above-placement had never worked as intended. Placement now computes `top = control top − height − GAP`, flips below only when that would clip the top edge, and clamps back inside if flipping below would clip the bottom.
- **Hovering between a control's own children no longer flickers the tooltip.** Several control-groups wrap a label, a slider and a value-display; crossing between them fired `mouseout` → `mouseover` and blinked the surface off and back on. `mouseout` now ignores moves whose `relatedTarget` is still inside the same tooltipped element.

### Changed

- Horizontal placement computes a real left edge (centre − width/2) and clamps both sides, replacing the previous centre-point plus `translateX(-50%)`. The clamp arithmetic and the box the browser lays out are now the same geometry.

### Testing

Verified in a browser harness at the plugin's true 850×550 editor size (`PluginEditor.cpp:334`) against a stubbed JUCE bridge — WebView layout defects are invisible to build, `auval` and `pluginval`, and a wider viewport never fires the edge clamp at all. All 53 `[data-tooltip]` elements were hovered in sequence so stale-state carry-over between hovers was exercised:

| Metric | v1.16.5 | v1.17.0 |
|---|---|---|
| Tooltips overlapping their own control | 46 | 0 |
| Tooltips rendering outside the container | 5 | 0 |
| Long tooltips squeezed below `max-width` | 10 | 0 |
| Worst case (`expand-0`) | 66 × 154 px | 220 × 42 px |

Clamps land exactly on their bounds: `minLeft` 10, `maxRight` 840 (= 850 − `EDGE_MARGIN`), `minTop` 8.

### Notes

- `.container` was deliberately left without `position: relative`. The absolute coordinates already resolve correctly because the universal `margin: 0; padding: 0` reset plus `html, body { width: 100%; height: 100% }` make the initial containing block coincide with `#plugin-container`; adding a positioned ancestor would have moved `.tooltip-toggle` and `.euclidean-panel` as a side effect.
- The sub-pixel `offsetWidth` rounding issue above also exists in the O-SpectralShaper v1.5.0 implementation this fix was ported from — flagged for a separate pass.

## [1.16.5] - 2026-08-02

Licensing release — no functional changes. PATCH: no audio-path, parameter, or state changes.

### Changed

- **AGPL-3.0 notice headers added to all Ouaricon-authored sources** as part of the repo-wide
  relicensing sweep (commit `1390fc3f`). Binaries rebuilt from identical functional code.

## [1.16.4] - 2026-07-08

Resolves the safe/mechanical info-level findings from the v1.16.2 deep code review
(`CODE_REVIEW.md`): IN-02, IN-03, IN-06, IN-10, IN-13. PATCH — no audio-path behavior change,
no parameter/state changes. (All 11 warnings were already resolved in v1.16.3.)

### Fixed

- **IN-02 — Stale docs described the removed FFT design + a phantom ~46 ms latency.** `NOTES.md`,
  `.planning/REQUIREMENTS.md` (NFR-2), and `.planning/STATUS.md` still claimed FFT-based
  processing (2048-sample, 4× overlap) reporting ~46 ms latency. **The code is correct** — it is
  time-domain Linkwitz-Riley LR4 crossovers reporting `setLatencySamples(0)`. Updated all three
  docs to the LR + zero-latency topology so nobody "restores" a phantom latency report.
- **IN-03 — WebView2 user-data folder pointed at the bare temp root.** `PluginEditor.cpp`
  set `withUserDataFolder(tempDirectory)`; multiple Ouaricon WebView plugins sharing the temp
  root invites cross-plugin lock contention on Windows. Now scoped to
  `tempDirectory.getChildFile("OFreqPulse_WebView")` per the documented pattern.
- **IN-06 — `calculateCurrentStep` had no internal guard for `numSteps == 0`.** Every current
  caller clamps to `jlimit(2, 32, …)` so it was safe today, but a future caller passing 0 would
  hit modulo-by-zero (UB). Added a self-safe `if (numSteps <= 0) return 0;` at function entry.
- **IN-10 — `"steps"` param carried version hint `2` while every other param uses `1`.** A typo.
  Verified against JUCE 8.0.9 that the version hint does **not** feed the VST3/AU param-ID hash
  (`generateVSTParamIDForParam` / AU wrapper hash the string ID only), so this is a no-op for
  automation/state — purely a consistency fix. Set to `1`.
- **IN-13 — Dead variable + stale comment.** Removed the unused `int globalSteps` local in
  `updateEuclideanPatterns()` (the loop uses per-band `effectiveSteps`), and corrected the gain-
  smoothing comment from "SmoothedValue (linear ramp)" to "custom BandEnvelope (linear ramp)"
  (the first smoothing stage is the custom `BandEnvelope` struct, not `juce::SmoothedValue`).
  The harmless one-shot first-block recompute noted in IN-13 was intentionally left as-is (RT-safe,
  changing the cache-init values risks a subtle first-block behavior shift).

### Notes

- Deferred (not in this batch): WR-06/07/08/IN-11 (factory-preset plumbing refactor via the shared
  `OuariconPresetManager` + `module-upgrade`), WR-09 (LR allpass transparency), WR-10 (`freq_low/high`
  intended role), WR-11 (native program menu). The remaining flag-only info items (IN-05/07/08/09/12/14)
  are documented no-ops. IN-01 (FileChooser SafePointer defense-in-depth) and IN-04 (timerCallback
  caching) were not selected for this batch.

## [1.16.3] - 2026-07-08

Resolves all 11 warnings from the v1.16.2 deep code review (`CODE_REVIEW.md`). No critical
issues existed. PATCH — no breaking parameter/state changes.

### Fixed

- **WR-01 — Tooltip preference never persisted (broken both directions).** The Save side
  called `window.__JUCE__.backend.getNativeFunction(...)`, but `getNativeFunction` lives on the
  `Juce` ES-module namespace — the `backend` object only exposes `emitEvent`/`addEventListener`
  — so the guard was always false and the C++ atomic never updated. The Restore side pushed
  `restoreTooltipState` from a single 30 Hz timer tick ~33 ms after construction, before the
  WebView's ES modules had loaded, then latched and never retried. **Root cause:** wrong bridge
  object + a one-shot cold-start race. **Fix:** Save now uses `Juce.getNativeFunction('setTooltipsEnabled')`;
  a new `getTooltipsEnabled` native function lets `initializeTooltips()` *pull* the persisted state
  once the JS is ready, and the racy timer push was removed.
- **WR-02 — Stale step velocities leaked into Euclidean-mode bands (baked into factory JSON).**
  `loadPreset(int)` (used to capture the 12 factory presets) never reset the step grids, and
  Euclidean bands skip `setStepVelocities()`, so a band inherited the previous preset's manual
  velocities — invisible while Euclidean, wrong the instant it switched to Manual. **Fix:** reset
  every band's step grid up-front in `loadPreset(int)`. Factory presets regenerate on load (WR-06).
- **WR-03 — Save dialog silently ignored the chosen directory.** The native save dialog let the
  user navigate anywhere, but `savePreset(name)` always wrote to the user presets folder using only
  the filename. **Fix:** honor the chosen path via a new additive `savePresetToFile()` on the shared
  preset module (symmetric with the load dialog); saving into the default user folder still lists
  the preset in the bar.
- **WR-04 — Crossover cutoff not clamped to Nyquist → NaN below ~40 kHz sample rate.**
  `LinkwitzRileyFilter::setCutoffFrequency` computes `tan(pi*f/fs)`, which is `inf`/negative at or
  past Nyquist. Unreachable at 44.1/48/96 kHz but a 22050 Hz offline render with a high crossover
  could inject NaN. **Fix:** clamp c1/c2/c3 to `0.49 * fs` in `updateCrossoverFrequencies`.
- **WR-05 — Dry/wet block not restricted to the processed channel count.** `DryWetMixer` is prepared
  for 2 channels; a host handing a >2-channel buffer could drive push/mix past the internal 2-wide
  dry buffer (OOB). Unreachable on the fixed stereo bus but now guarded via `getSubsetChannelBlock`.
- **WR-06 — Factory regeneration sentinel was a hardcoded `"1.16.0"` literal.** A future preset/param
  change that forgot to bump the literal would ship stale factory JSON. **Fix:** gate on and write
  `JucePlugin_VersionString`.
- **WR-07 — First-run factory init used a destructive `deleteRecursively` + recreate.** Concurrent
  construction (pluginval + session insert, or two instances) could both delete/recreate the Factory
  dir and interleave writes, and a concurrent `getPresetList()` could observe the emptied window.
  **Fix:** overwrite the 12 files in place; no delete.
- **WR-08 — Local `preset-manager.js` was behind the shared module.** Resynced from
  `modules/persistence/preset-manager/js/preset-manager.js`, picking up bounded `_waitForNative`
  (no infinite poll), fail-safe async `promptDelete`, and the `onConfirmDelete` hook.
- **WR-11 — DAW native program menu advertised 12 dead names.** `getNumPrograms()` returned 12 but
  `setCurrentProgram()` intentionally never loads (to avoid clobbering DAW state restoration). **Fix:**
  report a single program so the native menu isn't populated; presets are managed via the WebView bar.

### Changed

- **WR-10 — `freq_low`/`freq_high` marked non-automatable.** They drive only the WebView grid's
  frequency-axis display and are never read by the DSP; `withAutomatable(false)` stops the host from
  advertising automation lanes that change nothing audible. Parameter IDs and saved state unchanged.

### Documented

- **WR-09 — LR crossover binary tree is approximately (not bit-) unity-gain at rest.** A single LR4
  low+high sum is an allpass, not identity; the tree sums two differently-phased allpass halves, so a
  small magnitude ripple exists near c2 for *closely-spaced* crossovers (negligible at the default
  120/500/4000 Hz). Accepted for a creative rhythmic gate and documented in code + NOTES rather than
  adding per-path allpass compensation.

## [1.16.2] - 2026-03-06

### Added

- **Licensing module integration** - Compile-flag gated (`OUARICON_LICENSING`), off by default for local development. Adds license overlay, listener callbacks, and CMake wiring for commercial distribution.

## [1.16.0] - 2026-03-06

### Added

- **Inline per-band Mix slider** - The depth parameter (controls how much volume drops on OFF steps) is now visible directly in each band row, labelled "Mix". No longer hidden in the Band Controls panel.

### Changed

- **All 12 factory presets redesigned** to showcase features added since v1.0.0:
  - **Step velocity** — Manual presets now use accent patterns and ghost notes (0.0–1.0 velocity) instead of binary on/off. Classic Sidechain, Trance Gate, Dubstep Pulse, and Hi-Hat Chop all feature velocity grooves.
  - **Per-band rate** — Dubstep Pulse uses 1/4 on Sub and 1/8 on Low for polymetric bass. Euclidean Groove runs Sub at 1/4 and High at 1/16. Bass Foundation runs High at 1/8T. Half-Time Feel runs High at 1/8 with a 12-step loop.
  - **Per-band phase offset** — Phase Cascade (formerly Full Spectrum Gate) demonstrates cascading phase offsets (0/4/8/12) across all bands. Trance Gate, Ambient Shimmer, Euclidean Groove, and Triplet Bounce all use phase offsets for band separation.
  - **Per-band step count** — Polymetric Machine (formerly Polyrhythm 5-7-11) uses independent loop lengths (4/6/8/12 steps) per band. Half-Time Feel uses 4-step Sub and 12-step High loops. Triplet Bounce uses a 9-step High loop.
  - **Crossover variation** — Dubstep Pulse (80/300/3k), Ambient Shimmer (100/800/6k), and Hi-Hat Chop (120/600/5k) set custom crossover frequencies.
  - **Asymmetric attack/release** — Each preset has tuned attack/release values: fast gates (1/3ms Hi-Hat Chop), pump shapes (3/35ms Classic Sidechain), slow swells (80/120ms Ambient Shimmer).
- **Renamed presets** — "Polyrhythm 5-7-11" → "Polymetric Machine", "Full Spectrum Gate" → "Phase Cascade"
- **setBandParams now sets per-band rate** — All presets explicitly reset band rate, phase offset, and step count to prevent stale values when switching presets.

## [1.15.0] - 2026-03-05

### Added

- **Per-band step count** - New `band{N}_steps` parameter (integer 0-32, default 0) gives each frequency band its own loop length. When set to 0, the band follows the global Steps count. When set to 2-32, the band loops independently, creating polymetric patterns — e.g., Sub at 8 steps while Mid runs 12 steps and High runs 16 steps simultaneously.
- **Euclidean mode integration** - Per-band step count overrides `euc_steps` when set, so euclidean patterns also generate at the band's custom length.
- **Per-band cell visibility** - UI shows only the active number of cells per band row. Some bands may show 8 cells while others show 16, visually representing the polymetric structure.
- **Band Steps slider in Band Controls panel** - The expand panel now includes a "Steps" slider (0-32) where 0 displays as "Global".

### Technical Notes

- 4 new `AudioParameterInt` parameters: `band0_steps` through `band3_steps` (range 0-32, default 0, version 1)
- `processBlock()` calculates `bandEffSteps[band]` per-band, used for step position calculation via `calculateCurrentStep()` and gain lookup via `getTargetGainForBand()`
- `updateEuclideanPatterns()` uses band step count instead of `euc_steps` when band has custom steps
- Change detection added for band step parameters to trigger euclidean pattern regeneration
- `WebSliderRelay`/`WebSliderParameterAttachment` added to `BandRelays`/`BandAttachments` structs
- JS `updateStepVisibility()` now uses per-band effective step count
- Backward compatible: default 0 preserves identical behavior to v1.14.0

## [1.14.0] - 2026-03-05

### Added

- **Per-band phase offset** - New `band{N}_phase_offset` parameter (integer 0-31, default 0) shifts each band's pattern read position independently. In `getTargetGainForBand`, the offset is applied as `adjustedStep = (currentStep + phaseOffset) % numSteps` before reading the step pattern, creating phase-shifted polyrhythmic patterns between frequency bands.
- **Phase slider in Band Controls panel** - The Euclidean/Band Controls panel now includes a "Phase" slider (0-31) that works in both Manual and Euclidean modes.
- **Always-visible expand button** - The ▶ expand button is now always visible on each band row (not just in Euclidean mode), since Phase and Depth are useful in both Manual and Euclidean modes.

## [1.13.0] - 2026-03-05

### Added

- **Band Mute buttons (M)** - Each band row now has a small "M" button next to the band name that toggles `band{N}_enable`. When muted, the entire band row dims to 35% opacity while the label area remains readable for interaction. The mute state syncs with DAW automation in real-time.
- **Band Solo buttons (S)** - Each band row has an "S" button for exclusive solo. Clicking solo on a band mutes all other bands by disabling their enable parameters. Clicking again restores the pre-solo enable states. Only one band can be soloed at a time.
- **Automation-aware M/S visuals** - Mute and solo button states reflect automation changes via `valueChangedEvent` listeners. If the DAW changes a band's enable parameter, the M button updates accordingly and stale solo state is cleared.

## [1.12.0] - 2026-03-05

### Changed

- **Click-and-drag velocity control** - Left-click and hold on a step, then drag up/down to set velocity. Up increases, down decreases. 100px of vertical movement covers the full 0–1 range. A quick click without dragging still toggles the step on/off.
- **Fill-bar velocity visual** - Step velocity is now shown as a bottom-up fill level instead of opacity. The green highlight fills proportionally to the velocity value, making it easy to see relative velocity levels across the grid.
- **Shift+click retained** - Shift+click still cycles through velocity levels (0 → 0.25 → 0.5 → 0.75 → 1.0 → 0) as a quick alternative.

### Removed

- **Right-click-drag velocity** - Replaced by the more intuitive left-click-drag interaction.
- **Opacity-based velocity display** - Replaced by the fill-bar visual.

## [1.11.0] - 2026-03-05

### Added

- **Step velocity support** - Each step in the sequencer grid now has a velocity value (0.0–1.0) instead of simple on/off. Velocity controls how much gain is applied on active steps, enabling accent patterns and groove variations within each frequency band.
- **Shift+click velocity cycling** - Shift+click a step cell to cycle through velocity levels: 0 → 0.25 → 0.5 → 0.75 → 1.0 → 0. Quick way to set accent patterns.
- **Right-click-drag velocity control** - Right-click and drag vertically on a step cell to set precise velocity. Top of cell = 1.0 (full), bottom = 0.0 (off).
- **Visual velocity feedback** - Step cells display velocity as opacity intensity. Full velocity (1.0) = fully opaque green, lower velocities appear progressively more transparent.

### Changed

- **Step parameters changed from AudioParameterBool to AudioParameterFloat** - `step_b{N}_s{M}` parameters now store velocity floats (0.0–1.0) instead of boolean on/off. Backward compatible: existing presets with false/true (0.0/1.0) load seamlessly.
- **Velocity-aware gain calculation** - `getTargetGainForBand()` now interpolates gain using `(1-depth) + velocity * depth`. At velocity 1.0, gain = 1.0 (same as old ON). At velocity 0, gain = 1.0 - depth (same as old OFF). Intermediate velocities produce proportional gain reduction.
- **WebView relay type** - Step grid relays changed from `WebToggleButtonRelay` to `WebSliderRelay` with corresponding `WebSliderParameterAttachment` for continuous value communication.
- **Random pattern** - Randomize button now generates random velocities (0.5–1.0 range) for active steps, creating natural accent variation instead of uniform full-velocity patterns.
- **Factory presets regenerated** - Version check forces regeneration to capture new parameter types.

### Technical Notes

- 128 step parameters: `AudioParameterFloat(0.0–1.0, step 0.01, default 0.0)` replacing `AudioParameterBool`
- Parameter IDs unchanged (`step_b{N}_s{M}`, version 1) — APVTS XML stores both types as float, enabling seamless migration
- Euclidean mode remains binary (velocity 0.0 or 1.0) since patterns are algorithmically generated
- DSP formula: `gain = (1.0f - depth) + velocity * depth` — linear interpolation between gated floor and unity
- UI opacity range: 0.3 (minimum visible) to 1.0 (full velocity) for active cells

## [1.10.0] - 2026-03-04

### Added

- **Per-band playhead highlighting** - Replaced the single scrolling playhead bar with independent per-cell highlights for each frequency band. Each band's current step glows bright green (`--playhead`) when playing, allowing bands running at different rates (e.g., Sub at 1/4, High at 1/16) to show their individual positions simultaneously.

### Changed

- **Per-band step atomics** - Added `bandStepAtomics[4]` to the processor for thread-safe per-band step communication to the GUI timer. Timer now sends 4 independent step positions instead of 1 global step.

### Removed

- **Scrolling playhead bar** - The vertical green bar that spanned all bands at a single column position. Replaced by per-cell highlighting which correctly represents polymetric sequencing.

## [1.9.0] - 2026-03-04

### Fixed

- **Attack/Release range expanded from 0-100ms to 0-500ms** - The previous 100ms maximum was too narrow for a gating effect, making parameter changes imperceptible at most tempos. Now extends to 500ms with logarithmic skew (0.4) for fine control at short times and dramatic sweeping at long times.

### Changed

- **Skewed slider response** - Attack and Release sliders now use logarithmic-style mapping: ~20ms at 25%, ~88ms at 50%, ~250ms at 75%, 500ms at 100%. Provides better resolution in the musically useful 0-50ms range.
- **Factory presets regenerated** - Version check forces preset regeneration to capture new parameter range.

## [1.8.0] - 2026-03-04

### Added

- **Separate Attack and Release parameters** - Replaced the single "Smoothing" parameter with independent Attack (0-100ms) and Release (0-100ms) controls. Attack controls fade-in time when a step turns ON; Release controls fade-out time when a step turns OFF. Fast attack + slow release creates plucky gates; slow attack + fast release creates swells. Both enforce a 2ms minimum.

### Changed

- **Custom BandEnvelope replaces SmoothedValue** - Per-band gain envelope uses a linear ramp with separate attack/release rates instead of JUCE's SmoothedValue (which only supports symmetric ramp times). One-pole LPF softener retained for smooth corners.
- **Factory presets updated** - Select presets now showcase asymmetric attack/release: Classic Sidechain (5/25ms), Trance Gate (2/5ms), Ambient Shimmer (40/60ms), Half-Time Feel (15/40ms), Euclidean Groove (5/10ms).
- **Factory preset versioning** - Presets auto-regenerate when plugin version changes, ensuring new parameters are captured.

### Removed

- **Smoothing parameter** - Superseded by Attack and Release. Old saved states will load with default 5ms attack/release.

## [1.7.0] - 2026-03-04

### Added

- **Per-band rate parameter** - Each frequency band (Sub, Low, Mid, High) now has its own Rate dropdown with 11 options: "Global" (follows the main Rate knob) plus all 10 tempo divisions (1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T, 1/4D, 1/8D). Enables polymetric sequencing — e.g., sub at 1/4, highs at 1/16.

### Technical Notes

- 4 new `AudioParameterChoice` parameters: `band0_rate` through `band3_rate` (default: "Global")
- Per-band step positions computed independently in `processBlock()` from host PPQ using each band's effective rate index
- Global playhead continues to follow the global Rate for consistent visual feedback
- Rate dropdowns appear in each band row between Clear/Random buttons and the Manual/Euclidean mode toggle
- `WebComboBoxRelay`/`WebComboBoxParameterAttachment` added to `BandRelays`/`BandAttachments` structs
- Full preset/automation compatibility: band rates saved/restored with plugin state

## [1.6.7] - 2026-03-04

### Improved

- **Cache all 128 step-cell DOM references in a 2D array** - `cachedCells[band][step]` is populated once after `renderGrid()`, replacing per-call `querySelectorAll`/`querySelector` in three hot paths: `updateStepVisibility()` (was querying all 128 cells), `updateStepVisual()` (was querying by attribute per cell), and `updateEuclideanGrid()` (was querying 32 cells per band). Also replaced `classList.add`/`remove` pairs with `classList.toggle` for cleaner conditionals.

## [1.6.6] - 2026-03-04

### Improved

- **Cache band-0 step cells for playhead positioning** - `updatePlayhead()` (called at 30Hz) no longer runs `querySelector()` and `.grid-area` lookup every tick. Band-0 cells are cached as an array after `renderGrid()` completes, and the grid area element is cached once. Cell is now accessed by index (`cachedBand0Cells[step]`) instead of a DOM query each frame.

## [1.6.5] - 2026-03-04

### Improved

- **Replace manual RMS loop with JUCE's AudioBuffer::getRMSLevel()** - Signal-presence detection in processBlock() now uses the built-in getRMSLevel() per channel with jmax, replacing the manual sumSquares loop. Same result, less code.

## [1.6.4] - 2026-03-04

### Improved

- **Eliminate redundant atomic load in processBlock()** - RMS silence detection now computes `signalPresent` as a local bool, stores it to the `hasAudioSignal` atomic for the GUI thread, and uses the local variable for audio-thread logic. Removes one unnecessary `atomic::load()` per block.

## [1.6.3] - 2026-03-04

### Fixed

- **Corrected misleading DryWetMixer comment** - The inline comment on the `dryWetMixer` member said "10ms max latency" but the constructor argument is `maximumDelayInSamples`, not milliseconds. Updated to "10 samples max delay for dry/wet alignment".

## [1.6.2] - 2026-03-04

### Removed

- **Dead `setupGlobalControls()` function** - Removed empty function (only contained a console.log) and its call from DOMContentLoaded handler. All parameter binding already handled by `initializeGlobalParameters()`. No functionality change.

## [1.6.1] - 2026-03-04

### Changed

- **Refactored per-band relays and attachments to array-based structs** - Replaced 24 individually-named relay members and 24 individually-named attachment members with `BandRelays` and `BandAttachments` structs using `std::array<..., 4>`. Constructor uses loops instead of copy-pasted blocks. Critical destruction order (relays → webview → attachments) preserved.

## [1.6.0] - 2026-02-07

### Added

- **Preset manager module** - Full preset save/load system using the Ouaricon preset-manager module. Users can save custom presets, browse factory presets via a dropdown menu, navigate with prev/next buttons, and import/export preset files via native system dialogs.
- **Preset bar in header** - Compact preset navigation bar with prev/next arrows, clickable preset name (opens dropdown), Load and Save buttons.
- **12 factory presets** - All existing factory presets (Init, Classic Sidechain, Trance Gate 16th, Dubstep Pulse, Ambient Shimmer, Polyrhythm 5-7-11, Bass Foundation, Hi-Hat Chop, Full Spectrum Gate, Euclidean Groove, Half-Time Feel, Triplet Bounce) are now available as JSON files in `~/Library/O-FreqPulse/Presets/Factory/`.
- **User presets** - Save your own presets to `~/Library/O-FreqPulse/Presets/User/`. Factory presets are read-only.

### Technical Notes

- Integrated `OuariconPresetManager` from `modules/persistence/preset-manager/` (v1.0.0)
- 9 native functions registered: `savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`, `savePresetWithDialog`, `loadPresetFromFile`
- Factory presets generated dynamically from existing `loadPreset()` method on first run
- State save/load delegates to preset manager's `getStateAsXml()`/`setStateFromXml()` while preserving tooltip state
- `FileChooser` used for native save/load dialogs (async, non-blocking)
- Preset dropdown uses `:has()` CSS selector to escape stacking context and appear above other UI elements
- JavaScript `PresetManager` class imported as ES module from `preset-manager.js`

## [1.5.1] - 2026-02-07

### Changed

- **UI title updated** - Header and page title changed from "O-FreqPulse" to "Ouaricon Frequency Pulse" for consistent branding.

## [1.5.0] - 2026-02-07

### Added

- **Tooltip toggle system** - A `?` button in the bottom-right corner toggles tooltips on/off. When enabled, hovering over any UI element displays a descriptive tooltip explaining what it does. Tooltip state is saved with the plugin preset and restored on load.
- Tooltips for all UI elements: Mix, Steps, Rate, Swing, Smoothing, crossover dividers, frequency boundaries, band labels, clear/random buttons, mode toggles, expand buttons, Euclidean panel controls (Steps, Pulses, Offset, Depth), and the step grid area.

### Technical Notes

- Tooltip toggle state persisted via `tooltipsEnabled` atomic<bool> in PluginProcessor, saved/restored in XML state
- C++ native function `setTooltipsEnabled` registered via `withNativeFunction()` for JS→C++ communication
- State synced from processor to WebView on first timer callback via `restoreTooltipState()` JS function
- Smart tooltip positioning: appears above element by default, falls below if too close to top edge, constrained within container bounds horizontally
- CSS uses naturalist theme colors (dark brown bg, warm paper text, green accent for active state)
- `data-tooltip` attributes on both static HTML elements and dynamically created elements in `createBandRow()` and `createDividerSlider()`

## [1.4.0] - 2026-02-06

### Changed

- **Variable step count (2-32)** - Steps parameter changed from a fixed 4-option dropdown (4, 8, 16, 32) to a continuous slider allowing any integer from 2 to 32. Enables odd time signatures and non-power-of-two patterns like 5, 7, 12, etc.

### Technical Notes

- Parameter type changed from `AudioParameterChoice` to `AudioParameterInt` (range 2-32, default 16)
- Parameter version bumped to 2 for the "steps" ID to handle state migration
- UI changed from `<select>` dropdown to range slider with live value display
- Editor relay changed from `WebComboBoxRelay`/`WebComboBoxParameterAttachment` to `WebSliderRelay`/`WebSliderParameterAttachment`
- All 12 factory presets updated to use direct step count values
- Processor reads step count directly (`jlimit(2, 32, ...)`) instead of indexing into a lookup array

## [1.3.2] - 2026-02-06

### Fixed

- **Two-stage gain smoothing eliminates residual step-transition clicks** - Added a one-pole lowpass filter after the SmoothedValue linear ramp to soften the discontinuous first derivative at ramp start/end into a smooth S-curve. Also enforced a 2ms minimum smoothing time to prevent instant gain jumps when the user sets smoothing to zero.

### Technical Notes

- SmoothedValue produces a linear ramp with sharp corners at onset/offset — in STFT-reconstructed audio these corners can produce brief transient clicks
- One-pole LPF with ~1.5ms time constant (`1 - exp(-1 / (0.0015 * sampleRate))`) applied per-sample after `getNextValue()` rounds the ramp into an S-curve
- `bandGainFiltered[4]` array tracks the filtered gain state per band, initialized to 1.0 in `prepareToPlay()`
- Minimum smoothing floor of 2ms prevents degenerate zero-length ramps

## [1.3.1] - 2026-02-06

### Fixed

- **Eliminated clicks at step onset/offset transitions** - Two root causes addressed:
  1. `SmoothedValue::reset()` was called every processBlock, which internally calls `setCurrentAndTargetValue(target)` — instantly snapping the gain to its target and killing any in-progress smoothing ramp. Now only called when the smoothing parameter actually changes.
  2. Step transitions were detected once per block boundary, not at the exact sample. Added sample-accurate PPQ interpolation within the processBlock loop so gain target changes align precisely with beat positions.

### Technical Notes

- `SmoothedValue::reset(sampleRate, rampLength)` calls `setCurrentAndTargetValue(target)` which sets `currentValue = target` and `countdown = 0`. Calling this every block truncated ramps shorter than the buffer size (e.g., 5ms = 220 samples truncated at 128-sample buffers).
- PPQ is now interpolated per-sample using host BPM: `samplePpq = blockStartPpq + ppqPerSample * sampleIndex`. Step transitions detected within the sample loop trigger immediate `setTargetValue()` calls at the exact transition sample.
- Free-running standalone mode also benefits from the same per-sample PPQ tracking.

## [1.3.0] - 2026-02-06

### Changed

- **Inline Manual/Euclidean mode toggle** - The "Manual" label on each band lane is now a clickable toggle that switches between Manual and Euclidean modes directly in the main UI. No longer requires opening the popup panel just to change modes.
- **Expand button visibility** - The `▶` button to open euclidean controls (Steps, Pulses, Offset, Depth) is now hidden when a band is in Manual mode and only appears when Euclidean mode is active.
- **Removed mode toggle from popup panel** - The euclidean controls panel now only shows the pattern parameters (Steps, Pulses, Offset, Depth) since mode switching is handled inline.
- **Auto-close panel** - Switching a band back to Manual mode automatically closes the euclidean panel if it was open for that band.

## [1.2.0] - 2026-02-05

### Fixed

- **Eliminated buzzing artifact at step on/off transitions** - Moved band gain application from spectral domain (per-FFT-frame) to time domain (per-sample), eliminating the ~86Hz amplitude modulation caused by applying different gains to overlapping STFT frames.

### Technical Notes

- Root cause: with 75% overlap (hopSize=512), applying band gain once per FFT frame meant overlapping frames received different gain values, creating amplitude modulation at the frame rate (44100/512 ≈ 86Hz = audible buzz)
- Fix: each band's frequency bins are now reconstructed separately via IFFT into per-band time-domain output FIFOs, then per-sample `SmoothedValue::getNextValue()` gain is applied in the time domain before summing
- Added per-band output FIFO buffers (`bandOutputFifo[4][2]`), passthrough FIFO for unassigned bins, and temporary FFT buffer for per-band IFFT
- `processFrame()` now produces 5 separate overlap-add outputs (4 bands + passthrough) instead of a single gain-weighted output
- Passthrough bins (frequencies not assigned to any band) are reconstructed once and passed through at unity gain
- COLA (Constant Overlap-Add) compliance maintained: Hann synthesis window + correction factor applied per-band

## [1.1.2] - 2026-02-04

### Fixed

- **Euclidean patterns now display on the grid** - When Euclidean mode is enabled for a band, the generated pattern visually populates the step grid cells with a distinct warm brown color. Previously, the C++ processor computed Euclidean patterns internally but never communicated them to the UI, so the grid always showed the stale manual pattern.

### Technical Notes

- Root cause: `euclideanPatterns[band]` array in C++ was used for audio processing but never written to `step_b{N}_s{M}` parameters or sent to the WebView
- Fix is UI-only: Bresenham euclidean algorithm replicated in JavaScript (`generateEuclidean()`)
- When euclidean mode is active: grid shows computed pattern, manual step clicking is disabled, clear/random buttons are blocked
- When euclidean mode is off: manual step parameters are restored to display, clicking re-enabled
- Manual patterns are preserved in step parameters and never overwritten by euclidean mode
- Euclidean grid updates reactively when euc_steps, euc_pulses, or euc_offset parameters change

## [1.1.1] - 2026-02-04

### Fixed

- **Playhead no longer moves when no audio is present** - Added input RMS detection (~-60 dB threshold) that gates playhead advancement in both host-sync and standalone modes. Playhead fades out when signal drops below threshold and reappears when audio returns.

### Technical Notes

- Root cause: playhead step counter advanced unconditionally in processBlock regardless of input signal level
- RMS computed per buffer across all channels; threshold at 0.001f (~-60 dB)
- Signal state communicated to WebView via existing timer callback (30 Hz)
- Playhead opacity animated with 150ms CSS ease transition for smooth fade

## [1.1.0] - 2026-02-04

### Added

- **Clear button per lane** - Resets all 32 steps in a band to OFF state
- **Random button per lane** - Fills steps with 50% probability pattern
- Both buttons appear in each band row between the step grid and mode indicator

### Technical Notes

- Lane actions use Unicode symbols for compact display (⌀ for clear, ⚄ for random)
- Random pattern applies to all 32 steps regardless of current step count setting
- Parameter changes propagate immediately to JUCE backend via existing toggle bindings

## [1.0.0] - 2026-02-04

### Added

- Initial release
- 4-band spectral step sequencer (SUB, LOW, MID, HIGH)
- 32-step grid with variable step counts (4, 8, 16, 32)
- Euclidean rhythm generator per band
- Tempo-synced playback with swing
- WebView UI with naturalist aesthetic
- 12 factory presets
