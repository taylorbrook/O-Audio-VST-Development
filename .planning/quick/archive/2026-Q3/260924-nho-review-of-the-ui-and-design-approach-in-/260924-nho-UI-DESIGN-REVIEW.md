# UI and Design Approach Review — Ouaricon suite (44 WebView plugins)

Reviewed at commit `b7528dc3`, 2026-09-24.
Scope: how plugin UIs are structured, styled, generated, gated, and experienced, across all 44 folders under `plugins/`. Analysis only. No plugin, module, skill, script, or CI file was changed.

> Every number below comes from Appendix A or from a command in Appendix B. The census reads a `git archive b7528dc3` snapshot, not the working tree, so other sessions' uncommitted edits do not affect it.

## Executive summary

The suite *looks* like one product: the naturalist palette core reaches 31–40 of 44 plugins, and i18n is complete. Underneath, it is 44 hand-built UIs.

- **Where drift happens.** Hand-copied code drifts wherever no gate protects it:
  - `setupTooltips` has 18 variants;
  - `initializeTipsToggle` has 22 variants across 23 plugins;
  - `bindKnob` has 13 variants across 15 plugins;
  - the knob comes in 5 visual techniques;
  - there are 7 versions of `preset-manager.js` and 4 tuning-panel forks, none of which match canon.
- **Where it does not.** The one hand-copied block with a canon gate, the i18n runtime, has **1 variant across 44 plugins**. The "no shared module" decision holds up; what is missing is the gate.
- **The generator is stale.** The mockup/finalization pipeline predates every UI requirement added since August: i18n, tooltips, hover-help, `getScaledValue()` readouts, keyboard access. Its template knob is a generic dark SVG that uses the mirrored-range anti-pattern, so each new plugin is born needing the same retrofit.
- **No UI gate runs automatically**: not in CI, not in hooks, not in skills. Proof: `i18n-zh-lint.js` has been **red on `main` for 10 days** (O-Comp `scSource` "来源", Z5), and nothing noticed. Verified again this session: `GATE FAILED — exit 2`.
- **Accessibility is systemic.**
  - The brand template's own text colours (`#8B7355`, sage, moss on paper) fail WCAG AA at its prescribed 9–11 px sizes. On screen, 23–75% of text is below AA in 6 of the 10 sampled plugins.
  - 34/44 plugins have mouse-only knobs.
  - 34/44 declare font sizes below 9 px.
- **The house serif never renders.** No font is bundled, and "Garamond" does not exist on stock macOS. The brand ships as Times New Roman, Georgia, or Iowan Old Style, depending on stack order.
- **Fixed frames.** 40/44 plugins are not resizable, and none has a UI scale. Five frames are 820–980 px tall, which is off-screen on a 13" laptop.

## Ranked recommendations

Ordered by impact ÷ effort. The first three stop *new* drift without touching any working plugin UI. That respects the recorded per-plugin decision (`260826-ieq-CONTEXT.md:51`) and the "don't modify working functionality" rule.

| # | Recommendation | Impact | Effort | Regression risk | Touches |
|---|---|---|---|---|---|
| **R1** | **Run the three static gates automatically.** Add a `ui-static-gates` job to CI (on push to `main`) running `check-i18n.js`, `i18n-fr-lint.js`, `i18n-zh-lint.js` (total < 1 s, no browser). Optionally add the same three to a pre-commit hook scoped to `plugins/*/Source/ui/**`, `plugins/*/Resources/ui/**`. | High: catches today's class of silent red | XS (≈20 lines YAML) | None to plugins. First run will be red until O-Comp Z5 is fixed (fix first, or add a glossary exemption) | `.github/workflows/`, optionally `.githooks/` |
| **R2** | **Teach the generator the current contract.** Update `ui-finalization-agent.md` + `ui-mockup/references/html-generation.md` to emit O-ReverseDelay's patterns: the Family A knob (stem + conic ring), the pointer-capture lifecycle, arrow keys + `role=slider`/`aria-valuetext`, `getScaledValue()` readouts, the canonical i18n block from `i18n-canon.js`, a single `#tips-toggle` hover-help, and `data-tip` bindings. Delete the dark `#333`/`#4a9eff` knob and `formatValue()` min/max mirroring. Add the gate commands to the finalization checklist. | High: every future plugin is born compliant | S–M (docs only) | None to shipped plugins | `.claude/agents/ui-finalization-agent.md`, `.claude/skills/ui-mockup/references/{html-generation,ui-design-rules}.md` |
| **R3** | **Extend canon-as-data to the drifting components.** Following the `i18n-canon.js` pattern, add canon blocks + a check-i18n-style drift assertion for tooltip renderer, hover-help init, and knob binding, as **report-only first** (lists which plugins diverge from canon). Separately, add a content-hash check that the tracked `preset-manager.js`/`tuning-panel.js` copies match `modules/` (or are explicitly marked forks). | High: makes drift visible, turns 13–22 variants into a burn-down list | M | None while report-only. Converging a plugin onto canon *is* a behaviour change, so do it per plugin via `/improve` with a version bump, never as a sweep | `scripts/`, new `scripts/ui-canon.js` |
| **R4** | **Fix the palette at the source, then per plugin.** Add AA-passing text variants to `ouaricon-naturalist-001/aesthetic.md` (`#7A654B` for walnut text on paper, `#715D45` on `#EBD9C7`, `#55703E` for sage text). Keep the existing colours for borders and decoration. Set a 9 px floor (the template's own). Add a contrast assertion to `measure-ui.js` (report-only). Apply the fixes per plugin, worst first: O-Prism (75%), O-TextureForge (69%), O-MicrotonalSampler (44%). | High (legibility is the most visible UX defect) | S template / M per plugin | Visible colour shift. Do it per plugin with before/after screenshots | template, then `plugins/<Name>/…/css` |
| **R5** | **Bundle the serif.** Ship one OFL face (EB Garamond, woff2, ~150–250 KB per weight) as a module asset embedded via direct embed like `tuning-panel.js`, with `@font-face` in a shared snippet. Pin the fallback order (Georgia before Times) so un-migrated plugins at least converge on one face. | Medium–High (brand fidelity, one face everywhere, Windows parity) | S module / S per plugin | Text metrics change, so width pins may shift. Run `check-ui-labels.js` per plugin on migration | `modules/ui/…`, per-plugin CMake + CSS |
| **R6** | **Delete or adopt the dead `modules/ui/*`.** `playable-keyboard` and `instrument-footer-panel` have 0 consumers while 9 plugins hand-roll keyboards. Either delete them (with registry entries) or make `playable-keyboard` the R2 generator default for new instruments. Don't retrofit the 9. | Low–Medium (clarity) | XS | None | `modules/ui/`, `modules/registry.yaml` |
| **R7** | **Keyboard access + resizing, opportunistically.** When a plugin comes up for `/improve` anyway, port the O-ReverseDelay knob interaction (arrows, capture loss, ARIA) and, for frames ≥ 820 px tall, add `setResizable` with a fixed-aspect `ComponentBoundsConstrainer` + CSS `zoom`/transform scale. Track as a checklist in the canon report (R3). | Medium (accessibility, laptop users) | M per plugin | Knob interaction is core UX. Needs hands-on DAW check per plugin | per-plugin, via `/improve` |

**First steps, in order:** fix O-Comp `scSource` zh (Z5) → land R1 → R2 doc update → R3 report-only canon check.

### Not recommended

- **A shared runtime JS/CSS component library retrofitted into all 44 UIs.** This would reverse a recorded decision and touch every shipped UI at once, with high regression risk. The canon + gate approach (R3) buys most of the consistency at a fraction of the risk. *Requires decision* if you want it anyway.
- **Unifying the 5 knob families visually across shipped plugins.** Families B and C are established looks users know. Standardise new plugins (R2) instead.
- **Consolidating the 23 `ui_tip_render_check.js` copies now.** Worth doing eventually (one parametrised `scripts/` gate using `serve-ui.js`), but these are test-only and not user-facing. Rank it after R1–R4.
- **Merging the `Source/ui/public` vs `Resources/ui` layouts.** It is cosmetic, and CMake plus `serve-ui.js` already handle both.

## 1. Architecture of UI code

### 1.1 What a UI is

Each plugin ships one static page that JUCE's `WebBrowserComponent` serves from `juce_add_binary_data`. The page is built from the same parts every time:

- `index.html`. It often holds most of the CSS and JS inline. O-Prism's `index.html` is 225 KB. 18 plugins have no external controller JS at all (only `i18n.js`/`tuning-panel.js` beside the page). O-Bells, O-Wind, O-Bassoon, O-Reed, O-Comp, and O-Chorus are examples: their controller lives entirely in inline `<script>` blocks.
- `css/*.css` and `js/app.js` (or `main.js`, `sampler-app.js`, `i18n_init.js`) where a plugin splits its code out.
- `js/i18n.js`: the translation table plus the canonical `applyI18n`/`initI18n` runtime. It is present in 44/44 plugins, and all 44 carry `en/fr/zh-Hans`.
- `js/juce/index.js` + `check_native_interop.js`: the vendored JUCE frontend library. It exists as 44 copies in 3 distinct versions, and nothing compares them to the pinned JUCE 8.0.15.
- Optional module JS: `preset-manager.js`, `tuning-panel.js`/`.css`, `webview-drop-streaming.js`, `vu-meter.js`, `analog-eq-unit.js`, `compressor-unit.js`.
- Raster art: an aged-paper JPG and one naturalist illustration PNG. There are no font files. **0 bundled fonts suite-wide.**

Across the suite, authored UI code comes to 17,300 markup + 48,345 CSS + 66,649 JS = **132,294 lines**, plus 49,332 lines of i18n tables and 38.0 MB of payload (Appendix A.1, A.5).

### 1.2 Two UI roots, decided by CMake rather than by folder

The planner's grounding said 36 plugins use `Source/ui/public` and 8 use `Resources/ui`. **The shipping split is actually 34 / 10.** O-MicrotonalSampler and O-Orbit carry both folders. In each, `Source/ui/public` only holds a module copy: `modules/webview-drop-streaming.js` and `modules/preset-manager.js` respectively. Their `CMakeLists.txt` embeds `Resources/ui/index.html`, as `plugins/O-MicrotonalSampler/CMakeLists.txt:80-86` and `plugins/O-Orbit/CMakeLists.txt:54-58` show. So the census resolves each root from the `index.html` that `juce_add_binary_data` names, the same rule `scripts/serve-ui.js` already follows ("The UI root is READ, never guessed", its header). A probe that tests folder existence first would have counted the wrong page for 2 plugins.

The 10 `Resources/ui` plugins are O-Bassoon, O-Bells, O-Bowed, O-FreqPulse, O-Lyrica, O-MicrotonalSampler, O-Orbit, O-Reed, O-SpectralShaper, and O-Wind. Most are physical-model or microtonal instruments from the Stage-3 "Pattern A" era (`plugins/O-Bassoon/CMakeLists.txt:87`). The layout difference is historical. The pages are built the same way.

### 1.3 Shared code: three delivery mechanisms, only one of which drifts

| Mechanism | How it works | Consumers | Drift |
|---|---|---|---|
| **Direct embed** | `juce_add_binary_data` lists `${CMAKE_SOURCE_DIR}/modules/.../x.js`, so the build reads the module file itself | 8 plugins (O-Bassoon, O-Bowed, O-Contrabass, O-Marimba, O-MicrotonalSampler, O-Reed, O-ReverseDelay, O-Wind), e.g. tuning-panel.js ×5 | None by construction |
| **Configure-time copy** | `ouaricon_add_module()` runs `configure_file(... COPYONLY)` into `Source/ui/public/modules/` (`modules/cmake/OuariconModules.cmake:104-115`). The folder is gitignored (`.gitignore:26`) | preset-manager for 12 plugins that call `ouaricon_add_module(<T> preset-manager)` | None at build time |
| **Frozen tracked copy** | A copy committed inside the UI root and listed in the plugin's own SOURCES, with no `ouaricon_add_module` call to refresh it | **6 preset-manager forks** (O-Bass, O-Comp, O-DigiDelay, O-FreqPulse, O-Polystutter, O-SpectralShaper) and **4 tuning-panel forks** (O-Bells, O-Formant, O-IntonationPad, O-MicrotonalSampler) | preset-manager: 7 distinct hashes across 12 tracked copies. Only 6 match `modules/persistence/preset-manager/js/preset-manager.js`. The forks are 406 or 377 lines against 447 canonical, a 57–162 line diff. tuning-panel: 0 of 4 match the canonical 1,094-line file (241–534 line diffs) |

Shipped UI runtime code is therefore **already shared** in 8 plugins, through direct embed of `tuning-panel.js`, `preset-manager.js`, `webview-drop-streaming.js`, and the Marimba units. That mechanism has zero drift. Every drifted copy comes from the third mechanism.

The two modules under `modules/ui/` are dead:

- `instrument-footer-panel` and `playable-keyboard` have **0 consumers**. `modules/registry.yaml` lists `used_by: []` for playable-keyboard, and no `CMakeLists.txt` or UI file references either module.
- Meanwhile **9 plugins hand-roll an on-screen keyboard**: O-Bells, O-Lyrica, O-Marimba, and the six O-simple* synths that have one (Additive, FM, Grain, PhysicalModelSynth, Sampler, Subtractive).

### 1.4 Knob families — five techniques, not two

The recorded note (`memory/pattern_no_shared_knob_module_two_families.md`, surveyed at 31 plugins) describes Family A ("seed cross-section") and Family B ("SVG vine arc"). Read at 44 plugins, the markers split further (Appendix A.1, "Knob family"):

| Family | Marker in authored code | Plugins | Notes |
|---|---|---|---|
| **A** | `.knob-stem` child on a conic-gradient ring | 10 | O-ReverseDelay (reference), O-Tapestop, O-MultiBandCompressor, and 7 O-simple* synths. Also the only 10 with arrow-key knob handling |
| **A′** | conic-gradient ring, no `.knob-stem` (usually a `.knob-indicator` child) | 12 | Looks like A but uses a different DOM and JS: O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bitrot, O-Emulator, O-Formant, O-GrainScatter, O-Marimba, O-Orbit, O-SpectralShaper, O-TextureForge, O-Tremolo |
| **B** | `.knob-vine` SVG `stroke-dashoffset` arc | 13 | O-Prism, O-Comp, O-Chorus, O-DigiDelay, O-Freeze, O-SimpleReverb, O-Detune, O-IntonationPad, O-Strata, plus the Effects tab of O-Bells, O-Lyrica, O-MicrotonalSampler, and O-Wind |
| **C** | `.knob-fill` arc + `.knob-indicator` (no conic ring) | 6 | O-Bassoon, O-Bowed, O-Contrabass, O-Reed, O-Wind (main page), and O-Gain (SVG `drawKnob`) |
| **D** | bare `.knob-indicator` rotation | 1 | O-Texture |
| **R** | native `input[type=range]` (only control, or alongside knobs) | 2 only / 8 alongside | Only control: O-FreqPulse, O-Octagon |
| unclassified | bespoke `.knob` rotate | 1 | O-Polystutter |

O-Wind mixes families C and B on one page. The instrument families (Bells, Lyrica, MicrotonalSampler, Wind) render the main page and the Effects tab with different knob techniques.

The interaction code varies even more than the visuals (Appendix A.3). 51 different knob/drag function names exist. `updateKnobVisual` appears in 20 plugins with 15 distinct bodies. `bindKnob` appears in 15 plugins with 13 distinct bodies, median 60 lines. `setupKnob` appears in 7 with 7 distinct bodies, and `createKnobSVG` in 5 with 5. Only 4 plugins handle `lostpointercapture` (O-MultiBandCompressor, O-Octagon, O-ReverseDelay, O-Tapestop). 25 never call `setPointerCapture`.

### 1.5 The recorded decision and what it costs

`260826-ieq-CONTEXT.md:51` records **"Per-plugin JS, no shared module"** for the i18n runtime. It cites "this repo's existing hand-copy convention for UI code (there is no shared knob module either)". Its risk register (`:138-142`) accepts "43 independent copies of the i18n runtime… copies diverge silently" as a deliberate cost.

The same decision already contains the mitigation that works. `scripts/i18n-canon.js` holds the runtime block as **data**, and `scripts/check-i18n.js` assertion [6] compares each plugin's extracted region against it. The i18n-canon header calls this "the only mitigation available under that rule". The measurement shows it is sufficient:

| Hand-copied code | Protected by a canon + drift gate? | Plugins | Distinct variants |
|---|---|---|---|
| `applyI18n`, `initI18n`, `applyLabel`, `setLabel`, `trLabel`, `applyI18nAttributes` | yes (check-i18n [6]) | 44 | **1** |
| `setSettingsPopoverOpen` | partially (inside the canon region) | 25 | 1 |
| `setupTooltips` (tooltip renderer) | no | 29 | **18** |
| `initializeTipsToggle` | no (check-i18n [16] checks the switch exists, not its code) | 23 | **22** |
| `updateKnobVisual` | no | 20 | **15** |
| `bindKnob` | no | 15 | **13** |
| `showPresetDropdown` | no | 9 | **9** |

The decision is not the problem. Hand-copy plus a canon gate produced 1 variant across 44 copies. Hand-copy **without** a gate produced 13–22 variants per component. The hover-help switch still carries two id spellings: `#tips-toggle` in 33 plugins and `#help-toggle` in 11. The gear popover initializer exists under two names, `initializeSettingsPopover` (22 plugins, 6 variants) and `initSettingsPopover` (14 plugins, 6 variants).

## 2. Design system consistency

**Sample (10 plugins).** The deep-dive covers the 8 plugins the plan named — O-ReverseDelay, O-Prism, O-Gain, O-Octagon, O-MicrotonalSampler, O-simpleSubtractive, O-Bells, O-TextureForge — plus 2 added. O-Bassoon was added because Family C and the `Resources/ui` physical-model instruments had no member in the named sample. O-Chorus was added because it is the only strip-format frame (700×125) and a Family B effect without custom properties. None of the named 8 was swapped out.

Sample roots read:

- `plugins/O-ReverseDelay/Source/ui/public/{index.html,css/styles.css,js/app.js}`
- `plugins/O-Prism/Source/ui/public/index.html`
- `plugins/O-Gain/Source/ui/public/index.html`
- `plugins/O-Octagon/Source/ui/public/{index.html,css/styles.css}`
- `plugins/O-MicrotonalSampler/Resources/ui/{index.html,css/sampler-shell.css}`
- `plugins/O-simpleSubtractive/Source/ui/public/{index.html,css/styles.css}`
- `plugins/O-Bells/Resources/ui/index.html`
- `plugins/O-TextureForge/Source/ui/public/css/ouaricon-naturalist.css` + `Source/ui/src/app.js`
- `plugins/O-Bassoon/Resources/ui/index.html`
- `plugins/O-Chorus/Source/ui/public/index.html`

**Visual pass: performed.** A scratch script used `scripts/serve-ui.js`:

- it called `resolvePlaywright()`, `buildRoot(name, {repoRoot: <HEAD snapshot>})`, `readEditorSize()`, and `serve()`;
- it rendered each page headless (Chromium, `deviceScaleFactor: 1`) at its shipping `setSize` frame and viewed the PNGs;
- it measured computed styles in the page and read the resolved platform font through CDP `CSS.getPlatformFontsForNode`.

`buildRoot` copies into `os.tmpdir()`, so nothing was written to the repo.

Caveats:

- Seven pages ran on the generic stub. Control **values** in those screenshots are stub fixtures, so no finding below is based on a displayed value.
- Headless Chromium and WKWebView both resolve fonts through CoreText from the same installed set. The face names should transfer, but they were not re-verified inside a DAW.

Numbers are in Appendix A.4.

### 2.1 Tokens

- **26 of 44 plugins** define CSS custom properties. **18 define none** and style entirely with literals: O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Chorus, O-Comp, O-DigiDelay, O-Freeze, O-Gain, O-GrainScatter, O-IntonationPad, O-Lyrica, O-Marimba, O-MultiBandCompressor, O-Orbit, O-Polystutter, O-Prism, O-Strata, O-Tremolo. The literal-only group includes the two largest UIs, O-Prism (221 hex literals) and O-Strata (208).
- **153 distinct custom-property names** exist suite-wide. The most common family is `--bg-paper`/`--brown-text`/`--brown-border`/`--brown-frame`/`--green-light|mid|dark`, shared by only 17 plugins. It is the O-ReverseDelay / O-simple* vocabulary.
- Other plugins invent their own. O-Octagon has `--ground --panel --ink --ink-dim --rule`. O-MicrotonalSampler has `--bg-cream --text-body --accent-gold --gap-sm|md|lg`. O-TextureForge has `--paper-bg --text-primary --knob-wheat-a`. O-Bells defines only `--tuning-*` (8, for the tuning panel) and hard-codes its main page. So tokens exist, but **no token file is shared**, and the same colour lives under 3–4 names.
- The brand template asks for "Set up color palette using CSS variables" in its checklist (`.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`, Implementation Checklist) but names no variables. Every plugin therefore names its own.

### 2.2 Colour

- **A real de-facto palette exists.** The same hex values recur across plugins even without shared tokens: `#3C2F2F` ink in 40/44 plugins, `#8B7355` walnut in 39, `#5C4033` dark brown in 34, `#F5E6D3` paper in 32, `#6B8E4E` sage in 31, `#8BA870` moss in 29. These are exactly the values in the brand template's Color System (`aesthetic.md:36-63`).
- The outliers are O-Octagon (dark `#1A1613` ground with gold accent, a different theme), O-Contrabass, O-Freeze, and O-Texture, which use none of the ink/walnut/paper core. Accents also diverge inside the brand: sage green (O-ReverseDelay, O-simpleSubtractive, O-Bassoon), slate-teal arcs (O-Prism, O-Chorus), and mustard gold (O-MicrotonalSampler).
- Long tail: 248 distinct hex colours suite-wide, **173 used by exactly one plugin**. Per-plugin distinct hex values run from 8 to 38 (median 17).
- **Contrast is set by the palette, not the plugins.** WCAG ratios of the template's own text pairs:
  - on paper `#F5E6D3`: `#3C2F2F` 10.45:1, `#5C4033` 7.67:1, `#8B7355` **3.66:1**, `#6B8E4E` **3.06:1**, `#8BA870` **2.16:1**;
  - on `#EBD9C7`: `#8B7355` 3.26:1.

  The template assigns `#8B7355` to "borders, text", sage/moss to active states, and "lighter brown with reduced opacity" to subtle text (`aesthetic.md:50-58`). All of these fail AA for body text at the 9–11 px label sizes the template prescribes (`aesthetic.md:105-110`).
- **Measured on screen (A.4).** Share of visible text below AA:
  - O-Prism: **75%** (109 of 146 elements; knob values in `#A08870`-ish on cream, median ratio 3.44);
  - O-TextureForge: **69%**;
  - O-MicrotonalSampler: **44%** (the active tab and technique buttons are the worst, 2.16–2.65);
  - O-Bassoon 25%, O-Octagon 23%, O-Bells 23%;
  - O-simpleSubtractive 15%, O-Gain 12%;
  - O-ReverseDelay 5% (only its decorative fleurons);
  - O-Chorus 0%.

  AA-passing replacements in the same hue, computed:
  - `#7A654B` for `#8B7355` on `#F5E6D3` (4.51:1);
  - `#715D45` on `#EBD9C7` (4.56:1);
  - `#55703E` for `#6B8E4E` on `#F5E6D3` (4.54:1).
- Busy illustration backgrounds are a second contrast risk that the numeric check cannot see. It only knows the nearest opaque ancestor colour, and A.4 counts how many measured elements sit over a background image. For example, O-Chorus (700×125) places its RATE / DEPTH / SPREAD / WIDTH / MIX knobs directly over the insect plate, so the dragonfly's wings run through the DEPTH, SPREAD, and MIX knob faces (screenshot `O-Chorus.png`).

### 2.3 Typography

- **The house serif is never rendered.** 0 plugins bundle a font. The dominant stack is `'Garamond', 'Times New Roman', …` (106 declarations) or `'Garamond', 'Georgia', …` (58), and stock macOS has no face called Garamond (`/System/Library/Fonts*`, `/Library/Fonts`, `~/Library/Fonts` contain none). CDP confirms the substitution on the sample, and which face you get depends on stack order:
  - Times New Roman: O-ReverseDelay, O-MicrotonalSampler, O-simpleSubtractive, O-Bells, O-Chorus;
  - Georgia: O-Prism, O-Bassoon (whose stack leads with `'EB Garamond'`, also absent);
  - Iowan Old Style: O-Octagon, which is the only sampled plugin that names a face that exists.

  So the "one serif brand" ships as **three faces**, chosen by whether the second family in each stack is Times New Roman or Georgia. On Windows, WebView2 resolves `Garamond` only if Microsoft Office installed it.
- **Font stacks:** 1–7 distinct per plugin (A.1), 43 distinct stack strings suite-wide.
- **Size scale:** 27 distinct px font sizes suite-wide, 4–14 per plugin (median 8). **34 of 44 plugins declare a font size below 9 px.** The floor is 6 px in O-Bells, O-IntonationPad, O-Lyrica, and O-Marimba. On screen, O-Bells renders its version label at 7 px with contrast 2.03 and its keyboard note names at 6 px. The brand template's own floor is 9 px (`aesthetic.md:108`).

### 2.4 Spacing and layout

- **Units:** 12,191 px lengths against 62 em/rem lengths in shipped CSS.
- **Spacing:** margin/padding/gap use **56 distinct px values**. The most frequent are 8 (448), 4 (429), 6 (338), 10 (310), 2 (253), 3 (182), 12 (158), 5 (112), and 14 (112). That mixes a 4/8 rhythm with 3/5/6/7/10/14, so no spacing scale is in force. O-MicrotonalSampler is the only sampled plugin with spacing tokens (`--gap-sm|md|lg`).
- **Layout:** all 44 plugins use flex, 23 also use grid, and all 44 use `position: absolute` somewhere (overlays, decorative plates, popovers). The layouts are fixed-frame compositions, not fluid ones (see §5.5).

### 2.5 Component inventory (implementations counted from A.3, not by name alone)

| Component | Implementations | Evidence |
|---|---|---|
| Rotary knob, visual | 5 techniques (A, A′, B, C, D) + native range + 1 bespoke | §1.4 |
| Rotary knob, interaction | at least 13 distinct `bindKnob` + 15 distinct `updateKnobVisual` + 7 `setupKnob` + 5 `createKnobSVG` bodies (51 knob/drag function names) | A.3 |
| Tooltip renderer | 18 distinct `setupTooltips` + 4 distinct `showTooltip` bodies; 17 tip function names | A.3; the 6 renderer locations outside `index.html` are in `memory/pattern_no_shared_knob_module_two_families.md` |
| Hover-help switch | 2 ids (`#tips-toggle` ×33, `#help-toggle` ×11); 22 distinct `initializeTipsToggle` bodies in 23 plugins | A.1, A.3 |
| Gear/settings popover | 2 function names, 6 + 6 variants; `setSettingsPopoverOpen` 1 variant ×25 | A.3 |
| Preset bar | `preset-manager.js` 7 versions in the tree; `showPresetDropdown` 9/9 distinct | A.2, A.3 |
| On-screen keyboard | 9 hand-rolled; the `modules/ui/playable-keyboard` module has 0 consumers | §1.3 |
| Tuning panel | 5 direct-embed (canonical) + 4 forks | §1.3 |
| Meters | `vu-meter.js` direct-embed in 4 (O-AnalogEQ, O-AnalogSaturation, O-Marimba, O-SimpleReverb); others bespoke | Appendix B step 5 |

### 2.6 Traceability to `.claude/aesthetics/`

`.claude/aesthetics/manifest.json` holds 5 templates. Only `ouaricon-naturalist-001` is `status: official-brand`, and its `aesthetic.md:529` says "ALL plugins should use this system". The suite broadly follows its palette: 40/44 plugins carry its ink colour. It also follows the paper-and-illustration idea, with 19 paper JPGs (3 distinct) and one naturalist plate per plugin.

The template does not fix the knob. It specifies the "Botanical Seed Cross-Section" (conic-gradient 10-segment ring + stem, `aesthetic.md:205-262`), which only Families A and A′ implement (22 plugins). The 13 Family B and 6 Family C plugins trace to no template.

O-Octagon's dark theme traces to no template. None of the four non-brand templates has a shipped consumer the census can identify. `vintage-hardware-001` and `vintage-bakelite-001` name `sourcePlugin`s (DriveVerb, AutoClip) that are not in `plugins/`, and `studio-hardware-001` and `swiss-minimal-001` have `sourcePlugin: null`.

### 2.7 Visual observations (screenshots in the scratchpad; element + value)

- **O-ReverseDelay (940×768):** the most coherent page. It uses a Family A knob grid in bordered sections, sage active buttons, and one type face (resolved to Times New Roman). Section titles, labels, and values keep a consistent 13/10/9 px hierarchy. This is the page to copy.
- **O-Prism (1200×800):** Family B arcs. Knob values are 10 px at a 2.57:1 contrast ratio and section titles 9–10 px. The whole instrument reads low-contrast, and 75% of text fails AA. The dark footer strip (MASTER / OSC MIX) breaks the paper theme.
- **O-Gain (380×500):** the INPUT and OUTPUT meter headers (9 px, `#8B7355`) almost disappear into the stained paper texture. Nothing overlaps them: `elementFromPoint` returns the label itself, and a 3× crop confirms it. The problem is contrast against the image. The numeric check scores 3.66:1 against the fallback colour, and the real value over the dark stain is lower. This is the kind of defect a colour-only contrast gate would miss.
- **O-MicrotonalSampler (900×640):** mixes three families on one page: `-apple-system` sans (21 elements, including all 18 `.ouaricon-knob-label`/`.ouaricon-knob-value` footer captions), Garamond → Times New Roman (54), and `ui-monospace` (34). The accent is gold rather than the brand sage, and the active tab ("Sample Map") is 2.37:1.
- **O-Octagon (1100×720):** a deliberate dark "venue" theme using `Iowan Old Style` + `ui-monospace`. It is internally consistent and outside the brand palette.
- **O-Bassoon (900×600) / Family C:** thin arcs with a separate indicator. It shares the paper and the green with Family A but reads as a different product line. Captions run 8.5–11 px at 3.32:1.
- **O-Bells (800×600):** uses horizontal range sliders rather than knobs on the main page. The footer version label is 7 px at 2.03:1 and the keyboard note names are 6 px.
- **O-TextureForge (900×600):** a large empty-state left panel with a single italic prompt. Only 2 text colours are in use, and the tagline and fleurons fall below AA.
- **O-simpleSubtractive (1180×820):** the tallest sampled frame. Its VOICE / OUTPUT row is cut off at the bottom of the shipping viewport. The page's `.frame` scroll container is 814 px tall around 1,118 px of content, so 27% of the UI is reachable only by scrolling inside the plugin window, even at the frame it chose.

## 3. Pipeline and process

### 3.1 How a UI is born

The chain has five steps:

1. `.claude/skills/aesthetic-dreaming/SKILL.md` (352 lines) captures a visual concept as prose.
2. `.claude/skills/ui-template-library/SKILL.md` (267) saves or applies templates under `.claude/aesthetics/`.
3. `.claude/skills/ui-mockup/SKILL.md` (360) orchestrates the mockup.
4. `.claude/agents/ui-design-agent.md` (1,291) emits `vN-ui.yaml` + `vN-ui-test.html` and validates layout arithmetic (bounds, overlap, minimum sizes; its sections 4.5–5).
5. `.claude/agents/ui-finalization-agent.md` (1,244) emits production `vN-ui.html`, the `PluginEditor` templates, a CMake snippet, and an integration checklist (its Phases 6–9).

Supporting references in `.claude/skills/ui-mockup/references/` (16 files) include `ui-design-rules.md` (854), `html-generation.md` (274), `layout-validation.md` (1,244), and `aesthetic-integration.md` (309). In total the UI pipeline docs come to **8,066 lines**.

Mockups persist in only **10 plugins**, holding 34 versions: O-Comp v1–v8, O-DigiDelay v1–v7, O-Polystutter v1–v5, O-AnalogSaturation v1–v4, O-AnalogEQ v1–v3, O-Tremolo v1–v3, O-Bitrot v1, O-Contrabass v1, O-Marimba v1, and O-Strata v2. The other 34 plugins have no mockup on disk. Their UIs were written or rewritten directly in the plugin tree, so for most of the suite the pipeline is not the path that UIs actually take.

### 3.2 The pipeline generates fresh component code every time — the root cause of §1's drift

- `references/html-generation.md:166-203` gives the knob as a generic dark SVG. It uses `fill="#333"` and an `#4a9eff` pointer line, which is neither brand family.
- `references/html-generation.md:132-163` formats values with `formatValue()`, which linearly maps a hand-mirrored `data-min`/`data-max`.
- `references/ui-design-rules.md:414-466` (Rule 8) computes knob rotation as `(value - min) / (max - min)` from mirrored ranges.

The memory notes this suite converged on reject that pattern. Readouts must use `getScaledValue()` (`memory/pattern_webview_knob_readout_scaled_value.md`), and O-Prism's numeric entry is the reference because it "reads range/skew from live `state.properties` instead of a hand-mirrored JS min/max map" (`memory/pattern_no_shared_knob_module_two_families.md`). **The generator's own template therefore produces the anti-pattern.** Each new plugin gets a new knob, which a later wave then corrects by hand. The `formatValue` result in A.3 shows it: 8 plugins, 7 distinct bodies.

### 3.3 The pipeline does not know about the requirements that became mandatory

I grepped all 8,066 lines of the pipeline docs, meaning the ui-mockup skill + references + BOUNDARIES, both agents, ui-template-library, and aesthetic-dreaming. Each term below has **0 hits**:

`i18n`, `data-i18n`, `tooltip`, `hover-help`/`tips-toggle`, `getScaledValue`, `aria-`, `tabindex`, `focus-visible`, `prefers-reduced-motion`, `O-ReverseDelay`, `O-Prism`, `knob-stem`, `knob-vine`, `conic-gradient`, `preset-manager`, `playable-keyboard`/`instrument-footer`, `check-i18n`/`check-ui-labels`/`boot-all-uis`/`ui_tip_render`/`serve-ui`, `Garamond`.

The only hit among the checked terms is `setResizable` (2, in `ui-design-rules.md`). Last-modified dates show the gap:

| Doc | Last modified |
|---|---|
| `ui-design-agent.md`, `ui-finalization-agent.md` | 2026-02-08 |
| `ui-mockup/SKILL.md` | 2026-01-30 |
| `html-generation.md` | 2026-01-03 |
| naturalist `aesthetic.md` | 2026-01-08 |
| `ui-design-rules.md` | 2026-07-02 |

The i18n canon and its gate landed 2026-08-26 (`scripts/i18n-canon.js`, `scripts/check-i18n.js` Stage A), and zh-lint became a gate 2026-09-05. **Every mandatory UI requirement since August has been retrofitted plugin by plugin.** None of it was taught to the generator, so the next new plugin will be born without i18n, tooltips, hover-help, keyboard access, or gates, and will need the same retrofit.

Stale or contradicting guidance, cited so it can be fixed:

- `ui-design-rules.md:161-178` (Rule 4) recommends **resizable** for 13+ parameters and for multi-page or visualizer UIs, but only 4/44 plugins are resizable (§5.5). O-Prism (1200×800, 5 tabs) and O-Octagon (a visualizer) are fixed.
- `html-generation.md:166-178`'s dark knob contradicts `aesthetic.md:529`, "ALL plugins should use this system".
- The brand template's Typography section (`aesthetic.md:97-103`) says "Fallback chain ensures graceful degradation on all systems", but the fallback **is** the system (§2.3).
- The brand template's text colours fail AA at its own label sizes (§2.2).

## 4. Quality gates and tooling

### 4.1 Inventory

**Repo-level tools in `scripts/`.** Line counts are at HEAD. Runtimes were measured this session against the working tree, all read-only.

| Tool | Lines | Needs a browser | What it asserts | Current state |
|---|---|---|---|---|
| `check-i18n.js` | 1,455 | no (static) | Table completeness per language [1]; tip bindings resolve [2]; no literal tooltip attrs [3]; fr ≠ en without `sameAsEn` [4]; reviewed flags [5]; **canon match of the i18n runtime [6]**; embed/serve wiring [8]; inertness [7,9]; module files exist [12]; exactly one hover-help switch [16] | **PASS, 44 plugins, 0.8 s** |
| `i18n-fr-lint.js` | 274 | no | French typography/terminology | **CLEAN, 0.10 s** |
| `i18n-zh-lint.js` | 732 | no | Simplified Chinese typography/terminology (Z1–Z8, F1, R1) | **RED, exit 2, 0.07 s**: O-Comp `label.scSource` / `tip.scSource` "来源" breaks Z5 (glossary root 源) |
| `check-ui-labels.js` | 1,147 | Playwright | No label clips, ellipsizes, or displaces a neighbour, in both languages, at the shipping frame | by hand |
| `boot-all-uis.js` | 377 | Playwright | Every page boots, plus page errors, 404s, unknown natives, and DEAD/LATE tip bindings. **Report-only** by design (exit 0), with `--strict-tips` → exit 2 | by hand |
| `measure-ui.js` | 639 | Playwright | Per-node computed-style census: fonts, boxes, wrap counts | by hand (diagnostic) |
| `serve-ui.js` + `ui-stub/` | 590 + 3 files | Playwright (consumer) | Harness: CMake-derived root, embed placement, generic stub, port 0 | library |
| `i18n-extract.js`, `i18n-canon.js`, glossaries, `i18n-zh-backtranslate.js`, `gen-zh-trad-only.js` | 3,649 combined | no | Authoring aids and the canon | — |

**Per-plugin gates** (A.1): 34 files, 28,159 lines, plus 11 `tests/ui-stub/` directories (15 files, 3,184 lines).

| Gate | Plugins |
|---|---|
| `ui_tip_render_check.js` | 23 |
| `ui_tooltip_clamp_check.js` | 3 (O-Bitrot, O-ReverseDelay, O-Tapestop) |
| `ui_frontend_check.js` | 3 (O-Contrabass, O-Octagon, O-ReverseDelay) |
| `ui_preset_menu_check.js` | 2 (O-Bitrot, O-SpectralShaper) |
| `ui_layout_check.js` | 2 (O-Octagon, O-Strata) |
| `ui_shell_diff_check.js` | 1 (O-Strata) |

**13 plugins have no per-plugin UI gate**: O-FreqPulse, O-IntonationPad, O-Lyrica, O-Marimba, O-MultiBandCompressor, O-Polystutter, and the 7 O-simple* synths. The O-simple* synths and O-MultiBandCompressor are exactly the Family A plugins with the best knob code (§1.4).

### 4.2 Coverage matrix

| Defect class | Caught by | Scope | Runs in CI |
|---|---|---|---|
| Label clip / overflow / displacement (fr, zh) | `check-ui-labels.js` | any plugin, per run | no |
| i18n table completeness, reviewed flags, hover-help switch, runtime drift | `check-i18n.js` [1–16] | all 44 | no |
| French typography | `i18n-fr-lint.js` | all 44 | no |
| Chinese typography / terminology | `i18n-zh-lint.js` | all 44 | no |
| Dead / late tooltip binding | `boot-all-uis.js --strict-tips` | all 44 | no |
| Tooltip actually renders (help layer on, data-tip → visible box) | `ui_tip_render_check.js` | 23 plugins | no |
| Tooltip clamp at frame edges | `ui_tooltip_clamp_check.js` | 3 plugins | no |
| Dead selector / native-function binding | `ui_frontend_check.js`; `boot-all-uis` (unknown natives) | 3 plugins; all (report) | no |
| Preset menu behaviour | `ui_preset_menu_check.js` | 2 plugins | no |
| Layout geometry | `ui_layout_check.js` | 2 plugins | no |
| Visual regression (pixels) | `ui_shell_diff_check.js` (shell merge only) | 1 plugin | no |
| Page boots at all / blank WebView | `boot-all-uis.js`; **pluginval --strictness-level 10 opens the editor** | all (report); the tagged plugin | **yes, Windows only, at release** (`build-and-release.yml:603-620`) |
| Knob interaction (drag, capture loss, wheel, reset) | — | **nothing** | — |
| Keyboard operability / focus order | — (22 per-plugin copies mention `focus-visible`, for tooltip focus only) | **nothing** | — |
| Colour contrast | — (0 hits for `contrast` in `scripts/*.js` or `plugins/*/tests/ui_*.js`) | **nothing** | — |
| Font actually resolved | `measure-ui.js` can report it; nothing asserts it | **nothing** | — |
| Reduced motion | — | **nothing** | — |
| Vendored module drift (preset-manager, tuning-panel forks) | — (check-i18n [12] checks existence, not content) | **nothing** | — |
| Vendored JUCE frontend lib version | — | **nothing** | — |

### 4.3 What runs automatically: nothing UI-level except pluginval

Three places could trigger a UI gate automatically, and none does:

- **CI.** `grep -nE "check-i18n|check-ui-labels|boot-all-uis|i18n-fr-lint|i18n-zh-lint|ui_" .github/workflows/*.yml` returns **0 hits** in both workflows. `ci-tests.yml` is `workflow_dispatch`-only (`:71-81`, choice of O-Octagon or O-Strata). `build-and-release.yml` fires on `*-v*` tags and runs pluginval 10 on Windows. Node is available in CI; `ci-tests.yml:233` already runs `node plugins/O-Strata/tests/orbit-golden.mjs`.
- **Git hooks.** None exist. `core.hooksPath` is unset, and `.git/hooks` has only samples.
- **Workflow tooling.** None of the gate names appears anywhere under `.claude/` (skills, agents, hooks) or in `scripts/*.sh` (including `build-and-install.sh` and `verify-suite-battery.sh`).

**Every UI gate runs only when a session remembers to run it.** The memory notes are what carry that knowledge (`memory/index_ui_gates_and_probes.md`).

The cost is visible right now:

- `i18n-zh-lint.js` was promoted from report to gate in `fed08208` (2026-09-05, "exit 2 on any finding").
- `9c11abc5` (2026-09-14, O-Comp v1.10.0 detector/sidechain UI strip) then added `label.scSource`/`tip.scSource` = "来源", which the gate flags under Z5.
- **`main` has carried a red gate for 10 days, and nothing noticed.** O-Comp has no uncommitted changes, so this is the state at HEAD.

### 4.4 Hand-copied gate drift

| Gate copy set | Copies | Distinct after renaming the plugin to a placeholder | Size |
|---|---|---|---|
| `ui_tip_render_check.js` | 23 | **23** | 17,564 lines, 501 (O-Emulator) to 1,047 (O-Strata) lines each |
| `ui_tooltip_clamp_check.js` | 3 | 3 | 2,462 lines |
| `ui_frontend_check.js` | 3 | 3 | 3,964 lines |
| `ui-stub/juce-stub.js` | 5 | 5 | — |
| `ui-stub/generic-overrides.json` | 6 | 6 | — |

The per-plugin gates repeat the problem they guard against: a fix to the tip-render method must be applied 23 times. `scripts/serve-ui.js` records the same history in its header: the 3 clamp gates "each carry their own hand-written buildRoot()/serve() pair. Those three keep theirs".

### 4.5 Cost of the gate estate

- Gate code totals **40,742 lines**: 28,159 per-plugin + 3,184 per-plugin stubs + 9,399 repo-level UI and i18n scripts. That is **31% of the 132,294 lines of authored UI** it protects.
- 69% of the gate code is per-plugin copies, and that is the part that drifts.
- The three static gates are the cheapest and broadest (0.07–0.8 s, all 44 plugins, no browser), yet nothing runs them automatically.

## 5. UX, accessibility, i18n, resizing

### 5.1 Keyboard and ARIA

- **Keyboard-operable knobs exist in only 10 of 44 plugins**, meaning code that handles `Arrow*` keys: O-MultiBandCompressor, O-ReverseDelay, O-Tapestop, and 7 O-simple* synths.
- `role="slider"` appears in 11 plugins (those 10 plus O-Octagon). `aria-valuenow`/`aria-valuetext` appears in 5 (O-MultiBandCompressor, O-ReverseDelay, O-simpleAdditive, O-simpleBeatmaker, O-Tapestop).
- The remaining **34 plugins have mouse-only knobs**. Measured on screen (A.4):

  | Plugin | Knob-like controls | Reachable by Tab |
  |---|---|---|
  | O-Prism | 94 | 0 |
  | O-Bassoon | 20 | 0 |
  | O-Chorus | 16 | 0 |
  | O-TextureForge | 11 | 0 |
  | O-Gain | 3 | 0 |
  | O-ReverseDelay | 21 | 21 |
  | O-simpleSubtractive | 16 | 16 |

  Native `input[type=range]` controls (O-Octagon, O-FreqPulse, parts of O-MicrotonalSampler) are keyboard-operable by default.
- The ~18–25 `aria-*` sites that almost every plugin has (A.1) are the canonical gear popover and hover-help chrome (`aria-expanded`, `aria-describedby`), not the controls.
- Pointer robustness follows the same split. 25 plugins never call `setPointerCapture`, and only 4 handle `lostpointercapture`. A drag that leaves the WebView, or is interrupted by the host, can leave those knobs stuck (see `memory/pattern_focus_latch_is_three_defects.md`).

### 5.2 Focus styling and motion

- 42/44 plugins have at least one `:focus-visible` rule. Most have exactly 3, the popover chrome. O-Contrabass and O-Orbit have none.
- Only the 10 keyboard-knob plugins style focus on controls (7–10 rules each).
- `prefers-reduced-motion` appears in **1/44** plugins (O-ReverseDelay).

### 5.3 Contrast

See §2.2. It is systemic because the brand template's text colours fail AA at its prescribed sizes. On the sample, the share of text below AA ranges from 0% (O-Chorus) to 75% (O-Prism). No gate measures contrast.

### 5.4 Hover-help and tooltips

- **The contract is consistent:**
  - canon `applyI18n` writes `data-tip`/`data-tip-title`;
  - check-i18n [16] enforces exactly one hover-help switch per plugin, bound and keyed, and passes on all 44;
  - tooltip copy is `textContent` only (check-i18n [9]).
- **The implementation is not consistent:**
  - 18 distinct `setupTooltips` renderers;
  - 2 switch ids;
  - 22 distinct switch initializers;
  - rendered-tooltip verification in 23 plugins only;
  - clamp verification in 3.

  So 21 plugins have authored and bound tooltips that no gate has seen rendered.

### 5.5 i18n coverage and text fit

- **44/44 plugins ship en/fr/zh-Hans**, and check-i18n passes on all of them. Tables live per plugin in `js/i18n.js` (49,332 lines total), with reviewed flags and fr/zh glossaries. This part of the design is complete.
- **Text fit** relies on per-language width pins. All 44 plugins' CSS carries comments pinning boxes to the widest language, while only 9 use language-scoped selectors (`:lang()`/`[lang=]`) and 2 use `@media`. Pinning works, but its cost grows with each language added.
- **A 4th language would touch every plugin in three ways:**
  - check-i18n hard-codes the allowed language sets (`LANG_SHAPES = ['en,fr', 'en,fr,zh-Hans']`, `scripts/check-i18n.js:586`);
  - every width pinned "at the widest language" must be re-measured in 44 UIs;
  - 18 plugins keep their controller inline, so each edit is a rebuild (`juce_add_binary_data`).

### 5.6 Resizing and scale

- `setResizable(true, …)` appears in **4/44** plugins:
  - O-MicrotonalSampler: limits 720×480–1600×1080, the only one with `@media` breakpoints;
  - O-Orbit;
  - O-simpleBeatmaker;
  - O-simpleSubtractive.
- `setResizable(false, false)` appears in 2 (O-Contrabass, O-DigiDelay). The other 38 never call it, so they get JUCE's non-resizable default. The planner's "6 of 44 resizable" counted callers, not resizable editors.
- **No plugin offers a UI scale or zoom.** `grep setScaleFactor|getDesktopScaleFactor|zoomFactor` over `plugins/*/Source/*.cpp` returns 0, and no CSS `zoom` is applied to a root.
- **HiDPI:**
  - CSS and SVG knobs and system-font text render crisply at the device scale factor.
  - The raster layers do not. Every paper texture is an 800×600 JPG (19 copies, 3 distinct) stretched over frames up to 1200×800. That is 1.5× at 1× density and up to 3× on Retina, so the paper grain softens exactly where the brand leans on it.
  - Illustration plates are 300–1,082 px wide.
- **Small screens:**
  - 5 frames are 820 px tall or more: O-simpleFM 980, O-simpleAdditive 930, O-simpleBeatmaker 900, O-simplePhysicalModelSynth 860, O-simpleSubtractive 820.
  - A 13" laptop at its default scaled resolution leaves roughly 800 px or less below the menu bar and a DAW's plugin-window title bar, so those windows fall off-screen, and 4 of the 5 are not resizable.
  - Their in-page `.frame` scroll container only helps once the whole window fits on screen. O-simpleSubtractive scrolls 1,118 px of content inside its own 814 px frame even at full size (§2.7).
- There is no uniform scale strategy. Resizable plugins reflow with flex; fixed plugins cannot shrink.

## What works — keep

These practices are paying off. None of the recommendations should regress them.

1. **Canon-as-data + drift gate** (`scripts/i18n-canon.js` + `check-i18n.js` [6]). It turns 44 hand-copies into 1 variant without adding a shared runtime module, and it is the template for R3.
2. **Direct embed of module JS from `${CMAKE_SOURCE_DIR}/modules/`** (8 plugins, zero drift). Shipped code is shared without a vendored copy.
3. **`scripts/serve-ui.js`.** It reads the root from CMake (which caught the 34/10 root split), places embedded modules, builds in `os.tmpdir()`, and binds port 0. It made this review's visual pass safe and reproducible.
4. **Gate design that refuses vacuous greens.** Exit 77 when Playwright is missing, non-zero when nothing was measured, `--strict-tips`, the vacuity check in [16]. That discipline should carry over to any new gate.
5. **O-ReverseDelay as the reference UI.** It has the Family A visual, pointer capture with every termination path, arrow keys, `role=slider` + `aria-valuetext`, `getScaledValue()` readouts, the only `prefers-reduced-motion`, and 5% of text below AA (only decorative fleurons). It is the best-built page in the suite.
6. **The shared brand palette core.** `#3C2F2F`/`#8B7355`/`#F5E6D3`/`#6B8E4E` reach 31–40 plugins, one illustration per plugin, on a paper ground. The suite reads as one product family even without shared tokens.
7. **Complete trilingual coverage with review provenance**: 44/44 en/fr/zh-Hans, reviewed flags, glossaries, fr/zh lint.
8. **Static gates that are fast and dependency-free** (0.07–0.8 s), which makes R1 nearly free.

## Appendix A — Suite census

All counts are read from the HEAD snapshot (`git archive b7528dc3`). Method is in Appendix B.

### A.1 Per-plugin census (44 rows)

Column notes:

- **Lines** are authored code only. They exclude `js/juce/`, any `modules/` copy, vendored tuning-panel files, images, and bundled files. Inline `<style>`/`<script>` count as CSS/JS, not markup.
- † in the JS column: includes `Source/ui/src` (webpack source). ‡: a bundled file was excluded (`js/app.bundle.js`).
- **Knob family** is defined in §1.4.
- **CSS var defs** counts `--name:` declarations. **Hex literals** counts `#rgb`/`#rrggbb` occurrences in CSS.
- **aria / role / tabindex / arrow-key sites** count code sites, not elements.
- **Font stacks** counts distinct `font-family` values in the plugin's CSS.
- **Bundled fonts** are 0 for every plugin, so the column is omitted.

| Plugin | UI root | Markup / CSS / JS / i18n lines | Knob family | CSS var defs | Hex literals | Font stacks | Vendored copies | Langs | aria / role / tabindex / arrow-key sites | :focus-visible / reduced-motion | setResizable | setSize | Help-toggle id | Per-plugin UI gates | Payload KB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| O-AnalogEQ | Source/ui/public | 298 / 840 / 839 / 906 | A' | 0 | 106 | 2 | preset-manager.js | en/fr/zh-Hans | 23 / 2 / 0 / 0 | 3 / 0 | — | 920x220 | tips-toggle | tip_render | 3448 |
| O-AnalogSaturation | Source/ui/public | 188 / 672 / 544 / 619 | A' | 0 | 67 | 2 | — | en/fr/zh-Hans | 20 / 2 / 0 / 0 | 3 / 0 | — | 600x450 | tips-toggle | tip_render | 1558 |
| O-Bass | Source/ui/public | 170 / 805 / 650 / 779 | A' | 0 | 133 | 2 | preset-manager.js | en/fr/zh-Hans | 25 / 2 / 0 / 0 | 3 / 0 | — | 420x320 | tips-toggle | tip_render | 2708 |
| O-Bassoon | Resources/ui | 294 / 909 / 752 / 842 | C | 27 | 18 | 5 | — | en/fr/zh-Hans | 23 / 10 / 0 / 0 | 3 / 0 | — | 900x600 | tips-toggle | tip_render | 743 |
| O-Bells | Resources/ui | 606 / 1474 / 1550 / 1657 | B+R | 8 | 121 | 2 | tuning-panel.css, tuning-panel.js | en/fr/zh-Hans | 21 / 2 / 0 / 0 | 3 / 0 | — | 800x600 | tips-toggle | ui-stub, tip_render | 710 |
| O-Bitrot | Source/ui/public | 519 / 1063 / 822 / 1311 | A' | 15 | 16 | 1 | — | en/fr/zh-Hans | 36 / 5 / 0 / 0 | 3 / 0 | — | 900x740 | help-toggle | ui-stub, preset_menu, tooltip_clamp | 1787 |
| O-Bowed | Resources/ui | 405 / 1033 / 1418 / 1218 | C | 19 | 15 | 2 | — | en/fr/zh-Hans | 23 / 11 / 0 / 0 | 3 / 0 | — | 900x600 | tips-toggle | tip_render | 811 |
| O-Chorus | Source/ui/public | 254 / 798 / 640 / 1042 | B | 0 | 35 | 2 | preset-manager.js | en/fr/zh-Hans | 24 / 2 / 0 / 0 | 3 / 0 | — | 700x125 | tips-toggle | tip_render | 844 |
| O-Comp | Source/ui/public | 348 / 738 / 1092 / 880 | B | 0 | 62 | 4 | preset-manager.js | en/fr/zh-Hans | 25 / 2 / 0 / 0 | 3 / 0 | — | 620x420 | tips-toggle | tip_render | 2752 |
| O-Contrabass | Source/ui/public | 442 / 1085 / 1235 / 1046 | C | 28 | 33 | 2 | — | en/fr/zh-Hans | 20 / 10 / 0 / 0 | 0 / 0 | false | 1000x650 | help-toggle | frontend | 209 |
| O-Detune | Source/ui/public | 341 / 980 / 874 / 891 | B+R | 13 | 11 | 3 | preset-manager.js | en/fr/zh-Hans | 25 / 2 / 0 / 0 | 3 / 0 | — | 600x480 | tips-toggle | tip_render | 565 |
| O-DigiDelay | Source/ui/public | 251 / 798 / 902 / 779 | B | 0 | 47 | 1 | preset-manager.js | en/fr/zh-Hans | 25 / 2 / 0 / 0 | 3 / 0 | false | 700x196 | tips-toggle | tip_render | 989 |
| O-Emulator | Source/ui/public | 176 / 655 / 695 / 636 | A' | 30 | 24 | 2 | — | en/fr/zh-Hans | 24 / 3 / 0 / 0 | 3 / 0 | — | 620x430 | tips-toggle | tip_render | 1383 |
| O-Formant | Source/ui/public | 474 / 1162 / 1887 / 1643 | A' | 8 | 133 | 3 | tuning-panel.js | en/fr/zh-Hans | 8 / 1 / 0 / 0 | 1 / 0 | — | 800x600 | tips-toggle | tip_render | 972 |
| O-Freeze | Source/ui/public | 281 / 737 / 776 / 787 | B | 0 | 38 | 2 | — | en/fr/zh-Hans | 20 / 2 / 0 / 0 | 3 / 0 | — | 550x530 | tips-toggle | tip_render | 772 |
| O-FreqPulse | Resources/ui | 202 / 1138 / 1743 / 816 | R | 11 | 34 | 5 | preset-manager.js | en/fr/zh-Hans | 20 / 2 / 0 / 0 | 3 / 0 | — | 850x550 | tips-toggle | — | 195 |
| O-Gain | Source/ui/public | 316 / 1141 / 1203 / 915 | C | 0 | 92 | 2 | — | en/fr/zh-Hans | 18 / 2 / 0 / 0 | 3 / 0 | — | 380x500 | tips-toggle | ui-stub | 994 |
| O-GrainScatter | Source/ui/public | 350 / 588 / 937 / 1439 | A' | 0 | 127 | 3 | — | en/fr/zh-Hans | 16 / 2 / 0 / 0 | 2 / 0 | — | 900x800 | tips-toggle | tip_render | 201 |
| O-IntonationPad | Source/ui/public | 273 / 1449 / 1933 / 1467 | B | 0 | 74 | 5 | tuning-panel.css, tuning-panel.js | en/fr/zh-Hans | 21 / 2 / 0 / 0 | 3 / 0 | — | 800x500 | tips-toggle | — | 824 |
| O-Lyrica | Resources/ui | 792 / 2125 / 3400 / 1273 | B+R | 0 | 142 | 5 | — | en/fr/zh-Hans | 23 / 2 / 0 / 0 | 3 / 0 | — | 700x450 | tips-toggle | — | 1142 |
| O-Marimba | Source/ui/public | 335 / 1262 / 1672 / 859 | A' | 0 | 168 | 3 | — | en/fr/zh-Hans | 20 / 2 / 0 / 0 | 3 / 0 | — | 600x400 | tips-toggle | — | 937 |
| O-MicrotonalSampler | Resources/ui | 740 / 2563 / 4117 / 1922 | B+R | 13 | 40 | 7 | tuning-panel.css, tuning-panel.js | en/fr/zh-Hans | 63 / 14 / 0 / 0 | 2 / 0 | true | 900x640 | tips-toggle | ui-stub, tip_render | 1066 |
| O-MultiBandCompressor | Source/ui/public | 468 / 1336 / 1978 / 812 | A+R | 0 | 200 | 3 | — | en/fr/zh-Hans | 43 / 48 / 40 / 4 | 4 / 0 | — | 900x640 | tips-toggle | — | 221 |
| O-Octagon | Source/ui/public | 952 / 1919 / 4271 / 1514 | R | 26 | 17 | 2 | — | en/fr/zh-Hans | 123 / 19 / 1 / 0 | 2 / 0 | — | 1100x720 | tips-toggle | ui-stub, frontend, layout | 442 |
| O-Orbit | Resources/ui | 419 / 894 / 1558 / 911 | A' | 0 | 159 | 1 | preset-manager.js | en/fr/zh-Hans | 25 / 4 / 0 / 0 | 0 / 0 | true | 800x600 | help-toggle | ui-stub | 508 |
| O-Polystutter | Source/ui/public | 623 / 1165 / 1652 / 924 | unclassified | 0 | 98 | 4 | preset-manager.js | en/fr/zh-Hans | 20 / 2 / 0 / 0 | 3 / 0 | — | 1000x690 | tips-toggle | — | 824 |
| O-Prism | Source/ui/public | 812 / 1801 / 3297 / 2573 | B+R | 0 | 221 | 6 | — | en/fr/zh-Hans | 11 / 1 / 0 / 0 | 1 / 0 | — | 1200x800 | tips-toggle | ui-stub, tip_render | 455 |
| O-Reed | Resources/ui | 441 / 1071 / 903 / 1525 | C | 25 | 21 | 3 | — | en/fr/zh-Hans | 20 / 10 / 0 / 0 | 3 / 0 | — | 900x600 | tips-toggle | tip_render | 248 |
| O-ReverseDelay | Source/ui/public | 651 / 1534 / 1708 / 940 | A | 28 | 22 | 1 | — | en/fr/zh-Hans | 47 / 7 / 1 / 4 | 10 / 1 | — | 940x768 | tips-toggle | ui-stub, frontend, tooltip_clamp | 458 |
| O-simpleAdditive | Source/ui/public | 282 / 807 / 955 / 1115 | A | 31 | 51 | 3 | — | en/fr/zh-Hans | 25 / 6 / 2 / 8 | 7 / 0 | — | 860x930 | help-toggle | — | 645 |
| O-simpleBeatmaker | Source/ui/public | 218 / 664 / 1081 / 997 | A | 29 | 35 | 4 | — | en/fr/zh-Hans | 22 / 5 / 3 / 4 | 2 / 0 | true | 1060x900 | help-toggle | — | 172 |
| O-simpleFM | Source/ui/public | 293 / 905 / 1126 / 992 | A | 28 | 42 | 3 | — | en/fr/zh-Hans | 26 / 7 / 4 / 4 | 9 / 0 | — | 760x980 | help-toggle | — | 644 |
| O-simpleGrain | Source/ui/public | 342 / 1171 / 1358 / 1224 | A | 28 | 49 | 3 | — | en/fr/zh-Hans | 22 / 6 / 1 / 4 | 9 / 0 | — | 900x760 | help-toggle | — | 690 |
| O-simplePhysicalModelSynth | Source/ui/public | 323 / 680 / 1010 / 891 | A | 28 | 41 | 4 | — | en/fr/zh-Hans | 29 / 6 / 2 / 4 | 8 / 0 | — | 1040x860 | help-toggle | — | 159 |
| O-SimpleReverb | Source/ui/public | 284 / 795 / 972 / 985 | B | 4 | 47 | 1 | preset-manager.js | en/fr/zh-Hans | 24 / 2 / 0 / 0 | 3 / 0 | — | 500x350 | tips-toggle | tip_render | 1063 |
| O-simpleSampler | Source/ui/public | 315 / 959 / 1450 / 1006 | A | 28 | 44 | 3 | — | en/fr/zh-Hans | 19 / 6 / 2 / 4 | 9 / 0 | — | 980x720 | help-toggle | — | 212 |
| O-simpleSubtractive | Source/ui/public | 331 / 824 / 1002 / 1043 | A | 29 | 39 | 3 | — | en/fr/zh-Hans | 21 / 4 / 3 / 4 | 9 / 0 | true | 1180x820 | help-toggle | — | 183 |
| O-SpectralShaper | Resources/ui | 193 / 1086 / 2849 / 764 | A' | 23 | 107 | 1 | preset-manager.js | en/fr/zh-Hans | 20 / 5 / 0 / 0 | 3 / 0 | — | 700x500 | tips-toggle | ui-stub, preset_menu | 359 |
| O-Strata | Source/ui/public | 777 / 1700 / 3622 / 2726 | B+R | 0 | 208 | 6 | — | en/fr/zh-Hans | 18 / 1 / 0 / 0 | 1 / 0 | — | 1200x800 | tips-toggle | ui-stub, layout, shell_diff, tip_render | 817 |
| O-Tapestop | Source/ui/public | 417 / 1196 / 1761 / 875 | A | 29 | 21 | 1 | — | en/fr/zh-Hans | 47 / 10 / 3 / 4 | 10 / 0 | — | 860x580 | help-toggle | ui-stub, tooltip_clamp | 514 |
| O-Texture | Source/ui/public | 282 / 685 / 881 / 732 | D | 12 | 12 | 1 | — | en/fr/zh-Hans | 20 / 2 / 0 / 0 | 3 / 0 | — | 800x600 | tips-toggle | tip_render | 432 |
| O-TextureForge | Source/ui/public | 226 / 816 / 1214†‡ / 795 | A' | 16 | 15 | 3 | — | en/fr/zh-Hans | 14 / 2 / 0 / 0 | 3 / 0 | — | 900x600 | tips-toggle | tip_render | 645 |
| O-Tremolo | Source/ui/public | 188 / 1055 / 1015 / 818 | A'+R | 0 | 138 | 1 | preset-manager.js | en/fr/zh-Hans | 25 / 2 / 0 / 0 | 3 / 0 | — | 600x400 | tips-toggle | tip_render | 2742 |
| O-Wind | Resources/ui | 408 / 1267 / 1305 / 1443 | B+C | 29 | 26 | 5 | — | en/fr/zh-Hans | 23 / 10 / 0 / 0 | 2 / 0 | — | 900x600 | tips-toggle | tip_render | 834 |

"Vendored copies" lists files present inside the UI root at HEAD. Direct-embed consumers (§1.3) pull module files from `modules/` at build time, so no copy appears here. O-Orbit's copy lives at `Resources/ui/js/modules/`.

### A.2 Duplication — recurring files (copies / distinct md5 / match to canonical)

| Recurring file | Copies | Distinct md5 | Canonical source | Copies matching canonical | Largest hash groups |
|---|---|---|---|---|---|
| js/i18n.js | 44 | 44 | none. Tables are per-plugin by design; the runtime region is canon-gated (A.3) | n/a | all singletons |
| js/juce/check_native_interop.js | 44 | 4 | none in repo | n/a | 3c4db27f×28, efa284d8×7, e560b307×6, 928d5975×3 |
| js/juce/index.js | 44 | 3 | none in repo | n/a | 01ff05e3×32, c52a4c52×7, 0fe3c2a8×5 |
| tests/ui_tip_render_check.js | 23 | 23 (23 even after replacing the plugin name with a placeholder) | none | n/a | all singletons; 17,564 lines total |
| vendored preset-manager.js | 12 | 7 | modules/persistence/preset-manager/js/preset-manager.js | 6 | 32c43220×6 (canonical), then O-Bass, O-Comp, O-DigiDelay, O-FreqPulse, O-Polystutter, O-SpectralShaper as singletons |
| tests/ui-stub/generic-overrides.json | 6 | 6 | none | n/a | all singletons |
| tests/ui-stub/juce-stub.js | 5 | 5 | scripts/ui-stub/generic-juce-stub.js is the generic alternative | n/a | all singletons |
| vendored tuning-panel.js | 4 | 4 | modules/tuning/scala-tuning-engine/js/tuning-panel.js (v3.2.0) | 0 | O-Bells, O-Formant, O-IntonationPad, O-MicrotonalSampler; diffs of 395, 241, 534, and 411 lines |
| tests/ui-stub/serve-stub.sh | 4 | 4 | none | n/a | all singletons |
| vendored tuning-panel.css | 3 | 3 | modules/tuning/scala-tuning-engine/snippets/tuning-panel.css | 0 | all singletons |
| tests/ui_tooltip_clamp_check.js | 3 | 3 (3 after name normalization) | none | n/a | 2,462 lines total |
| tests/ui_frontend_check.js | 3 | 3 (3 after name normalization) | none | n/a | 3,964 lines total |
| tests/ui_preset_menu_check.js | 2 | 2 | none | n/a | — |
| tests/ui_layout_check.js | 2 | 2 | none | n/a | — |
| paper texture JPG (`paper*.jpg`) | 19 | 3 | none | n/a | 0a2a916f×14 (800×600, 207 KB) |
| font binaries | 0 | — | — | — | no plugin bundles a font |

### A.3 Duplication — recurring hand-copied functions (named `function X(`, defined in at least 8 plugins)

"Distinct" counts md5 of the body after stripping comments and all whitespace. Per-name spread for knob code: 51 distinct knob/drag function names suite-wide. `setupKnob` 7/7, `createKnobSVG` 5/5, `makeFxKnob` 3/3, `setupFxKnob` 3/3. Tooltip code uses 17 distinct tip/tooltip function names.

| Function (by name) | Plugins defining it | Distinct normalized bodies | Largest identical group | Median length (lines) |
|---|---|---|---|---|
| `applyI18n` | 44 | 1 | 44 | 19 |
| `applyI18nAttributes` | 44 | 1 | 44 | 7 |
| `applyLabel` | 44 | 1 | 44 | 10 |
| `initI18n` | 44 | 1 | 44 | 23 |
| `setLabel` | 44 | 1 | 44 | 6 |
| `tr` | 44 | 2 | 28 | 20 |
| `trLabel` | 44 | 1 | 44 | 12 |
| `applyTipsEnabled` | 31 | 3 | 23 | 13 |
| `setupTooltips` | 29 | 18 | 8 | 118 |
| `setSettingsPopoverOpen` | 25 | 1 | 25 | 5 |
| `initializeTipsToggle` | 23 | 22 | 2 | 14 |
| `initializeSettingsPopover` | 22 | 6 | 10 | 30 |
| `updateKnobVisual` | 20 | 15 | 5 | 16 |
| `bindKnob` | 15 | 13 | 3 | 60 |
| `initSettingsPopover` | 14 | 6 | 5 | 34 |
| `bindToggle` | 11 | 11 | 1 | 26 |
| `handleTooltipOut` | 10 | 2 | 8 | 11 |
| `handleTooltipOver` | 10 | 3 | 8 | 11 |
| `hideTooltip` | 10 | 2 | 8 | 8 |
| `showTooltip` | 10 | 4 | 7 | 94 |
| `hidePresetDropdown` | 9 | 4 | 6 | 3 |
| `normToDeg` | 9 | 1 | 9 | 1 |
| `nudge` | 9 | 4 | 6 | 7 |
| `showPresetDropdown` | 9 | 9 | 1 | 40 |
| `bindCombo` | 8 | 5 | 4 | 31 |
| `formatValue` | 8 | 7 | 2 | 8 |
| `initializeHelpToggle` | 8 | 3 | 4 | 28 |
| `initializeTooltips` | 8 | 7 | 2 | 22 |
| `setTooltipsEnabled` | 8 | 7 | 2 | 23 |
| `tipAllowed` | 8 | 1 | 8 | 3 |

### A.4 Visual pass metrics (10 sampled plugins, shipping frame, HEAD snapshot)

These metrics come from `shots.js` (Appendix B step 7).

- **Contrast** is the WCAG 2.x ratio of each visible element's own text colour, composited with its alpha and opacity, against its nearest opaque ancestor's background colour. The AA threshold is 4.5:1, or 3:1 for text of 24 px and up (18.66 px and up if bold).
- **"Text over image bg"** counts elements whose ancestor chain carries a `background-image`. Their true contrast depends on the image and is not captured by the ratio.
- **Resolved face** is from CDP `CSS.getPlatformFontsForNode` on the first `[data-i18n]` element, with glyph count in brackets.

| Plugin | Frame | Stub | Visible text els | Contrast median / min | Below WCAG AA | Text over image bg | Font sizes (px) | Declared first family | Resolved face (CDP) | Knob-like controls / keyboard-focusable |
|---|---|---|---|---|---|---|---|---|---|---|
| O-ReverseDelay | 940x768 | plugin | 83 | 9.39 / 1.79 | 4 (5%) | 67 | 9, 9.5, 10, 11, 13, 25 | Garamond | Times New Roman (38) | 21 / 21 |
| O-Prism | 1200x800 | generic | 146 | 3.44 / 2.57 | 109 (75%) | 0 | 8, 9, 10, 12, 13, 14, 15 | Garamond | Georgia (32) | 94 / 0 |
| O-Gain | 380x500 | generic | 73 | 7.67 / 3.66 | 9 (12%) | 0 | 8, 9, 10, 11, 12, 13, 14, 32 | Garamond | — (first node had no glyphs) | 3 / 0 |
| O-Octagon | 1100x720 | plugin | 104 | 6.7 / 2.86 | 24 (23%) | 13 | 8, 9, 10, 11, 12, 13, 22 | Iowan Old Style | Iowan Old Style (20) | 18 / 18 (native ranges) |
| O-MicrotonalSampler | 900x640 | generic | 111 | 6.83 / 2.16 | 49 (44%) | 24 | 10, 11, 12, 13, 14, 15, 20 | Garamond | Times New Roman (10) | 5 / 5 (native ranges) |
| O-simpleSubtractive | 1180x820 | generic | 61 | 9.32 / 2.52 | 9 (15%) | 47 | 8, 8.5, 9, 9.5, 10, 11, 12, 13, 18, 26 | Garamond | Times New Roman (60) | 16 / 16 |
| O-Bells | 800x600 | generic | 74 | 10.45 / 2.03 | 17 (23%) | 8 | 6, 7, 8, 9, 10, 11, 13, 22 | Garamond | Times New Roman (4) | 0 / 0 (custom sliders) |
| O-TextureForge | 900x600 | generic | 29 | 4.49 / 1.45 | 20 (69%) | 28 | 8, 9, 10, 12, 13, 14, 18 | Garamond | — (first node had no glyphs) | 11 / 0 |
| O-Bassoon | 900x600 | generic | 32 | 5.3 / 3.32 | 8 (25%) | 0 | 8.5, 9.5, 10, 10.5, 11, 12, 16 | EB Garamond | Georgia (34) | 20 / 0 |
| O-Chorus | 700x125 | generic | 24 | 8.79 / 5.43 | 0 (0%) | 0 | 8, 9, 10, 11, 14, 16 | Times | Times New Roman (4) | 16 / 0 |

### A.5 Palette, type, and layout census (all 44, shipped CSS = external `.css` + inline `<style>`, excluding vendored module files)

| Measure | Value |
|---|---|
| Plugins defining ≥1 CSS custom property / none | 26 / 18 |
| Distinct custom-property names suite-wide | 153 (most shared: `--bg-paper`, `--brown-text`, `--brown-border`, `--brown-frame`, `--green-light`, `--green-mid`, `--green-dark`, each in 17 plugins) |
| Distinct hex colours suite-wide / used by exactly 1 plugin | 248 / 173 |
| Per-plugin distinct hex (min / median / max) | 8 / 17 / 38 |
| Brand-core hex reach | `#3C2F2F` 40, `#8B7355` 39, `#5C4033` 34, `#F5E6D3` 32, `#6B8E4E` 31, `#8BA870` 29 plugins |
| Distinct `font-family` stack strings suite-wide | 43 |
| Bundled font files | 0 |
| Distinct px font sizes suite-wide; per plugin min / median / max | 27; 4 / 8 / 14 |
| Plugins declaring a font-size < 9 px | 34 (floor 6 px: O-Bells, O-IntonationPad, O-Lyrica, O-Marimba) |
| px vs em/rem lengths | 12,191 vs 62 |
| Distinct px values in margin/padding/gap | 56 (top: 8, 4, 6, 10, 2, 3, 12, 5, 14 px) |
| Plugins using flex / grid / absolute | 44 / 23 / 44 |
| Plugins with `@media` rules | 2 (O-MicrotonalSampler, O-ReverseDelay) |
| Plugins with language-scoped CSS selectors (`:lang()`, `[lang=]`) | 9; all 44 carry CSS comments about French/zh widths |
| Paper textures | 19 JPGs, 3 distinct, all 800×600 |
| Mockup dirs / versions | 10 plugins / 34 versions (§3.1) |

<!-- APPENDIX-A-EXTRA -->

## Appendix B — Method

Everything below ran from the session scratchpad, which is outside the repo. The only file written into the repo is this report.

1. **Baseline.** Ran `git rev-parse HEAD` (gave `b7528dc3d75c…`) and `git status --porcelain -- plugins modules .claude scripts .github`, saved as `nho-baseline.porcelain`.
2. **Snapshot.** `git archive b7528dc3 -- ':(glob)plugins/*/Source/ui/**' ':(glob)plugins/*/Resources/ui/**' ':(glob)plugins/*/CMakeLists.txt' ':(glob)plugins/*/Source/PluginEditor.*' ':(glob)plugins/*/tests/ui*' ':(glob)plugins/*/tests/ui-stub/**' ':(glob)plugins/*/.planning/mockups/**' ':(glob)plugins/*/.planning/params.tsv' modules scripts .github .claude | tar -x -C $SCRATCH/snap`. Tracked files only, so the gitignored configure-time `Source/ui/public/modules/` copies are excluded.
3. **Census (`census.js`, node, no dependencies).** For each `plugins/O-*`:
   - **Root:** the `(Source/ui/public|Resources/ui)/index.html` named in `CMakeLists.txt`. Fallback: whichever folder has an `index.html`.
   - **Walk:** images and fonts are counted separately. `js/juce/*` and `modules/*` go to the duplication table. `i18n.js` is counted as i18n lines. A file with `sourceMappingURL` or any line over 2,000 chars counts as bundled.
   - **Line counts:** `.html` is split into markup, inline `<style>` (CSS), and inline non-`src` `<script>` (JS). `Source/ui/src/*.js` is added for O-TextureForge.
   - **Knob family,** first match wins per group: `knob-stem` → A; else `conic-gradient` in CSS/HTML → A′; `knob-vine` → B; `knob-fill` without conic/stem, or `drawKnob` → C; bare `knob-indicator` → D; `type="range"` → R.
   - **CSS vars:** `/(^|[\s;{])--[\w-]+\s*:/g`. **Hex:** `/#[0-9a-fA-F]{3,8}\b/g`. **Fonts:** distinct `font-family:` values.
   - **Languages:** `en: {`, `fr: {`, `'zh-Hans': {` in `i18n.js`.
   - **A11y sites:** `aria-*=` and `setAttribute('aria-`; `role=`; `tabindex=`, `tabIndex=`, `setAttribute('tabindex'`; `'Arrow(Up|Right|Down|Left)'`; `:focus-visible`; `prefers-reduced-motion`.
   - **Editor size:** `setResizable\s*\(\s*(true|false)` and the last `setSize(w, h)` in `Source/PluginEditor.cpp`.
   - **Gates:** `tests/ui*` entries.
   - **md5:** computed on every recurring file. Vendored copies are compared against `modules/**/js|css/<same basename>`.
4. **Function census (`funcs.js`).** Collected every `function NAME(…) {` in shipped authored JS (external plus inline, excluding `js/juce`, `modules/`, vendored module files, and bundles). Bodies were extracted by brace matching with string/comment awareness, normalized by stripping comments and all whitespace, then md5'd. Reported names defined in 8 or more plugins; knob and tip names were listed at 1 or more.
5. **Module consumption.** Ran `grep -oE "ouaricon_add_module\([^)]*\)" plugins/*/CMakeLists.txt` and `grep -h '^\s*\${CMAKE_SOURCE_DIR}/modules/.*\.\(js\|css\)' plugins/*/CMakeLists.txt`, plus a repo-wide grep for `instrument-footer-panel|playable-keyboard` under `plugins/*/{CMakeLists.txt,Source/ui,Resources/ui}`, which returned no hits.
6. **Gate copy normalization.** `sed -e "s/$P/__P__/g" -e "s/${P#O-}/__S__/g" tests/<gate> | md5` per copy.

7. **Visual pass (`shots.js`).**
   - Setup: `require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js')`, then `resolvePlaywright()`, then for each sampled plugin `buildRoot(name, {repoRoot: $SCRATCH/snap})`. That copies into `fs.mkdtempSync(os.tmpdir())`, places embedded module files, and overlays the stub.
   - Render: `readEditorSize(name, snap)`, then `serve(root)` (port 0), then Chromium with `viewport = setSize` and `deviceScaleFactor 1`. Wait 2.5 s, then screenshot.
   - Measure: in-page `getComputedStyle` walk over visible elements that own a text node, then CDP `DOM.getDocument` / `DOM.querySelector` / `CSS.getPlatformFontsForNode`. Afterwards the temp root is removed.
   - Spot checks (`probe.js`): scroll containers (`overflowY auto|scroll && scrollHeight > clientHeight`), `elementFromPoint` on the O-Gain meter labels, a 3× clip screenshot, and font classes per element.
8. **Palette / type / layout (`palette.js`, `type.js`, `layout.js`).**
   - Hex values were normalized to 6-digit uppercase and counted once per plugin.
   - Font sizes come from `font-size:\s*(\d+(?:\.\d+)?)px`.
   - Spacing comes from `(margin|padding|gap)[a-z-]*: … Npx`.
   - Layout counts are `position:absolute`, `display:grid`, and `display:(inline-)flex`. Custom-property names are `--x` followed by `:`.
   - WCAG helper: relative luminance with sRGB linearization, ratio `(L1+0.05)/(L2+0.05)`. AA-passing variants were found by scaling RGB by k, stepping k down by 0.01 until the ratio reached 4.5 or more.
9. **Font availability.** `ls /System/Library/Fonts /System/Library/Fonts/Supplemental /Library/Fonts ~/Library/Fonts | grep -i garamond` returns nothing.
10. **Pipeline docs.**
    - Sizes: `wc -l` over the ui-mockup `SKILL.md`, `BOUNDARIES.md`, `references/*.md`, both agents, `ui-template-library/SKILL.md`, and `aesthetic-dreaming/SKILL.md` gives 8,066 lines.
    - Term search: `grep -c -- <term>` per file for each term listed in §3.3.
    - Dates: `git log -1 --format=%ad --date=short -- <doc>`.
    - Mockups: `ls plugins/*/.planning/mockups | grep -oE '^v[0-9]+' | sort -u`.

