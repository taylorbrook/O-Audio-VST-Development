# Stage 3 (GUI) Integration Checklist — O-Contrabass mockup v1

**Plugin:** O-Contrabass (`OContrabassAudioProcessor`, target `O-Contrabass`)
**Mockup version:** v1 (finalized 2026-07-11, Phase 5.5)
**Window:** fixed 1000×650, non-resizable
**Parameters:** 31 (parameter-spec.md authoritative — 29 sliders + 1 combo + 1 toggle)
**Generated:** 2026-07-10 by ui-finalization-agent

---

## 0. Render-harness protection (DO FIRST — silent-failure class)

The Stage-2 render harness compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0`.
The moment the editor gains WebView types, the harness goes un-buildable —
undetected until it is next rebuilt.

- [ ] Guard `createEditor()` in `Source/PluginProcessor.cpp`:
  ```cpp
  juce::AudioProcessorEditor* OContrabassAudioProcessor::createEditor()
  {
  #if JUCE_WEB_BROWSER
      return new OContrabassAudioProcessorEditor(*this);
  #else
      return nullptr;   // render-harness build — headless
  #endif
  }
  ```
- [ ] Remove `Source/PluginEditor.cpp` from `tests/render-harness/CMakeLists.txt` sources
- [ ] Rebuild harness with `-DOUARICON_BUILD_TESTS=ON`
- [ ] **Re-run the 19-entry `reproduce-goldens.sh` battery at Stage 3 execute START**
      (all byte-identical — Phase 2.2 strict bar)

## 1. Copy UI files (Phase 3.1)

- [ ] `mkdir -p Source/ui/public/js/juce`
- [ ] Copy `.planning/mockups/v1-ui.html` → `Source/ui/public/index.html`
- [ ] Copy JUCE 8.0.9 frontend library:
      `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/index.js`
      → `Source/ui/public/js/juce/index.js`
      (or from `plugins/O-GrainScatter/Source/ui/public/js/juce/` — same 8.0.9)
- [ ] Copy `check_native_interop.js` (same origin) → `Source/ui/public/js/juce/`
- [ ] Verify the HTML import path matches: `import * as Juce from "./js/juce/index.js"`

## 2. Update PluginEditor files

- [ ] Replace `Source/PluginEditor.h` with `v1-PluginEditor.h` content
- [ ] Replace `Source/PluginEditor.cpp` with `v1-PluginEditor.cpp` content
- [ ] **Verify member order: relays (31) → webView → attachments (31)** —
      members destroy in reverse order; attachments call `evaluateJavascript()`
      in their destructors and MUST die before the WebView (release-build
      reload crash otherwise)
- [ ] Verify initialization order in .cpp matches declaration order in .h
- [ ] Verify all 31 relay IDs match APVTS IDs exactly (case-sensitive):
      grep-diff `WebSliderRelay>\("` IDs vs `createParameterLayout()` in
      `PluginProcessor.cpp`

## 3. Update CMakeLists.txt

- [ ] Merge `v1-CMakeLists.txt` block: `juce_add_binary_data(OContrabass_UIResources ...)`
      + `target_link_libraries(O-Contrabass PRIVATE OContrabass_UIResources)`
- [ ] Confirm already present (do NOT duplicate): `NEEDS_WEB_BROWSER TRUE`,
      `NEEDS_WEBVIEW2 TRUE`, `juce::juce_gui_extra`, `JUCE_WEB_BROWSER=1`,
      `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`
- [ ] Only ONE `juce_add_binary_data` target uses the default `BinaryData`
      namespace (dual-target namespace collision gotcha)

## 4. Native function grep-diff gate (silent-failure class)

Every JS `getNativeFunction` name MUST have a matching C++ `withNativeFunction`
registration. An unregistered fn passes build/auval but the control is dead.

| JS (`Source/ui/public/index.html`)             | C++ (`PluginEditor.cpp`)                          | Status |
|------------------------------------------------|---------------------------------------------------|--------|
| `Juce.getNativeFunction("getParameterDefaults")` | `.withNativeFunction("getParameterDefaults", ...)` | ✅ both scaffolded |
| `Juce.getNativeFunction("openTuningFilePicker")` | `.withNativeFunction("openTuningFilePicker", ...)` | ✅ both scaffolded |

- [ ] Run the gate after any UI change:
  ```bash
  diff <(grep -o 'getNativeFunction("[^"]*"' Source/ui/public/index.html | sort -u) \
       <(grep -o 'withNativeFunction(\s*juce::Identifier("[^"]*"' Source/PluginEditor.cpp \
         | grep -o '"[^"]*"' | sed 's/^/getNativeFunction(/' | sort -u)
  ```
  (any asymmetry = dead control or dead registration)

## 5. Resource provider path audit

Provider receives **bare paths** — direct equality checks only.

- [ ] `/` and `/index.html` → `text/html`
- [ ] `/js/juce/index.js` → `application/javascript` (wrong MIME = silent module-import failure)
- [ ] `/js/juce/check_native_interop.js` → `application/javascript`
- [ ] Any file added later (CSS/images/fonts/whale engraving asset) gets a
      provider entry + binary-data source — no 404s in the WebView console

## 6. Build and test — Debug

- [ ] `ninja O-Contrabass_VST3 O-Contrabass_AU` clean
- [ ] Install per CLAUDE.md cache-clear sequence (`./scripts/build-and-install.sh O-Contrabass`)
- [ ] Standalone loads WebView (not blank), all 7 sections render at 1000×650
- [ ] Right-click → Inspect: console clean, `window.__JUCE__` exists
- [ ] No stray scrollbars; no viewport-unit regressions (grep index.html for `vh`/`vw`)

## 7. Build and test — Release (member-order gate)

- [ ] Release build succeeds
- [ ] **Open/close plugin editor 10× in Logic — no crash, no freeze**
      (this is the relays→webView→attachments destruction-order test;
      Debug builds hide the UAF)

## 8. Parameter binding validation (31 params)

- [ ] All 24 knobs drag + sync UI ↔ APVTS (incl. mini REFERENCE_PITCH, featured EXPRESSION_MACRO)
- [ ] 4 detune fine-tuners: drag, ±25¢ soft center detent, wheel = 10¢ (shift = 1¢)
- [ ] ACTIVE_STRINGS segmented stepper: 4 buttons map to Int 1–4
- [ ] TUNING_SYSTEM dropdown: 3 choices, default 12-TET; `Load .scl/.tun` button
      enabled ONLY at Scala/TUN
- [ ] NOTE_EXPRESSION toggle syncs (default ON)
- [ ] **Knob readouts use `getScaledValue()`** — spot-check the 6 log-curve params
      (BOW_SPEED, BOW_PRESSURE, BRIGHTNESS, VIBRATO_RATE, SLOW_LFO_RATE,
      REFERENCE_PITCH) read identical values in UI and DAW generic view
      (skew drift = the O-MicrotonalSampler 2×-wrong-for-20-versions bug)
- [ ] Dblclick reset on every control returns to parameter-spec.md defaults
      (via `getParameterDefaults` — e.g. VIBRATO_DEPTH → 0¢, EXPRESSION_MACRO → 0%,
      LIMITER_CEILING_DB → −0.3 dB)
- [ ] Host automation writes update the UI; preset/state recall updates the UI
- [ ] Drone panel green "awake" glow when INFINITE_SUSTAIN or SUB_HARMONICS > 0

## 9. Scala/TUN file picker (SafePointer contract)

- [ ] Picker opens from the microtonal strip button; `.scl` load calls
      `processorRef.loadScalaFile()` and retunes
- [ ] TODO(Stage 3 research): `.tun` route (extend loadScalaFile or sibling fn)
- [ ] Cancel path completes with `false` (editor alive) — no hang in JS promise
- [ ] **UAF test:** open picker, close the plugin window, then cancel/choose —
      no crash (bare-return-on-null-SafePointer; `complete()` must NOT be
      called when the editor is gone)

## 10. Visualizations (throttled feeds)

- [ ] Schelleng wedge: dot eases to (BOW_SPEED, BOW_PRESSURE), wedge shifts with β,
      jitter ∝ BOW_NOISE, green in-wedge / rust outside — rAF-driven
- [ ] Body spectrum: 8 modes respond to SIZE/DAMPING/MIX/BRIGHTNESS — dirty-flag redraw
- [ ] VU meter: needle follows `vuLevel` events; C++ emit site scaffolded in
      `timerCallback()` at 30 Hz (16 Hz sufficient) with `emitEventIfBrowserIsVisible`
- [ ] **TODO(Stage 3 research):** wire `vuLevel` to real post-limiter RMS
      (processor `std::atomic<float>` published from processBlock; placeholder
      currently emits −20 dB)
- [ ] TODO(Stage 3 research, optional): `bowState` / `bodyModes` feeds if viz
      should track post-macro/LFO DSP truth instead of raw params
- [ ] Editor hidden ⇒ no emit churn (`webView->isShowing()` early-out)

## 11. Full regression bar (Stage 3 exit)

- [ ] 19-entry `reproduce-goldens.sh` — all byte-identical (GUI must not touch DSP)
- [ ] `auval -a | grep -i contrabass` → SUCCEEDED
- [ ] pluginval level 10 SUCCESS
- [ ] Logic smoke: instantiate, play E1 drone, automate BOW_SPEED, reload project

## Deferred / assets

- Whale engraving: inline SVG placeholder ships in v1; final production artwork
  is an asset TODO (`v1-ui.yaml` assets_todo)
- Preset bar (prev/next/name/save) is a visual affordance in v1.0 — wire to
  OuariconPresetManager only if Stage 3 CONTEXT puts preset UX in scope
  (if so: `applyPresetJson` must reset-to-defaults first; no "/" in preset names)

## Parameter → relay map (authoritative, 31)

| # | Param ID | Type | Relay | Attachment |
|---|----------|------|-------|------------|
| 1 | BOW_SPEED | Float log | WebSliderRelay | WebSliderParameterAttachment |
| 2 | BOW_PRESSURE | Float log | WebSliderRelay | WebSliderParameterAttachment |
| 3 | BOW_POSITION | Float | WebSliderRelay | WebSliderParameterAttachment |
| 4 | ROSIN | Float | WebSliderRelay | WebSliderParameterAttachment |
| 5 | BOW_NOISE | Float | WebSliderRelay | WebSliderParameterAttachment |
| 6 | BODY_SIZE | Float | WebSliderRelay | WebSliderParameterAttachment |
| 7 | BODY_DAMPING | Float | WebSliderRelay | WebSliderParameterAttachment |
| 8 | BODY_MIX | Float | WebSliderRelay | WebSliderParameterAttachment |
| 9 | BRIGHTNESS | Float log | WebSliderRelay | WebSliderParameterAttachment |
| 10 | STRING_TENSION | Float | WebSliderRelay | WebSliderParameterAttachment |
| 11 | STRING_STIFFNESS | Float | WebSliderRelay | WebSliderParameterAttachment |
| 12 | ACTIVE_STRINGS | Int 1–4 | WebSliderRelay | WebSliderParameterAttachment |
| 13 | DETUNE_E | Float | WebSliderRelay | WebSliderParameterAttachment |
| 14 | DETUNE_A | Float | WebSliderRelay | WebSliderParameterAttachment |
| 15 | DETUNE_D | Float | WebSliderRelay | WebSliderParameterAttachment |
| 16 | DETUNE_G | Float | WebSliderRelay | WebSliderParameterAttachment |
| 17 | EXPRESSION_MACRO | Float | WebSliderRelay | WebSliderParameterAttachment |
| 18 | VIBRATO_RATE | Float log | WebSliderRelay | WebSliderParameterAttachment |
| 19 | VIBRATO_DEPTH | Float | WebSliderRelay | WebSliderParameterAttachment |
| 20 | VIBRATO_ONSET | Float | WebSliderRelay | WebSliderParameterAttachment |
| 21 | SLOW_LFO_RATE | Float log | WebSliderRelay | WebSliderParameterAttachment |
| 22 | SLOW_LFO_DEPTH | Float | WebSliderRelay | WebSliderParameterAttachment |
| 23 | INFINITE_SUSTAIN | Float | WebSliderRelay | WebSliderParameterAttachment |
| 24 | SUB_HARMONICS | Float | WebSliderRelay | WebSliderParameterAttachment |
| 25 | OUTPUT_GAIN | Float | WebSliderRelay | WebSliderParameterAttachment |
| 26 | WIDTH | Float | WebSliderRelay | WebSliderParameterAttachment |
| 27 | MASTER_SAT_AMOUNT | Float | WebSliderRelay | WebSliderParameterAttachment |
| 28 | LIMITER_CEILING_DB | Float | WebSliderRelay | WebSliderParameterAttachment |
| 29 | REFERENCE_PITCH | Float log | WebSliderRelay | WebSliderParameterAttachment |
| 30 | TUNING_SYSTEM | Choice ×3 | WebComboBoxRelay | WebComboBoxParameterAttachment |
| 31 | NOTE_EXPRESSION | Bool | WebToggleButtonRelay | WebToggleButtonParameterAttachment |
