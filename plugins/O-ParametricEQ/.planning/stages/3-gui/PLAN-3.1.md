---
title: "O-ParametricEQ Stage 3 (GUI) — Phase 3.1 Execution Plan (layout, bindings, scale check)"
created: 2026-11-15
juce_version: "8.0.15"
phase: "3.1"
summary: "Seven waves, serial: (W0) pre-flight, baselines, params.tsv from the param dump; (W1) the page: v1-ui.html → index.html with the dragonfly plate ported from the test page (P4) and a pageReady call, i18n.js, the JUCE frontend; (W2) C++: the WebView editor from the template mapped onto the Stage 2 processor (Q2), 196 relays + attachments, 19 native functions, the processor owns OuariconPresetManager (C2) with PresetSaveGuard, createEditor swap behind JUCE_WEB_BROWSER, CMake snippet, version 0.7.0; (W3) render harness: the owed26 patch (8 rows) + 3 preset rows; (W4) gate fixtures (generic-overrides.json generated, i18n-states.json); (W5) build, both harnesses default and --long, the 11-mutant control, five UI gates, install, pluginval 10, auval; (W6) editor bench on the real editor: open / paint timing, preset settle, 196-parameter binding sweep both ways, disabled-state matrix, 10 open / close cycles Release and Debug, native-function diff; (W7) summary, STATUS, one path-scoped commit. No eqSections, analyzer, meter, listen state machine or node editing (3.2 / 3.3). No tag."
stages: [3]
agents: [gui-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 3 / Phase 3.1 Execution Plan (layout, bindings, scale check)

## Goal

The finalized v1 page is the plugin's editor, bound to all 196 parameters, opening at 960 × 600 in VST3, AU and the Standalone:

- **Bindings.** 196 relays + 196 attachments (C1: no fallback). Host automation and preset loads move every control; the page's edits reach the host. Controls for inert parameters are disabled per type / slope.
- **Scale.** Editor constructor → painted page ≤ 500 ms (bench ≤ 274 ms); a 196-value preset paints within 2 frames; idle relay traffic 0; 10 open / close cycles clean in Release and Debug.
- **Natives.** All 18 functions the page and `preset-manager.js` call, plus `pageReady`, are registered; the grep diff is empty.
- **Presets (C2).** The processor owns `OuariconPresetManager`; save / load work through the real path; presets never touch `output_gain` or `bypass`.
- **Look.** The mockup's dragonfly plate ships (U4), EB Garamond on canvas and DOM, all five UI gates pass.
- **Owed rows.** The eight VERIFICATION-2.6 probe rows + DM2 / AN1 / PX22 are render-harness rows (U8).

Version 0.7.0 built, installed, validated. Listen-off audio output unchanged (no DSP edit in 3.1).

**In scope:** `Source/ui/public/index.html`, `js/i18n.js`, `js/juce/*`; `Source/PluginEditor.h/.cpp`; `Source/PresetSaveGuard.h`; processor: preset manager member, `createEditor`; CMake UI binary data; render-harness rows (owed26 + preset); `tests/ui-stub/generic-overrides.json` + generator, `tests/i18n-states.json`, `.planning/params.tsv`.

**Out of scope (and what 3.1 does instead):**

| Later | 3.1 behaviour |
|---|---|
| 3.2 response display | `requestEqSections` registered, completes with `true`, emits nothing. No `DisplayModel` in the editor. The display shows grid and nodes, no curve (checklist "Deviations"). Node drag / create / delete and the 25th-band tip (U5) are 3.2. |
| 3.3 analyzer, meter, fills, listen | No `SpectrumAnalyzer` constructed (construction registers the consumer and turns the taps on). No `outputLevel`. `setListenBand (n)` stores `n − 1` clamped (−1 off) and the destructor resets it; the drag / latch state machine (U2) and the listen NaN fix are 3.3. `#meter-mount` stays the empty bordered box. |
| Stage 4 | Factory presets (empty set now), fr / zh-Hans review, WebView2 timing on Windows CI, session persistence of the current preset name (P9). |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/3-gui/RESEARCH-3.1.md` | All of it. Q1 (relay mechanics, bench figures), Q2 (template → processor map), Q3 (natives), Q4 (plate port), Q5 (fixtures), Q6 (owed rows), Calls C1 / C2 (resolved). |
| `stages/3-gui/CONTEXT.md` | U4, U6, U8, "Kept from the mockup", "Constraints". Binding. |
| `stages/3-gui/research-3.1/relay-bench/` | The bench app, injected timing script (`src/BenchScript.h`), `run-all.sh`, `analyze.py`. W6 re-points it at the real editor. |
| `stages/3-gui/research-3.1/plate-bench/plate_bench.js` | Plate geometry check (W5 reuses for the ported page). |
| `stages/3-gui/research-3.1/probe-port/owed26-render-harness.patch`, `owed26.inc`, `run.py` | P6: the rows, applied as written; `run.py` is the mutant control. |
| `mockups/v1-integration-checklist.md` | §§ 0 – 8 are acceptance; § 9 is verify / hands-on. § 0's fallback bullet is struck (C1). |
| `mockups/v1-PluginEditor-TEMPLATE.h / .cpp`, `v1-CMakeLists-SNIPPET.txt`, `v1-ui.html`, `v1-ui-test.html` (`:167`, `:334`, `:879-912`), `v1-i18n.js` | Sources of W1 / W2. The templates were never compiled. |
| `parameter-spec.md` | 196 IDs, ranges, skews, choice lists — fixture source (W4). |
| `Source/PluginProcessor.h/.cpp` | UI atomics (`:300-345`), codec (`getStateInformation` / `setStateInformation`), `createEditor` (`:1238`). |
| `plugins/O-AnalogEQ/Source/PluginEditor.cpp`, `PresetSaveGuard.h`, `tests/i18n-states.json` | Preset bridge, guard, states shape. |
| `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` | `setExcludedParameters`, `setValidatedApply`, lazy directory creation, reset-to-defaults apply (WR-01), `isFactoryPreset` = file in `Factory/`. |
| `scripts/param-dump/README.md`, `scripts/serve-ui.js:338-372`, `scripts/ui-stub/generic-juce-stub.js:154-215` | params.tsv and override merge. |
| Memory: `critical_webview_completion_gated_on_isvisible`, `pattern_webview_native_fn_bridge_gap`, `pattern_webview_one_shot_state_push_stale_on_preset_load`, `pattern_webview_launchasync_safepointer_no_complete`, `pattern_webview_knob_readout_scaled_value`, `critical_webview_resource_provider_and_schemes`, `critical_juce_webview_namespace_vs_postmessage`, `critical_webview2_runtime_gotchas_windows`, `pattern_canvas_text_needs_font_load_repaint`, `pattern_resolve_target_matches_juce_add_plugin_in_comment`, `pattern_check_i18n_rule8_fails_stage1_placeholder_editor`, `pattern_i18n_states_new_state_goes_last`, `critical_preset_manager_stale_customstate_child`, `pattern_savepresettofile_has_no_factory_guard`, `pattern_module_js_move_build_recopies_tracked_file`, `pattern_source_grep_gate_hits_configure_copied_module_js`, `index_ui_gates_and_probes` | Traps this phase walks through. |

---

## Plan-phase decisions

Taylor's calls (RESEARCH-3.1, resolved 2026-11-15), applied in this plan phase:

- **C1** All 196 relays; the fallback is gone. ROADMAP 3.1 criterion and risk row amended; checklist § 0 fallback bullet struck. WebView2 timing → Stage 4 Windows CI.
- **C2** The processor owns `OuariconPresetManager` from 3.1. ROADMAP 3.1 components amended.
- Count fix (no call): "nine" → "eight" probe-only rows in ROADMAP 3.1 and CONTEXT U8. ROADMAP checksum refreshed in STATUS.

The plan's calls:

- **P1 Files.**

  | File | Status | Content |
  |---|---|---|
  | `Source/ui/public/index.html` | new | `v1-ui.html` + P4 plate + P3 `pageReady` call. Nothing else edited without a recorded reason. |
  | `Source/ui/public/js/i18n.js` | new | `v1-i18n.js` verbatim. |
  | `Source/ui/public/js/juce/index.js`, `check_native_interop.js` | new | JUCE 8.0.15 frontend, copied from `~/JUCE/modules/juce_gui_extra/native/javascript/`. |
  | `Source/PluginEditor.h/.cpp` | new | P2. |
  | `Source/PresetSaveGuard.h` | new | O-AnalogEQ's, verbatim except the namespace / plugin name. |
  | `Source/PluginProcessor.h/.cpp` | edit | P5 only: preset manager member + setup, `createEditor`. Nothing in `processBlock`, the codec or any DSP path. |
  | `CMakeLists.txt` | edit | Snippet merge (P7), `OPEQ_VERSION 0.7.0`, `Source/PluginEditor.cpp` in `target_sources`, the shared-modules comment updated (preset-manager is now used). |
  | `tests/render-harness/main.cpp` | edit | P6 rows only. |
  | `tests/ui-stub/generic-overrides.json`, `tests/ui-stub/gen-overrides.py`, `tests/i18n-states.json` | new | P8. |
  | `.planning/params.tsv` | new | param-dump output. |

  `Source/dsp/*` and `Source/analysis/*`: **no edit.** `git diff HEAD -- Source/dsp Source/analysis` is empty at the commit.

- **P2 Editor = template mapped onto the Stage 2 processor (RESEARCH Q2).**
  - Class `OuariconParametricEQAudioProcessorEditor`, `juce::AudioProcessorEditor, private juce::Timer`.
  - **Member order:** 4 global relays, `std::array<BandRelays, 24>`, `std::unique_ptr<juce::WebBrowserComponent> webView`, 4 global attachments, `std::array<BandAttachments, 24>`, then the plain state (`pageIsReady`, `presetDialogInFlight`, chooser). Attachments after the WebView, relays before (destruction order). `BandRelays::addTo` registers all eight per band with `withOptionsFrom`.
  - **Removed from the template:** `displayModel` member, `takeAnalyzerFrame`, `outputLevelDB`, `editorOpen`. `timerCallback` exists but its body emits nothing in 3.1 (3.2 adds `eqSections`, 3.3 analyzer / meter). Comments in the body name the phase that fills it.
  - **Timer gate (P3):** `startTimerHz (30)` moves from the constructor into the `pageReady` handler; `pageIsReady` reset and `stopTimer()` first in the destructor.
  - **Destructor:** `stopTimer()`, `audioProcessor.listenBand.store (-1)` (U2; transient). Does not touch `setAnalyzerConsumer` (never registered in 3.1).
  - **WebView options:** `withNativeIntegrationEnabled`, `withResourceProvider` (bare paths, literal `if (url == "...") return makeBinaryResource (UIBinaryData::...` shape), `#if JUCE_WINDOWS` `withBackend (webview2)` + `withWinWebView2Options (...withUserDataFolder (…/Ouaricon/WebView2/O-ParametricEQ))` with the folder created first; `areOptionsSupported()` probe, `webView` null on failure and every later `webView->` guarded. Debug: `withKeepPageLoadedWhenBrowserIsHidden` not used.
  - **Size:** literal `setSize (960, 600);` then the restored `uiEditorWidth / Height` (read before the first `setSize`, because `resized()` writes them). `setResizable (true, true)`, limits 800 × 500 – 1600 × 1000, free aspect. `resized()` stores width / height to the atomics.
  - **Async natives** (`savePresetWithDialog`, `loadPresetFromFile`) capture a `SafePointer`; when it is gone they still call `complete (presetResult (false, {}))`. `presetDialogInFlight` cleared above every early return.
  - Any template line that does not compile against the real processor is mapped per Q2, never by adding a processor member (except P5).

- **P3 Native functions: 19.** The 18 of RESEARCH Q3 plus `pageReady`:

  | Name | Behaviour in 3.1 |
  |---|---|
  | `getParameterDefaults` | 74 slider IDs → default in engineering units, from `getDefaultValue()` + `convertFrom0to1`. |
  | `getUiLanguage` / `setUiLanguage` | Through the processor's codec; unknown code → en. |
  | `getUiState` | `{ analyzerMode, displayRange, selectedBand }` from the atomics. |
  | `setUiState (key, int)` | Same validation as `setStateInformation` (analyzerMode 0–3, displayRange ∈ {6, 12, 30}, selectedBand 0–24); invalid → `false`, atomic unchanged. |
  | `setListenBand (n)` | `listenBand = (1 ≤ n ≤ 24) ? n − 1 : −1`. |
  | `requestEqSections` | Completes `true`, emits nothing (3.2). |
  | `resizeEditor (w, h)` | Clamped 800 × 500 – 1600 × 1000, `setSize`. |
  | Preset ×10 | O-AnalogEQ bridge; `savePreset (name)` and `savePresetWithDialog` both consult `presetSaveRefusal` before writing (memory: `savePresetToFile` has no guard). |
  | `pageReady` | Sets `pageIsReady`, starts the 30 Hz timer, completes `true`. Idempotent (a reload calls it again). |

  The page calls `pageReady` once, after `bindBandStates` has run and the first `getUiState` answer applied (one line at the end of its init, recorded in the summary). Acceptance: `grep -o 'getNativeFunction("[^"]*"' index.html modules/preset-manager.js | sort -u` against `grep -o 'withNativeFunction ("[^"]*"' PluginEditor.cpp | sort -u` → identical, 19 each.

- **P4 Plate port (RESEARCH Q4, U4).**
  - Replace `v1-ui.html`'s `<div id="plate">` placeholder (and its comment block `:262-268`, the `images/dragonfly_plate.png` reference) with the test page's `<svg id="plate" viewBox="-300 -205 600 545">` and `buildPlate()` (seed `sd = 24`).
  - Geometry from the test page `:167`: `right: 1.5%; top: 3%; height: 94%; width: auto; max-width: 62%; opacity: 0.3`. Rotation only inside the SVG (`rotate(-21 0 60)`); delete the box rotation and `width: 60%` of `v1-ui.html:269`.
  - Colours: `buildPlate()` emits classes; CSS `.plate-ink { stroke: var(--frame) }`, `.plate-wash { fill: var(--paper-light) }`. No hex literal `#5C4033` / `#FAF0E6` left in the plate code (grep).
  - Behind the grid canvas in z-order; `pointer-events: none`; `aria-hidden="true"`.
  - Check: `plate-bench` re-run against the served `index.html` (W5) — plate bounding box within the test page's ± 1 px at 960 × 600, 800 × 500 and 1600 × 1000; idle task time shown vs hidden within run-to-run noise.

- **P5 Processor edits (C2).**
  - Member `OuariconPresetManager presetManager { parameters, "O-ParametricEQ" };` declared after `parameters`. Constructor body: `presetManager.setExcludedParameters ({ "output_gain", "bypass" }); presetManager.setValidatedApply (true);`. No `initializeFactoryPresets` call (empty factory set; no file I/O at construction — the module creates directories lazily).
  - **Session state does not go through the preset manager.** `getStateInformation` / `setStateInformation` are untouched: no `getStateAsXml` / `setStateFromXml` (that path carries the stale `<CustomState>` trap and would change the layout golden). Consequence: the preset name shown after a session reopen is "Default" until Stage 4 (P9).
  - `createEditor()`: `#if JUCE_WEB_BROWSER return new OuariconParametricEQAudioProcessorEditor (*this); #else return new juce::GenericAudioProcessorEditor (*this); #endif`. `hasEditor()` stays `true`. The render harness and param dump build with `JUCE_WEB_BROWSER=0` and exclude the editor TU (`ParamDump.cmake:138-157`); confirm both still link.
  - Nothing else in the processor. `git diff HEAD -- Source/PluginProcessor.cpp` touches only the constructor, `createEditor` and includes.

- **P6 Render-harness rows.**
  - Apply `research-3.1/probe-port/owed26-render-harness.patch` as written (`git apply --check` first). Eight rows XR-ls10, XR-ls11, XR-ls15, XR-px38, XR-px39, XR-px22, XR-dm2, XR-an1 in `owed26RowsXr()` after `owedRowsXr()`. No edit to their stimuli or thresholds.
  - Three new preset rows, in the harness's fake home (`CFFIXED_USER_HOME` re-exec already in place; check the resolved presets directory is under it before the first write, else STOP):
    - **XR-pm** round trip: randomise all 196 (seeded), `savePreset ("t31")`, randomise again with `output_gain` = +7 dB and `bypass` = on, `loadPreset ("t31")`. Every parameter except the two equals the saved value (normalised, exact); `output_gain` and `bypass` keep +7 / on; the six UI atomics and `listenBand` unchanged; `getCurrentPresetName() == "t31"`. Control: with `setExcludedParameters ({})` on a second processor, `output_gain` does change (the exclusion is what holds it).
    - **XR-pmd** omitted keys: a hand-written preset JSON carrying only `b03_gain` loads every other non-excluded parameter at its default (WR-01).
    - **XR-psg** guard: a file placed at `<fake home>/Library/O-ParametricEQ/Presets/Factory/Locked.json` → `isFactoryPreset ("Locked")` true and `presetSaveRefusal` refuses a save to that name and to that file; a user name is not refused. Control: with the file absent, no refusal.
  - Tally after W5: **166 PASS / 0 FAIL** (155 + 8 + 3). Designer harness unchanged at 150 / 0 / 18.

- **P7 CMake.** Merge `v1-CMakeLists-SNIPPET.txt`: one `juce_add_binary_data (OuariconParametricEQ_UI NAMESPACE UIBinaryData HEADER_NAME UIBinaryData.h SOURCES …)` with `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`, the module JS from their module paths (`${CMAKE_SOURCE_DIR}/modules/...`, not the configure-copied `Source/ui/public/modules/` files), `eb-garamond.css` and its three woff2; linked `PRIVATE`. Keep the comment guard on the declaration-command name; no new comment spells it. `Source/ui/public/modules/*.js` stay untracked (check `git status` before the commit; memory: build re-copies a tracked module file).

- **P8 Gate fixtures (RESEARCH Q5).**
  - `.planning/params.tsv` from `O-ParametricEQ-param-dump` (README recipe).
  - `tests/ui-stub/gen-overrides.py` reads `params.tsv` and `parameter-spec.md` and writes `generic-overrides.json`: 48 freq / Q sliders with their real `start / end / skew / interval` (freq skew 0.203214481472969, Q 0.186652…, taken from the dump, never typed), 72 combos with their real choice lists and default indices, a `_why` key. Run it, commit both. A second run reproduces the file byte for byte.
  - `tests/i18n-states.json` (new file; states in this order, any later state appended last): settings popover open; preset dropdown open only if the page has one (v1 shows `#preset-name` as a plain label: omit, with a note); hover-help off; **band 1 enabled and selected** (`eval` sets `b01_enabled` through the stub and `selectedBand` 1, so the strip's 14 labels are measured); band 1 as Low Cut at 6 dB/oct (disabled gain and Q labels measured). If `check-ui-labels` has a state-count pin, set it to the new count.

- **P9 Not persisting the preset name** in the session in 3.1 (P5 consequence). Recorded as a Stage 4 item: either a root attribute `presetName` in the codec (layout-golden change, own row) or the module's `getStateAsXml` with the `<CustomState>` strip. Taylor can pull it into 3.1 at execute if wanted; it is not needed for any 3.1 criterion.

- **P10 Timing in the real editor.** The research bench measured a bench editor. W6 re-builds the bench around the **real** `PluginEditor.cpp` and `index.html`, with the timing script injected by a bench-only compile definition `OPEQ_EDITOR_BENCH` (adds `withUserScript` + a `benchReport` native). The plugin build never defines it: acceptance greps the built plugin's compile commands and `PluginEditor.cpp` for the define being the only reference. For Logic (C1's measured row), the editor logs constructor → `pageReady` wall time and the page's own `performance.now()` at first paint (sent as `pageReady`'s argument) to `~/Library/O-ParametricEQ/Logs/ui-timing.log` **only if** `~/Library/O-ParametricEQ/ui-timing.flag` exists (checked once in the constructor, message thread). Five cold opens in Logic, median and worst recorded at verify. Stage 4 decides whether the flag hook stays.

---

## Tasks

### Wave 0 — pre-flight

**T1. Baselines** [run]
- `git branch --show-current` = main; `git status --short plugins/O-ParametricEQ` clean except `.planning/` (other sessions' files elsewhere are not ours).
- Configure `-DOUARICON_BUILD_TESTS=ON`; build both harnesses; record default tallies (expect designer 150 / 0 / 18, render 155 / 0). Log to the scratchpad.
- Build `O-ParametricEQ-param-dump`; write `.planning/params.tsv`; 196 data rows.
- Record a 0.6.0 render dump of 4 seeded sessions (listen off, analyzer no consumer) for T9's "no audio change" check.
- Depends on: none.

### Wave 1 — the page

**T2. UI files** [gui-agent]
- `index.html` from `v1-ui.html`; P4 plate port; one `pageReady` call at the end of init (P3). `i18n.js` verbatim; JUCE frontend copied.
- No other page edit. If a gate in W5 forces one, record it with its reason.
- Grep: no `#5C4033` / `#FAF0E6` in the plate code; no `dragonfly_plate.png`; no `vh` / `vw`; `user-select: none` present.
- Depends on: T1.

### Wave 2 — C++

**T3. Processor (P5)** [gui-agent]
- Preset manager member + setup; `createEditor` guard; include. Nothing else.
- Depends on: T1.

**T4. PresetSaveGuard** [gui-agent]
- Port `O-AnalogEQ/Source/PresetSaveGuard.h`; pure function, JUCE only.
- Depends on: T1.

**T5. Editor (P2, P3, P10)** [gui-agent]
- `PluginEditor.h/.cpp` from the template per P2; 196 relays + attachments; 19 natives; resource provider for every file in the binary-data target with the right MIME (`application/javascript; charset=utf-8`, `text/css`, `font/woff2`, `text/html`); `pageReady` timer gate; destructor; flag-gated timing log.
- Depends on: T3, T4.

**T6. CMake (P7)** [gui-agent]
- Snippet merge, version 0.7.0, editor TU in `target_sources`.
- Depends on: T2, T5.

### Wave 3 — render-harness rows

**T7. Owed rows + preset rows (P6)** [gui-agent]
- `git apply --check` then apply the owed26 patch; add XR-pm, XR-pmd, XR-psg with their controls.
- Depends on: T3, T4.

### Wave 4 — gate fixtures

**T8. Fixtures (P8)** [gui-agent]
- `gen-overrides.py` → `generic-overrides.json` (twice, byte-identical); `i18n-states.json`.
- Depends on: T1, T2.

### Wave 5 — build, gates, install, validators

**T9. Build and run** [run] — long commands in the background, each with its own log (600 s watchdog).
1. `ninja OuariconParametricEQ_VST3 OuariconParametricEQ_AU OuariconParametricEQ_Standalone` → no new warnings vs 0.6.0 in the plugin's TUs.
2. Render harness default → 166 / 0; `--long` → 0 FAIL. Designer harness default → 150 / 0 / 18 (unchanged).
3. Audio unchanged: the 4 sessions of T1 rendered by 0.7.0 are `memcmp`-equal.
4. Mutant control: `research-3.1/probe-port/run.py` against the real tree (11 mutants) → each fails its row; LS12 may also trip XR-px22 (expected).
5. UI gates from the repo root, all five, exit codes recorded: `check-i18n` 0, `i18n-fr-lint` 0, `i18n-zh-lint` 0, `check-ui-labels` 0, `boot-all-uis --strict-tips` 0 with 0 DEAD. **77 is not a pass** — install Playwright and re-run.
6. `plate-bench` against the served `index.html` (P4 check).
7. Greps: native-function diff (P3) identical, 19 / 19; `OPEQ_EDITOR_BENCH` absent from the plugin's compile commands; `git diff HEAD -- Source/dsp Source/analysis` empty; `Source/ui/public/modules/*.js` untracked.
- **On a failure:** a harness row red that is not new → STOP (3.1 changes no DSP). A new row red on the unmodified engine → the row port; compare to the research run (163 / 0 there) before touching anything. A gate red → fix the page / editor / fixture, never the gate script; re-run all five.
- Depends on: T2 – T8.

**T10. Install and validate** [run]
1. `./scripts/build-and-install.sh O-ParametricEQ` → rc 0, two `-dev` bundles, 0.7.0, no alternate-variant orphan.
2. pluginval strictness 10, `--timeout-ms 600000`: VST3 ×2, AU ×1 → SUCCESS (pluginval opens the editor: the WebView path is now exercised).
3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`. Never `auval -a`.
4. Standalone launch: WebView not blank (screenshot to the scratchpad), page console 0 errors.
- Depends on: T9.

### Wave 6 — editor bench on the real editor (§ 0, § 4 – 7 of the checklist)

**T11. Editor bench** [run; gui-agent for the bench code]
- Copy `research-3.1/relay-bench/` to `stages/3-gui/execute-3.1/editor-bench/`; swap `BenchEditor` for the real `PluginEditor.cpp` + real `UIBinaryData` sources, `OPEQ_EDITOR_BENCH` defined. Build Release and Debug.
- Rows (each recorded with figures):
  - **E-open:** 5 fresh processes × 10 opens: constructor → painted page, cold median / worst ≤ 500 ms (expect ≈ 260 / 280); warm recorded.
  - **E-props:** `b01_freq` / `b24_freq` start 10, end 30000, skew 0.203214…; `b24_type` 8 choices; boot console 0 warnings; 0 "unknown to the backend".
  - **E-idle:** 10 s idle after paint: 0 relay messages, 0 script calls except the empty timer (none emitted).
  - **E-preset:** 50 runs each of (a) 196 sets on the message thread, (b) `setStateInformation`, (c) sets from another thread, (d) `presetManager.loadPreset`: last value in the page and the painted frame ≤ 2 frames.
  - **E-bind-in:** every one of the 196 parameters set to a seeded random value from C++ → the page state's normalised value equals the host's within 1e-6, and for the 74 sliders the page's `getScaledValue()` equals the host's text value's number.
  - **E-bind-out:** the page sets each of the 196 through its state API (`setNormalisedValue`, `setValue`, `setChoiceIndex`) with begin / end gestures → the host parameter equals it; one gesture pair per set (counted by a listener in the bench).
  - **E-type:** typed-entry arithmetic: page writes 1000 Hz → host normalised 0.5 ± 1e-6 and text "1000 Hz"; 42.7 Hz → 0.25 ± 1e-6 (checklist § 6).
  - **E-disabled:** for each of the 8 types × 8 slopes on band 1 (selected): gain field disabled exactly for Low Cut / High Cut / Notch / Band Pass; slope select disabled exactly for Bell / Notch / Band Pass; Q disabled exactly at 6 dB/oct on slope-honouring types. 64 / 64.
  - **E-ui:** `getUiState` / `setUiState` round trip; invalid values refused; analyzerMode, displayRange, selectedBand absent from `getParameters()` and unchanged by (d).
  - **E-listen:** `setListenBand (5)` → `listenBand` 4; `(0)`, `(25)`, `(-3)` → −1; editor destroyed with listen on → −1.
  - **E-cycle:** 10 open / close cycles, Release ×5 processes and Debug ×1: no crash, no leak report, **0 assertions** in Debug (the `pageReady` gate).
  - **E-res:** every URL the page requests returns 200 with the right MIME (bench logs the resource provider), incl. `/css/eb-garamond.css`, three `/fonts/*.woff2`, both modules; `document.fonts.check ("16px 'EB Garamond'")` true and canvas axis text drawn after `fonts.load` (pixel probe: canvas glyph width of "1k" equals the EB Garamond width ± 0.5 px, not the fallback's).
  - **E-size:** opens at 960 × 600; `resizeEditor (700, 400)` → 800 × 500; `(2000, 2000)` → 1600 × 1000; reopen restores the last size.
- **On a failure:** fix the editor / page and re-run W5 from step 1 (Source changed). E-open > 500 ms: STOP with the figures (C1 assumed it cannot happen; the call is Taylor's).
- Depends on: T10.

### Wave 7 — bookkeeping + commit

**T12. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/3-gui/SUMMARY-3.1.md`: harness tallies (default, `--long`); the 11-mutant table; the five gate exit codes; validator results; every E-row with figures beside the research bench; plate check; native diff; deviations (every page edit beyond P1 with its reason).
- Carry-forward:
  - **For verify 3.1 (hands-on, Taylor):** opens at 960 × 600 in Logic (AU) and one VST3 host; Logic timing via the flag log, 5 cold opens, ≤ 500 ms; automating a band's freq / type moves the chip, node and strip; save a user preset, change things, load it back; output gain and bypass untouched by the load; language switch en / fr / zh-Hans at 800 × 500; plate look against the mockup; the § 9 items that need no curve.
  - **For 3.2:** `DisplayModel` owned by the editor, `eqSections` from the timer (gated by `pageIsReady`), full push on `requestEqSections` and after a preset load; the Debug 0-assertion row re-run with something emitted.
- `PLUGINS.md` version cell → 0.7.0 (check `git diff PLUGINS.md` first; other sessions' rows → temp-index recipe or leave out and say so).
- STATUS: `status: stage_3_phase_3_1_executed`, `next_action: stage_3_verify_3_1`.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` new files by name (`Source/PluginEditor.h/.cpp`, `Source/PresetSaveGuard.h`, `Source/ui/public/index.html`, `Source/ui/public/js/…`, `tests/ui-stub/*`, `tests/i18n-states.json`, `.planning/params.tsv`, `.planning/stages/3-gui/SUMMARY-3.1.md`, `execute-3.1/`), then `git commit -- plugins/O-ParametricEQ` (+ `PLUGINS.md` if clean). Never `-A` / `-a`. `git log -1 --stat` lists only these paths; no `Source/ui/public/modules/*.js`. No tag.
- Depends on: T11.

---

## Dependency graph

```
T1 ─┬→ T2 ─────────────┬→ T6 ─┐
    ├→ T3 ─┬→ T5 ──────┘      │
    ├→ T4 ─┤                  │
    │      └→ T7 ─────────────┤
    └→ T8 (after T2) ─────────┤
                              ↓
               T9 (build, harnesses, mutants, gates) → T10 (install, validators) → T11 (editor bench) → T12
                ↑  │                                                                  │
                └──┴──────────── fix → rebuild → rerun from T9 ←──────────────────────┘
```

T2, T3, T4 are independent. T7 and T8 touch files no other task does.

## Success criteria

| Criterion (ROADMAP 3.1 as amended; checklist §§ 0 – 8) | Proven by |
|---|---|
| Editor opens at the mockup size in VST3 and AU | T10 (pluginval opens it), E-size; Logic / VST3 host at verify |
| All 196 relays bound; open → painted ≤ 500 ms; idle traffic 0 | E-open, E-props, E-idle; Logic flag log at verify |
| Host automation moves the controls; the page's edits reach the host | E-bind-in, E-bind-out, E-type |
| Preset load refreshes every control within 2 frames, by every load path | E-preset (a – d) |
| Controls for inert parameters disabled per type / slope | E-disabled 64 / 64; `i18n-states` Low Cut state |
| Every native the page calls is registered | P3 grep diff 19 / 19 |
| Presets: real save / load; never `output_gain` / `bypass`; factory names refused | XR-pm (+ control), XR-pmd, XR-psg (+ control) |
| UI state not in presets or parameters; listen transient | E-ui, E-listen, XR-pm |
| No crash / leak / Debug assertion over 10 cycles | E-cycle |
| Resources, MIME, fonts | E-res; `boot-all-uis` |
| Plate as the mockup, token colours, no cost | P4 greps, plate-bench |
| Five UI gates | T9.5 |
| Owed rows (8 + DM2 / AN1 / PX22) | XR-ls10 … XR-an1; 11-mutant control |
| No DSP change | T9.3 memcmp; `Source/dsp`, `Source/analysis` diff empty; 150 / 0 / 18, 166 / 0 |
| 0.7.0 validates | T10 |

## Risks

- **R1 Template drift.** The templates were never compiled and assume six processor members that do not exist (Q2). Map, never add (P2); a missing piece is a deviation recorded in the summary, not a new processor member.
- **R2 Unregistered native = silent dead control.** Build, pluginval and auval all pass with one missing. The grep diff (P3) is the only gate; run it on the final files.
- **R3 Timer before page.** Without the `pageReady` gate, Debug asserts on `__JUCE__.backend`. In 3.1 nothing is emitted, so E-cycle's 0 is partly vacuous; 3.2 re-runs it with `eqSections` live (carry-forward).
- **R4 Preset apply is 196 host gestures** (validated apply). Fine in the bench; a host may log 196 undo steps. Note Logic's behaviour at verify; no change in 3.1.
- **R5 Fake-home leak.** The preset rows write files. The harness must resolve `userHomeDirectory` under `CFFIXED_USER_HOME` before the first write (memory: JUCE mac home ignores `HOME`); STOP if not.
- **R6 Configure-copied module JS.** `Source/ui/public/modules/*.js` exist untracked; the binary-data target takes the module paths. Grep gates over `Source/` exclude `modules/`; the commit must not add them.
- **R7 Gate fixtures with skew 1.** Without `generic-overrides.json` the stub maps typed frequencies wrongly and the label / typed-entry checks are vacuous. Generate, never hand-write.
- **R8 Plate double rotation.** `v1-ui.html:269` rotates the box; the SVG rotates inside. Remove the box's rotation (P4); plate-bench's bounding box catches a miss.
- **R9 Shared checkout.** Other sessions have uncommitted files (`modules/registry.yaml`, `.planning/STATE.md`, agent memory). Path-scope the commit and re-check immediately before it.
