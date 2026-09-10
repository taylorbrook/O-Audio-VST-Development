# Stage 1 — Foundation: SUMMARY

**Plugin:** O-Strata · **Stage:** 1 of 4 (Foundation + Shell, single phase) · **Phase:** execute ✓
**Date:** 2026-09-07 · **Mode:** manual · **Branch:** `main` (path-scoped commit)
**Plan:** `stages/1-foundation/PLAN.md` — 16 tasks in 5 waves, all executed. Tasks 8–10 and the CSS migration ran in a forked subagent in parallel with the C++/CMake work; everything else ran in the orchestrator (the plan was shell-heavy — copy, sed, delete, build — beyond the `foundation-shell-agent`'s Read/Write/Edit tool set).
**Gate:** `0-ideation → 1-foundation` BYPASSED with `--force` (the build check has no source to build before Stage 1; logged in `.planning/gate-bypasses.log`).

---

## Result

O-Strata builds, installs and validates as a complete, independent copy of O-Prism v1.24.0 (commit `e88ec412`) with the wavetable library removed and 48 inert geometry parameters added. Both oscillators play `WavetableGenerator::generateProceduralTable (Sine)` through the unchanged voice/FX path. `plugins/O-Prism/` is untouched.

**COMPAT-01 verified.** FUNC-09/FUNC-10 smoke: partially — see "Manual smoke left for verify".

| Check | Result |
|---|---|
| `cmake -S . -B build` (root GLOB picks up `plugins/O-Strata`) | exit 0; `juce_cryptography` configured (first consumer in the repo) |
| `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` | exit 0; 28 warnings, all in inherited files not edited in Stage 1 (`SVFFilter.cpp`, `EQProcessor.cpp`, `WavetableGenerator.cpp`, `WavetableOscillator.cpp`, `TuningEngine.*`, `PluginProcessor.cpp:761` masterTune compare) — same set O-Prism emits |
| `./scripts/build-and-install.sh O-Strata` | exit 0; 7 phases green; installed `O-Strata-dev.vst3` + `O-Strata-dev.component`; dual-variant sweep ran (nothing to sweep — fresh plugin) |
| pluginval strictness 10, VST3 (installed bundle) | **SUCCESS**, 0 failed tests, `Plugin name: O-Strata-dev` |
| pluginval strictness 10, AU (installed bundle) | **SUCCESS**, 0 failed tests (the pre-install AU run reported "No types found" — expected for an uninstalled component) |
| `auval -v aumu OuSt OuDv` | `AU VALIDATION SUCCEEDED.` |
| `auval -a \| grep -i strata` (cold rescan, background) | `aumu OuSt OuDv  -  Ouaricon Audio Development: O-Strata-dev` |
| Version proof | `VERSION 1.0.0` keyword in `juce_add_plugin`; `CFBundleShortVersionString` = 1.0.0 in VST3 and AU plists; `moduleinfo.json` `"Version": "1.0.0"`; AU `version` = 65536 (= 1.0.0 — here the correct answer). pluginval's header does not print a version line, so the plists/moduleinfo are the evidence |
| Param-dump (`OUARICON_BUILD_TESTS=ON`) | `# params 219`; ID diff vs O-Prism's `params.tsv` = exactly `−oscATable −oscBTable +48`; the 24 `oscA*` rows sit between `oscAWarpAmt` and `oscBPos`; norm defaults equal the spec's column (0.166667 Torus, 1.000000 = 256 frames, 0.6/0.3/0.8/0.2/1.0 floats); `Imported…` byte-identical to the spec (`e2 80 a6`); `TerCX/TerCY` default prints `-0.00` — same JUCE artefact as O-Prism's `oscAPan` |
| `allSliderIds()` / relays | 35 × 2 + 117 + `delayDivision` = 188 (comment corrected from the stale `// 126`); `jassert (sliderAttachments.size() == sliderRelays.size())` added; 30 toggle relays unchanged; **31** native functions (45 − 14) |
| Removal-set grep gate (RESEARCH §4, 15 tokens) | empty over `Source/`, `CMakeLists.txt`, `tests/` |
| `grep -rln "Prism" plugins/O-Strata/Source/` | exactly the three tolerated files — `PluginProcessor.h` (banner: *Forked from O-Prism v1.24.0 (2026-09-07); see CHANGELOG.md.*), `ui/public/index.html`, `ui/public/js/i18n.js` (two-line provenance notes) |
| O-Prism isolation | `git status --short plugins/O-Prism` and `git diff --stat HEAD -- plugins/O-Prism` both empty |
| `check-i18n --plugin O-Strata` | ALL CHECKS PASS; [12] 1 module; [6] canon v2; [8] embedded + served; [10]/[15] pass; 0/242 zh unreviewed |
| `i18n-fr-lint --strict` / `i18n-zh-lint` | CLEAN exit 0 (348 rows) / GATE PASSED exit 0 (242 entries) |
| `check-ui-labels --plugin O-Strata` | ALL CHECKS PASSED; default + **20** states; [7] geometry diff 0 moved on fr and zh-Hans in every state |
| `boot-all-uis --plugin O-Strata --strict-tips` | 0 warn (console errors / failed subresources), **0 DEAD**, **0 late** tip bindings (O-Prism baseline: also 0 late) |
| `tests/ui_tip_render_check.js` | ALL CHECKS PASSED (2799; O-Prism 2851) with 106 bindings — after the test fix below |
| Standalone launch | `O-Strata-dev.app` stays alive 10 s after `open`, quits clean |
| PLUGINS.md | row → `🚧 Stage 1 / 1.0.0`; duplicate-row check empty |

---

## What changed (by task)

1. **Fork copy** — `Source/**`, `CMakeLists.txt`, `tests/{i18n-states.json, ui_tip_render_check.js, ui-stub/generic-overrides.json}`; `diff -r` against O-Prism was empty before the edits. Not copied: `backups/`, `BUG-tuning-tab-cutoff.md`, `CHANGELOG.md`, `NOTES.md`.
2. **Rename** — `PrismVoice.{h,cpp}` → `StrataVoice.{h,cpp}`, `PrismSound.h` → `StrataSound.h`, `PrismParamIds.h` → `StrataParamIds.h`; three-case sed over 62 files; `grep -rn -i prism` printed nothing afterwards. Covers `OStrataAudioProcessor(Editor)`, `"OStrataParameters"`, `presetManager (parameters, "O-Strata")`, `"OStrata_WebView"`, `O-Strata_UIResources`, `<h1>O-STRATA</h1>`, `ostrata.tipsEnabled`, `__strataTuningCleanup`, `PLUGIN = 'O-Strata'`, `'ostrata-i18n-'`, all AGPL banners.
3. **Removal set** — deleted `dsp/WavetableFactory.*`, `UserWavetableManager.*`, `WavetableImporter.*`, `WavetableEditor.*`, `js/wavetable-editor.js`, `css/wavetable-editor.css` (empty `css/` removed). Kept `WavetableData.h`, `WavetableGenerator.*`, `WavetableOscillator.*`. Preset-save modal CSS (`.wt-op-btn`, `.wt-save-modal*`) migrated verbatim into `index.html` first.
4. **Processor / editor** — `PluginProcessor.h`: three includes, factory getters, user-WT + editor API, `pOsc?Table`, `resolveActiveTable` removed; `placeholderTable` + `std::atomic<const WavetableData*> oscTablePtr[2]` + `lastAssignedTable[2]` replace the user/factory members; `updateWavetableAssignments`, `blockGeneration`, `RetiredTable`, `retireTable`, `timerCallback` kept byte-for-byte for Phase 2.1. `PluginProcessor.cpp`: constructor builds the sine placeholder and publishes it to both slots; `updateWavetableAssignments` reads `oscTablePtr[n]` (acquire); `getActiveOscTable` returns the published pointer; `getStateInformation` writes an empty `geometryImports` child, `setStateInformation` reads it with `ignoreUnused`; the `#if JUCE_WEB_BROWSER` guard around `createEditor()` was already present (verified, not duplicated — the param-dump build at `JUCE_WEB_BROWSER=0` is the proof). `PluginEditor.cpp`: two resource branches and 14 native functions removed; `getActiveOscInfo` rewritten to report `{"isUser":false,"factoryIndex":0,"name":"Sine","numFrames":N}`; `jassert` on relay/attachment parity added.
5. **Parameters** — the `Table` `AudioParameterInt` removed; 24 geometry pushes appended per oscillator via two local lambdas (`choice` asserts ≥ 2 entries). Host names family-prefixed (PLAN Decision 5):
   `Osc ? Source · Frames · Shape Drive · Mesh · Mesh Tilt X · Mesh Tilt Y · Mesh Projection Phi · Mesh Unwrap Mode · Mesh Loop Policy · Vol Field · Vol Orbit · Vol Sweep Axis · Vol Sweep Range · Vol Field Detail · Terrain · Ter Orbit Shape · Ter Centre X · Ter Centre Y · Ter Aspect · Ter Rotation · Ter Sweep Axis · Ter Sweep Range · Ter Image Blur · Ter Edge Mode`.
   Interval convention (RESEARCH A1 / PLAN Decision 3): 0.001 on unit ranges (`GeoDrive`, `VolSweepRange`, `VolDetail`, `TerAspect`, `TerSweepRange`, `TerBlur`), 0.01 on ±1 (`TerCX`, `TerCY`), 0.1 on degrees (`MeshTiltX/Y`, `MeshPhi`, `TerRot`). Non-ASCII spec strings built with `juce::CharPointer_UTF8` hex escapes (`Imported…`, `Epitrochoid 3·5·7`, `Hypocycloid 3·5·7`); source stays ASCII. `StrataParamIds.h`: `oscIds()` = 35 suffixes; `delayDivision` joins the delay group (D4); `// 188`.
6. **FactoryPresets.cpp** — 385 `osc?Table` entries and the `WT_*` enum stripped by the RESEARCH §2.2 sed + token-located block delete; 0 hits remain; braces 2807/2807.
7. **CMake** — `PLUGIN_CODE OuSt`, `VERSION 1.0.0`; four removed sources; `juce::juce_cryptography` after `juce_core`; `O-Strata_UIResources` lists exactly `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`; param-dump target renamed; comments refreshed. No root CMake edit (GLOB); 14 `O-Strata` tokens (= O-Prism's 14 `O-Prism`; PLAN's "15" was off by one). No `Source/geometry/`, no `geometry-view.js`, no PNG (D2).
8. **index.html** (4740 → 4299 lines) — wavetable tab, both drop overlays, both Shape dropdown groups, `#wavetable-tab`, `#wt-save-modal-overlay`, `#wt-modal-backdrop`, `JuceGetNativeFunction` bridge, `switchTab`'s editor hooks, the `WavetableSelector` class + manager modal + `_wtSelectors`, the `<link>`/`<script>` tags and their CSS removed; `WavetableDisplay` kept; the 6-line `posState` glue drives `wtDisplayA/B.fetchAndDraw` (like-for-like replacement of `WavetableSelector.wireDisplay()`). Canon region untouched. Surviving `wt-*` classes: exactly `wt-op-btn wt-save-modal wt-save-modal-buttons wt-save-modal-overlay`.
9. **i18n.js** (2568 → 2483 lines) — LABELS 159 → 136, I18N 108 → 106, TIP_BINDINGS 108 → 106, wavetable-catalogue `I18N_EXEMPT` block removed. **Deviation:** check-i18n [10] then reported 21 uncovered option texts (`Sine/Triangle/Saw/Square` on `subShape` and the four `lfoNShape` dropdowns, `Wind` on `noiseType`) that the deleted catalogue block had been covering incidentally; 5 arm-1 exempt entries were added (scope `.param-select`, `where` cites `PluginProcessor.cpp`). No new i18n keys. `I18N_EXEMPT` 106 → 83.
10. **Test fixtures** — `i18n-states.json` 23 → 20 states (`settings-popover` and `preset-save-modal` evals rewritten); `generic-overrides.json` −4 natives; `ui_tip_render_check.js` 4-tab loop, `[8]` 108 → 106, comments corrected.
11–14. Build / dump / validate / gates — table above.
15. **Docs** — `CHANGELOG.md` created (`## v1.0.0 (unreleased)`); `NOTES.md` Stage 1 entry + known issues; `PLUGINS.md` row; ROADMAP.md (13 lines) and ARCHITECTURE.md (15 lines) corrected to 48 / 219 / 141 → 188 / `StrataParamIds` (D5); STATUS.md `contract_checksums.roadmap` / `.architecture` re-recorded; `params.tsv` written.

## Fix made outside the plan

**`tests/ui_tip_render_check.js` `openTab()` clicks the tab's left edge** (`position: {x: 16, y: 12}`). The gate walks the tabs with the settings popover deliberately OPEN; the popover is `right: 0; width: 168px` and hangs over the rightmost tab's right part. Tabs are `flex: 1`, so removing the fifth tab moved Effects from 720–960 px to 900–1200 px and its centre (1050) under the popover (1032–1200) — Playwright's centre click was intercepted by the `Language` label and timed out. In O-Prism the covered tab was Wavetable, which the loop never clicks. A positioned click is still an actionability-checked click on the real control (never `force: true`). The real-UI behaviour (popover covering part of the rightmost tab) is inherited from O-Prism and disappears when Phase 3.1's Geometry tab restores five tabs.

## Manual smoke left for verify (not executable headlessly here)

- A held note sounds on Osc A and on Osc B (osc mix each way) — the sine placeholder through the voice/FX path (FUNC-09 smoke).
- Tuning tab loads a `.scl` file (FUNC-10 smoke).
- Both oscillator canvases paint the sine (the `posState` glue calls `getActiveOscFrame`; under the generic stub it returns null by design, so only the Standalone shows it).
- Save state → reload → all 219 params + tuning + `uiLanguage` restored; the XML contains an empty `<geometryImports/>` child.
- Standalone Inspect: no console errors, no resource-provider 404s (the headless boot shows 0 errors / all resources served; the WKWebView path is unverified).

## Notes for later stages

- **Stage 2.1:** the reaper (`retireTable`, `timerCallback`, `blockGeneration`) is live and idle; publish a baked table with `oscTablePtr[osc].store (…, release)` and `retireTable (old)`. `getActiveOscInfo` currently hard-codes `"Sine"` / `factoryIndex 0`.
- **Stage 3:** `timerCallback`'s `evaluateJavascript` push is untouched (ARCHITECTURE Decision 7 is a Stage 3 change). `WavetableDisplay` is the base for the ≋ view.
- **Stage 4.1:** every one of the 96 factory presets depended on a non-sine table (default `WT_Saw`; `WT_Sine` on 36 of 385 slots) — all are re-authored. `presetManager (parameters, "O-Strata")` gives a fresh folder; factory JSON regenerates at `1.0.0`.
- `stereoWidth` remains the only unbound parameter (as in O-Prism).
- `check-i18n` [12] now reports 1 module (the inline `<script type="module">`).
- Two prose mentions of `css/wavetable-editor.css` remain as history (`index.html` migration comment, `i18n.js` font-tail history) — intentional.
- `modules/registry.yaml` `used_by` for `note-expression` not regenerated (docs-only; `scripts/regen-registry-used-by.sh` when convenient).

## Requirements traceability

| ID | Evidence |
|---|---|
| COMPAT-01 | pluginval strictness 10 SUCCESS ×2 (installed VST3 + AU), `auval -v aumu OuSt OuDv` SUCCEEDED, `auval -a` lists the AU |
| FUNC-09 / FUNC-10 (smoke) | build + param contract + Standalone launch done here; audible/Scala checks are manual → verify phase; formal verification in Phase 2.1 |
