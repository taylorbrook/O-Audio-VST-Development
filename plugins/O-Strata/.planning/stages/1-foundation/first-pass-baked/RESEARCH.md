# Stage 1 — Foundation: RESEARCH

**Plugin:** O-Strata · **Stage:** 1 of 4 (Foundation + Shell) · **Phase:** research
**Researched:** 2026-09-07 · **Domain:** codebase investigation of `plugins/O-Prism/` v1.24.0 (fork base) · **Confidence:** HIGH
**Method:** every claim below was read from the O-Prism tree at HEAD (`git status --short plugins/O-Prism` empty; `CMakeLists.txt:12 VERSION 1.24.0`) with `grep`/`awk`/`wc` this session. Line numbers are O-Prism v1.24.0 line numbers and shift as soon as an edit lands above them — re-locate by the quoted token, never by the number (memory `pattern_deferred_item_line_number_drifts_record_a_token`).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Inherited contracts — NOT re-opened** (CONTEXT.md table): fork base `plugins/O-Prism/` at v1.24.0; target/folder `O-Strata`; `PRODUCT_NAME "O-Strata${OUARICON_DEV_SUFFIX}"`; `PLUGIN_CODE OuSt`; **`VERSION 1.0.0`** inside `juce_add_plugin`, never `PLUGIN_VERSION`; formats VST3 AU Standalone, `IS_SYNTH TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; modules = O-Prism's 13 + `juce::juce_cryptography`; **219** parameters = 171 inherited + 48 geometry, `oscATable`/`oscBTable` removed; parameter order = O-Prism's `createParameterLayout()` minus the two Table params, 24 geometry params appended **inside each oscillator's block** in the "all 48 IDs" table order; geometry IDs camelCase `oscA…`/`oscB…` + `GeoSource GeoFrames GeoDrive Mesh MeshTiltX MeshTiltY MeshPhi MeshUnwrap MeshLoop VolField VolOrbit VolSweepAxis VolSweepRange VolDetail Terrain TerOrbit TerCX TerCY TerAspect TerRot TerSweepAxis TerSweepRange TerBlur TerEdge`; choice lists = English display strings exactly as the spec tables, `Imported…` always last, Terrain list is the 3-entry mockup list; bake params are plain `AudioParameterFloat`/`AudioParameterChoice`, **not** added to `modSlot?Dst` (26 entries), no APVTS listener; `osc?Pos` unchanged; `oscIds()` drops `"Table"`, appends the 24 suffixes → 35 per oscillator, `allSliderIds()` 141 → 187 (+1 for D4 = **188**); `createEditor()` wrapped in `#if JUCE_WEB_BROWSER … #else GenericAudioProcessorEditor`; exactly one `juce_add_binary_data` target `O-Strata_UIResources`; `geometryImports` child **stubbed empty** in get/setStateInformation, `tuningEngine`/`uiLanguage` inherited; removal set = `WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor` (.h/.cpp), `wavetable-editor.js`, `wavetable-editor.css`, `oscATable`/`oscBTable`, `pOscATable`/`pOscBTable`, the user-wavetable ValueTree child, `resolveActiveTable` → `oscTablePtr[osc]`, 14 native functions, their i18n rows; kept = `WavetableGenerator` (+ `WavetableData.h`), `generateProceduralTable(Sine)` as the initial table; root CMake auto-discovers `plugins/*` — no root edit; `ouaricon_add_param_dump(O-Strata …)` behind `OUARICON_BUILD_TESTS`, expect 219 rows, diff vs O-Prism = −2 +48.

**Read-only in this stage:** `parameter-spec.md` (locked v1), `BRIEF.md`, `REQUIREMENTS.md`, everything under `plugins/O-Prism/`.

**D1 — Full rename: O-Strata is a new plugin, O-Prism stays untouched.** Every `Prism`/`OPrism` identifier becomes its `Strata` counterpart in one pass (`OPrismAudioProcessor`→`OStrataAudioProcessor` 41, `OPrismAudioProcessorEditor`→`OStrataAudioProcessorEditor` 20, `PrismVoice`(+files)→`StrataVoice` 27, `PrismSound`(+file)→`StrataSound` 7, `PrismParamIds`(+file)→`StrataParamIds` 9, `"OPrismParameters"`→`"OStrataParameters"` 1, `"O-Prism"` in `getName()`/`presetManager(parameters, "O-Prism")`/tuning HTML export title/file headers→`"O-Strata"`, `"OPrism_WebView"`→`"OStrata_WebView"` 1, `O-Prism_UIResources`→`O-Strata_UIResources` 2). No file under `plugins/O-Prism/` is created, modified or deleted; every literal `Prism` token remaining in `plugins/O-Strata/Source/` after the rename is a defect to explain or remove — only fork-origin comments tolerated. `presetManager (parameters, "O-Strata")`; `FactoryPresets.cpp` carried over with `oscATable`/`oscBTable` entries stripped explicitly. APVTS Identifier change means O-Prism state cannot load into O-Strata — intended.

**D2 — GeometryBakeScheduler skeleton deferred to Phase 2.1.** No `Source/geometry/`, no bake code; `oscTablePtr[osc]` initialised once in the constructor to the sine placeholder. CMake snippet hunk 2 `Source/geometry/*.cpp` lines NOT applied (only the three removals); hunk 4's `js/geometry-view.js` and the shell PNG are Stage 3 — `O-Strata_UIResources` lists exactly `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`.

**D3 — UI: strip only.** `index.html` loses the wavetable tab/editor/selection UI, the `wavetable-editor.js`/`.css` references and the JS that called the 14 removed native functions; no geometry controls added; oscillator panels keep 11 controls each. Relays/attachments still generated for all 188 slider IDs; `jassert (sliderAttachments.size() == sliderRelays.size())` holds at 188. `i18n.js`: remove the wavetable-tab keys (en/fr/zh-Hans rows together — all three or none), no new keys. `check-i18n`, `check-ui-labels`, `boot-all-uis` must pass; no dead tip bindings. 14 removed native functions: `getUserWavetableList importUserWavetable importUserWavetableData selectUserWavetable clearUserWavetableOverride deleteUserWavetable startWavetableEditor stopWavetableEditor getEditorFrameWaveform getFrameHarmonics setFrameHarmonics applyFrameOperation saveEditedWavetable getAllEditorFrameWaveforms`; **kept** `getActiveOscInfo`, `getActiveOscFrame`; 45 → 31. `timerCallback`'s `evaluateJavascript` push left as-is.

**D4 — `delayDivision` added to `allSliderIds()`.** 188 sliders + 30 toggles = 218 relays for 219 parameters (only `stereoWidth` unbound). Spec is not edited; CONTEXT is the record.

**D5 — Count corrections propagate to ROADMAP / ARCHITECTURE** (24 stale occurrences: `217`, `46`, `23 ×`, `126 → 170`, `170`) and to the Stage 1 test criteria; STATUS.md checksums for `roadmap`/`architecture` re-recorded; the `// 126` comment corrected in the fork.

**D6 — Fork mechanics.** Copy only `Source/` (all, then delete the removal set), `CMakeLists.txt`, `tests/` (`i18n-states.json`, `ui_tip_render_check.js`, `ui-stub/generic-overrides.json`). Do not copy `backups/`, `BUG-tuning-tab-cutoff.md`, `CHANGELOG.md` (new stub), `NOTES.md` (exists). Fresh `CHANGELOG.md` `## v1.0.0 (unreleased)` naming the fork base; `NOTES.md` gets a Stage 1 entry.

**Constraints carried into implementation:** two-way grep gate on the removal set; O-Prism isolation gate (`git status --short plugins/O-Prism` empty; `git diff --stat HEAD -- plugins/O-Prism` empty); Choice params ≥ 2 choices; param-ID identifiers must not shadow `juce::` free functions; `VERSION 1.0.0` and pluginval header prints `1.0.0`; `O-Strata_Standalone` built explicitly; cold `auval -a | grep -i strata` in the background at batch end, 2-minute budget; `git commit -- plugins/O-Strata PLUGINS.md` after `git branch --show-current` / `git status --short`, untracked files `git add`ed first; no tags.

### Claude's Discretion

CONTEXT.md has no explicit discretion section. The five "Open for the research phase" items are answered in §2 below; where an answer required a choice not fixed by CONTEXT (e.g. which CSS rules migrate, how `getActiveOscInfo` is rewired, non-ASCII choice strings) it is marked **[recommendation]** and listed in §10.

### Deferred Ideas (OUT OF SCOPE)

Bake scheduler skeleton (→ 2.1); geometry panel / `geometry-view.js` / shell PNG (→ 3.x); event-push timer (→ 3.x, ARCHITECTURE Decision 7); factory-preset re-authoring (→ Stage 4); `stereoWidth` UI (untouched); any O-Prism edit.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description (REQUIREMENTS.md / CONTEXT traceability) | Research support |
|----|---|---|
| COMPAT-01 | pluginval strictness 10 passes VST3 + AU; auval passes | §6 exact commands (`/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 …`, `auval -v aumu OuSt OuDv`); §3 choice-list ≥ 2 and NaN-free defaults; §8 pitfalls P1–P4 |
| FUNC-09 / FUNC-10 (smoke only) | sine placeholder plays on both oscillators; Scala file loads | §2.3 (placeholder table wiring), §7 (tuning state untouched) |
</phase_requirements>

---

## 1. Summary

1. **O-Prism v1.24.0 has 173 parameters** (`params.tsv` header `# params 173`; layout sums 12+12+6+4+6+5+5+1+7+7+7+4+4+5+20+64+4 = 173 `[VERIFIED: PluginProcessor.cpp:73-491]`). `allSliderIds()` holds **141** (the `// 126` comment at `PrismParamIds.h:134` is stale); toggles = 30. After the fork: 219 params, 188 sliders (35×2 + 117 + `delayDivision`), 30 toggles.
2. **`createEditor()` is already guarded** in O-Prism (`PluginProcessor.cpp:1265-1276`, comment at :32-37) — the Stage 1 task is *verify present*, not add. `[VERIFIED]`
3. **There is no `oscTablePtr` in O-Prism.** The live mechanism is `userTablePtrA/B` (atomics) + `pOscATable/pOscBTable` + `factoryTables` resolved by `resolveActiveTable()` (:1043-1055) every block from `updateWavetableAssignments()` (:980-1000). The minimal edit is a *rename-and-simplify*: `std::atomic<const WavetableData*> oscTablePtr[2]` replaces `userTablePtrA/B`, `resolveActiveTable` collapses to `oscTablePtr[osc].load()`, the retire/reaper/`blockGeneration`/`lastAssignedTable*` machinery stays byte-for-byte (§2.3).
4. **The preset-save modal depends on `css/wavetable-editor.css`** — `.wt-save-modal-overlay`, `.wt-save-modal`, `.wt-save-modal-buttons`, `.wt-op-btn` are defined only there (css:189-201, 225-269) and used by the surviving `#preset-save-modal-overlay` (index.html:2054-2063). Deleting the file as-is breaks preset saving. Migrate those four rule blocks into `index.html`'s `<style>` (§2.1).
5. **i18n triage:** delete 23 LABELS keys + 2 I18N tip entries + 2 TIP_BINDINGS rows + the 28-name wavetable-catalogue `I18N_EXEMPT` block; rename `'O-PRISM'` exempt → `'O-STRATA'`; every other "table" hit is prose inside a surviving entry or a comment (§2.1). check-i18n [15] would fail on any dead LABELS key left behind, and [6] byte-compares a canon region (`index.html:2110-2206`) that contains no `Prism` token — the rename cannot touch it.
6. **`FactoryPresets.cpp` carries 385 `osc?Table` references** (202 A + 183 B, incl. `completeBase()` :123/:128) plus a `WT_*` enum (:104-114); a two-expression BSD `sed` + one line-range delete removes all of them cleanly (dry-run in scratchpad: 0 left, braces balanced 2807/2807). Every preset's audible identity depended on a non-sine table (base default `WT_Saw`; `WT_Sine` appears on only 36 of 385 slots) — Stage 4 note, not a Stage 1 fix.
7. **Rename is a three-case sed over 62 files** (`Prism`/`PRISM`/`prism`), safe as a blanket because no legitimate English word containing "prism" exists in the tree; the fork-origin notes and the fresh CHANGELOG are written *after* the sed so they are not rewritten (§5).
8. **Locked spec strings contain non-ASCII** (`Imported…` U+2026, `Epitrochoid 3·5·7`/`Hypocycloid 3·5·7` U+00B7, host name `Projection φ`). `juce::String (const char*)` is ASCII-only (memory `critical_juce_string_char_ctor_is_ascii_only`) and no `/utf-8` MSVC flag exists in the repo: build these with `juce::CharPointer_UTF8 ("Imported\xE2\x80\xA6")` (hex escapes keep the source ASCII; precedent O-Octagon/O-Texture) — §3, §10.
9. **Gates need no allowlist entry** — `check-i18n`, `check-ui-labels`, `boot-all-uis`, fr/zh lint and `serve-ui.js` discover plugins by `readdirSync(plugins/)` + CMake; O-Strata joins automatically once `Source/ui/public/js/i18n.js` exists. O-Strata *today* is the cause of the repo-wide fr/zh lint exit 2 ("1 plugin(s) could not be read"); the fork fixes that as a side effect.
10. **Concurrent session alert:** at research time the working tree carries uncommitted edits under `plugins/O-simpleAdditive/` (wave 4g) — path-scoped commits are mandatory, `git add plugins/O-Strata` explicitly (new files are untracked), then `git show --stat` to confirm.

**Primary recommendation:** execute in the order §9 gives (copy → sed rename → delete removal set → C++ processor/editor edits → params → CMake → UI strip + CSS migration → i18n → tests → build/param-dump → gates → docs), with the grep gates run after step 3 and again at the end.

---

## 2. Open items 1–5 resolved

### 2.1 Open item 1 — `index.html` wavetable regions + `i18n.js` keys

**Files (all under `plugins/O-Prism/Source/ui/public/`):** `index.html` (4740 lines), `js/i18n.js` (2568), `js/wavetable-editor.js` (660), `css/wavetable-editor.css` (269). `[VERIFIED: wc -l]`

**Tab structure** `[VERIFIED: index.html:1424-1429, 1438, 1755, 1772, 1908, 1990]`: five tabs `synth | mod | tuning | effects | wavetable`; `#wavetable-tab` at :1990-2039.

**External references to remove** `[VERIFIED: index.html:4736-4737]`:
```html
<link rel="stylesheet" href="./css/wavetable-editor.css">
<script src="./js/wavetable-editor.js"></script>
```
`wavetable-editor.js` is a non-module IIFE exporting `window.WavetableEditor` (:25, :660) and reaching JUCE through `window.JuceGetNativeFunction` (:48), which `index.html:2090-2091` exposes solely for it (`// Expose getNativeFunction for non-module scripts (wavetable-editor.js)`). It references no i18n key (grep for `label.|aria.|setLabel|data-i18n` = 0 hits).

**DOM/CSS/JS regions belonging to the wavetable UI** (O-Prism line numbers; action):

| Region | Lines | Contents | Action |
|---|---|---|---|
| CSS drag-drop overlay | 661-679 | `.wt-drop-overlay`, `.osc-canvas-wrap.drag-over .wt-drop-overlay` | delete (keep 658-660 `.osc-canvas-wrap { position: relative; }` — the canvas wrap survives) |
| CSS manager modal | 681-760 | `.wt-modal-backdrop`, `.wt-modal`, `.wt-modal h3`, `.wt-modal-list`, `.wt-delete-btn`, `.wt-modal-close`, `.wt-modal-empty` | delete |
| CSS line-height pins | 1222 `.wt-label,` · 1224 `.wt-modal-close` · 1244 `.wt-waveform-preview-label,` · 1245 `.wt-modal-list .wt-delete-btn,` · 1257 `.wt-osc-btn` · 1263 `.wt-modal h3,` | selector-list members | remove those selectors only; **keep** `.wt-op-btn` (:1223) and `.wt-save-modal h3` (:1264) — both serve the preset-save modal. Watch trailing-comma syntax when a group's last member goes (`.tab { line-height: 1.1667; }` at :1256-1257). |
| CSS zh padding | 1339-1353 | `html[lang="zh-Hans"] .wt-osc-btn` + its comment | delete |
| Tab bar | 1429 | `<div class="tab" data-tab="wavetable" data-i18n="tab.wavetable" …>` | delete |
| Osc A drop overlay | 1445 | `<div class="wt-drop-overlay" data-i18n="label.dropWav">` | delete (keep 1443-1444 `#canvasWrapA` + `#canvasOscA`) |
| Osc A Shape dropdown | 1448-1474 | `.dropdown-group` with `label.shape` + `<select id="select-oscATable">` (28 options incl. `Prism Spectrum` :1466) | delete the whole group |
| Osc B drop overlay | 1499 | same as 1445 | delete |
| Osc B Shape dropdown | 1502-1528 | `<select id="select-oscBTable">` (`Prism Spectrum` :1520) | delete the whole group |
| Wavetable editor tab | 1989-2039 | `#wavetable-tab`: `.wt-osc-toggle`, frame strip, harmonic editor, ops bar (keys `label.oscAShort/oscBShort/harmonics/waveform/normalize/normalizeGlobal/fadeEdges/reverse/reverseOrder/smooth/saveShort`, `aria.undo/redo`) | delete |
| Save-wavetable modal | 2041-2051 | `#wt-save-modal-overlay` (`label.saveWavetable`, `aria.wavetableName`, `label.cancel`, `label.save`) | delete (the *preset* save modal at 2053-2063 reuses the same classes and **stays**) |
| Native-fn bridge | 2090-2091 | `window.JuceGetNativeFunction = Juce.getNativeFunction;` | delete |
| `switchTab` | 2514-2515, 2521-2525 | `wasWavetable` + `window.WavetableEditor.onTab(De)Activated()` | delete those lines; keep the 4-line class toggle |
| Bind comments | 3177, 3190 | `// Osc A (9 knobs + 1 dropdown — wavetable selector handles dropdown binding)` | cosmetic fix |
| `WavetableDisplay` | 3611-3682 | canvas painter using **kept** `getActiveOscFrame` | **keep** (Stage 3 ≋ view builds on it) |
| `WavetableSelector` + manager modal + instantiation | 3684-3936 | `Juce.getSliderState('oscATable'/'oscBTable')` :3695, the 6 user-WT native fns :3697-3702, 3887-3888, `getActiveOscInfo` :3702, `window._wtSelectors` :3936, `setLabel(…,'label.importWav'/'label.manage'/'label.delete')`, `label.noUserWavetables` template :3898 | delete; replace with the glue in §7 code (posState listener → `fetchAndDraw`) so the canvas keeps painting the placeholder |
| Manager modal markup | 4722-4729 | `#wt-modal-backdrop` (`label.userWavetables`, `label.close`) | delete |
| link + script | 4736-4737 | see above | delete |

**CSS that must MIGRATE, not die** `[VERIFIED: css/wavetable-editor.css:189-201, 225-269; index.html:2053-2063]`: `.wt-op-btn` (+`:hover`/`:active`), `.wt-save-modal-overlay` (+`.active`), `.wt-save-modal`, `.wt-save-modal h3`, `.wt-save-modal input` (+`:focus`), `.wt-save-modal-buttons`. Copy these ~45 lines verbatim into `index.html`'s `<style>` (beside the modal pins at :1263-1264) before deleting the file. **[recommendation]** keep the class names rather than renaming — `i18n-states.json` and the geometry gates key on ids, and a byte-identical rule set keeps `check-ui-labels` [7] geometry unchanged.

**Surviving markup that depends on kept native functions:** `WavetableDisplay` (:3611-3682) calls only `getActiveOscFrame(osc, pos)`; nothing on the stripped page calls `getActiveOscInfo` (its only caller was `WavetableSelector.init` :3713). Both stay registered in C++ (D3) — but `getActiveOscInfo` **must be rewired** because its body reads `"oscATable"`/`getNumFactoryTables()` (`PluginEditor.cpp:632-636`), see §4.

**`getSliderState("delayDivision")`** `[VERIFIED: index.html:1928, 3261]`: `<select id="select-delayDivision">` and `bindDropdown(document.getElementById('select-delayDivision'), 'delayDivision', 18);` exist; `"delayDivision"` is absent from `allSliderIds()` (`PrismParamIds.h:122-123` lists `delayTime, delayFeedback, delayMode, delayMix`) — confirms D4.

**i18n.js structure** `[VERIFIED: i18n.js:251, 308, 1940-2110, 2116-2350, 2399-2543, 2551]`: `export const LANGUAGES = ['en', 'fr', 'zh-Hans']` (:251); `export const I18N = Object.freeze({…})` (:308, 108 tip entries, each `{ en:{t,b}, fr:{t,b,reviewed:true}, 'zh-Hans':{t,b,reviewed:'bt'} }`); `TIP_BINDINGS` rows `[selector, key, wrapper]` (:1965 etc.); `LABELS` (159 keys, one line each, all three languages on the same line); `I18N_EXEMPT` (:2399-2543); `export function tr()` (:2551). Keys are removed by deleting whole entries/lines — one deletion removes all three languages together, which satisfies D3's "all three or none".

**Exact i18n deletion list** (node census over LABELS/I18N against the strip ranges above, `[VERIFIED: scratchpad/census.mjs run]`):

*LABELS (23 keys → 159 becomes 136):* `tab.wavetable` (:2144), `label.dropWav`, `label.oscAShort` (:2312), `label.oscBShort` (:2313), `label.harmonics` (:2322), `label.waveform` (:2323), `label.normalize` (:2324), `label.normalizeGlobal` (:2325), `label.fadeEdges` (:2326), `label.reverse` (:2327), `label.reverseOrder` (:2328), `label.smooth` (:2329), `label.saveShort` (:2333), `label.saveWavetable` (:2335), `label.userWavetables` (:2337), `label.close` (:2338), `label.delete` (:2339), `label.importWav` (:2340), `label.manage` (:2341), `label.noUserWavetables` (:2342-2343), `aria.wavetableName` (:2344), `aria.undo` (:2346), `aria.redo` (:2347-2348). Also drop the block comment :2309-2321 ("── Wavetable tab ──" + the ops-bar pin rationale) and :2330-2331 ("TWO keys for one English word").

*I18N (2 entries → 108 becomes 106):* `tip.oscATable` (:314-329), `tip.oscBTable` (:492-505).

*TIP_BINDINGS (2 rows → 108 becomes 106):* `['#select-oscATable', 'tip.oscATable', '.dropdown-group']` (:1965), `['#select-oscBTable', 'tip.oscBTable', '.dropdown-group']` (:1979).

*I18N_EXEMPT:* delete the 28-name wavetable catalogue block (:2481-2504, contains `'Prism Spectrum'`); change `['O-PRISM', 'the product name — …']` (:2418) to `'O-STRATA'` in step with `<h1>O-PRISM</h1>` (index.html:1362) — check-i18n [10] needs the exempt text byte-identical to the node text.

*Keys that contain "table"/"wavetable" but STAY (legitimate):* `label.shape` (5 other uses: sub/noise/filter dropdowns :1554-1742), `label.save` / `label.cancel` / `label.savePreset` / `aria.presetName` (preset modal :2056-2060), `label.subtitle` "Microtonal Wavetable Synthesizer" (:2116-2117; re-authoring is a Stage 3 copy decision, no new keys in Stage 1), prose bodies of surviving tips that mention tables (`tip.oscAPos` :332-338, `tip.oscALevel` :348-352, `tip.oscBPos` :508-512, `tip.eqHighGain` :1503-1506), the tuning-generator `label.genHarmonic/genStartHarm/genEndHarm` (:2285-2291 — "Harmonic" ≠ wavetable), and header comments (:82-87, :261, :301). The CONTEXT's "51 matches" resolve to: 4 removable entries/rows, 23 removable label keys, 1 removable exempt block, and the rest prose/comments.

*Consequences for the gates:* check-i18n [15] fails on any LABELS key with no surviving `data-i18n`/`setLabel` reference — the 23 above are exactly the keys the census found dead after the strip (`label.dropWav` is referenced at both :1445 and :1499, remove both). check-i18n [12] will report "1 module: the inline `<script type="module">`" instead of 2. `boot-all-uis` reports a DEAD binding for any TIP_BINDINGS selector absent from the page — the two `#select-osc?Table` rows are the only ones (the `#knob-*` ids are generated at runtime by `expandKnobMarkup`, index.html:2893, and remain valid).

*Canon region and the rename* `[VERIFIED: index.html:2107-2110, 2184-2206; scripts/i18n-canon.js:259-262]`: assertion 6 compares from `let uiLanguage = 'en';` (:2110) to the close of `function initI18n()` (:2206). `grep -n -i prism` over :2110-2206 → no hit (the nearest are `oprism.tipsEnabled` at :2262/:2268 and `__prismTuningCleanup` at :4469-4473, both outside). The sed cannot alter the canon region.

### 2.2 Open item 2 — `FactoryPresets.cpp`

`[VERIFIED: FactoryPresets.cpp:36-135, grep -c]` The file (2381 lines; header claims "96 presets across 9 categories") builds raw `RawMap` overrides on top of `completeBase()` and normalises through `apvts.getParameter(id)`:

```cpp
// FactoryPresets.cpp:53-64
static std::map<juce::String, float>
normalize (juce::AudioProcessorValueTreeState& apvts, const RawMap& raw)
{
    std::map<juce::String, float> out;
    for (const auto& [id, value] : raw)
    {
        if (auto* p = apvts.getParameter (id))
            out[id] = p->convertTo0to1 (value);
    }
    return out;
}
```
So an unknown ID is dropped silently at build time. The preset manager is equally lenient at load `[VERIFIED: OuariconPresetManager.h:262-269]`: `if (auto* param = parameters.getParameter(id)) param->setValueNotifyingHost(...)`. Compile would still fail on the `WT_*` enum only if it were used elsewhere — it is not.

**References:** `oscATable` ×202, `oscBTable` ×183 (= 385), of which `completeBase()` :123 `m["oscATable"] = WT_Saw;` and :128 `m["oscBTable"] = WT_Saw;`; the `WT_*` enum + its comment at :104-114 (`WT_Saw=0 … WT_FilteredNoise=27`, `WT_PrismSpectrum=21` — a `Prism` token the grep gate would catch).

**Strip procedure (dry-run passed on a scratch copy: 0 `osc?Table`, 0 `WT_`, `{`/`}` 2807/2807, 249 whitespace-only lines produced inside braced initialisers — legal C++):**
```bash
sed -i '' -E 's/\{ "osc[AB]Table", WT_[A-Za-z]+ \},? ?//g; s/m\["osc[AB]Table"\] = WT_Saw; //g' Source/FactoryPresets.cpp
sed -i '' '104,114d' Source/FactoryPresets.cpp     # the "Wavetable indices" comment + WT_ enum — re-locate by token first
grep -c 'osc[AB]Table\|WT_' Source/FactoryPresets.cpp   # must print 0
```
Optionally collapse the blank lines it leaves (`cat -s` on the affected ranges) — cosmetic.

**Audible-identity note for Stage 4** `[VERIFIED: WT_ histogram]`: `WT_Sine` occupies 36 of the 385 table slots; the default is `WT_Saw`; the rest spread across 26 other tables (`WT_FMBell` 33, `WT_Triangle` 26, `WT_FMMetallic` 22, …, `WT_PrismSpectrum` 14). Every one of the 96 presets therefore changes character on the sine placeholder — CONTEXT already commits Stage 4 to re-authoring all of them; no list of "affected" presets is needed because the set is "all". Six preset comments mention lowercase "prism" (:412, :859, :1224, :1508, :1902, :2142, e.g. `// 19. Cathedral Furnace — deep prism roar`); the blanket sed turns them into "strata …" — harmless prose that Stage 4 rewrites.

### 2.3 Open item 3 — retire/reaper/assign region and the minimal `oscTablePtr` edit

**Where things live** `[VERIFIED]`:
- `PluginProcessor.h:237-252` — `factoryTables`, `tableInfoList`, `userWavetableManager`, `wavetableEditor`, `editingOscIndex`, `userTableNameA/B`, `userTablePtrA/B` (`std::atomic<const WavetableData*>`), `lastAssignedTableA/B`.
- `PluginProcessor.h:309-310` — `pOscATable`, `pOscBTable`; `:359` `updateWavetableAssignments()`; `:361-376` reaper (`blockGeneration`, `RetiredTable`, `retiredTables`, `retireTable`, `timerCallback`); `:378-380` `resolveActiveTable`.
- `PluginProcessor.cpp:737-738` — `processBlock` calls `updateWavetableAssignments();` every block; `:921-922` `blockGeneration.fetch_add (1, …)` at block end.
- `:945-1000` reaper + assignment (quoted below); `:1002-1067` user-wavetable API; `:1069-1176` editor API.

```cpp
// PluginProcessor.cpp:949-1000 (kept verbatim except the two resolve lines)
void OPrismAudioProcessor::retireTable (std::unique_ptr<WavetableData> table)
{
    if (table != nullptr)
        retiredTables.push_back ({ std::move (table),
                                   blockGeneration.load (std::memory_order_acquire) });
}

void OPrismAudioProcessor::timerCallback()
{
    // IN-04: follow distortion bypass with the reported latency …
    const int wantedLatency = pDistBypass->load() > 0.5f
        ? 0 : static_cast<int> (distortion.getLatencyInSamples());
    if (wantedLatency != getLatencySamples())
        setLatencySamples (wantedLatency);

    if (retiredTables.empty())
        return;
    const auto gen = blockGeneration.load (std::memory_order_acquire);
    retiredTables.erase (
        std::remove_if (retiredTables.begin(), retiredTables.end(),
                        [gen] (const RetiredTable& r) { return gen >= r.retiredAt + 2; }),
        retiredTables.end());
}

void OPrismAudioProcessor::updateWavetableAssignments()
{
    const WavetableData* targetA = resolveActiveTable (0);      // ← becomes oscTablePtr[0].load (std::memory_order_acquire)
    const WavetableData* targetB = resolveActiveTable (1);      // ← becomes oscTablePtr[1].load (std::memory_order_acquire)

    if (targetA != lastAssignedTableA || targetB != lastAssignedTableB)
    {
        for (int i = 0; i < synthesiser.getNumVoices(); ++i)
            if (auto* voice = dynamic_cast<PrismVoice*> (synthesiser.getVoice (i)))
            {
                if (targetA != lastAssignedTableA) voice->setWavetableA (targetA);
                if (targetB != lastAssignedTableB) voice->setWavetableB (targetB);
            }
        lastAssignedTableA = targetA;
        lastAssignedTableB = targetB;
    }
}
```
```cpp
// PluginProcessor.cpp:1043-1055 — DELETED; this is the library lookup
const WavetableData* OPrismAudioProcessor::resolveActiveTable (int oscIndex) const
{
    const auto& userPtr = (oscIndex == 0) ? userTablePtrA : userTablePtrB;
    if (auto* userTable = userPtr.load (std::memory_order_relaxed))
        return userTable;
    auto* pTable = (oscIndex == 0) ? pOscATable : pOscBTable;
    int idx = juce::jlimit (0, static_cast<int> (factoryTables.size()) - 1,
        static_cast<int> (pTable->load()));
    return factoryTables[static_cast<size_t> (idx)].get();
}
```

**Minimal Stage 1 edit [recommendation]:**

*Header:* replace `:237-252` with
```cpp
// Placeholder table (Stage 1): both oscillators read this until Phase 2.1's
// GeometryBakeScheduler publishes baked tables into oscTablePtr[].
std::unique_ptr<WavetableData> placeholderTable;                       // owned for the processor lifetime
std::atomic<const WavetableData*> oscTablePtr[2] { nullptr, nullptr }; // published table per oscillator (message thread writes, audio thread reads)
const WavetableData* lastAssignedTable[2] { nullptr, nullptr };        // audio thread only
int lastTuningPreset = -1;   // keep (was :240)
int lastTonic = -1;          // keep (was :241)
```
delete `:41,47,48` includes, `:118-139` factory getters (`getFactoryTable`, `getNumFactoryTables`, `getTableName`, `getTableCategory`), `:141-180` user-WT + editor API **except** keep `const WavetableData* getActiveOscTable (int oscIndex) const;` (:152), delete `:309-310`, `:378-380`. Keep `:359` and `:361-376` unchanged. Keep `#include "dsp/WavetableData.h"` and `"dsp/WavetableGenerator.h"` (:39-40).

*Constructor* (`:521-528` and `:538-539`, `:544-545`):
```cpp
placeholderTable = WavetableGenerator::generateProceduralTable (WaveShape::Sine);   // 1 frame, 10 mipmap levels, guard samples set (WavetableGenerator.cpp:107-127)
oscTablePtr[0].store (placeholderTable.get(), std::memory_order_release);
oscTablePtr[1].store (placeholderTable.get(), std::memory_order_release);
…
voice->setWavetableA (placeholderTable.get());
voice->setWavetableB (placeholderTable.get());
…
lastAssignedTable[0] = lastAssignedTable[1] = placeholderTable.get();
```
delete `:594-595`. `generateProceduralTable` allocates one frame, fills level 0, runs `generateMipmaps` (FFT band-limiting per level) and `setGuardSamples()` `[VERIFIED: WavetableGenerator.cpp:107-127, 129-193]` — the same object the voices already consume, so the voice/oscillator path is untouched. `WavetableOscillator` is null-safe (`WavetableOscillator.cpp:144, 216`) but never sees a null here.

*Assignment:* `:982-983` read `oscTablePtr[n]`; `getActiveOscTable` (`:1038-1041`) returns `oscTablePtr[osc].load (std::memory_order_acquire)`; delete `:1002-1037`, `:1043-1067`, `:1069-1176`. The reaper is then unused until 2.1 but compiles and runs (empty vector) — leave it, as CONTEXT D2 requires.

### 2.4 Open item 4 — `tests/` fixtures

**`tests/i18n-states.json`** `[VERIFIED: 25 lines, 23 states]` encodes three wavetable-only states and two that *clean up* wavetable overlays before opening something else:
```json
{ "name": "wavetable",         "click": ".tab[data-tab=\"wavetable\"]" },
{ "name": "wt-save-modal",     "click": "#wt-op-save" },
{ "name": "wt-manager-modal",  "eval": "document.getElementById('wt-save-modal-overlay').classList.remove('active'); const sel=window._wtSelectors[0].selectEl; …" },
{ "name": "settings-popover",  "eval": "document.getElementById('wt-modal-backdrop').classList.remove('visible'); document.getElementById('wt-save-modal-overlay').classList.remove('active'); document.getElementById('settings-popover').classList.add('visible');" },
{ "name": "preset-save-modal", "eval": "document.getElementById('settings-popover').classList.remove('visible'); document.getElementById('wt-modal-backdrop').classList.remove('visible'); document.getElementById('btn-preset-save').click();" },
```
Delete the first three; rewrite the last two so they no longer dereference the removed ids (a null `getElementById(...).classList` throws inside the state eval and the walker records the state as unreachable):
```json
{ "name": "settings-popover",  "eval": "document.getElementById('settings-popover').classList.add('visible');" },
{ "name": "preset-save-modal", "eval": "document.getElementById('settings-popover').classList.remove('visible'); document.getElementById('btn-preset-save').click();" },
```
23 → 20 states. `check-ui-labels` (`scripts/check-ui-labels.js:558-569`) and `measure-ui` read this file; both print the state count.

**`tests/ui-stub/generic-overrides.json`** `[VERIFIED: 14 lines]`: `natives` carries `getPresetListWithCategories` (keep — O-Prism returns a JSON *string*, the generic stub's object would break the page), `getEmbeddedTuningList` (keep), `getUserWavetableList`, `startWavetableEditor`, `getAllEditorFrameWaveforms`, `getFrameHarmonics` (delete all four). Rewrite `_why` to drop its wavetable paragraphs and the "Prism's real shape" wording (the sed turns it into "Strata's real shape", which is fine). The generic stub returns `null` for any unmodelled `get*` native (`scripts/ui-stub/generic-juce-stub.js:393-406`), so `getActiveOscFrame` → `null` → `WavetableDisplay.fetchAndDraw` takes its `if (result)` branch and paints nothing — no console error.

**`tests/ui_tip_render_check.js`** `[VERIFIED: 1017 lines]`: `const PLUGIN = 'O-Prism';` (:111) and `os.tmpdir(), 'oprism-i18n-'` (:137) — both handled by the rename sed; the five-tab loop `for (const tab of ['synth', 'mod', 'tuning', 'effects', 'wavetable'])` (:985) → drop `'wavetable'`; `check(TIP_BINDINGS.length === 108, '[8] 105 parameter tips + 3 chrome tips = 108 bindings …')` (:999-1000) → 106 / "103 parameter tips"; comments :57 ("#wavetable-tab"), :453 ("23 `#select-*` rows walk to `.dropdown-group`" → 21), :992-996 ("105 of O-Prism's 173 parameters" → "103 of O-Strata's 219"; the 48 geometry params have no control in Stage 1 and join the "no control" list). `openTab()` (:338-346) drives tabs by clicking `.tab[data-tab=…]`, so no other tab-name table exists. `TAB_OF(sel)` (:514) derives the tab from the DOM.

### 2.5 Open item 5 — safe `Prism` → `Strata` rename procedure

**Inventory** `[VERIFIED: grep -o | wc -l per file over Source/, CMakeLists.txt, tests/]` — 62 files carry a case variant; per-file counts (`Prism` includes the `OPrism`/`O-Prism` substrings):

| File | `Prism` | `PRISM` | `prism` | of which `OPrism` | of which `O-Prism` |
|---|---|---|---|---|---|
| Source/PluginProcessor.cpp | 37 | 0 | 0 | 29 | 3 |
| Source/PluginEditor.cpp | 28 | 0 | 0 | 20 | 3 |
| Source/PrismVoice.cpp | 21 | 0 | 0 | 1 | 2 |
| Source/FactoryPresets.cpp | 18 | 0 | 6 | 0 | 2 |
| CMakeLists.txt | 15 | 0 | 0 | 0 | 14 |
| Source/PrismVoice.h | 10 | 0 | 0 | 3 | 2 |
| Source/PluginEditor.h | 9 | 0 | 0 | 6 | 2 |
| Source/PluginProcessor.h | 9 | 0 | 0 | 4 | 3 |
| tests/ui_tip_render_check.js | 7 | 0 | 1 | 0 | 7 |
| Source/dsp/WavetableFactory.cpp (removed) | 6 | 0 | 0 | 0 | 2 |
| Source/PrismParamIds.h | 5 | 0 | 0 | 0 | 2 |
| Source/ui/public/js/i18n.js | 5 | 1 | 0 | 0 | 3 |
| Source/ui/public/index.html | 4 | 2 | 5 | 0 | 2 |
| Source/PrismSound.h | 4 | 0 | 0 | 0 | 2 |
| Source/dsp/WavetableFactory.h (removed) | 3 | 0 | 0 | 0 | 2 |
| Source/NoteDivisions.h, FactoryPresets.h, OuariconPresetManager.h, tests/ui-stub/generic-overrides.json | 2 each | 0 | 0 | 0 | 1–2 |
| 24 dsp/*.h/.cpp + wavetable-editor.js/.css (removed) | 1–2 each (AGPL header + title line) | css/js: 1 | 0 | 0 | 1–2 |
| TuningEngine.h/.cpp, TuningExporter.h/.cpp, EmbeddedTunings.h/.cpp, ScaleGenerator.h/.cpp, EnsembleChorus.h/.cpp | 1 each | 0 | 0 | 0 | 1 |

Specific non-class tokens the sweep must cover `[VERIFIED]`: `juce::Identifier ("OPrismParameters")` (PluginProcessor.cpp:503); `presetManager (parameters, "O-Prism")` (:504); `getName() … return "O-Prism"` (PluginProcessor.h:67); `exportTuningHTML … "O-Prism"` (PluginEditor.cpp:487); `.getChildFile ("OPrism_WebView")` (:1002); `<h1>O-PRISM</h1>` (index.html:1362) + `['O-PRISM', …]` (i18n.js:2418) + `— O-PRISM` banner comments (index.html:27, css:21, js:21); `localStorage 'oprism.tipsEnabled'` (index.html:2262, 2268 — becomes `ostrata.tipsEnabled`, a separate key from O-Prism's, which is what D1 wants); `window.__prismTuningCleanup` (:4469-4473); `'oprism-i18n-'` tmpdir (ui_tip_render_check.js:137); `PrismVoice.cpp:131` cited in an i18n.js comment (:301); `<option>Prism Spectrum</option>` (index.html:1466, 1520 — deleted with the selects) and `'Prism Spectrum'` (i18n.js:2497 — deleted with the catalogue block); `WT_PrismSpectrum`/`generatePrismSpectrum` (deleted with FactoryPresets strip / WavetableFactory). **No CSS class, JS id, `data-i18n` key or i18n key contains "prism"** (grep -i over the UI: only the tokens listed here). `js/juce/index.js` and `check_native_interop.js` contain no "prism".

**Is a blanket sed safe?** Yes: `grep -rli prism Source CMakeLists.txt tests` shows every hit is one of the tokens above; there is no English word containing "prism" in the tree (no "prismatic"). The i18n canon region contains none (§2.1). The only thing a blanket sed would *wrongly* rewrite is a fork-origin note — so write those after the sed.

**Ordered procedure (BSD sed; run from repo root after the copy in §9 step 1):**
```bash
cd plugins/O-Strata
# 1. file renames first (mv, not git mv — nothing is tracked yet)
mv Source/PrismVoice.cpp   Source/StrataVoice.cpp
mv Source/PrismVoice.h     Source/StrataVoice.h
mv Source/PrismSound.h     Source/StrataSound.h
mv Source/PrismParamIds.h  Source/StrataParamIds.h
# 2. three-case token sweep over every text file that carries the token
grep -rl -i 'prism' Source CMakeLists.txt tests \
  | xargs sed -i '' -e 's/Prism/Strata/g' -e 's/PRISM/STRATA/g' -e 's/prism/strata/g'
# 3. verify — must print nothing
grep -rn -i 'prism' Source CMakeLists.txt tests
```
`#include` lines that change as a result `[VERIFIED]`: `PluginProcessor.h:37-38` (`"PrismSound.h"`, `"PrismVoice.h"` → `StrataSound.h`, `StrataVoice.h`), `PluginEditor.h:34` (`"PrismParamIds.h"` → `StrataParamIds.h`), `PrismVoice.cpp:30-31` (`"PrismVoice.h"`, `"PrismSound.h"`), CMakeLists `Source/PrismVoice.cpp` (:26). The sed rewrites the include text and the `mv` renames the files in the same pass; nothing else includes those headers (grep confirmed).

**Then add the tolerated survivors** (after step 3, so they are not swept): (a) one fork-origin line in `Source/PluginProcessor.h`'s banner, e.g. `Forked from O-Prism v1.24.0 (2026-09-07); see CHANGELOG.md.`; (b) one line at the top of `index.html`'s and `i18n.js`'s history comments noting that the version history cited below (v1.21.0 … v1.24.0) is O-Prism's — otherwise the sed leaves comments like "v1.24.0: SIMPLIFIED CHINESE … O-Strata" that describe a release O-Strata never had; (c) `CHANGELOG.md` `## v1.0.0 (unreleased)` naming the fork base. The `grep -rln "Prism" plugins/O-Strata/Source/` gate then lists exactly `PluginProcessor.h`, `ui/public/index.html`, `ui/public/js/i18n.js` — record that list in SUMMARY.md. **[recommendation]** keep (b) to two lines; do not attempt to rewrite the ~200 lines of O-Prism history prose in i18n.js.

Lowercase-`prism` survivors after the sweep: none (`strata shimmer`, `strata spectrum bass` in FactoryPresets comments — Stage 4 rewrites the presets; note the gate is `grep "Prism"`, case-sensitive, so these would not have failed it anyway).

---

## 3. Parameter layout facts (B)

### 3.1 Helpers and counts `[VERIFIED: PrismParamIds.h:50-135; PluginProcessor.cpp:73-115, 466-494; PluginEditor.cpp:933-964, 1013-1022]`

```cpp
// PrismParamIds.h:50-58
inline juce::StringArray oscIds (const juce::String& prefix)
{
    juce::StringArray ids;
    for (const auto* s : { "Table", "Pos", "Level", "Pan", "Coarse",
                            "Fine", "Phase", "Unison", "Detune", "Width",
                            "WarpType", "WarpAmt" })
        ids.add (prefix + s);
    return ids;
}
// :97-101  bypassToggleIds() → { "reverbBypass", "delayBypass", "chorusBypass", "distBypass", "eqBypass" }   // 5
// :87-93   modSlotToggleIds() → modSlot0On … modSlot15On                                                    // 16
// :105-135 allSliderIds(): oscA 12 + oscB 12 + 6 + 4 + 6 + 5 + 5 + 1 + 7 + 6 + 4 + 3 + 3 + 4 + 12 + 48 + 3 = 141   (comment says "// 126")
//          delay group at :122-123: { "delayTime", "delayFeedback", "delayMode", "delayMix" }  ← "delayDivision" absent (D4)
```
Constructor relay loop (`PluginEditor.cpp:934-964`): `sliderIds = PrismParamIds::allSliderIds()` → one `WebSliderRelay` per id; toggles: `delaySync` (1) + `lfo{1..4}{Sync,FreeRun}` (8) + `bypassIds` (5) + `modToggleIds` (16) = **30**. Attachments (`:1013-1022`) are only created when `getAPVTS().getParameter (id) != nullptr`, so an id in `allSliderIds()` that is not in the layout silently produces a relay without an attachment — that is why CONTEXT's `jassert (sliderAttachments.size() == sliderRelays.size())` (not present in O-Prism; add it after `:1022`) is the right Stage 1 check.

**Counts:** O-Prism 173 params (`.planning/params.tsv` header `# params 173`; `createParameterLayout()` section comments at `:475-491` are stale — they say 10/10/5/5/5/5/6/8/3 — the real per-section sizes are 12/12/6/4/6/5/5/1/7/7/7/4/4/5/20/64/4). O-Strata: 173 − 2 + 48 = **219**; sliders 141 − 2 + 48 + 1 = **188**; toggles 30; unbound: `stereoWidth` only.

### 3.2 Declaration patterns `[VERIFIED: PluginProcessor.cpp:73-112, 121-138, 147-158]`

```cpp
// Choice — PluginProcessor.cpp:107-109
params.push_back (std::make_unique<juce::AudioParameterChoice> (
    juce::ParameterID { prefix + "WarpType", 1 }, label + " Warp Type",
    juce::StringArray { "Off", "Sync", "Bend", "FM", "Window" }, 0));
// Float, linear — :82-84
params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID { prefix + "Pos", 1 }, label + " Position",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
// Float with skew (not needed for geometry; shown for the helper shape) — :147-149
juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.01f));
// Int (the removed Table param) — :80-81
params.push_back (std::make_unique<juce::AudioParameterInt> (
    juce::ParameterID { prefix + "Table", 1 }, label + " Wavetable", 0, WavetableFactory::kNumFactoryTables - 1, 0));
```
`label` is `"Osc " + last char of prefix` (:77). **Insertion point:** inside `createOscParameters (prefix)` delete `:80-81` and append the 24 pushes after the `WarpAmt` push (`:110-112`) before `return params;` (:114). `createParameterLayout()` (`:475-476`) then emits `oscA` (35) then `oscB` (35) exactly as the spec's order requires — no other edit to the layout function.

### 3.3 The 48 declarations (24 per prefix; `prefix` ∈ {`"oscA"`, `"oscB"`}, `label` ∈ {`"Osc A"`, `"Osc B"`}) `[CITED: parameter-spec.md:46-71]`

| # | Suffix | Type | Range / choices (verbatim from spec) | Default | Declaration (pattern) |
|---|---|---|---|---|---|
| 1 | `GeoSource` | Choice | `Mesh / Volume / Terrain` | 0 (Mesh) | `AudioParameterChoice ({prefix+"GeoSource",1}, label+" Source", {"Mesh","Volume","Terrain"}, 0)` |
| 2 | `GeoFrames` | Choice | `64 / 128 / 256` | 2 (256) | `AudioParameterChoice (…"GeoFrames"…, label+" Frames", {"64","128","256"}, 2)` |
| 3 | `GeoDrive` | Float | 0.0 to 1.0 | 0.0 | `AudioParameterFloat (…"GeoDrive"…, label+" Shape Drive", NormalisableRange<float>(0.0f,1.0f,0.001f), 0.0f)` |
| 4 | `Mesh` | Choice | `Sphere / Torus / Twisted Star / Crescent / Torus Knot / fBm Blob / Imported…` | 1 (Torus) | `AudioParameterChoice (…"Mesh"…, label+" Mesh", {"Sphere","Torus","Twisted Star","Crescent","Torus Knot","fBm Blob", IMPORTED}, 1)` |
| 5 | `MeshTiltX` | Float | −90.0 to 90.0 ° | 0.0 | `AudioParameterFloat (…"MeshTiltX"…, label+" Mesh Tilt X", NormalisableRange<float>(-90.0f,90.0f,0.1f), 0.0f)` |
| 6 | `MeshTiltY` | Float | −90.0 to 90.0 ° | 0.0 | as 5 with `"MeshTiltY"`, `" Mesh Tilt Y"` |
| 7 | `MeshPhi` | Float | 0.0 to 360.0 ° | 0.0 | `AudioParameterFloat (…"MeshPhi"…, label+" Mesh Projection Phi", NormalisableRange<float>(0.0f,360.0f,0.1f), 0.0f)` — see §10 on `φ` |
| 8 | `MeshUnwrap` | Choice | `XY Projection / Centroid Distance` | 0 | `AudioParameterChoice (…"MeshUnwrap"…, label+" Mesh Unwrap Mode", {"XY Projection","Centroid Distance"}, 0)` |
| 9 | `MeshLoop` | Choice | `Largest / Sum` | 0 | `AudioParameterChoice (…"MeshLoop"…, label+" Mesh Loop Policy", {"Largest","Sum"}, 0)` |
| 10 | `VolField` | Choice | `Torus SDF / Box SDF / Gyroid / Sphere SDF / fBm Noise` | 0 | `AudioParameterChoice (…"VolField"…, label+" Vol Field", {…5…}, 0)` |
| 11 | `VolOrbit` | Choice | `Torus Knot (2,3) / Torus Knot (3,5) / Torus Knot (5,7) / Lissajous Knot` | 0 | `AudioParameterChoice (…"VolOrbit"…, label+" Vol Orbit", {…4…}, 0)` |
| 12 | `VolSweepAxis` | Choice | `Orbit Scale / Knot Phase / Z Offset` | 0 | `AudioParameterChoice (…"VolSweepAxis"…, label+" Vol Sweep Axis", {…3…}, 0)` |
| 13 | `VolSweepRange` | Float | 0.0 to 1.0 | 0.6 | `AudioParameterFloat (…"VolSweepRange"…, label+" Vol Sweep Range", NormalisableRange<float>(0.0f,1.0f,0.001f), 0.6f)` |
| 14 | `VolDetail` | Float | 0.0 to 1.0 | 0.3 | `… label+" Vol Field Detail" … 0.3f` |
| 15 | `Terrain` | Choice | `Sine Product / Radial Rings / Imported…` | 0 | `AudioParameterChoice (…"Terrain"…, label+" Terrain", {"Sine Product","Radial Rings", IMPORTED}, 0)` |
| 16 | `TerOrbit` | Choice | `Ellipse / Epitrochoid 3·5·7 / Hypocycloid 3·5·7 / Superellipse` | 0 | `AudioParameterChoice (…"TerOrbit"…, label+" Ter Orbit Shape", {"Ellipse", EPI, HYPO, "Superellipse"}, 0)` |
| 17 | `TerCX` | Float | −1.0 to 1.0 | 0.0 | `AudioParameterFloat (…"TerCX"…, label+" Ter Centre X", NormalisableRange<float>(-1.0f,1.0f,0.01f), 0.0f)` |
| 18 | `TerCY` | Float | −1.0 to 1.0 | 0.0 | as 17 with `"TerCY"`, `" Ter Centre Y"` |
| 19 | `TerAspect` | Float | 0.1 to 1.0 | 1.0 | `… NormalisableRange<float>(0.1f,1.0f,0.001f), 1.0f` |
| 20 | `TerRot` | Float | 0.0 to 360.0 ° | 0.0 | `… label+" Ter Rotation", NormalisableRange<float>(0.0f,360.0f,0.1f), 0.0f` |
| 21 | `TerSweepAxis` | Choice | `Radius / Rotation / Centre X / Centre Y` | 0 | `AudioParameterChoice (…"TerSweepAxis"…, label+" Ter Sweep Axis", {…4…}, 0)` |
| 22 | `TerSweepRange` | Float | 0.0 to 1.0 | 0.8 | `… label+" Ter Sweep Range" … 0.8f` |
| 23 | `TerBlur` | Float | 0.0 to 1.0 | 0.2 | `… label+" Ter Image Blur" … 0.2f` |
| 24 | `TerEdge` | Choice | `Mirror / Window` | 0 | `AudioParameterChoice (…"TerEdge"…, label+" Ter Edge Mode", {"Mirror","Window"}, 0)` |

Where `IMPORTED = juce::String (juce::CharPointer_UTF8 ("Imported\xE2\x80\xA6"))`, `EPI = juce::String (juce::CharPointer_UTF8 ("Epitrochoid 3\xC2\xB7" "5\xC2\xB7" "7"))`, `HYPO` likewise — the three spec strings with non-ASCII code points (U+2026, U+00B7). Build the `StringArray` from `juce::String` objects, not bare literals, for those entries.

**Choice-list sizes:** 3, 3, 7, 2, 2, 5, 4, 3, 3, 4, 4, 2 — all ≥ 2 `[VERIFIED against spec]` (memory `critical_choice_param_needs_two_choices`). Default indices: `GeoFrames` default `256` = index 2 (spec "Norm default 1.000000" = 2/(3−1)); `Mesh` default `Torus` = index 1 (spec 0.166667 = 1/6). Every other default is index 0 / the float value listed.

**Host display names [recommendation]:** the spec's UI captions repeat across families (`Sweep Axis` ×2, `Sweep Range` ×2, `Orbit` vs `Orbit Shape`) and `Projection φ` is non-ASCII; the table above prefixes the family (`Mesh`/`Vol`/`Ter`) so the 48 host lanes read unambiguously and stay ASCII (ARCHITECTURE Decision 2's "family-prefixed names"). The spec locks IDs, ranges, defaults and *choice strings*, not the host `name` argument; record the chosen names in the plan.

**Interval [ASSUMED]:** the spec does not state `NormalisableRange` intervals; the table uses O-Prism's convention (0.001 on unit ranges, 0.01 on ±1, 0.1 on degrees). This decides how the param-dump prints `textAtMin` (`0.000` vs `0.0`) — see §10 item 3.

**Identifier collisions:** the 24 suffixes are string literals inside an initializer list and the `ParameterID` strings are built by concatenation — no C++ identifier is introduced, so `juce::` free-function shadowing (`begin`/`end`) cannot occur; no `p…` raw-value cache is added in Stage 1 (CONTEXT). No existing ID starts with `oscAGeo`/`oscAMesh`/`oscAVol`/`oscATer` (`grep -c` in the layout = 0). `[VERIFIED]`

**`modSlot?Dst` choices** `[VERIFIED: dsp/ModulationMatrix.h:92-101]`: `getModDestNames()` returns the 26 entries `"None", "OscA Pos", … "OscA Warp", "OscB Warp"`; `createModMatrixParameters()` (`PluginProcessor.cpp:411-440`) and the `getModDestNames` native function (`PluginEditor.cpp:676-682`) both consume it — no change, and the i18n `I18N_EXEMPT` "37 modulation matrix names" block (i18n.js:2506-2530) stays valid.

---

## 4. Removal-set trace table

| File | Symbol / region | Lines (O-Prism) | Action |
|---|---|---|---|
| `Source/dsp/WavetableFactory.{h,cpp}` | whole files (102 + 809 lines; defines `TableInfo`, `kNumFactoryTables`, `generatePrismSpectrum`) | — | delete |
| `Source/dsp/UserWavetableManager.{h,cpp}` | whole files (102 + 238) | — | delete |
| `Source/dsp/WavetableImporter.{h,cpp}` | whole files (57 + 215) | — | delete |
| `Source/dsp/WavetableEditor.{h,cpp}` | whole files (103 + 444) | — | delete |
| `Source/ui/public/js/wavetable-editor.js`, `css/wavetable-editor.css` | whole files (660 + 269) | — | delete after migrating css:189-201, 225-269 (§2.1) |
| `Source/dsp/WavetableGenerator.{h,cpp}`, `WavetableData.h`, `WavetableOscillator.{h,cpp}` | kept; include only `WavetableData.h`/`MathConstants.h`/JuceHeader | — | keep (no include of a removed header `[VERIFIED: include graph]`) |
| `Source/PluginProcessor.h` | `#include "dsp/WavetableFactory.h"`, `"dsp/UserWavetableManager.h"`, `"dsp/WavetableEditor.h"` | 41, 47, 48 | delete |
| | `getFactoryTable`, `getNumFactoryTables`, `getTableName`, `getTableCategory` | 118-139 | delete |
| | `getUserWavetableManager`, `selectUserWavetable`, `clearUserWavetableOverride`, `getActiveUserTableName`, `isUserTableActive`, `deleteUserWavetable`, `saveEditedWavetable`, `getWavetableEditor`, `startEditing`, `stopEditing`, `getEditingOscIndex` | 141-180 (keep `getActiveOscTable` :152) | delete |
| | `factoryTables`, `tableInfoList` | 237-239 | delete |
| | `userWavetableManager`, `wavetableEditor`, `editingOscIndex`, `userTableNameA/B`, `userTablePtrA/B`, `lastAssignedTableA/B` | 243-252 | replace with `placeholderTable`, `oscTablePtr[2]`, `lastAssignedTable[2]` (§2.3) |
| | `pOscATable`, `pOscBTable` | 309-310 | delete |
| | `updateWavetableAssignments`, reaper block | 359, 361-376 | keep |
| | `resolveActiveTable` | 378-380 | delete |
| `Source/PluginProcessor.cpp` | `#include "dsp/WavetableOscillator.h"` | 41 | keep |
| | `AudioParameterInt {prefix+"Table"}` | 80-81 | delete; append 24 geometry pushes after :112 |
| | factory library + `userWavetableManager.loadFromDisk()` | 521-528 | replace with placeholder creation |
| | `voice->setWavetableA/B (factoryTables[0]…)`, `lastAssignedTableA/B = factoryTables[0]` | 538-539, 544-545 | placeholder |
| | `pOscATable/pOscBTable = getRawParameterValue(…)` | 594-595 | delete |
| | `updateWavetableAssignments()` body | 980-1000 | two-line edit |
| | user-wavetable API, `resolveActiveTable`, editor API | 1002-1067 (keep 1038-1041 rewired), 1069-1176 | delete |
| | `userWavetables` state child | 1197-1200, 1246-1257 | replace with `geometryImports` stub (§7) |
| | `createEditor()` guard | 1265-1276 | **already present — verify only** |
| `Source/PluginEditor.cpp` | `/js/wavetable-editor.js`, `/css/wavetable-editor.css` resource branches | 118-124 | delete |
| | 6 user-WT native fns: `getUserWavetableList` 501, `importUserWavetable` 510, `importUserWavetableData` 542, `selectUserWavetable` 576, `clearUserWavetableOverride` 590, `deleteUserWavetable` 603 | 498-613 | delete |
| | `getActiveOscInfo` | 616-645 | **keep, rewrite** body: `auto* table = processorRef.getActiveOscTable (oscIndex); complete ("{\"isUser\":false,\"factoryIndex\":0,\"name\":\"Sine\",\"numFrames\":" + juce::String (table ? table->numFrames : 0) + "}");` |
| | `getActiveOscFrame` | 648-665 | keep (uses `getActiveOscTable`) |
| | 8 editor native fns: `startWavetableEditor` 686, `stopWavetableEditor` 705, `getEditorFrameWaveform` 713, `getFrameHarmonics` 731, `setFrameHarmonics` 748, `applyFrameOperation` 778, `saveEditedWavetable` 817, `getAllEditorFrameWaveforms` 832 | 684-841 | delete |
| | `toJsonArray`/`toJsonFloatArray` helpers | 134, 148 | keep — still used at 209, 377-413, 660, 671-679, 855 |
| `Source/FactoryPresets.cpp` | 385 `osc?Table` entries + `WT_*` enum | 104-114, 123, 128, 189-2348 | sed strip (§2.2) |
| `CMakeLists.txt` | `Source/dsp/WavetableFactory.cpp`, `WavetableImporter.cpp`, `UserWavetableManager.cpp`, `WavetableEditor.cpp` | 32, 34, 35, 46 | delete |
| | `Source/ui/public/js/wavetable-editor.js`, `css/wavetable-editor.css` in `juce_add_binary_data` | 90-91 | delete |
| `Source/ui/public/index.html`, `js/i18n.js` | see §2.1 | | strip |
| `tests/*` | see §2.4 | | edit |

**All 45 native-function registrations** `[VERIFIED: grep -n withNativeFunction PluginEditor.cpp = 45]` (line → name; `*` = remove): 183 getUiLanguage · 190 setUiLanguage · 206 getTuningIntervals · 212 getTuningName · 217 setSingleInterval · 232 setTonicNote · 247 getTonicNote · 254 getOctaveStretch · 260 loadScalaFile · 292 loadKBMFile · 314 saveScalaFile · 342 saveKBMFile · 371 generateEDO · 383 generateHarmonicSeries · 395 generateRank2 · 410 getEmbeddedTuningList · 423 loadEmbeddedTuning · 444 applyGeneratedScale · 469 exportTuningHTML · 501 getUserWavetableList* · 510 importUserWavetable* · 542 importUserWavetableData* · 576 selectUserWavetable* · 590 clearUserWavetableOverride* · 603 deleteUserWavetable* · 616 getActiveOscInfo (keep, rewire) · 648 getActiveOscFrame · 668 getModSourceNames · 676 getModDestNames · 686 startWavetableEditor* · 705 stopWavetableEditor* · 713 getEditorFrameWaveform* · 731 getFrameHarmonics* · 748 setFrameHarmonics* · 778 applyFrameOperation* · 817 saveEditedWavetable* · 832 getAllEditorFrameWaveforms* · 845 getPresetListWithCategories · 861 getCurrentPreset · 866 loadPresetFromCategory · 878 loadPresetByName · 888 selectNextPreset · 893 selectPreviousPreset · 898 savePreset · 908 isFactoryPreset. 45 − 14 = **31** ✓.

**User-wavetable ValueTree child** `[VERIFIED: PluginProcessor.cpp:1198-1200, 1247-1257]`: `state.getOrCreateChildWithName ("userWavetables", nullptr)` with properties `oscAUserTable`/`oscBUserTable`.

**Two-way grep gate (must print nothing after the edits):**
```bash
grep -rn "WavetableFactory\|UserWavetableManager\|WavetableImporter\|WavetableEditor\|oscATable\|oscBTable\|pOscATable\|pOscBTable\|resolveActiveTable\|userWavetables\|userTablePtr\|factoryTables\|tableInfoList\|TableInfo\|WT_" plugins/O-Strata/Source/ plugins/O-Strata/CMakeLists.txt plugins/O-Strata/tests/
```
(the extra tokens catch the members the CONTEXT list does not name).

---

## 5. Rename inventory + ordered procedure

See §2.5 for the per-file table and the three-command procedure. Summary of what changes, by category:

| Category | Tokens | Where |
|---|---|---|
| C++ types | `OPrismAudioProcessor`, `OPrismAudioProcessorEditor`, `PrismVoice`, `PrismSound`, namespace `PrismParamIds`, `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (…)` args | PluginProcessor.{h,cpp}, PluginEditor.{h,cpp}, PrismVoice.{h,cpp}, PrismSound.h, PrismParamIds.h |
| Files | `PrismVoice.cpp/.h`, `PrismSound.h`, `PrismParamIds.h` → `Strata*` | `mv` + CMake `:26` + includes |
| String literals | `"OPrismParameters"` (APVTS Identifier), `"O-Prism"` ×3 (getName, preset folder, HTML export title), `"OPrism_WebView"` | PluginProcessor.h:67, .cpp:503-504, PluginEditor.cpp:487, :1002 |
| CMake | `O-Prism` ×14 (target, PRODUCT_NAME, sources, include dirs, link, header, module, binary-data target ×2, compile defs, option text, comment, param-dump) | CMakeLists.txt |
| UI | `<h1>O-PRISM</h1>`, `'O-PRISM'` exempt, banner comments, `oprism.tipsEnabled`, `__prismTuningCleanup`, `Prism Spectrum` (deleted anyway) | index.html, i18n.js |
| Tests | `PLUGIN = 'O-Prism'`, `'oprism-i18n-'`, `_why` prose | ui_tip_render_check.js, generic-overrides.json |
| AGPL headers + title lines | `This file is part of O-Prism` / `O-Prism - Microtonal Wavetable Synthesizer` | every Source file (the title line stays "Microtonal Wavetable Synthesizer" under the new name — acceptable in Stage 1; BRIEF's subtitle is a Stage 3 copy item) |

Preserve/exclude: nothing is excluded from the sweep; the fork-origin note (PluginProcessor.h), the two history notes (index.html, i18n.js) and CHANGELOG.md are written *after* it. `plugins/O-Strata/.planning/**` is not swept (it legitimately names O-Prism), and neither is `PLUGINS.md`.

`ARCHITECTURE.md:283, 564` and `parameter-spec.md:44, 558, 575` say `PrismParamIds` — under D1 the file is `StrataParamIds.h`. The spec is locked; treat its `PrismParamIds` as "the param-ids header" (doc-only, §10 item 5). D5's ARCHITECTURE/ROADMAP count edit is a good moment to also spell `StrataParamIds` there.

---

## 6. Build / CMake (C) and gates (D)

### 6.1 CMake `[VERIFIED: plugins/O-Prism/CMakeLists.txt, 119 lines]`

- `juce_add_plugin(O-Prism …)` :6-18 — hunk 1 applies cleanly (`PLUGIN_CODE OuPr`→`OuSt`, `PRODUCT_NAME`, `VERSION 1.24.0`→`1.0.0`; `OuSt` unused across `plugins/*/CMakeLists.txt` re-confirmed).
- 13 modules :56-75 — `juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_plugin_client, juce_audio_processors, juce_audio_utils, juce_core, juce_data_structures, juce_dsp, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra` (+ the three `juce_recommended_*` flag targets, PUBLIC). Hunk 3 inserts `juce::juce_cryptography` after `juce_core` — the module exists at `/Users/taylorbrook/JUCE/modules/juce_cryptography` `[VERIFIED: ls]`; no other plugin links it yet (grep = 0), so this is the first consumer — a plain module with no extra deps, but expect a longer first configure.
- `target_sources` :21-47 — the four removal lines are :32, :34, :35, :46 (`WavetableFactory.cpp`, `WavetableImporter.cpp`, `UserWavetableManager.cpp`, `WavetableEditor.cpp`); :26 `Source/PrismVoice.cpp` → `StrataVoice.cpp`. Hunk 2's `+` lines are **not** applied (D2).
- `juce_add_binary_data(O-Prism_UIResources SOURCES …)` :84-92 lists exactly `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`, `js/wavetable-editor.js`, `css/wavetable-editor.css` — drop the last two, rename the target (also at :96). One binary-data target only (memory `critical_dual_binary_data_namespace_collision`). Symbols after the strip: `index_html`, `i18n_js`, `index_js`, `check_native_interop_js` — the editor's `getResource` (`PluginEditor.cpp:96-116`) already uses exactly these four.
- `ouaricon_add_module(O-Prism note-expression)` :81 — `OuariconModules.cmake:30-…` globs the module's `cpp/` into the target; no registry write at configure time; `modules/registry.yaml` `used_by` lists are regenerated on demand by `scripts/regen-registry-used-by.sh` (grep of `plugins/*/CMakeLists.txt` for the `ouaricon_add_module(...)` token) — run it once after the fork so O-Strata appears as a note-expression consumer (optional, docs-only).
- Param-dump :108-119 — already `option(OUARICON_BUILD_TESTS … OFF)` + `if(OUARICON_BUILD_TESTS) include(ParamDump.cmake); ouaricon_add_param_dump(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source) endif()`. Hunk 6 = rename only. The target name derives from the **folder**: `O-Strata-param-dump` (`ParamDump.cmake:16-18, 67-68`); it compiles the plugin's own `.cpp` sources with `JUCE_WEB_BROWSER=0` and never the editor TU (`:130-140`) — which is why the `createEditor()` guard is load-bearing.
- Root `CMakeLists.txt:47-55` — `file(GLOB PLUGIN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")` + `add_subdirectory(${PLUGIN_DIR})`: no root edit. The snippet's checklist line "Root CMakeLists.txt: add_subdirectory(plugins/O-Strata)" is wrong for this repo — ignore it (§10 item 6). Note GLOB is evaluated at configure time: a **re-configure** (`cmake -S . -B build`) is required for the new folder to be picked up; `ninja` alone will not see it.
- `scripts/build-and-install.sh:134-163` resolves the target from the first token after `juce_add_plugin(` → `O-Strata`; PRODUCT_NAME is read from the built artefact filename (:291-321). It builds only `_VST3` + `_AU` (:280) — build `O-Strata_Standalone` explicitly (memory `pattern_build_install_skips_standalone_stale_ui`).
- `.github/workflows/*` — no per-plugin matrix; both workflows iterate `for d in plugins/*/` (`ci-tests.yml:104, 197`, `build-and-release.yml:547`) — nothing to add. (The Windows CI compiles the new non-ASCII-escaped choice strings as plain ASCII source because they are `\x` escapes — §3.)

**Exact commands:**
```bash
cd /Users/taylorbrook/Dev/VST-development
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release          # re-configure: GLOB picks up plugins/O-Strata
ninja -C build O-Strata_VST3 O-Strata_AU O-Strata_Standalone     # run in background; > 8 min possible on first build
./scripts/build-and-install.sh O-Strata                          # dual-variant sweep + install (VST3/AU only)
# param-dump
cmake -S . -B build -DOUARICON_BUILD_TESTS=ON && ninja -C build O-Strata-param-dump
"$(find build/plugins/O-Strata -name 'O-Strata-param-dump' -perm +111 | head -1)" > plugins/O-Strata/.planning/params.tsv
grep -c . plugins/O-Strata/.planning/params.tsv    # header lines + 219 rows (O-Prism's file is 177 lines for 173 params → expect 223)
diff <(cut -f1 plugins/O-Prism/.planning/params.tsv) <(cut -f1 plugins/O-Strata/.planning/params.tsv)   # expect −2 +48
# pluginval (path verified; strictness 10 per COMPAT-01)
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --timeout-ms 60000 --validate ~/Library/Audio/Plug-Ins/VST3/O-Strata-dev.vst3
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --timeout-ms 60000 --validate ~/Library/Audio/Plug-Ins/Components/O-Strata-dev.component
# auval — cold rescan: run_in_background, budget 2 min (measured 83 s twice); three unquoted words
auval -a | grep -i strata
auval -v aumu OuSt OuDv
# version proof
/usr/libexec/PlistBuddy -c "Print AudioComponents:0:version" ~/Library/Audio/Plug-Ins/Components/O-Strata-dev.component/Contents/Info.plist   # 65536 = 1.0.0 — here that is the CORRECT answer
```
`[VERIFIED: pluginval path + flags from scripts/verify-suite-battery.sh:41, 171-173; PlistBuddy from memory critical_plugin_version_keyword_ignored_by_juce]`. Note the PlistBuddy check is ambiguous for a plugin whose real version *is* 1.0.0 — the discriminating evidence is that `CMakeLists.txt` uses the `VERSION` keyword (grep) plus pluginval's header line.

### 6.2 Gates `[VERIFIED: script headers/usages]`

| Gate | One-plugin invocation | Discovery / allowlist | Stage 1 expectation |
|---|---|---|---|
| check-i18n | `node scripts/check-i18n.js --plugin O-Strata` | `readdirSync(plugins/)`, requires `Source/ui/public/js/i18n.js` (`:513-526`); none | exit 0; [12] reports 1 module; [15] no dead keys; [6] canon v2; [8] i18n.js embedded+served |
| check-ui-labels | `node scripts/check-ui-labels.js --plugin O-Strata [--verbose]` | `serve-ui.js` `listPlugins()` + CMake `juce_add_binary_data` list + `PluginEditor.cpp` `setSize` + `tests/i18n-states.json` | 0 moved on en/fr/zh arms, 20 states |
| boot-all-uis | `node scripts/boot-all-uis.js --plugin O-Strata --strict-tips` | `S.listPlugins()` | 0 console errors, 0 dead, late = as O-Prism today |
| fr lint | `node scripts/i18n-fr-lint.js --plugin O-Strata --strict` | `readdirSync` | CLEAN exit 0 (O-Prism baseline: 0 findings, 20 covered straight copies) |
| zh lint | `node scripts/i18n-zh-lint.js --plugin O-Strata` | `readdirSync` | GATE PASSED exit 0 (O-Prism baseline: 267 entries, 0 findings; O-Strata after strip: 242) |
| tip render | `node plugins/O-Strata/tests/ui_tip_render_check.js` | hard-coded `PLUGIN` (renamed by sed) | all checks pass with 106 bindings |
| measure-ui (report) | `node scripts/measure-ui.js --plugin O-Strata --mode box --report all` | — | optional census |
| param-dump | see §6.1 | folder-derived target | 219 rows |

Baselines measured this session on O-Prism: check-i18n `ALL CHECKS PASS — 1 localized plugin(s)`; fr-lint `CLEAN — exit 0`; zh-lint `GATE PASSED — exit 0`. Repo-wide fr/zh lint currently **exit 2** with "1 plugin(s) could not be read" = O-Strata (no `i18n.js`); the fork clears that. No `package.json`/test manifest exists (memory `project_no_unit_test_framework_ci_never_runs_tests`); Playwright resolves (`S.resolvePlaywright()` ok).

**PLUGINS.md** `[VERIFIED: :27-28, :51-52]`: header `| Plugin Name | Status | Version | Type | Last Updated |`; current row `| O-Strata | 🚧 Stage 0 | - | Synth (3D-Geometry Microtonal Wavetable) | 2026-09-07 |` → `| O-Strata | 🚧 Stage 1 | 1.0.0 | Synth (3D-Geometry Microtonal Wavetable) | <date> |` (legend :7 "🚧 Stage N"). After editing run the duplicate-row check `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` (CLAUDE.md).

---

## 7. State I/O (E) `[VERIFIED: PluginProcessor.cpp:1182-1259]`

```cpp
void OPrismAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    auto tuningState = state.getOrCreateChildWithName ("tuningEngine", nullptr);   // :1188
    tuningEngine.writeStateTo (tuningState);
    state.setProperty ("uiLanguage",                                                // :1194-1195, STRING "en"/"fr"/"zh-Hans"
                       languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);
    auto userWtState = state.getOrCreateChildWithName ("userWavetables", nullptr);  // :1198-1200 → REPLACE
    userWtState.setProperty ("oscAUserTable", userTableNameA, nullptr);
    userWtState.setProperty ("oscBUserTable", userTableNameB, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}
```
Replacement for :1197-1200:
```cpp
// Stage 1: reserve the child Phase 4.1 fills (ARCHITECTURE "State Persistence"); written empty so the
// round-trip test can assert its presence and a 4.1 reader never sees a missing node.
state.getOrCreateChildWithName ("geometryImports", nullptr);
```
`setStateInformation` (:1206-1259): `replaceState` (:1213), tuning scalar sync (:1216-1226), `uiLanguage` read with the `isVoid()` guard (:1235-1238), `tuningEngine` child (:1242-1244), then :1246-1257 restore of `userWavetables` → replace with
```cpp
// Stage 1: nothing to restore yet — an absent or empty geometryImports child is the pre-4.1 state.
juce::ignoreUnused (state.getChildWithName ("geometryImports"));
```
The test criterion "empty `geometryImports` child is written and read without error" is satisfied by the pair above; `tuningEngine` and `uiLanguage` code is untouched.

The preset manager's `customSave/customLoad` hooks (`OuariconPresetManager.h:65-66, 226-227, 273-274`) are not registered by O-Prism — Stage 4.1 wires `geometryImports` into presets there; nothing in Stage 1. Presets live under `~/Library/<pluginName>/Presets/{Factory,User}` (`:163-176`), so `presetManager (parameters, "O-Strata")` gives a fresh folder; factory JSON is regenerated whenever `JucePlugin_VersionString` changes (`PluginProcessor.cpp:516-519`) — first run writes `1.0.0`.

**Minimal page glue replacing `WavetableSelector` (keeps the canvas painting the placeholder through the kept `getActiveOscFrame`):**
```js
// after the WavetableDisplay instances (index.html:3681-3682)
for (const [display, posId] of [[wtDisplayA, 'oscAPos'], [wtDisplayB, 'oscBPos']]) {
    const posState = Juce.getSliderState(posId);
    const refresh = () => display.fetchAndDraw(posState.getNormalisedValue());
    posState.valueChangedEvent.addListener(refresh);
    refresh();
}
```
`[recommendation]` — `fetchAndDraw` already tolerates a null/absent native (:3634-3641).

---

## 8. Pitfalls checklist (memory → Stage 1 step it guards)

| # | Memory | Stage 1 step | What to do |
|---|---|---|---|
| P1 | `critical_choice_param_needs_two_choices` | §3 params | all 12 Choice lists ≥ 2 (min is 2: `MeshUnwrap`, `MeshLoop`, `TerEdge`); a one-entry list births NaN that strictness-10 catches |
| P2 | `critical_juce_string_char_ctor_is_ascii_only` (MEMORY.md) | §3 params | `Imported…`, `Epitrochoid/Hypocycloid 3·5·7` via `CharPointer_UTF8` + `\x` escapes; host names ASCII (`Phi`, not `φ`); no `/utf-8` MSVC flag in repo `[VERIFIED]` |
| P3 | `critical_paramid_shadows_juce_free_function`, `critical_class_name_shadows_juce_type` | §3, §5 | no new identifiers; `StrataVoice`/`StrataSound` collide with nothing in `juce::` (`grep -rn "class JUCE_API  *Strata" ~/JUCE/modules` = 0 `[ASSUMED — run it]`) |
| P4 | `critical_plugin_version_keyword_ignored_by_juce` | §6.1 hunk 1 | `VERSION 1.0.0`; O-Prism already uses `VERSION` (:12) so the fork inherits the right keyword |
| P5 | `critical_binary_data_strips_hyphens`, `critical_dual_binary_data_namespace_collision` | §6.1 | one target; the four surviving symbols have no hyphen |
| P6 | `pattern_render_harness_breaks_on_webview_editor` | §4 | guard already present (:32-37, :1265-1276); param-dump compiles with `JUCE_WEB_BROWSER=0` and proves it |
| P7 | `critical_dev_release_variant_shadowing` | §6.1 install | fresh plugin — no orphan bundle can exist yet; `build-and-install.sh` Phase 4 sweeps anyway |
| P8 | `pattern_build_install_skips_standalone_stale_ui` | §6.1 | build `O-Strata_Standalone` explicitly before any visual check |
| P9 | `pattern_cold_auval_after_install_rescans_registry` | §6.1 | one cold `auval -a` in the background at batch end; `auval -v aumu OuSt OuDv` unquoted |
| P10 | `pattern_ui_labels_gate_blind_to_width_pinned_data_i18n` | §2.1 strip | the strip removes pins, adds none; the migrated modal CSS is byte-identical → [7] geometry unchanged |
| P11 | `pattern_git_commit_pathspec_takes_only_tracked_files` | commit | `git add plugins/O-Strata` (everything under Source/, tests/, CMakeLists.txt, CHANGELOG.md is untracked), then `git commit -- plugins/O-Strata PLUGINS.md`, then `git show --stat` |
| P12 | `pattern_pathspec_commit_resurrects_rm_cached` | n/a | no `git rm --cached` in this stage |
| P13 | `feedback_module_extraction_regression_check` | §2.1 JS strip | after deleting `WavetableSelector`/`showWavetableManager`, grep the page for `_wtSelectors`, `showWavetableManager`, `WavetableEditor`, `JuceGetNativeFunction`, `wt-` — a single dangling reference kills the whole inline module silently; then boot-all-uis for 0 console errors |
| P14 | `pattern_canon_line_three_authors_reached_around` | §2.1/§5 | never edit inside `let uiLanguage` … `initI18n()`; the rename cannot touch it (verified), the strip must not either |
| P15 | `project_i18n_fr_lint_and_glossary`, `project_i18n_zh_hans_rollout` | §2.1 i18n | deletions only — no new copy, so no `reviewed:false`/`'mt'` rows appear; both lints stay green; 242 zh entries remain `'bt'` |
| P16 | `pattern_deferred_item_line_number_drifts_record_a_token` | all | every line number in this document is O-Prism v1.24.0's; re-locate by the quoted token |
| P17 | `pattern_executor_watchdog_stall_run_long_commands_in_background` | §6.1 | first configure+build of a new plugin target with 14 modules: run in background, one build per call |
| P18 | `pattern_shared_checkout_index_race_between_sessions` | commit | uncommitted wave-4g edits under `plugins/O-simpleAdditive/` are live in this checkout right now `[VERIFIED: git status]` — re-check `git status --short` immediately before each commit |

---

## 9. Recommended task ordering (each step independently verifiable)

1. **Fork copy** — `cp -R plugins/O-Prism/Source plugins/O-Strata/Source; cp plugins/O-Prism/CMakeLists.txt plugins/O-Strata/; mkdir -p plugins/O-Strata/tests/ui-stub; cp` the three test files. *Verify:* `diff -r` against O-Prism shows no difference; `git status --short plugins/O-Prism` empty.
2. **Rename** — §2.5 procedure. *Verify:* `grep -rn -i prism plugins/O-Strata/Source plugins/O-Strata/CMakeLists.txt plugins/O-Strata/tests` prints nothing; `ls Source/Strata*` = 4 files.
3. **Delete removal set** — 8 dsp files + 2 UI files (after migrating the modal CSS into `index.html`). *Verify:* `ls Source/dsp | grep -c Wavetable` = 6 (Data.h, Generator.{h,cpp}, Oscillator.{h,cpp}); §4 grep gate still red (expected until step 4).
4. **C++ processor/editor edits** — §2.3, §4, §7 (`PluginProcessor.h/.cpp`, `PluginEditor.cpp` incl. `getActiveOscInfo` rewrite and the `jassert` in the constructor). *Verify:* §4 grep gate prints nothing.
5. **Parameters** — `createOscParameters` (§3.2-3.3), `StrataParamIds.h` `oscIds()` + `delayDivision` + `// 188` comment. *Verify:* `grep -c 'prefix + "' Source/PluginProcessor.cpp` rises by 24−1; hand-count `oscIds` = 35.
6. **FactoryPresets strip** — §2.2 sed. *Verify:* `grep -c 'osc[AB]Table\|WT_' = 0`.
7. **CMake** — hunks 1, 3, 4 (minus the Stage-3 lines), 6; remove 4 source lines. *Verify:* `grep -c O-Strata CMakeLists.txt` = 15, `grep -c Wavetable` = 3 (Generator, Oscillator, and nothing else… i.e. the two kept .cpp lines + none).
8. **UI strip** — §2.1 table + §7 glue. *Verify:* P13 grep; `grep -c "wt-" index.html` = only the migrated modal classes and the preset modal.
9. **i18n** — delete the 23 + 2 + 2 + exempt block, rename `'O-STRATA'`. *Verify:* `node scripts/check-i18n.js --plugin O-Strata` exit 0; fr/zh lint exit 0.
10. **Tests fixtures** — §2.4. *Verify:* `node -e "JSON.parse(require('fs').readFileSync('plugins/O-Strata/tests/i18n-states.json'))"`; 20 states.
11. **Configure + build** — §6.1 (background). *Verify:* three artefacts exist; warnings diff vs an O-Prism build log = none new.
12. **Param-dump** — 219 rows, diff −2 +48, choice strings/defaults match the spec. *Verify:* `diff` output.
13. **Install + validate** — `build-and-install.sh`, pluginval ×2 at strictness 10, cold `auval -a` (background), `auval -v aumu OuSt OuDv`. *Verify:* exit codes; pluginval header prints `1.0.0`.
14. **UI gates** — check-ui-labels, boot-all-uis `--strict-tips`, tip render check, Standalone smoke (no console errors, no 404s, notes play, Scala file loads, state round-trip incl. empty `geometryImports`).
15. **Docs** — CHANGELOG.md stub, NOTES.md Stage 1 entry, PLUGINS.md row, ROADMAP/ARCHITECTURE count corrections (24 lines listed by `grep -n "217\|\b46\b\|23 ×\|126 → 170\|\b170\b"`), STATUS.md checksums, SUMMARY.md with the tolerated-survivor file list. *Verify:* PLUGINS.md duplicate-row check empty.
16. **Commit** — `git branch --show-current; git status --short; git add plugins/O-Strata; git commit -- plugins/O-Strata PLUGINS.md; git show --stat`. No tag.

Steps 2–10 are file edits that can be verified without a build; 11–14 need the build. Steps 6, 9, 10 are independent of 4–5 and can be parallelised if the executor splits work; 8 depends on 3 (CSS migration).

---

## 10. Contradictions and gaps to flag (not silently resolved)

1. **`createEditor()` guard already exists in O-Prism** (`PluginProcessor.cpp:1265-1276`). CONTEXT/ROADMAP/checklist phrase it as something to *add*. Plan it as "verify present; keep the include inside the guard".
2. **CMake snippet hunk 4 + integration checklist name assets that are Stage 3** (`js/geometry-view.js`, the shell PNG); D2 already overrides this — Stage 1's target lists four files. Also the snippet's closing checklist says "Root CMakeLists.txt: add_subdirectory(plugins/O-Strata)" — wrong for this repo (GLOB at root `:48`); no root edit, but a **re-configure** is required.
3. **Param-dump text vs locked spec:** the spec's Float ranges are written as `0.0 to 1.0` / `-90.0 to 90.0` (YAML formatting), while O-Prism's inherited 0–1 floats dump as `0.000 to 1.000` because of the 0.001 interval. The interval is unspecified in the spec; whichever interval the plan picks, the dump's `textAtMin/textAtMax` strings will not match the spec's decimals for some rows. Recommend: match ranges/defaults numerically, not textually, in the Stage 1 criterion, and record the chosen intervals (§3 table) in the plan. `[ASSUMED interval convention]`
4. **Non-ASCII in locked choice strings** (`Imported…`, `Epitrochoid 3·5·7`, `Hypocycloid 3·5·7`) and the caption `Projection φ`: O-Prism's C++ has zero non-ASCII literals today `[VERIFIED]`, the suite has no MSVC `/utf-8` flag, and `juce::String(const char*)` asserts on non-ASCII. The spec is locked, so carry the strings but construct them with `CharPointer_UTF8` + hex escapes; use an ASCII host *name* (`… Mesh Projection Phi`). If the planner prefers ASCII APVTS strings (`Imported...`), that is a spec bump, not a Stage 1 call.
5. **Host display names collide in the spec** (`Sweep Axis`, `Sweep Range`, `Orbit`/`Orbit Shape` per oscillator). The spec locks IDs/ranges/defaults/choices, not the `name` argument — the plan should choose family-prefixed names (§3.3) and say so, since two lanes named "Osc A Sweep Axis" would be a usability defect that no gate catches.
6. **`PrismParamIds` in locked docs:** parameter-spec.md:44/558/575 and ARCHITECTURE.md:283/564 name `PrismParamIds`; D1 renames it to `StrataParamIds`. Doc-only; propagate in the D5 edit of ARCHITECTURE/ROADMAP, leave the locked spec.
7. **`FactoryPresets.cpp` is not "silently ignored" only at load** — `normalize()` drops unknown IDs at build time too (`:60`), so the compile would have succeeded with the entries left in *if* the `WT_*` enum stayed; CONTEXT's explicit strip is still right (the enum carries `WT_PrismSpectrum`, a `Prism` token, and 385 dead entries). Nothing contradicts; noting the mechanism.
8. **`getActiveOscInfo` is "kept" but cannot survive unchanged** — its body references `"oscATable"` and `getNumFactoryTables()` (`PluginEditor.cpp:632-636`). It must be rewritten in Stage 1 (§4); CONTEXT lists it only under "Kept".
9. **The preset-save modal's CSS lives in the file being deleted** (§2.1) — not mentioned in CONTEXT/ARCHITECTURE/snippet; the migration is mandatory or preset saving loses its overlay/layout.
10. **`tests/i18n-states.json` has two non-wavetable states that reference wavetable ids** (`settings-popover`, `preset-save-modal`) — CONTEXT item 4 anticipated only the wavetable states; both evals must be rewritten (§2.4).
11. **ROADMAP.md:13 complexity arithmetic** (`217/5 = 43.4`) is among D5's 24 stale lines; correcting to 219 leaves the capped score unchanged (2.0). Cosmetic.
12. **i18n.js / index.html history comments become O-Strata history after the sed** ("v1.24.0: SIMPLIFIED CHINESE … O-Strata"). Recommend the two-line provenance note (§2.5) rather than rewriting ~200 comment lines; this adds two files to the tolerated-survivor list — CONTEXT allows "historical comments that name O-Prism as the fork origin".

---

## Environment Availability

| Dependency | Required by | Available | Version / path | Fallback |
|---|---|---|---|---|
| cmake | configure | ✓ | 4.2.1 (`/opt/homebrew/bin/cmake`) | — |
| ninja | build | ✓ | 1.13.2 | — |
| JUCE | modules incl. `juce_cryptography` | ✓ | `/Users/taylorbrook/JUCE` (8.0.14 per memory; `modules/juce_cryptography` present) | — |
| existing `build/` | incremental configure | ✓ | `build/build.ninja` present; `build/plugins/O-Prism/` exists | — |
| node | gates | ✓ | v24.19.0 | — |
| Playwright | check-ui-labels / boot-all-uis | ✓ | `S.resolvePlaywright()` ok | `SKIP` exit 77 is not a pass |
| pluginval | COMPAT-01 | ✓ | `/Applications/pluginval.app/Contents/MacOS/pluginval` (not on PATH; `.claude/system-config.json` `pluginval_path` empty) | — |
| auval | COMPAT-01 | ✓ | `/usr/bin/auval`, macOS 26.6.2 | — |
| pluginval Windows / WebView2 | not in Stage 1 criteria | n/a | CI only | — |

**Missing dependencies with no fallback:** none.

## Validation Architecture

(`.planning/config.json` has no `nyquist_validation` key → section included.)

| Property | Value |
|---|---|
| Framework | none (repo has no unit-test framework; CI runs no tests — memory `project_no_unit_test_framework_ci_never_runs_tests`). Gates are the `scripts/*.js` static/headless checks, pluginval, auval, param-dump |
| Config file | none — see per-gate commands §6.2 |
| Quick run command | `node scripts/check-i18n.js --plugin O-Strata && node scripts/i18n-fr-lint.js --plugin O-Strata --strict && node scripts/i18n-zh-lint.js --plugin O-Strata` (< 30 s, no browser) |
| Full suite command | quick + `node scripts/check-ui-labels.js --plugin O-Strata` + `node scripts/boot-all-uis.js --plugin O-Strata --strict-tips` + `node plugins/O-Strata/tests/ui_tip_render_check.js` + param-dump diff + pluginval ×2 + auval |

| Req ID | Behavior | Test type | Automated command | File exists? |
|---|---|---|---|---|
| COMPAT-01 | VST3/AU validate at strictness 10; AU passes auval | smoke (external tool) | §6.1 pluginval/auval lines | ✅ tools present |
| COMPAT-01 (param contract) | 219 params, exact order/choices | integration | param-dump `diff` (§6.1) | ✅ `ParamDump.cmake` |
| D3 (UI strip green) | i18n/labels/boot/tips gates | static + headless | §6.2 table | ✅ scripts + forked `tests/` |
| D1 (isolation, rename) | grep gates | static | §4 grep + `grep -rln "Prism" plugins/O-Strata/Source/` + `git diff --stat HEAD -- plugins/O-Prism` | ✅ |
| FUNC-09/10 smoke | sine plays, Scala loads, state round-trips | manual (Standalone) | — | manual-only: no offline harness exists for O-Prism (a render harness would need `JUCE_WEB_BROWSER=0`, which the guard already supports — a 2.5 deliverable) |

**Sampling:** per task — the quick command + the two grep gates; per wave — full suite; phase gate — full suite green before `/plugin-verify`.

**Wave 0 gaps:** none to create — the forked `tests/` fixtures plus repo scripts cover every Stage 1 criterion; the fixture *edits* in §2.4 are part of the strip task, not new infrastructure.

## Security Domain

Stage 1 adds no input path (no file import, no network, no new native function). `juce_cryptography` is linked but unused until 4.1. ASVS categories V2/V3/V4/V6: not applicable; V5 input validation: unchanged from O-Prism (Scala/KBM file parsing inherited, out of scope). No new threat surface.

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | `NormalisableRange` intervals 0.001 / 0.01 / 0.1 by range family (O-Prism convention) | §3.3, §10.3 | param-dump text differs from the spec's decimals; numeric contract unaffected |
| A2 | Family-prefixed host names (`Osc A Mesh Tilt X` …) are acceptable under the locked spec (which fixes captions, not host names) | §3.3, §10.5 | if the planner wants the spec captions verbatim, two lanes per oscillator share a name and `Projection φ` needs the UTF-8 path too |
| A3 | `StrataVoice`/`StrataSound` collide with no `juce::` type | §8 P3 | one build cycle; check with `grep -rn "class JUCE_API  *Strata" ~/JUCE/modules` before building |
| A4 | Keeping `WavetableDisplay` + the 6-line glue (§7) is within D3 "strip only" (it removes a class, adds no control) | §2.1, §7 | if rejected, delete `WavetableDisplay` too and leave the canvases blank until Stage 3 — also acceptable |
| A5 | Two-line provenance notes in `index.html`/`i18n.js` are tolerated survivors under D1 | §2.5, §10.12 | if not, the ~200 lines of O-Prism history prose must be rewritten or removed |
| A6 | Blank first-configure of `juce_cryptography` has no extra platform deps (a plain JUCE module) | §6.1 | configure error on first use — no other plugin has linked it |

## Sources

**Primary (HIGH):** every file cited with `[VERIFIED]` above, read this session under `plugins/O-Prism/` (Source/*.h/.cpp, dsp/*, ui/public/index.html, js/i18n.js, js/wavetable-editor.js, css/wavetable-editor.css, tests/*, CMakeLists.txt, .planning/params.tsv), repo scripts (`check-i18n.js`, `i18n-canon.js`, `check-ui-labels.js`, `boot-all-uis.js`, `i18n-fr-lint.js`, `i18n-zh-lint.js`, `serve-ui.js`, `ui-stub/generic-juce-stub.js`, `param-dump/ParamDump.cmake`, `build-and-install.sh`, `verify-suite-battery.sh`, `regen-registry-used-by.sh`), root `CMakeLists.txt`, `.github/workflows/*`, `PLUGINS.md`, `modules/cmake/OuariconModules.cmake`; gate baselines run on O-Prism; scratchpad dry-runs (`census.mjs`, `FP.cpp` sed). **Secondary (MEDIUM):** `plugins/O-Strata/.planning/{CONTEXT.md, parameter-spec.md, ROADMAP.md, research/ARCHITECTURE.md, mockups/v1-*}` `[CITED]`; memory files listed in the task `[CITED]`. **Tertiary:** none — no web or library lookups were needed for a codebase-investigation phase.

## Metadata

**Confidence:** standard stack HIGH (no new libraries; one JUCE module added, present on disk) · architecture HIGH (every edit located by line and token in the fork base) · pitfalls HIGH (memory-backed, each mapped to a step) · open-item answers HIGH except the four `[recommendation]`/`[ASSUMED]` choices in §10.
**Research date:** 2026-09-07 · **Valid until:** the next commit that touches `plugins/O-Prism/` (line numbers) — tokens remain valid indefinitely.
