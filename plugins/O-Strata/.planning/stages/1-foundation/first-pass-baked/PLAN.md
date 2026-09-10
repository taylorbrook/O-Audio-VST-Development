# Stage 1 — Foundation: PLAN

**Plugin:** O-Strata · **Stage:** 1 of 4 (Foundation + Shell, single phase) · **Phase:** plan
**Date:** 2026-09-07 · **Mode:** manual · **Branch:** `main` (trunk-based; path-scoped commits)
**Inputs:** `stages/1-foundation/CONTEXT.md` (D1–D6, amended test criteria), `stages/1-foundation/RESEARCH.md` (§2 open items, §3 declarations, §4 removal trace, §5 rename, §6 build/gates, §7 state, §9 ordering, §10 contradictions), `parameter-spec.md` v1 (locked), `research/ARCHITECTURE.md` Decisions 2/5/7, `ROADMAP.md` Stage 1.
**Line numbers** below are O-Prism v1.24.0's and drift the moment an edit lands above them — always re-locate by the quoted token (memory `pattern_deferred_item_line_number_drifts_record_a_token`).

---

## Goal

A buildable, validating **O-Strata** that is a complete, independent copy of O-Prism v1.24.0 with the wavetable library removed and 48 inert geometry parameters added. No generator code. Both oscillators play `WavetableGenerator::generateProceduralTable(Sine)` through the unchanged voice/FX path. `plugins/O-Prism/` is not touched.

**Requirement verified:** COMPAT-01 (pluginval strictness 10 on VST3 + AU; auval pass). FUNC-09/FUNC-10 smoke only.

---

## Decisions resolved for the executor

These close RESEARCH §10's twelve flags. They are plan decisions, not open questions; the executor applies them without asking.

| # | Flag (RESEARCH §10) | Decision |
|---|---|---|
| 1 | `createEditor()` guard "to add" | **Verify present** (`PluginProcessor.cpp` `#if JUCE_WEB_BROWSER` around the `PluginEditor.h` include and the `createEditor()` body). Do not duplicate it. The param-dump build (`JUCE_WEB_BROWSER=0`) is the proof. |
| 2 | CMake snippet hunk 4 / root `add_subdirectory` | Hunk 2 `+` lines and hunk 4's `geometry-view.js` + PNG are **not** applied (D2). No root CMake edit; a **re-configure** (`cmake -S . -B build`) is mandatory because the root GLOB is evaluated at configure time. |
| 3 | Param-dump text vs spec decimals | Compare ranges/defaults **numerically**. Intervals follow O-Prism's convention: `0.001` on unit ranges, `0.01` on ±1, `0.1` on degrees (RESEARCH A1). Record in SUMMARY.md. |
| 4 | Non-ASCII spec strings | Carry the locked strings, constructed as `juce::String (juce::CharPointer_UTF8 ("Imported\xE2\x80\xA6"))`, `("Epitrochoid 3\xC2\xB7" "5\xC2\xB7" "7")`, `("Hypocycloid 3\xC2\xB7" "5\xC2\xB7" "7")`. Source stays ASCII. Host **names** are ASCII (`Phi`, never `φ`). |
| 5 | Colliding host display names | **Family-prefixed host names** per RESEARCH §3.3 (`Osc A Mesh Tilt X`, `Osc A Vol Sweep Axis`, `Osc A Ter Sweep Axis`, …). The spec locks IDs/ranges/defaults/choice strings, not `name`. The full 24-name list is in Task 5. |
| 6 | `PrismParamIds` in locked docs | File becomes `StrataParamIds.h` (D1). Spell `StrataParamIds` in ROADMAP/ARCHITECTURE during the D5 edit; leave `parameter-spec.md` untouched. |
| 7 | `FactoryPresets.cpp` mechanism | Strip anyway (D1): 385 entries + the `WT_*` enum (carries `WT_PrismSpectrum`). Two-expression sed + one token-located block delete (Task 6). |
| 8 | `getActiveOscInfo` "kept" but reads `oscATable` | **Keep and rewrite** the body (Task 4). Still registered → 31 native functions. |
| 9 | Preset-save modal CSS lives in `wavetable-editor.css` | **Migrate** `.wt-op-btn` (+hover/active), `.wt-save-modal-overlay` (+.active), `.wt-save-modal`, `.wt-save-modal h3`, `.wt-save-modal input` (+focus), `.wt-save-modal-buttons` verbatim into `index.html`'s `<style>` before deleting the file. Class names unchanged. |
| 10 | Two non-wavetable `i18n-states.json` states reference removed ids | Delete 3 wavetable states, **rewrite** `settings-popover` and `preset-save-modal` evals (Task 10). 23 → 20 states. |
| 11 | ROADMAP:13 complexity arithmetic | Correct to 219 in the D5 sweep; capped score stays 2.0. |
| 12 | O-Prism history comments become "O-Strata history" after the sed | Two-line provenance note at the top of the history comment in `index.html` and `i18n.js`; do not rewrite the prose. Tolerated survivors = `PluginProcessor.h`, `ui/public/index.html`, `ui/public/js/i18n.js` (list in SUMMARY.md). |

Also: keep `WavetableDisplay` and add the 6-line `posState` glue (RESEARCH §7, A4) so the oscillator canvases keep painting the placeholder through `getActiveOscFrame`. This is within D3 (removes a class, adds no control).

---

## Tasks

Order follows RESEARCH §9. Tasks 2–10 are pure file edits verifiable without a build; 11–14 need the build. Each task ends with a check the executor runs before moving on.

### Task 1 — Fork copy
- **Create:** `plugins/O-Strata/Source/**` (all of O-Prism's `Source/`), `plugins/O-Strata/CMakeLists.txt`, `plugins/O-Strata/tests/i18n-states.json`, `tests/ui_tip_render_check.js`, `tests/ui-stub/generic-overrides.json`
- **Do not copy:** `backups/`, `BUG-tuning-tab-cutoff.md`, `CHANGELOG.md`, `NOTES.md`, `.planning/`
- **Depends on:** none
- **Verify:** `diff -r plugins/O-Prism/Source plugins/O-Strata/Source` empty; `git status --short plugins/O-Prism` empty.

### Task 2 — Rename `Prism` → `Strata` (RESEARCH §2.5)
- **Modify:** 62 files under `plugins/O-Strata/{Source,tests,CMakeLists.txt}`
- **Rename files:** `PrismVoice.{h,cpp}` → `StrataVoice.{h,cpp}`, `PrismSound.h` → `StrataSound.h`, `PrismParamIds.h` → `StrataParamIds.h` (plain `mv`; nothing is tracked yet)
- **Sweep:** `grep -rl -i 'prism' Source CMakeLists.txt tests | xargs sed -i '' -e 's/Prism/Strata/g' -e 's/PRISM/STRATA/g' -e 's/prism/strata/g'` from `plugins/O-Strata/`
- **Covers (must all change):** `OPrismAudioProcessor(Editor)`, `PrismVoice/Sound/ParamIds`, `"OPrismParameters"`, `"O-Prism"` ×3 (`getName`, `presetManager (parameters, "O-Prism")`, tuning HTML export title), `"OPrism_WebView"`, `O-Prism_UIResources` ×2, `<h1>O-PRISM</h1>`, `'O-PRISM'` exempt row, `oprism.tipsEnabled`, `__prismTuningCleanup`, `PLUGIN = 'O-Prism'`, `'oprism-i18n-'`, all AGPL banner lines, the `#include` lines for the four renamed files, `Source/PrismVoice.cpp` in CMake
- **Do NOT sweep:** `plugins/O-Strata/.planning/**`, `PLUGINS.md`
- **Depends on:** Task 1
- **Verify:** `grep -rn -i prism plugins/O-Strata/Source plugins/O-Strata/CMakeLists.txt plugins/O-Strata/tests` prints nothing; `ls plugins/O-Strata/Source/Strata*` = 4 files. Provenance notes (Task 15) are written **after** this task so the sed cannot rewrite them.

### Task 3 — Delete the removal set (after CSS migration)
- **First, migrate CSS:** copy the rule blocks listed in Decision 9 from `Source/ui/public/css/wavetable-editor.css` (`.wt-op-btn` block ≈ :189-201, save-modal blocks ≈ :225-269) verbatim into `index.html`'s `<style>`, beside the existing `.wt-save-modal h3` line-height pin.
- **Delete:** `Source/dsp/WavetableFactory.{h,cpp}`, `UserWavetableManager.{h,cpp}`, `WavetableImporter.{h,cpp}`, `WavetableEditor.{h,cpp}`, `Source/ui/public/js/wavetable-editor.js`, `Source/ui/public/css/wavetable-editor.css` (remove the now-empty `css/` dir if nothing else is in it)
- **Keep:** `WavetableData.h`, `WavetableGenerator.{h,cpp}`, `WavetableOscillator.{h,cpp}`
- **Depends on:** Task 2
- **Verify:** `ls plugins/O-Strata/Source/dsp | grep -c Wavetable` = 5 files (Data.h, Generator.h/.cpp, Oscillator.h/.cpp); `grep -c 'wt-save-modal-overlay' Source/ui/public/index.html` ≥ 2 (rule + markup).

### Task 4 — Processor / editor C++ edits (RESEARCH §2.3, §4, §7)
- **Modify:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`
- **`PluginProcessor.h`:**
  - delete the three removed includes (`dsp/WavetableFactory.h`, `dsp/UserWavetableManager.h`, `dsp/WavetableEditor.h`); keep `dsp/WavetableData.h`, `dsp/WavetableGenerator.h`
  - delete factory getters (`getFactoryTable`, `getNumFactoryTables`, `getTableName`, `getTableCategory`) and the user-WT/editor API block, **keeping** `const WavetableData* getActiveOscTable (int oscIndex) const;`
  - replace `factoryTables`, `tableInfoList`, `userWavetableManager`, `wavetableEditor`, `editingOscIndex`, `userTableNameA/B`, `userTablePtrA/B`, `lastAssignedTableA/B` with:
    ```cpp
    // Placeholder table (Stage 1): both oscillators read this until Phase 2.1's
    // GeometryBakeScheduler publishes baked tables into oscTablePtr[].
    std::unique_ptr<WavetableData> placeholderTable;
    std::atomic<const WavetableData*> oscTablePtr[2] { nullptr, nullptr };
    const WavetableData* lastAssignedTable[2] { nullptr, nullptr };   // audio thread only
    ```
    keep `lastTuningPreset`, `lastTonic`
  - delete `pOscATable`, `pOscBTable`, `resolveActiveTable`; **keep** `updateWavetableAssignments()`, `blockGeneration`, `RetiredTable`, `retiredTables`, `retireTable`, `timerCallback` byte-for-byte (D2: reaper stays for 2.1)
- **`PluginProcessor.cpp`:**
  - constructor: replace factory-library build + `userWavetableManager.loadFromDisk()` with `placeholderTable = WavetableGenerator::generateProceduralTable (WaveShape::Sine);` + two `oscTablePtr[n].store (…, release)`; voices get `setWavetableA/B (placeholderTable.get())`; `lastAssignedTable[0] = lastAssignedTable[1] = placeholderTable.get()`
  - delete `pOscATable/pOscBTable = getRawParameterValue(…)`
  - `updateWavetableAssignments()`: the two `resolveActiveTable (n)` reads become `oscTablePtr[n].load (std::memory_order_acquire)`; `lastAssignedTableA/B` → `lastAssignedTable[0]/[1]`; the voice loop is otherwise unchanged
  - `getActiveOscTable` returns `oscTablePtr[osc].load (acquire)`; delete the user-wavetable API, `resolveActiveTable`, and the editor API
  - `getStateInformation`: replace the `userWavetables` child write with `state.getOrCreateChildWithName ("geometryImports", nullptr);` (+ the RESEARCH §7 comment)
  - `setStateInformation`: replace the `userWavetables` restore with `juce::ignoreUnused (state.getChildWithName ("geometryImports"));`; `tuningEngine`/`uiLanguage` code untouched
  - `createEditor()`: **verify** the `#if JUCE_WEB_BROWSER … #else GenericAudioProcessorEditor` guard is present; no edit
- **`PluginEditor.cpp`:**
  - `getResource`: delete the `/js/wavetable-editor.js` and `/css/wavetable-editor.css` branches
  - delete the 14 native functions (`getUserWavetableList importUserWavetable importUserWavetableData selectUserWavetable clearUserWavetableOverride deleteUserWavetable startWavetableEditor stopWavetableEditor getEditorFrameWaveform getFrameHarmonics setFrameHarmonics applyFrameOperation saveEditedWavetable getAllEditorFrameWaveforms`)
  - rewrite `getActiveOscInfo`'s body: `auto* table = processorRef.getActiveOscTable (oscIndex); complete ("{\"isUser\":false,\"factoryIndex\":0,\"name\":\"Sine\",\"numFrames\":" + juce::String (table ? table->numFrames : 0) + "}");` (keep the existing arg validation)
  - keep `getActiveOscFrame`, `toJsonArray`, `toJsonFloatArray`
  - after the attachment loop add `jassert (sliderAttachments.size() == sliderRelays.size());`
- **Depends on:** Task 3
- **Verify:** the RESEARCH §4 two-way grep gate prints nothing:
  `grep -rn "WavetableFactory\|UserWavetableManager\|WavetableImporter\|WavetableEditor\|oscATable\|oscBTable\|pOscATable\|pOscBTable\|resolveActiveTable\|userWavetables\|userTablePtr\|factoryTables\|tableInfoList\|TableInfo\|WT_" plugins/O-Strata/Source/ plugins/O-Strata/CMakeLists.txt plugins/O-Strata/tests/`
  (Tasks 6, 8, 10 also feed this gate — it goes fully green after Task 10; after Task 4 the remaining hits must be only in `FactoryPresets.cpp`, `index.html`, `i18n.js`, `tests/`.)
  `grep -c withNativeFunction Source/PluginEditor.cpp` = 31.

### Task 5 — Parameters (RESEARCH §3)
- **Modify:** `Source/PluginProcessor.cpp` (`createOscParameters`), `Source/StrataParamIds.h` (`oscIds`, `allSliderIds`)
- **`createOscParameters (prefix)`:** delete the `AudioParameterInt {prefix + "Table"}` push; after the `WarpAmt` push and before `return params;` append the 24 pushes in this exact order with these host names (`label` = `"Osc A"` / `"Osc B"`):

  | # | ID suffix | Type | Host name | Range · interval / choices | Default |
  |---|---|---|---|---|---|
  | 1 | `GeoSource` | Choice | `label + " Source"` | `Mesh, Volume, Terrain` | 0 |
  | 2 | `GeoFrames` | Choice | `label + " Frames"` | `64, 128, 256` | 2 |
  | 3 | `GeoDrive` | Float | `label + " Shape Drive"` | 0–1 · 0.001 | 0.0 |
  | 4 | `Mesh` | Choice | `label + " Mesh"` | `Sphere, Torus, Twisted Star, Crescent, Torus Knot, fBm Blob, Imported…` | 1 |
  | 5 | `MeshTiltX` | Float | `label + " Mesh Tilt X"` | −90–90 · 0.1 | 0.0 |
  | 6 | `MeshTiltY` | Float | `label + " Mesh Tilt Y"` | −90–90 · 0.1 | 0.0 |
  | 7 | `MeshPhi` | Float | `label + " Mesh Projection Phi"` | 0–360 · 0.1 | 0.0 |
  | 8 | `MeshUnwrap` | Choice | `label + " Mesh Unwrap Mode"` | `XY Projection, Centroid Distance` | 0 |
  | 9 | `MeshLoop` | Choice | `label + " Mesh Loop Policy"` | `Largest, Sum` | 0 |
  | 10 | `VolField` | Choice | `label + " Vol Field"` | `Torus SDF, Box SDF, Gyroid, Sphere SDF, fBm Noise` | 0 |
  | 11 | `VolOrbit` | Choice | `label + " Vol Orbit"` | `Torus Knot (2,3), Torus Knot (3,5), Torus Knot (5,7), Lissajous Knot` | 0 |
  | 12 | `VolSweepAxis` | Choice | `label + " Vol Sweep Axis"` | `Orbit Scale, Knot Phase, Z Offset` | 0 |
  | 13 | `VolSweepRange` | Float | `label + " Vol Sweep Range"` | 0–1 · 0.001 | 0.6 |
  | 14 | `VolDetail` | Float | `label + " Vol Field Detail"` | 0–1 · 0.001 | 0.3 |
  | 15 | `Terrain` | Choice | `label + " Terrain"` | `Sine Product, Radial Rings, Imported…` | 0 |
  | 16 | `TerOrbit` | Choice | `label + " Ter Orbit Shape"` | `Ellipse, Epitrochoid 3·5·7, Hypocycloid 3·5·7, Superellipse` | 0 |
  | 17 | `TerCX` | Float | `label + " Ter Centre X"` | −1–1 · 0.01 | 0.0 |
  | 18 | `TerCY` | Float | `label + " Ter Centre Y"` | −1–1 · 0.01 | 0.0 |
  | 19 | `TerAspect` | Float | `label + " Ter Aspect"` | 0.1–1 · 0.001 | 1.0 |
  | 20 | `TerRot` | Float | `label + " Ter Rotation"` | 0–360 · 0.1 | 0.0 |
  | 21 | `TerSweepAxis` | Choice | `label + " Ter Sweep Axis"` | `Radius, Rotation, Centre X, Centre Y` | 0 |
  | 22 | `TerSweepRange` | Float | `label + " Ter Sweep Range"` | 0–1 · 0.001 | 0.8 |
  | 23 | `TerBlur` | Float | `label + " Ter Image Blur"` | 0–1 · 0.001 | 0.2 |
  | 24 | `TerEdge` | Choice | `label + " Ter Edge Mode"` | `Mirror, Window` | 0 |

  `Imported…`, `Epitrochoid 3·5·7`, `Hypocycloid 3·5·7` via `CharPointer_UTF8` hex escapes (Decision 4); build those `StringArray`s from `juce::String` objects. All Choice lists ≥ 2 (min 2). `ParameterID` version hint `1` like every other O-Prism param. Not added to `modSlot?Dst` (26 entries unchanged). No `p…` raw-value caches.
- **`StrataParamIds.h`:** `oscIds()` drops `"Table"` and appends the 24 suffixes in the table order (→ 35 per oscillator); `allSliderIds()` delay group gains `"delayDivision"` (D4); fix the trailing `// 126` comment to `// 188`.
- **Depends on:** Task 4
- **Verify:** `grep -c 'prefix + "' Source/PluginProcessor.cpp` rose by 23 net (−1 +24); hand-count `oscIds` list = 35; `grep -c delayDivision Source/StrataParamIds.h` = 1.

### Task 6 — `FactoryPresets.cpp` strip (RESEARCH §2.2)
- **Modify:** `Source/FactoryPresets.cpp`
- **Procedure:** `sed -i '' -E 's/\{ "osc[AB]Table", WT_[A-Za-z]+ \},? ?//g; s/m\["osc[AB]Table"\] = WT_Saw; //g'` then delete the "Wavetable indices" comment + `WT_*` enum block (re-locate by the `WT_Saw` token, ≈ :104-114). Optionally `cat -s` the blank runs it leaves.
- **Depends on:** Task 2 (independent of 3–5; may run in parallel)
- **Verify:** `grep -c 'osc[AB]Table\|WT_' Source/FactoryPresets.cpp` = 0; brace count balanced (`grep -o '{' | wc -l` = `grep -o '}' | wc -l`).
- **Stage 4 note for SUMMARY.md:** every preset's audible identity depended on a non-sine table (default `WT_Saw`; `WT_Sine` on 36/385 slots) — all 96 presets are re-authored in Stage 4.1.

### Task 7 — CMake (RESEARCH §6.1; `mockups/v1-CMakeLists-SNIPPET.txt` hunks 1, 3, 6 + removals only)
- **Modify:** `plugins/O-Strata/CMakeLists.txt`
- `juce_add_plugin(O-Strata … PLUGIN_CODE OuSt PRODUCT_NAME "O-Strata${OUARICON_DEV_SUFFIX}" VERSION 1.0.0 …)` — `VERSION`, never `PLUGIN_VERSION`; formats/flags inherited (VST3 AU Standalone, `IS_SYNTH TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`)
- `target_sources`: remove `WavetableFactory.cpp`, `WavetableImporter.cpp`, `UserWavetableManager.cpp`, `WavetableEditor.cpp`; `StrataVoice.cpp` already renamed by Task 2. **No** `Source/geometry/*.cpp` lines (D2).
- `target_link_libraries`: add `juce::juce_cryptography` after `juce_core` (first consumer in the repo — expect a longer first configure)
- `juce_add_binary_data(O-Strata_UIResources SOURCES …)` lists exactly `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js` — remove the two wavetable-editor lines; **no** `geometry-view.js`/PNG (D2). One binary-data target only.
- `ouaricon_add_param_dump(O-Strata …)` inside the existing `OUARICON_BUILD_TESTS` block (rename only)
- No root `CMakeLists.txt` edit.
- **Depends on:** Task 3
- **Verify:** `grep -c "O-Strata" CMakeLists.txt` = 15; `grep -n "Wavetable" CMakeLists.txt` shows only `WavetableGenerator.cpp` and `WavetableOscillator.cpp`; `grep -c juce_cryptography` = 1; `grep -n "VERSION 1.0.0"` = 1 hit inside `juce_add_plugin`.

### Task 8 — `index.html` strip + glue (RESEARCH §2.1 table, §7)
- **Modify:** `Source/ui/public/index.html`
- **Delete regions (re-locate by token):** CSS `.wt-drop-overlay` block; CSS manager-modal block (`.wt-modal-backdrop` … `.wt-modal-empty`); the `.wt-label`, `.wt-modal-close`, `.wt-waveform-preview-label`, `.wt-modal-list .wt-delete-btn`, `.wt-osc-btn`, `.wt-modal h3` members of the line-height selector lists (**keep** `.wt-op-btn` and `.wt-save-modal h3`; mind trailing commas); `html[lang="zh-Hans"] .wt-osc-btn` block; the `data-tab="wavetable"` tab; both `.wt-drop-overlay` divs (keep `#canvasWrapA/B` + `#canvasOscA/B`); both Shape `.dropdown-group`s containing `#select-oscATable` / `#select-oscBTable`; `#wavetable-tab` in full; `#wt-save-modal-overlay` (the **preset** save modal `#preset-save-modal-overlay` stays); `window.JuceGetNativeFunction = …`; the `wasWavetable` / `WavetableEditor.onTab(De)Activated` lines in `switchTab` (keep the 4-line class toggle); `WavetableSelector` class + manager-modal functions + the `window._wtSelectors` instantiation; `#wt-modal-backdrop` markup; the `<link … wavetable-editor.css>` and `<script … wavetable-editor.js>` tags. Fix the two "wavetable selector handles dropdown binding" bind comments.
- **Keep:** `WavetableDisplay` class and its two instances.
- **Add** after the two `WavetableDisplay` instances:
  ```js
  for (const [display, posId] of [[wtDisplayA, 'oscAPos'], [wtDisplayB, 'oscBPos']]) {
      const posState = Juce.getSliderState(posId);
      const refresh = () => display.fetchAndDraw(posState.getNormalisedValue());
      posState.valueChangedEvent.addListener(refresh);
      refresh();
  }
  ```
  (use the actual instance identifiers found in the file if they differ from `wtDisplayA/B`)
- **Do not touch** the canon region `let uiLanguage = 'en';` … end of `initI18n()` (check-i18n assertion 6).
- **Depends on:** Task 3 (CSS migration)
- **Verify (P13):** `grep -n "_wtSelectors\|showWavetableManager\|WavetableSelector\|WavetableEditor\|JuceGetNativeFunction\|select-osc[AB]Table\|wt-drop\|wt-modal\|wt-osc\|wavetable-tab\|data-tab=\"wavetable\"" Source/ui/public/index.html` prints nothing; `grep -o "wt-[a-z-]*" Source/ui/public/index.html | sort -u` = only `wt-op-btn`, `wt-save-modal`, `wt-save-modal-buttons`, `wt-save-modal-overlay`; `node --check` is not applicable to inline modules — the console-error check is Task 14.

### Task 9 — `i18n.js` strip (RESEARCH §2.1 deletion list)
- **Modify:** `Source/ui/public/js/i18n.js`
- **Delete whole entries/lines (all three languages go together):**
  - `LABELS` 23 keys: `tab.wavetable label.dropWav label.oscAShort label.oscBShort label.harmonics label.waveform label.normalize label.normalizeGlobal label.fadeEdges label.reverse label.reverseOrder label.smooth label.saveShort label.saveWavetable label.userWavetables label.close label.delete label.importWav label.manage label.noUserWavetables aria.wavetableName aria.undo aria.redo` + the "── Wavetable tab ──" block comment and the "TWO keys for one English word" comment (159 → 136)
  - `I18N` 2 entries: `tip.oscATable`, `tip.oscBTable` (108 → 106)
  - `TIP_BINDINGS` 2 rows: `['#select-oscATable', …]`, `['#select-oscBTable', …]` (108 → 106)
  - `I18N_EXEMPT`: the 28-name wavetable-catalogue block (contains `'Strata Spectrum'` after the sed)
- **Keep:** `label.shape`, `label.save`, `label.cancel`, `label.savePreset`, `aria.presetName`, `label.subtitle`, tuning `label.gen*` keys, every tip whose *body* mentions tables. `'O-STRATA'` exempt row already renamed by Task 2 in step with `<h1>O-STRATA</h1>`.
- **No new keys.**
- **Depends on:** Task 2 (independent of 3–8; may run in parallel)
- **Verify:** `node scripts/check-i18n.js --plugin O-Strata` exit 0 ([15] no dead keys, [12] 1 module, [6] canon v2); `node scripts/i18n-fr-lint.js --plugin O-Strata --strict` exit 0; `node scripts/i18n-zh-lint.js --plugin O-Strata` exit 0 (242 entries). Note check-i18n needs Task 8 done too (dead-key check reads the page).

### Task 10 — Test fixtures (RESEARCH §2.4)
- **Modify:** `tests/i18n-states.json`, `tests/ui-stub/generic-overrides.json`, `tests/ui_tip_render_check.js`
- **`i18n-states.json`:** delete `wavetable`, `wt-save-modal`, `wt-manager-modal`; rewrite
  `{ "name": "settings-popover", "eval": "document.getElementById('settings-popover').classList.add('visible');" }` and
  `{ "name": "preset-save-modal", "eval": "document.getElementById('settings-popover').classList.remove('visible'); document.getElementById('btn-preset-save').click();" }` → 20 states
- **`generic-overrides.json`:** delete the `getUserWavetableList`, `startWavetableEditor`, `getAllEditorFrameWaveforms`, `getFrameHarmonics` natives (keep `getPresetListWithCategories`, `getEmbeddedTuningList`); drop the wavetable paragraphs from `_why`
- **`ui_tip_render_check.js`:** tab loop drops `'wavetable'`; `[8]` expectation 108 → 106 ("103 parameter tips + 3 chrome tips"); `#select-*` row comment 23 → 21; the "105 of O-Prism's 173 parameters" comment → "103 of O-Strata's 219" (48 geometry params join the no-control list)
- **Depends on:** Task 2 (independent; may run in parallel)
- **Verify:** `node -e "JSON.parse(require('fs').readFileSync('plugins/O-Strata/tests/i18n-states.json'))"` and the same for `generic-overrides.json`; state count 20.

### Task 11 — Configure + build (background; RESEARCH §6.1)
- **Commands (repo root):**
  ```bash
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release        # re-configure: root GLOB picks up plugins/O-Strata
  ninja -C build O-Strata_VST3 O-Strata_AU O-Strata_Standalone   # run_in_background, one call; first build > 8 min possible
  ```
- **Depends on:** Tasks 4–10
- **Verify:** three artefacts under `build/plugins/O-Strata/O-Strata_artefacts/Release/{VST3,AU,Standalone}/`; warning diff vs an O-Prism build log = no new warnings. Fix compile errors in place (expected sources: a missed `lastAssignedTableA/B` reference, a `StringArray` initialiser mixing `const char*` and `juce::String`).

### Task 12 — Param-dump contract
- **Commands:**
  ```bash
  cmake -S . -B build -DOUARICON_BUILD_TESTS=ON && ninja -C build O-Strata-param-dump
  "$(find build/plugins/O-Strata -name 'O-Strata-param-dump' -perm +111 | head -1)" > plugins/O-Strata/.planning/params.tsv
  diff <(cut -f1 plugins/O-Prism/.planning/params.tsv) <(cut -f1 plugins/O-Strata/.planning/params.tsv)
  ```
- **Create:** `plugins/O-Strata/.planning/params.tsv`
- **Depends on:** Task 11
- **Verify:** header `# params 219`; ID diff vs O-Prism = exactly `−oscATable −oscBTable +48 geometry IDs`, the 24 `oscA*` rows sitting between `oscAWarpAmt` and `oscBPos`; choice strings byte-equal to the spec (incl. `Imported…`, `3·5·7`); defaults numerically equal (Decision 3). Then reset `-DOUARICON_BUILD_TESTS=OFF` (or leave — the option is harmless).

### Task 13 — Install + validate (COMPAT-01)
- **Commands:**
  ```bash
  ./scripts/build-and-install.sh O-Strata      # dual-variant sweep; installs VST3 + AU (not Standalone)
  /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --timeout-ms 60000 --validate ~/Library/Audio/Plug-Ins/VST3/O-Strata-dev.vst3
  /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --timeout-ms 60000 --validate ~/Library/Audio/Plug-Ins/Components/O-Strata-dev.component
  auval -v aumu OuSt OuDv
  auval -a | grep -i strata                    # cold rescan — run_in_background at the END of the batch, budget 2 min
  ```
- **Depends on:** Task 11
- **Verify:** both pluginval runs exit 0 and print version `1.0.0` in the header; `auval -v` prints `AU VALIDATION SUCCEEDED`; `auval -a` lists `aumu OuSt OuDv`. Capture the pluginval headers into SUMMARY.md.

### Task 14 — UI gates + Standalone smoke
- **Commands:**
  ```bash
  node scripts/check-ui-labels.js --plugin O-Strata
  node scripts/boot-all-uis.js --plugin O-Strata --strict-tips
  node plugins/O-Strata/tests/ui_tip_render_check.js
  open build/plugins/O-Strata/O-Strata_artefacts/Release/Standalone/O-Strata*.app
  ```
- **Depends on:** Tasks 11, 13 (and 8–10 green)
- **Verify:** check-ui-labels 0 moved on en/fr/zh arms, 20 states; boot-all-uis 0 console errors, 0 DEAD tip bindings (LATE count = O-Prism's baseline); tip render check passes with 106 bindings. **Standalone (manual, record each):** no console errors, no resource-provider 404s; both oscillator canvases paint the sine; a held note sounds on Osc A and on Osc B (osc mix each way); tuning tab loads a `.scl` file; save state → reload → all 219 params + tuning + `uiLanguage` restored and the XML contains an empty `<geometryImports/>` child.

### Task 15 — Docs, provenance notes, counts (D5, D6, Decision 12)
- **Create:** `plugins/O-Strata/CHANGELOG.md` — `## v1.0.0 (unreleased)` naming the fork base O-Prism v1.24.0 (commit `e88ec412`), the removal set, the 48 params, D4
- **Modify:** `plugins/O-Strata/NOTES.md` (append a Stage 1 entry — no rewrite); `Source/PluginProcessor.h` banner: one line `Forked from O-Prism v1.24.0 (2026-09-07); see CHANGELOG.md.`; `Source/ui/public/index.html` and `js/i18n.js`: a two-line note at the top of the history comment stating the version history below is O-Prism's
- **Modify:** `PLUGINS.md` row → `| O-Strata | 🚧 Stage 1 | 1.0.0 | Synth (3D-Geometry Microtonal Wavetable) | 2026-09-07 |`, then `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` must be empty
- **Modify:** `.planning/ROADMAP.md` and `.planning/research/ARCHITECTURE.md` — the 26 stale lines found by `grep -n "217\|\b46\b\|23 ×\|126 → 170\|\b170\b\|PrismParamIds"` → `219 / 48 / 24 × / 141 → 188 / StrataParamIds`; Stage 1 test criteria 217 → 219; ROADMAP:13 complexity line → `219/5 = 43.8, capped at 2.0`
- **Modify:** `.planning/STATUS.md` — re-record `contract_checksums.roadmap` and `.architecture` (`shasum -a 256`) after the edit
- **Create:** `stages/1-foundation/SUMMARY.md` — must include: the tolerated-survivor file list from `grep -rln "Prism" plugins/O-Strata/Source/` (expected exactly `PluginProcessor.h`, `ui/public/index.html`, `ui/public/js/i18n.js`), the 24 host names and the interval convention, the pluginval headers, the Stage 4 preset note, the LATE tip-binding baseline
- **Depends on:** Tasks 2 and 14 (provenance notes must post-date the sed; counts post-date the dump)
- **Verify:** `grep -rln "Prism" plugins/O-Strata/Source/` = the three files above and nothing else; PLUGINS.md duplicate check empty.

### Task 16 — Commit (path-scoped)
- **Commands:**
  ```bash
  git branch --show-current            # main
  git status --short                   # other sessions' edits (O-simpleGrain wave 4g) must NOT be staged
  git status --short plugins/O-Prism   # must be empty
  git diff --stat HEAD -- plugins/O-Prism   # must be empty
  git add plugins/O-Strata PLUGINS.md
  git commit -m "feat(O-Strata): Stage 1 foundation — fork of O-Prism v1.24.0, 48 geometry params, wavetable library removed" -- plugins/O-Strata PLUGINS.md
  git show --stat HEAD | head -40
  ```
- **Depends on:** Task 15
- **Verify:** `git show --stat` lists only `plugins/O-Strata/**` and `PLUGINS.md`; no tag created.

---

## Parallelism

| Wave | Tasks | Note |
|---|---|---|
| 1 | 1 → 2 | sequential; the sed must run over the full copy |
| 2 | 3 → 4 → 5 · 6 · 9 · 10 | 6, 9, 10 depend only on Task 2 and can run beside 3–5; 8 waits for 3 |
| 3 | 7, 8 | after 3 |
| 4 | 11 → 12, 13 (bg) → 14 | build once; pluginval/auval and gates fan out |
| 5 | 15 → 16 | docs after the numbers are known |

Grep gates: run the §4 gate after Task 4 (expect residual hits only in `FactoryPresets.cpp`, `index.html`, `i18n.js`, `tests/`), after Task 10 (expect nothing), and once more before Task 16.

---

## Files

**Created:** `plugins/O-Strata/Source/**` (fork), `plugins/O-Strata/CMakeLists.txt`, `plugins/O-Strata/tests/{i18n-states.json, ui_tip_render_check.js, ui-stub/generic-overrides.json}`, `plugins/O-Strata/CHANGELOG.md`, `plugins/O-Strata/.planning/params.tsv`, `stages/1-foundation/SUMMARY.md`
**Renamed:** `StrataVoice.{h,cpp}`, `StrataSound.h`, `StrataParamIds.h`
**Deleted (from the copy):** 8 `dsp/Wavetable{Factory,Importer,Editor}*` + `UserWavetableManager*` files, `js/wavetable-editor.js`, `css/wavetable-editor.css`
**Modified:** `PluginProcessor.{h,cpp}`, `PluginEditor.cpp`, `StrataParamIds.h`, `FactoryPresets.cpp`, `index.html`, `i18n.js`, the three test fixtures, `NOTES.md`, `PLUGINS.md`, `.planning/{ROADMAP.md, research/ARCHITECTURE.md, STATUS.md}`
**Read-only:** `parameter-spec.md`, `BRIEF.md`, `REQUIREMENTS.md`, everything under `plugins/O-Prism/`, root `CMakeLists.txt`

---

## Success criteria (CONTEXT.md Stage 1 criteria as amended by D1–D5)

- [ ] `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` builds clean, no new warnings vs O-Prism; `./scripts/build-and-install.sh O-Strata` installs with the dual-variant sweep
- [ ] `auval -a | grep -i strata` lists the AU; `auval -v aumu OuSt OuDv` passes
- [ ] pluginval strictness 10 passes VST3 and AU (**COMPAT-01**); header reports version `1.0.0`
- [ ] Host automation list shows **219** parameters; `oscATable`/`oscBTable` absent
- [ ] Param-dump prints **219** IDs in `parameter-spec.md` order; diff vs O-Prism = −2 +48; choice strings byte-equal to the spec; ranges/defaults numerically equal
- [ ] `allSliderIds().size() == 188`; `jassert (sliderAttachments.size() == sliderRelays.size())` holds; 30 toggle relays unchanged; 31 native functions
- [ ] Removal-set grep gate prints nothing; `grep -rln "Prism" plugins/O-Strata/Source/` = exactly the three tolerated files; `git diff --stat HEAD -- plugins/O-Prism` empty
- [ ] Stripped page passes `check-ui-labels` (20 states), `check-i18n`, `boot-all-uis --strict-tips` (0 errors, 0 dead), tip render check (106); fr lint exit 0; zh lint exit 0; Standalone shows no console errors and no 404s
- [ ] Notes play the sine placeholder on both oscillators through the unchanged voice/FX path; tuning tab loads a Scala file
- [ ] State save/reload round-trips all 219 params + tuning + `uiLanguage`; empty `geometryImports` child written and read without error
- [ ] `CHANGELOG.md` `## v1.0.0 (unreleased)`; PLUGINS.md row → 🚧 Stage 1 / 1.0.0; ROADMAP/ARCHITECTURE counts read 48 / 219 / 141 → 188 and `StrataParamIds`; STATUS.md checksums re-recorded
- [ ] One path-scoped commit; no tag

---

## Out of scope (do not do in this stage)

`Source/geometry/` and any bake code (→ 2.1) · geometry controls, `geometry-view.js`, shell PNG (→ 3.1/3.2) · replacing `timerCallback`'s `evaluateJavascript` push (→ Stage 3, ARCHITECTURE Decision 7) · factory-preset re-authoring (→ 4.1) · `stereoWidth` UI · `label.subtitle` copy change · any edit under `plugins/O-Prism/` · `parameter-spec.md` edits.

---

## Requirements traceability

| ID | Evidence produced by this plan |
|---|---|
| COMPAT-01 | Task 13 pluginval ×2 + auval outputs in SUMMARY.md |
| FUNC-09 / FUNC-10 (smoke) | Task 14 Standalone notes: sine audible on A and B; `.scl` loads — formal verification in Phase 2.1 |
