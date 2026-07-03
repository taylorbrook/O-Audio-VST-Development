# Stage 3: GUI — Execution Plan

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 3 (GUI) — plan phase
**Date:** 2026-06-27
**Inputs:** CONTEXT.md (D1–D7), RESEARCH.md (14 §, phase map §13), parameter-spec.md (17 locked IDs), VizTap.h (Stage-2 data contract), PluginProcessor.{h,cpp} (current seam), harness CMakeLists (JUCE_WEB_BROWSER=0).

---

## Goal

Build the single-page WebView UI for O-simplePhysicalModelSynth: a left→right
signal-flow layout (Excitation → Resonator → Material → Amp/Output) with all **17
parameters bound** and **resonator-/exciter-aware grey-out**, the **four pedagogical
visuals** (animated loop/flow diagram, decay scope, live spectrum, modal stems),
on-hover **tooltips**, an **on-screen keyboard**, and a **preset-bar shell** — all
consuming the live viz taps wired in Stage 2.

This is a **near-verbatim port of the O-simpleFM WebView template** + **one new widget**
(the SVG loop/flow diagram). No new external research; integration/assembly against
proven in-house patterns. Risk is concentrated in two **silently-failing** seams (the
harness `#include` guard, the WebView native-fn bridge) and the new SVG diagram.

**Verifies:** UI-01 (layout), UI-02 (loop diagram), UI-03 (scope), UI-04 (spectrum),
UI-05 (modal stems), UI-06 (tooltips).

---

## Execution strategy — 3 component-gated phases

Execute in the order the RESEARCH phase map prescribes (§13). Each phase ends with a
**build + DAW smoke test** before the next begins; the harness is re-run at the START
of Stage 4 (not gated per-phase, since the editor is excluded from the harness TU).

- **Phase 3.1** — WebView foundation: CMake, editor seam, layout, 17-param binding,
  grey-out, preset shell, keyboard. *Gate:* plugin loads, all 17 controls move the DSP,
  grey-out tracks engine, keyboard plays, preset bar shells (save/load/navigate).
- **Phase 3.2** — Scope + spectrum: `PmVizAnalyzer.h` + 30 Hz timer emit + canvas draw.
  *Gate:* scope shows decay, spectrum shows harmonic comb (String) vs inharmonic (Modal).
- **Phase 3.3** — Loop diagram + stems + tooltips: SVG diagram (String skin + Modal stem
  skin) driven by `loopEnergy`/stems, tooltips on every control. *Gate:* diagram dampens
  with the audible note, Modal skin shows live stems, every control has a tooltip.

---

## Tasks

### Phase 3.1 — WebView foundation + binding + preset shell + keyboard

**1. [ ] Processor seam: guard `createEditor` + `#include "PluginEditor.h"` under `#if JUCE_WEB_BROWSER`** ⚠ SILENT FOOTGUN
   - Replace the inlined `createEditor()` in `PluginProcessor.h` with a **declaration** only
     (`juce::AudioProcessorEditor* createEditor() override;`).
   - In `PluginProcessor.cpp`: `#if JUCE_WEB_BROWSER` → `#include "PluginEditor.h"` `#endif`;
     define `createEditor()` returning `new ...AudioProcessorEditor(*this)` under
     `JUCE_WEB_BROWSER`, else `new juce::GenericAudioProcessorEditor(*this)` (harness build).
   - **DO NOT** copy O-simpleFM's unconditional `#include "PluginEditor.h"` (O-simpleFM:13) — that
     drags WebView types into the harness TU under `JUCE_WEB_BROWSER=0` → un-buildable.
   - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
   - Depends on: none
   - Ref: RESEARCH §11; harness CMake confirms `JUCE_WEB_BROWSER=0` + no `PluginEditor.cpp`.

**2. [ ] CMake: add the single binary-data target + preset-manager module**
   - `ouaricon_add_module(O-simplePhysicalModelSynth preset-manager)` **before** `juce_add_binary_data`
     (the helper copies `preset-manager.js` into the embed dir).
   - `juce_add_binary_data(O-simplePhysicalModelSynth_UIResources SOURCES ...)` listing the 6 UI files
     (index.html, css/styles.css, js/app.js, js/juce/index.js, js/juce/check_native_interop.js,
     modules/preset-manager.js). Default `BinaryData` namespace OK (no samples embedded).
   - Link target **FIRST** in PRIVATE; keep `juce_generate_juce_header(...)` **AFTER** `target_link_libraries`.
   - WebView flags already set in Stage 1 — verify, do not re-add.
   - Files: `CMakeLists.txt`
   - Depends on: none (parallel to Task 1)
   - Ref: RESEARCH §4.1, §8; memory: dual_binary_data_namespace_collision (N/A here — single target).

**3. [ ] Port the WebView UI scaffold (HTML/CSS/JS + juce glue)**
   - Copy from O-simpleFM/O-simpleAdditive into `Source/ui/public/`:
     `index.html`, `css/styles.css`, `js/app.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`.
   - **Re-author top-level layout** vertical→**left→right** (4 signal-flow columns + central viz row +
     header/preset bar + keyboard). Reuse naturalist `:root` palette + `.frame/.group/.knob-row/.knob`
     building blocks verbatim; port the DPR-aware `makeCanvas`, knob component (CSS disc + JS drag),
     and combo binding verbatim.
   - Files: NEW `Source/ui/public/{index.html, css/styles.css, js/app.js, js/juce/index.js, js/juce/check_native_interop.js}`
   - Depends on: none (authored in parallel; wired by Task 5)
   - Ref: RESEARCH §2 (reuse map), §3 Q1 (1040×860 + keyboard), §13.

**4. [ ] Editor C++: relays → WebView → attachments, all 17 params**
   - NEW `PluginEditor.{h,cpp}`. **Member order load-bearing** (reverse-destruction): relays first →
     `webView` → attachments last → `PmVizAnalyzer vizAnalyzer` (Task 8) → `std::unique_ptr<FileChooser> fileChooser`.
   - Split: **3 combos** (`excitationType`, `resonatorType`, `stringModel`) via `WebComboBoxRelay`/`...Attachment`;
     **14 sliders** (the rest, incl. `coarseTune` as stepped int) via `WebSliderRelay`/`...Attachment`. **0 toggles.**
   - Ctor order: build relays → `Options{}.withNativeIntegrationEnabled().withResourceProvider(...)` +
     `for(relay) options = options.withOptionsFrom(*relay)` → register native fns (Tasks 6,7,10) →
     `#if JUCE_WINDOWS withWinWebView2Options().withUserDataFolder(tempDir/"OsimplePMS_WebView")` →
     construct `webView` → build attachments (3-arg `(*param,*relay,nullptr)`, `jassert(param)` each) →
     `addAndMakeVisible(*webView); goToURL(getResourceProviderRoot()); setSize(1040,860); startTimerHz(30);`.
   - Relay/attachment ID strings **== APVTS IDs == JS `KNOB_IDS`/`COMBO_IDS`** exactly (drift = dead control).
   - Files: NEW `Source/PluginEditor.{h,cpp}`
   - Depends on: Tasks 1, 2
   - Ref: RESEARCH §4.2, §5.1; memory: critical_juce_webview_namespace_vs_postmessage.

**5. [ ] Resource provider (bare-path matcher) + JS param binding**
   - C++ `getResource()`: compare **bare paths** with `==` (never strip scheme/host); `charset=utf-8` on
     every text resource; BinaryData symbol mangling (`presetmanager_js`, `index_js`, etc.). Return
     `std::nullopt` on miss.
   - JS: bind sliders via `Juce.getSliderState(id)` (normalised value + `sliderDragStarted/Ended`), combos via
     `Juce.getComboBoxState(id)` (`getChoiceIndex/setChoiceIndex`). Define `KNOB_IDS` (14) + `COMBO_IDS` (3).
   - **Material macro co-move is automatic** — `parameterChanged()` already writes `damping`+`decay` via
     `setValueNotifyingHost()` (PluginProcessor.cpp:202-220), so the two attachments track with zero extra JS.
     Verify by dragging Material and watching both knobs.
   - Files: `Source/PluginEditor.cpp` (getResource), `Source/ui/public/js/app.js`
   - Depends on: Tasks 3, 4
   - Ref: RESEARCH §4.3, §5.2; memory: resource-provider-paths.

**6. [ ] Resonator-/exciter-aware grey-out (D5)**
   - CSS: `.knob-cell.pm-disabled { opacity:.38; pointer-events:none; transition:opacity .2s; filter:grayscale(30%); }`
     (adapt O-simpleGrain `.env-bypassed`).
   - JS `applyEngineGating()`: read `getComboBoxState("resonatorType")/("excitationType").getChoiceIndex()`;
     `setDisabled("stringModel", res!==0)`, `inharmonicity`/`modeBrightness` (`res!==1`), `bowForce` (`exc!==2`).
     Wire `valueChangedEvent.addListener` on both selectors + call once at boot.
   - **Keep the choice selectors OUTSIDE any dimmed group** (pointer-events escape hatch — O-Polystutter v1.0.2).
   - `stringModel` "Waveguide" option: leave selectable with a tooltip "Waveguide — coming in v1.1" (KS-only shipped).
   - Files: `Source/ui/public/js/app.js`, `Source/ui/public/css/styles.css`
   - Depends on: Task 5
   - Ref: RESEARCH §5.3; memory: feedback_module_extraction_regression_check (grep-diff helper refs).

**7. [ ] Preset-bar shell (D4) — C++ backend + 10 native fns + JS bar**
   - Processor: add `OuariconPresetManager presetManager;` member, init `presetManager(parameters, "O-simplePhysicalModelSynth")`
     in ctor list; **switch state I/O** from plain APVTS XML to `presetManager.getStateAsXml()/setStateFromXml()`
     (replaces current `getStateInformation/setStateInformation` bodies). `initializeFactoryPresets({...})` stays
     Default-only until Stage 4.
   - Editor: register all **10** native fns (`savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile,
     getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset, deletePreset, isFactoryPreset`).
     `savePresetWithDialog`/`loadPresetFromFile` are editor-side (held `FileChooser`, return `{success,name}`);
     `selectNext/PreviousPreset` → C++ `getNextPreset()/getPreviousPreset()` (return name, JS then loads it).
   - JS: `new PresetManager({ ..., getNativeFunction: Juce.getNativeFunction })` — **`Juce.getNativeFunction`,
     NOT `window.__JUCE__`** (namespace gotcha → silent dead bar).
   - Storage: `~/Library/O-simplePhysicalModelSynth/Presets/{Factory,User}/`.
   - Files: `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `Source/ui/public/js/app.js`,
     `Source/ui/public/modules/preset-manager.js` (helper-copied), `CMakeLists.txt`
   - Depends on: Tasks 2, 4
   - Ref: RESEARCH §8; memory: critical_juce_webview_namespace_vs_postmessage, pattern_webview_native_fn_bridge_gap.

**8. [ ] On-screen keyboard + processor MIDI plumbing**
   - Processor (NEW plumbing — absent in Stage 1): `void handleUiMidi(int note,bool on,float vel)` building
     `MidiMessage::noteOn/Off` with **timestamp in seconds** (`Time::getMillisecondCounterHiRes()*0.001`) into a
     `juce::MidiMessageCollector midiCollector;` member; `midiCollector.reset(sampleRate)` in `prepareToPlay`;
     drain at the **TOP** of `processBlock`: `midiCollector.removeNextBlockOfMessages(midiMessages, n)` before
     `readParams()/renderNextBlock`. `MidiMessageCollector` is not a WebView type → harness stays clean.
   - Editor: `uiMidi` native fn → `processorRef.handleUiMidi((int)args[0],(bool)args[1], args.size()>=3?(float)args[2]:0.8f)`.
   - JS: port O-simpleFM keyboard (C3–C5, QWERTY map, mouse + computer-key) → `uiMidi`.
   - Files: `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `Source/ui/public/js/app.js`, `index.html`
   - Depends on: Tasks 4, 5
   - Ref: RESEARCH §10; memory: pattern_webview_native_fn_bridge_gap (gate `handleUiMidi` reaches DSP).

**GATE 3.1** — Build VST3+AU clean (cache-clear + dual-variant sweep per CLAUDE.md). DAW smoke:
plugin loads (no blank page), all 17 controls move the DSP, Material co-moves Damping+Decay, grey-out
tracks `resonatorType`/`excitationType`, keyboard plays, preset bar saves/loads/navigates. `jassert(param)`
clean in Debug. **grep-diff** JS `getNativeFunction(...)` names vs C++ `withNativeFunction(...)` names — all match.

---

### Phase 3.2 — Scope + spectrum (UI-03, UI-04)

**9. [ ] `PmVizAnalyzer.h` — message-thread FFT + scope (omit VizRing)**
   - NEW slim analyzer holding only state (NOT FmVizAnalyzer's `VizRing` — reuse `VizTap`'s). Constants:
     `kFftOrder=12` (4096), Blackman-Harris window, `kSpectrumBins=256` (log 20 Hz→Nyquist), `kScopeWindow=1024`,
     `kScopePoints=128`, asymmetric smoothing (0.5 rising / 0.1 falling).
   - `process(const VizRing& ring, double sr)`: **Scope FIRST** (FFT clobbers the work buffer) — `readLatest(scopeRaw,1024)`
     → 128 max-abs (sign-kept) points clamped ±1. Then **Spectrum** — `readLatest(work,4096)` → window →
     `performFrequencyOnlyForwardTransform` → 256 log bins, `mag/4096`, `gainToDecibels(mag+1e-9,-100)`, per-bin smoothing.
   - Files: NEW `Source/PmVizAnalyzer.h`
   - Depends on: GATE 3.1
   - Ref: RESEARCH §6.1; reuse `VizTap::VizRing` verbatim (do NOT re-paste).

**10. [ ] Editor `timerCallback()` (30 Hz) emits spectrum + scope**
   - `vizAnalyzer.process(processorRef.getVizTap().waveform, processorRef.getCurrentSampleRate())`; guard
     `if (webView == nullptr) return;`. Pack spectrum (256) + scope (128) into `juce::Array<juce::var>` →
     `webView->emitEventIfBrowserIsVisible("spectrumUpdate"|"scopeUpdate", ...)`.
   - `getCurrentSampleRate()` exists on `AudioProcessor` — no new processor method. (Optional `getSampleRate`
     native fn only if JS wants axis labels.)
   - Files: `Source/PluginEditor.cpp`
   - Depends on: Task 9
   - Ref: RESEARCH §6.2.

**11. [ ] JS `drawSpectrum` + `drawScope` on DPR-aware canvases**
   - Two `<canvas>` in the viz row via `makeCanvas` (DPR backing store: `canvas.width=clientWidth*dpr; ctx.setTransform(dpr,...)`;
     CSS `width/height:100%` inside a positioned `overflow:hidden` wrap — NOT `right/bottom`).
   - `window.__JUCE__.backend.addEventListener("spectrumUpdate", ...)` → 256 bars log axis; `"scopeUpdate"` → 128-pt polyline.
     (Incoming events use `window.__JUCE__.backend` — correct here; `Juce.*` is for native-fn calls.)
   - Files: `Source/ui/public/js/app.js`, `index.html`, `css/styles.css`
   - Depends on: Task 10
   - Ref: RESEARCH §6.2; memory: Canvas-replaced-element gotcha.

**GATE 3.2** — Build clean. DAW: scope shows the waveform decaying after note-off; spectrum shows a **harmonic
comb** in String mode and **inharmonic spacing** in Modal (log axis renders `f_k=f0·k·√(1+B·k²)` unevenly); harmonics
visibly fade top-down on decay. No audio-thread FFT (analyzer is message-thread only).

---

### Phase 3.3 — Loop/flow diagram + modal stems + tooltips (UI-02, UI-05, UI-06)

**12. [ ] Inline SVG loop/flow diagram — String skin (UI-02)**
   - `<svg>` in `index.html`: the BRIEF's block diagram `[EXCITATION]→[RESONATOR ⟲]→[MATERIAL]→[OUT]`. A pulse element
     travels the loop arrow; **opacity/glow driven by `loopEnergy`** so it visibly dims each pass as the note decays.
     Animate by attribute/CSS updates each 30 Hz tick (no canvas — sidesteps the replaced-element gotcha).
   - Files: `Source/ui/public/index.html`, `css/styles.css`, `js/app.js`
   - Depends on: GATE 3.2
   - Ref: RESEARCH §7, §3 Q2.

**13. [ ] Editor emits `loopUpdate` (energy + 8 stems)**
   - In `timerCallback()`: build a `DynamicObject` with `energy = getVizTap().getLoopEnergy()` + `stemFreqs`/`stemAmps`
     arrays (`getStemFreq(k)/getStemAmp(k)`, k=0…7) → `emitEventIfBrowserIsVisible("loopUpdate", var(o))`. JS reads the
     current resonator from `getComboBoxState("resonatorType")` (no need to send it).
   - Files: `Source/PluginEditor.cpp`
   - Depends on: Task 12
   - Ref: RESEARCH §7 (VizTap::kStems=8; loopEnergy 0…~1 envelope follower).

**14. [ ] Modal stem skin (UI-05) — fold stems INTO the diagram, one widget**
   - On `resonatorType == Modal`, the RESONATOR box interior **replaces** the KS loop with **8 vertical stems**:
     `x = log-map(stemFreq[k])`, `height = stemAmp[k]`, decaying live. This IS the stem display (no separate panel,
     no duplicate data — Q4). String↔Modal skin toggled on the `resonatorType` listener.
   - Files: `Source/ui/public/js/app.js`, `index.html`, `css/styles.css`
   - Depends on: Tasks 12, 13
   - Ref: RESEARCH §7, §3 Q4.

**15. [ ] Tooltip system (UI-06) — on every control + diagram boxes**
   - Port O-simpleAdditive verbatim: single floating `#tooltip` div + `data-tip="key"` on every control cell +
     `TIPS` map (`key→[title,body]`). `setupTooltips()` wires `pointerenter/move/leave/down` + `focusin/out` + Escape +
     viewport-edge flipping. **Avoid native `title=`.** Author plain-language `TIPS` content from the BRIEF
     (pitch = SR ÷ delay length; feedback near one sustains; higher modes decay faster; what inharmonicity does).
   - Files: `Source/ui/public/js/app.js`, `css/styles.css`, `index.html`
   - Depends on: Task 14
   - Ref: RESEARCH §9, §7 (diagram-box teaching text).

**GATE 3.3** — Build clean. DAW: loop diagram pulse dims in lockstep with the audible decay (real `loopEnergy`);
Modal mode swaps to live stems (correct freq spacing + decaying heights); every control + diagram box shows an
on-hover tooltip; no console errors.

---

## Files summary

| File | Phase | New/Edit |
|------|-------|----------|
| `CMakeLists.txt` | 3.1 | EDIT (binary-data target + `ouaricon_add_module`) |
| `Source/PluginProcessor.h` | 3.1 | EDIT (createEditor decl, presetManager member, handleUiMidi, midiCollector) |
| `Source/PluginProcessor.cpp` | 3.1 | EDIT (`#if JUCE_WEB_BROWSER` include+branch, state I/O swap, handleUiMidi, processBlock drain) |
| `Source/PluginEditor.{h,cpp}` | 3.1 | **NEW** (relays/webView/attachments, resource provider, 10 preset + uiMidi native fns) |
| `Source/ui/public/index.html` | 3.1/3.2/3.3 | **NEW** then EDIT (layout, canvases, SVG diagram, tooltips) |
| `Source/ui/public/css/styles.css` | 3.1/3.2/3.3 | **NEW** then EDIT (layout, grey-out, canvas, diagram, tooltip CSS) |
| `Source/ui/public/js/app.js` | 3.1/3.2/3.3 | **NEW** then EDIT (binding, grey-out, preset, keyboard, draw*, loop/stem, tooltips) |
| `Source/ui/public/js/juce/{index.js, check_native_interop.js}` | 3.1 | **NEW** (port verbatim) |
| `Source/ui/public/modules/preset-manager.js` | 3.1 | **NEW** (module-helper-copied) |
| `Source/PmVizAnalyzer.h` | 3.2 | **NEW** (message-thread FFT + scope, reuses VizTap::VizRing) |

---

## Success Criteria (stage-level — verify phase will check)

- [ ] **UI-01** — single page, projector-readable, left→right signal flow; window 1040×860 fixed.
- [ ] **All 17 params bound** — every control moves its APVTS param (3 combos + 14 sliders); IDs zero-drift; `jassert(param)` clean.
- [ ] **Material macro co-moves** — dragging Material visibly moves Damping + Decay knobs (no extra JS).
- [ ] **Grey-out (D5)** — `stringModel` greys in Modal; `inharmonicity`/`modeBrightness` grey in String; `bowForce` greys unless Bow; selectors never trapped.
- [ ] **UI-02** — loop/flow diagram pulse dims in lockstep with the audible decay (driven by real `loopEnergy`).
- [ ] **UI-03** — scope shows live waveform + visible decay after note-off.
- [ ] **UI-04** — spectrum shows harmonic comb (String) vs inharmonic spacing (Modal); harmonics fade top-down.
- [ ] **UI-05** — Modal mode shows 8 live stems (freq spacing + decaying heights) in the diagram's Modal skin.
- [ ] **UI-06** — on-hover plain-language tooltip on every control + diagram box.
- [ ] **Preset bar** — save/load/navigate/delete work; factory-preset population deferred to Stage 4.
- [ ] **Keyboard** — on-screen keys + QWERTY play notes (mouse + computer-key); `handleUiMidi` reaches the DSP.
- [ ] **Harness seam intact** — `createEditor`/`#include "PluginEditor.h"` guarded under `#if JUCE_WEB_BROWSER`; harness still builds under `JUCE_WEB_BROWSER=0` (re-run at START of Stage 4).
- [ ] **Build** — VST3 + AU clean (0 warnings); pluginval strictness-10 SUCCESS; auval SUCCEEDED; no console errors in the WebView.

---

## Risk register / gotchas (from RESEARCH §12 — carry into execute)

1. **Harness `#include` guard** (Task 1) — the single most likely Stage-3→Stage-4 regression. Diverge from O-simpleFM's unconditional include.
2. **`Juce` vs `window.__JUCE__`** — `Juce.getNativeFunction` into `PresetManager` (native-fn calls); `window.__JUCE__.backend.addEventListener` for incoming viz events. Mixing them = silent failure.
3. **Native-fn bridge gaps fail silently** — grep-diff all 10 preset fns + `uiMidi` JS↔C++ at GATE 3.1.
4. **Editor member order** — relays → webView → attachments (reverse-destruction crash otherwise).
5. **Param-ID drift** — relay/attachment strings == APVTS IDs == JS `KNOB_IDS`/`COMBO_IDS`; `jassert(param)` per attachment.
6. **Resource provider = bare paths**, `==` match, `charset=utf-8`; BinaryData symbol mangling.
7. **Canvas replaced-element** — DPR backing store + CSS `width/height:100%` (spectrum/scope only; SVG diagram unaffected).
8. **Windows WebView2** — `withUserDataFolder(tempDir)` (sandbox denial → blank/IE fallback).
9. **Grey-out escape hatch** — selectors outside any `pointer-events:none` group.
10. **Module helper ordering** — `ouaricon_add_module` before `juce_add_binary_data`.
11. **Single binary-data target** → `BinaryData` namespace OK (no samples embedded).

---

## Dependency graph

```
1 (seam) ─┐
2 (cmake)─┼─→ 4 (editor) ─→ 5 (resource+bind) ─→ 6 (grey-out)
          │              └─→ 7 (preset shell)
3 (ui)  ──┘              └─→ 8 (keyboard)
                                   │
                            GATE 3.1
                                   │
                    9 (analyzer) ─→ 10 (timer emit) ─→ 11 (draw)
                                   │
                            GATE 3.2
                                   │
        12 (SVG String skin) ─→ 13 (loopUpdate emit) ─→ 14 (Modal stem skin) ─→ 15 (tooltips)
                                   │
                            GATE 3.3 → Stage 4 (re-run harness FIRST)
```
