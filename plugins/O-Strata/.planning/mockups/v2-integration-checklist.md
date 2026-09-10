# Integration Checklist — O-Strata mockup v2 (live wave-terrain oscillator)

**Plugin:** O-Strata (fork of O-Prism; `PLUGIN_CODE OuSt`, VERSION 1.0.0)
**Mockup version:** v2 (`mockups/v2-ui.yaml`, finalized 2026-09-10T15:15:37Z; `fork_of: O-Prism v1.26.0`)
**Generated:** 2026-09-10 by ui-finalization-agent
**Contract:** `.planning/parameter-spec.md` v2 — **205 params, 46 mod destinations, locked**
**Scaffold files (this directory):** `v2-ui.html` · `v2-PluginEditor-TEMPLATE.h` · `v2-PluginEditor-TEMPLATE.cpp` · `v2-CMakeLists-SNIPPET.txt` · `img/shell_conchologiaiconi12reev_0090.png`

Two consumers, in workflow order: **A. Stage 1 second pass** (re-parameterise the verified fork — ROADMAP "Stage 1: Foundation — second pass") and **B. Stage 3** (splice the v2 UI over the fork, re-forked from O-Prism v1.26.0). Requirement IDs are ROADMAP's.

---

## A. Stage 1 second pass — re-parameterise (`/plugin-discuss O-Strata 1-foundation`)

### A.1 `Source/PluginProcessor.cpp` — `createOscParameters(prefix)` (region ~73–135)
- [ ] Delete the 24 baked-geometry parameters per oscillator (`GeoSource … TerSweepRange`, incl. the old `TerOrbit / TerCX / TerCY / TerAspect / TerRot / TerSweepAxis / TerSweepRange`)
- [ ] Emit the 28 parameters per oscillator in **parameter-spec.md §"APVTS order"** (rows 1–28): 11 carried over, then `Terrain, TerFreq, TerModX, TerModY, TerTrack, TerSat, TerBlur, TerEdge, Orbit, OrbAspect, OrbRot, OrbCX, OrbCY, OrbMod, OrbFeedback, OrbFbDamp, Quality`
- [ ] `osc?Pos`: default **0.5f**, host name `"Osc ? Orbit Size"` (range 0–1 step 0.001 unchanged)
- [ ] `osc?Unison`: `AudioParameterInt (1, 4, 1)` (was 1–8)
- [ ] `osc?TerFreq`: exact-log `NormalisableRange<float>` from lambdas — `from0to1: 0.25 · 32^n`, `to0to1: log(v/0.25)/log(32)`, snap identity — default 1.0f (norm 0.400000). No skew factor.
- [ ] `osc?OrbRot` 0–360 step 0.1, default 0; `osc?OrbCX` −1–1 default 0.13; `osc?OrbCY` −1–1 default 0.21; `osc?OrbAspect` 0.1–1 default 0.7; other floats per the spec table
- [ ] Choice lists **verbatim**: Terrain (7, `Imported…` last, built via `CharPointer_UTF8 ("Imported\xE2\x80\xA6")` as the fork already does), Orbit (11 — `Limaçon` via `"Lima\xC3\xA7on"`), TerEdge (2), Quality (3 — `"2\xC3\x97"`, `"4\xC3\x97"`); every list ≥ 2 entries
- [ ] `jassert (params.size() == 28)` per oscillator; `addSection` comments `// 10` → `// 28`
- [ ] State child `geometryImports` → `terrainImports` (`PluginProcessor.cpp` ~1081, ~1129), still empty until Stage 4

### A.2 `Source/StrataParamIds.h` (lines 47–60 region)
- [ ] `oscIds()` = 11 carried + the 13 Float suffixes (v2-PluginEditor-TEMPLATE.cpp §0 — exact list and order)
- [ ] NEW `oscComboIds()` = `Terrain, TerEdge, Orbit, Quality`; NEW `allComboIds()` = 8
- [ ] `allSliderIds()` count comments: `// 35` → `// 24` ×2, `// 188` → `// 166`
- [ ] Grep every new suffix against `juce::` free functions (`Terrain`, `Orbit`, `Quality`, `TerFreq`, `OrbCX` … — none collide; memory `critical_paramid_shadows_juce_free_function`)

### A.3 `Source/dsp/ModulationMatrix.h` (`ModDest` enum ~52, `getModDestNames()` 92–101)
- [ ] Relabel index 1 / 2: `"OscA Pos"` → **`"OscA Orbit Size"`**, `"OscB Pos"` → **`"OscB Orbit Size"`** (same index — the mockup form, NOT ROADMAP's shorter "OscA Size")
- [ ] Append the 20 entries **A then B**, exact strings from parameter-spec.md §"Mod-matrix destinations" (indices 26–45): `OscA Orbit Aspect, OscA Orbit Rot, OscA Orbit CX, OscA Orbit CY, OscA Orbit Mod, OscA Terrain Freq, OscA Terrain Mod X, OscA Terrain Mod Y, OscA Feedback, OscA Saturation`, then the OscB ten
- [ ] `ModDest` enum grows in the same order; `NumDests = 46`; the voice reads the 20 new offsets and discards them until Phase 2.1
- [ ] `modSlot?Dst` `AudioParameterChoice` picks the list up automatically; `getModDestNames` native fn unchanged

### A.4 `Source/PluginEditor.cpp / .h` (relay plumbing only — the page is still the Stage 1 placeholder)
- [ ] Add the `comboRelays` / `comboAttachments` group in the order shown in `v2-PluginEditor-TEMPLATE.h` (relays → webView → attachments); `.withOptionsFrom()` for all 8; `jassert (sliderAttachments.size() == sliderRelays.size())` now = 166
- [ ] Placeholder `index.html` for Stage 1: the v1.24.0 fork page with the v2 oscillator panels and Terrain tab rendered as plain knobs / selects / segmented control bound by relay (full styling, 3D view and import are Stage 3) — or splice `v2-ui.html` early; either way every new control must move its parameter

### A.5 Presets, inventory, gates
- [ ] `FactoryPresets.cpp`: re-point the existing set at the new IDs (values at defaults; real presets Stage 4.1) so the factory-preset version bump regenerates cleanly
- [ ] `params.tsv` regenerated (`OUARICON_BUILD_TESTS=ON` param-dump) → **205 rows**; diff vs O-Prism v1.24.0 = −2 (`osc?Table`) +34, `osc?Pos` default / `osc?Unison` max / `modSlot?Dst` steps changed in place (FUNC-09)
- [ ] `modSlot0Dst` exposes 46 choices; indices 0–25 match O-Prism's strings except 1 / 2 (FUNC-05)
- [ ] Smoke harness (`stages/1-foundation/smoke/`) asserts 205 / `<terrainImports/>`; 205-param state round-trip into a fresh processor
- [ ] `ninja O-Strata_VST3 O-Strata_AU` clean → `./scripts/build-and-install.sh O-Strata` (dual-variant sweep) → pluginval strictness 10 VST3 + AU, `auval` pass (**COMPAT-01 re-verify**)
- [ ] CHANGELOG `## v1.0.0 (unreleased)`: Stage 1 second-pass entry recording the `osc?Pos` default and `osc?Unison` range change as the **last free range change** (no O-Strata preset shipped; from v1.0.0 every range change needs its own gate — memory `pattern_preset_migration_per_param_version_gate`)
- [ ] `STATUS.md` → Stage 1 second pass complete; `PLUGINS.md` row → 🚧 Stage 1
- [ ] Commit: `feat(O-Strata): Stage 1 second pass — 205 live-oscillator parameters, 46 mod destinations, COMPAT-01 re-verified`

---

## B. Stage 3 (GUI) — splice the v2 UI over the fork

### B.0 Prerequisites
- [ ] Stage 1 second pass done (A above); Stage 2 delivers the processor API in `v2-PluginEditor-TEMPLATE.cpp` §7 (`getTerrainPlayhead`, `getTerrainGeneration`, `copyTerrainHeightmap`, `getTerrainStatus`, `submitTerrainImport`) and Core 10 `CycleCapture`
- [ ] **Re-fork the UI shell from O-Prism v1.26.0** (`plugins/O-Prism/Source/ui/public/index.html` @ 4f12ef57, `js/i18n.js` v1.24.0+): the fork's `Source/ui/public/index.html` is the v1.24.0 shell and lacks the card grid, the 12-column Synth / Effects grids, the 3-column Tuning grid and the `appearance: none` selects that `v2-ui.html` assumes

### B.1 Copy UI files (Phase 3.1)
- [ ] `mockups/v2-ui.html` → `Source/ui/public/index.html`, then **merge** with O-Prism v1.26.0's `index.html`. `v2-ui.html`'s markup + CSS are byte-identical to the mockup; its script marks every region `INHERITED FROM O-PRISM` (take O-Prism's verbatim where it is fuller) or `NEW` (O-Strata's):
  - take from O-Prism verbatim: the i18n canon region (check-i18n assertion 6 byte-compares it), `initializeTipsToggle()` + `setupTooltips()` + the `.tooltip` CSS + the trailing `<div class="tooltip" id="tooltip">`, the **full Tuning tab** (markup + IIFE: library accordion, generators, circle / polar / matrix / TrueKeys / rotation views, TrueKeys bar — the mockup's tuning tab is a simplified stand-in with the same grid and ids), the preset save modal markup + CSS (`#preset-save-modal-overlay`; `v2-ui.html` builds it in JS as a stop-gap)
  - keep from `v2-ui.html`: the two oscillator cards, the fifth tab (`#terrain-tab`), the terrain / orbit maths, both renderers, the proxies, the pushed-event listeners, `bindCombo()`, the 46-entry mod-dest fallback, `unisonFmt` (1–4), the osc?Pos default 0.5 in `bindKnob`
  - drop from O-Prism: the Wavetable tab (`#wavetable-tab`, every `.wt-*` editor element except `.wt-osc-toggle` / `.wt-osc-btn` which move inline), `js/wavetable-editor.js`, `css/wavetable-editor.css`, the `<link>` / `<script src>` for them, `WavetableDisplay`, `getActiveOscInfo` / `getActiveOscFrame` / user-wavetable native calls, the Shape dropdown (`select-osc?Table`), `label.dropWav`
- [ ] `mockups/img/shell_conchologiaiconi12reev_0090.png` → `Source/ui/public/img/` (referenced as `img/shell_conchologiaiconi12reev_0090.png`)
- [ ] `js/juce/index.js` + `check_native_interop.js`: keep the fork's copies (JUCE 8.0.14)
- [ ] **Inherited O-Prism v1.26.0 defect — fix in BOTH plugins:** `.octave-stretch-slider { min-width: 0 }` on the Tuning tab. `input[type=range]` carries a UA intrinsic min-width (~129 px in Chromium) that `flex: 1` will not shrink below, so label + slider + readout summed to 217 px in the 210 px column and pushed the "1.00" readout 7 px past the panel edge (measured by the headless gate on the mockup; `v2-ui.html` already carries the fix at `.octave-stretch-slider`). Port the one-line rule to `plugins/O-Prism/Source/ui/public/index.html` as an O-Prism patch release.
- [ ] Verify on the merged file: `grep -cE "[0-9](vh|vw|dvh|svh)\b" index.html` = 0; `html, body { height: 100% }`; `user-select: none`; `contextmenu` disabled; every `wheel` listener `{ passive: false }`; exactly one `<script type="module">`; `grep -c evaluateJavascript` = 0 in `index.html` AND `PluginEditor.cpp`

### B.2 `js/i18n.js` — 30 new keys + 2 changed strings, en / fr / zh-Hans (Phase 3.1)
Convention: `label.*` visible captions, `tab.*` tab titles, `aria.*` icon-only controls; `label.feedback` is **reused** (delay Feedback) not redefined; readouts, the HUD, `2×` / `4×`, note names, `F = 1`, `87 %`, the ⬡ / ≋ glyphs and the orbit / terrain choice names are **never localised**. Mark every fr / zh-Hans string `reviewed: false` (project i18n gate) — Taylor reads French; zh-Hans awaits back-translation (`scripts/i18n-zh-backtranslate.js`, `reviewed: 'bt'`). Glossary roots used: fr `qualité`, `réinjection` / `réinj.`, `taille`, `rotation`, `amort.`, `saturation`; zh 质量, 反馈, 尺寸, 旋转, 阻尼, 导入, 轨道 (the O-Orbit sense). French colons take U+00A0 before them.

| key | en | fr | zh-Hans |
|-----|----|----|---------|
| `tab.terrain` | Terrain | Terrain | 地形 |
| `label.terrain` | Terrain | Terrain | 地形 |
| `label.orbit` | Orbit | Orbite | 轨道 |
| `label.orbitSize` | Orbit Size | Taille orbite | 轨道尺寸 |
| `label.terrainFreq` | Freq | Fréq. | 频率 |
| `label.terrainModX` | Mod X | Mod X | 调制 X |
| `label.terrainModY` | Mod Y | Mod Y | 调制 Y |
| `label.pitchTrack` | Pitch Track | Suivi hauteur | 音高跟踪 |
| `label.saturation` | Saturation | Saturation | 饱和 |
| `label.imageBlur` | Image Blur | Flou image | 图像模糊 |
| `label.edgeMode` | Edge Mode | Mode bord | 边缘模式 |
| `label.orbitAspect` | Aspect | Aspect | 纵横比 |
| `label.orbitRotation` | Rotation | Rotation | 旋转 |
| `label.orbitCentreX` | Centre X | Centre X | 中心 X |
| `label.orbitCentreY` | Centre Y | Centre Y | 中心 Y |
| `label.orbitMod` | Orbit Mod | Mod orbite | 轨道调制 |
| `label.feedbackDamp` | Fb Damp | Amort. réinj. | 反馈阻尼 |
| `label.quality` | Quality | Qualité | 质量 |
| `label.bandlimited` | Bandlimited | Bande limitée | 带限 |
| `label.import` | Import… | Importer… | 导入… |
| `label.dropPng` | Drop PNG | Déposer PNG | 拖入 PNG |
| `label.webglUnavailable` | WebGL unavailable — 2D fallback | WebGL indisponible — repli 2D | WebGL 不可用——2D 回退 |
| `label.sourceMissing` | Source missing — using library fallback | Source manquante — repli sur la bibliothèque | 源文件缺失——使用库内替代 |
| `aria.view3d` | 3D view | Vue 3D | 3D 视图 |
| `aria.viewWaveform` | Waveform view | Vue forme d'onde | 波形视图 |
| `label.hintView` | Drag: centre · Wheel: size · ⌥ drag: rotation | Glisser : centre · Molette : taille · ⌥ glisser : rotation | 拖动：中心 · 滚轮：尺寸 · ⌥ 拖动：旋转 |
| `label.readoutPartials` | {n} partials at {note} | {n} partiels à {note} | {note} 处 {n} 个分音 |
| `label.approx` | approx. | approx. | 近似 |
| `label.imageProjected` | image projected at F = {f} · fit {pct} % | image projetée à F = {f} · ajustement {pct} % | 图像投影 F = {f} · 拟合 {pct} % |
| `label.subtitle` *(changed)* | Microtonal Wave-Terrain Synthesizer | Synthétiseur microtonal à terrain d'onde | 微分音波地形合成器 |
| `label.modMatrixInfo` *(changed)* | Route any source to any destination. 16 slots available, 46 destinations. | Routez n'importe quelle source vers n'importe quelle destination. 16 emplacements, 46 destinations. | 将任意源路由到任意目标。16 个槽位，46 个目标。 |

- [ ] Retire with the Wavetable tab: `tab.wavetable`, `label.harmonics`, `label.normalize*`, `label.fadeEdges`, `label.reverse*`, `label.smooth`, `label.saveWavetable`, `label.userWavetables`, `label.noUserWavetables`, `label.importWav`, `label.manage`, `label.waveform`, `label.dropWav`, `label.position`
- [ ] `TIP_BINDINGS`: one row per Terrain-tab knob from the `title=""` attributes in the mockup markup (`terFreq`, `terModX`, `terModY`, `terTrack`, `terSat`, `terBlur`, `terSize`, `terAspect`, `terRot`, `terCX`, `terCY`, `terOrbMod`, `terFb`, `terFbDamp`), then **delete the `title` attributes** (the tooltip canon owns hover help)
- [ ] `I18N_EXEMPT`: `#seg-terQuality` faces `2×` / `4×` (numerals), `#terrain-hud`, `#terrain-readout` numerals, the two glyph buttons
- [ ] Mod-destination host strings stay native-only (STATUS open question 5 closed as O-Prism does it)
- [ ] Run `node scripts/i18n-fr-lint.js --plugin O-Strata` (exit 0) and `node scripts/i18n-zh-lint.js --plugin O-Strata` (Z1/Z2/Z4/Z5/Z7/Z8), then `node scripts/check-i18n.js --plugin O-Strata` (assertions 6, 8, 10, 13) and `node scripts/check-ui-labels.js --plugin O-Strata` (all 23 states — the Terrain tab adds the Bandlimited / Imported… / source-missing walk)

### B.3 `Source/PluginEditor.h / .cpp` (Phase 3.1 + 3.2)
- [ ] Apply `v2-PluginEditor-TEMPLATE.h`: `comboRelays` (after `sliderRelays`) and `comboAttachments` (after `sliderAttachments`), the four `push*` helpers, `repushAll()`, the change-gate members
- [ ] **Verify member order:** `grep -n "RELAYS FIRST\|WEBVIEW SECOND\|ATTACHMENTS LAST" Source/PluginEditor.h` prints ascending line numbers; relay groups = attachment groups (7 = 7)
- [ ] Apply `v2-PluginEditor-TEMPLATE.cpp` §2 (+1 PNG route, `image/png`), §3 (−`getActiveOscInfo` −`getActiveOscFrame`, +`chooseTerrainImage` +`importTerrainImageData` +`requestTerrainRepush`), §4 (combo relays through all three constructor steps, `jassert` 166 and 8), §5 (timer → `emitEventIfBrowserIsVisible`, four pushes, all change-gated)
- [ ] Every `launchAsync` completion captures `Component::SafePointer` and returns **without** `complete()` on the null path (`chooseTerrainImage` + the inherited Scala / KBM / export choosers — memory `pattern_webview_launchasync_safepointer_no_complete`)
- [ ] `grep -c evaluateJavascript Source/PluginEditor.cpp` = 0 (the fork's `updateHeldNotes` call is gone; `heldNotes` is pushed)
- [ ] Windows: `withUserDataFolder (… "OStrata_WebView")` kept
- [ ] Preset apply / `setStateInformation` trigger a re-push (bump the generation counters or call `repushAll()`; UI-04)

### B.4 `CMakeLists.txt`
- [ ] Apply `v2-CMakeLists-SNIPPET.txt` hunk 2 (the PNG in `juce_add_binary_data`) — exactly **one** binary-data target; hunk 1 follows Stage 2's source list; hunk 3 comment
- [ ] Offline render harness still configures with `JUCE_WEB_BROWSER=0` (memory `pattern_render_harness_breaks_on_webview_editor`)

### B.5 Phase 3.1 — panels, proxies, inert rules (UI-03)
- [ ] Build Debug + Release: `cd build && ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` — no warnings from `PluginEditor.cpp`
- [ ] Standalone shows the WebView (not blank); Inspect works; console has no errors; `window.__JUCE__` exists; no `i18n: missing label key`; no `Creating ComboBoxState … unknown to the backend` warnings (all 8 combo names registered)
- [ ] All 34 new controls move their parameter and follow host automation (two-way); mirrored `osc?Pos` ⇄ `terSize` move together; the two Synth-tab dropdowns ⇄ the Terrain-tab toolbar dropdowns stay in sync; **Osc A / B toggle repoints, does not copy** (Osc B shows its own Quality and Freq)
- [ ] `<select>` option lists come from C++ (`state.properties.choices`) — change a choice string in `createOscParameters` and the page follows without a JS edit
- [ ] Inert rules: `TerBlur` + `TerEdge` greyed unless Terrain = Imported…; `OrbMod` greyed for Ellipse; `TerTrack` greyed in Bandlimited; Freq readout clamps at 2.00× in Bandlimited; the greyed control **keeps its column** (nothing moves)
- [ ] Readout forms (never localised numerals): `2× · 12 partials at C4` · `Bandlimited · 9 partials at C4 · approx.` (Superellipse / Butterfly / Squarcle only — Limaçon is degree 2 and exact) · `image projected at F = 1 · fit 87 %` (Bandlimited + Imported…). Estimate until `terrainStatus` overrides it: **n = round(12 · F · K · (0.52 + 0.96 · size)), clamped 1…160**, K = orbit degree `[Ellipse 1, Superellipse 1, Limaçon 2, Epitrochoid 3/5/7 → 4/6/8, Hypocycloid 3/5/7 → 2/4/6, Butterfly 4, Squarcle 1]` — the default patch reads 12
- [ ] Alt-click resets to the **normalised** defaults in the template (`terFreq` 0.4, `terAspect` 0.6667, `terCX` 0.565, `terCY` 0.605, `osc?Pos` 0.5); dblclick value entry parses `×`, `°`, `%`
- [ ] Commit: `feat(O-Strata): Phase 3.1 — Synth-tab oscillator cards, Terrain tab panels + proxies, comboBox relays, i18n en/fr/zh-Hans`

### B.6 Phase 3.2 — 3D view, pushes, import (UI-01, UI-02, UI-04, UI-05, FUNC-06/07/08, PERF-03)
- [ ] WebGL2 path renders the big view (R32F `texImage2D`, `texelFetch` in the vertex shader, ribbon lines, no GLSL `flat`); Canvas 2D path renders the two mini canvases and the big view when WebGL2 is unavailable (`#terrain-webgl-unavailable` shows, then hides); `webglcontextlost` → `restored` rebuilds resources and redraws
- [ ] `terrainState` at 30 Hz drives the scan point on all three canvases; idle plugin emits nothing (change gate); hidden editor drops payloads (`emitEventIfBrowserIsVisible`) and the page recovers on show via `requestTerrainRepush`
- [ ] `terrainHeightmap` replaces the JS analytic surface on first push; PNG import shows the real image surface; Bandlimited shows the truncated surface
- [ ] Preset load → `__refreshAllControls` → `requestTerrainRepush` → all three events re-push (UI-04); `setStateInformation` (DAW session load) likewise
- [ ] View interaction: drag → Centre X 0.13 → the point under the cursor; wheel → Orbit Size ±; ⌥ drag → Rotation wraps at 360; one undo step per gesture in the host (drag-start / drag-end pair)
- [ ] `Import…` → native chooser (`*.png`) → `submitTerrainImport` → Terrain = Imported… set by C++; cancel leaves the parameter alone; drop a PNG on the big view or either mini canvas (macOS: `webkitGetAsEntry` path) → same import; > 2 MB drop refused with a console warning; missing source on reload → `.terrain-notice` (`terrainStatus.sourceMissing`)
- [ ] PERF-03: big view ≤ 2 ms mean per frame at DPR 2 in WKWebView and WebView2 (measure with the Performance panel; the wireframe layer is static — only the ribbon / point layer redraws)
- [ ] Release build: no crash on 10 × reload (member order); `auval` + pluginval strictness 10 pass
- [ ] `check-ui-labels`, `check-i18n`, `boot-all-uis` (no late / dead tip bindings), fr lint exit 0, zh lint pass — all green in **all three languages** at the shipped 1200 × 800 frame; every tab `scrollHeight === clientHeight` (Terrain tab 608 ≤ 650 budget)
- [ ] Commit: `feat(O-Strata): Phase 3.2 — WebGL2 terrain view + Canvas 2D fallback, 30 Hz pushed state, PNG import + drop, preset re-push`

### B.7 Phase 3.3 — polish + measured gates
- [ ] Botanical overlay reads only below the Orbit card (opacity 0.3, multiply); `#terrain-tab` `scrollHeight === clientHeight === 666` in every state of the Bandlimited / Imported… / source-missing walk
- [ ] `.osc-params` single row in en / fr / zh-Hans (980 px available, 900 used in en); if a translation wraps, stack the two dropdowns inside one `.dropdown-group` column (YAML `wrap_fallback`)
- [ ] Width-pinned captions (`delayFeedback` 54.25 px, `osc?WarpAmt` 55.16 px) still hold in fr / zh-Hans; `oscAPos` is no longer pinned ("Orbit Size" sets its own column)
- [ ] Hover help present on every Terrain-tab knob (tips gate); `boot-all-uis --strict-tips`
- [ ] Commit + `STATUS.md` (Stage 3 complete) + `PLUGINS.md` row

---

## Parameter list (from parameter-spec.md v2 — 34 new + 2 changed; relay kinds)

| ID (×A/B) | Type | Range | Default | Relay | Tab(s) |
|-----------|------|-------|---------|-------|--------|
| `osc?Terrain` | Choice | 7 (Imported… last) | Sine Product | **ComboBox** | synth, terrain |
| `osc?TerFreq` | Float | 0.25–8.0 × exact log | 1.0 (norm 0.4) | Slider | terrain |
| `osc?TerModX` / `TerModY` | Float | 0–1 | 0.5 | Slider | terrain |
| `osc?TerTrack` | Float | 0–1 | 1.0 | Slider | terrain |
| `osc?TerSat` | Float | 0–1 | 0.0 | Slider | terrain |
| `osc?TerBlur` | Float | 0–1 | 0.2 | Slider | terrain |
| `osc?TerEdge` | Choice | Mirror / Window | Mirror | **ComboBox** | terrain |
| `osc?Orbit` | Choice | 11 | Ellipse | **ComboBox** | synth, terrain |
| `osc?Pos` *(changed)* | Float | 0–1 | **0.5** | Slider | synth, terrain |
| `osc?OrbAspect` | Float | 0.1–1 | 0.7 | Slider | terrain |
| `osc?OrbRot` | Float | 0–360 ° | 0 | Slider | terrain |
| `osc?OrbCX` / `OrbCY` | Float | −1–1 | 0.13 / 0.21 | Slider | terrain |
| `osc?OrbMod` | Float | 0–1 | 0.5 | Slider | terrain |
| `osc?OrbFeedback` | Float | 0–1 | 0.0 | Slider | terrain |
| `osc?OrbFbDamp` | Float | 0–1 | 0.5 | Slider | terrain |
| `osc?Quality` | Choice | Bandlimited / 2× / 4× | 2× | **ComboBox** | terrain |
| `osc?Unison` *(changed)* | Int | 1–**4** | 1 | Slider | synth |

Relays: 166 slider + 8 comboBox + 30 toggle = **204** = attachments (205 params; `stereoWidth` unbound as in O-Prism).
