# O-SpectralShaper Changelog

## [1.7.3] - 2026-09-03

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
- **The stale width note in `js/i18n.js` re-measured, not scaled.**
  `Infobulles` renders **45.55 px** in the nowrap `.settings-label`, down from
  the 65.89 px of the caption it replaces, and 4.17 px NARROWER than English
  `Hover help` (49.72). The popover holds its 168 px min-width and nothing
  inside it moves, either way. The `.settings-toggle` 64 px pin is unchanged.

### Fixed — the switch faces now agree

- **The switch's two faces `ui.on` / `ui.off` now read `Activées` /
  `Désactivées`** — feminine PLURAL, agreeing with the noun this switch governs,
  which is `les infobulles` as of this release. They were singular because they
  agreed with the singular noun the suite used before it. That dependency is
  stated only in a source comment beside them: the two faces carry no occurrence
  of the noun, so the sweep that rewrote every string mentioning it did not reach
  them. They kept `reviewed: true` — these are the grammar-forced number of a
  word the developer read and approved, not authored wording.
- **The `.settings-toggle` 64 px `min-width` no longer covers the widest face,
  and was NOT changed.** Re-measured with the plural: en `Off` holds the pin at
  64.00 px; fr `Désactivées` is content-sized at **65.77 px** (47.77 text + 18
  padding), so the button resizes **1.77 px** between languages — which is what
  the v1.7.2 pin was raised to 64 px to stop. Nothing is clipped and nothing
  outside the button moves. **No gate sees it:** `.settings-toggle` carries
  `data-i18n`, so it is a LABEL, and `check-ui-labels` assertion [7] watches
  NON-label elements — the plugin exits 0. Left for a deliberate decision rather
  than widened on the spot: closing it means `min-width: 66px` (2.23 px slack).
  The faces are not to be abbreviated to fit 64.
- `scripts/i18n-fr-glossary.js` accepts `activées` / `désactivées` on the
  `on` / `off` rows alongside the existing renderings. The singulars stay: every
  other plugin's toggle agrees with a singular antecedent. **Positive control
  fired before the glossary moved** — the plural on an unchanged glossary made
  `i18n-fr-lint` exit **2** with two G1 findings naming both faces; with the
  glossary landed the same file exits **0**.

## [1.7.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed
- **item 40 — the hover-help switch in the settings popover (`.settings-toggle`, `css/styles.css`):** its `min-width: 40px` covered the English faces (On / Off both render the button at 40.00 px) but not the French ones, so the button **resized between its own two French faces on every toggle** — `Désactivée` 61.88 px, `Activée` 49.09 px, its left edge jumping 12.78 px inside the row. The pin is now **64 px**, measured on the live node at the shipping 700×500 frame: the widest face's text is 43.88 px, plus 8 px padding and 1 px border each side (the rule is `box-sizing: border-box`, so the pin governs the border-box), plus 2.12 px of slack. The button now renders 64.00 px on all four faces in both languages; the popover holds at 168 px, `#lang-select` and the row's left edge do not move, and `check-ui-labels` reports 0 non-label elements moved in both languages across all four driven states. Worst-case row is the French caption beside the French face: 65.89 + 12 + 64 = 141.89 px against 148 px of content width, 6.11 px to spare. No copy changed; no French changed; no parameter, preset or state-format change.

## [1.7.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **21 French entries revised** against the suite glossary (`scripts/i18n-fr-glossary.js`) and lint (`scripts/i18n-fr-lint.js`): 13 terminology, 5 typography, 3 meaning, 0 grammar. The lint went **24 findings to 0**, `--strict` exit 0. The most visible: **Tenue → Maintien** on the Sustain caption, both Sustain tip titles and every body that named the curve — the textbook ADSR term; **Aide → Aide au survol** in the settings popover, where the tip title and the accessible name on that same control already said so; **Oui / Non → Activée / Désactivée** on the hover-help switch, because On/Off there is a feature state, not a power state; straight `0.1–50 ms` → `0,1–50 ms` decimal commas; and no-break spaces between every number and its unit (`20 Hz`, `±12 dB`, `100 ms`) and before every `%` and `;`.
- **Three tooltips named a control by its English caption and now name what the page shows.** The preset body said "utiliser Save" where the French button reads **Enr.**; the draw-mode body said "Freehand" and "Node" where the buttons read **Libre** and **Points**; the lookahead-time body said "voir le commutateur Lookahead" where the toggle reads **Anticipation**. A tip that points at a caption the page does not display is a tip that lies.
- **"La déflexion maximale est de ±12 dB" → "Le débattement total est de ±12 dB"** on both curve editors. *Déflexion* is a calque of "deflection"; a control's full travel is a *débattement* in French.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so assistive technology reads the page in the language it is displayed in.

### Notes — what was measured, and what was kept
- **Reset keeps `Init.`, and the 1.7.0 header's width defence is confirmed rather than inherited.** Three of the nine plugins reviewed before this one had a defence that measured backwards, so this one was re-measured with the gate's own method at the shipping 700×500 frame: the glossary's **Réinit.** renders `.curve-reset-btn` at **58.69px** and **Réinit** at 56.30px against its **53.52px** `min-width` pin, and `.curve-controls` is a right-pinned flex row — so the overflow pushes `#attack-undo-btn` and `#attack-redo-btn` **5.17px** and 2.78px left. Those two carry no `data-i18n`, so that is a `check-ui-labels` assertion-7 geometry regression, not a cosmetic one. **Réinitialiser** is 96.39px. Recorded as the single `termNote` on the table.
- **`Maintien` fits where `Tenue` did**: 57.44px in an 80px `.knob-label`, no wrap, `.knob-wrapper` height unchanged at 87px. **`Aide au survol`** is 65.89px in a nowrap `.settings-label`; the popover holds at its 168px `min-width` and nothing inside it moves.
- **`.settings-toggle`'s 40px `min-width` no longer covers the widest face.** `Désactivée` renders the button at 61.88px. The **row** still cannot resize — the popover holds at 168px, `#lang-select` and the row's left edge do not move, and 8.23px of slack remains in the 12px gap — but the button does, and the CSS comment that claimed otherwise was corrected. Restoring the invariant would need a wider pin, which is a rule change this stage does not make.
- **The trailing periods on `Enr.` and `Ouv.` stay.** The label-in-name rule (WCAG 2.5.3) that dropped them elsewhere does not apply here: `#preset-save` and `#preset-load` carry no `aria-label`, so the accessible name **is** the visible caption.
- **Geometry unchanged**: `check-ui-labels` reports **0 non-label elements moved** between English and French across all four states, 21/21 keyed elements covered, no French caption clipped. `check-i18n` ALL CHECKS PASS, canon v2. `boot-all-uis` 43/43 clean, 0 DEAD.
- **`reviewed: false` stays `false` on all 47 entries.** That flag means a native speaker has read the string. This pass was a second machine reading against a glossary and a lint; the file header records it, the flag still records the human.

## [1.7.0] - 2026-08-28

### Added
- **The interface speaks French.** A settings gear in the header opens a popover holding a language selector (English / Français) and the hover-help switch. Every label on the page, every tooltip, and every accessible name is localized; the choice persists with the host session. 45 strings — 24 tooltip entries and 21 label entries — live in the new `Resources/ui/js/i18n.js`. **All French is machine-drafted and flagged `reviewed: false`; no native speaker has read it.** `node scripts/check-i18n.js` prints the worklist.
- **The hover-help switch moved into the settings popover.** It was the wax-seal "?" in the same header slot through 1.6.2. The gear occupies that slot's exact 18×18 footprint in the same `.header-right` flex row, so the new control contributes **zero** geometry delta. The gear and the switch carry `data-tip-always`, so the two controls that reach and restore the help layer keep explaining themselves while help is off.
- **`getUiLanguage` / `setUiLanguage` native functions**, joining the existing `setTooltipsEnabled` / `getTooltipsEnabled` pair — 15 native functions become 17. The language rides the session XML as a plain attribute beside `tooltipsEnabled`, written as the language **code** rather than the index: a non-parameter value on a state tree comes back from XML as a `var` over the attribute *string*, so an index would have to be re-parsed anyway (`critical_valuetree_xml_roundtrip_loses_type`). It is deliberately not an `AudioParameterChoice` — it must not appear in a DAW automation lane, and a preset must not be able to change which language somebody reads their interface in.

### Changed
- **The tooltip renderer is replaced by the repo's measure-then-pin runtime, and the old positioner is deleted.** This plugin was the only one of the seven ported in this stage that already *measured* its surface — but it positioned against `#app`, not the viewport:
  ```
  const containerRect = container.getBoundingClientRect();
  let left = rect.left - containerRect.left + rect.width / 2 - width / 2;
  const maxLeft = containerRect.width - width - EDGE_MARGIN;
  ```
  `#app` carries `padding: 12px` inside a 700×500 body, so its content box is inset and its clamp rails were **24px narrower than the window** — every tip was held 12px further from each edge than it needed to be, and the vertical rail was computed against `containerRect.height` rather than `window.innerHeight`. That is a small enough error to read as a styling choice rather than a wrong reference frame, which is exactly why it was replaced outright rather than adapted: `.tooltip` moves to `position: fixed` in the same commit, and adapting in place would have left `absolute` positioning being fed viewport coordinates.
- **What the port brings that 1.6.2 did not have:** a title/body pair built from `data-tip-title` + `data-tip` rather than one flat string; a 120 ms dwell delay so a tip does not fire on every crossing; a `pointerdown` suppression so a tip cannot hang over a knob or a curve canvas mid-drag; a width measured as the **fractional** `getBoundingClientRect().width` rather than the integer `offsetWidth` 1.6.2 pinned (188.48 rounds to 188, and pinning that pushes the last word onto a second line); an arrow whose offset is recomputed **after** the horizontal clamp so a clamped tip still points at its control; and delegated listeners on the document rather than on `#app`.
- **English hover-help copy was moved, not rewritten.** 1.6.2 authored its 25 `data-tooltip` attributes as single `"Label: sentence."` strings. All 21 unique strings split cleanly on their first `": "` into the title/body pair the ported renderer wants — **there is no hand-split on this plugin**, and that is measured rather than assumed: the longest surviving title is "Previous Preset" at 15 characters and no body reaches its own colon before the separator. Four tips are worn by two controls each (Spectrum, Undo, Redo, Draw Mode), which is the reuse 1.6.2 authored by hand; Reset is not among them, because its two bodies name different curves.

### Fixed
- **The header version string read `v1.6.1` on a 1.6.2 build.** It was hard-coded in the markup and not bumped with the release. It now reads `v1.7.0`.

### Notes — the two D-04 geometry rules, and what each was measured against
Every width below was measured **as rendered**, in the real element with the real CSS — never from `getComputedStyle().font`, which carries neither the `text-transform: uppercase` nor the `letter-spacing` these controls have and so reads them narrower than they paint.

1. **`.knob-wrapper` and `.toggle-container` get `width: 100%`.** Both were sized by their widest *child*, and on the Sensitivity knob that child is the caption — so the container's box tracked the caption's language (`dw = -16.1px` between English and French). Filling the grid cell they already sat centred in makes both language-independent. Applying this to all seven containers moves **zero of their 20 children**: `align-items: center` already centred the knob, the caption and the readout on the cell's centre line, and that centre does not move when the box around it grows. It also freed the two captions to take the natural French word rather than an abbreviation — "Sensibilité" (70.45px) is in fact *narrower* than the English "Sensitivity" (72.11px).
2. **The five captioned buttons of `.preset-bar` and `.curve-controls` are pinned per-element to their English rendered widths** — 43.55 / 46.20 / 75.92 / 53.52 / 76.41 px. These are flex rows whose width is the *sum* of their items, so a caption that changes width moves every sibling and the row itself; `Save`→`Enreg.` and `Load`→`Ouvrir` alone shifted `‹`, the preset readout and `›` left by 11.3px. Pinned **per-element and not uniformly**: one pin wide enough for `.curve-mode-toggle` would have added 23px to `.curve-reset-btn` and pushed the row off its plate. `min-width` rather than `width`, so a wider font on another platform can still grow rather than clip.

   The mode toggle's pin covers all four of its faces, so the row no longer jumps 26px when the draw mode is switched — which it did in **English** through 1.6.2.

Five French captions were then sized to fit those pins. D-04 forbids an auto-shrink font and forbids a runtime short-variant fallback, so the string itself is sized — exactly one French form per key: `Enreg.` (55.06px) → **`Enr.`** (41.45), `Ouvrir` (57.31) → **`Ouv.`** (41.80), `Réinit.` (58.69) → **`Init.`** (45.58), `Main levée` (84.75) → **`Libre`** (52), and `Node` → **`Points`** (58.31).

### Notes — verification
- **`node scripts/check-i18n.js --strict-v2`** passes; the plugin is on canon v2 and the repo total is 18 canon-v2 plugins, 0 on v1.
- **`node scripts/check-ui-labels.js --plugin O-SpectralShaper`** passes with **21 of 21** `[data-i18n]` elements measured — no coverage hole. A new `tests/i18n-states.json` drives the three states no resting page shows: the Node face of the draw-mode toggle, the settings popover open, and the hover-help switch on.
- **Geometry, English 1.6.2 → English 1.7.0:** zero visible elements moved. The only differences are the seven invisible container boxes widened by rule 1 above, and the `#tooltip-toggle` → `#gear-btn` swap in the same 18×18 slot.
- **Geometry, English 1.7.0 → French 1.7.0:** nine elements change size and **every one of them is a `[data-i18n]` label**. Zero non-label movement — the assertion the whole gate exists for.
- **The hover sweep: 28 anchors × 2 languages = 56 hovers, every tip fully inside the 700×500 window, zero page errors.**
- **The vertical clamp is NOT independently reproducible on this plugin**, and that is reported rather than dressed up. In French the *smallest* slack between a tip's bottom edge and the frame is **111px**, and deleting that line alone leaves all 56 hovers inside the window. The two shapes that could have produced an overhang both miss: the tallest anchor is `.spectrogram-container` at 202px, but it sits at y=70 and flips `below` to 386.7; the deepest anchors are the sustain plate's five buttons at y=397, and those all fit `above`. It is ported anyway because the point of this stage is one runtime repo-wide.
- **That negative result is a measurement, not a probe that passes either way.** Removing the **horizontal** clamp instead, in the same harness, reports **14 off-frame tips out to 120px** across both languages. The sweep can see an off-frame tip; it simply does not find one when the vertical clamp is gone.
- No DSP, no parameter, no preset-format and no state-format change beyond the additive `uiLanguage` attribute. Sessions saved before 1.7.0 restore with the language defaulting to English; `languageIndex()` maps anything that is not `"fr"` to English, so a hand-edited attribute degrades rather than being stored unvalidated. `tests/ui_preset_menu_check.js` passes unchanged, 32/32.

## [1.6.2] - 2026-08-20

### Fixed
- **The editor no longer freezes when audio is processed faster than realtime.** Opening the UI during an offline bounce — or any render that is not rate-limited to the clock — could hang the whole message thread indefinitely: the window never finished appearing and the plugin stopped responding until the host was killed. Realtime playback was never affected, which is why this survived to release.
- **Root cause:** the editor's 60 Hz visualization drain was `while (fifo.getNumReady() > 0)`, re-asking the audio thread how many frames were pending on *every* iteration. The producer is one frame per FFT hop (`HOP_SIZE` 256 → 187 frames/sec at 48 kHz), so at realtime the loop drains ~3 frames and the count reaches zero. Under a faster-than-realtime render the audio thread refills the FIFO faster than the message thread can emit, the count never reaches zero, and `timerCallback()` never returns — starving the message loop that WebView2 needs to finish opening. **Fix:** snapshot `getNumReady()` once before the loop and read exactly that many frames, so frames arriving mid-drain wait for the next tick instead of extending the current one. Emission is additionally capped at the 16 newest frames of the snapshot, with the entire snapshot retired via `finishedRead()` whether or not it was emitted (leaving skipped frames ready would re-create the same stall on the following tick).
- **Caught by:** Windows `pluginval` strictness 10, which hung in *"Open editor whilst processing"* until its 10-minute timeout and failed the v1.6.1 release build. The plain *"Editor"* test — which opens the editor with no concurrent `processBlock` — passed in 1.0 s, isolating the fault to the concurrent path.

### Changed
- **Visualization frames are no longer serialized when the browser is hidden.** `emitVisualizationFrame` now returns before building its ~4 kB JSON payload (289 float→string conversions) if the WebView is not visible. The gate is `isVisible()` — deliberately the exact condition JUCE tests inside `emitEventIfBrowserIsVisible()`, not the stricter `isShowing()` — so it can only skip payloads that were already going to be discarded, and cannot stall the spectrogram during window reparenting.

### Notes
- No DSP, parameter, preset-format or state-format changes. Presets and sessions from 1.6.x load unchanged; the audio path is untouched.
- The 16-frame cap is clear of every realtime hop rate — ~3 frames/tick at 48 kHz, ~13 at 192 kHz — so no frame is dropped during normal playback at any supported sample rate. It engages only when a render outruns the clock, where the extra columns are stale before they could be drawn.
- This is the first O-SpectralShaper release to pass through the Windows `pluginval` gate: `ci-tests.yml` runs `pluginval` on O-Octagon only, so the strictness-10 editor tests are first reached at release-tag time by `build-and-release.yml`. The defect dates from the Phase 3.3 visualization work, not from 1.6.1.

## [1.6.1] - 2026-08-19

### Fixed
- **Curve editors now update when a preset is loaded.** Every preset already carried full 32-band attack/sustain curve data (all 29 factory presets since 1.6.0 via `makeCurveState`, user presets via the preset manager's `customState`), and loading one did change the DSP — but the WebView curve editors received curve data exactly once, 100 ms after the editor first attached (`parentHierarchyChanged`). No load path (menu, ◀ ▶ arrows, `loadPreset`, load-from-file, session restore with the editor open) ever re-sent the curves, so the editors kept drawing the previous preset's shapes and the bank *looked* like it shipped without curve settings.
- **Root cause:** one-shot C++→JS curve push with no notification on state-driven curve replacement. **Fix:** the processor keeps a `curvesRevision` atomic bumped *only* in the preset manager's `customLoad` callback — deliberately not in `setAttackCurve`/`setSustainCurve`, so the UI's own drag-edits never echo back into an in-progress drag. The editor's existing 60 fps timer polls the revision and re-sends both curves through the existing `sendAttackCurveToJS`/`sendSustainCurveToJS` path when it changes. This covers every load path, including session restore, with no new bridge functions.

### Notes
- No DSP, parameter, preset-format or state-format changes. Presets and sessions from 1.6.0 load unchanged.
- The timer send is gated on `hasNavigated`; before the page exists the revision simply stays pending and is delivered on the first tick after navigation. `sendCurveToJS` already guards with `if (window.fn)` on the JS side.

## [1.6.0] - 2026-08-19

### Added
- **The preset readout is now a click-to-open menu, grouped by category.** Clicking the preset name in the header drops a scrollable menu of the whole bank under seven narrative headings — Essentials, Drums & Percussion, Cymbals & Air, Vocals & Speech, Instruments, Mix & Master, Creative — with sticky category headers, the loaded preset highlighted and scrolled into view, and a `▾` affordance on the readout. Selection goes through the same `loadPreset()` path the ◀ ▶ arrows use, so the menu and the arrows cannot disagree about what is loaded. Escape or a click anywhere outside closes it.
- **Factory bank grown 9 → 29 presets.** Twenty new presets, each with purpose-authored 32-band attack/sustain curves (band frequencies taken from the analyzer's log spacing, values in the ±12 dB curve range) and parameter values in engineering units converted through each parameter's own `NormalisableRange` — the CR-02 skew lesson applied from the start. New: Extra Snap; Kick Tightener, Snare Crack, Tom Focus, Room Tamer, Percussion Sparkle; Hat De-Harsh, Shimmer Sustain; Plosive Guard, Vocal Presence, Breath & Air; Strum Snap, Piano Hammer, Bass Definition, String Swell, Pick Bite; Low-End Tightener, Master Polish; Attack Eraser, Infinite Bloom. All nine pre-existing presets keep their exact values and names.
- **`getPresetListGrouped` native function (ported from O-Bitrot v1.13.0).** Categories are expressed as index *spans* over the factory vector's declaration order — never a second list of names, which would go stale silently on the first rename — and the constructor asserts the spans tile the bank exactly. The function returns an ordered array of `{category, presets}` sections cross-checked against the live preset list, then a "User" section holding everything else on disk (omitted when empty).

### Fixed
- **The ◀ ▶ arrows now step through the menu's grouped order.** The preset-manager module's native prev/next walk the C++ flat *alphabetical* list, which matched the old flat display only by coincidence; against a grouped menu, ▶ from the last drum preset would land mid-way through another category (`pattern_grouping_preset_dropdown_breaks_prev_next`). The arrows are now bound in `app.js` to the flattened menu order (`presetWalkOrder`), wrapping at the ends; a preset loaded from a file enters the walk at the top going forward, the bottom going back.

### Notes
- No DSP and no parameter changed: same 7 parameter IDs, same ranges, same state format. Existing sessions and user presets load unchanged; the 20 new factory presets appear on first launch (factory files are refreshed at startup).
- Verified by a new headless gate, `tests/ui_preset_menu_check.js` (ported from O-Bitrot): it derives the expected grouping from `PluginProcessor.cpp` itself, holds the browser stub and the rendered DOM to it, and drives the real page at the shipping 700×500 — 32/32 checks. The walk-order probe was negative-controlled: rebinding the arrows to the module's alphabetical navigation makes exactly that check fail (▶ from "Cymbal Control" lands on "De-Esser" instead of "Hat De-Harsh").
- The category header background is opaque (`--paper-accent`) because the headers are `position: sticky` — a translucent tint lets rows show through once a header pins. The `▾` affordance is a CSS `::after`, not a child node, because the module's `_updateDisplay()` writes `textContent` to the readout and would erase any real child on the first preset change.

## [1.5.0] - 2026-08-13

### Added
- **Hover tooltips across the whole interface, gated by a "?" toggle in the header.** Every control now carries a `data-tooltip` describing what it does and its real range — 25 in total: the seven knobs/toggles, both curve editors and their ten buttons, the spectrogram, and the five preset-bar controls. Tooltips are off by default and only arm when the "?" beside the version string is lit, so the field-guide layout stays uncluttered for users who don't need them. Ranges quoted in the text are taken from the actual `NormalisableRange` definitions (Attack 0.1–50 ms, Sustain 10–500 ms, LA Time 0.1–10 ms, Output −12 to +12 dB) and the curve range from `STFTProcessor::MAX_SHAPE_DB` (±12 dB), rather than being written from the UI labels.
- **Tooltip preference persists with the session.** The toggle state round-trips through two new native functions (`setTooltipsEnabled` / `getTooltipsEnabled`) into a `tooltipsEnabled` attribute on the session XML. The WebView *pulls* the stored value during its own init rather than having the editor push it on open, which would race the WebView load. The attribute is stamped on after `presetManager.getStateAsXml()` has built the tree, so a UI preference never leaks into a saved preset file — loading a preset cannot change whether your tooltips are on.

### Changed
- **The nine existing `title=` attributes became `data-tooltip`.** They previously produced the OS's own native tooltip; leaving them in place alongside the new system would have shown two overlapping tooltips on the same control. They are now part of the toggle-gated system and share its styling.

### Fixed
- **Tooltip measurement is done at a neutral origin before placing.** An absolutely positioned element's shrink-to-fit width is computed against (containing-block width − `left`), so measuring the surface while it still sits at its previous position reports a wrapped, narrow box near the right edge and the edge-clamp then mispositions it. Measured against the Lookahead tooltip at `left: 600px`, the naive ordering collapses the surface to **100 × 269 px** instead of the correct **240 × 110 px**. The surface is now reset to `0,0` with `width: auto`, measured, and its width pinned in px before the final placement is applied. (The reference implementation in O-FreqPulse v1.5.0 measures in the naive order and carries this latent bug — flagged for a separate pass.)

### Notes
- UI-only release. No DSP, parameter, preset-format or state-format changes: parameter IDs, ranges and factory presets are untouched, and the added session attribute is optional on read (absent in pre-1.5.0 sessions ⇒ defaults to off), so existing sessions and presets load unchanged.
- The Lookahead tooltips state plainly that the control is inert rather than describing behaviour it does not have, matching the Known Limitation carried since 1.3.2.
- Verified in a headless WebView harness against a stubbed JUCE bridge: all 25 tooltips resolve their text, become visible only while armed, and stay inside the 700×500 window (0 overflowing, 0 collapsed, widths 206–240 px). Toggle off/on, hover-while-disabled, and mouseout were each asserted, as was the C++ persistence round-trip.
- The comment at `STFTProcessor.cpp:320` describes the curve range as ±18 dB while `MAX_SHAPE_DB` is `12.0f`. The tooltips follow the constant. The stale comment is left as-is here — flagged, not fixed, to keep this release UI-only.

## [1.4.0] - 2026-08-12

### Changed
- **UI reskinned to the Ouaricon Naturalist brand aesthetic.** The interface was a generic dark charcoal theme (`#1A1A1A`) with modern blue/orange accents, Georgia type and plain dark-disc knobs — it carried none of the house style. It is now a field-guide page: aged-paper ground, Garamond typography, warm earth palette (walnut `#8B7355` / oak `#5C4033` / `#3C2F2F` text), botanical seed cross-section knobs, green botanical toggle and buttons, and fleuron ornaments.
- **Analysis displays kept as dark specimen plates.** The spectrogram and both curve editors retain dark grounds, now set in 3px walnut frames with inset shadow so they read as photographic plates mounted on the paper page. The WebGL inferno colormap is unchanged — spectral legibility was the reason to keep these areas dark rather than invert them to ink-on-cream.
- **Curve accents moved from modern blue/orange to earth tones** that stay legible on the dark plate: attack moss `#9BB877`, sustain ochre `#D4A257`. These were duplicated as string literals in three places in `app.js`; they are now a single `ACCENT_COLORS` constant mirroring the `--accent-attack` / `--accent-sustain` CSS custom properties.
- **Botanical specimen made visible.** The plugin already shipped a nudibranch illustration (after Trinchese, lith. Armanino, *Atti della R. Università di Genova*, Vol. II, Tav. VI — public domain) but rendered it at 0.08 opacity where it was effectively invisible, and as a dark-plate image that could not sit on a light ground. It is now converted to sepia ink on a transparent ground and placed right-side per the house spec at 0.42 opacity, bleeding off the edge behind the control column.

### Fixed
- **Three controls were unreachable.** The knob sidebar laid seven controls out in a single flex column whose content ran 638px tall inside a 418px container — a 220px overflow with `overflow` unset, so **Lookahead, LA Time and Output Gain were all clipped off the bottom of the window with no way to scroll to them**. Output Gain in particular has been inaccessible from the UI since the sidebar was introduced. The sidebar is now a two-column grid (Mix/Attack, Sustain/Sensitivity, Output/LA Time, with the Lookahead toggle spanning both columns); all seven controls fit with 60px of vertical slack, verified stable across the Garamond, Times New Roman and Georgia font fallbacks.
- **Header version string was stale:** `index.html` hard-coded `v1.3.0` while the plugin shipped as 1.3.2. Now reads v1.4.0.

### Removed
- **Watermarked stock background texture.** `Resources/ui/images/paper-bg.webp` was a tiled-"Adobe Stock"-watermarked image (visible when brightened; it went unnoticed because it rendered at 0.1 opacity over a dark background). It was also a *dark navy* grunge texture, unusable for the aged-paper ground. Replaced with the clean aged-paper texture already used by O-Tremolo, re-encoded to WebP at 700×500. Filename is unchanged, so the `juce_add_binary_data` list needed no edit.

### Notes
- Visual restyle only — no DSP, parameter, preset-format or state changes. Parameter IDs, ranges, factory presets and the saved-state format are untouched, so existing sessions and presets load unchanged.
- The Lookahead control remains inert (see the 1.3.2 Known Limitations); this release only makes it reachable, it does not change its behaviour.
- Two sibling plugins still ship the same watermarked texture — `O-Lyrica/Resources/ui/images/paper1.jpg` and `O-Gain/Source/ui/public/images/paper1.jpg` are byte-identical (md5 `b7c865c45f2fb95a7a8651071da186e6`). Out of scope here; flagged for a separate pass.

## [1.3.2] - 2026-07-07

### Fixed
- **CR-01 — Preset file-dialog use-after-free:** the `savePresetWithDialog` and `loadPresetFromFile` `FileChooser::launchAsync` completions captured raw `this` + the WebView-owned `complete` callback. If the plugin window closed while a dialog was open, the completion dereferenced the freed editor and invoked a dead callback. Now capture a `juce::Component::SafePointer` and bail with a bare `return` on teardown (matching the correct pattern already used in `parentHierarchyChanged`).
- **CR-02 — Factory presets ignored the ATTACK_TIME/SUSTAIN_TIME skew:** preset values were authored as linear fractions that ignored the 0.3 skew, so every preset recalled attack/sustain times ~10–30× too short and the "Default" preset did not match the plugin's power-on state. Factory preset values are now authored in engineering units (ms/dB/fractions) and converted through each parameter's own `NormalisableRange` via `convertTo0to1()` at init, so the skew is applied correctly. "Default" now reproduces the APVTS defaults (ATTACK_TIME 10 ms, SUSTAIN_TIME 100 ms).
- **WR-01 — Attack/Sustain knob readouts showed wrong numbers:** the JS `formatValue` callbacks re-derived the skewed range with an incorrect exponential formula (attack displayed ~0.3 ms at the 10 ms default). They now read the real engineering value from JUCE via `Juce.getSliderState(id).getScaledValue()`.
- **WR-02 — Curve-less presets left stale curves applied:** loading a preset without `customState` (Default, Gentle Shaping, Aggressive Bite, Sustain Lift) left the previously-loaded preset's attack/sustain curves in effect. These four presets now carry an explicit flat (all-0.0) curve `customState`, so loading them resets both curves to neutral.
- **WR-03 — Latency re-reported from the audio thread every block:** `processBlock` called `setLatencySamples()` on every block, continuously signalling a (potentially changed) latency from the audio thread — which many hosts glitch on or ignore. Latency is now cached in `lastReportedLatency` and re-signalled only when it actually changes (Lookahead toggled or its time changed). Only the latency-thrash bug is fixed here; the Lookahead control's underlying no-op behaviour is a known limitation deferred to a dedicated DSP pass.

### Known Limitations
- **Lookahead is currently inert:** enabling Lookahead delays both the transient detection and the shaped signal by the same amount, so gain and signal stay time-aligned and nothing audible changes (it only adds reported latency). True lookahead requires splitting detection from application in `STFTProcessor` and is deferred to a future release.

## [1.3.1] - 2026-07-01

### Fixed
- Preset-manager module sync (`preset-manager` v1.0.2) — fixes from the O-DigiDelay code review:
  - **WR-04:** preset names are sanitized before use as filenames (`/\\:` → `_`) in save/load/delete/isFactory, so a name containing `/` no longer silently drops the file.
  - **IN-02:** preset JSON records the real plugin version (`JucePlugin_VersionString`) instead of a hard-coded `"1.0.0"`.
  - **IN-03:** prev/next resume from the last in-list position instead of snapping to index 0 after loading an out-of-list preset from file.
  - **IN-01:** corrected the preset-path docstring.

## [1.3.0] - 2026-03-08

### Added

- **Real-time spectrum overlay on curve editors:** Toggle "Spectrum" button on each curve editor to display the live input FFT magnitude as a semi-transparent filled shape behind the editable curve. Maps 257 FFT bins to the logarithmic X-axis with 60dB dynamic range. Uses accent-colored fill (blue for attack, orange for sustain) at ~15% opacity so the curve remains clearly visible. Spectrum state persists across freehand/node mode switches.
- **Spectrum toggle buttons:** Each curve editor (attack, sustain) has a "Spectrum" button in the control bar. Active state shows accent-colored highlight matching the curve type.

## [1.2.0] - 2026-03-08

### Added

- **Undo/redo for curve editors:** Both freehand and node curve modes now support undo (Ctrl/Cmd+Z) and redo (Ctrl/Cmd+Shift+Z) with a 30-step snapshot stack per editor. Snapshots capture at action boundaries (stroke start, node add/move/delete, reset) so undo reverts entire gestures, not intermediate frames. NodeCurve snapshots include node positions for full state restoration.
- **Undo/redo UI buttons:** Small arrow buttons added to each curve editor's control bar with automatic enable/disable state tracking.
- **Focus-aware keyboard routing:** Keyboard shortcuts route to whichever curve editor (attack or sustain) was last clicked, with proper cleanup on editor destroy/mode switch.

## [1.1.5] - 2026-03-08

### Fixed

- **Add synthesis window to STFT overlap-add reconstruction:** Previously only the analysis Hann window was applied before the forward FFT. Now a Hann synthesis window with per-sample WOLA normalization is applied after the inverse FFT, smoothing frame-boundary discontinuities caused by spectral modification. Reduces metallic ringing and "musical noise" artifacts during aggressive shaping. Root cause: Hann² is not COLA at 50% overlap (sum varies 0.5–1.0), so a precomputed normalized synthesis window `w[i] / (w²[i] + w²[i+H])` is used for correct reconstruction. Zero additional runtime cost (single multiply, precomputed table).

## [1.1.4] - 2026-03-08

### Changed

- **Remove dead code:** `hopTime` member in STFTProcessor (set but never read), empty `loadCurvesFromProcessor()` in app.js, redundant `#include <juce_dsp/juce_dsp.h>` in PluginProcessor.h (already included via STFTProcessor.h).
- **Encapsulation:** `handleAttackCurveUpdate`, `handleSustainCurveUpdate`, `sendAttackCurveToJS`, `sendSustainCurveToJS` moved from public to private in PluginEditor.h.
- **Deduplicate curve handlers:** Identical `handleAttackCurveUpdate`/`handleSustainCurveUpdate` merged into single `handleCurveUpdate(args, setter)` using member function pointer.
- **Deduplicate dialog result creation:** Extracted `makeDialogResult` helper lambda for preset save/load file dialog callbacks.
- **Remove FreehandCurve.drawCurve() indirection:** Eliminated wrapper that just called `drawDataCurve()`; renamed `drawDataCurve` to `drawCurve` directly.
- **Fix JS event listener leaks:** Store bound references for CurveEditor resize handler, Spectrogram WebGL context lost/restored handlers, and NodeCurve keydown listener; add `destroy()` methods for proper cleanup.

## [1.1.3] - 2026-03-08

### Changed

- **Cache APVTS parameter pointers:** `getRawParameterValue()` pointers now cached as class members in constructor instead of 7 string-hash lookups per `processBlock` call.
- **Replace magic number 512:** Dry delay buffer size and modulo operations now use `STFTProcessor::FFT_SIZE` constant.
- **Reuse lastMagnitudes[] in detectTransients():** Band magnitude loop reuses pre-computed magnitudes instead of recomputing `sqrt(r²+i²)` for ~257 bins per frame.
- **Pre-allocate visualization JSON:** `emitVisualizationFrame()` uses `preallocateBytes(4096)` and `<<` operator instead of repeated `+=` at 60fps.
- **Cache getBandFrequencies() and log constants:** Band frequencies computed once in constructor. `logMinFreq`/`logMaxFreq`/`logFreqRange` cached for `freqToX()`/`xToFreq()` (called hundreds of times per render frame).

## [1.1.2] - 2026-03-08

### Fixed

- **Dangling pointer crash in parentHierarchyChanged:** `callAfterDelay(100, [this]{...})` could fire on a destroyed editor if the DAW closed/recreated the UI within 100ms. Now guarded with `juce::Component::SafePointer`.
- **Unclamped/NaN curve values from WebView:** `handleAttackCurveUpdate` and `handleSustainCurveUpdate` now clamp values to [-1.0, 1.0] and replace NaN with 0.0 before passing to DSP.
- **Division by zero in NodeCurve.interpolateAndSample():** When two nodes share the same frequency, the denominator `(rightNode.freq - leftNode.freq)` was zero, producing NaN that propagated through curve data. Now guards the denominator and falls back to `t = 0.0` (left node's gain).

## [1.1.1] - 2026-03-08

### Fixed

- Attack/sustain time knobs not responding
- Curve data race condition
- Lookahead latency reporting

## [1.1.0] - 2026-02-07

### Added

- Initial release with WebView UI

## [1.0.0] - 2026-02-03

### Added

- Per-frequency transient shaping with 32 logarithmic bands
- Freehand and node-based curve editing
- Real-time spectrogram with transient heat overlay
