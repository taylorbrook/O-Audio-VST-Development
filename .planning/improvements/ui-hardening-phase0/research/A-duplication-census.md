# A — UI duplication and drift census (46 plugins), with UI-kit extraction candidates

**Scope.** Read-only census of every plugin's WebView UI. It quantifies duplication and drift so shared-module candidates for a cross-plugin UI kit can be chosen.

**Snapshot.** Run on 2026-10-26 against `git archive HEAD` (`ea44af28`, 2026-10-22). The archive holds tracked files only and was extracted into the scratchpad.

- Five plugins (O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM) have **uncommitted** R4/R5 work in the working tree: AA inks, 9 px floor and EB Garamond. Their CHANGELOGs are dated 2026-09-25. The census counts them as they are at HEAD.
- Where the working tree differs, the text says so.

Nothing in the repo was modified.

**Prior art this builds on.** `.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md`, the 2026-09-24 review at 44 plugins. Several of its recommendations have since shipped:

- R1: the `ui-static-gates` CI job.
- R2: generator docs.
- R3: `scripts/ui-canon.js` + `check-ui-canon.js`.
- R4/R5: AA inks, 9 px floor, `modules/ui/eb-garamond`.
- R6: the dead `modules/ui/*` were deleted.

The numbers below are re-measured and are not copied from that review.

---

## 0. Headline findings

1. **Duplication is large and mostly unconverged.**
   - Authored UI (excluding i18n tables and vendored JUCE) is 18.5k markup + 53.6k CSS + 83.3k JS = **155k lines**. On top of that sit **51.5k lines of i18n tables**, which are per-plugin by design, and **33k lines of vendored JUCE frontend JS** (46 copies).
   - Of the 91k *normalized* authored lines (no blanks, comments or brace-only lines), **39% recur in at least one other plugin** (4-line clone windows), **20% in ≥5 plugins** and **10% in ≥20**.
2. **A gate is what converges code, not the copying itself.**
   - The i18n runtime is byte-compared by a gate: **1 variant across 46 copies**.
   - Every ungated shared component has fragmented. Knob binders: **30 distinct of 32**. Tooltip renderers: **31 of 42**. Hover-help init: **43 bodies under 6 names**. Settings-popover JS: 18. Value entry: **21 of 40**, a feature written in a single-day sweep on 2026-10-19.
3. **Every suite-wide UI fix costs one commit per plugin.**
   - Since 2026-09-24: R4/R5 contrast + font = **38 commits**, with 5 more still uncommitted. Value entry = **40 commits**. Its click-away bugfix landed **37 more commits the same day**.
   - The generator docs were taught the contract on 09-24 and are already stale again: `.claude/agents/ui-finalization-agent.md` has 0 mentions of `attachValueEntry` or `eb-garamond`.
4. **Bugs live in the copies. One fix exists in one lineage and has not propagated:**
   - **Settings popover / preset dropdown cannot be dismissed by pressing a knob in 8 plugins.** They use a document `mousedown` dismisser while the knobs `preventDefault()` `pointerdown`. This includes the canon reference O-ReverseDelay. O-Chorus and the O-simple* lineage already fixed it.
   - **29 of 42 tooltip renderers lack measure-then-pin.** They are exposed to the documented shrink-to-fit edge squeeze.
   - **O-Strata reuses O-Prism's `localStorage` key.** JUCE serves every plugin from the single origin `juce://juce.backend/`, so the two share one hover-help setting on macOS.
   - **Hover help defaults OFF and is never persisted** in O-Gain, O-IntonationPad and O-Polystutter.
   - **8 plugins draw canvas text in EB Garamond with no font-load repaint.**
5. **The existing shared-JS mechanism is broken in CI.**
   - `ouaricon_add_module()` copies module JS at configure time into a *gitignored* `Source/ui/public/modules/`. 11 plugins embed that untracked file.
   - In a fresh checkout it does not exist, so `check-i18n` [12] fails. **The "UI Static Gates" job has failed on every push to main since 2026-09-28** (last green 09-26; run `39454657864` on 10-20: `11 FAILED`, plus 1 fr-lint finding).
   - Direct embed from `modules/` (39 plugins already use it: eb-garamond, tuning-panel and others) has zero drift and is the right carrier for a kit. It needs a CMake helper and a generic resource resolver to scale.
6. **A kit reverses a recorded decision.**
   - `260826-ieq-CONTEXT.md:51-53` says "Per-plugin JS, no shared module".
   - The 09-24 review lists "shared runtime library retrofitted into all UIs" under *Not recommended — requires decision*.
   - This needs an explicit decision record. It also needs a versioned, opt-in rollout so it respects the "converge per plugin via /improve, never a sweep" rule.

---

## 1. Subsystem matrix

**Column definitions:**

- **Plugins**: plugins that have the subsystem.
- **Variants**: distinct normalized bodies (comments and whitespace stripped, md5) unless noted.
- **JS lines in copies**: raw lines of the named functions summed over all copies (function census).
- **≥5 shared**: share of the subsystem's normalized JS + CSS + HTML lines that sit in a 4-line window found in ≥5 plugins. High means the copies are near-identical, so extraction is mechanical. Low means the copies are fragmented, so extraction means picking a winner, which is a behaviour change.

| # | Subsystem | Plugins | Variants | JS lines in copies (removable*) | ≥5 shared | Bug risk | Best reference |
|---|---|---|---|---|---|---|---|
| 1 | JUCE frontend lib (`js/juce/index.js`, `check_native_interop.js`) | 46 | index: 3 byte-level, **1 functional** (= JUCE 8.0.15). interop: 4 (30 upstream, 6 CRLF, **7 patched, 3 stubbed**) | **33.3k (100%)** | 100% | Low (inert, see §2.9) | Upstream `~/JUCE/modules/juce_gui_extra/native/javascript/` |
| 2 | i18n runtime (`applyI18n`/`initI18n`/`setLabel`/`trLabel`…) | 46 | **1** (gated). `tr()` 2, differing only by a local name | 4,460 (4,363) | 86% | Low | `scripts/i18n-canon.js` |
| 3 | Tooltip renderer (+CSS) | 46 (42 named) | **31 / 42**. Two designs: anchor + measure-pin ×13, cursor-follow ×29 | 5,028 (4,752) + ~1.4k CSS | 71% | **Med-High** (29 lack pin) | Canon in `scripts/ui-canon.js` (O-ReverseDelay@ed1fc465); shape-matched by O-simpleAmbisonics / O-simpleWavetable |
| 4 | Hover-help switch | 46 | **43 bodies / 6 fn names**, 2 ids (`#tips-toggle` 35, `#help-toggle` 11), 3 persistence schemes | 1,191 (1,088) | 42% | **Medium** (key collision, 3 default-OFF) | Canon `applyTipsEnabled` + `initTipsToggle` |
| 5 | Settings popover + language select (gear) | 46 | JS 18 (12 shapes). Markup: popover 21 / gear 12 / lang-select 11 exact | 1,518 (1,417) + ~4.0k CSS + ~550 HTML | 45% | **High** (8 dismiss-dead) | O-simple* `initializeSettingsPopover` (`b0217a57` ×6) or O-Chorus (`3ca2b949`): pointerdown + Esc + focus return |
| 6 | Value entry (`attachValueEntry`, `scaledToNorm`, parse) | 40 (+5 WIP) | **21 / 40**. 6 signatures, ≥4 formatter contracts. `scaledToNorm` 9 / 42 | 3,498 (3,368) + ~1k CSS | 84% | Medium | Group V1 (`40ef3998`, O-simpleSubtractive / -Sampler / -Grain / -PMS / -Additive / -Beatmaker) |
| 7 | Rotary knob: interaction + visual | 44 rotary (32 with a named binder) | binders **30 / 32**. `updateKnobVisual` 19 / 23. Visual families A 13 · A′ 12 · B 12 · C 6 · D 1 · bespoke 1 (+R-only 2) | 4,232 (3,548) + ~3.3k CSS + ~2.7k HTML | 15% | **High** (22 no pointer capture, 27 no arrow keys, 4 mirrored ranges) | Interaction: O-simpleReverb v3.0.1 `bindKnob` (superset of canon). Visual: O-ReverseDelay Family A |
| 8 | Preset bar / dropdown | 33 (+5 lesson-only) | **4 native API families**. 3 markup-id conventions. `preset-manager.js` in 4 delivery modes, 7 hashes. `showPresetDropdown` 10 / 10 | 2,722 (1,774) + ~3.4k CSS | 4.5% | Medium (4 dismiss-dead) | O-ReverseDelay preset bar (grouped dropdown, keyboard nav, delete-arm), *after* its dismiss fix |
| 9 | Toggles / combos / segments | 32 (named) | **47 / 51** across 13 names (`bindToggle` 11 / 11) | 1,344 (971) + ~3.6k CSS | 8% | Low-Med | O-ReverseDelay `bindSelectCombo` + segment binders |
| 10 | Meters / visualizers | 28 | Bespoke. `vu-meter.js` module has **0 UI consumers**. 3 transports | ~2.5k JS + 1.6k CSS | 6% | Low | Only a generic in/out level meter + `makeCanvas` (DPR) are worth sharing |
| 11 | Header / footer chrome | 46 | ≥4 wordmark spellings. Title is `<h1>` in 27 plugins, other markup in 19. `<header>` 17 / `.header` 39 | ~9k (mostly CSS) | 12% | Low | O-ReverseDelay header + `.frame` |
| 12 | CSS tokens | 43 define tokens | **441 names (251 used once)**, 2 vocabularies, **52 names with conflicting values** | — | — | Medium (contrast) | `aesthetic.md` palette + O-ReverseDelay `:root` |
| 13 | Paper / illustration assets | 19 paper, 7 insects | paper: **4 distinct across 19 copies** | 13.3 MB duplicate bytes in repo | — | None | — |
| 14 | Font loading (EB Garamond) | 37 at HEAD (42 WT) | 21 resolved stacks. Font token `--font-serif` 23 vs `--serif` 15 | — | — | Medium (8 canvas fallback) | `modules/ui/eb-garamond` |

\* **Removable** = total lines minus one median-sized copy per function name. It is an upper bound for JS only.

**Lineages** (pairwise containment of 4-line shingles, excluding the shingles present in ≥30 plugins):

- O-Prism ~ O-Strata 0.72. Strata's UI is a Prism fork.
- The six O-simple synths cluster at 0.31–0.45.
- O-Bassoon ~ O-Reed ~ O-Wind 0.26–0.29, with O-Bowed weaker.
- O-Bass ~ O-Tremolo 0.32.
- O-simpleAmbisonics ~ O-simpleReverb 0.30.
- O-ReverseDelay ~ O-Tapestop 0.20.

Every other plugin is a singleton below 0.25. Fragmentation is per lineage, so migration can go lineage by lineage.

---

## 2. Subsystem detail (variants, references, divergences that are bugs)

### 2.1 Rotary knob
- **Visual families** (markers: `knob-stem` → A, conic ring without stem → A′, `knob-vine` → B, `knob-fill`/`drawKnob` → C, bare `knob-indicator` → D, `type=range` → R):
  - A: 13 — ReverseDelay, Tapestop, MBC, and all 10 O-simple*. O-simpleReverb moved here from B in its v3.0.0 "family retrofit".
  - A′: 12.
  - B: 12.
  - C: 6.
  - D: 1 (Texture).
  - R-only: FreqPulse, Octagon.
  - Bespoke: Polystutter.
  - O-Wind mixes B and C. Bells, Lyrica, MicrotonalSampler and Prism mix B with native ranges.
- **Interaction:**
  - 32 plugins declare a binder (`bindKnob` 18, `setupKnob` 7, `createKnobSVG` 5, `setupFxKnob`/`makeFxKnob` 3 …). There are **30 distinct bodies and 30 distinct shapes**: no two plugins share one.
  - 14 more bind knobs as arrow functions or classes, for example O-SpectralShaper `js/components/RotaryKnob.js`.
  - Page-level presence:
    - `setPointerCapture`: 24 / 46.
    - `lostpointercapture`: 12.
    - Arrow keys: 19. The other 27 are mouse-only.
    - `role=slider`: 20.
    - `prefers-reduced-motion`: 4.
  - Mouse-event drags with window/document listeners, which are stuck-drag-prone when released outside the WebView, remain in Bass, Bells, Comp, Contrabass, Freeze, GrainScatter, IntonationPad, Lyrica, Orbit, Polystutter, TextureForge, Tremolo and Wind.
- **JS-mirrored ranges** (anti-pattern; readouts should use `getScaledValue()`):
  - O-Reed: 28 sites.
  - O-Bassoon: 10 sites.
  - O-AnalogEQ: 8 sites.
  - O-Freeze: 8 sites. `setupKnob(knobId, state, min, max, …)` at `index.html:1255`.
- **Stale canon.** `scripts/ui-canon.js:81-86` anchors the knob canon at O-ReverseDelay `ed1fc465` (v1.20.0). ReverseDelay is now v1.22.4. The 10-19 value-entry sweep rewrote every `bindKnob`, so **0 plugins match the knob-binding or knob-visual canon, including ReverseDelay itself**.
- **Best reference today:** O-simpleReverb v3.0.1 `bindKnob` (`plugins/O-simpleReverb/Source/ui/public/index.html`). It is a superset of the canon: PageUp/PageDown/Home/End, `aria-valuemin`/`max`, a drag-active guard on keydown, plus capture + lost + cancel, the wheel gesture and Shift fine-drag. Family A visual from O-ReverseDelay.

### 2.2 Value entry (double-click numeric)
- **Coverage:** `attachValueEntry` is in 40 plugins at HEAD.
  - Absent: Bassoon, Freeze, Gain, Reed, simpleFM (all five mid-migration in the working tree) and Polystutter.
  - 21 distinct bodies. The largest groups are V1 ×6 (simple*), V2 ×5 (Detune, DigiDelay, Formant, SpectralShaper, Tremolo), V3 ×4 (Bass, Bitrot, Chorus, Comp) and V4 ×3 (Bells, Lyrica, Wind). There are 12 singletons.
- **What all 40 share:** the capture-phase click-away fix, `sliderDragStarted`/`Ended` pairing, Enter/Esc handling and parse.
- **Contract drift under one name:**
  - 6 signatures: `(valueEl,state,formatFn)` ×28, `+opts` ×6, `(…,parse,render)` ×3, `(…,opts)` Emulator, `(…,render,parseScaled)` MBC, and Octagon's own.
  - `formatFn` is called with **normalised** values in most plugins, **scaled** in simple* ×6 + Texture, and `(norm, extra)` in Strata and TextureForge.
  - Copying a version between lineages renders wrong readouts unless the call sites change too. This is a latent copy-paste hazard, not a live bug.
- **`scaledToNorm`:** 42 plugins, 9 variants. The common one (`cad6a2fc` ×33) handles skew only. No plugin uses symmetric-skew or lambda ranges today (grep), so this is a kit design note rather than a bug.

### 2.3 Tooltip renderer
- **Coverage:** 42 named renderers plus 4 arrow-function renderers (Bitrot, Contrabass, Octagon, Orbit). 31 distinct bodies; check-ui-canon reports 29 distinct under its own grouping (§3).
- **Two designs:**
  - **Anchor-placed + measure-then-pin** (13): FreqPulse, Gain, IntonationPad, Lyrica, Marimba, MBC, Polystutter, ReverseDelay, SpectralShaper, Tapestop, simpleAmbisonics, simpleReverb, simpleWavetable.
  - **Cursor-following, measured at the previous offset** (29). Example: `plugins/O-Comp/Source/ui/public/index.html:2338-2365`.
- **Bug class (likely, cosmetic):** all 46 tooltips are `position:fixed` with `max-width` and no `width`. The 29 cursor renderers read `getBoundingClientRect()` at the old `left` and then flip or clamp from that. That is exactly `pattern_fixed_tooltip_shrink_to_fit_edge`, which squeezes into a ribbon near the right and bottom edges. Only 3 plugins have a clamp gate (`ui_tooltip_clamp_check.js`).
- **CSS drift:**
  - `max-width` takes 14 values (200–420 px).
  - `z-index` takes 12 values (20 → 10001).
  - The state class is `.tooltip.show` ×28 vs `.tooltip.visible` ×17.
- No renderer uses `innerHTML` for content; every hit is a comment.

### 2.4 Hover-help switch
- **Ids:** `#tips-toggle` 35, `#help-toggle` 11. `check-i18n` [16] accepts both.
- **Init:** 43 bodies over `applyTipsEnabled` (33 plugins, 4 variants), `initializeTipsToggle` (21 plugins, 20 variants), `setTooltipsEnabled` (8, 7), `initializeHelpToggle` (8, 3), `setupTipsToggle` (6, 6) and `initTipsToggle` (5, 3).
  - The 20 `initializeTipsToggle` "variants" are **one shape**. They differ only in the per-plugin `localStorage` key, which is exactly the kind of difference a kit parameter absorbs.
- **Persistence, three schemes:**
  - `localStorage`: 32 plugins, 31 keys.
  - C++ native: 11. Native names are split: `set/getTooltipsEnabled` ×10 vs `set/getTipsEnabled` in O-simpleSampler.
  - None, default **OFF**: O-Gain `js/app.js:258`, O-IntonationPad `js/app.js:253`, O-Polystutter `js/app.js:255`. This contradicts the canon (ON, persisted) and the memory guidance (OFF blanks tip-render gates).
- **Bug: O-Strata reads and writes `'oprism.tipsEnabled'`** (`plugins/O-Strata/Source/ui/public/index.html:2739,2745`).
  - JUCE serves every plugin at `juce://juce.backend/` on macOS (`~/JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.cpp:613-630`) and configures no per-plugin data store.
  - So Prism and Strata share one stored preference in a host.

### 2.5 Settings popover + language select
- **Markup:** all 46 have `#gear-btn`, `#settings-popover` and `#lang-select` (en / fr / zh-Hans).
  - Exact / structural variants: popover 21 / 17, gear 12 / 5, lang-select 11 / 8.
  - CSS: `.settings-row` gap is 10 px (×17), 8 px (×15) or 12 px (×9).
- **JS:** 45 plugins declare a popover function (Formant does it inline). 18 distinct bodies; `setSettingsPopoverOpen` is the 1 identical piece (×24).
- **Dismissal:** document `mousedown` in 24 plugins, `pointerdown` in 18, `click` in 3.
  - The `click` group is Prism and Strata (`de23339d`, 14 lines: no Escape, no `aria-expanded`, no focus return) and Tremolo.
- **Bug (confirmed by code reading; needs a hands-on check) — popover or preset dropdown stays open when a knob is pressed.** These plugins combine a document `mousedown` dismisser with a `preventDefault()`ed knob `pointerdown`, which suppresses compat mouse events (`pattern_pointerdown_preventdefault_kills_document_mousedown`):
  - O-ReverseDelay: `js/app.js:2066` (popover) and `:1901` (preset dropdown); knob `pointerdown` at `:658` calls `preventDefault()`.
  - O-Tapestop: `js/app.js:1129`.
  - O-Bitrot: `index.html:2437`.
  - O-MultiBandCompressor: `js/app.js:1622,1661`.
  - O-MicrotonalSampler: `Resources/ui/js/sampler-app.js:4319`.
  - O-SpectralShaper: `Resources/ui/js/app.js:1263`.
  - O-Polystutter: `js/app.js:207`.
  - O-FreqPulse: `Resources/ui/js/app.js:1563`.
- **Latent in 16 more** `mousedown`-dismisser plugins: it turns live the moment their knobs adopt the canon pointer-capture binding. Converging knobs without converging the popover creates the bug.

### 2.6 i18n engine
- The canon region is identical in 46/46 (`check-i18n` [6]); `tr()` has 2 variants differing only by a local variable name.
- Tables are per-plugin by design: 51.5k lines, en / fr / zh-Hans everywhere.
- This is the purest extraction: ~4.4k byte-identical lines.
- **But** the runtime does `import … from './i18n.js'` and sets `window.__setLanguage`, `__setLabel` and `__reapplyI18n`. A kit version must take the tables as parameters (`createI18n({LANGUAGES,I18N,LABELS,TIP_BINDINGS,tr})`).

### 2.7 Preset bar
- **Native API families:**
  - preset-manager module natives (`PresetManager` JS): 21 plugins.
  - Category API (`getPresetListWithCategories`/`loadPresetFromCategory`): Bells, Formant, Prism, Strata.
  - Catalog API (`getPresetCatalog`/`applyFactoryPreset`): simpleReverb, simpleWavetable, and simpleAmbisonics (which also uses `PresetManager`).
  - Bespoke: IntonationPad (`loadPrevPreset`, `renamePreset`), MicrotonalSampler (`saveCurrentPreset`), Bowed / Lyrica / Marimba / Wind (module-like names, no `PresetManager`).
  - That makes **33 plugins with a preset bar** (21 + 4 + 2 + 6).
  - Lesson recipes only (`applyFactoryPreset`, no bar): simpleAdditive, simpleBeatmaker, simpleGrain, simpleSampler, simpleSubtractive.
  - No preset UI at all: 8.
- **Markup ids:** kebab `preset-prev/next/name/dropdown` (~15), camel `presetPrev/presetName/presetDropdown` (~10), and bespoke.
- **`preset-manager.js` delivery** (module `9d67b2da`, 461 lines):
  - Direct embed: Contrabass, ReverseDelay.
  - Configure copy: 14.
  - 11 tracked copies: 5 MATCH, plus 6 FORKs that are *all distinct* (Bass, Comp, Polystutter, SpectralShaper at 406 lines with 4 different hashes; DigiDelay 377; FreqPulse 447).
  - O-Orbit and O-MicrotonalSampler also carry dead configure-copy leftovers under `Source/ui/public/modules/`, because the copy destination is hard-coded but their UI root is `Resources/ui`.

### 2.8 Toggles, combos, meters, chrome
- **Toggles and combos:** 13 binder names, 51 definitions, 47 distinct. `bindToggle` 11 / 11, `bindCombo` 8 / 5, segment binders in ReverseDelay, Tapestop, simpleWavetable and simpleAmbisonics.
- **Meters and visualizers:**
  - Bespoke in 28 plugins.
  - `modules/metering/vu-meter/js/vu-meter.js` (478 lines) has no UI consumer. The `.vu-meter` CSS in AnalogSaturation and Marimba is hand-rolled, and module.yaml's `used_by` lists old plugin names.
  - Data transport splits three ways: C++ event emit with plugin-specific event names (~25), `evaluateJavascript` push (10), and native polling (ReverseDelay `getGrainMeter`).
- **Chrome:**
  - Wordmarks: "Ouaricon X" (Bass, Bells, Freeze, FreqPulse, IntonationPad), "O-X", hair-spaced "O – X", upper-case "O-PRISM" / "O-STRATA" / "O-TEXTURE". 19 plugins have no `<h1>`.
  - Botanical overlay image in 22; fleuron in 13.

### 2.9 JUCE frontend lib
- **`index.js`:** 34 byte-identical to `~/JUCE` 8.0.15 (`01ff05e3`, unchanged since 8.0.9). 7 CRLF (Bassoon, Bowed, Chorus, Reed, SpectralShaper, TextureForge, Wind). 5 with one extra blank line (Bass, IntonationPad, MBC, simpleReverb, Tremolo). **All functionally upstream.**
- **`check_native_interop.js`:** 30 upstream, 6 CRLF.
  - **7 patched** (AnalogSaturation, Bass, Comp, Gain, IntonationPad, Marimba, MBC): `Backend.removeEventListener` calls `this.listeners.removeEventListener(eventId, id)` against a destructuring `([eventId, id])` signature, which could never remove a listener (e.g. `plugins/O-Gain/Source/ui/public/js/juce/check_native_interop.js:130`).
  - **3 replaced by a 5-line stub** (Detune, Freeze, SpectralShaper).
  - Both are **inert in a real WebView**: JUCE's C++ injects its own `Backend` before the page runs (`juce_WebBrowserComponent.cpp:159-160`), and the page file only installs `if (typeof window.__JUCE__.backend === "undefined")`. They only matter for headless stubs and future JUCE upgrades.
- **Glue around it:**
  - `import * as Juce from "./juce/index.js"` in **46/46**. ES modules are universal, so a kit can be ES modules.
  - `loadParameterDefaults`: 11 plugins, 5 variants (C++ default fetch).
  - Resource providers: **45 hand-written `if (url == …)` chains + 1 table (MBC), 0 generic lookups**.
  - BinaryData namespace: default `BinaryData` in 38, `UIBinaryData` in a few. The `makeBinaryResource` helper exists in 15 editors only.

---

## 3. The existing "canon" system: copy-paste with a gate

**How it works.**

- **i18n (hard gate).**
  - `scripts/i18n-canon.js` holds the applyI18n…initI18n block verbatim between sentinels inside a block comment (`:113-226`). The module reads its own source to recover it.
  - `scripts/check-i18n.js` assertion [6] extracts the same region from each plugin, strips comments, normalizes whitespace and compares. The import line is asserted separately.
  - Exit code = number of failed assertions, across 16 assertions (table completeness, review flags, inertness, embed/serve wiring [8]/[12], label/attr/JS-string coverage [10–13], one hover-help switch [16] with a vacuity check).
  - Result: **46/46 on canon v2, 1 variant.**
- **UI components (report-only).**
  - `scripts/ui-canon.js` stores 4 components as `//|`-prefixed lines machine-emitted from O-ReverseDelay `ed1fc465` (`:81-111`): tooltip renderer (5 fns), hover-help (2), `bindKnob` and `updateKnobVisual`.
  - `scripts/check-ui-canon.js` lexes each plugin's JS with a template- and regex-aware lexer, extracts `function NAME(){}` declarations and classifies each as `canon` (exact after comment/whitespace strip), `shape` (only string literals differ), `v:<hash>`, `nd` (bound via `const NAME =`) or `absent`.
  - It also hashes tracked module copies against `modules/` (MATCH/FORK, ships, sync) and lists direct embeds.
  - **Exit 0 always.** CI runs it `continue-on-error` (`.github/workflows/ui-static-gates.yml:56-60`).

**Current output** (HEAD snapshot, `node scripts/check-ui-canon.js --repo-root <snap>`):

| Component | canon | shape | variant (distinct) | non-decl | absent |
|---|---|---|---|---|---|
| tooltip-renderer | 1 | 2 | 39 (29) | 0 | 4 |
| hover-help-init | 1 | 1 | 39 (29) | 4 | 1 |
| knob-binding | **0** | 0 | 25 (23) | 2 | 19 |
| knob-visual | **0** | 0 | 23 (19) | 0 | 23 |

Module copies: 18 tracked, 5 MATCH, 13 FORK (preset-manager ×6, tuning-panel.js ×4, tuning-panel.css ×3). Direct embeds: 39 plugins.

**Limits.**

1. **It detects drift; it cannot propagate a fix.** Each fix is still N commits (§0.3). A report-only gate does not even stop new drift: value entry fragmented to 21 variants within the same day it was written.
2. **The canon goes stale as its reference evolves.** Re-anchoring is manual (emit → paste → `--self-test` N4). The 10-19 sweep invalidated the knob canon suite-wide, and nobody re-anchored it.
3. **Name-keyed and function-granular.**
   - Arrow functions, closures and classes read as `nd` or `absent` (Bitrot, Contrabass, Octagon, Orbit; SpectralShaper's `RotaryKnob` class).
   - CSS and markup are not compared: tooltip `z-index`/`max-width`, `.settings-row` gaps, popover markup with 21 variants.
   - Only 4 components are covered. Popover, value entry, preset dropdown, toggles and meters are uncovered.
4. **Exact text cannot express per-plugin configuration.**
   - 20 "variants" of `initializeTipsToggle` are one shape plus a storage key.
   - Canon equality therefore forces identical ids and keys, which conflicts with per-plugin storage namespacing.
5. **Hand-copied gates repeat the problem.** There are 22 hand-copied `ui_tip_render_check.js` today; the 09-24 review found all 23 then-existing copies distinct. Only 3 plugins have a tooltip clamp gate, and none of them uses the cursor-following renderer that is exposed to the edge squeeze.

**Verdict.** The canon system is the right *detector*, and it proves that a single source converges (i18n). As a *distribution* mechanism, copy-plus-gate scales O(plugins × components) per change. A kit keeps the gate but changes what it checks, from "is your copy identical?" to "do you import kit vN and define none of its functions locally?". That is trivially enforceable and needs no canon re-anchoring.

---

## 4. Distribution mechanisms: can `modules/` carry a UI kit?

| Mechanism | How | Consumers today | Carries | Drift | Problems |
|---|---|---|---|---|---|
| **Direct embed** | `${CMAKE_SOURCE_DIR}/modules/...` listed in `juce_add_binary_data` SOURCES + a literal `getResource` branch + `<link>`/`<script>` | **39 plugins**: eb-garamond 37, tuning-panel 5, preset-manager 2, drop-streaming 1, Marimba effect units 3 | JS, CSS, fonts, images | **None by construction** | Per file per plugin: 1 CMake line + 1 C++ branch + 1 HTML include, and there is no CMake helper (`modules/ui/eb-garamond/README.md` "There is no CMake helper"). The eb snippet assumes `UIBinaryData` + `makeBinaryResource`, which match a minority of editors, so each adoption is hand-adapted. `serve-ui.js` *depends* on the literal one-branch-per-URL shape (it pairs URL literal ↔ symbol within ~400 chars) |
| **Configure-time copy** | `ouaricon_add_module()` → `configure_file(COPYONLY)` into `${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/modules` (`modules/cmake/OuariconModules.cmake:104-118`), which is gitignored (`.gitignore:26`) | preset-manager ×14, drop-streaming ×3 | `js/*.js` only | None at build time | **Writes into the source tree.** It ignores `Resources/ui` roots, leaving dead copies in O-Orbit and O-MicrotonalSampler. The embedded file is **untracked**, so fresh clones, `git archive` and CI lack it, and `check-i18n` [12] fails for 11 plugins. **That is why UI Static Gates has been red on main since 2026-09-28.** 5 copies were committed before the ignore rule, which leaves a mixed state |
| **Frozen tracked copy** | Copy committed in the UI root | preset-manager ×6 FORK + 5 MATCH; tuning-panel 4 + 3 FORK | anything | **All drift** | The source of every module fork |
| *(planned, never built)* | `resource-provider` "maps flattened BinaryData names back to URL paths" | `modules/registry.yaml:62-72`; **directory absent**, `used_by: []` | — | — | Exactly the piece a kit needs. Also: `webview-relay-manager` exists but has 0 consumers |

**Recommendation: direct embed, made cheap, versioned and opt-in.**

1. **Module layout.** `modules/ui/kit/v1/` holds flat, unique basenames, because JUCE flattens paths into symbol names. ES modules + CSS: `tokens.css`, `i18n-runtime.js`, `tooltip.js/.css`, `hover-help.js`, `settings-popover.js/.css`, `value-entry.js`, `range.js`, `knob.js` (+ `knob-a.css`, `knob-b.js` renderers), `controls.js`, `canvas.js`, `level-meter.js`. It reuses `eb-garamond` as a dependency and serves JUCE's own `index.js` / `check_native_interop.js` from `${JUCE_MODULES_DIR}/juce_gui_extra/native/javascript/`, which pins the JS frontend to the C++ backend version actually built against.
2. **CMake helper `ouaricon_add_ui_kit(<Plugin> VERSION 1 [COMPONENTS …])`.** It creates a *separate* binary-data target `<Plugin>_UiKit` with `NAMESPACE OuaUiKit` (a unique namespace avoids the duplicate-symbol trap the README warns about) and links it. Plugin edit: 1 line.
3. **Header-only C++ resolver `ouaricon::uikit::serve(url)`.** It maps `/kit/v1/<file>` to the resource through `OuaUiKit::originalFilenames` / `getNamedResource` and returns MIME types (including `font/woff2`). Plugin edit: 1 line in `getResource()`. This is the unbuilt `resource-provider`.
4. **Teach the tools once:**
   - `scripts/serve-ui.js` `buildRoot` places `modules/ui/kit/vN/*` at `/kit/vN/` when CMake calls the helper.
   - `check-i18n` [12] and scan-set (c), plus `check-ui-canon`, read kit sources.
   - Hand-built per-plugin gate trees must switch to `serve-ui`, otherwise they 404 kit files (`pattern_hand_built_gate_tree_404s_module_embedded_assets`).
5. **Versioning, to respect "never a sweep" and "don't modify working functionality".**
   - A plugin pins `VERSION n`, so a kit edit never silently changes a shipped plugin on its next unrelated rebuild.
   - A fix becomes a one-line pin bump per plugin via `/improve`, instead of re-applying 40–80 lines × N.
   - `registry.yaml` `used_by` + `regen-registry-used-by.sh` give the burn-down.
   - *Alternative:* allow in-place patch releases within v1 for pure bug fixes. Faster propagation, but every rebuild then changes behaviour. Not recommended, given the regression sensitivity.
6. **Retire configure-copy for UI JS.** Convert the 14 + 3 `ouaricon_add_module` JS consumers to direct embed (or the kit). This is also the fastest way to turn CI green.
7. **Gate:** for migrated plugins, "imports `/kit/vN/x.js` and defines no local `bindKnob`/`setupTooltips`/…" becomes a hard assertion. It stays report-only for the rest.
8. **Generator:** `ui-finalization-agent` emits kit imports, so new plugins are born on the kit.

---

## 5. CSS token census (palette, fonts, sizes)

Measured over shipped CSS: stylesheets + inline `<style>` + `style=""`, excluding vendored module CSS.

### Colours

- **338 distinct colours** (hex + `rgb()`), **229 used by exactly one plugin**. Per plugin: min 7, median 24, max 46.
- House palette from `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` has 22 hex values. All 22 appear somewhere. Reach:

| Colour | Plugins |
|---|---|
| `#8B7355` | 45 |
| `#3C2F2F` | 43 |
| `#F5E6D3` | 41 |
| `#5C4033` | 40 |
| `#8BA870` | 38 |
| `#6B8E4E` | 34 |
| `#FFF8DC` | 34 |
| `#C9A27B` | 32 |
| `#6A5641` (AA) | 21 |
| `#715D45` (AA) | 14 |
| `#4E6839` (AA) | 14 |
| `#7A654B` (AA) | 1 |

- **Divergence from the house palette:** in **14 plugins ≥40% of colour occurrences are off-house** — AnalogSaturation, Bass, Chorus, Detune, DigiDelay, Formant, Freeze (67%), FreqPulse, IntonationPad (59%), MicrotonalSampler, Octagon (89%, dark theme by design), Polystutter, simpleAmbisonics, simpleWavetable.
- Three plugins use fewer than 5 house colours: Octagon 0, Freeze 1, Chorus 3.
- **Near-misses = forked family palettes:** **37 plugins** use colours within ΔE < 6 of a house colour without matching it (70 such colours). The forks are by lineage:

| Off-house | House | Plugins | Lineage |
|---|---|---|---|
| `#F5F0E6` | `#FAF0E6` | 10 | simple* |
| `#6B5340` | `#6A5641` | 10 | simple* |
| `#4A6B35` | `#55703E` | 8 | winds/Prism/Strata |
| `#E6D4BA` | `#E8D5B7` | 7 | simple* |
| `#D4C4AD` | `#D4C4B0` | 4 | winds |
| `#E8D5BE` | `#E8D0B8` | 4 | winds |
| `#8BA370` | `#8BA870` | ×6 in O-Orbit | — |

### Tokens

- **441 distinct custom-property names** (153 at the 09-24 review), **251 used by one plugin**. 43 plugins define tokens; AnalogEQ, Freeze and Gain define none.
- **Two vocabularies:**
  - **A** `--bg-paper`/`--brown-text`/`--brown-border`/`--brown-frame`/`--green-*`: 19 plugins (ReverseDelay, Tapestop, simple*, winds, Bitrot, Emulator, Marimba).
  - **B** `--ink`/`--paper`/`--walnut`/`--sage`: 15.
  - Both: 3. Own vocabulary: 6 (Bells, Chorus, FreqPulse, MicrotonalSampler, Octagon, TextureForge). None: 3.
- **The same colour under 5–10 names:**

| Colour | Names | Names used |
|---|---|---|
| `#6B8E4E` | 10 | `--green-mid` 20, `--sage` 5, `--knob-fill` 4, `--leaf` 3, … |
| `#8B7355` | 9 | — |
| `#5C4033` | 9 | — |
| `#C9A27B` | 9 | — |
| `#F5E6D3` | 7 | — |
| `#3C2F2F` | 5 | `--brown-text` 23, `--ink` 14, … |

- **52 token names carry conflicting values** across ≥4 plugins. Examples:
  - `--ink`: 5 values / 20 plugins.
  - `--text-muted`: 3 values / 19.
  - `--paper`: 3 / 16.
  - `--serif`: 3 / 15.
  - `--tuning-text-muted`: 6 / 6.

### Fonts

- **21 distinct resolved stacks.** **36 plugins** resolve to the canonical `'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif`, and 38 lead with EB Garamond.
- 6 still lead with bare `Garamond`: AnalogEQ, Gain, Reed, Tapestop, simpleFM, simpleGrain. Gain, Reed and simpleFM are being migrated in the uncommitted working tree; AnalogEQ, Tapestop and simpleGrain are not.
- Octagon uses Iowan Old Style by design.
- **Font token name is split:** `--font-serif` 23 vs `--serif` 15 vs `--font-primary` 1 vs none 7.
- The brand template still says `'Garamond', 'Times New Roman', serif` (`aesthetic.md:125`), contradicting `modules/ui/eb-garamond/README.md`.
- **EB Garamond embedded in 37 at HEAD** (42 in the working tree). Not at HEAD: AnalogEQ, Bassoon, Freeze, Gain, Octagon, Reed, Tapestop, simpleFM, simpleGrain.
- **Canvas text in EB Garamond with no `document.fonts` wait or repaint:** Comp, GrainScatter, Lyrica, Orbit, Prism, Strata, simplePhysicalModelSynth, simpleSubtractive. Static canvases may keep Georgia (`pattern_canvas_text_needs_font_load_repaint`); needs a visual check.

### Sizes and spacing

- **28 distinct px font sizes.** **14 plugins still declare < 9 px**: AnalogEQ, IntonationPad, Lyrica, Marimba and Reed at 7 px; Bassoon at 8.5 px; Bells, Bitrot, Gain, Polystutter, Tapestop, Tremolo, simpleFM and simpleGrain at 8 px. The 09-24 review counted 34.
- **Shared-rule census:** 208 CSS rules are verbatim in ≥3 plugins. The most shared:
  - `.settings-popover[hidden]` ×42.
  - `.knob-value-input` spin-button reset ×39.
  - `.tooltip.show` ×28.
  - `.knob:hover{scale(1.05)}` ×17.

### Assets

- Paper textures: **19 copies, 4 distinct**. A 202 KB JPG appears ×13 under 4 filenames (`paper1.jpg`, `paper.jpg`, `paper-bg.jpg`, `paper-background.jpg`), and a 1.1 MB JPG ×4.
- `insects.png` ×7 (466 KB).
- 29.7 MB of images in UI trees against 16.4 MB unique: **13.3 MB duplicated in the repo**. Per-binary size is unaffected.

---

## 6. Ranked extraction candidates

Ranking = (copies × convergence × bug payoff) ÷ (effort × regression risk). Effort assumes Claude-assisted work: kit build plus per-plugin migration via `/improve`, a rebuild and a check.

| Rank | Candidate | Plugins | Lines removed (approx.) | Behaviour change | Effort | Why this rank |
|---|---|---|---|---|---|---|
| 0 | **Prereq:** convert configure-copy UI JS (preset-manager ×14, drop-streaming ×3) to direct embed + decision record superseding `260826-ieq-CONTEXT.md:51` | 17 | — | none | **S** (~1 d) | Turns CI green; establishes the one carrier |
| 1 | **Kit infrastructure:** CMake helper, `OuaUiKit` resolver, serve-ui / check-i18n / check-ui-canon awareness, versioned `modules/ui/kit/v1`, registry | — | — | none | **M** (2–3 d) | Every later item depends on it |
| 2 | **JUCE frontend lib** served from JUCE's module dir | 46 | 33.3k | none (functionally identical; interop forks are inert) | **S** (scripted; sweep-safe) | Largest single duplication; removes the 10 forked interop files; pins JS to the C++ version |
| 3 | **i18n runtime** as `createI18n(tables)` | 46 | ~4.4k | none (byte-identical today) | **S** (+5 min/plugin) | Lowest risk. `check-i18n` [6] becomes an "imports kit" check. Needs the decision record (rank 0) |
| 4 | **tokens.css** with house palette + AA inks + `--serif` / `--font-serif` aliases, *both* vocabularies as aliases, spacing scale + eb-garamond | 43 | CSS `:root` blocks; resolves 52 conflicting names over time | visual only if a plugin drops its own values | **S** author / **M** per plugin (screenshot diff + `measure-ui --contrast`) | Stops palette forking at the source; the alias layer makes adoption non-breaking |
| 5 | **Settings popover + language select** (JS + CSS + markup partial) | 46 | ~1.4k JS + ~2.5k CSS | yes (dismissal) | **S–M** | Fixes 8 dismiss-dead plugins and pre-empts 16 latent ones; small surface |
| 6 | **Tooltip renderer + hover-help switch** (config: `storageKey` or native, `toggleId`, `mode: anchor|cursor`) | 46 | ~5.8k JS + ~1.4k CSS | yes (placement; persistence default for Gain, IntonationPad, Polystutter) | **M** kit / **S** per plugin | Fixes the 29-renderer squeeze class, the Prism/Strata key collision and 3 default-OFF plugins. Keep a `cursor` mode to avoid a UX shock |
| 7 | **Value entry + range math** (`attachValueEntry`, `scaledToNorm`, `nudgeStep`, parse helpers; plugins keep their PARSE/FORMAT tables) | 45 | ~3.4k | small (formatter contract unified) | **M** | Already 84% shared and the most recent sweep, so cheap now and expensive later |
| 8 | **Knob interaction core** (pointer capture, wheel gesture, keys, Alt-reset, dblclick → value entry) + per-family visual renderers (A / A′ / B / C) | 44 | ~3.5k JS + knob CSS | **yes** (core UX; DAW hands-on per plugin) | **L** | Biggest a11y and bug payoff (22 without capture, 27 without arrow keys) but the highest regression risk; do it lineage by lineage (simple* → ReverseDelay/Tapestop → effects → Prism/Strata → winds) |
| 9 | **Preset bar UI** over `PresetManager` | 33 | ~1.8k JS + CSS | yes | **L** (needs adapters for the 4 native API families) | Fragmented (4.5% shared). Do it after rank 0 removes the forks |
| 10 | **Toggles / combos / segments** | 32 | ~1k JS | yes | **M** | Low convergence (8%); modest payoff |
| 11 | **Canvas helper** (`makeCanvas` DPR + font-ready repaint) + generic level meter; delete or adopt `vu-meter.js` | 10 / 28 | small | small | **S** | Fixes the 8 canvas-font cases; meters are otherwise bespoke |
| 12 | **Shared naturalist assets** (paper JPGs, insects.png) via direct embed | 19 | 13.3 MB in repo | none | **XS** | Repo hygiene only |

**Suggested phasing:**

- Ranks 0–3 have no behaviour change and can ship as one infra milestone.
- Ranks 4–7 go per plugin through `/improve` with a minor bump, lineage by lineage, so one migration covers 6 siblings.
- Rank 8 goes last, with hands-on DAW checks.

At each step the generator is updated, so new plugins never re-enter the copy cycle.

---

## 7. Risks and decisions to flag

- **Decision reversal.** The "no shared module" rule is recorded in `260826-ieq-CONTEXT.md:51-53` (risk register `:138-142`), and the 09-24 review marks a retrofitted shared library "Not recommended — requires decision". Write the superseding decision before any code.
- **Trunk-based builds.** All 46 build from `main`. An unversioned kit means any kit edit changes every consumer's next build. Pinned versions make kit changes explicit per plugin.
- **In-flight work.** Five plugins carry uncommitted R4/R5 edits from 2026-09-25. Land or park them before kit migration touches the same files.
- **CI is already red for unrelated reasons.** fr-lint has 1 finding (`node scripts/i18n-fr-lint.js` exits 2 locally). Fixing the copy mechanism alone will not turn the job green.
- **Headless gates.** 22 per-plugin `ui_tip_render_check.js` copies plus the hand-built trees in the 3 clamp gates will 404 kit files unless they switch to `serve-ui.buildRoot`.

---

## Appendix A — per-plugin burn-down matrix (HEAD)

**Column key:**

- **Root:** S = `Source/ui/public`, R = `Resources/ui`.
- **Dup≥2:** share of the plugin's normalized lines in a 4-line window that also appears in another plugin.
- **Capture/Arrows:** page-level `setPointerCapture` / arrow-key handling.
- **Value entry:** V1…V21 identity groups (V1 ×6 simple*, V2 ×5, V3 ×4, V4 ×3).
- **Tooltip:** `(nd)` = arrow-function renderer.
- **Popover dismiss:** **DEAD** = `mousedown` dismisser + `preventDefault()`ed `pointerdown` (§2.5).
- **Preset:** PM.js = PresetManager module JS; category / catalog / lessons / bespoke native APIs.
- **Tokens:** A / B vocabulary · font token name.
- **JUCE idx/interop:** up = upstream, CRLF, +1ln, PATCH, STUB.

| Plugin | Root | Authored mk/css/js | Dup≥2 | Knob fam | Capture/Arrows | Value entry | Tooltip | Help id · persist | Popover dismiss | Preset | Tokens · font var | EB Gar. | JUCE idx/interop |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| AnalogEQ | S | 307/879/985 | 36% | A' | Y/n | V5 | cursor | tips·LS `oaeq.tipsEnabled` | pointerdown | PM.js | none·— | n | up/up |
| AnalogSaturation | S | 197/785/757 | 40% | A' | n/n | V5 | cursor | tips·LS `oasat.tipsEnabled` | mousedown | — | B·font-serif | Y | up/PATCH |
| Bass | S | 174/917/805 | 73% | A' | n/n | V3 | cursor | tips·LS `obass.tipsEnabled` | mousedown | PM.js | A+B·font-serif | Y | +1ln/PATCH |
| Bassoon | R | 297/910/754 | 63% | C | n/n | — | cursor | tips·LS `obsn.tipsEnabled` | mousedown | — | A·— | n | CRLF/CRLF |
| Bells | R | 610/1533/1722 | 33% | B+R | Y/n | V4 | cursor | tips·LS `obells.tipsEnabled` | mousedown | category | own·serif | Y | up/up |
| Bitrot | S | 525/1151/996 | 43% | A' | Y/n | V3 | (nd) | help·C++ | mousedown **DEAD** | PM.js | A·serif | Y | up/up |
| Bowed | R | 421/1132/1589 | 45% | C | n/n | V10 | cursor | tips·LS `obow.tipsEnabled` | mousedown | bespoke | A·font-serif | Y | CRLF/CRLF |
| Chorus | S | 260/907/888 | 42% | B | Y/n | V3 | cursor | tips·LS `ochor.tipsEnabled` | pointerdown | PM.js | own·font-serif | Y | CRLF/CRLF |
| Comp | S | 351/818/1297 | 40% | B | n/n | V3 | cursor | tips·LS `ocomp.tipsEnabled` | mousedown | PM.js | B·font-serif | Y | up/PATCH |
| Contrabass | S | 434/1163/1432 | 24% | C | n/n | V6 | (nd) | help·C++ | mousedown | PM.js | B·serif | Y | up/up |
| Detune | S | 346/1144/1076 | 38% | B+R | n/n | V2 | cursor | tips·LS `odet.tipsEnabled` | mousedown | PM.js | B·font-serif | Y | up/STUB |
| DigiDelay | S | 255/947/1225 | 42% | B | n/n | V2 | cursor | tips·LS `odd.tipsEnabled` | mousedown | PM.js | B·font-serif | Y | up/up |
| Emulator | S | 182/738/706 | 61% | A' | Y/n | V11 | cursor | tips·LS `oemu.tipsEnabled` | pointerdown | PM.js | A·serif | Y | up/up |
| Formant | S | 478/1258/2342 | 13% | A' | Y/Y | V2 | cursor | tips·LS `ofmt.tipsEnabled` | (nd) | category | B·font-serif | Y | up/up |
| Freeze | S | 283/739/778 | 32% | B | n/n | — | cursor | tips·LS `ofrz.tipsEnabled` | mousedown | — | none·— | n | up/STUB |
| FreqPulse | R | 206/1224/2031 | 25% | R | n/n | V12 | anchor+pin | tips·C++ | mousedown **DEAD** | PM.js | own·font-serif | Y | up/up |
| Gain | S | 318/1142/1206 | 24% | C | n/n | — | anchor+pin | tips·**none** | mousedown | — | none·— | n | up/PATCH |
| GrainScatter | S | 445/808/1295 | 26% | A' | n/n | V7 | cursor | tips·LS `ogs.tipsEnabled` | mousedown | — | B·font-serif | Y | up/up |
| IntonationPad | S | 285/1580/2054 | 30% | B | n/n | V13 | anchor+pin | tips·**none** | mousedown | bespoke | B·font-serif | Y | +1ln/PATCH |
| Lyrica | R | 799/2282/3590 | 29% | B+R | n/n | V4 | anchor+pin | tips·C++ | mousedown | bespoke | B·font-serif | Y | up/up |
| Marimba | S | 341/1389/1885 | 37% | A' | n/n | V6 | anchor+pin | tips·C++ | mousedown | bespoke | A·font-serif | Y | up/PATCH |
| MicrotonalSampler | R | 743/2649/4406 | 8% | B+R | Y/Y | V14 | cursor | tips·LS `oms.tipsEnabled` | mousedown **DEAD** | bespoke | own·font-serif | Y | up/up |
| MultiBandCompressor | S | 506/1588/2203 | 21% | A+R | Y/Y | V15 | anchor+pin | tips·C++ | mousedown **DEAD** | PM.js | B·font-serif | Y | +1ln/PATCH |
| Octagon | S | 997/2088/4839 | 10% | R | Y/Y | V16 | (nd) | tips·C++ | pointerdown | PM.js | own·serif | n | up/up |
| Orbit | R | 423/1019/1709 | 23% | A' | n/n | V7 | (nd) | help·C++ | mousedown | PM.js | B·font-serif | Y | up/up |
| Polystutter | S | 627/1241/1703 | 20% | - | n/n | — | anchor+pin | tips·**none** | mousedown **DEAD** | PM.js | B·font-serif | Y | up/up |
| Prism | S | 815/1885/3430 | 74% | B+R | Y/Y | V17 | cursor | tips·LS `oprism.tipsEnabled` | click | category | B·font-serif | Y | up/up |
| Reed | R | 432/1072/905 | 49% | C | Y/n | — | cursor | tips·LS `oreed.tipsEnabled` | pointerdown | — | A·— | n | CRLF/CRLF |
| ReverseDelay | S | 649/1693/2320 | 37% | A | Y/Y | V8 | anchor+pin | tips·LS `ord.tipsEnabled` | mousedown **DEAD** | PM.js | A·serif | Y | up/up |
| SpectralShaper | R | 196/1170/3146 | 23% | A' | Y/Y | V2 | anchor+pin | tips·C++ | mousedown **DEAD** | PM.js | A+B·serif | Y | CRLF/STUB |
| Strata | S | 780/1796/3755 | 72% | B+R | Y/Y | V18 | cursor | tips·LS `oprism.tipsEnabled` | click | category | B·font-serif | Y | up/up |
| Tapestop | S | 418/1226/1897 | 42% | A | Y/Y | V8 | anchor+pin | help·C++ | mousedown **DEAD** | PM.js | A·serif | n | up/up |
| Texture | S | 296/754/1044 | 37% | D | Y/n | V19 | cursor | tips·LS `otex.tipsEnabled` | pointerdown | — | B·font-serif | Y | up/up |
| TextureForge | S | 230/904/1389 | 31% | A' | n/n | V20 | cursor | tips·LS `otf.tipsEnabled` | pointerdown | — | own·font-primary | Y | CRLF/CRLF |
| Tremolo | S | 194/1159/1221 | 55% | A'+R | n/n | V2 | cursor | tips·LS `otrem.tipsEnabled` | click | PM.js | A+B·font-serif | Y | +1ln/up |
| Wind | R | 412/1339/1462 | 55% | B+C | Y/n | V4 | cursor | tips·LS `owind.tipsEnabled` | pointerdown | bespoke | A·serif | Y | CRLF/CRLF |
| simpleAdditive | S | 284/858/1181 | 70% | A | Y/Y | V1 | cursor | help·LS `osadd.tipsEnabled` | pointerdown | lessons | A·serif | Y | up/up |
| simpleAmbisonics | S | 373/798/4754 | 23% | A | Y/Y | V9 | anchor+pin | tips·LS `osamb.tipsEnabled` | pointerdown | PM.js | A·serif | Y | up/up |
| simpleBeatmaker | S | 222/714/1298 | 37% | A | Y/Y | V1 | cursor | help·LS `osbm.tipsEnabled` | pointerdown | lessons | A·font-serif | Y | up/up |
| simpleFM | S | 294/907/1129 | 74% | A | Y/Y | — | cursor | help·LS `osfm.tipsEnabled` | pointerdown | PM.js | A·— | n | up/up |
| simpleGrain | S | 343/1205/1565 | 69% | A | n/Y | V1 | cursor | help·LS `osg.tipsEnabled` | pointerdown | lessons | A·— | n | up/up |
| simplePhysicalModelSynth | S | 325/731/1236 | 70% | A | n/Y | V1 | cursor | help·LS `opms.tipsEnabled` | pointerdown | PM.js | A·serif | Y | up/up |
| simpleReverb | S | 369/503/1763 | 44% | A | Y/Y | V9 | anchor+pin | tips·LS `osrev.tipsEnabled` | pointerdown | catalog | A·serif | Y | +1ln/up |
| simpleSampler | S | 317/1003/1679 | 62% | A | Y/Y | V1 | cursor | help·C++ | pointerdown | lessons | A·serif | Y | up/up |
| simpleSubtractive | S | 431/990/1339 | 63% | A | n/Y | V1 | cursor | help·LS `ossub.tipsEnabled` | pointerdown | lessons | A·font-serif | Y | up/up |
| simpleWavetable | S | 332/830/2485 | 30% | A | Y/Y | V21 | anchor+pin | tips·LS `oswt.tipsEnabled` | pointerdown | catalog | A·serif | Y | up/up |

---

## Appendix B — method and reproducibility

All scripts are read-only and live in `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/39983b81-85a2-44fc-8aff-422a28a35353/scratchpad/scripts/`. Outputs are in `…/scratchpad/work/`.

- **Snapshot:** `git archive HEAD` of `plugins/*/{Source/ui,Resources/ui,CMakeLists.txt,Source/PluginEditor.*,tests/ui*}`, `modules`, `scripts`, `.github` → `…/scratchpad/snap/`.
- **UI roots:** `scripts/serve-ui.js resolveUiRoot`, read from CMake SOURCES.
- **Lexing and extraction:** reuses `scripts/check-ui-canon.js` (`lexJs`, `extractFunctions`, `normExact`, `normShape`, `pluginSources`). Vendored `js/juce/`, `modules/` copies, module basenames and bundles are excluded; inline `<script>` and `Source/ui/src` are included.
- **Per-script notes:**
  - `funcs.js`: function census — names defined in ≥2 plugins, distinct exact/shape hashes, line totals.
  - `census.js`: markers (knob family, a11y, presets, bridge, meters, fonts, chrome), a CSS parser (colours incl. `rgb()`, stacks, sizes, custom properties) and image md5.
  - `tokens.js`: custom-property vocabularies and resolved `var()` font stacks.
  - `clones.js`: 4-line normalized shingles; coverage at ≥2/5/10/20 plugins; subsystem attribution by enclosing function name (JS) or rule selector (CSS) via regex. *Heuristic*: subsystem line counts are estimates.
  - `pairs.js`: lineage containment, excluding shingles present in ≥30 plugins.
  - `fnbodies.js`: per-component body dumps plus feature matrices (`work/ve`, `work/tt`, `work/sp`, `work/kn`).
  - `pdprevent.js`: `mousedown`-dismisser × `preventDefault`ed `pointerdown` detector.
  - `markup.js`: chrome markup variants.
  - `matrix.js`: Appendix A.
- **Gates run read-only:**
  - `node scripts/check-ui-canon.js --repo-root <snap>` (and on the working tree).
  - `node scripts/check-i18n.js --root <snap>` → 11 FAILED, all [12] missing configure copies. On the working tree it passes.
  - `gh run list/view` for UI Static Gates history.
- **JUCE comparison:** `~/JUCE` (8.0.15), `~/JUCE-8.0.14-backup` and `~/JUCE-8.0.9-backup`; the frontend files are identical across all three.
